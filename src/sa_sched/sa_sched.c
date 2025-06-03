#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>
#include <fftw3.h>
#include <wiringPi.h> // Include new WiringPi library
#include <mcp23017.h>

#include "../common/hardware.h"

bool app_exit = false;
int exit_code = 0;
int i;
uint8_t samode = 0;


static void cleanexit(int calling_exit_code)
{
  app_exit = true;
  usleep(100000);
  exit_code = calling_exit_code;
  printf("Clean Exit Code %d\n", exit_code);
  usleep(1000000);
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);
  exit(exit_code);
}


static void terminate(int dummy)
{
  app_exit = true;
  printf("Terminate\n");
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);
  sprintf(Commnd,"reset");
  system(Commnd);
  exit(207);
}


int main(int argc, char **argv)
{
  // Catch sigaction and call terminate
  for (i = 0; i < 16; i++)
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = terminate;
    sigaction(i, &sa, NULL);
  }

  // Start Wiring Pi and set up WPi 6, pin 22 as flyback trigger
  wiringPiSetup();

//bool *exit_requested = (bool *)arg;

  #define AD_BASE 200
  #define M1_BASE 100
  #define M2_BASE 150

  #define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
  #define BYTE_TO_BINARY(byte)  \
    ((byte) & 0x80 ? '1' : '0'), \
    ((byte) & 0x40 ? '1' : '0'), \
    ((byte) & 0x20 ? '1' : '0'), \
    ((byte) & 0x10 ? '1' : '0'), \
    ((byte) & 0x08 ? '1' : '0'), \
    ((byte) & 0x04 ? '1' : '0'), \
    ((byte) & 0x02 ? '1' : '0'), \
    ((byte) & 0x01 ? '1' : '0') 

  int i;
  char ModeText[30];

  uint8_t M1A = 0;
  uint8_t M1B = 0;
  uint8_t M2A = 0;
  uint8_t M2B = 0;

  // Set up first MCP23017 for reading front panel switches
  mcp23017Setup (M1_BASE, 0x20);

  // Set pins GPA0 - 6 and GPB0 - 6 as inputs.  GPBA7 and GPB7 are hard wired as outputs
  for (i = 0; i < 7; i++)
  {
    pinMode (M1_BASE + i, INPUT);
    pinMode (M1_BASE + i + 8, INPUT);
  }

  // Set up second MCP23017 for reading front panel switches
  mcp23017Setup (M2_BASE, 0x21);

  // Set pins GPA0 - 6 and GPB0 - 6 as inputs.  GPBA7 and GPB7 are hard wired as outputs
  for (i = 0; i < 7; i++)
  {
    pinMode (M2_BASE + i, INPUT);
    pinMode (M2_BASE + i + 8, INPUT);
  }

  // Read the mode parameters

  {
    // Read Multiplexers
    M1A = 0;
    M1B = 0;
    M2A = 0;
    M2B = 0;

    for (i = 6; i >= 0; i--)
    {
      M1A = M1A * 2;
      if (digitalRead (M1_BASE + i) == HIGH)
      {
        M1A = M1A + 1;
      }

      M1B = M1B * 2;
      if (digitalRead (M1_BASE + i + 8) == HIGH)
      {
        M1B = M1B + 1;
      }

      M2A = M2A * 2;
      if (digitalRead (M2_BASE + i) == HIGH)
      {
        M2A = M2A + 1;
      }

      M2B = M2B * 2;
      if (digitalRead (M2_BASE + i + 8) == HIGH)
      {
        M2B = M2B + 1;
      }
    }

    // Extract the mode
    samode = M2A & 0x0F;
    
    //Extract the Exit Code
    exit_code = 170 + samode;

    // Prepare the diagnostics
    switch (samode)
    {
      case 1:                                            // DATV RX on SA IF
        strcpy(ModeText, "DATV RX on SA IF");
        break;
      case 2:                                            // Direct DATV RX
        strcpy(ModeText, "Direct DATV RX");
        break;
      case 3:                                            // AD8318 Power Detector
        strcpy(ModeText, "Power Detector");
        break;
      case 4:                                            // Bolo power Meter
        strcpy(ModeText, "Bolo power Meter");
        break;
      case 5:                                            // Direct SDR
        strcpy(ModeText, "Direct SDR");
        break;
      case 6:                                            // SDR fft on SA IF
        strcpy(ModeText, "SDR fft on SA IF");
        break;
      case 7:                                            // Swept SA
        strcpy(ModeText, "Swept SA");
        break;
      case 8:                                            // Swept Tracking Generator
        strcpy(ModeText, "Swept Tracking Generator");
        break;
      case 9:                                            // Zero Span Tracking Generator
        strcpy(ModeText, "Zero Span Tracking Generator");
        break;
    }
  }

  printf("\nMode %d detected. %s.  Exit code %d\n\n", samode, ModeText, exit_code);
  cleanexit(exit_code);     // Exit
  usleep(100000);           // Give it time to exit
  printf("Didn't Exit\n");  // Print error message if it didn't
}