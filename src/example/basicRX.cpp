/**
    @file   basicRX.cpp
    @author Lime Microsystems (www.limemicro.com)
    @brief  minimal RX example
 */
#include "lime/LimeSuite.h"
#include <iostream>
#include <chrono>

using namespace std;

//Device structure, should be initialize to NULL
lms_device_t* device = NULL;

int error()
{
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}

int main(int argc, char** argv)
{
    //Find devices
    int n;
    lms_info_str_t list[8]; //should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0) //NULL can be passed to only get number of devices
        error();

    cout << "Devices found: " << n << endl; //print number of devices
    if (n < 1)
        return -1;

    //open the first device
    if (LMS_Open(&device, list[0], NULL))
        error();

printf("Device Open\n");

    //Initialize device with default configuration
    //Do not use if you want to keep existing configuration
    //Use LMS_LoadConfig(device, "/path/to/file.ini") to load config from INI
    if (LMS_Init(device) != 0)
        error();

printf("Device Initialised\n");

    if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0) // Fix for v2
        error();

printf("TX Channel enabled\n");

    //Enable RX channel
    //Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        error();

printf("RX Channel enabled\n");

    //Set center frequency to 800 MHz
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, 800e6) != 0)
        error();

printf("Centre frequency set\n");

    //Set sample rate to 8 MHz, ask to use 2x oversampling in RF
    //This set sampling rate for all channels
    if (LMS_SetSampleRate(device, 8e6, 1) != 0)
        error();
    printf("Sample Rate set\n");

    if (LMS_Calibrate(device, LMS_CH_RX, 0, 8e6, 0) != 0)
    {
        printf("Calibrate error\n");
    }

    //Set center frequency to 850 MHz
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, 850e6) != 0)
        error();
   
    if (LMS_Calibrate(device, LMS_CH_RX, 0, 4e6, 0) != 0)
    {
        printf("Calibrate error\n");
    }


    //Streaming Setup

    //Initialize stream
    lms_stream_t streamId; //stream structure
    streamId.channel = 0; //channel number
    streamId.fifoSize = 1024 * 1024; //fifo size in samples
    streamId.throughputVsLatency = 1.0; //optimize for max throughput
    streamId.isTx = false; //RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    //Initialize data buffers
    const int sampleCnt = 5000; //complex samples per buffer
    int16_t buffer[sampleCnt * 2]; //buffer to hold complex values (2*samples))

    //Start streaming
    LMS_StartStream(&streamId);

    //Streaming
    int samplesRead = 0;
    auto t1 = chrono::high_resolution_clock::now();
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(1)) //run for 1 seconds
    {
        //Receive samples
        samplesRead = LMS_RecvStream(&streamId, buffer, sampleCnt, NULL, 1000);
        //I and Q samples are interleaved in buffer: IQIQIQ...
        //printf("Received %d samples\n", samplesRead);
        /*
        INSERT CODE FOR PROCESSING RECEIVED SAMPLES
        */
    }

    printf("Received %d samples\n", samplesRead);

    //Stop streaming
    LMS_StopStream(&streamId); //stream is stopped but can be started again with LMS_StartStream()

    //Set sample rate to 7 MHz, ask to use 2x oversampling in RF
    //This set sampling rate for all channels
    if (LMS_SetSampleRate(device, 7e6, 1) != 0)
        error();
    printf("Sample Rate set\n");


   t1 = chrono::high_resolution_clock::now();
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(1)) //run for 1 seconds
    {
//
     }

    if (LMS_Calibrate(device, LMS_CH_RX, 0, 4e6, 0) != 0)
    {
        printf("Calibrate error\n");
    }


   LMS_StartStream(&streamId);

    //Streaming
    samplesRead = 0;
    t1 = chrono::high_resolution_clock::now();
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(1)) //run for 1 seconds
    {
        //Receive samples
        samplesRead = LMS_RecvStream(&streamId, buffer, sampleCnt, NULL, 1000);
        //I and Q samples are interleaved in buffer: IQIQIQ...
        //printf("Received %d samples\n", samplesRead);
        /*
        INSERT CODE FOR PROCESSING RECEIVED SAMPLES
        */
    }

    printf("Received %d samples\n", samplesRead);




    LMS_DestroyStream(device, &streamId); //stream is deallocated and can no longer be used

    //Close device
    LMS_Close(device);

    return 0;
}