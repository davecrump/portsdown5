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
#include <limesuiteng/LimeSuite.h>
#include "lime.h"
#include "../common/timing.h"
#include "../common/buffer/buffer_circular.h"

lime_fft_buffer_t lime_fft_buffer;

extern bool NewFreq;
extern bool NewGain;
extern bool NewSpan;
extern bool NewCal;
extern float gain;

// Display Defaults:
double bandwidth = 512e3; // 512ks
double frequency_actual_rx = 437000000;
double CalFreq;

//Device structure, should be initialize to NULL
static lms_device_t* device = NULL;

void *lime_thread(void *arg)
{
  bool *exit_requested = (bool *)arg;
  int AntRequested = 0;
  int AntFound;

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

//LMS_Reset(device);

  // Initialize device with default configuration
  if (LMS_Init(device) != 0)
  {
    LMS_Close(device);
    return NULL;
  }

  // Query and display the device details
  const lms_dev_info_t *device_info;

  double Temperature;

  device_info = LMS_GetDeviceInfo(device);

  printf("%s Serial: 0x%016" PRIx64 "\n", device_info->deviceName, device_info->boardSerialNumber);
  printf(" - Hardware: v%s, Firmware: v%s, Gateware: v%s\n", device_info->hardwareVersion, device_info->firmwareVersion,
          device_info->gatewareVersion);

  printf(" - Protocol version: %s\n", device_info->protocolVersion);
  printf(" - gateware target: %s\n", device_info->gatewareTargetBoard);

  LMS_GetChipTemperature(device, 0, &Temperature);
  printf(" - Temperature: %.2f°C\n", Temperature);

  // Enable the RX channel
  if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
  {
    LMS_Close(device);
    return NULL;
  }

  // List the available antennas
  int nb_antenna = LMS_GetAntennaList(device, LMS_CH_RX, 0, NULL);
  lms_name_t antlist[nb_antenna];
  LMS_GetAntennaList(device, LMS_CH_RX, 0, antlist);

  int i;
  for (i = 0; i < nb_antenna; i++)
  {
    printf("Antenna %d: %s\n", i, antlist[i]);
  }

  // Set the RX center frequency
  if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, (frequency_actual_rx)) != 0)
  {
    LMS_Close(device);
    return NULL;
  }

  // Set the initial sample rate and decimation
  if (bandwidth < 200000.0)                    // 100 kHz bandwidth
  {
    printf("Set init sample rate:     LMS_SetSampleRate(device, %.1f, 32)\n", bandwidth);
    LMS_SetSampleRate(device, bandwidth, 32);
  }
  else if (bandwidth < 400000.0)              // 200 kHz bandwidth
  {
    printf("Set init sample rate:     LMS_SetSampleRate(device, %.1f, 16)\n", bandwidth);
    LMS_SetSampleRate(device, bandwidth, 16);
  }
  else if (bandwidth < 1000000.0)             // 500 kHz bandwidth
  {
    printf("Set init sample rate:     LMS_SetSampleRate(device, %.1f, 8)\n", bandwidth);
    LMS_SetSampleRate(device, bandwidth, 8);
  }
  else                                       // 1 - 20 MHz bandwidth
  {
    printf("Set init sample rate:     LMS_SetSampleRate(device, %.1f, 4)\n", bandwidth);
    LMS_SetSampleRate(device, bandwidth, 4);
  }

  double sr_host, sr_rf;
  if (LMS_GetSampleRate(device, LMS_CH_RX, 0, &sr_host, &sr_rf) < 0)
  {
    fprintf(stderr, "Warning : LMS_GetSampleRate() : %s\n", LMS_GetLastErrorMessage());
    return NULL;
  }
  //printf("Lime RX Samplerate: Host: %.1fKs, RF: %.1fKs\n", (sr_host/1000.0), (sr_rf/1000.0));
  printf("Check sample rates:       LMS_GetSampleRate(device, LMS_CH_RX, 0, &sr_host, &sr_rf)\n");
  printf("                                                                   sr_host = %.1f\n", sr_host);
  printf("                                                                             sr_rf = %.1f\n", sr_rf);

  // Set the RX gain
  printf("Setting initial gain:     LMS_SetNormalizedGain(device, LMS_CH_RX, 0, %.2f)\n", gain);
  if (LMS_SetNormalizedGain(device, LMS_CH_RX, 0, gain) != 0)  // gain is 0  to 1.0
  {
    LMS_Close(device);
    return NULL;
  }

  // Perform the initial calibration
  printf("Intitial calibration:     LMS_Calibrate(device, LMS_CH_RX, 0, %.1f, 0)\n", bandwidth);
  if (LMS_Calibrate(device, LMS_CH_RX, 0, bandwidth, 0) < 0)
  {
    fprintf(stderr, "LMS_Calibrate() : %s\n", LMS_GetLastErrorMessage());
    // Reset gain to zero 
    LMS_SetNormalizedGain(device, true, 0, 0);
    return false;
  }
  CalFreq = frequency_actual_rx;

  // Report the actual LO Frequency
  double rxfreq = 0;
  LMS_GetLOFrequency(device, LMS_CH_RX, 0, &rxfreq);
  printf("Checking LO frequency:    LMS_GetLOFrequency(device, LMS_CH_RX, 0, &rxfreq) returns %.1f\n", rxfreq);


  // Antenna is set automatically by LimeSuite for LimeSDR Mini
  // but needs setting for LimeSDR USB and LimeSDR XTRX.  Options:
  // LMS_SetAntenna(device, LMS_CH_RX, 0, 1); //LNA_H
  // LMS_SetAntenna(device, LMS_CH_RX, 0, 2); //LNA_L
  // LMS_SetAntenna(device, LMS_CH_RX, 0, 3); //LNA_W

  if (strcmp(device_info->deviceName, "LimeSDR-USB") == 0)
  {
    if (frequency_actual_rx >= 2e9)
    {
      LMS_SetAntenna(device, LMS_CH_RX, 0, 1); //LNA_H
    }
    else
    {
      LMS_SetAntenna(device, LMS_CH_RX, 0, 2); //LNA_L

    }
  }

  if (strcmp(device_info->deviceName, "LimeSDR XTRX") == 0)
  {
    if (frequency_actual_rx >= 1200000000)
    {
      printf("Setting Antenna:          LMS_SetAntenna(device, LMS_CH_RX, 0, 1)\n");
      LMS_SetAntenna(device, LMS_CH_RX, 0, 1); //LNA_H
    }
    else
    {
      printf("Setting Antenna:          LMS_SetAntenna(device, LMS_CH_RX, 0, 2)\n");
      LMS_SetAntenna(device, LMS_CH_RX, 0, 2); //LNA_L
    }
  }
  // Set correct Antenna port for LimeSDR Mini:
  if ((strcmp(device_info->deviceName, "LimeSDR-Mini") == 0) || (strcmp(device_info->deviceName, "LimeSDR-Mini_v2")) == 0)
  {
    printf("Setting Antenna:          LMS_SetAntenna(device, LMS_CH_RX, 0, 3)\n");
    LMS_SetAntenna(device, LMS_CH_RX, 0, 3); //LNA_W
  }

  AntFound = LMS_GetAntenna(device, LMS_CH_RX, 0);
  printf("Checking Antenna:         LMS_GetAntenna(device, LMS_CH_RX, 0) returns %d %s\n", AntFound, antlist[AntFound]);

  printf("Setting bandwidth:        LMS_SetLPFBW(device, LMS_CH_RX, 0, %.1f)\n", (bandwidth * 1.2));
  // Set the Analog LPF bandwidth (minimum of device is 1.4 MHz)
  // Note that for LimeSuiteNG, gain must be reset after setting bandwidth
  if (LMS_SetLPFBW(device, LMS_CH_RX, 0, (bandwidth * 1.2)) != 0)
  {
    LMS_Close(device);
    printf("Failed to set analog bandwidth\n");
    return NULL;
  }

  printf("Setting gain:             LMS_SetNormalizedGain(device, LMS_CH_RX, 0, %.2f)\n", gain);
  LMS_SetNormalizedGain(device, LMS_CH_RX, 0, gain);

  printf("Disabling Test Signals:   LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NONE, 0, 0)\n");
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
  rx_stream.fifoSize = 1024 * 1024; //fifo size in samples
  rx_stream.throughputVsLatency = 0.5;
  rx_stream.isTx = false; //RX channel
  rx_stream.dataFmt = LMS_FMT_F32;
  printf("Setting up the stream:    LMS_SetupStream(device, &rx_stream)\n");
  printf("                                                  rx_stream.channel = 0\n");
  printf("                                                  rx_stream.fifoSize = 1024 * 1024;\n");
  printf("                                                  rx_stream.throughputVsLatency = 0.5\n");
  printf("                                                  rx_stream.isTx = false\n");
  printf("                                                  rx_stream.dataFmt = LMS_FMT_F32\n");
  if (LMS_SetupStream(device, &rx_stream) != 0)
  {
    LMS_Close(device);
    printf("Failed to set-up stream\n");
    return NULL;
  }

  // Initialize data buffers
  const int buffersize = 8 * 1024; // 8192 I+Q samples
  float *buffer;
  buffer = malloc(buffersize * 2 * sizeof(float)); //buffer to hold complex values (2*samples))

  printf("Starting the stream:      LMS_StartStream(&rx_stream)\n");
  // Start streaming
  if (LMS_StartStream(&rx_stream) != 0)
  {
    LMS_Close(device);
    printf("Failed to start streaming\n");
    return NULL;
  }


      // Stop the stream
      //LMS_StopStream(&rx_stream);

      //LMS_SetupStream(device, &rx_stream);
    
      // Restart the stream
      //LMS_StartStream(&rx_stream);


  // Streaming
  lms_stream_meta_t rx_metadata; //Use metadata for additional control over sample receive function behavior
  rx_metadata.flushPartialPacket = false; //currently has no effect in RX
  rx_metadata.waitForTimestamp = false; //currently has no effect in RX

  //uint64_t last_stats = 0;

  int samplesRead = 0;

  // Calibrate again now all settings settled
  printf("Calibrate again:          LMS_Calibrate(device, LMS_CH_RX, 0, %.1f, 0)\n", bandwidth);
  LMS_Calibrate(device, LMS_CH_RX, 0, bandwidth, 0);

  while(false == *exit_requested) 
  {
    // Receive samples
    samplesRead = LMS_RecvStream(&rx_stream, buffer, buffersize, &rx_metadata, 1000);
    //printf("Samples Read = %d", samplesRead);

    // Break out of loop if required
    if (*exit_requested) 
    {
      break;
    }

    // Deal with parameter changes

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Frequency
    if (NewFreq == true)
    {
      printf("Frequncy Change requested\n");
      printf("Setting LO frequency:     LMS_SetLOFrequency(device, LMS_CH_RX, 0, %.1f)\n", frequency_actual_rx);
      LMS_SetLOFrequency(device, LMS_CH_RX, 0, frequency_actual_rx);

      // Calibrate on significant frequency change
      if ((frequency_actual_rx > (1.1 * CalFreq)) || (frequency_actual_rx < (0.9 * CalFreq)))
      {
        // Pause to let frequency settle
        usleep(10000); 
        printf("Calibrate again:          LMS_Calibrate(device, LMS_CH_RX, 0, %.1f, 0)\n", bandwidth);
        LMS_Calibrate(device, LMS_CH_RX, 0, bandwidth, 0);
        CalFreq = frequency_actual_rx;
      }
      NewFreq = false;

      LMS_GetLOFrequency(device, LMS_CH_RX, 0, &rxfreq);
      printf("Checking LO frequency:    LMS_GetLOFrequency(device, LMS_CH_RX, 0, &rxfreq) returns %.1f\n", rxfreq);

      // Set correct Antenna port for LimeSDR_ USB or LimeSDR XTRX:
      if (strcmp(device_info->deviceName, "LimeSDR-USB") == 0)
      {
        if (frequency_actual_rx >= 2e9)
        {
          LMS_SetAntenna(device, LMS_CH_RX, 0, 1); //LNA_H
        }
        else
        {
          LMS_SetAntenna(device, LMS_CH_RX, 0, 2); //LNA_L
        }
      }
      if (strcmp(device_info->deviceName, "LimeSDR XTRX") == 0)
      {
        if (frequency_actual_rx >= 1200000000)
        {
          LMS_SetAntenna(device, LMS_CH_RX, 0, 1); //LNA_H
        }
        else
        {
          LMS_SetAntenna(device, LMS_CH_RX, 0, 2); //LNA_L
        }
      }

      // Set correct Antenna port for LimeSDR Mini:
      if ((strcmp(device_info->deviceName, "LimeSDR-Mini") == 0) || (strcmp(device_info->deviceName, "LimeSDR-Mini_v2")) == 0)
      {
        printf("Setting Antenna:          LMS_SetAntenna(device, LMS_CH_RX, 0, 3)\n");
        LMS_SetAntenna(device, LMS_CH_RX, 0, 3); //LNA_W
      }
      AntFound = LMS_GetAntenna(device, LMS_CH_RX, 0);
      printf("Checking Antenna:         LMS_GetAntenna(device, LMS_CH_RX, 0) returns %d %s\n", AntFound, antlist[AntFound]);
    }

    // Calibration requested
    if (NewCal == true)
    {
      LMS_SetLOFrequency(device, LMS_CH_RX, 0, frequency_actual_rx);
      printf("Calibrate again:          LMS_Calibrate(device, LMS_CH_RX, 0, %.1f, 0)\n", bandwidth);
      LMS_Calibrate(device, LMS_CH_RX, 0, bandwidth, 0);
      CalFreq = frequency_actual_rx;
      NewCal = false;

      LMS_GetLOFrequency(device, LMS_CH_RX, 0, &rxfreq);
      printf("Requested frequency  = %f, RXFREQ after cal = %f\n", frequency_actual_rx, rxfreq);
    }

    if (NewGain == true)
    {
      printf("\nChange of gain to %.2f requested\n", gain);
      printf("Setting gain:             LMS_SetNormalizedGain(device, LMS_CH_RX, 0, %.2f)\n", gain);
      LMS_SetNormalizedGain(device, LMS_CH_RX, 0, gain);

      // Pause to let gain settle
      usleep(10000); 
      printf("New calibration:          LMS_Calibrate(device, LMS_CH_RX, 0, %.1f, 0)\n", bandwidth);
      LMS_Calibrate(device, LMS_CH_RX, 0, bandwidth, 0);
      CalFreq = frequency_actual_rx;

if (false)  // Set true fro antenna switch testing
{
  if (AntRequested <= 2)
  {
    AntRequested++;
  }
  else
  {
    AntRequested = 0;
  }

  int SetResult = LMS_SetAntenna(device, LMS_CH_RX, 0, AntRequested);

  AntFound = LMS_GetAntenna(device, LMS_CH_RX, 0);
  printf("Antenna requested = %d, LMS_SetAntenna returns %d, Antenna %d %s selected\n", AntRequested, SetResult, AntFound, antlist[AntFound]);
}
      NewGain = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    // Change of span (sample rate and decimation)
    if (NewSpan == true)
    {
      printf("\nChange of Sample rate to %.1f requested\n", bandwidth);

      // Stop the stream
      printf("Stopping stream:          LMS_StopStream(&rx_stream)\n");
      LMS_StopStream(&rx_stream);

      printf("Amend the bandwidth:      LMS_SetLPFBW(device, LMS_CH_RX, 0, %.1f)\n", bandwidth * 1.2);
      LMS_SetLPFBW(device, LMS_CH_RX, 0, (bandwidth * 1.2));

      // Set the new sample rate with the correct decimation
      if (bandwidth < 200000.0)
      {
        printf("Amend sample rate:        LMS_SetSampleRate(device, %.1f, 32)\n", bandwidth);
        LMS_SetSampleRate(device, bandwidth, 32);
      }
      else if (bandwidth < 400000.0)
      {
        printf("Amend sample rate:        LMS_SetSampleRate(device, %.1f, 16)\n", bandwidth);
        LMS_SetSampleRate(device, bandwidth, 16);
      }
      else if (bandwidth < 1000000.0)
      {
        printf("Amend sample rate:        LMS_SetSampleRate(device, %.1f, 8)\n", bandwidth);
        LMS_SetSampleRate(device, bandwidth, 8);
      }
      else
      {
        printf("Amend sample rate:        LMS_SetSampleRate(device, %.1f, 4)\n", bandwidth);
        LMS_SetSampleRate(device, bandwidth, 4);
      }


      if (LMS_GetSampleRate(device, LMS_CH_RX, 0, &sr_host, &sr_rf) < 0)
      {
        fprintf(stderr, "Warning : LMS_GetSampleRate() : %s\n", LMS_GetLastErrorMessage());
        return NULL;
      }

      printf("Check sample rates:       LMS_GetSampleRate(device, LMS_CH_RX, 0, &sr_host, &sr_rf)\n");
      printf("                          sr_host = %.1f\n", sr_host);
      printf("                          sr_rf = %.1f\n", sr_rf);

      printf("Destroy old stream:       LMS_DestroyStream(device, &rx_stream)\n");
      LMS_DestroyStream(device, &rx_stream);

      printf("Set up the new stream:    LMS_SetupStream(device, &rx_stream)\n");
      LMS_SetupStream(device, &rx_stream);

      // Gain needs to be reset after stream stop/start in LimeSuiteNG
      //printf("Resetting gain:           LMS_SetNormalizedGain(device, LMS_CH_RX, 0, %.2f)\n", gain);
      //LMS_SetNormalizedGain(device, LMS_CH_RX, 0, gain);
         
      // Restart the stream
      printf("Restart the stream:       LMS_StartStream(&rx_stream)\n");
      LMS_StartStream(&rx_stream);

      // Calibrate after a pause
      usleep(10000); 
      printf("Calibrate again:          LMS_Calibrate(device, LMS_CH_RX, 0, %.1f, 0)\n", bandwidth);
      LMS_Calibrate(device, LMS_CH_RX, 0, bandwidth, 0);

      // Print sample rate for debug
      //if (LMS_GetSampleRate(device, LMS_CH_RX, 0, &sr_host, &sr_rf) < 0)
      //{
      //  fprintf(stderr, "Warning : LMS_GetSampleRate() : %s\n", LMS_GetLastErrorMessage());
      //  return NULL;
      //}
      //printf("Lime RX Samplerate: Host: %.1fKs, RF: %.1fKs\n", (sr_host/1000.0), (sr_rf/1000.0));

      NewSpan = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    // Copy out buffer for Band FFT

    // Lock the fft buffer
    pthread_mutex_lock(&lime_fft_buffer.mutex);

    // Reset index so FFT knows it's new data
    lime_fft_buffer.index = 0;
    memcpy(lime_fft_buffer.data, buffer, (samplesRead * 2 * sizeof(float)));
    lime_fft_buffer.size = (samplesRead * sizeof(float));

    // Signal that buffer is ready and unlock buffer
    pthread_cond_signal(&lime_fft_buffer.signal);
    pthread_mutex_unlock(&lime_fft_buffer.mutex);

    if(*exit_requested)
    {
      printf("\nLime Exit Requested\n\n");
      break;
    }

#if 0
    uint32_t head, tail, capacity, occupied;
    buffer_circular_stats(&buffer_circular_iq_main, &head, &tail, &capacity, &occupied);
    printf(" - Head: %d, Tail: %d, Capacity: %d, Occupied: %d\n", head, tail, capacity, occupied);
#endif

#if 0
        samples_total += samplesRead;
        printf("Lime samplerate: %.3f (total: %lld\n", (float)(samples_total * 1000) / (monotonic_ms() - start_monotonic), samples_total);
#endif
  }  // end of streaming loop.  8k samples each iteration

  // Stop streaming
  LMS_StopStream(&rx_stream); //stream is stopped but can be started again with LMS_StartStream()

  LMS_DestroyStream(device, &rx_stream); //stream is deallocated and can no longer be used
    
  free(buffer);

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

