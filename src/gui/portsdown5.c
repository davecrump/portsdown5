#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <linux/input.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <time.h>


#include "../common/font/font.h"
#include "../common/touch.h"
#include "../common/graphics.h"
#include "../common/timing.h"
#include "../common/hardware.h"

bool mouse_active = false;             // set true after first movement of mouse
bool MouseClickForAction = false;      // set true on left click of mouse
int mouse_x;                           // click x 0 - 799 from left
int mouse_y;                           // click y 0 - 479 from top

pthread_t thmouse;          //  Listens to the mouse

int main(int argc, char **argv)
{
  printf("start_test\n");

  initScreen();



clearScreen(63, 0, 0);
//rectangle(0, 0, 100, 100, 0, 255, 0);
setLargePixel(200, 200, 5, 0, 255, 0);

rectangle(0, 0, 799, 1, 255, 255, 255);
//rectangle(799, 0, 1, 479, 255, 255, 255);


  const font_t *font_ptr = &font_dejavu_sans_22;

  if (CheckMouse() == 0)
  {
    printf("Mouse Connected\n");
  }
  else
  {
    printf("Mouse not Connected\n");
  }

  //clearScreen();
  LargeText(50, 200, 2, "y Lime y Firmware", font_ptr, 0, 0, 0, 255, 255, 255);

//rectangle(0, 200, 799, 1, 255, 255, 255);

setPixel(1, 1, 255, 255, 255);
setPixel(0, 0, 255, 255, 255);
setPixel(479, 479, 255, 255, 255);

setPixel(478, 478, 255, 255, 255);

setPixel(799, 479, 255, 255, 255);

setPixel(798, 478, 255, 255, 255);


//  for(int i = 0; i < 200; i++)
//  {
//  setPixel(i,0,0,255,0);
//  }

  printf("end_test\n");

}

