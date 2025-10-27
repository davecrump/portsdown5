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

#include "../common/font/font.h"
#include "../common/graphics.h"

#define PATH_SCONFIG "/home/pi/portsdown/configs/system_config.txt"

bool webcontrol = true;                // Sets the extern variable in graphics.c
bool mouse_active = false;             // Sets the extern variable in graphics.c
int mouse_x;                           // Sets the extern variable in graphics.c
int mouse_y;                           // Sets the extern variable in graphics.c
int FBOrientation = 0;                 // 0, 90, 180 or 270.  Sets the extern variable in graphics.c
char response[63] = "0";
 
/****************************************************************************//**
 * @brief Looks up the value of a Param in PathConfigFile and sets value
 *        Used to look up the configuration from portsdown_config.txt
 *
 * @param PatchConfigFile (str) the name of the configuration text file
 * @param Param the string labeling the parameter
 * @param Value the looked-up value of the parameter
 *
 * @return void
*******************************************************************************/

void GetConfigParam(char *PathConfigFile, char *Param, char *Value)
{
  char * line = NULL;
  size_t len = 0;
  int read;
  char ParamWithEquals[255];
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");

  FILE *fp=fopen(PathConfigFile, "r");
  if(fp != 0)
  {
    while ((read = getline(&line, &len, fp)) != -1)
    {
      if(strncmp (line, ParamWithEquals, strlen(Param) + 1) == 0)
      {
        strcpy(Value, line+strlen(Param)+1);
        char *p;
        if((p=strchr(Value,'\n')) !=0 ) *p=0; //Remove \n
        break;
      }
    }
  }
  else
  {
    printf("Config file not found \n");
  }
  fclose(fp);
}


static void terminate(int sig)
{
  printf("img2fb terminating abnormally\n");
}


int main(int argc, char** argv)
{
  int i;
  char filename[63] = "";


  // Catch sigaction and call terminate
  for (int i = 0; i < 16; i++)
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = terminate;
    sigaction(i, &sa, NULL);
  }

  // Print usage if no arguments
  if ( argc < 2 )
  {
    printf("Usage: %s <OPTIONS>\n", argv[0]);
    printf("  -f <Image File to be displayed\n");
    return 1;
  }

  // Read in the text to display
  for ( i = 1; i < argc-1; i += 2 ) 
  {
    if      (strcmp(argv[i], "-f") == 0) { strcpy(filename, argv[i + 1]); }
  }


  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "fborientation", response);
  FBOrientation = atoi(response);

  initScreen();
  img2fb(filename);
}

