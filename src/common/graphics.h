#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__


uint32_t font_width_string(const font_t *font_ptr, char *string);

void displayChar(const font_t *font_ptr, char c, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB);

void displayLargeChar(int sizeFactor, const font_t *font_ptr, char c, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB);

void TextMid (int xpos, int ypos, char*s, const font_t *font_ptr, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB);

void Text (int xpos, int ypos, char*s, const font_t *font_ptr,uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB);

void LargeText (int xpos, int ypos, int sizeFactor, char*s, const font_t *font_ptr, uint8_t backColourR, uint8_t backColourG, uint8_t backColourB, uint8_t foreColourR, uint8_t foreColourG, uint8_t foreColourB);
uint8_t waterfall_newcolour[256][3];

void MsgBox4(char *message1, char *message2, char *message3, char *message4);

typedef struct {
  uint8_t Blue;
  uint8_t Green;
  uint8_t Red;
  uint8_t Alpha; // 0x80
} __attribute__((__packed__)) screen_pixel_t;

screen_pixel_t waterfall_map(uint8_t value);

void rectangle(int xpos, int ypos, int xsize, int ysize, int r, int g, int b);
void gotoXY(int x, int y);
void clearScreen(uint8_t r, uint8_t g, uint8_t b);
void clearBackBuffer(uint8_t r, uint8_t g, uint8_t b);
void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void setLargePixel(int x, int y, int size, uint8_t r, uint8_t g, uint8_t b);
void HorizLine(int xpos, int ypos, int xsize, int r, int g, int b);
void VertLine(int xpos, int ypos, int ysize, int r, int g, int b);
void DrawAALine(int x1, int y1, int x2, int y2, int rb, int gb, int bb, int rf, int gf, int bf);
void closeScreen(void);
int initScreen(void);
void MarkerGrn(int markerx, int x, int y);
void fb2png();
void publishRectangle(int x, int y, int w, int h);
void publish();
void moveCursor(int new_x, int new_y);
void addCursor(int x, int y);

#endif /* __GRAPHICS_H__ */

