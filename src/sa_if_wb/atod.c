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
#include <mcp23017.h>
#include <math.h>
#include <wiringPi.h> // Include new WiringPi library

#include "../common/timing.h"
#include "../common/mcp3002.h"

extern int ya[501];
extern bool yavalid[501];
extern uint32_t tperdiv;

int i;


void *atod_thread(void *arg)
{
  bool *exit_requested = (bool *)arg;
  int reading;
  bool flybackstatus = false;
  uint64_t end_blanking_time;
  uint64_t reading_interval = 50000;
  int initial_blank = 11;

  // Wait for start of flyback to initialise for long scans
  do
  {
    flybackstatus = digitalRead(6);
    usleep(1);  
  }
  while (flybackstatus == false);  // Digital Read 6 goes true when flyback starts

  // Main a-d reading loop
  while(false == *exit_requested)
  {
    // Loop once round for each 501 bytes

    switch (tperdiv)
    {
      case 1:                               // 0.001 sec/dev
        reading_interval = 25000;
        break;
      case 2:                               // 0.002 sec/dev
        reading_interval = 50000;
        break;
      case 5:                               // 0.005 sec/dev
        reading_interval = 115000;
        break;
      case 10:                              // 0.01 sec/dev
        reading_interval = 226000;
        break;
      case 20:                              // 0.02 sec/dev
        reading_interval = 445000;
        break;
      case 50:                              // 0.05 sec/dev
        reading_interval = 1080000;
        break;
      case 100:                             // 0.1 sec/dev
        reading_interval = 2130000;
        break;
      case 200:                             // 0.2 sec/dev
        reading_interval = 4190000;
        break;
      case 500:                             // 0.5 sec/dev
        reading_interval = 10100000;
        break;
      case 1000:                            // 1.0 sec/dev
        reading_interval = 19800000;
        break;
      case 2000:                            // 2.0 sec/dev
        reading_interval = 39000000;
        break;
      case 5000:                            // 5.0 sec/dev
        reading_interval = 98000000;
        break;
    }

    do  // Wait here for end of flyback.  Digital Read 6 goes false when flyback has finished
    {
      flybackstatus = digitalRead(6);
      usleep(1);  
    }
    while (flybackstatus == true);

    // Record time at the end of the blanking pulse
    end_blanking_time = monotonic_ns();

    // Initial zeros
    for (i = 0; i < initial_blank; i++)
    {
      ya[i] = 0;
      yavalid[i] = true;
    }


    for (reading = initial_blank; reading < 501; reading ++)
    {
      do  // wait here for correct time to take reading
      {
        usleep(1);
      }
      while (monotonic_ns() < (end_blanking_time + (reading_interval * (reading - initial_blank))));

      // Read the MCP3002
      ya[reading] = mcp3002_value(0);
      yavalid[reading] = true;

      flybackstatus = digitalRead(6);
   
      // Break out of loop if required
      if ((*exit_requested) ||  (flybackstatus == true))
      {
        break;
      }
    }

    // End zeros
    for (i = reading; i < 501; i++)
    {
      ya[i] = 0;
      yavalid[i] = true;
    }

  }  // end of a-d reading loop
  return NULL;
}

