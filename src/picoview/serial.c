#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <termios.h>

#include "../common/timing.h"

extern bool app_exit;             // Set by clean_exit and terminate

int sfd;                          // Serial File Descriptor
char buf2[6000];                  // Used to print serial input for debugging
char buf[1];                      // Character read from serial port
int count = 0;                    // Counts characters read from serial port
bool metadata = false;            // true while receiving metadata
bool preamble = false;            // true while receiving the preample
int preamble_count = 0;           // Character count
char preamble_text[7];
int preamble_value = 0;
bool samples = false;
int samples_count = 0;            // Character count
char samples_text[7];
int samples_value = 0;
bool ADCclkDiv = false;
int ADCclkDiv_count = 0;          // Character count
char ADCclkDiv_text[7];
extern int ADCclkDiv_value;
bool cpuClkKhz = false;
int cpuClkKhz_count = 0;          // Character count
char cpuClkKhz_text[7];
int cpuClkKhz_value = 0;
bool magdata = false;
int dataindex = 0;
int datachar = 0;                 // 0, 1 or 2 depending on expected character
char hexstring[4];
extern int adresult[2047];
int i;
extern bool dataready;            // signals that full meassage has been received

void *serial_thread(void *arg)
{
  // Open the serial port
  sfd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY );

  if (sfd == -1)
  {
    printf ("Error no is : %d\n", errno);
    printf("Serial open Error description is : %s\n",strerror(errno));
    exit(-1);
  }
  else
  {
    printf("Serial Port open\n");
  }

  // Read the data from the port
 
  while (app_exit == false)
  {
    read(sfd, buf, 1);
    //printf("%c", buf[0]);
    if (buf[0] !=  '.')
    {
      if ((buf[0] == 'S') || (metadata == true))                  // Read the metadata
      {
        metadata = true;
        if (buf[0] == 'p')                                        // Start of preample
        {
          preamble = true;
          preamble_count = 0;
        }
        if ((preamble == true) && (buf[0] != 'p'))                // Preamble digits
        {
          preamble_text[preamble_count] = buf[0];
          preamble_count++;
          if (preamble_count > 5)
          {
            preamble_text[6] = '\0';
            preamble = false;
            preamble_count = 0;
            preamble_value = atoi(preamble_text);
           // printf("Preamble: %s, %d\n", preamble_text, preamble_value);
          }
        }

        if (buf[0] == 'n')                                        // Start of samples
        {
          samples = true;
          samples_count = 0;
        }
        if ((samples == true) && (buf[0] != 'n'))                 // Sample digits
        {
          samples_text[samples_count] = buf[0];
          samples_count++;
          if (samples_count > 5)
          {
            samples_text[6] = '\0';
            samples = false;
            samples_count = 0;
            samples_value = atoi(samples_text);
            //printf("samples: %s, %d\n", samples_text, samples_value);
          }
        }

        if (buf[0] == 'd')                                        // Start of ADCclkDiv
        {
          ADCclkDiv = true;
          ADCclkDiv_count = 0;
        }
        if ((ADCclkDiv == true) && (buf[0] != 'd'))                 // ADCclkDiv digits
        {
          ADCclkDiv_text[ADCclkDiv_count] = buf[0];
          ADCclkDiv_count++;
          if (ADCclkDiv_count > 5)
          {
            ADCclkDiv_text[6] = '\0';
            ADCclkDiv = false;
            ADCclkDiv_count = 0;
            ADCclkDiv_value = atoi(ADCclkDiv_text);
            //printf("ADCclkDiv: %s, %d\n", ADCclkDiv_text, ADCclkDiv_value);
          }
        }

        if (buf[0] == 'c')                                        // Start of cpuClkKhz
        {
          cpuClkKhz = true;
          cpuClkKhz_count = 0;
        }
        if ((cpuClkKhz == true) && (buf[0] != 'c'))                 // cpuClkKhz digits
        {
          cpuClkKhz_text[cpuClkKhz_count] = buf[0];
          cpuClkKhz_count++;
          if (cpuClkKhz_count > 5)
          {
            cpuClkKhz_text[6] = '\0';
            cpuClkKhz = false;
            cpuClkKhz_count = 0;
            cpuClkKhz_value = atoi(cpuClkKhz_text);
            //printf("cpuClkKhz: %s, %d\n", cpuClkKhz_text, cpuClkKhz_value);
            metadata = false;
            magdata = true;
            dataindex = 0;
            datachar = 0;
          }
        }
      } 

      if (magdata == true)                                        // expecting magdata
      {
        if (buf[0] == ',')                                        // separator
        {
          datachar = 0;
          if (dataindex > 1034)                                   // End of data
          {
            magdata = false;
          }
        }
        else                                                      // use data to build hex string of length 3
        {
          hexstring[datachar] = buf[0];
          datachar++;
          if (datachar > 2)                                       // Convert hex string to int
          {
            hexstring[3] = '\0';
            adresult[dataindex] = (int)strtol(hexstring, NULL, 16);
            //printf("Point %d Value %d\n", dataindex, adresult[dataindex]);
            dataindex++;

            // Fill in missing samples from 1015 to 1023
            if (dataindex > 1014)      // (will be 1015)
            {
              for (i = 0; i < 9; i++)
              {
                adresult[1015 + i] = adresult[1014];
                //printf("Point %d Value %d\n", 1015 + i, adresult[1015 + i]);
              }
              dataready = true;
              magdata = false;   // Stop data collection
            }
          }
        }

      }

      buf2[count] = buf [0];
      count++;
    }
    else
    {
      buf2[count + 1] = 0;
      //printf("\nCount = %d\n", count);
      count = 0;
      //printf("-%s-\n\n\n",buf2);
      //printf("DataIndex = %d\n", dataindex);
      dataindex = 0;
      magdata = false;
    }
  }

  close(sfd);
  return NULL;
}

