#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include "font/font.h"
#include "graphics.h"

#include <getopt.h>
#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include <errno.h>
#include <png.h>

#define DEFAULT_NAME "/home/pi/tmp/snapshot.png"

char *fbp = 0;
char *back_fbp = 0;
char *cursor_fbp = 0;
int fbfd = 0;
long int screenSize = 0;
int screenXsize = 0;
int screenYsize = 0;
//int currentX = 0;
//int currentY = 0;
int textSize= 1 ;

extern bool mouse_active;                     // Only set with no touchscreen when mouse has first moved
extern int mouse_x;                           // click x 0 - 799 from left
extern int mouse_y;                           // click y 0 - 479 from bottom
const int canvasXsize = 800;
const int canvasYsize = 480;
int Xinvert = 799;
int Yinvert = 479;

extern bool rotate_display;
extern int FBOrientation;
extern bool webcontrol;

int32_t fbBytesPerPixel = 2;  // RGB565
//int32_t fbBytesPerPixel = 4;  // RGBA8888

// Functions not declared externally (not in .h)

int displayChar(const font_t *font_ptr, char c, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB, int currentX, int currentY);

int displayLargeChar(int sizeFactor, const font_t *font_ptr, char c, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB, int currentX, int currentY);

uint8_t waterfall_newcolour[256][3] = 
{
{0,27,32},
{0,29,37},
{0,29,39},
{0,30,40},
{0,31,41},
{0,32,42},
{0,33,43},
{0,34,46},
{0,35,48},
{0,35,49},
{0,36,52},
{0,37,53},
{0,38,55},
{0,38,57},
{0,40,59},
{0,41,61},
{0,42,64},
{0,42,65},
{0,43,68},
{0,43,70},
{0,44,72},
{0,45,74},
{0,46,76},
{0,46,79},
{0,47,81},
{0,47,84},
{0,48,86},
{0,48,88},
{2,49,91},
{4,50,91},
{6,50,95},
{9,50,98},
{11,49,100},
{13,50,102},
{17,50,104},
{19,51,107},
{21,52,109},
{24,52,112},
{28,52,116},
{29,51,118},
{32,53,120},
{33,52,121},
{35,52,124},
{39,53,126},
{40,52,131},
{42,52,132},
{45,52,133},
{47,52,134},
{50,52,137},
{51,53,139},
{55,52,142},
{57,51,144},
{59,50,146},
{61,51,148},
{64,51,150},
{67,50,153},
{69,50,155},
{70,50,155},
{75,49,157},
{76,48,159},
{79,49,160},
{81,48,161},
{83,48,164},
{85,47,166},
{87,46,166},
{90,46,167},
{92,45,168},
{96,46,171},
{97,45,172},
{99,43,172},
{102,43,172},
{104,43,173},
{107,42,175},
{110,42,177},
{112,42,177},
{113,41,177},
{114,40,177},
{117,40,178},
{119,39,178},
{122,39,179},
{124,39,179},
{126,39,178},
{129,38,178},
{130,37,178},
{133,37,179},
{134,37,178},
{137,37,179},
{139,36,179},
{141,36,180},
{142,36,178},
{145,36,179},
{147,36,177},
{148,36,178},
{152,35,177},
{153,35,176},
{155,36,176},
{158,35,175},
{159,36,173},
{161,36,172},
{162,36,171},
{164,36,171},
{166,37,169},
{168,37,167},
{170,37,167},
{171,38,166},
{172,38,166},
{175,39,164},
{177,40,162},
{179,40,161},
{180,41,159},
{181,42,157},
{183,42,156},
{185,43,156},
{187,43,155},
{190,44,153},
{192,46,152},
{193,46,150},
{193,47,147},
{194,47,146},
{197,49,145},
{199,49,143},
{200,50,141},
{201,51,140},
{202,52,138},
{204,53,136},
{206,55,136},
{208,55,134},
{209,57,132},
{210,59,130},
{212,59,129},
{213,59,128},
{214,61,126},
{215,63,124},
{217,65,122},
{218,65,121},
{219,66,120},
{220,67,117},
{221,68,112},
{222,70,111},
{225,71,111},
{226,72,110},
{227,74,108},
{228,75,106},
{229,77,104},
{230,79,102},
{231,80,99},
{232,81,98},
{233,82,95},
{234,83,94},
{235,84,93},
{236,85,90},
{237,87,89},
{238,88,89},
{239,89,88},
{240,91,84},
{241,93,82},
{242,94,81},
{243,95,80},
{243,96,78},
{244,97,77},
{245,99,74},
{246,101,72},
{247,104,70},
{248,106,69},
{249,107,67},
{250,109,66},
{250,109,64},
{250,111,62},
{251,113,60},
{252,115,58},
{252,116,56},
{252,118,55},
{253,120,53},
{254,123,51},
{255,125,50},
{255,126,49},
{255,128,47},
{255,129,46},
{255,130,44},
{255,133,42},
{255,135,41},
{255,138,40},
{255,139,36},
{255,140,36},
{255,142,36},
{255,144,36},
{255,146,35},
{255,150,35},
{255,152,34},
{253,152,34},
{254,153,35},
{253,156,35},
{253,161,36},
{252,160,37},
{251,164,38},
{250,165,39},
{250,168,40},
{249,170,42},
{248,173,44},
{247,174,46},
{247,176,50},
{246,178,53},
{245,179,56},
{245,182,58},
{244,186,60},
{243,187,64},
{241,189,67},
{240,190,69},
{240,192,73},
{239,194,75},
{237,196,78},
{236,199,82},
{234,201,85},
{235,203,92},
{235,205,93},
{233,207,97},
{231,208,92},
{231,211,106},
{230,212,112},
{229,213,115},
{228,216,120},
{228,216,124},
{227,218,127},
{226,220,132},
{226,221,137},
{224,221,140},
{224,222,145},
{224,224,150},
{225,226,156},
{225,228,159},
{224,231,164},
{223,232,167},
{223,233,171},
{225,234,177},
{227,235,183},
{227,236,186},
{228,237,190},
{229,238,195},
{230,238,197},
{231,239,203},
{233,240,207},
{234,241,210},
{236,242,216},
{237,242,220},
{238,243,223},
{240,245,225},
{241,245,230},
{242,246,232},
{245,247,236},
{246,247,239},
{247,248,243},
{248,248,245},
{251,250,248},
{253,252,252},
{255,255,255},
{255,255,255}
};






/***************************************************************************//**
 * @brief Returns the width of a string in a given font in pixels
 *
 * @param Font pointer, string
 *
 * @return Width of string
 * 
*******************************************************************************/
int font_width_string(const font_t *font_ptr, char *string)
{
    uint32_t total_width = 0;
    uint32_t string_length = strlen(string);
    for(uint32_t i = 0; i < string_length; i++)
    {
        total_width += font_ptr->characters[(uint8_t)string[i]].render_width;
    }
    return total_width;
}


