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
