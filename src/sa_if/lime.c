#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <lime/LimeSuite.h>
#include <math.h>

#include "lime.h"
#include "../common/timing.h"

lime_fft_buffer_t lime_fft_buffer;

//typedef struct
//{
//  int val;
//  int64_t timestamp;
//  bool valid;
//} pixelvt_t;

extern bool NewBW;
extern bool NewGain;
extern bool NewSpan;
extern bool NewCal;
extern float gain;
extern bool LimeStarted;
extern int freq_power;
extern bool peak;
uint64_t volatile sample_read_time;
extern uint64_t  sample_read_time_was;
extern uint64_t  smoothed_sample_read_time;
uint64_t volatile smoothed_sample_read_time_was;
uint64_t volatile smoothed_interval;
uint64_t volatile interval;
int64_t volatile delta;
int64_t volatile deltaSRT;


extern uint32_t span;                 // Span in integer MHz
extern uint32_t filter_bandwidth;     // Bandwidth in integer Hz
extern float SDRsamplerate;           // Hz
extern uint32_t tperdiv;              // Scan time per division in mS (1 - 5000)
extern bool scanstart;
extern pixelvt_t pixelbuff[1023];

double frequency_actual_rx = 323000000;
extern bool buffer1_updated;
extern bool buffer2_updated;

//Device structure, should be initialize to NULL
static lms_device_t* device = NULL;

int LimeTemp()
{
  double Temperature;

  LMS_GetChipTemperature(device, 0, &Temperature);
  //printf(" - Temperature: %.0f°C\n", Temperature);
  return (int)(Temperature);
}