/***************************************************************************//**
 * @brief Draws a single character in a given font with given background and 
 *        foreground colours at the position specified
 *
 * @param Font pointer, background colour, foreground colour, position
 *
 * @return the character width
 * 
*******************************************************************************/

int displayChar(const font_t *font_ptr, char c, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB, int currentX, int currentY)
{
  // Draws the character based on currentX and currentY at the bottom line (descenders below)
  int row;
  int col;

  int32_t y_offset; // height of top of character from baseline

  int32_t thisPixelR;  // Values for each pixel
  int32_t thisPixelB;
  int32_t thisPixelG;

  // Calculate scale from background clour to foreground colour
  const int32_t red_contrast = foreColourR - backColourR;
  const int32_t green_contrast = foreColourG - backColourG;
  const int32_t blue_contrast = foreColourB - backColourB;

  // Calculate height from top of character to baseline
  y_offset = font_ptr->ascent;

  // For each row
  for(row = 0; row < font_ptr->characters[(uint8_t)c].height; row++)
  {
    // For each column in the row
    for(col = 0; col < font_ptr->characters[(uint8_t)c].width; col++)
    {
      // For each pixel
      thisPixelR = backColourR + ((red_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);
      thisPixelG = backColourG + ((green_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);
      thisPixelB = backColourB + ((blue_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);


      if ((currentX + col < 800) && (currentY + row - y_offset < 480))
      {
         setPixel(currentX + col, currentY - row + y_offset, thisPixelR, thisPixelG, thisPixelB);
      }
    }
  }
  // Move position on by the character width
  return font_ptr->characters[(uint8_t)c].render_width;
}

int displayCharOnly(const font_t *font_ptr, char c, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB, int currentX, int currentY)
{
  // Draws the character based on currentX and currentY at the bottom line (descenders below)
  int row;
  int col;

  int32_t y_offset; // height of top of character from baseline

  int32_t thisPixelR;  // Values for each pixel
  int32_t thisPixelB;
  int32_t thisPixelG;

  // Calculate scale from background clour to foreground colour
  const int32_t red_contrast = foreColourR - backColourR;
  const int32_t green_contrast = foreColourG - backColourG;
  const int32_t blue_contrast = foreColourB - backColourB;

  // Calculate height from top of character to baseline
  y_offset = font_ptr->ascent;

  // For each row
  for(row = 0; row < font_ptr->characters[(uint8_t)c].height; row++)
  {
    // For each column in the row
    for(col = 0; col < font_ptr->characters[(uint8_t)c].width; col++)
    {
      // For each pixel
      if ((int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)] > 0)  // leave background unchanged
      {

        thisPixelR = backColourR + ((red_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);
        thisPixelG = backColourG + ((green_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);
        thisPixelB = backColourB + ((blue_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);


        if ((currentX + col < 800) && (currentY + row - y_offset < 480))
        {
           setPixel(currentX + col, currentY - row + y_offset, thisPixelR, thisPixelG, thisPixelB);
        }
      }
    }
  }
  // Move position on by the character width
  return font_ptr->characters[(uint8_t)c].render_width;
}

int displayLargeChar(int sizeFactor, const font_t *font_ptr, char c, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB, int currentX, int currentY)
{
  // Draws the character based on currentX and currentY at the bottom line (descenders below)
  // magnified by sizeFactor
  int row;
  int col;
  int extrarow;
  int extracol;
  int pixelX;
  int pixelY;

  int32_t y_offset; // height of top of character from baseline

  int32_t thisPixelR;  // Values for each pixel
  int32_t thisPixelB;
  int32_t thisPixelG;

  // Calculate scale from background colour to foreground colour
  const int32_t red_contrast = foreColourR - backColourR;
  const int32_t green_contrast = foreColourG - backColourG;
  const int32_t blue_contrast = foreColourB - backColourB;

  // Calculate height from top of character to baseline
  y_offset = font_ptr->ascent * sizeFactor;

  // For each row
  for(row = 0; row < font_ptr->characters[(uint8_t)c].height; row++)
  {
    // For each column in the row
    for (col = 0; col < font_ptr->characters[(uint8_t)c].width; col++)
    {
      // For each pixel
      thisPixelR = backColourR + ((red_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);
      thisPixelG = backColourG + ((green_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);
      thisPixelB = backColourB + ((blue_contrast * (int32_t)font_ptr->characters[(uint8_t)c].map[col+(row*font_ptr->characters[(uint8_t)c].width)]) / 0xFF);

      for(extrarow = 0; extrarow < sizeFactor; extrarow++)
      {
        for(extracol = 0; extracol < sizeFactor; extracol++)
        {
          pixelX = currentX + (col * sizeFactor) + extracol;
          pixelY = currentY - extrarow - (row * sizeFactor) + y_offset;
          if ((pixelX < 800) && (pixelY < 480))
          {
            setPixel(pixelX, pixelY, thisPixelR, thisPixelG, thisPixelB);
          }
          else
          {
            printf("Error: Trying to write pixel outside screen bounds.\n");
          }
        }
      }
    }
  }
  // Move position on by the character width
  return font_ptr->characters[(uint8_t)c].render_width * sizeFactor;
}


void TextMid (int xpos, int ypos, char*s, const font_t *font_ptr, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB)
{
  int sx;   // String width in pixels
  int p;    // Character Counter
  p=0;
  int incremental_xpos;
  int increment;

  // Calculate String Length and position string write position start
  sx = (font_width_string(font_ptr, s)) / 2;  

  // Position string write position start
  incremental_xpos = xpos - sx;

  //printf("TextMid sx %d, x %d, y %d, %s\n", sx, xpos, ypos, s);

  // Display each character
  do
  {
    char c=s[p++];
    increment = displayChar(font_ptr, c, backColourR, backColourG, backColourB, foreColourR, foreColourG, foreColourB, incremental_xpos, ypos);
    incremental_xpos = incremental_xpos + increment;
  }
  while(s[p] != 0);  // While not end of string
}

void Text (int xpos, int ypos, char*s, const font_t *font_ptr, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB)
{
  int p;    // Character Counter
  char c;
  int incremental_xpos;
  int increment;

  // Position string write position start
  incremental_xpos = xpos;

  //printf("TextMid x %d, y %d, %s\n", xpos, ypos, s);

  // Display each character
  for (p = 0; p < strlen(s); p++)
  {
    //printf("%c\n", c);
    c = s[p];
    increment = displayChar(font_ptr, c, backColourR, backColourG, backColourB, foreColourR, foreColourG, foreColourB, incremental_xpos, ypos);
    incremental_xpos = incremental_xpos + increment;
  }
}


void TextOnly (int xpos, int ypos, char*s, const font_t *font_ptr, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB)
{
  int p;    // Character Counter
  char c;
  int incremental_xpos;
  int increment;

  // Position string write position start
  incremental_xpos = xpos;

  //printf("TextMid x %d, y %d, %s\n", xpos, ypos, s);

  // Display each character
  for (p = 0; p < strlen(s); p++)
  {
    //printf("%c\n", c);
    c = s[p];
    increment = displayCharOnly(font_ptr, c, backColourR, backColourG, backColourB, foreColourR, foreColourG, foreColourB, incremental_xpos, ypos);
    incremental_xpos = incremental_xpos + increment;
  }
}


void LargeText (int xpos, int ypos, int sizeFactor, char*s, const font_t *font_ptr, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB)
{
  int p;    // Character Counter
  p=0;
  int incremental_xpos;
  int increment;

  // Position string write position start
  incremental_xpos = xpos;

  //printf("TextMid x %d, y %d, %s\n", xpos, ypos, s);

  // Display each character
  do
  {
    char c = s[p++];
    increment = displayLargeChar(sizeFactor, font_ptr, c, backColourR, backColourG, backColourB, foreColourR, foreColourG, foreColourB, incremental_xpos, ypos);
    incremental_xpos = incremental_xpos + increment;
  }
  while(s[p] != 0);  // While not end of string
}


void MsgBox4(char *message1, char *message2, char *message3, char *message4)
{
  // Display a 4-line message

  const font_t *font_ptr = &font_dejavu_sans_32;
  int txtht =  font_ptr->ascent;
  int linepitch = (14 * txtht) / 10;

  clearScreen(0, 0, 0);
  TextMid(canvasXsize / 2, canvasYsize - (linepitch * 2), message1, font_ptr, 0, 0, 0, 255, 255, 255);
  TextMid(canvasXsize / 2, canvasYsize - 2 * (linepitch * 2), message2, font_ptr, 0, 0, 0, 255, 255, 255);
  TextMid(canvasXsize / 2, canvasYsize - 3 * (linepitch * 2), message3, font_ptr, 0, 0, 0, 255, 255, 255);
  TextMid(canvasXsize / 2, canvasYsize - 4 * (linepitch * 2), message4, font_ptr, 0, 0, 0, 255, 255, 255);

  publish();
  UpdateWeb();
}


void rectangle(int xpos, int ypos, int xsize, int ysize, int r, int g, int b)
{
  // xpos and y pos are 0 to screensize (479 799).  (0, 0) is bottom left
  // xsize and ysize are 1 to 800 (x) or 480 (y)
  int x;     // pixel count
  int y;     // pixel count
  int p;     // Pixel Memory offset

  char gb;   // gggBBBbb RGB565 byte 2 (written first)
  char rg;   // RRRrrGGG RGB565 byte 1 (written second)

  // Convert to RGB565
  gb = ((g & 0x1C) << 3) | (b  >> 3); // Take middle 3 Bits of G component and top 5 bits of Blue component
  rg = (r & 0xF8) | (g >> 5);         // Take top 5 bits of Red component and top 3 bits of G component

  // Check rectangle bounds to save checking each pixel
  if (((xpos < 0) || (xpos > screenXsize))
    || ((xpos + xsize < 0) || (xpos + xsize > screenXsize))
    || ((ypos < 0) || (ypos > screenYsize))
    || ((ypos + ysize < 0) || (ypos + ysize > screenYsize)))
  {
    printf("Rectangle xpos %d xsize %d ypos %d ysize %d tried to write off-screen\n", xpos, xsize, ypos, ysize);
  }
  else
  {
    for(x = 0; x < xsize; x++)  // for each vertical slice along the x
    {
      for(y = 0; y < ysize; y++)  // Draw a vertical line
      {
        if (FBOrientation == 0)
        {
          p = (xpos + x + screenXsize * (Yinvert - (ypos + y))) * 2;
        }
        else if (FBOrientation == 180)
        {
          p = (Xinvert - (xpos + x) + screenXsize * (ypos + y)) * 2;
        }
        //memset(fbp + p,     gb, 1); 
        //memset(fbp + p + 1, rg, 1);
        memset(back_fbp + p,     gb, 1); 
        memset(back_fbp + p + 1, rg, 1);
      }
    }
  }
}


void clearScreen(uint8_t r, uint8_t g, uint8_t b)
{
  int p;    // Pixel Memory offset
  int x;    // x pixel count, 0 - 799
  int y;    // y pixel count, 0 - 479

  char gb;  // gggBBBbb RGB565 byte 2 (written first)
  char rg;  // RRRrrGGG RGB565 byte 1 (written second)

  gb = ((g & 0x1C) << 3) | (b  >> 3); // Take middle 3 Bits of G component and top 5 bits of Blue component
  rg = (r & 0xF8) | (g >> 5);         // Take top 5 bits of Red component and top 3 bits of G component

  for(y = 0; y < screenYsize; y++)
  {
    for(x = 0; x < screenXsize; x++)
    {
      p = (x + screenXsize * y) * 2;

      //memset(fbp + p,     gb, 1); 
      //memset(fbp + p + 1, rg, 1);
      memset(back_fbp + p,     gb, 1); 
      memset(back_fbp + p + 1, rg, 1);
    }
  }
  publish();
}

void clearBackBuffer(uint8_t r, uint8_t g, uint8_t b)
{
  int p;    // Pixel Memory offset
  int x;    // x pixel count, 0 - 799
  int y;    // y pixel count, 0 - 479

  char gb;  // gggBBBbb RGB565 byte 2 (written first)
  char rg;  // RRRrrGGG RGB565 byte 1 (written second)

  gb = ((g & 0x1C) << 3) | (b  >> 3); // Take middle 3 Bits of G component and top 5 bits of Blue component
  rg = (r & 0xF8) | (g >> 5);         // Take top 5 bits of Red component and top 3 bits of G component

  for(y = 0; y < screenYsize; y++)
  {
    for(x = 0; x < screenXsize; x++)
    {
      p = (x + screenXsize * y) * 2;

      //memset(fbp + p,     gb, 1); 
      //memset(fbp + p + 1, rg, 1);
      memset(back_fbp + p,     gb, 1); 
      memset(back_fbp + p + 1, rg, 1);
    }
  }
}

void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  int p;      // Pixel Memory offset
  int inv_y;  // y pos from bottom left
  char gb;    // gggBBBbb RGB565 byte 2 (written first)
  char rg;    // RRRrrGGG RGB565 byte 1 (written second)

  gb = ((g & 0x1C) << 3) | (b  >> 3); // Take middle 3 Bits of G component and top 5 bits of Blue component
  rg = (r & 0xF8) | (g >> 5);         // Take top 5 bits of Red component and top 3 bits of G component

  inv_y = 479 - y;

  if (FBOrientation == 180)
  {
    x = Xinvert - x;
    inv_y = Yinvert - inv_y;
  }

  if ((x < screenXsize) && (inv_y < screenYsize) && (x >= 0) && (inv_y >= 0))
  {
    p=(x + screenXsize * inv_y) * 2;

    //memset(fbp + p,     gb, 1); 
    //memset(fbp + p + 1, rg, 1);
    memset(back_fbp + p,     gb, 1); 
    memset(back_fbp + p + 1, rg, 1);
  }
  else
  {
    printf("Error: setPixelTrying to write pixel outside screen bounds. %d, %d\n", x, inv_y);
  }
}


void setInvPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  int p;      // Pixel Memory offset
  int inv_y;  // y pos from bottom left
  char gb;    // gggBBBbb RGB565 byte 2 (written first)
  char rg;    // RRRrrGGG RGB565 byte 1 (written second)

  gb = ((g & 0x1C) << 3) | (b  >> 3); // Take middle 3 Bits of G component and top 5 bits of Blue component
  rg = (r & 0xF8) | (g >> 5);         // Take top 5 bits of Red component and top 3 bits of G component

  inv_y = y;

  if (FBOrientation == 180)
  {
    x = Xinvert - x;
    inv_y = Yinvert - inv_y;
  }

  if ((x < screenXsize) && (inv_y < screenYsize) && (x >= 0) && (inv_y >= 0))
  {
    p=(x + screenXsize * inv_y) * 2;

    //memset(fbp + p,     gb, 1); 
    //memset(fbp + p + 1, rg, 1);
    memset(back_fbp + p,     gb, 1); 
    memset(back_fbp + p + 1, rg, 1);
  }
  else
  {
    printf("Error: setPixelTrying to write pixel outside screen bounds. %d, %d\n", x, inv_y);
  }
}


void setLargePixel(int x, int y, int size, uint8_t R, uint8_t G, uint8_t B)
{
  int px; // Horizontal pixel counter within large pixel
  int py; // Vertical pixel counter within large pixel

  for (px = 0; px < size; px++)
  {
    for (py=0; py < size; py++)
    {
      setPixel(x + px, y + py, R, G, B);
    }
  }
}


void HorizLine(int xpos, int ypos, int xsize, int r, int g, int b)
{
  // xpos and y pos are 0 to screensize (479 799).  (0, 0) is bottom left
  // xsize is 1 to 800

  rectangle(xpos, ypos, xsize, 1, r, g, b);
}


void VertLine(int xpos, int ypos, int ysize, int r, int g, int b)
{
  // xpos and y pos are 0 to screensize (479 799).  (0, 0) is bottom left
  // ysize is 1 to 480 

  rectangle(xpos, ypos, 1, ysize, r, g, b);
}


void DrawAALine(int x1, int y1, int x2, int y2, int rb, int gb, int bb, int rf, int gf, int bf)
{
  float yIntersect;
  float xIntersect;
  float yIntersect_offset;
  float xIntersect_offset;
  int x;
  int y;
  int r = 0;
  int g = 0;
  int b = 0;
  int r_contrast;
  int g_contrast;
  int b_contrast;
  //int p;     // Pixel Memory offset

  // Check the line bounds first
  if (((x1 < 0) || (x1 > screenXsize))
   || ((x2 < 0) || (x2 > screenXsize))
   || ((y1 < 1) || (y1 > screenXsize - 1))
   || ((y2 < 1) || (y2 > screenXsize - 1)))
  {
    printf("DrawLine x1 %d y1 %d x2 %d y2 %d tried to write off-screen\n",x1, y1, x2, y2);
  }
  else
  {
    //printf("DrawLine x1 %d y1 %d x2 %d y2 %d tried to write\n",x1, y1, x2, y2);

    if ((abs(x2 - x1) == abs(y2 - y1)) && (y2 - y1 != 0))  // 45 degree line
    {
      // No smoothing required, so
      r = rf;
      g = gf;
      b = bf;
      if (x1 < x2)  // 45 degree line on right hand side
      {
        //printf("45 RHS x %d y %d\n", x2 - x1, y2 - y1);
        for (x = x1; x <= x2; x++)
        {
          y = y1 + (x - x1) * (y2 - y1)/abs(y2 - y1);

          //p = (x + screenXsize * (479 - y)) * 4;

          setPixel(x, y, r, g, b);

          //memset(p + fbp,     b, 1);     // Blue
          //memset(p + fbp + 1, g, 1);     // Green
          //memset(p + fbp + 2, r, 1);     // Red
          //memset(p + fbp + 3, 0x80, 1);  // A
        }
      }
      else  // 45 degree line on left hand side
      {
        //printf("45 LHS x %d y %d\n", x2 - x1, y2 - y1);
        for (x = x1; x >= x2; x--)
        {
          y = y1 + (x1 - x) * (y2 - y1)/abs(y2 - y1);
          //p = (x + screenXsize * (479 - y)) * 4;

          setPixel(x, y, r, g, b);

          //memset(p + fbp,     b, 1);     // Blue
          //memset(p + fbp + 1, g, 1);     // Green
          //memset(p + fbp + 2, r, 1);     // Red
          //memset(p + fbp + 3, 0x80, 1);  // A
        }
      }
      return; // 45 degree line drawn.  Nothing else to do
    }
     
    // Calculate contrast
    r_contrast = rf - rb;
    g_contrast = gf - gb;
    b_contrast = bf - bb;

    if (abs(x2 - x1) > abs(y2 - y1))  // Less than 45 degree gradient
    {
      if (x2 > x1)  // Rightwards line
      {
        //printf("Rightwards < 45 x %d y %d\n", x2 - x1, y2 - y1);
        for (x = x1; x <= x2; x++)
        {

          yIntersect = (float)y1 + (float)(y2 - y1) * (float)(x - x1) / (float)(x2 - x1);
          y = (int)yIntersect;
          yIntersect_offset = yIntersect - (float)y;

          r = rf + (int)((1.0 - yIntersect_offset) * (float)r_contrast);
          g = gf + (int)((1.0 - yIntersect_offset) * (float)g_contrast);
          b = bf + (int)((1.0 - yIntersect_offset) * (float)b_contrast);

          setPixel(x, y, r, g, b);

          //p = (x + screenXsize * (479 - y)) * 4;

          //memset(p + fbp,     b, 1);     // Blue
          //memset(p + fbp + 1, g, 1);     // Green
          //memset(p + fbp + 2, r, 1);     // Red
          //memset(p + fbp + 3, 0x80, 1);  // A

          if (yIntersect_offset > 0.004)
          {
            r = rf + (int)(yIntersect_offset * (float)r_contrast);
            g = gf + (int)(yIntersect_offset * (float)g_contrast);
            b = bf + (int)(yIntersect_offset * (float)b_contrast);

            y = y + 1;

            setPixel(x, y, r, g, b);

            //p = (x + screenXsize * (479 - y)) * 4;

            //memset(p + fbp,     b, 1);     // Blue
            //memset(p + fbp + 1, g, 1);     // Green
            //memset(p + fbp + 2, r, 1);     // Red
            //memset(p + fbp + 3, 0x80, 1);  // A
          }
        }
      }
      else if (x1 > x2) // Leftwards line
      {
        //printf("Leftwards < 45 x %d y %d\n", x2 - x1, y2 - y1);
        for (x = x1; x >= x2; x--)
        {
          yIntersect = (float)y1 + (float)(y2 - y1) * (float)(x - x1) / (float)(x2 - x1);
          y = (int)yIntersect;
          yIntersect_offset = yIntersect - (float)y;

          r = rf + (int)((1.0 - yIntersect_offset) * (float)r_contrast);
          g = gf + (int)((1.0 - yIntersect_offset) * (float)g_contrast);
          b = bf + (int)((1.0 - yIntersect_offset) * (float)b_contrast);

          setPixel(x, y, r, g, b);

          //p = (x + screenXsize * (479 - y)) * 4;

          //memset(p + fbp,     b, 1);     // Blue
          //memset(p + fbp + 1, g, 1);     // Green
          //memset(p + fbp + 2, r, 1);     // Red
          //memset(p + fbp + 3, 0x80, 1);  // A

          if (yIntersect_offset > 0.004)
          {
            r = rf + (int)(yIntersect_offset * (float)r_contrast);
            g = gf + (int)(yIntersect_offset * (float)g_contrast);
            b = bf + (int)(yIntersect_offset * (float)b_contrast);

            y = y + 1;

            setPixel(x, y, r, g, b);

            //p = (x + screenXsize * (479 - y)) * 4;

            //memset(p + fbp,     b, 1);     // Blue
            //memset(p + fbp + 1, g, 1);     // Green
            //memset(p + fbp + 2, r, 1);     // Red
            //memset(p + fbp + 3, 0x80, 1);  // A
          }
        }
      }
      else //  x1 = x2 so vertical line

      {
         printf("need to draw vertical\n");
      }
    }


    if (abs(x2 - x1) < abs(y2 - y1))  // Nearer the vertical
    {
      if (y2 > y1)  // Upwards line
      {
        //printf("Upwards > 45 x %d y %d\n", x2 - x1, y2 - y1);
        for (y = y1; y <= y2; y++)
        {
          xIntersect = (float)x1 + (float)(x2 - x1) * (float)(y - y1) / (float)(y2 - y1);
          x = (int)xIntersect;
          xIntersect_offset = xIntersect - (float)x;

          r = rf + (int)((1.0 - xIntersect_offset) * (float)r_contrast);
          g = gf + (int)((1.0 - xIntersect_offset) * (float)g_contrast);
          b = bf + (int)((1.0 - xIntersect_offset) * (float)b_contrast);

          setPixel(x, y, r, g, b);

          //p = (x + screenXsize * (479 - y)) * 4;

          //memset(p + fbp,     b, 1);     // Blue
          //memset(p + fbp + 1, g, 1);     // Green
          //memset(p + fbp + 2, r, 1);     // Red
          //memset(p + fbp + 3, 0x80, 1);  // A

          if (xIntersect_offset > 0.004)
          {
            r = rf + (int)(xIntersect_offset * (float)r_contrast);
            g = gf + (int)(xIntersect_offset * (float)g_contrast);
            b = bf + (int)(xIntersect_offset * (float)b_contrast);

            x = x + 1;
           
            setPixel(x, y, r, g, b);

            //p = (x + screenXsize * (479 - y)) * 4;

            //memset(p + fbp,     b, 1);     // Blue
            //memset(p + fbp + 1, g, 1);     // Green
            //memset(p + fbp + 2, r, 1);     // Red
            //memset(p + fbp + 3, 0x80, 1);  // A
          }
        }
      }
      else  // Downwards line
      {
        //printf("Downwards > 45 x %d y %d\n", x2 - x1, y2 - y1);
        for (y = y1; y >= y2; y--)
        {
          xIntersect = (float)x1 + (float)(x2 - x1) * (float)(y - y1) / (float)(y2 - y1);
          x = (int)xIntersect;
          xIntersect_offset = xIntersect - (float)x;

          r = rf + (int)((1.0 - xIntersect_offset) * (float)r_contrast);
          g = gf + (int)((1.0 - xIntersect_offset) * (float)g_contrast);
          b = bf + (int)((1.0 - xIntersect_offset) * (float)b_contrast);

          setPixel(x, y, r, g, b);

          //p = (x + screenXsize * (479 - y)) * 4;

          //memset(p + fbp,     b, 1);     // Blue
          //memset(p + fbp + 1, g, 1);     // Green
          //memset(p + fbp + 2, r, 1);     // Red
          //memset(p + fbp + 3, 0x80, 1);  // A

          if (xIntersect_offset > 0.004)
          {
            r = rf + (int)(xIntersect_offset * (float)r_contrast);
            g = gf + (int)(xIntersect_offset * (float)g_contrast);
            b = bf + (int)(xIntersect_offset * (float)b_contrast);

            x = x + 1;

            setPixel(x, y, r, g, b);

            //p = (x + screenXsize * (479 - y)) * 4;

            //memset(p + fbp,     b, 1);     // Blue
            //memset(p + fbp + 1, g, 1);     // Green
            //memset(p + fbp + 2, r, 1);     // Red
            //memset(p + fbp + 3, 0x80, 1);  // A
          }
        }
      }
    }
  }
}


void MarkerGrn(int markerx, int x, int y)
{
  int p;  // Pixel Memory offset
  int row; // Marker Row
  int disp; // displacement from marker#

  disp = x - markerx;

//printf ("marker x = %d, y = %d disp  = %d\n", markerx, y, disp);

  if ((disp > -11 ) && (disp < 11))
  {
    if (disp <= 0)  // left of marker
    {
      for ( row = (11 + disp); row > 0; row--)
      {
        if ((y - 11 + row) > 10)  // on screen
        {
          //p = (x + screenXsize * (y - 11 + row)) * 4;
          if (FBOrientation == 180)
          {
            p = ((Xinvert - x) + screenXsize * (Yinvert - (y - 11 + row))) * 2;
          }
          else
          {
            p = (x + screenXsize * (y - 11 + row)) * 2;
          }
          memset(back_fbp + p + 1, 0x07, 1);
        }
      }
    }
    else // right of marker
    {
      for ( row = 1; row < (12 - disp); row++)
      {
        if ((y - 11 + row) > 10)  // on screen
        {
          //p = (x + screenXsize * (y - 11 + row)) * 4;
          if (FBOrientation == 180)
          {
            p = ((Xinvert - x) + screenXsize * (Yinvert - (y - 11 + row))) * 2;
          }
          else
          {
            p = (x + screenXsize * (y - 11 + row)) * 2;
          }
          memset(back_fbp + p + 1, 0x07, 1);
        }
      }
    }
  }
}


screen_pixel_t waterfall_map(uint8_t value)
{
  screen_pixel_t result;

  result.Alpha = 0x80;
  result.Red = waterfall_newcolour[value][0];
  result.Green = waterfall_newcolour[value][1];
  result.Blue = waterfall_newcolour[value][2];

  return result;
}


void closeScreen(void)
{
  munmap(fbp, screenSize);
  munmap(cursor_fbp, screenSize);
  munmap(back_fbp, screenSize);
  close(fbfd);
}


int initScreen(void)
{

  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;
 
  fbfd = open("/dev/fb0", O_RDWR);
  if (!fbfd) 
  {
    printf("Error: cannot open framebuffer device.\n");
    return(0);
  }

  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) 
  {
    printf("Error reading fixed information.\n");
    return(0);
  }

  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) 
  {
    printf("Error reading variable information.\n");
    return(0);
  }
  
  screenXsize = vinfo.xres;

  screenYsize = vinfo.yres;

  printf("Screen %d x %d\n", screenXsize, screenYsize);
  
  screenSize = finfo.smem_len;
  fbp = (char*)mmap(0, screenSize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

  // Create frame store for use to build the background
  back_fbp = malloc(screenSize);

  // Create frame store for use to build the cursor
  cursor_fbp = (char*)malloc(screenSize);

  if(fbp != NULL) 
  {
    return 0;
  }
  else 
  {
    return 1;
  }
}


void setCursorPixel(int x, int y, int level)
{
  int p;  // Pixel Memory offset
  int inv_y = 0;
  if ((x < 800) && (y < 480) && (x >= 0) && (y >= 0))
  {
    p=(x + screenXsize * y) * 2;

    char gb;    // gggBBBbb RGB565 byte 2 (written first)
    char rg;    // RRRrrGGG RGB565 byte 1 (written second)

    gb = ((level & 0x1C) << 3) | (level  >> 3); // Take middle 3 Bits of G component and top 5 bits of Blue component
    rg = (level & 0xF8) | (level >> 5);         // Take top 5 bits of Red component and top 3 bits of G component

    inv_y = 479 - y;

    if (FBOrientation == 180)
    {
      x = Xinvert - x;
      inv_y = Yinvert - inv_y;
    }

    p=(x + screenXsize * inv_y) * 2;

    memset(cursor_fbp + p,     gb, 1); 
    memset(cursor_fbp + p + 1, rg, 1);

    //memset(cursor_fbp + p, level, 1);         // Blue
    //memset(cursor_fbp + p + 1, level, 1);     // Green
    //memset(cursor_fbp + p + 2, level, 1);     // Red
    //memset(cursor_fbp + p + 3, 0x80, 1);  // A
  }
  else
  {
    printf("Error: setCursorPixel Trying to write pixel outside screen bounds. %d, %d \n", x, inv_y);
  }
}


int cursorLine0[3] = {110, 38, 124}; 
int cursorLine1[5] = {143, 36, 155, 22, 155}; 
int cursorLine2[5] = {82, 127, 255, 96, 110}; 
int cursorLine3[5] = {84, 128, 255, 100, 112};
int cursorLine4[5] = {85, 128, 255, 99, 116};
int cursorLine5[6] = {85, 127, 255, 103, 88, 216};
int cursorLine6[9] = {85, 127, 255, 114, 9, 15, 57, 212, 236};
int cursorLine7[12] = {85, 127, 255, 101, 104, 212, 80, 12, 35, 74, 197, 237};
int cursorLine8[14] = {85, 127, 255, 99, 15, 255, 180, 49, 207, 65, 6, 33, 76, 222};
int cursorLine9[17] = {231, 223, 255, 88, 127, 255, 97, 110, 255, 174, 52, 255, 190, 41, 208, 91, 61};
int cursorLine10[18] = {170, 32, 28, 100, 70, 131, 255, 105, 117, 255, 179, 64, 255, 186, 45, 255, 221, 38};
int cursorLine11[18] = {39, 148, 210, 47, 0, 141, 255, 234, 255, 255, 245, 230, 255, 187, 54, 255, 220, 42};
int cursorLine12[18] = {47, 145, 255, 207, 0, 137, 255, 255, 255, 255, 255, 255, 255, 242, 223, 255, 218, 42};
int cursorLine13[18] = {187,  13, 165, 255,  63, 132, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 213,  41};
int cursorLine14[17] = {     144,  14, 199, 231, 229, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 204,  43};
int cursorLine15[16] = {          110, 249, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 185,  54};
int cursorLine16[16] = {          245,  59,  99, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144,  77};
int cursorLine17[15] = {               189,  11, 221, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  83, 124};
int cursorLine18[14] = {                     80, 101, 255, 255, 255, 255, 255, 255, 255, 255, 255, 237,  31, 199};
int cursorLine19[14] = {                    181,  39, 246, 255, 255, 255, 255, 255, 255, 255, 255, 170,  28, 247};
int cursorLine20[13] = {                    229,  30, 213, 255, 255, 255, 255, 255, 255, 255, 255, 103,  82};
int cursorLine21[13] = {                    254,  50, 167, 255, 255, 255, 255, 255, 255, 255, 253,  44, 165};
int cursorLine22[12] = {                          78, 121, 236, 232, 232, 232, 232, 232, 235, 199,  31, 219};
int cursorLine23[12] = {                         116,  24,  38,  37,  36,  36,  37,  36,  37,  29,  59, 247};


void moveCursor(int new_x, int new_y)
{
  int vert;
  int cursor_height = 24;
  int cursor_width = 18;
  int cursor_spot_x = 6;

  int xoffset;
  int yoffset;
  int fbpBytes = 0;

  static int old_x;
  static int old_y;
  static int stage_x;
  static int stage_y;

//  int cursor_spot_y = 23;
  int redraw_min_x = 0;
  int redraw_max_x = 799;
  int redraw_min_y = 0;
  int redraw_max_y = 0;

  //printf("moveCursor old_x %d, old_y %d, new_x %d, new_y %d\n", old_x, old_y, new_x, new_y);


  // Calculate rewrite area y dimensions
  if (new_y > old_y)               // cursor moving up
  {
    redraw_max_y = new_y;
    redraw_min_y = old_y - cursor_height;
  }
  else if (new_y < old_y)         // cursor moving down
  {
    redraw_max_y = old_y;
    redraw_min_y = new_y - cursor_height;
  }
  else                           // cursor not moving up or down
  {
    redraw_max_y = old_y;
    redraw_min_y = old_y - cursor_height;
  }

  // Check limits
  if (redraw_max_y < 0)
  {
    redraw_max_y = 0;
  }
  if (redraw_max_y > 479)
  {
    redraw_max_y = 479;
  }
  if (redraw_min_y < 0)
  {
    redraw_min_y = 0;
  }
  if (redraw_min_y > 479)
  {
    redraw_min_y = 479;
  }

  // Calculate rewrite area x dimensions
  if (new_x > old_x)               // cursor moving right
  {
    redraw_max_x = new_x + cursor_width - cursor_spot_x;
    redraw_min_x = old_x - cursor_spot_x;
  }
  else if (new_x < old_x)         // cursor moving left
  {
    redraw_max_x = old_x + cursor_width - cursor_spot_x;
    redraw_min_x = new_x - cursor_spot_x;
  }
  else                           // cursor not moving left or right
  {
    redraw_max_x = new_x + cursor_width - cursor_spot_x;
    redraw_min_x = new_x - cursor_spot_x;
  }

  // Check limits
  if (redraw_max_x < 0)
  {
    redraw_max_x = 0;
  }
  if (redraw_max_x > 799)
  {
    redraw_max_x = 799;
  }
  if (redraw_min_x < 0)
  {
    redraw_min_x = 0;
  }
  if (redraw_min_x > 799)
  {
    redraw_min_x = 799;
  }

  // Reverse orientation if required
  if (FBOrientation == 180)
  {
    int temp_redraw_min_x = redraw_min_x;
    redraw_min_x = 799 - redraw_max_x;
    redraw_max_x = 799 - temp_redraw_min_x;

    int temp_redraw_min_y = redraw_min_y;
    redraw_min_y = 479 - redraw_max_y;
    redraw_max_y = 479 - temp_redraw_min_y;
  }

  //printf("moveCursor old_x %d, old_y %d, new_x %d, new_y %d, screenXsize %d\n", old_x, old_y, new_x, new_y, screenXsize);
  //printf("moveCursor redraw_min_x %d, redraw_max_x %d, redraw_min_y %d, redraw_max_y %d\n", redraw_min_x, redraw_max_x, redraw_min_y, redraw_max_y);

  // Calculate byte offset and number of bytes along line for x
  xoffset = redraw_min_x* fbBytesPerPixel;
  fbpBytes = (redraw_max_x - redraw_min_x + 1) * fbBytesPerPixel;

  // copy memory for each line
  for (vert = redraw_max_y; vert >= redraw_min_y ; vert--)  // top line first
  {
    // Calculate y byte offset
    yoffset = (479 - vert) * screenXsize * fbBytesPerPixel;

    memcpy(cursor_fbp + xoffset + yoffset, back_fbp + xoffset + yoffset, fbpBytes);
    //memset(cursor_fbp + xoffset + yoffset, 0xFF, fbpBytes);

    //printf("vert = %d xoffset = %d, yoffset = %d, fbpBytes = %d\n", vert, xoffset, yoffset, fbpBytes);
  }

  //memcpy(cursor_fbp, back_fbp, screenSize/4);

  // Paint the new mouse position into the cursor buffer
  addCursor(new_x, new_y);
  //printf("newX = %d, newY = %d\n", new_x, new_y);

  // Store the mouse position for next time
  old_x = stage_x;
  old_y = stage_y;
  stage_x = new_x;
  stage_y = new_y;

  // Display the Cursor buffer
  memcpy(fbp, cursor_fbp, screenSize);
}


void addCursor(int x, int y)
{
  int vert;
  int horiz;
  int xpixel = 0;
  int ypixel = 0;

  //printf("AddCursor: Mouse_x %d, Mouse_y %d\n", x, y);

  for (vert = 0; vert < 24; vert++)
  {
    ypixel =y - 24 + vert;
    if ((ypixel >= 0) && (ypixel < 480))
    {
      for (horiz = 0; horiz < 18; horiz++)
      {
        xpixel = x - 6  + horiz;
        if ((xpixel >= 0) && (xpixel < 800))
        {
          if ((vert == 23) && (horiz >= 5) && (horiz < 8))
          {
            setCursorPixel(xpixel, ypixel, cursorLine0[horiz - 5]);
          }
          else if ((vert == 22) && (horiz >= 4) && (horiz < 9))
          {
            setCursorPixel(xpixel, ypixel, cursorLine1[horiz - 4]);
          }
          else if ((vert == 21) && (horiz >= 4) && (horiz < 9))
          {
            setCursorPixel(xpixel, ypixel, cursorLine2[horiz - 4]);
          }
          else if ((vert == 20) && (horiz >= 4) && (horiz < 9))
          {
            setCursorPixel(xpixel, ypixel, cursorLine3[horiz - 4]);
          }
          else if ((vert == 19) && (horiz >= 4) && (horiz < 9))
          {
            setCursorPixel(xpixel, ypixel, cursorLine4[horiz - 4]);
          }
          else if ((vert == 18) && (horiz >= 4) && (horiz < 10))
          {
            setCursorPixel(xpixel, ypixel, cursorLine5[horiz - 4]);
          }
          else if ((vert == 17) && (horiz >= 4) && (horiz < 13))
          {
            setCursorPixel(xpixel, ypixel, cursorLine6[horiz - 4]);
          }
          else if ((vert == 16) && (horiz >= 4) && (horiz < 16))
          {
            setCursorPixel(xpixel, ypixel, cursorLine7[horiz - 4]);
          }
          else if ((vert == 15) && (horiz >= 4) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine8[horiz - 4]);
          }
          else if ((vert == 14) && (horiz >= 1) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine9[horiz - 1]);
          }
          else if ((vert == 13) && (horiz >= 0) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine10[horiz]);
          }
          else if ((vert == 12) && (horiz >= 0) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine11[horiz]);
          }
          else if ((vert == 11) && (horiz >= 0) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine12[horiz]);
          }
          else if ((vert == 10) && (horiz >= 0) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine13[horiz]);
          }
          else if ((vert == 9) && (horiz >= 1) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine14[horiz - 1]);
          }
          else if ((vert == 8) && (horiz >= 2) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine15[horiz - 2]);
          }
          else if ((vert == 7) && (horiz >= 2) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine16[horiz - 2]);
          }
          else if ((vert == 6) && (horiz >= 3) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine17[horiz - 3]);
          }
          else if ((vert == 5) && (horiz >= 4) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine18[horiz - 4]);
          }
          else if ((vert == 4) && (horiz >= 4) && (horiz < 18))
          {
            setCursorPixel(xpixel, ypixel, cursorLine19[horiz - 4]);
          }
          else if ((vert == 3) && (horiz >= 4) && (horiz < 17))
          {
            setCursorPixel(xpixel, ypixel, cursorLine20[horiz - 4]);
          }
          else if ((vert == 2) && (horiz >= 4) && (horiz < 17))
          {
            setCursorPixel(xpixel, ypixel, cursorLine21[horiz - 4]);
          }
          else if ((vert == 1) && (horiz >= 5) && (horiz < 17))
          {
            setCursorPixel(xpixel, ypixel, cursorLine22[horiz - 5]);
          }
          else if ((vert == 0) && (horiz >= 5) && (horiz < 17))
          {
            setCursorPixel(xpixel, ypixel, cursorLine23[horiz - 5]);
          }
        }
      }
    }
  }
}


