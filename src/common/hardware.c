#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware.h"

/***************************************************************************//**
 * @brief Detects if a mouse is currently connected
 *
 * @param nil
 *
 * @return 0 if connected, 1 if not connected
*******************************************************************************/

int CheckMouse()
{
  FILE *fp;
  char response[255];

  // Look for a mouse event entry
  fp = popen("echo \"0\" | sudo evemu-describe 2>&1 | grep '/dev/input/event' | grep -e 'Mouse' -e 'mouse'", "r");
  if (fp == NULL)
  {
    printf("Failed to run command\n" );
    exit(1);
  }

  while (fgets(response, 65, fp) != NULL)
  {
    if (strlen(response) > 4)  //   Found a mouse
    {
      pclose(fp);
      return 0;
    }
  }

  // No mouse found
  pclose(fp);
  return 1;
}


/***************************************************************************//**
 * @brief Looks up the current IPV4 address
 *
 * @param IPAddress (str) IP Address to be passed as a string
 *
 * @return void
*******************************************************************************/

void GetIPAddr(char IPAddress[18])
{
  FILE *fp;

  /* Open the command for reading. */
  fp = popen("ifconfig | grep -Eo \'inet (addr:)?([0-9]*\\.){3}[0-9]*\' | grep -Eo \'([0-9]*\\.){3}[0-9]*\' | grep -v \'127.0.0.1\' | head -1", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(IPAddress, 17, fp) != NULL)
  {
    //printf("%s", IPAddress);
  }

  /* close */
  pclose(fp);
}


/***************************************************************************//**
 * @brief Looks up the current second IPV4 address
 *
 * @param IPAddress (str) IP Address to be passed as a string
 *
 * @return void
*******************************************************************************/

void GetIPAddr2(char IPAddress[18])
{
  FILE *fp;

  /* Open the command for reading. */
  fp = popen("ifconfig | grep -Eo \'inet (addr:)?([0-9]*\\.){3}[0-9]*\' | grep -Eo \'([0-9]*\\.){3}[0-9]*\' | grep -v \'127.0.0.1\' | sed -n '2 p'", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(IPAddress, 17, fp) != NULL)
  {
    //printf("%s", IPAddress);
  }

  /* close */
  pclose(fp);
}


/***************************************************************************//**
 * @brief Checks for the presence of an FTDI Device or a PicoTuner
 *        
 * @param None
 *
 * @return 0 if present, 1 if not present
*******************************************************************************/

int CheckTuner()
{
  char FTDIStatus[255];
  FILE *fp;
  int ftdistat = 1;
  char response_line[255];

  /* Open the command for reading. */
  fp = popen("lsusb | grep -E -o \"FT2232C\"", "r");
  if (fp == NULL)
  {
    printf("Failed to run command\n" );
    exit(1);
  }

  // Read the output a line at a time - output it
  while (fgets(FTDIStatus, sizeof(FTDIStatus)-1, fp) != NULL)
  {
    if (strlen(response_line) > 1)
    {
      printf("FTDI Module Detected\n" );
      ftdistat = 0;
    }
  }
  pclose(fp);

  // Check for PicoTuner if no MiniTiouner

  if (ftdistat != 0)
  {
    fp = popen("lsusb | grep '2e8a:ba2c'", "r");  // RPi Pico interface
    if (fp == NULL)
    {
      printf("Failed to run command\n" );
      exit(1);
    }

    /* Read the output a line at a time - output it. */
    while (fgets(response_line, 250, fp) != NULL)
    {
      if (strlen(response_line) > 1)
      {
        printf("PicoTuner Detected\n" );
        ftdistat = 0;
      }
    }
    pclose(fp);
  }

  return(ftdistat);
}



/***************************************************************************//**
 * @brief Looks up the current wireless (wlan0) IPV4 address
 *
 * @param IPAddress (str) IP Address to be passed as a string
 *
 * @return void
*******************************************************************************/

