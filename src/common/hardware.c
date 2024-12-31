#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware.h"

/***************************************************************************//**
 * @brief Detects if a mouse is currently connected
 * Note gives false positive if touchscreen connected
 *
 * @param nil
 *
 * @return 0 if connected, 1 if not connected
*******************************************************************************/

int CheckMouse()
{
  FILE *fp;
  char response_line[255];

  fp = popen("ls -l /dev/input | grep 'mouse'", "r");
  if (fp == NULL)
  {
    printf("Failed to run command\n" );
    exit(1);
  }

  // Response is "crw-rw---- 1 root input 13, 32 Apr 29 17:02 mouse0" if present, null if not
  // So, if there is a response, return 0.

  /* Read the output a line at a time - output it. */
  while (fgets(response_line, 250, fp) != NULL)
  {
    if (strlen(response_line) > 1)
    {
      pclose(fp);
      return 0;
    }
  }
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