void fb2png()
{
  // fbp is the source framebuffer pointer

  char *pngName = DEFAULT_NAME;
  int32_t pngWidth;
  int32_t pngHeight;

  // set up output buffer for png image data
  int8_t pngBytesPerPixel;
  int32_t pngPitch;
  int32_t fbPitch;

  //char *fbPixelPtr;

  int32_t j;
  int32_t i;

  // Dimensions from initscreen()
  

  //pngWidth = screenXsize;
  //pngHeight = screenYsize;
  pngWidth = 800;
  pngHeight = 480;


  pngBytesPerPixel = 3;
  pngPitch = pngBytesPerPixel * pngWidth;

  char gb;    // gggBBBbb RGB565 byte 2 (written first)
  char rg;    // RRRrrGGG RGB565 byte 1 (written second)
  char r;
  char g;
  char b;

  void *pngImagePtr = malloc(pngPitch * pngHeight);

  //fbPitch = fbBytesPerPixel  * pngWidth;
  fbPitch = fbBytesPerPixel  * screenXsize;
  uint8_t *pngPixelPtr = pngImagePtr;
  char *fbPixelPtr = fbp;


  uint32_t fbXoffset;
  uint32_t fbYoffset;

  if (FBOrientation == 0)
  {
    for (j = 0 ; j < pngHeight ; j++)       // For each line
    {
      fbYoffset = j * fbPitch;              // calculate input framebuffer offset at left hand end of line

      for (i = 0 ; i < pngWidth ; i++)      // for each pixel in the output png
      {

        fbXoffset = i * fbBytesPerPixel;    // calculate the input framebuffer offset for the pixel

        pngPixelPtr = pngImagePtr + (i * pngBytesPerPixel) + (j * pngPitch);     // Set the png pointer

        fbPixelPtr = fbp + fbXoffset + fbYoffset;                                // Set the framebuffer pointer                                

        // take the bytes at *fbPixelPtr and *fbPixelPtr + 1
        gb = *fbPixelPtr;
        fbPixelPtr = fbPixelPtr +1;
        rg = *fbPixelPtr;

        //  Convert RGB565 to RGB888

        r = rg & 0xF8;
        g = ((rg & 0x07) << 5) | ((gb & 0xE0) >> 3);
        b = (gb & 0x1F) << 3;

        // write to png buffer
        memset(pngPixelPtr, r, 1);
        memset(pngPixelPtr + 1, g, 1);
        memset(pngPixelPtr + 2, b, 1);
      }
    }
  }
  else if (FBOrientation == 180)  //   
  {

    for (j = 0 ; j < pngHeight ; j++)       // For each line
    {
      fbYoffset = (pngHeight - 1 - j) * fbPitch;    // calculate input framebuffer offset at left hand end of the line, bottom line first

      for (i = 0 ; i < pngWidth ; i++)      // for each pixel in the output png
      {

        fbXoffset = (pngWidth - 1 - i) * fbBytesPerPixel;    // calculate the input framebuffer offset for the pixel starting at the right

        fbPixelPtr = fbp + fbXoffset + fbYoffset;                                // Set the framebuffer pointer                                

        // take the bytes at *fbPixelPtr and *fbPixelPtr + 1
        gb = *fbPixelPtr;
        fbPixelPtr = fbPixelPtr + 1;
        rg = *fbPixelPtr;

        //  Convert RGB565 to RGB888

        r = rg & 0xF8;
        g = ((rg & 0x07) << 5) | ((gb & 0xE0) >> 3);
        b = (gb & 0x1F) << 3;

        pngPixelPtr = pngImagePtr + (i * pngBytesPerPixel) + (j * pngPitch);     // Set the png pointer

        // write to png buffer
        memset(pngPixelPtr, r, 1);
        memset(pngPixelPtr + 1, g, 1);
        memset(pngPixelPtr + 2, b, 1);
      }
    }
  }

  // now write the png

  png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (pngPtr == NULL)
  {
    printf("unable to allocated PNG write structure\n");
    return;
  }

  png_infop infoPtr = png_create_info_struct(pngPtr);

  if (infoPtr == NULL)
  {
     printf("unable to allocated PNG info structure\n");
     return;
  }

  if (setjmp(png_jmpbuf(pngPtr)))
  {
    printf("unable to create PNG\n");
    return;
  }

  FILE *pngfp = NULL;

  pngfp = fopen(pngName, "wb");

  if (pngfp == NULL)
  {
    printf("unable to create %s - %s\n", pngName, strerror(errno));
    return;
  }

  png_init_io(pngPtr, pngfp);

  png_set_IHDR(
    pngPtr,
    infoPtr,
    pngWidth,
    pngHeight,
    8,
    PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_BASE,
    PNG_FILTER_TYPE_BASE);

  png_write_info(pngPtr, infoPtr);

  for (int y = 0; y < pngHeight; y++)
  {
    png_write_row(pngPtr, pngImagePtr + (pngPitch * y));
  }

  png_write_end(pngPtr, NULL);
  png_destroy_write_struct(&pngPtr, &infoPtr);

  fclose(pngfp);

  free(pngImagePtr);
  pngImagePtr = NULL;
}