void Get_wlan0_IPAddr(char IPAddress[18])
{
  FILE *fp;

  /* Open the command for reading. */
  fp = popen("ifconfig | grep -A1 \'wlan0\' | grep -Eo \'inet (addr:)?([0-9]*\\.){3}[0-9]*\' | grep -Eo \'([0-9]*\\.){3}[0-9]*\' | grep -v \'127.0.0.1\' ", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(IPAddress, 16, fp) != NULL)
  {
    //printf("%s", IPAddress);
  }

  /* close */
  pclose(fp);
}


/***************************************************************************//**
 * @brief Looks up the Broadcom number for a physical GPIO pin
 *
 * @param GPIO physical pin number
 *
 * @return Broadcom software GPIO number
*******************************************************************************/

int PinToBroadcom(int Pin)
{
  int Broadcom = -1;  // Illegal default

  switch(Pin)
  {
    case 3:
      Broadcom = 2;
      break;
    case 5:
      Broadcom = 3;
      break;
    case 7:
      Broadcom = 4;
      break;
    case 8:
      Broadcom = 14;
      break;
    case 10:
      Broadcom = 15;
      break;
    case 11:
      Broadcom = 17;
      break;
    case 12:
      Broadcom = 18;
      break;
    case 13:
      Broadcom = 27;
      break;
    case 15:
      Broadcom = 22;
      break;
    case 16:
      Broadcom = 23;
      break;
    case 18:
      Broadcom = 24;
      break;
    case 19:
      Broadcom = 10;
      break;
    case 21:
      Broadcom = 9;
      break;
    case 22:
      Broadcom = 25;
      break;
    case 23:
      Broadcom = 11;
      break;
    case 24:
      Broadcom = 8;
      break;
    case 26:
      Broadcom = 7;
      break;
    case 27:
      Broadcom = 0; // Reserved
      break;
    case 28:
      Broadcom = 1; // Reserved
      break;
    case 29:
      Broadcom = 5;
      break;
    case 31:
      Broadcom = 6;
      break;
    case 32:
      Broadcom = 12;
      break;
    case 33:
      Broadcom = 13;
      break;
    case 35:
      Broadcom = 19;
      break;
    case 36:
      Broadcom = 16;
      break;
    case 37:
      Broadcom = 26;
      break;
    case 38:
      Broadcom = 20;
      break;
    case 40:
      Broadcom = 21;
      break;
    default:
      Broadcom = 0; // Reserved
      break;
  }
  return Broadcom;
}


/***************************************************************************//**
 * @brief Looks up the physical GPIO pin number for a Broadcom number
 *
 * @param Broadcom number
 *
 * @return physical GPIO pin number
*******************************************************************************/

int BroadcomToPin(int Broadcom)
{
  int Pin= -1;  // Illegal default

  switch(Broadcom)
  {
    case 0:
      Pin = 27;
      break;
    case 1:
      Pin = 28;
      break;
    case 2:
      Pin = 3;
      break;
    case 3:
      Pin = 5;
      break;
    case 4:
      Pin = 7;
      break;
    case 5:
      Pin = 29;
      break;
    case 6:
      Pin = 31;
      break;
    case 7:
      Pin = 26;
      break;
    case 8:
      Pin= 24;
      break;
    case 9:
      Pin= 21;
      break;
    case 10:
      Pin = 19;
      break;
    case 11:
      Pin = 23;
      break;
    case 12:
      Pin = 32;
      break;
    case 13:
      Pin = 33;
      break;
    case 14:
      Pin = 8;
      break;
    case 15:
      Pin = 10;
      break;
    case 16:
      Pin = 36;
      break;
    case 17:
      Pin = 11;
      break;
    case 18:
      Pin = 12;
      break;
    case 19:
      Pin = 35;
      break;
    case 20:
      Pin = 38;
      break;
    case 21:
      Pin = 40;
      break;
    case 22:
      Pin = 15;
      break;
    case 23:
      Pin = 16;
      break;
    case 24:
      Pin = 18;
      break;
    case 25:
      Pin = 22;
      break;
    case 26:
      Pin = 37;
      break;
    case 27:
      Pin = 13;
      break;
  }
  return Pin;
}