void *lime_thread(void *arg)
{
  bool *exit_requested = (bool *)arg;

  // Find devices
  int n;
  lms_info_str_t list[8]; //should be large enough to hold all detected devices
  if ((n = LMS_GetDeviceList(list)) < 0) //NULL can be passed to only get number of devices
  {
    return NULL;
  }

  if (n < 1)
  {
    fprintf(stderr, "Error: No LimeSDR device found, aborting.\n");
    return NULL;
  }

  if(n > 1)
  {
     printf("Warning: Multiple LimeSDR devices found (%d devices)\n", n);
  }

  // open the first device
  if (LMS_Open(&device, list[0], NULL))
  {
    return NULL;
  }

  // Query and display the device details
  const lms_dev_info_t *device_info;
  double Temperature;

  device_info = LMS_GetDeviceInfo(device);

  printf("%s Serial: 0x%016" PRIx64 "\n", device_info->deviceName, device_info->boardSerialNumber);
  printf(" - Hardware: v%s, Firmware: v%s, Gateware: v%s\n", device_info->hardwareVersion, device_info->firmwareVersion,
          device_info->gatewareVersion);

  //printf(" - Protocol version: %s\n", device_info->protocolVersion);
  //printf("gateware target: %s\n", device_info->gatewareTargetBoard);

  LMS_GetChipTemperature(device, 0, &Temperature);
  printf(" - Temperature: %.0f°C\n", Temperature);

  // Initialize device with default configuration
  if (LMS_Init(device) != 0)
  {
    LMS_Close(device);
    return NULL;
  }
  LMS_GetChipTemperature(device, 0, &Temperature);
  printf(" - Temperature: %.0f°C\n", Temperature);

  // Enable the RX channel
  if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
  {
    LMS_Close(device);
    return NULL;
  }

  // Set the RX center frequency
  if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, (frequency_actual_rx)) != 0)
  {
    LMS_Close(device);
    return NULL;
  }

  // Set the initial sample rate and decimation
  if (SDRsamplerate < 200000.0)                    // 100 kHz bandwidth
  {
    LMS_SetSampleRate(device, (float)(SDRsamplerate), 32);
  }
  else if (SDRsamplerate < 400000.0)              // 200 kHz bandwidth
  {
    LMS_SetSampleRate(device, (float)(SDRsamplerate), 16);
  }
  else if (SDRsamplerate < 1000000.0)             // 500 kHz bandwidth
  {
    LMS_SetSampleRate(device, (float)(SDRsamplerate), 8);
  }
  else                                       // 1 - 20 MHz bandwidth
  {
    LMS_SetSampleRate(device, (float)(SDRsamplerate), 4);
  }

  double sr_host, sr_rf;
  if (LMS_GetSampleRate(device, LMS_CH_RX, 0, &sr_host, &sr_rf) < 0)
  {
    fprintf(stderr, "Warning : LMS_GetSampleRate() : %s\n", LMS_GetLastErrorMessage());
    return NULL;
  }
  printf("Lime RX Samplerate: Host: %.1fKs, RF: %.1fKs\n", (sr_host/1000.0), (sr_rf/1000.0));

  // Set the RX gain
  if (LMS_SetNormalizedGain(device, LMS_CH_RX, 0, gain) != 0)  // gain is 0  to 1.0
  {
    LMS_Close(device);
    return NULL;
  }

  // Report the actual LO Frequency
  double rxfreq = 0;
  LMS_GetLOFrequency(device, LMS_CH_RX, 0, &rxfreq);
  printf("RXFREQ after cal = %f\n", rxfreq);

  // Set the Analog LPF bandwidth (minimum of device is 1.4 MHz)
  if (LMS_SetLPFBW(device, LMS_CH_RX, 0, (float)(filter_bandwidth)) != 0)
  {
    LMS_Close(device);
    printf("Failed to set analog bandwidth\n");
    return NULL;
  }

  // Disable test signals generation in RX channel
  if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NONE, 0, 0) != 0)
  {
    LMS_Close(device);
    printf("Failed to disable test signal\n");
    return NULL;
  }

  // Streaming Setup
  lms_stream_t rx_stream;

  // Initialize streams
  rx_stream.channel = 0; //channel number
  rx_stream.fifoSize = 0; //fifo size in samples
  rx_stream.throughputVsLatency = 0.0;
  rx_stream.isTx = false; //RX channel
  rx_stream.dataFmt = LMS_FMT_F32;
  if (LMS_SetupStream(device, &rx_stream) != 0)
  {
    LMS_Close(device);
    printf("Failed to set-up stream\n");
    return NULL;
  }

  // Initialize data buffers

  extern float buffer1[1024];
  extern float buffer2[1024];

  // Start streaming
  if (LMS_StartStream(&rx_stream) != 0)
  {
    LMS_Close(device);
    printf("Failed to start streaming\n");
    return NULL;
  }

  // Streaming
  lms_stream_meta_t rx_metadata; //Use metadata for additional control over sample receive function behavior
  rx_metadata.flushPartialPacket = false; //currently has no effect in RX
  rx_metadata.waitForTimestamp = false; //currently has no effect in RX

  // Calibrate now all settings settled
  LMS_Calibrate(device, LMS_CH_RX, 0, (float)(filter_bandwidth), 0);

  LimeStarted = true;  // allows mode change now that Lime can be shut down

  uint64_t sample_read_time_was;
  sample_read_time_was = monotonic_ns();
  smoothed_sample_read_time = monotonic_ns();
 
  gain = 0.9;
  NewGain = true;

  smoothed_interval = 0;

  while(false == *exit_requested) // Main streaming loop
  {
    // Loop once round for each 1020 bytes for each of 2 buffers, and check for changes

    // Read buffer1
    LMS_RecvStream(&rx_stream, buffer1, 1020, &rx_metadata, 1000);
    sample_read_time = monotonic_ns();

    // Calculate smoothed interval between samples (= 2 * 1020 /sample rate = 19.92uS at 102400 sample rate)
    interval = sample_read_time - sample_read_time_was;
    smoothed_interval = (interval + 19 * smoothed_interval) / 20;
    delta = (int)(smoothed_interval - interval);
    sample_read_time_was = sample_read_time;

    // Calculate smoothed sample_read_time 
    smoothed_sample_read_time = (sample_read_time + 19 * (smoothed_sample_read_time_was + smoothed_interval)) / 20;
    smoothed_sample_read_time_was = smoothed_sample_read_time;
    deltaSRT = (int)(smoothed_sample_read_time - sample_read_time);

    // Print diagnostics
    //if ((delta < -100000) || (delta > 100000 ))
    //{
    //  printf("Interval = %ld nS, smoothed interval = %ld nS, dif = %ld nS.  SRT = %ld, SSRT = %ld, diff = %d nS\n", interval, smoothed_interval, delta, sample_read_time, smoothed_sample_read_time, deltaSRT);
    //}

    // Signal that data is valid
    buffer1_updated = true;

    // Read buffer2
    LMS_RecvStream(&rx_stream, buffer2, 1020, &rx_metadata, 1000);

    // Signal that data is valid
    buffer2_updated = true;

    // Break out of loop if required
    if (*exit_requested) 
    {
      break;
    }

    // Deal with parameter changes

    // Calibration requested
    if (NewCal == true)
    {
      LMS_Calibrate(device, LMS_CH_RX, 0, (float)(filter_bandwidth), 0);
      NewCal = false;
    }

    if (NewGain == true)
    {
      LMS_SetNormalizedGain(device, LMS_CH_RX, 0, gain);

      // Pause to let gain settle
      usleep(10000); 
      LMS_Calibrate(device, LMS_CH_RX, 0, (float)(filter_bandwidth), 0);
      NewGain = false;
    }

    // Change of filter bandwidth (sample rate and decimation)
    if (NewBW == true)
    {
      // Stop the stream
      LMS_StopStream(&rx_stream);

      // Set the new bandwidth
      LMS_SetLPFBW(device, LMS_CH_RX, 0, (float)(filter_bandwidth));

      // Set the new sample rate with the correct decimation
      if (SDRsamplerate < 200000.0)                    // 100 kHz bandwidth
      {
        LMS_SetSampleRate(device, (float)(SDRsamplerate), 32);
      }
      else if (SDRsamplerate < 400000.0)              // 200 kHz bandwidth
      {
        LMS_SetSampleRate(device, (float)(SDRsamplerate), 16);
      }
      else if (SDRsamplerate < 1000000.0)             // 500 kHz bandwidth
      {
        LMS_SetSampleRate(device, (float)(SDRsamplerate), 8);
      }
      else                                       // 1 - 20 MHz bandwidth
      {
        LMS_SetSampleRate(device, (float)(SDRsamplerate), 4);
      }
        
      // Restart the stream
      LMS_StartStream(&rx_stream);

      // Calibrate after a pause
      usleep(10000); 
      LMS_Calibrate(device, LMS_CH_RX, 0, (float)(filter_bandwidth), 0);

      NewBW = false;
    }

    if(*exit_requested)
    {
      printf("\nLime Exit Requested\n\n");
      break;
    }


  }  // end of streaming loop.  2 * 1020 samples each iteration

  // Stop streaming
  LMS_StopStream(&rx_stream); //stream is stopped but can be started again with LMS_StartStream()

  LMS_DestroyStream(device, &rx_stream); //stream is deallocated and can no longer be used
    
  // Tidily shut down the Lime

  LMS_Reset(device);
  LMS_Init(device);
  int nb_channel = LMS_GetNumChannels( device, LMS_CH_RX );
  int c;
  for( c = 0; c < nb_channel; c++ )
  {
    LMS_EnableChannel(device, LMS_CH_RX, c, false);
  }
  nb_channel = LMS_GetNumChannels( device, LMS_CH_TX );
  for( c = 0; c < nb_channel; c++ )
  {
    LMS_EnableChannel(device, LMS_CH_TX, c, false);
  }

  LMS_Close(device);
  printf("LimeSDR Closed\n");
  return NULL;
}