void publish()
{
  if (mouse_active == true)
  {
    //printf("PUBLISH with mouse_active true\n");
    // Copy the back buffer to the cursor buffer
    memcpy(cursor_fbp, back_fbp, screenSize);

    // Paint the mouse into the cursor buffer
    addCursor(mouse_x, mouse_y);

    // copy the cursor buffer into the display framebuffer
    memcpy(fbp, cursor_fbp, screenSize);
  }
  else  // copy the backbuffer straight to the framebuffer
  {
    //printf("PUBLISH with mouse_active false\n");
    // Copy the back buffer to the cursor buffer
    memcpy(fbp, back_fbp, screenSize);
  }
}


void publishRectangle(int x, int y, int w, int h)
{
  int xoffset;
  int xlength;
  int linebytes;
  int yoffset;
  int offset;
  int yline;

  // Transform if screen inverted
  if (FBOrientation == 180)
  {
    x = 800 - x - w;
    y = 480 - y - h;
  }

  // define the xoffset and xlength

  xoffset = x * fbBytesPerPixel;
  xlength = w * fbBytesPerPixel;
  linebytes = screenXsize * fbBytesPerPixel;

  // define the yoffset

  yoffset = (479 - y) * screenXsize * fbBytesPerPixel;


  if (mouse_active == true)
  {
    for (yline = 0; yline < h; yline++)
    {
      offset = yoffset - yline * linebytes + xoffset;
      //printf (" with mouse yline = %d, offset = %d\n", yline, offset);

      // Copy the back buffer to the cursor buffer
      memcpy(cursor_fbp + offset, back_fbp + offset, xlength);

      // Paint the mouse into the cursor buffer
      addCursor(mouse_x, mouse_y);

      // copy the cursor buffer into the display framebuffer
      memcpy(fbp + offset, cursor_fbp + offset, xlength);
    }
  }
  else  // copy the backbuffer straight to the framebuffer
  {
    // Copy the back buffer to the cursor buffer
    for (yline = 0; yline < h; yline++)
    {
      //printf ("no mouse yline = %d, offset = %d\n", yline, offset);
      offset = yoffset - yline * linebytes + xoffset;
      memcpy(fbp + offset, back_fbp + offset, xlength);
    }
  }
}


void UpdateWeb()
{
  // Called after any screen update to update the web page if required.

  if(webcontrol == true)
  {
    system("/home/pi/portsdown/scripts/single_screen_grab_for_web.sh &");
  }
}

