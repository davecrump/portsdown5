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


bool mouse_active = false;             // Sets the extern variable in graphics.c
int mouse_x;                           // Sets the extern variable in graphics.c
int mouse_y;                           // Sets the extern variable in graphics.c


static void terminate(int sig)
{
  printf("fb2png terminating abnormally\n");
}


int main(void)
{
  // Catch sigaction and call terminate
  for (int i = 0; i < 16; i++)
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = terminate;
    sigaction(i, &sa, NULL);
  }

  initScreen();
  fb2png();

}

