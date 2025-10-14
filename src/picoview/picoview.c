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
#include <termios.h>
#include <errno.h>

#include "serial.h"
#include "fft.h"
#include "../common/font/font.h"
#include "../common/touch.h"
#include "../common/graphics.h"
#include "../common/timing.h"
#include "../common/hardware.h"
#include "../common/buffer/buffer_circular.h"
#include "../common/ffunc.h"
#include "../common/LMNspi/LMNspi.h"

pthread_t thbutton;
pthread_t thwebclick;                  //  Listens for clicks from web interface
pthread_t thtouchscreen;               //  listens to the touchscreen   
pthread_t thmouse;                     //  Listens to the mouse
static pthread_t serial_thread_obj;
static pthread_t fft_thread_obj;

int fd = 0;
int wscreen, hscreen;
float scaleXvalue, scaleYvalue; // Coeff ratio from Screen/TouchArea

typedef struct
{
  int r,g,b;
} color_t;

typedef struct
{
  char Text[255];
  color_t  Color;
} status_t;


#define MAX_STATUS 6
typedef struct
{
  int x, y, w, h;                // Position, width and height of button
  status_t Status[MAX_STATUS];   // Array of text and required colour for each status
  int IndexStatus;               // The number of valid status definitions.  0 = do not display
  int NoStatus;                  // This is the active status (colour and text)
} button_t;

// Set the Colours up front
color_t Green = {.r = 0  , .g = 128, .b = 0  };
color_t Blue  = {.r = 0  , .g = 0  , .b = 128};
color_t LBlue = {.r = 64 , .g = 64 , .b = 192};
color_t DBlue = {.r = 0  , .g = 0  , .b = 64 };
color_t Grey  = {.r = 127, .g = 127, .b = 127};
color_t DGrey = {.r = 32 , .g = 32 , .b = 32 };
color_t Red   = {.r = 255, .g = 0  , .b = 0  };
color_t Black = {.r = 0  , .g = 0  , .b = 0  };

#define PATH_PCONFIG "/home/pi/portsdown/configs/portsdown_config.txt"
#define PATH_SCONFIG "/home/pi/portsdown/configs/system_config.txt"
#define PATH_CONFIG "/home/pi/portsdown/configs/picoview_config.txt"

#define MAX_BUTTON 675
int IndexButtonInArray=0;
button_t ButtonArray[MAX_BUTTON];

int CurrentMenu = 1;
int CallingMenu = 1;
char KeyboardReturn[64];

bool NewFreq = false;
bool NewGain = false;
bool NewSpan = false;
bool NewCal  = false;
float gain;

int scaledX, scaledY;

int FBOrientation = 0;                 // 0, 90, 180 or 270

int wbuttonsize = 100;
int hbuttonsize = 50;
int rawX, rawY;
int FinishedButton = 0;
int i;
bool freeze = false;
bool frozen = false;
bool activescan = false;
bool PeakPlot = false;
bool PortsdownExitRequested = false;

int PeakValue[513] = {0};
bool RequestPeakValueZero = false;

int startfreq = 0;
int stopfreq = 0;
int32_t freqoffset = 0;   // 0 kHz for none, positive for low side, negative for high side

char PlotTitle[63] = "-";
bool ContScan = false;

uint32_t centrefreq = 437000;
uint32_t span = 512; 
uint32_t pfreq1 = 146500;
uint32_t pfreq2 = 437000;
uint32_t pfreq3 = 748000;
uint32_t pfreq4 = 1255000;
uint32_t pfreq5 = 2409000;

float smoothing_factor;

int markerx = 250;
int markery = 15;
bool markeron = false;
int markermode = 7;       // 2 peak, 3, null, 4 man, 7 off
int historycount = 0;
int markerxhistory[10];
int markeryhistory[10];
int manualmarkerx = 250;
bool MarkerRefresh = false;
bool waterfall;
bool spectrum;
bool scope = false;
bool invert_scope = false;

bool Range20dB = false;
int BaseLine20dB = -80;
int8_t BaselineShift = 0;

int WaterfallBase;
int WaterfallRange;
uint16_t wfalltimespan = 0;
char TraceType[10] = "single"; // single, average or peak

bool webcontrol = false;   // Enables webcontrol

int tracecount = 0;  // Used for speed testing
int exit_code;

int yscalenum = 18;       // Numerator for Y scaling fraction
int yscaleden = 30;       // Denominator for Y scaling fraction
int yshift    = 5;        // Vertical shift (pixels) for Y

int xscalenum = 25;       // Numerator for X scaling fraction
int xscaleden = 20;       // Denominator for X scaling fraction

bool app_exit = false;

double frequency_actual_rx;
double bandwidth;
extern int y[515];
int y3[515];

bool webclicklistenerrunning = false; // Used to only start thread if required
char WebClickForAction[7] = "no";  // no/yes
char ProgramName[255];             // used to pass prog name char string to listener
int *web_x_ptr;                // pointer
int *web_y_ptr;                // pointer
int web_x;                     // click x 0 - 799 from left
int web_y;                     // click y 0 - 480 from top
int TouchX;
int TouchY;
int TouchPressure;
int TouchTrigger = 0;
bool touchneedsinitialisation = true;
bool touch_inverted = false;
bool FalseTouch = false;     // used to simulate a screen touch if a monitored event finishes

bool mouse_active = false;             // set true after first movement of mouse
bool MouseClickForAction = false;      // set true on left click of mouse
int mouse_x;                           // click x 0 - 799 from left
int mouse_y;                           // click y 0 - 479 from bottom
bool image_complete = true;            // prevents mouse image buffer from being copied until image is complete
bool mouse_connected = false;          // Set true if mouse detected at startup

char DisplayType[31];
bool rotate_display = true;
bool touchscreen_present = false;

int adresult[2047];
int ADCclkDiv_value;
bool dataready = false;

// ******************* External functions for reference *********************

// *************** hardware.c: ***************

// int FindMouseEvent();
// int CheckMouse();

///////////////////////////////////////////// FUNCTION PROTOTYPES ///////////////////////////////

void GetConfigParam(char *PathConfigFile, char *Param, char *Value);
void SetConfigParam(char *PathConfigFile, char *Param, char *Value);
void CheckConfigFile();
void ReadSavedParams();
void *WaitTouchscreenEvent(void * arg);
void *WaitMouseEvent(void * arg);
//void handle_mouse();
void *WebClickListener(void * arg);
void parseClickQuerystring(char *query_string, int *x_ptr, int *y_ptr);
FFUNC touchscreenClick(ffunc_session_t * session);
void take_snap();
void snap_from_menu();
void do_snapcheck();
int IsImageToBeChanged(int x,int y);
void Keyboard(char RequestText[63], char InitText[63], int MaxLength);
int openTouchScreen(int NoDevice);
int getTouchScreenDetails(int *screenXmin, int *screenXmax,int *screenYmin,int *screenYmax);
void TransformTouchMap(int x, int y);
int IsButtonPushed(int NbButton,int x,int y);
int IsMenuButtonPushed(int x,int y);
int InitialiseButtons();
int AddButton(int x,int y,int w,int h);
int ButtonNumber(int MenuIndex, int Button);
int CreateButton(int MenuIndex, int ButtonPosition);
int AddButtonStatus(int ButtonIndex,char *Text,color_t *Color);
void AmendButtonStatus(int ButtonIndex, int ButtonStatusIndex, char *Text, color_t *Color);
void DrawButton(int ButtonIndex);
void SetButtonStatus(int ButtonIndex,int Status);
int getTouchSampleThread(int *rawX, int *rawY, int *rawPressure);
int getTouchSample(int *rawX, int *rawY, int *rawPressure);
void UpdateWindow();
void wait_touch();
void CalculateMarkers();
void SetSpanWidth(int button);
void SetMode(int button);
void SetFreqPreset(int button);
void SetWfall(int button);
void ShiftFrequency(int button);
void CalcSpan();
uint32_t ConstrainFreq(uint32_t CheckFreq);
void ChangeLabel(int button);
void RedrawDisplay();
void *WaitButtonEvent(void * arg);
void Define_Menu1();
void Start_Highlights_Menu1();
void Define_Menu2();
void Start_Highlights_Menu2();
void Define_Menu3();
void Start_Highlights_Menu3();
void Define_Menu4();
void Start_Highlights_Menu4();
void Define_Menu5();
void Start_Highlights_Menu5();
void Define_Menu6();
void Start_Highlights_Menu6();
void Define_Menu7();
void Start_Highlights_Menu7();
void Define_Menu8();
void Start_Highlights_Menu8();
void Define_Menu9();
void Start_Highlights_Menu9();
void Define_Menu10();
void Start_Highlights_Menu10();
void Define_Menu11();
void Define_Menu12();
void Start_Highlights_Menu12();
void Define_Menu41();
void DrawEmptyScreen();
void DrawTickMarks();
void DrawYaxisLabels();
void DrawSettings();
void DrawTrace(int xoffset, int prev2, int prev1, int current);
static void cleanexit(int calling_exit_code);


///////////////////////////////////////////// SETUP FUNCTIONS ////////////////////////


/***************************************************************************//**
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

  //printf("Get Config reads %s for %s ", PathConfigFile , Param);

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

/***************************************************************************//**
 * @brief sets the value of Param in PathConfigFile from a program variable
 *        Used to store the configuration in portsdown_config.txt
 *
 * @param PatchConfigFile (str) the name of the configuration text file
 * @param Param the string labeling the parameter
 * @param Value the looked-up value of the parameter
 *
 * @return void
*******************************************************************************/


void SetConfigParam(char *PathConfigFile, char *Param, char *Value)
{
  char * line = NULL;
  size_t len = 0;
  int read;
  char Command[511];
  char BackupConfigName[240];
  strcpy(BackupConfigName,PathConfigFile);
  strcat(BackupConfigName,".bak");
  FILE *fp=fopen(PathConfigFile,"r");
  FILE *fw=fopen(BackupConfigName,"w+");
  char ParamWithEquals[255];
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");

  if(fp!=0)
  {
    while ((read = getline(&line, &len, fp)) != -1)
    {
      if(strncmp (line, ParamWithEquals, strlen(Param) + 1) == 0)
      {
        fprintf(fw, "%s=%s\n", Param, Value);
      }
      else
      {
        fprintf(fw, line);
      }
    }
    fclose(fp);
    fclose(fw);
    snprintf(Command, 511, "cp %s %s", BackupConfigName, PathConfigFile);
    system(Command);
  }
  else
  {
    printf("Config file not found \n");
    fclose(fp);
    fclose(fw);
  }
}


/***************************************************************************//**
 * @brief Checks the Config file and adds new entries if required
 *
 * @param None
 *
 * @return void
 * 
*******************************************************************************/

void CheckConfigFile()
{
  // Template for future use

  //char shell_command[255];
  //FILE *fp;
  //int r;

  //sprintf(shell_command, "grep -q 'wfallbase=' %s", PATH_CONFIG);
  //fp = popen(shell_command, "r");
  //r = pclose(fp);
  //if (WEXITSTATUS(r) != 0)
  //{
  //  printf("Updating Config File with <new entry>\n");
  //  sprintf(shell_command, "echo wfallbase=-70 >> %s", PATH_CONFIG);
  //  system(shell_command); 
  //}
}


/***************************************************************************//**
 * @brief Reads the Config file into global variables
 *
 * @param None
 *
 * @return void
 * 
*******************************************************************************/

void ReadSavedParams()
{
  char response[63]="0";

  strcpy(PlotTitle, "-");  // this is the "do not display" response
  GetConfigParam(PATH_CONFIG, "title", PlotTitle);

  GetConfigParam(PATH_CONFIG, "centrefreq", response);
  centrefreq = ConstrainFreq(atoi(response));

  GetConfigParam(PATH_CONFIG, "span", response);
  span = atoi(response);

  strcpy(response, "spectrum");
  GetConfigParam(PATH_CONFIG, "mode", response);
  if (strcmp(response, "spectrum") == 0)
  {
    spectrum = true;
    waterfall = false;
  }
  if (strcmp(response, "20db") == 0)
  {
    spectrum = true;
    Range20dB = true;
    waterfall = false;
  }
  if (strcmp(response, "waterfall") == 0)
  {
    spectrum = false;
    waterfall = true;
  }
  if (strcmp(response, "mix") == 0)
  {
    spectrum = true;
    waterfall = true;
  }

  strcpy(response, "-60");
  GetConfigParam(PATH_CONFIG, "wfallbase", response);
  WaterfallBase = atoi(response);

  strcpy(response, "60");
  GetConfigParam(PATH_CONFIG, "wfallrange", response);
  WaterfallRange = atoi(response);

  strcpy(response, "0");
  GetConfigParam(PATH_CONFIG, "wfalltimespan", response);
  wfalltimespan = atoi(response);

  strcpy(response, "single");
  GetConfigParam(PATH_CONFIG, "tracetype", response);
  strcpy(TraceType, response);

  printf("TraceType read in as %s\n", TraceType);

  strcpy(response, "0");
  GetConfigParam(PATH_CONFIG, "freqoffset", response);
  freqoffset = atoi(response);

  strcpy(response, "0");
  GetConfigParam(PATH_CONFIG, "baselineshift", response);
  BaselineShift = atoi(response);

  strcpy(response, "-80");
  GetConfigParam(PATH_CONFIG, "baseline20db", response);
  BaseLine20dB = atoi(response);

  strcpy(response, "spectrum");
  GetConfigParam(PATH_CONFIG, "display", response);
  if (strcmp(response, "scope") == 0)
  {
    scope = true;
  }
  else
  {
    scope = false;
  }

  strcpy(response, "no");
  GetConfigParam(PATH_CONFIG, "invert", response);
  if (strcmp(response, "yes") == 0)
  {
    invert_scope = true;
  }
  else
  {
    invert_scope = false;
  }

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "fborientation", response);
  FBOrientation = atoi(response);

  GetConfigParam(PATH_SCONFIG, "webcontrol", response);
  if (strcmp(response, "enabled") == 0)
  {
    webcontrol = true;
    pthread_create (&thwebclick, NULL, &WebClickListener, NULL);
    webclicklistenerrunning = true;
  }
  else
  {
    webcontrol = false;
    system("cp /home/pi/portsdown/images/web_not_enabled.png /home/pi/tmp/screen.png");
  }

  GetConfigParam(PATH_SCONFIG, "touch", response);
  if (strcmp(response, "inverted") == 0)
  {
    touch_inverted = true;
  }
}


void *WaitTouchscreenEvent(void * arg)
{
  int TouchTriggerTemp;
  int rawX;
  int rawY;
  int rawPressure;
  while (true)
  {
    TouchTriggerTemp = getTouchSampleThread(&rawX, &rawY, &rawPressure);
    TouchX = rawX;
    TouchY = rawY;
    TouchPressure = rawPressure;
    TouchTrigger = TouchTriggerTemp;
  }
  return NULL;
}


void *WaitMouseEvent(void * arg)
{
  int x = 0;
  int y = 0;
  int scroll = 0;
  int fd;
  char mouse_event_name[63];

  bool left_button_action = false;
  snprintf(mouse_event_name, 63, "/dev/input/event%d", FindMouseEvent());

  // Note x = y = 0 at bottom left

  if ((fd = open(mouse_event_name, O_RDONLY)) < 0)
  {
    perror("evdev open");
    exit(1);
  }
  struct input_event ev;

  while(1)
  {
    read(fd, &ev, sizeof(struct input_event));

    if (ev.type == 2)  // EV_REL
    {
      if (ev.code == 0) // x
      {
        x = x + ev.value;
        if (x < 0)
        {
          x = 0;
        }
        if (x > 799)
        {
          x = 799;
        }
        //printf("value %d, type %d, code %d, x_pos %d, y_pos %d\n",ev.value,ev.type,ev.code, x, y);
        //printf("x_pos %d, y_pos %d\n", x, y);
        mouse_active = true;
        moveCursor(x, y);
        mouse_x = x;
        mouse_y = y;
      }
      else if (ev.code == 1) // y
      {
        y = y - ev.value;
        if (y < 0)
        {
          y = 0;
        }
        if (y > 479)
        {
          y = 479;
        }
        //printf("value %d, type %d, code %d, x_pos %d, y_pos %d\n",ev.value,ev.type,ev.code, x, y);
        //printf("x_pos %d, y_pos %d\n", x, y);
        mouse_active = true;
        while (image_complete == false)  // Wait for menu to be drawn
        {
          usleep(1000);
        }
        mouse_active = true;
        moveCursor(x, y);
        mouse_x = x;
        mouse_y = y;
      }
      else if (ev.code == 8) // scroll wheel
      {
        scroll = scroll + ev.value;
        //printf("value %d, type %d, code %d, scroll %d\n",ev.value,ev.type,ev.code, scroll);
      }
      else
      {
        //printf("value %d, type %d, code %d\n", ev.value, ev.type, ev.code);
      }
    }

    else if (ev.type == 4)  // EV_MSC
    {
      if (ev.code == 4) // ?
      {
        if (ev.value == 589825)
        { 
          //printf("value %d, type %d, code %d, left mouse click \n", ev.value, ev.type, ev.code);
          //printf("Waiting for up or down signal\n");
          left_button_action = true;
        }
        if (ev.value == 589826)
        { 
          printf("value %d, type %d, code %d, right mouse click \n", ev.value, ev.type, ev.code);
        }
      }
    }
    else if (ev.type == 1)
    {
      //printf("value %d, type %d, code %d\n", ev.value, ev.type, ev.code);

      if ((left_button_action == true) && (ev.code == 272) && (ev.value == 1) && (mouse_active == true))
      {
        mouse_x = x;
        mouse_y = y;
        MouseClickForAction = true;
      }
      left_button_action = false;
    }
    else
    { 
      //printf("value %d, type %d, code %d\n", ev.value, ev.type, ev.code);
    }
    //printf("Mouse X = %d, mouse Y = %d\n", x, y);
  }
}


void *WebClickListener(void * arg)
{
  while (webcontrol)
  {
    //(void)argc;
	//return ffunc_run(ProgramName);
	ffunc_run(ProgramName);
  }
  webclicklistenerrunning = false;
  return NULL;
}


void parseClickQuerystring(char *query_string, int *x_ptr, int *y_ptr)
{
  char *query_ptr = strdup(query_string),
  *tokens = query_ptr,
  *p = query_ptr;

  while ((p = strsep (&tokens, "&\n")))
  {
    char *var = strtok (p, "="),
         *val = NULL;
    if (var && (val = strtok (NULL, "=")))
    {
      if(strcmp("x", var) == 0)
      {
        *x_ptr = atoi(val);
      }
      else if(strcmp("y", var) == 0)
      {
        *y_ptr = atoi(val);
      }
    }
  }
}


FFUNC touchscreenClick(ffunc_session_t * session)
{
  ffunc_str_t payload;

  if( (webcontrol == false) || ffunc_read_body(session, &payload) )
  {
    if( webcontrol == false)
    {
      return;
    }

    ffunc_write_out(session, "Status: 200 OK\r\n");
    ffunc_write_out(session, "Content-Type: text/plain\r\n\r\n");
    ffunc_write_out(session, "%s\n", "click received.");
    fprintf(stderr, "Received click POST: %s (%ld)\n", payload.data?payload.data:"", payload.len);

    int x = -1;
    int y = -1;
    parseClickQuerystring(payload.data, &x, &y);
    printf("After Parse: x: %d, y: %d\n", x, y);

    if((x >= 0) && (y >= 0))
    {
      web_x = x;                 // web_x is a global int
      web_y = y;                 // web_y is a global int
      strcpy(WebClickForAction, "yes");
      printf("Web Click Event x: %d, y: %d\n", web_x, web_y);
    }
  }
  else
  {
    ffunc_write_out(session, "Status: 400 Bad Request\r\n");
    ffunc_write_out(session, "Content-Type: text/plain\r\n\r\n");
    ffunc_write_out(session, "%s\n", "payload not found.");
  }
}


void take_snap()
{
  FILE *fp;
  char SnapIndex[255];
  int SnapNumber;
  char mvcmd[255];
  char echocmd[255];
  char SnapIndexText[255];

  // Fetch the Next Snap serial number
  fp = popen("cat /home/pi/snaps/snap_index.txt", "r");
  if (fp == NULL) 
  {
    printf("Failed to run command\n" );
    exit(1);
  }
  // Read the output a line at a time - output it. 
  while (fgets(SnapIndex, 20, fp) != NULL)
  {
    //printf("%s", SnapIndex);
  }

  pclose(fp);

  SnapNumber=atoi(SnapIndex);  // this is the next snap number to be saved.  Needs incrementing before exit
printf("pre-fb2png\n");
  fb2png(); // saves snap to /home/pi/tmp/snapshot.png
printf("post-fb2png\n");

  system("convert /home/pi/tmp/snapshot.png /home/pi/tmp/screen.jpg");

  sprintf(SnapIndexText, "%d", SnapNumber);
  strcpy(mvcmd, "cp /home/pi/tmp/screen.jpg /home/pi/snaps/snap");
  strcat(mvcmd, SnapIndexText);
  strcat(mvcmd, ".jpg >/dev/null 2>/dev/null");
  system(mvcmd);

  snprintf(echocmd, 50, "echo %d > /home/pi/snaps/snap_index.txt", SnapNumber + 1);
  system (echocmd); 
}


void snap_from_menu()
{
  freeze = true; 
  SetButtonStatus(ButtonNumber(CurrentMenu, 0), 1);  // hide the capture button 
  UpdateWindow();                                    // paint the hide

  while(! frozen)                                    // wait till the end of the scan
  {
    usleep(10);
  }

  take_snap();

  SetButtonStatus(ButtonNumber(CurrentMenu, 0), 0);  // unhide the button
  UpdateWindow();                                    // paint the un-hide
  freeze = false;
}


void do_snapcheck()
{
  FILE *fp;
  char SnapIndex[255];
  int SnapNumber;
  int Snap;
  int LastDisplayedSnap = -1;
  int rawX, rawY, rawPressure;
  int TCDisplay = -1;

  char fbicmd[255];
  char rotatecmd[255];
  char labelcmd[255];

  // Fetch the Next Snap serial number
  fp = popen("cat /home/pi/snaps/snap_index.txt", "r");
  if (fp == NULL) 
  {
    printf("Failed to run command\n" );
    exit(1);
  }
  // Read the output a line at a time - output it. 
  while (fgets(SnapIndex, 20, fp) != NULL)
  {
    printf("%s", SnapIndex);
  }

  pclose(fp);

  SnapNumber=atoi(SnapIndex);
  Snap = SnapNumber - 1;

  while (((TCDisplay == 1) || (TCDisplay == -1)) && (SnapNumber != 0))
  {
    if(LastDisplayedSnap != Snap)  // only redraw if not already there
    {
      sprintf(SnapIndex, "%d", Snap);

      if (FBOrientation == 180)
      {
        system("rm /home/pi/tmp/snapinverted.jpg >/dev/null 2>/dev/null");
        system("rm /home/pi/tmp/labelledsnap.jpg >/dev/null 2>/dev/null");

        // Label the snap before inverting it
        strcpy(labelcmd, "convert /home/pi/snaps/snap");
        strcat(labelcmd, SnapIndex);
        strcat(labelcmd, ".jpg -pointsize 20 -fill white -draw 'text 5,460 \"Snap ");
        strcat(labelcmd, SnapIndex);
        strcat(labelcmd, "\"' /home/pi/tmp/labelledsnap.jpg");
        system(labelcmd);

        // Invert the labelled snap
        strcpy(rotatecmd, "convert /home/pi/tmp/labelledsnap");
        strcat(rotatecmd, ".jpg -rotate 180 /home/pi/tmp/snapinverted.jpg");
        system(rotatecmd);

        // Display it
        strcpy(fbicmd, "sudo fbi -T 1 -noverbose -a /home/pi/tmp/snapinverted.jpg >/dev/null 2>/dev/null");
        system(fbicmd);
      }
      else
      {
        // Label the snap
        strcpy(labelcmd, "convert /home/pi/snaps/snap");
        strcat(labelcmd, SnapIndex);
        strcat(labelcmd, ".jpg -pointsize 20 -fill white -draw 'text 5,460 \"Snap ");
        strcat(labelcmd, SnapIndex);
        strcat(labelcmd, "\"' /home/pi/tmp/labelledsnap.jpg");
        system(labelcmd);

        // Display it
        strcpy(fbicmd, "sudo fbi -T 1 -noverbose -a /home/pi/tmp/labelledsnap.jpg >/dev/null 2>/dev/null");
        system(fbicmd);
      }
      LastDisplayedSnap = Snap;
      UpdateWeb();
    }

    if (getTouchSample(&rawX, &rawY, &rawPressure) == 0) continue;

    system("sudo killall fbi >/dev/null 2>/dev/null");  // kill instance of fbi

    TCDisplay = IsImageToBeChanged(rawX, rawY);  // check if touch was previous snap, next snap or exit
    if (TCDisplay != 0)
    {
      Snap = Snap + TCDisplay;
      if (Snap >= SnapNumber)
      {
        Snap = 0;
      }
      if (Snap < 0)
      {
        Snap = SnapNumber - 1;
      }
    }
  }

  // Tidy up and display touch menu
  system("sudo killall fbi >/dev/null 2>/dev/null");  // kill any instance of fbi
}


int IsImageToBeChanged(int x,int y)
{
  // Returns -1 for LHS touch, 0 for centre and 1 for RHS

  TransformTouchMap(x,y);       // Sorts out orientation and approx scaling of the touch map

  if (scaledX <= wscreen/8)
  {
    return -1;
  }
  else if (scaledX >= 7 * wscreen/8)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}


void Keyboard(char RequestText[63], char InitText[63], int MaxLength)
{
  char EditText[64];
  strcpy (EditText, InitText);
  int token;
  int PreviousMenu;
  int i;
  int CursorPos;
  char thischar[2];
  int KeyboardShift = 1;
  int ShiftStatus = 0;
  char KeyPressed[2];
  char PreCuttext[63];
  char PostCuttext[63];
  bool refreshed;
  
  // Store away currentMenu
  PreviousMenu = CurrentMenu;

  // Trim EditText to MaxLength
  if (strlen(EditText) > MaxLength)
  {
    strncpy(EditText, &EditText[0], MaxLength);
    EditText[MaxLength] = '\0';
  }

  // Set cursor position to next character after EditText
  CursorPos = strlen(EditText);

  // On initial call set Menu to 41
  CurrentMenu = 41;
  int rawX, rawY, rawPressure;
  refreshed = false;

  // Set up text
  clearScreen(0, 0, 0);
  const font_t *font_ptr = &font_dejavu_sans_28;

  // Look up pitch for W (87), the widest caharcter
  int charPitch = font_ptr->characters[87].render_width;

  for (;;)
  {
    if (!refreshed)
    {
      // Sort Shift changes
      ShiftStatus = 2 - (2 * KeyboardShift); // 0 = Upper, 2 = lower
      for (i = ButtonNumber(CurrentMenu, 0); i <= ButtonNumber(CurrentMenu, 49); i = i + 1)
      {
        if (ButtonArray[i].IndexStatus > 0)  // If button needs to be drawn
        {
          SetButtonStatus(i, ShiftStatus);
        }
      }  

      // Display the keyboard here as it would overwrite the text later
      UpdateWindow();

      // Display Instruction Text
      Text(10, 420 , RequestText, font_ptr, 0, 0, 0, 255, 255, 255);

      // Blank out the text line to erase the previous text and cursor
      rectangle(10, 320, 780, 40, 0, 0, 0);

      // Display Text for Editing
      for (i = 1; i <= strlen(EditText); i = i + 1)
      {
        strncpy(thischar, &EditText[i-1], 1);
        thischar[1] = '\0';
        if (i > MaxLength)
        {
          TextMid(i * charPitch, 330, thischar, font_ptr, 0, 0, 0, 255, 63, 63);    // Red text
        }
        else
        {
          TextMid(i * charPitch, 330, thischar, font_ptr, 0, 0, 0, 255, 255, 255);    // White text
        }
      }

      // Draw the cursor
      rectangle(12 + charPitch * CursorPos, 320, 2, 40, 255, 255, 255);

      refreshed = true;
    }
    publish();
    UpdateWeb();

    // Wait for key press
    if (getTouchSample(&rawX, &rawY, &rawPressure)==0) continue;
    refreshed = false;

    token = IsMenuButtonPushed(rawX, rawY);
    printf("Keyboard Token %d\n", token);

    // Highlight special keys when touched
    if ((token == 5) || (token == 8) || (token == 9) || (token == 2) || (token == 3)) // Clear, Enter, backspace, L and R arrows
    {
      SetButtonStatus(ButtonNumber(41, token), 1);
      DrawButton(ButtonNumber(41, token));
      UpdateWindow();
      usleep(300000);
      SetButtonStatus(ButtonNumber(41, token), 0);
      DrawButton(ButtonNumber(41, token));
      UpdateWindow();
    }

    if (token == 8)  // Enter pressed
    {
      if (strlen(EditText) > MaxLength) 
      {
        strncpy(KeyboardReturn, &EditText[0], MaxLength);
        KeyboardReturn[MaxLength] = '\0';
      }
      else
      {
        strcpy(KeyboardReturn, EditText);
      }
      CurrentMenu = PreviousMenu;
      printf("Exiting KeyBoard with String \"%s\"\n", KeyboardReturn);
      break;
    }
    else
    {    
      if (KeyboardShift == 1)     // Upper Case
      {
        switch (token)
        {
        case 0:  strcpy(KeyPressed, " "); break;
        case 4:  strcpy(KeyPressed, "-"); break;
        case 10: strcpy(KeyPressed, "Z"); break;
        case 11: strcpy(KeyPressed, "X"); break;
        case 12: strcpy(KeyPressed, "C"); break;
        case 13: strcpy(KeyPressed, "V"); break;
        case 14: strcpy(KeyPressed, "B"); break;
        case 15: strcpy(KeyPressed, "N"); break;
        case 16: strcpy(KeyPressed, "M"); break;
        case 17: strcpy(KeyPressed, ","); break;
        case 18: strcpy(KeyPressed, "."); break;
        case 19: strcpy(KeyPressed, "/"); break;
        case 20: strcpy(KeyPressed, "A"); break;
        case 21: strcpy(KeyPressed, "S"); break;
        case 22: strcpy(KeyPressed, "D"); break;
        case 23: strcpy(KeyPressed, "F"); break;
        case 24: strcpy(KeyPressed, "G"); break;
        case 25: strcpy(KeyPressed, "H"); break;
        case 26: strcpy(KeyPressed, "J"); break;
        case 27: strcpy(KeyPressed, "K"); break;
        case 28: strcpy(KeyPressed, "L"); break;
        case 30: strcpy(KeyPressed, "Q"); break;
        case 31: strcpy(KeyPressed, "W"); break;
        case 32: strcpy(KeyPressed, "E"); break;
        case 33: strcpy(KeyPressed, "R"); break;
        case 34: strcpy(KeyPressed, "T"); break;
        case 35: strcpy(KeyPressed, "Y"); break;
        case 36: strcpy(KeyPressed, "U"); break;
        case 37: strcpy(KeyPressed, "I"); break;
        case 38: strcpy(KeyPressed, "O"); break;
        case 39: strcpy(KeyPressed, "P"); break;
        case 40: strcpy(KeyPressed, "1"); break;
        case 41: strcpy(KeyPressed, "2"); break;
        case 42: strcpy(KeyPressed, "3"); break;
        case 43: strcpy(KeyPressed, "4"); break;
        case 44: strcpy(KeyPressed, "5"); break;
        case 45: strcpy(KeyPressed, "6"); break;
        case 46: strcpy(KeyPressed, "7"); break;
        case 47: strcpy(KeyPressed, "8"); break;
        case 48: strcpy(KeyPressed, "9"); break;
        case 49: strcpy(KeyPressed, "0"); break;
        }
      }
      else                          // Lower Case
      {
        switch (token)
        {
        case 0:  strcpy(KeyPressed, " "); break;
        case 4:  strcpy(KeyPressed, "_"); break;
        case 10: strcpy(KeyPressed, "z"); break;
        case 11: strcpy(KeyPressed, "x"); break;
        case 12: strcpy(KeyPressed, "c"); break;
        case 13: strcpy(KeyPressed, "v"); break;
        case 14: strcpy(KeyPressed, "b"); break;
        case 15: strcpy(KeyPressed, "n"); break;
        case 16: strcpy(KeyPressed, "m"); break;
        case 17: strcpy(KeyPressed, "!"); break;
        case 18: strcpy(KeyPressed, "."); break;
        case 19: strcpy(KeyPressed, "?"); break;
        case 20: strcpy(KeyPressed, "a"); break;
        case 21: strcpy(KeyPressed, "s"); break;
        case 22: strcpy(KeyPressed, "d"); break;
        case 23: strcpy(KeyPressed, "f"); break;
        case 24: strcpy(KeyPressed, "g"); break;
        case 25: strcpy(KeyPressed, "h"); break;
        case 26: strcpy(KeyPressed, "j"); break;
        case 27: strcpy(KeyPressed, "k"); break;
        case 28: strcpy(KeyPressed, "l"); break;
        case 30: strcpy(KeyPressed, "q"); break;
        case 31: strcpy(KeyPressed, "w"); break;
        case 32: strcpy(KeyPressed, "e"); break;
        case 33: strcpy(KeyPressed, "r"); break;
        case 34: strcpy(KeyPressed, "t"); break;
        case 35: strcpy(KeyPressed, "y"); break;
        case 36: strcpy(KeyPressed, "u"); break;
        case 37: strcpy(KeyPressed, "i"); break;
        case 38: strcpy(KeyPressed, "o"); break;
        case 39: strcpy(KeyPressed, "p"); break;
        case 40: strcpy(KeyPressed, "1"); break;
        case 41: strcpy(KeyPressed, "2"); break;
        case 42: strcpy(KeyPressed, "3"); break;
        case 43: strcpy(KeyPressed, "4"); break;
        case 44: strcpy(KeyPressed, "5"); break;
        case 45: strcpy(KeyPressed, "6"); break;
        case 46: strcpy(KeyPressed, "7"); break;
        case 47: strcpy(KeyPressed, "8"); break;
        case 48: strcpy(KeyPressed, "9"); break;
        case 49: strcpy(KeyPressed, "0"); break;
        }
      }

      // If shift key has been pressed Change Case
      if ((token == 6) || (token == 7))
      {
        if (KeyboardShift == 1)
        {
          KeyboardShift = 0;
        }
        else
        {
          KeyboardShift = 1;
        }
      }
      else if (token == 9)   // backspace
      {
        if (CursorPos == 1)
        {
           // Copy the text to the right of the deleted character
          strncpy(PostCuttext, &EditText[1], strlen(EditText) - 1);
          PostCuttext[strlen(EditText) - 1] = '\0';
          strcpy (EditText, PostCuttext);
          CursorPos = 0;
        }
        if (CursorPos > 1)
        {
          // Copy the text to the left of the deleted character
          strncpy(PreCuttext, &EditText[0], CursorPos - 1);
          PreCuttext[CursorPos-1] = '\0';

          if (CursorPos == strlen(EditText))
          {
            strcpy (EditText, PreCuttext);
          }
          else
          {
            // Copy the text to the right of the deleted character
            strncpy(PostCuttext, &EditText[CursorPos], strlen(EditText));
            PostCuttext[strlen(EditText)-CursorPos] = '\0';

            // Now combine them
            strcat(PreCuttext, PostCuttext);
            strcpy (EditText, PreCuttext);
          }
          CursorPos = CursorPos - 1;
        }
      }
      else if (token == 5)   // Clear
      {
        EditText[0]='\0';
        CursorPos = 0;
      }
      else if (token == 2)   // left arrow
      {
        CursorPos = CursorPos - 1;
        if (CursorPos < 0)
        {
          CursorPos = 0;
        }
      }
      else if (token == 3)   // right arrow
      {
        CursorPos = CursorPos + 1;
        if (CursorPos > strlen(EditText))
        {
          CursorPos = strlen(EditText);
        }
      }
      else if ((token == 0) || (token == 4) || ((token >=10) && (token <= 49)))
      {
        // character Key has been touched, so highlight it for 300 ms
 
        ShiftStatus = 3 - (2 * KeyboardShift); // 1 = Upper, 3 = lower
        SetButtonStatus(ButtonNumber(41, token), ShiftStatus);
        DrawButton(ButtonNumber(41, token));
        publish();
        //UpdateWindow();
        usleep(300000);
        ShiftStatus = 2 - (2 * KeyboardShift); // 0 = Upper, 2 = lower
        SetButtonStatus(ButtonNumber(41, token), ShiftStatus);
        DrawButton(ButtonNumber(41, token));
        publish();
        //UpdateWindow();

        if (strlen(EditText) < 23) // Don't let it overflow
        {
        // Copy the text to the left of the insert point
        strncpy(PreCuttext, &EditText[0], CursorPos);
        PreCuttext[CursorPos] = '\0';
          
        // Append the new character to the pre-insert string
        strcat(PreCuttext, KeyPressed);

        // Append any text to the right if it exists
        if (CursorPos == strlen(EditText))
        {
          strcpy (EditText, PreCuttext);
        }
        else
        {
          // Copy the text to the right of the inserted character
          strncpy(PostCuttext, &EditText[CursorPos], strlen(EditText));
          PostCuttext[strlen(EditText)-CursorPos] = '\0';

          // Now combine them
          strcat(PreCuttext, PostCuttext);
          strcpy (EditText, PreCuttext);
        }
        CursorPos = CursorPos+1;
        }
      }
    }
  }
}


int openTouchScreen(int NoDevice)
{
  char sDevice[255];

  sprintf(sDevice, "/dev/input/event%d", NoDevice);
  if(fd != 0) close(fd);
  if ((fd = open(sDevice, O_RDONLY)) > 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int getTouchScreenDetails(int *screenXmin, int *screenXmax,int *screenYmin,int *screenYmax)
{
  unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
  char name[256] = "Unknown";
  int abs[6] = {0};

  ioctl(fd, EVIOCGNAME(sizeof(name)), name);
  //printf("Input device name: \"%s\"\n", name);

  memset(bit, 0, sizeof(bit));
  ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
  //printf("Supported events:\n");

  int i,j,k;
  int IsAtouchDevice=0;
  for (i = 0; i < EV_MAX; i++)
  {
    if (test_bit(i, bit[0]))
    {
      //printf("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
      if (!i) continue;
      ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
      for (j = 0; j < KEY_MAX; j++)
      {
        if (test_bit(j, bit[i]))
        {
          //printf("    Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
          if(j==330) IsAtouchDevice=1;
          if (i == EV_ABS)
          {
            ioctl(fd, EVIOCGABS(j), abs);
            for (k = 0; k < 5; k++)
            {
              if ((k < 3) || abs[k])
              {
                //printf("     %s %6d\n", absval[k], abs[k]);
                if (j == 0)
                {
                  if ((strcmp(absval[k],"Min  ")==0)) *screenXmin =  abs[k];
                  if ((strcmp(absval[k],"Max  ")==0)) *screenXmax =  abs[k];
                }
                if (j == 1)
                {
                  if ((strcmp(absval[k],"Min  ")==0)) *screenYmin =  abs[k];
                  if ((strcmp(absval[k],"Max  ")==0)) *screenYmax =  abs[k];
                }
              }
            }
          }
        }
      }
    }
  }
  return IsAtouchDevice;
}

void TransformTouchMap(int x, int y)
{
  // This function takes the raw (0 - 4095 on each axis) touch data x and y
  // and transforms it to approx 0 - wscreen and 0 - hscreen in globals scaledX 
  // and scaledY prior to final correction by CorrectTouchMap  

  scaledX = x / scaleXvalue;
  scaledY = hscreen - y / scaleYvalue;
}


int IsButtonPushed(int NbButton,int x,int y)
{
  TransformTouchMap(x,y);  // Sorts out orientation and approx scaling of the touch map

  //printf("x=%d y=%d scaledx %d scaledy %d sxv %f syv %f Button %d\n",x,y,scaledX,scaledY,scaleXvalue,scaleYvalue, NbButton);

  int margin=10;  // was 20

  if((scaledX<=(ButtonArray[NbButton].x+ButtonArray[NbButton].w-margin))&&(scaledX>=ButtonArray[NbButton].x+margin) &&
    (scaledY<=(ButtonArray[NbButton].y+ButtonArray[NbButton].h-margin))&&(scaledY>=ButtonArray[NbButton].y+margin))
  {
    // ButtonArray[NbButton].LastEventTime=mymillis(); No longer used
    return 1;
  }
  else
  {
    return 0;
  }
}

int IsMenuButtonPushed(int x,int y)
{
  int  i, NbButton, cmo, cmsize;
  NbButton = -1;
  int margin=1;  // was 20 then 10
  cmo = ButtonNumber(CurrentMenu, 0); // Current Menu Button number Offset
  cmsize = ButtonNumber(CurrentMenu + 1, 0) - ButtonNumber(CurrentMenu, 0);
  TransformTouchMap(x,y);       // Sorts out orientation and approx scaling of the touch map

  //printf("x=%d y=%d scaledx %d scaledy %d sxv %f syv %f Button %d\n",x,y,scaledX,scaledY,scaleXvalue,scaleYvalue, NbButton);

  // For each button in the current Menu, check if it has been pushed.
  // If it has been pushed, return the button number.  If nothing valid has been pushed return -1
  // If it has been pushed, do something with the last event time

  for (i = 0; i <cmsize; i++)
  {
    if (ButtonArray[i + cmo].IndexStatus > 0)  // If button has been defined
    {
      //printf("Button %d, ButtonX = %d, ButtonY = %d\n", i, ButtonArray[i + cmo].x, ButtonArray[i + cmo].y);

      if  ((scaledX <= (ButtonArray[i + cmo].x + ButtonArray[i + cmo].w - margin))
        && (scaledX >= ButtonArray[i + cmo].x + margin)
        && (scaledY <= (ButtonArray[i + cmo].y + ButtonArray[i + cmo].h - margin))
        && (scaledY >= ButtonArray[i + cmo].y + margin))  // and touched
      {
        // ButtonArray[NbButton].LastEventTime=mymillis(); No longer used
        NbButton = i;          // Set the button number to return
        break;                 // Break out of loop as button has been found
      }
    }
  }
  return NbButton;
}


int InitialiseButtons()
{
  // Writes 0 to IndexStatus of each button to signify that it should not
  // be displayed.  As soon as a status (text and color) is added, IndexStatus > 0
  int i;
  for (i = 0; i <= MAX_BUTTON; i = i + 1)
  {
    ButtonArray[i].IndexStatus = 0;
  }
  return 1;
}


int AddButton(int x,int y,int w,int h)
{
  button_t *NewButton=&(ButtonArray[IndexButtonInArray]);
  NewButton->x=x;
  NewButton->y=y;
  NewButton->w=w;
  NewButton->h=h;
  NewButton->NoStatus=0;
  NewButton->IndexStatus=0;
  return IndexButtonInArray++;
}


int ButtonNumber(int MenuIndex, int Button)
{
  // Returns the Button Number (0 - 674) from the Menu number and the button position
  int ButtonNumb = 0;

  if (MenuIndex <= 20)  // 20 x 15-button main menus
  {
    ButtonNumb = (MenuIndex - 1) * 15 + Button;
  }
  if ((MenuIndex >= 41) && (MenuIndex <= 41))  // keyboard
  {
    ButtonNumb = 300 + (MenuIndex - 41) * 50 + Button;
  }
  if (MenuIndex == 42) //Edge Case
  {
    ButtonNumb = 350;
  }
  return ButtonNumb;
}


int CreateButton(int MenuIndex, int ButtonPosition)
{
  // Provide Menu number (int 1 - 46), Button Position (0 snap, 1 top, 8/9 bottom)
  // return button number

  // Menus 1 - 20 are classic 15-button menus
  // Menus 21 - 40 are undefined and have no button numbers allocated
  // Menu 41 is a keyboard
  // Menu 42 only exists as a boundary for Menu 41

  int ButtonIndex;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  int normal_width = 120;
  int normal_xpos = 640;

  ButtonIndex = ButtonNumber(MenuIndex, ButtonPosition);

  if ((MenuIndex != 41) && (MenuIndex != 1) && (MenuIndex != 2) && (MenuIndex != 6))   // All except Main, keyboard, Span, Markers
  {
    if (ButtonPosition == 0)  // Capture
    {
      x = 0;
      y = 0;
      w = 90;
      h = 55;
    }

    if ((ButtonPosition > 0) && (ButtonPosition < 9))  // 8 right hand buttons
    {
      x = normal_xpos;
      y = 480 - (ButtonPosition * 60);
      w = normal_width;
      h = 50;
    }
  }
  else if ((MenuIndex == 1) || (MenuIndex == 2))   // Main and Marker Menu
  {
    if (ButtonPosition == 0)  // Capture
    {
      x = 0;
      y = 0;
      w = 90;
      h = 50;
    }

    if ((ButtonPosition > 0) && (ButtonPosition < 5))  // 6 right hand buttons
    {
      x = normal_xpos;
      y = 480 - (ButtonPosition * 60);
      w = normal_width;
      h = 50;
    }
    if (ButtonPosition == 5) // Left hand arrow
    {
      x = normal_xpos;  
      y = 480 - (5 * 60);
      w = 50;
      h = 50;
    }
    if (ButtonPosition == 6) // Right hand arrow
    {
      x = 710;  // = normal_xpos + 50 button width + 20 gap
      y = 480 - (5 * 60);
      w = 50;
      h = 50;
    }
    if ((ButtonPosition > 6) && (ButtonPosition < 10))  // Bottom 3 buttons
    {
      x = normal_xpos;
      y = 480 - ((ButtonPosition - 1) * 60);
      w = normal_width;
      h = 50;
    }
  }
  else if (MenuIndex == 6)   // Scan Width Menu
  {
    if (ButtonPosition == 0)  // Capture
    {
      x = 0;
      y = 0;
      w = 90;
      h = 50;
    }

    if (ButtonPosition == 1)                            // Menu Title
    {
      x = normal_xpos;
      y = 480 - (ButtonPosition * 60);
      w = normal_width;
      h = 50;
    }
    if (ButtonPosition == 2)                             // 100 kHz
    {
      x = normal_xpos;  
      y = 480 - (2 * 60);
      w = 50;
      h = 50;
    }
    if (ButtonPosition == 3)                             // 200 kHz
    {
      x = 710;   // = normal_xpos + 50 button width + 20 gap
      y = 480 - (2 * 60);
      w = 50;
      h = 50;
    }

    if ((ButtonPosition == 4) || (ButtonPosition == 5))  // 500 kHz and 1 MHz
    {
      x = normal_xpos;
      y = 480 - (ButtonPosition * 60) + 60;
      w = normal_width;
      h = 50;
    }
    if (ButtonPosition == 6) // 2
    {
      x = normal_xpos;  
      y = 480 - (5 * 60);
      w = 50;
      h = 50;
    }
    if (ButtonPosition == 7) // 5
    {
      x = 710;  // = normal_xpos + 50 button width + 20 gap
      y = 480 - (5 * 60);
      w = 50;
      h = 50;
    }
    if (ButtonPosition == 8) // 10
    {
      x = normal_xpos;  
      y = 480 - (6 * 60);
      w = 50;
      h = 50;
    }
    if (ButtonPosition == 9) // 20
    {
      x = 710;  // = normal_xpos + 50 button width + 20 gap
      y = 480 - (6 * 60);
      w = 50;
      h = 50;
    }
    if ((ButtonPosition == 10) || (ButtonPosition == 11))  // Bottom 2 buttons
    {
      x = normal_xpos;
      y = 480 - ((ButtonPosition - 3) * 60);
      w = normal_width;
      h = 50;
    }
  }

  else  // Keyboard
  {
    w = 65;
    h = 59;

    if (ButtonPosition <= 9)  // Space bar and < > - Enter, Bkspc
    {
      switch (ButtonPosition)
      {
      case 0:                   // Space Bar
          y = 0;
          x = 165; // wscreen * 5 / 24;
          w = 362; // wscreen * 11 /24;
          break;
      case 1:                  // Not used
          y = 0;
          x = 528; //wscreen * 8 / 12;
          break;
      case 2:                  // <
          y = 0;
          x = 594; //wscreen * 9 / 12;
          break;
      case 3:                  // >
          y = 0;
          x = 660; // wscreen * 10 / 12;
          break;
      case 4:                  // -
          y = 0;
          x = 726; // wscreen * 11 / 12;
          break;
      case 5:                  // Clear
          y = 0;
          x = 0;
          w = 131; // 2 * wscreen/12;
          break;
      case 6:                 // Left Shift
          y = hscreen/8;
          x = 0;
          break;
      case 7:                 // Right Shift
          y = hscreen/8;
          x = 726; // wscreen * 11 / 12;
          break;
      case 8:                 // Enter
          y = 2 * hscreen/8;
          x = 660; // wscreen * 10 / 12;
          w = 131; // 2 * wscreen/12;
          h = 119;
          break;
      case 9:                 // Backspace
          y = 4 * hscreen / 8;
          x = 693; // wscreen * 21 / 24;
          w = 98; // 3 * wscreen/24;
          break;
      }
    }
    if ((ButtonPosition >= 10) && (ButtonPosition <= 19))  // ZXCVBNM,./
    {
      y = hscreen/8;
      x = (ButtonPosition - 9) * 66;
    }
    if ((ButtonPosition >= 20) && (ButtonPosition <= 29))  // ASDFGHJKL
    {
      y = 2 * hscreen / 8;
      x = ((ButtonPosition - 19) * 66) - 33;
    }
    if ((ButtonPosition >= 30) && (ButtonPosition <= 39))  // QWERTYUIOP
    {
      y = 3 * hscreen / 8;
      x = (ButtonPosition - 30) * 66;
    }
    if ((ButtonPosition >= 40) && (ButtonPosition <= 49))  // 1234567890
    {
      y = 4 * hscreen / 8;
      x = ((ButtonPosition - 39) * 66) - 33;
    }
  }

  button_t *NewButton=&(ButtonArray[ButtonIndex]);
  NewButton->x=x;
  NewButton->y=y;
  NewButton->w=w;
  NewButton->h=h;
  NewButton->NoStatus=0;
  NewButton->IndexStatus=0;

  return (ButtonIndex);
}


int AddButtonStatus(int ButtonIndex,char *Text,color_t *Color)
{
  button_t *Button=&(ButtonArray[ButtonIndex]);
  strcpy(Button->Status[Button->IndexStatus].Text,Text);
  Button->Status[Button->IndexStatus].Color=*Color;
  return Button->IndexStatus++;
}


void AmendButtonStatus(int ButtonIndex, int ButtonStatusIndex, char *Text, color_t *Color)
{
  button_t *Button=&(ButtonArray[ButtonIndex]);
  strcpy(Button->Status[ButtonStatusIndex].Text, Text);
  Button->Status[ButtonStatusIndex].Color=*Color;
}


void DrawButton(int ButtonIndex)
{
  button_t *Button = &(ButtonArray[ButtonIndex]);
  char label[255];
  char line1[15];
  char line2[15];

  // Look up the label
  strcpy(label, Button->Status[Button->NoStatus].Text);

  // Draw the basic button
  rectangle(Button->x, Button->y + 1, Button->w, Button->h, 
    Button->Status[Button->NoStatus].Color.r,
    Button->Status[Button->NoStatus].Color.g,
    Button->Status[Button->NoStatus].Color.b);

  // Separate button text into 2 lines if required  
  char find = '^';                                  // Line separator is ^
  const char *ptr = strchr(label, find);            // pointer to ^ in string

  if((ptr) && (CurrentMenu != 41))                  // if ^ found then
  {                                                 // 2 lines
    int index = ptr - label;                        // Position of ^ in string
    snprintf(line1, index+1, label);                // get text before ^
    snprintf(line2, strlen(label) - index, label + index + 1);  // and after ^

    // Display the text on the button
    TextMid(Button->x + Button->w/2, Button->y +Button->h * 11 /16, line1, &font_dejavu_sans_20, Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g, Button->Status[Button->NoStatus].Color.b, 255, 255, 255);	
    TextMid(Button->x + Button->w/2, Button->y +Button->h * 3 / 16, line2, &font_dejavu_sans_20, Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g, Button->Status[Button->NoStatus].Color.b, 255, 255, 255);	
  
  }
  else                                              // One line only
  {
    if (CurrentMenu <= 9)
    {
      TextMid(Button->x + Button->w/2, Button->y + Button->h/2, label, &font_dejavu_sans_20, Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g, Button->Status[Button->NoStatus].Color.b, 255, 255, 255);
    }
    else                // Keyboard
    {
      TextMid(Button-> x +Button->w/2, Button->y + Button->h/2 - hscreen / 64, label, &font_dejavu_sans_28, Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g, Button->Status[Button->NoStatus].Color.b, 255, 255, 255);
    }
  }
}


void SetButtonStatus(int ButtonIndex,int Status)
{
  button_t *Button=&(ButtonArray[ButtonIndex]);
  Button->NoStatus=Status;
}

int GetButtonStatus(int ButtonIndex)
{
  button_t *Button=&(ButtonArray[ButtonIndex]);
  return Button->NoStatus;
}


int getTouchSampleThread(int *rawX, int *rawY, int *rawPressure)
{
  int i;
  static bool awaitingtouchstart;
  static bool touchfinished;

  if (touchneedsinitialisation == true)
  {
    awaitingtouchstart = true;
    touchfinished = true;
    touchneedsinitialisation = false;
  }

  /* how many bytes were read */
  size_t rb;

  /* the events (up to 64 at once) */
  struct input_event ev[64];

  if (((strcmp(DisplayType, "Element14_7") == 0) || (strcmp(DisplayType, "Browser") == 0))
      && (strcmp(DisplayType, "dfrobot5") != 0))   // Browser or Element14_7, but not dfrobot5
  {
    // Thread flow blocks here until there is a touch event
    rb = read(fd, ev, sizeof(struct input_event) * 64);

    *rawX = -1;
    *rawY = -1;
    int StartTouch = 0;

    for (i = 0;  i <  (rb / sizeof(struct input_event)); i++)
    {
      if (ev[i].type ==  EV_SYN)
      {
        //printf("Event type is %s%s%s = Start of New Event\n",
        //        KYEL, events[ev[i].type], KWHT);
      }

      else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
      {
        StartTouch = 1;
        //printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s1%s = Touch Starting\n",
        //        KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
      }

      else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
      {
        //StartTouch=0;
        //printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s0%s = Touch Finished\n",
        //        KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
      }

      else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0)
      {
        //printf("Event type is %s%s%s & Event code is %sX(0)%s & Event value is %s%d%s\n",
        //        KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, ev[i].value, KWHT);
	    *rawX = ev[i].value;
      }

      else if (ev[i].type == EV_ABS  && ev[i].code == 1 && ev[i].value > 0)
      {
        //printf("Event type is %s%s%s & Event code is %sY(1)%s & Event value is %s%d%s\n",
        //        KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, ev[i].value, KWHT);
        *rawY = ev[i].value;
      }

      else if (ev[i].type == EV_ABS  && ev[i].code == 24 && ev[i].value > 0)
      {
        //printf("Event type is %s%s%s & Event code is %sPressure(24)%s & Event value is %s%d%s\n",
        //        KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, ev[i].value,KWHT);
        *rawPressure = ev[i].value;
      }

      if((*rawX != -1) && (*rawY != -1) && (StartTouch == 1))  // 1a
      {
        // Inversion required if toucscreen has been inverted.  Needs selecting on touchscreen orientation
        if(touch_inverted == true)
        {
          *rawX = 800 - *rawX;
          *rawY = 480 - *rawY;
        }
        printf("7 inch Touchscreen Touch Event: rawX = %d, rawY = %d, rawPressure = %d\n", 
                *rawX, *rawY, *rawPressure);
        return 1;
      }
    }
  }

  if (strcmp(DisplayType, "dfrobot5") == 0)
  {
    // Program flow blocks here until there is a touch event
    rb = read(fd, ev, sizeof(struct input_event) * 64);

    if (awaitingtouchstart == true)
    {    
      *rawX = -1;
      *rawY = -1;
      touchfinished = false;
    }

    for (i = 0;  i <  (rb / sizeof(struct input_event)); i++)
    {
      //printf("rawX = %d, rawY = %d, rawPressure = %d, \n\n", *rawX, *rawY, *rawPressure);

      if (ev[i].type ==  EV_SYN)
      {
        //printf("Event type is %s%s%s = Start of New Event\n",
        //        KYEL, events[ev[i].type], KWHT);
      }

      else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
      {
        awaitingtouchstart = false;
        touchfinished = false;

        //printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s1%s = Touch Starting\n",
        //        KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
      }

      else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
      {
        awaitingtouchstart = false;
        touchfinished = true;

        //printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s0%s = Touch Finished\n",
        //        KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
      }

      else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0)
      {
        //printf("Event type is %s%s%s & Event code is %sX(0)%s & Event value is %s%d%s\n",
        //        KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, ev[i].value, KWHT);
        *rawX = ev[i].value;
      }

      else if (ev[i].type == EV_ABS  && ev[i].code == 1 && ev[i].value > 0)
      {
        //printf("Event type is %s%s%s & Event code is %sY(1)%s & Event value is %s%d%s\n",
        //        KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, ev[i].value, KWHT);
        *rawY = ev[i].value;
      }

      else if (ev[i].type == EV_ABS  && ev[i].code == 24 && ev[i].value > 0)
      {
        //printf("Event type is %s%s%s & Event code is %sPressure(24)%s & Event value is %s%d%s\n",
        //        KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, ev[i].value,KWHT);
        *rawPressure = ev[i].value;
      }

      if((*rawX != -1) && (*rawY != -1) && (touchfinished == true))  // 1a
      {
        printf("DFRobot Touch Event: rawX = %d, rawY = %d, rawPressure = %d\n", 
                *rawX, *rawY, *rawPressure);
        awaitingtouchstart = true;
        touchfinished = false;
        return 1;
      }
    }
  }
  return 0;
}


int getTouchSample(int *rawX, int *rawY, int *rawPressure)
{
  while (true)
  {
    if (TouchTrigger == 1)
    {
      *rawX = TouchX;
      *rawY = TouchY;
      *rawPressure = TouchPressure;
      printf("Touchtrigger was 1\n");
      TouchTrigger = 0;
      return 1;
    }
    else if ((webcontrol == true) && (strcmp(WebClickForAction, "yes") == 0))
    {
      *rawX = web_x;
      *rawY = web_y;
      *rawPressure = 0;
      strcpy(WebClickForAction, "no");
      printf("Web rawX = %d, rawY = %d, rawPressure = %d\n", *rawX, *rawY, *rawPressure);
      return 1;
    }
    else if (MouseClickForAction == true)
    {
      *rawX = mouse_x;
      *rawY = 479 - mouse_y;
      *rawPressure = 0;
      MouseClickForAction = false;
      printf("Mouse rawX = %d, rawY = %d, rawPressure = %d\n", *rawX, *rawY, *rawPressure);
      return 1;
    }
    else if (FalseTouch == true)
    {
      *rawX = web_x;
      *rawY = web_y;
      *rawPressure = 0;
      FalseTouch = false;
      printf("False Touch prompted by other event\n");
      return 1;
    }
    else
    {
      usleep(1000);
    }
  }
  return 0;
}


void UpdateWindow()    // Paint each defined button
{
  int i;
  int first;
  int last;

  if (markeron == false)
  {
    // Draw a black rectangle where the buttons are to erase them
    rectangle(620, 0, 160, 480, 0, 0, 0);
  }
  else   // But don't erase the marker text
  {
    rectangle(620, 0, 160, 420, 0, 0, 0);
  }

  // Don't draw buttons if the Markers are being refreshed.  Wait here

  // Draw each button in turn
  first = ButtonNumber(CurrentMenu, 0);
  last = ButtonNumber(CurrentMenu + 1 , 0) - 1;
  if ((markeron == false) || (CurrentMenu == 41))
  {
    for(i = first; i <= last; i++)
    {
      if (ButtonArray[i].IndexStatus > 0)  // If button needs to be drawn
      {
        DrawButton(i);                     // Draw the button
      }
    }
  }
  else
  {
    if (ButtonArray[first].IndexStatus > 0)  // If button needs to be drawn
    {
      while (MarkerRefresh == true)          // Wait for marker refresh to prevent conflict
      {
        usleep(10);
      }
      DrawButton(first);                     // Draw button 0, but not button 1
    }
    for(i = (first + 2); i <= last; i++)
    {
      if (ButtonArray[i].IndexStatus > 0)  // If button needs to be drawn
      {
        while (MarkerRefresh == true)      // Wait for marker refresh to prevent conflict
        {
          usleep(10);
        }
        DrawButton(i);                     // Draw the button
      }
    }
  }
  publish();
  UpdateWeb();
}


void wait_touch()
// Wait for Screen touch, ignore position, but then move on
// Used to let user acknowledge displayed text
{
  int rawX, rawY, rawPressure;
  printf("wait_touch called\n");

  // Check if screen touched, if not, wait 0.1s and check again
  while(getTouchSample(&rawX, &rawY, &rawPressure)==0)
  {
    usleep(100000);
  }
  // Screen has been touched
  printf("wait_touch exit\n");
}

void CalculateMarkers()
{
  int maxy;
  int xformaxy = 0;
  int xsum = 0;
  int ysum = 0;
  int markersamples = 10;
  char markerlevel[31];
  char markerfreq[31];
  float markerlev;
  float markerf;

  switch (markermode)
  {
    case 2:  // peak
      maxy = 0;
      for (i = 10; i < 499; i++)
      {
         if((y[i + 6] + 1) > maxy)
         {
           maxy = y[i + 6] + 1;
           xformaxy = i;
         }
      }
    break;

    case 3:  // null
      maxy = 400;
      for (i = 50; i < 450; i++)
      {
         if((y[i + 6] + 1) < maxy)
         {
           maxy = y[i + 6] + 1;
           xformaxy = i;
         }
      }
    break;

    case 4:  // manual
      maxy = 0;
      //maxy = y[manualmarkerx + 6] + 1;


      for (i = (manualmarkerx - 5); i < (manualmarkerx + 6); i++)
      {
         if((y[i + 6] + 1) > maxy)
         {
           maxy = y[i + 6] + 1;
           //xformaxy = i;
         }
      }

      xformaxy = manualmarkerx;
    break;
  }

  MarkerRefresh = true;  // Prevent other text screen-writes
                         // Do it early so as not to catch the tail end of text

  // Now smooth the marker 
  markerxhistory[historycount] = xformaxy;
  markeryhistory[historycount] = maxy;

  for (i = 0; i < markersamples; i++)
  {
    xsum = xsum + markerxhistory[i];
    ysum = ysum + markeryhistory[i];
  }

  historycount++;
  if (historycount > (markersamples - 1))
  {
    historycount = 0;
  }
  markerx = 100 + (xsum / markersamples);
  markery = 410 - (ysum / markersamples);

  // And display it
  if (Range20dB == false)
  {
    markerlev = ((float)(410 - markery) / 5.0) - 50.0 - BaselineShift;  // in dB for a 0 to -80 screen
  }
  else
  {
    markerlev = ((float)(410 - markery) / 20.0) + (float)(BaseLine20dB - BaselineShift) + 30.0;  // in dB for a BaseLine20dB to 20 above screen
  }
  rectangle(620, 420, 160, 60, 0, 0, 0);  // Blank the Menu title
  snprintf(markerlevel, 14, "Mkr %0.1f dB", markerlev);
  Text(640, 450, markerlevel, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);

startfreq = -280;
stopfreq = 121820;

  if((startfreq != -1 ) && (stopfreq != -1)) // "Do not display" values
  {

    markerf = (float)((((markerx - 100) * (stopfreq - startfreq)) / 500 + startfreq)) / 1000.0; 
    snprintf(markerfreq, 14, "%0.2f kHz", markerf);
    Text(640, 425, markerfreq, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);
  }
  MarkerRefresh = false;  // Unlock screen writes

  publish();
}


void SetSpanWidth(int button)
{  
  char ValueToSave[63];

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 2:
      span = 102;
    break;
    case 3:
      span = 205;
    break;
    case 4:
      span = 512;
    break;
    case 5:
      span = 1024;
    break;
    case 6:
      span = 2048;
    break;
    case 7:
      span = 5120;
    break;
    case 8:
      span = 10240;
    break;
    case 9:
      span = 20480;
    break;
  }

  // Store the new span
  snprintf(ValueToSave, 63, "%d", span);
  SetConfigParam(PATH_CONFIG, "span", ValueToSave);
  printf("span set to %d \n", span);

  // Trigger the span change
  CalcSpan();
  NewSpan = true;

  DrawSettings();       // New labels
  freeze = false;
}


void SetMode(int button)
{  
  char ValueToSave[63] = "spectrum";

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 2:
      spectrum = true;
      waterfall = false;
      Range20dB = false;
      strcpy(ValueToSave, "spectrum");
    break;
    case 3:
      spectrum = true;
      waterfall = false;
      Range20dB = true;
      strcpy(ValueToSave, "20db");
    break;
    case 4:
      spectrum = false;
      waterfall = true;
      Range20dB = false;
      strcpy(ValueToSave, "waterfall");
    break;
    case 5:
      spectrum = true;
      waterfall = true;
      Range20dB = false;
      strcpy(ValueToSave, "mix");
    break;
  }

  // Store the new mode
  SetConfigParam(PATH_CONFIG, "mode", ValueToSave);
  printf("Mode set to %s \n", ValueToSave);

  // Trigger the mode change
  CalcSpan();
  RedrawDisplay();
  DrawTickMarks();
  freeze = false;
}


void SetFreqPreset(int button)
{  
  char ValueToSave[63];
  char RequestText[64];
  char InitText[63];
  div_t div_10;
  div_t div_100;
  div_t div_1000;
  int amendfreq;

  if (CallingMenu == 7)
  {
    // Stop the scan at the end of the current one and wait for it to stop
    freeze = true;
    while(! frozen)
    {
      usleep(10);                                   // wait till the end of the scan
    }

    switch (button)
    {
      case 2:
        centrefreq = pfreq1;
      break;
      case 3:
        centrefreq = pfreq2;
      break;
      case 4:
        centrefreq = pfreq3;
      break;
      case 5:
        centrefreq = pfreq4;
      break;
      case 6:
        centrefreq = pfreq5;
      break;
    }

    // Store the new frequency
    snprintf(ValueToSave, 63, "%d", centrefreq);
    SetConfigParam(PATH_CONFIG, "centrefreq", ValueToSave);
    printf("Centre Freq set to %d \n", centrefreq);

    // Calculate the new settings
    CalcSpan();

    // Trigger the frequency change
    NewFreq = true;

    DrawSettings();       // New labels
    freeze = false;
  }
  else if (CallingMenu == 10)  // Amend the preset frequency
  {

    // Stop the scan at the end of the current one and wait for it to stop
    freeze = true;
    while(! frozen)
    {
      usleep(10);                                   // wait till the end of the scan
    }

    switch (button)
    {
      case 2:
        amendfreq = pfreq1;
      break;
      case 3:
        amendfreq = pfreq2;
      break;
      case 4:
        amendfreq = pfreq3;
      break;
      case 5:
        amendfreq = pfreq4;
      break;
      case 6:
        amendfreq = pfreq5;
      break;
    }

    // Define request string
    strcpy(RequestText, "Enter new preset frequency in MHz");

    // Define initial value and convert to MHz
    div_10 = div(amendfreq, 10);
    div_1000 = div(amendfreq, 1000);

    if(div_10.rem != 0)  // last character not zero, so make answer of form xxx.xxx
    {
      snprintf(InitText, 40, "%d.%03d", div_1000.quot, div_1000.rem);
    }
    else
    {
      div_100 = div(amendfreq, 100);
      if(div_100.rem != 0)  // last but one character not zero, so make answer of form xxx.xx
      {
        snprintf(InitText, 40, "%d.%02d", div_1000.quot, div_1000.rem / 10);
      }
      else
      {
        if(div_1000.rem != 0)  // last but two character not zero, so make answer of form xxx.x
        {
          snprintf(InitText, 40, "%d.%d", div_1000.quot, div_1000.rem / 100);
        }
        else  // integer MHz, so just xxx (no dp)
        {
          snprintf(InitText, 40, "%d", div_1000.quot);
        }
      }
    }

    // Ask for the new value
    do
    {
      Keyboard(RequestText, InitText, 10);
    }
    while (strlen(KeyboardReturn) == 0);

    amendfreq = (int)((1000 * atof(KeyboardReturn)) + 0.1);
    snprintf(ValueToSave, 63, "%d", amendfreq);

    switch (button)
    {
      case 2:
        SetConfigParam(PATH_CONFIG, "pfreq1", ValueToSave);
        printf("Preset Freq 1 set to %d \n", amendfreq);
        pfreq1 = amendfreq;
      break;
      case 3:
        SetConfigParam(PATH_CONFIG, "pfreq2", ValueToSave);
        printf("Preset Freq 2 set to %d \n", amendfreq);
        pfreq2 = amendfreq;
      break;
      case 4:
        SetConfigParam(PATH_CONFIG, "pfreq3", ValueToSave);
        printf("Preset Freq 3 set to %d \n", amendfreq);
        pfreq3 = amendfreq;
      break;
      case 5:
         SetConfigParam(PATH_CONFIG, "pfreq4", ValueToSave);
        printf("Preset Freq 4 set to %d \n", amendfreq);
        pfreq4 = amendfreq;
      break;
      case 6:
        SetConfigParam(PATH_CONFIG, "pfreq5", ValueToSave);
        printf("Preset Freq 5 set to %d \n", amendfreq);
        pfreq5 = amendfreq;
      break;
    }
    centrefreq = amendfreq;

    // Store the new frequency
    snprintf(ValueToSave, 63, "%d", centrefreq);
    SetConfigParam(PATH_CONFIG, "centrefreq", ValueToSave);
    printf("Centre Freq set to %d \n", centrefreq);

    // Trigger the frequency change
    NewFreq = true;

    // Tidy up, paint around the screen and then unfreeze
    CalcSpan();         // work out start and stop freqs
    clearScreen(0, 0, 0);
    DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
    DrawYaxisLabels();  // dB calibration on LHS
    DrawSettings();     // Start, Stop RBW, Ref level and Title
    freeze = false;
  }
}


void SetWfall(int button)
{
  char ValueToSave[63];
  char RequestText[63];
  char InitText[63];

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 2:                                            // Set Waterfall Base
      // Define request string
      strcpy(RequestText, "Enter new base for the waterfall in dB (-80 to 0)");

      // Define initial value in dB
      snprintf(InitText, 25, "%d", WaterfallBase);

      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
      }
      while ((strlen(KeyboardReturn) == 0) || (atoi(KeyboardReturn) < -80) || (atoi(KeyboardReturn) > 0));

      WaterfallBase = atoi(KeyboardReturn);
      snprintf(ValueToSave, 63, "%d", WaterfallBase);
      SetConfigParam(PATH_CONFIG, "wfallbase", ValueToSave);
      printf("Waterfall Base set to %d dB\n", WaterfallBase);
    break;
    case 3:                                            // Set waterfall range
      // Define request string
      strcpy(RequestText, "Enter new range for the waterfall in dB (1 to 80)");

      // Define initial value in dB
      snprintf(InitText, 25, "%d", WaterfallRange);

      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
      }
      while ((strlen(KeyboardReturn) == 0) || (atoi(KeyboardReturn) < 1) || (atoi(KeyboardReturn) > 80));

      WaterfallRange = atoi(KeyboardReturn);
      snprintf(ValueToSave, 63, "%d", WaterfallRange);
      SetConfigParam(PATH_CONFIG, "wfallrange", ValueToSave);
      printf("Waterfall Range set to %d dB\n", WaterfallRange);
    break;
    case 12:                                            // Set Waterfall Base
      // Define request string
      strcpy(RequestText, "Enter new waterfall span in seconds (0 for minimum)");

      // Define initial value
      snprintf(InitText, 25, "%d", wfalltimespan);

      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
      }
      while ((strlen(KeyboardReturn) == 0) || (atoi(KeyboardReturn) < 0));

      wfalltimespan = atoi(KeyboardReturn);
      snprintf(ValueToSave, 63, "%d", wfalltimespan);
      SetConfigParam(PATH_CONFIG, "wfalltimespan", ValueToSave);
      printf("Waterfall timespan set to %d seconds\n", wfalltimespan);
    break;
  }
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawTickMarks();    // tick marks on X axis
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  freeze = false;
}


void ShiftFrequency(int button)
{  
  char ValueToSave[63];

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 5:                                                       // Left 1/10 span
      centrefreq = centrefreq - (span * 125) / 1280;
    break;
    case 6:                                                       // Left 1/10 span
      centrefreq = centrefreq + (span * 125) / 1280;
    break;
  }

  centrefreq = ConstrainFreq(centrefreq);

  // Store the new frequency
  snprintf(ValueToSave, 63, "%d", centrefreq);
  SetConfigParam(PATH_CONFIG, "centrefreq", ValueToSave);
  printf("Centre Freq set to %d \n", centrefreq);

  // Calculate the new settings
  CalcSpan();

  // Trigger the frequency change
  NewFreq = true;

  DrawSettings();       // New labels
  freeze = false;
}

void CalcSpan()    // takes centre frequency and span and calulates startfreq and stopfreq
{
  startfreq = centrefreq - (span * 125) / 256;
  stopfreq =  centrefreq + (span * 125) / 256;
  frequency_actual_rx = 1000.0 * (float)(centrefreq);

  if (span == 102)
  {
    bandwidth = 102400.0;
  }
  else if (span == 205)
  {
    bandwidth = 204800.0;
  }
  else
  {
    bandwidth = (float)(span * 1000);
  }

  // set a sensible time constant for the fft display
  if (Range20dB == false)
  {
    switch (span)
    {
      //case 102:                                             // 100 kHz
      //  smoothing_factor =  0.88;
      //  break;
      default:
        smoothing_factor =  0.7;
    }
  }
  else
  {
    switch (span)
    {
//      case 102:                                             // 100 kHz
//        smoothing_factor =  0.99;
//        break;
//      case 205:                                             // 200 kHz
//        smoothing_factor =  0.992;
//        break;
//      case 512:                                             // 500 kHz
//        smoothing_factor =  0.994;
//        break;
//      case 1024:                                            // 1 MHz
//        smoothing_factor =  0.9955;
//        break;
//      case 2048:                                            // 2 MHz
//        smoothing_factor =  0.9965;
//        break;
//      case 5120:                                            // 5 MHz
//        smoothing_factor =  0.997;
//        break;
//      case 10240:                                           // 10 MHz
//        smoothing_factor =  0.9981;
//        break;
//      case 20480:                                           // 20 MHz
//        smoothing_factor =  0.9985;
//        break;
      default:
        smoothing_factor =  0.7;
    }
  }

  if (waterfall == true)
  {
    smoothing_factor =  0;
  }
}


uint32_t ConstrainFreq(uint32_t CheckFreq)  // Check Freq is within bounds
{
  if (CheckFreq < 30000) // 30 MHz
  {
    return 30000;
  }
  if (CheckFreq > 3500000)  // 3.5 GHz
  {
    return 3500000;
  }
  return CheckFreq;
}


void ChangeLabel(int button)
{
  char RequestText[64];
  char InitText[63];
  div_t div_10;
  div_t div_100;
  div_t div_1000;
  char ValueToSave[63];
  
  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 2:                                                       // Centre Freq
      // Define request string
      strcpy(RequestText, "Enter new centre frequency in MHz");

      // Define initial value and convert to MHz
      div_10 = div(centrefreq, 10);
      div_1000 = div(centrefreq, 1000);

      if(div_10.rem != 0)  // last character not zero, so make answer of form xxx.xxx
      {
        snprintf(InitText, 25, "%d.%03d", div_1000.quot, div_1000.rem);
      }
      else
      {
        div_100 = div(centrefreq, 100);
        if(div_100.rem != 0)  // last but one character not zero, so make answer of form xxx.xx
        {
          snprintf(InitText, 25, "%d.%02d", div_1000.quot, div_1000.rem / 10);
        }
        else
        {
          if(div_1000.rem != 0)  // last but two character not zero, so make answer of form xxx.x
          {
            snprintf(InitText, 25, "%d.%d", div_1000.quot, div_1000.rem / 100);
          }
          else  // integer MHz, so just xxx (no dp)
          {
            snprintf(InitText, 25, "%d", div_1000.quot);
          }
        }
      }

      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
      }
      while (strlen(KeyboardReturn) == 0);

      centrefreq = (int)((1000 * atof(KeyboardReturn)) + 0.1);

      snprintf(ValueToSave, 63, "%d", centrefreq);
      SetConfigParam(PATH_CONFIG, "centrefreq", ValueToSave);
      printf("Centre Freq set to %d \n", centrefreq);
      NewFreq = true;
      break;

    case 6:                                                       // Plot Title
      strcpy(RequestText, "Enter the title to be displayed on this plot");
      if(strcmp(PlotTitle, "-") == 0)           // Active Blank
      {
        InitText[0] = '\0';
      }
      else
      {
        snprintf(InitText, 63, "%s", PlotTitle);
      }

      Keyboard(RequestText, InitText, 30);
  
      if(strlen(KeyboardReturn) > 0)
      {
        strcpy(PlotTitle, KeyboardReturn);
      }
      else
      {
        strcpy(PlotTitle, "-");
      }
      SetConfigParam(PATH_CONFIG, "title", PlotTitle);
      printf("Plot Title set to: %s\n", KeyboardReturn);
      break;

    case 16:                                                       // Offset Freq
      // Define request string
      strcpy(RequestText, "Enter freq offset in MHz. 0 for none, -ve for high side");

      // Define initial value and convert to MHz
      div_10 = div(freqoffset, 10);
      div_1000 = div(freqoffset, 1000);

      if (freqoffset >= 0)   // zero or positive offset
      {
        if(div_10.rem != 0)  // last character not zero, so make answer of form xxx.xxx
        {
          snprintf(InitText, 25, "%d.%03d", div_1000.quot, div_1000.rem);
        }
        else
        {
          div_100 = div(freqoffset, 100);
          if(div_100.rem != 0)  // last but one character not zero, so make answer of form xxx.xx
          {
            snprintf(InitText, 25, "%d.%02d", div_1000.quot, div_1000.rem / 10);
          }
          else
          {
            if(div_1000.rem != 0)  // last but two character not zero, so make answer of form xxx.x
            {
              snprintf(InitText, 25, "%d.%d", div_1000.quot, div_1000.rem / 100);
            }
            else  // integer MHz, so just xxx (no dp)
            {
              snprintf(InitText, 25, "%d", div_1000.quot);
            }
          }
        }
      }
      else              // Negative offset
      {
        if(div_10.rem != 0)  // last character not zero, so make answer of form xxx.xxx
        {
          snprintf(InitText, 25, "%d.%03d", div_1000.quot, 0 - div_1000.rem);
        }
        else
        {
          div_100 = div(freqoffset, 100);
          if(div_100.rem != 0)  // last but one character not zero, so make answer of form xxx.xx
          {
            snprintf(InitText, 25, "%d.%02d", div_1000.quot, 0 - (div_1000.rem / 10));
          }
          else
          {
            if(div_1000.rem != 0)  // last but two character not zero, so make answer of form xxx.x
            {
              snprintf(InitText, 25, "%d.%d", div_1000.quot, 0 - (div_1000.rem / 100));
            }
            else  // integer MHz, so just xxx (no dp)
            {
              snprintf(InitText, 25, "%d", div_1000.quot);
            }
          }
        }
      }

      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
      }
      while (strlen(KeyboardReturn) == 0);

      if (atof(KeyboardReturn) >= 0)
      {
        freqoffset = (int)((1000 * atof(KeyboardReturn)) + 0.1);
      }
      else   // negative frequncy offset
      {
        freqoffset = (int)((1000 * atof(KeyboardReturn)) - 0.1);
      }

      snprintf(ValueToSave, 63, "%d", freqoffset);
      SetConfigParam(PATH_CONFIG, "freqoffset", ValueToSave);
      printf("Freq Offset set to %d \n", freqoffset);
      break;
  }

  // Tidy up, paint around the screen and then unfreeze
  CalcSpan();         // work out start and stop freqs
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  freeze = false;
}

void RedrawDisplay()
{
  // Redraw the Y Axis
  DrawYaxisLabels();

}


//////////////////////////////////////// DEAL WITH TOUCH EVENTS //////////////////////////////////////


void *WaitButtonEvent(void * arg)
{
  int  rawPressure;
  char ValueToSave[63];

  for (;;)
  {
    while(getTouchSample(&rawX, &rawY, &rawPressure)==0)
    {
      usleep(10);                                   // wait without burnout
    }

    printf("x=%d y=%d\n", rawX, rawY);
    FinishedButton = 1;
    i = IsMenuButtonPushed(rawX, rawY);
    if (i == -1)
    {
      continue;  //Pressed, but not on a button so wait for the next touch
    }
    // Now do the reponses for each Menu in turn

    if (CurrentMenu == 1)  // Main Menu
    {
      printf("Button Event %d, Entering Menu 1 Case Statement\n",i);
      CallingMenu = 1;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Select Settings
          printf("Settings Menu 3 Requested\n");
          CurrentMenu = 3;
          Start_Highlights_Menu3();
          UpdateWindow();
          break;
        case 3:                                            // Markers
          printf("Markers Menu 2 Requested\n");
          CurrentMenu=2;
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 4:                                            // Mode
          if (Range20dB == false)
          {
            printf("Mode Menu 5 Requested\n");
            CurrentMenu = 5;
          }
          else
          {
            printf("20dB Menu Requested\n");
            CurrentMenu = 11;
          }
          UpdateWindow();
          break;
        case 5:                                            // Left Arrow
        case 6:                                            // Right Arrow
          printf("Shift Frequency Requested\n");
          ShiftFrequency(i);
          //UpdateWindow();
          RequestPeakValueZero = true;
          break;
        case 7:                                            // System
          printf("System Menu 4 Requested\n");
          CurrentMenu=4;
          Start_Highlights_Menu4();
          UpdateWindow();
          break;
        case 8:                                            // Exit to Portsdown
          if(PortsdownExitRequested)
          {
            freeze = true;
            usleep(100000);
            clearScreen(0, 0, 0);
            UpdateWeb();
            usleep(1000000);
            closeScreen();
            cleanexit(129);
          }
          else
          {
            PortsdownExitRequested = true;
            Start_Highlights_Menu1();
            UpdateWindow();
          }
          break;
        case 9:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 1 Error\n");
      }
      if(i != 8)
      {
        PortsdownExitRequested = false;
        Start_Highlights_Menu1();
        UpdateWindow();
      }
      continue;  // Completed Menu 1 action, go and wait for touch
    }

    if (CurrentMenu == 2)  // Marker Menu
    {
      printf("Button Event %d, Entering Menu 2 Case Statement\n",i);
      CallingMenu = 2;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Peak
          if ((markeron == false) || (markermode != 2))
          {
            // Freeze the scan to prevent text corruption
            freeze = true;
            while(! frozen)
            {
              usleep(10);                                   // wait till the end of the scan
            }
            markeron = true;
            markermode = i;
            SetButtonStatus(ButtonNumber(CurrentMenu, 2), 1);
            SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
          }
          else
          {
            markeron = false;
            markermode = 7;
            SetButtonStatus(ButtonNumber(CurrentMenu, 2), 0);
            SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
          }
          UpdateWindow();
          freeze = false;
          break;
        case 3:                                            // Peak Hold
        if (PeakPlot == false)
          {
            // Freeze the scan to prevent text corruption
            freeze = true;
            while(! frozen)
            {
              usleep(10);                                   // wait till the end of the scan
            }
            PeakPlot = true;
            SetButtonStatus(ButtonNumber(CurrentMenu, 3), 1);
          }
          else
          {
            PeakPlot = false;
            RequestPeakValueZero = true;
            SetButtonStatus(ButtonNumber(CurrentMenu, 3), 0);
          }
          UpdateWindow();
          freeze = false;
          break;
        case 4:                                            // Manual
          if ((markeron == false) || (markermode != 4))
          {
            // Freeze the scan to prevent text corruption
            freeze = true;
            while(! frozen)
            {
              usleep(10);                                   // wait till the end of the scan
            }
            markeron = true;
            markermode = i;
            SetButtonStatus(ButtonNumber(CurrentMenu, 2), 0);
            SetButtonStatus(ButtonNumber(CurrentMenu, 4), 1);
          }
          else
          {
            markeron = false;
            markermode = 7;
            SetButtonStatus(ButtonNumber(CurrentMenu, 2), 0);
            SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
          }
          Start_Highlights_Menu2();
          UpdateWindow();
          freeze = false;
          break;
        case 5:                                            // Left Arrow
          if ((manualmarkerx > 10) && (markeron == true) && (markermode = 4))
          {
            manualmarkerx = manualmarkerx - 10;
          }
          break;
        case 6:                                            // Right Arrow
          if ((manualmarkerx < 490) && (markeron == true) && (markermode = 4))
          {
            manualmarkerx = manualmarkerx + 10;
          }
          break;
        case 7:                                            // No Markers
          markeron = false;
          markermode = i;
          PeakPlot = false;
          RequestPeakValueZero = true;
          SetButtonStatus(ButtonNumber(CurrentMenu, 2), 0);
          SetButtonStatus(ButtonNumber(CurrentMenu, 3), 0);
          SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
          UpdateWindow();
          break;
        case 8:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          Start_Highlights_Menu1();
          UpdateWindow();
          break;
        case 9:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 9), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 9), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 2 Error\n");
      }
      continue;  // Completed Menu 2 action, go and wait for touch
    }

    if (CurrentMenu == 3)  // Settings Menu
    {
      printf("Button Event %d, Entering Menu 3 Case Statement\n",i);
      CallingMenu = 3;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Centre Freq
          ChangeLabel(i);
          CurrentMenu = 3;
          UpdateWindow();          
          RequestPeakValueZero = true;
          break;
        case 3:                                            // Frequency Presets
          printf("Frequency Preset Menu 7 Requested\n");
          CurrentMenu = 7;
          Start_Highlights_Menu7();
          UpdateWindow();
          RequestPeakValueZero = true;
          break;
        case 4:                                            // Span Width
          printf("Span Width Menu 6 Requested\n");
          CurrentMenu = 6;
          Start_Highlights_Menu6();
          UpdateWindow();
          RequestPeakValueZero = true;
          break;
        case 5:                                            // Scope inverted/normal
          if (invert_scope == true)
          {
            invert_scope = false;
            SetConfigParam(PATH_CONFIG, "invert", "no");
          }
          else
          {
            invert_scope = true;
            SetConfigParam(PATH_CONFIG, "invert", "yes");
          }
          Start_Highlights_Menu3();
          UpdateWindow();
          break;
        case 6:                                            // Waterfall Config
          printf("Waterfall Config Menu 12 Requested\n");
          CurrentMenu = 12;
          UpdateWindow();
          RequestPeakValueZero = true;
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          Start_Highlights_Menu1();
          CurrentMenu=1;
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 3 Error\n");
      }
      continue;  // Completed Menu 3 action, go and wait for touch
    }

    if (CurrentMenu == 4)  // System Menu
    {
      printf("Button Event %d, Entering Menu 4 Case Statement\n",i);
      CallingMenu = 4;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Snap Viewer
          freeze = true;
          while (frozen == false)   // Stop writing to screen
          {
            usleep(10);
          }
          do_snapcheck();           // Call the snapcheck

          initScreen();             // Start the screen again
          DrawEmptyScreen();        // Required to set A value, which is not set in DrawTrace
          DrawYaxisLabels();        // dB calibration on LHS
          DrawSettings();           // Start, Stop RBW, Ref level and Title
          UpdateWindow();           // Draw the buttons
          freeze = false;           // Restart the scan
          break;
        case 3:                                            // Config Menu
          printf("Config Menu 9 Requested\n");
          CurrentMenu = 9;
          Start_Highlights_Menu9();
          UpdateWindow();
          break;
        case 4:                                            // Spectrum/scope
          if (scope == true)
          {
            scope = false;
            SetConfigParam(PATH_CONFIG, "display", "spectrum");
          }
          else
          {
            scope = true;
            SetConfigParam(PATH_CONFIG, "display", "scope");
          }

          DrawEmptyScreen();        // Required to set A value, which is not set in DrawTrace
          DrawYaxisLabels();        // dB calibration on LHS
          DrawSettings();           // Start, Stop RBW, Ref level and Title
          UpdateWindow();           // Draw the buttons
          freeze = false;           // Restart the scan
          Start_Highlights_Menu4();
          UpdateWindow();
          break;
        case 5:                                            // Restart pico viewer
          freeze = true;
          usleep(100000);
          clearScreen(0, 0, 0);
          usleep(1000000);
          closeScreen();
          cleanexit(181);
          break;
        case 6:                                            // Shutdown
          system("sudo shutdown now");
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          Start_Highlights_Menu1();
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 4 Error\n");
      }
      continue;  // Completed Menu 4 action, go and wait for touch
    }

    if (CurrentMenu == 5)  // Mode Menu
    {
      printf("Button Event %d, Entering Menu 5 Case Statement\n",i);
      CallingMenu = 5;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Classic SA Mode
          SetMode(i);
          Start_Highlights_Menu5();
          //CalcSpan();
          //RedrawDisplay();
          break;
        case 3:                                            // Show 20 dB range
          SetMode(i);
          Range20dB = true;
          CalcSpan();
          RedrawDisplay();
          printf("20dB Menu 11 Requested\n");
          CurrentMenu = 11;
          UpdateWindow();
          break;
        case 4:                                            // Waterfall
        case 5:                                            // Mix
          //Range20dB = false;
          SetMode(i);
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 6:                                            // Show 20 dB lower
          printf("20 dB lower requested\n");
          if (BaselineShift == 20)
          {
            BaselineShift = 0;
          }
          else
          {
            BaselineShift = 20;
          }
          snprintf(ValueToSave, 8, "%d", BaselineShift);
          SetConfigParam(PATH_CONFIG, "baselineshift", ValueToSave);
          DrawYaxisLabels();
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          Start_Highlights_Menu1();
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 5 Error\n");
      }
      continue;  // Completed Menu 5 action, go and wait for touch
    }

    if (CurrentMenu == 6)  // Span Width Menu
    {
      printf("Button Event %d, Entering Menu 6 Case Statement\n",i);
      CallingMenu = 6;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // 100 kHz
        case 3:                                            // 200 kHz
        case 4:                                            // 500 kHz
        case 5:                                            // 1
        case 6:                                            // 2
        case 7:                                            // 5
        case 8:                                            // 10
        case 9:                                            // 20
          SetSpanWidth(i);
          CurrentMenu = 6;
          Start_Highlights_Menu6();
          UpdateWindow();
          break;
        case 10:                                            // Return to Settings Menu
          CurrentMenu=3;
          UpdateWindow();
          break;
        case 11:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 9), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 9), 1);
            freeze = true;
          }
          Start_Highlights_Menu6();
          UpdateWindow();
          break;
        default:
          printf("Menu 6 Error\n");
      }
      continue;  // Completed Menu 6 action, go and wait for touch
    }

    if (CurrentMenu == 7)  // Frequency Preset Menu
    {
      printf("Button Event %d, Entering Menu 7 Case Statement\n",i);
      CallingMenu = 7;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // pfreq1
        case 3:                                            // pfreq2 
        case 4:                                            // pfreq3
        case 5:                                            // pfreq4
        case 6:                                            // pfreq5
          SetFreqPreset(i);
          CurrentMenu = 3;
          UpdateWindow();
          break;
        case 7:                                            // Return to Settings Menu
          printf("Settings Menu 3 requested\n");
          CurrentMenu=3;
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 5 Error\n");
      }
      continue;  // Completed Menu 5 action, go and wait for touch
    }

    if (CurrentMenu == 8)  // LimeGain Menu
    {
      printf("Button Event %d, Entering Menu 8 Case Statement\n",i);
      CallingMenu = 8;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // 100%
        case 3:                                            // 90%
        case 4:                                            // 70%
        case 5:                                            // 50%
        case 6:                                            // 30%
          //SetLimeGain(i);
          Start_Highlights_Menu8();
          CurrentMenu = 8;
          UpdateWindow();
          break;
        case 7:                                            // Return to Settings Menu
          printf("Settings Menu 3 requested\n");
          CurrentMenu=3;
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          Start_Highlights_Menu8();
          UpdateWindow();
          break;
        default:
          printf("Menu 8 Error\n");
      }
      continue;  // Completed Menu 8 action, go and wait for touch
    }

    if (CurrentMenu == 9)  // Config Menu
    {
      printf("Button Event %d, Entering Menu 9 Case Statement\n",i);
      CallingMenu = 9;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Freq Presets
          printf("Freq Presets Menu 10 Requested\n");
          CurrentMenu = 10;
          Start_Highlights_Menu10();
          UpdateWindow();
          break;
        case 3:                                            // Freq Offset
          ChangeLabel(16);
          Start_Highlights_Menu9();
          UpdateWindow();
          break;
        case 4:                                            // Edit Title
          ChangeLabel(6);
          UpdateWindow();          
          break;
        case 5:                                            // 
          break;
        case 6:                                            // 
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 9 Error\n");
      }
      continue;  // Completed Menu 9 action, go and wait for touch
    }

    if (CurrentMenu == 10)  // Frequency Preset Setting Menu
    {
      printf("Button Event %d, Entering Menu 10 Case Statement\n",i);
      CallingMenu = 10;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // pfreq1
        case 3:                                            // pfreq2 
        case 4:                                            // pfreq3
        case 5:                                            // pfreq4
        case 6:                                            // pfreq5
          SetFreqPreset(i);
          CurrentMenu = 10;
          Start_Highlights_Menu10();
          UpdateWindow();
          break;
        case 7:                                            // Return to Main Menu
          printf("Settings Menu 1 requested\n");
          CurrentMenu=1;
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 10 Error\n");
      }
      continue;  // Completed Menu 10 action, go and wait for touch
    }
    if (CurrentMenu == 11)  // 20 dB range Menu
    {
      printf("Button Event %d, Entering Menu 11 Case Statement\n",i);
      CallingMenu = 11;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Back to Full Range
          Range20dB = false;
          SetConfigParam(PATH_CONFIG, "mode", "spectrum");
          CalcSpan();
          RedrawDisplay();
          CurrentMenu=1;
          UpdateWindow();
          break;
        case 5:                                            // up
          if (BaseLine20dB <= -25)
          {
            BaseLine20dB = BaseLine20dB + 5;
            RedrawDisplay();
            snprintf(ValueToSave, 8, "%d", BaseLine20dB);
            SetConfigParam(PATH_CONFIG, "baseline20db", ValueToSave);
          }
          UpdateWindow();
          break;
        case 6:                                            // down
          if (BaseLine20dB >= -75)
          {
            BaseLine20dB = BaseLine20dB - 5;
            RedrawDisplay();
            snprintf(ValueToSave, 8, "%d", BaseLine20dB);
            SetConfigParam(PATH_CONFIG, "baseline20db", ValueToSave);
          }
          UpdateWindow();
          break;
        case 7:                                            // Return to Main Menu
          printf("Settings Menu 1 requested\n");
          CurrentMenu=1;
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 11 Error\n");
      }
      continue;  // Completed Menu 11 action, go and wait for touch
    }
    if (CurrentMenu == 12)  // Waterfall Config
    {
      printf("Button Event %d, Entering Menu 12 Case Statement\n",i);
      CallingMenu = 12;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Set Waterfall Base
        case 3:                                            // Set waterfall range
          SetWfall(i);
          UpdateWindow();
          break;
        case 4:                                            // Set Waterfall TimeSpan
          SetWfall(12);
          UpdateWindow();
          break;
        case 5:                                            // Set Waterfall trace type 
          if (strcmp(TraceType, "single") == 0)
          {
            strcpy(TraceType, "average");
          }
          else if (strcmp(TraceType, "average") == 0)
          {
            strcpy(TraceType, "peak");
          }
          else if (strcmp(TraceType, "peak") == 0)
          {
            strcpy(TraceType, "single");
          }
          SetConfigParam(PATH_CONFIG, "tracetype", TraceType);
          Start_Highlights_Menu12();
          UpdateWindow();
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          UpdateWindow();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
            freeze = true;
          }
          UpdateWindow();
          break;
        default:
          printf("Menu 12 Error\n");
      }
      continue;  // Completed Menu 9 action, go and wait for touch
    }
  }
  return NULL;
}


/////////////////////////////////////////////// DEFINE THE BUTTONS ///////////////////////////////

void Define_Menu1()                                  // Main Menu

{
  int button = 0;

  button = CreateButton(1, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(1, 1);
  AddButtonStatus(button, "Main Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 2);
  AddButtonStatus(button, "Settings", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 3);
  AddButtonStatus(button, "Markers", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 4);
  AddButtonStatus(button, "Mode", &Blue);
  AddButtonStatus(button, "", &Green);

  button = CreateButton(1, 5);
  AddButtonStatus(button, "<-", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 6);
  AddButtonStatus(button, "->", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 7);
  AddButtonStatus(button, "System", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 8);
  AddButtonStatus(button, "Exit to^Portsdown", &DBlue);
  AddButtonStatus(button, "Exit to^Portsdown", &Red);

  button = CreateButton(1, 9);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu1()
{
  if (PortsdownExitRequested)
  {
    SetButtonStatus(ButtonNumber(1, 8), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(1, 8), 0);
  }
}

void Define_Menu2()                                         // Marker Menu
{
  int button = 0;

  button = CreateButton(2, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(2, 1);
  AddButtonStatus(button, "Marker^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(2, 2);
  AddButtonStatus(button, "Peak", &Blue);
  AddButtonStatus(button, "Peak", &Green);

  button = CreateButton(2, 3);
  AddButtonStatus(button, "Peak^Hold", &Blue);
  AddButtonStatus(button, "Peak^Hold", &Green);

  button = CreateButton(2, 4);
  AddButtonStatus(button, "Manual", &Blue);
  AddButtonStatus(button, "Manual", &Green);

  button = CreateButton(2, 5);
  AddButtonStatus(button, "<-", &Blue);
  AddButtonStatus(button, "<-", &Grey);

  button = CreateButton(2, 6);
  AddButtonStatus(button, "->", &Blue);
  AddButtonStatus(button, "->", &Grey);

  button = CreateButton(2, 7);
  AddButtonStatus(button, "Markers^Off", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(2, 8);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(2, 9);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu2()
{
  if ((markeron == true) && (markermode == 4))  // Manual Markers
  {
    SetButtonStatus(ButtonNumber(2, 5), 0);
    SetButtonStatus(ButtonNumber(2, 6), 0);
  }
  else
  {
    SetButtonStatus(ButtonNumber(2, 5), 1);
    SetButtonStatus(ButtonNumber(2, 6), 1);
  }
}


void Define_Menu3()                                           // Settings Menu
{
  int button = 0;

  button = CreateButton(3, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(3, 1);
  AddButtonStatus(button, "Settings^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 2);
  AddButtonStatus(button, "Centre^Freq", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 3);
  AddButtonStatus(button, "Freq^Presets", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 4);
  AddButtonStatus(button, "Span^Width", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 5);
  AddButtonStatus(button, "Scope^Normal", &Blue);
  AddButtonStatus(button, "Scope^Inverted", &Blue);


  button = CreateButton(3, 6);
  AddButtonStatus(button, "Waterfall^Config", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(3, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu3()
{
  if (invert_scope == true)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 0);
  }
}


void Define_Menu4()                                         // System Menu
{
  int button = 0;

  button = CreateButton(4, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(4, 1);
  AddButtonStatus(button, "System^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 2);
  AddButtonStatus(button, "Snap^Viewer", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 3);
  AddButtonStatus(button, "Config^Menu", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 4);
  AddButtonStatus(button, "Scope", &Blue);
  AddButtonStatus(button, "Spectrum", &Blue);

  button = CreateButton(4, 5);
  AddButtonStatus(button, "Re-start^BandView", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 6);
  AddButtonStatus(button, "Shutdown^System", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(4, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu4()
{
  if (scope == true)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 4), 1);
  }
}


void Define_Menu5()                                          // Mode Menu
{
  int button = 0;

  button = CreateButton(5, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(5, 1);
  AddButtonStatus(button, "Mode^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(5, 2);
  AddButtonStatus(button, "Classic^SA Display", &Blue);
  AddButtonStatus(button, "Classic^SA Display", &Green);

  button = CreateButton(5, 3);
  AddButtonStatus(button, "20 dB Range^SA Display", &Blue);
  AddButtonStatus(button, "20 dB Range^SA Display", &Green);

  button = CreateButton(5, 4);
  AddButtonStatus(button, "Waterfall", &Blue);
  AddButtonStatus(button, "Waterfall", &Green);

  button = CreateButton(5, 5);
  AddButtonStatus(button, "Mix", &Blue);
  AddButtonStatus(button, "Mix", &Green);

  button = CreateButton(5, 6);
  AddButtonStatus(button, "Show 20dB^Lower", &Blue);
  AddButtonStatus(button, "Show 20dB^Lower", &Green);

  button = CreateButton(5, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(5, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu5()
{
  if (BaselineShift == 0)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 6), 0);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 6), 1);
  }
}

void Define_Menu6()                                           // Span Menu
{
  int button = 0;

  button = CreateButton(6, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(6, 1);
  AddButtonStatus(button, "Span^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(6, 2);
  AddButtonStatus(button, "100", &Blue);
  AddButtonStatus(button, "100", &Green);

  button = CreateButton(6, 3);
  AddButtonStatus(button, "200", &Blue);
  AddButtonStatus(button, "200", &Green);

  button = CreateButton(6, 4);
  AddButtonStatus(button, "500 kHz", &Blue);
  AddButtonStatus(button, "500 kHz", &Green);

  button = CreateButton(6, 5);
  AddButtonStatus(button, "1 MHz", &Blue);
  AddButtonStatus(button, "1 MHz", &Green);

  button = CreateButton(6, 6);
  AddButtonStatus(button, "2", &Blue);
  AddButtonStatus(button, "2", &Green);

  button = CreateButton(6, 7);
  AddButtonStatus(button, "5", &Blue);
  AddButtonStatus(button, "5", &Green);

  button = CreateButton(6, 8);
  AddButtonStatus(button, "10", &Blue);
  AddButtonStatus(button, "10", &Green);

  button = CreateButton(6, 9);
  AddButtonStatus(button, "20", &Blue);
  AddButtonStatus(button, "20", &Green);

  button = CreateButton(6, 10);
  AddButtonStatus(button, "Back to^Settings", &DBlue);

  button = CreateButton(6, 11);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu6()
{
  if (span == 102)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 2), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 2), 0);
  }
  if (span == 205)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 3), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 3), 0);
  }
  if (span == 512)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 4), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
  }
  if (span == 1024)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 0);
  }
  if (span == 2048)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 6), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 6), 0);
  }
  if (span == 5120)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 7), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 7), 0);
  }
  if (span == 10240)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 8), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 8), 0);
  }
  if (span == 20480)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 9), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 9), 0);
  }
}

void Define_Menu7()                                            //Presets Menu
{
  int button = 0;

  button = CreateButton(7, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(7, 1);
  AddButtonStatus(button, "Presets^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(7, 2);
  AddButtonStatus(button, "146.5 MHz", &Blue);
  AddButtonStatus(button, "146.5 MHz", &Green);

  button = CreateButton(7, 3);
  AddButtonStatus(button, "437 MHz", &Blue);
  AddButtonStatus(button, "437 MHz", &Green);

  button = CreateButton(7, 4);
  AddButtonStatus(button, "748 MHz", &Blue);
  AddButtonStatus(button, "748 MHz", &Green);

  button = CreateButton(7, 5);
  AddButtonStatus(button, "1255 MHz", &Blue);
  AddButtonStatus(button, "1255 MHz", &Green);

  button = CreateButton(7, 6);
  AddButtonStatus(button, "2409 MHz", &Blue);
  AddButtonStatus(button, "2409 MHz", &Green);

  button = CreateButton(7, 7);
  AddButtonStatus(button, "Back to^Settings", &DBlue);

  button = CreateButton(7, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu7()
{
  char ButtText[15];
  snprintf(ButtText, 14, "%0.1f MHz", ((float)pfreq1) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 2), 0, ButtText, &Blue);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 2), 1, ButtText, &Green);

  snprintf(ButtText, 14, "%0.1f MHz", ((float)pfreq2) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 3), 0, ButtText, &Blue);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 3), 1, ButtText, &Green);

  snprintf(ButtText, 14, "%0.1f MHz", ((float)pfreq3) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 4), 0, ButtText, &Blue);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 4), 1, ButtText, &Green);

  snprintf(ButtText, 14, "%0.1f MHz", ((float)pfreq4) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 5), 0, ButtText, &Blue);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 5), 1, ButtText, &Green);

  snprintf(ButtText, 14, "%0.1f MHz", ((float)pfreq5) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 6), 0, ButtText, &Blue);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 6), 1, ButtText, &Green);


  if (centrefreq == pfreq1)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 2), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 2), 0);
  }
  if (centrefreq == pfreq2)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 3), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 3), 0);
  }
  if (centrefreq == pfreq3)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 4), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
  }
  if (centrefreq == pfreq4)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 0);
  }
  if (centrefreq == pfreq5)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 6), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 6), 0);
  }
}


void Define_Menu8()                                    // Lime Gain Menu
{
  int button = 0;

  button = CreateButton(8, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(8, 1);
  AddButtonStatus(button, "Gain^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(8, 2);
  AddButtonStatus(button, "100%", &Blue);
  AddButtonStatus(button, "100%", &Green);

  button = CreateButton(8, 3);
  AddButtonStatus(button, "90%", &Blue);
  AddButtonStatus(button, "90%", &Green);

  button = CreateButton(8, 4);
  AddButtonStatus(button, "70%", &Blue);
  AddButtonStatus(button, "70%", &Green);

  button = CreateButton(8, 5);
  AddButtonStatus(button, "50%", &Blue);
  AddButtonStatus(button, "50%", &Green);

  button = CreateButton(8, 6);
  AddButtonStatus(button, "30%", &Blue);
  AddButtonStatus(button, "30%", &Green);

  button = CreateButton(8, 7);
  AddButtonStatus(button, "Back to^Settings", &DBlue);

  button = CreateButton(8, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu8()
{
  //
}

void Define_Menu9()                                          // Config Menu
{
  int button = 0;

  button = CreateButton(9, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(9, 1);
  AddButtonStatus(button, "Config^Menu", &Black);

  button = CreateButton(9, 2);
  AddButtonStatus(button, "Set Freq^Presets", &Blue);

  button = CreateButton(9, 3);
  AddButtonStatus(button, "Set Freq^Offset", &Blue);
  AddButtonStatus(button, "Set Freq^Offset", &Green);

  button = CreateButton(9, 4);
  AddButtonStatus(button, "Edit^Title", &Blue);

  //button = CreateButton(9, 5);
  //AddButtonStatus(button, " ", &Blue);
  //AddButtonStatus(button, " ", &Green);

  //button = CreateButton(9 6);
  //AddButtonStatus(button, "Set^Config", &Blue);

  button = CreateButton(9, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(9, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu9()
{
  if (freqoffset == 0)
  {
    SetButtonStatus(ButtonNumber(9, 3), 0);
  }
  else
  {
    SetButtonStatus(ButtonNumber(9, 3), 1);
  }
}



void Define_Menu10()                                          // Set Freq Presets Menu
{
  int button = 0;

  button = CreateButton(10, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(10, 1);
  AddButtonStatus(button, "Set Freq^Presets", &Black);

  button = CreateButton(10, 2);
  AddButtonStatus(button, "Preset 1", &Blue);

  button = CreateButton(10, 3);
  AddButtonStatus(button, "Preset 2", &Blue);

  button = CreateButton(10, 4);
  AddButtonStatus(button, "Preset 3", &Blue);

  button = CreateButton(10, 5);
  AddButtonStatus(button, "Preset 4", &Blue);

  button = CreateButton(10, 6);
  AddButtonStatus(button, "Preset 5", &Blue);

  button = CreateButton(10, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(10, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu10()
{
  char ButtText[31];
  snprintf(ButtText, 30, "Preset 1^%0.1f MHz", ((float)pfreq1) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 2), 0, ButtText, &Blue);

  snprintf(ButtText, 30, "Preset 2^%0.1f MHz", ((float)pfreq2) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 3), 0, ButtText, &Blue);

  snprintf(ButtText, 30, "Preset 3^%0.1f MHz", ((float)pfreq3) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 4), 0, ButtText, &Blue);

  snprintf(ButtText, 30, "Preset 4^%0.1f MHz", ((float)pfreq4) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 5), 0, ButtText, &Blue);

  snprintf(ButtText, 30, "Preset 5^%0.1f MHz", ((float)pfreq5) / 1000);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 6), 0, ButtText, &Blue);
}

void Define_Menu11()                                          // 20db Range Menu
{
  int button = 0;

  button = CreateButton(11, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(11, 1);
  AddButtonStatus(button, "Control^20 dB Range", &Black);

  button = CreateButton(11, 2);
  AddButtonStatus(button, "Back to^Full Range", &Blue);

  //button = CreateButton(11, 3);
  //AddButtonStatus(button, "Preset 2", &Blue);

  //button = CreateButton(11, 4);
  //AddButtonStatus(button, "Preset 3", &Blue);

  button = CreateButton(11, 5);
  AddButtonStatus(button, "Up", &Blue);

  button = CreateButton(11, 6);
  AddButtonStatus(button, "Down", &Blue);

  button = CreateButton(11, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(11, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Define_Menu12()                                          // Waterfall Config Menu
{
  int button = 0;

  button = CreateButton(12, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(12, 1);
  AddButtonStatus(button, "Waterfall^Config Menu", &Black);

  button = CreateButton(12, 2);
  AddButtonStatus(button, "Set Wfall^Base Level", &Blue);

  button = CreateButton(12, 3);
  AddButtonStatus(button, "Set Wfall^Range", &Blue);

  button = CreateButton(12, 4);
  AddButtonStatus(button, "Set Wfall^Time Span", &Blue);

  button = CreateButton(12, 5);
  AddButtonStatus(button, "Waterfall^Normal", &Blue);
  AddButtonStatus(button, "Waterfall^Peak", &Blue);
  AddButtonStatus(button, "Waterfall^Single", &Blue);

  //button = CreateButton(12, 6);
  //AddButtonStatus(button, "", &Blue);

  button = CreateButton(12, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(12, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu12()
{
  if (strcmp(TraceType, "average") == 0)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 0);
  }
  if (strcmp(TraceType, "peak") == 0)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 1);
  }
  if (strcmp(TraceType, "single") == 0)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 2);
  }
}


void Define_Menu41()
{
  int button;

  button = CreateButton(41, 0);
  AddButtonStatus(button, "SPACE", &Blue);
  AddButtonStatus(button, "SPACE", &LBlue);
  AddButtonStatus(button, "SPACE", &Blue);
  AddButtonStatus(button, "SPACE", &LBlue);
  //button = CreateButton(41, 1);
  //AddButtonStatus(button, "X", &Blue);
  //AddButtonStatus(button, "X", &LBlue);
  button = CreateButton(41, 2);
  AddButtonStatus(button, "<", &Blue);
  AddButtonStatus(button, "<", &LBlue);
  AddButtonStatus(button, "<", &Blue);
  AddButtonStatus(button, "<", &LBlue);
  button = CreateButton(41, 3);
  AddButtonStatus(button, ">", &Blue);
  AddButtonStatus(button, ">", &LBlue);
  AddButtonStatus(button, ">", &Blue);
  AddButtonStatus(button, ">", &LBlue);
  button = CreateButton(41, 4);
  AddButtonStatus(button, "-", &Blue);
  AddButtonStatus(button, "-", &LBlue);
  AddButtonStatus(button, "_", &Blue);
  AddButtonStatus(button, "_", &LBlue);
  button = CreateButton(41, 5);
  AddButtonStatus(button, "Clear", &Blue);
  AddButtonStatus(button, "Clear", &LBlue);
  AddButtonStatus(button, "Clear", &Blue);
  AddButtonStatus(button, "Clear", &LBlue);
  button = CreateButton(41, 6);
  AddButtonStatus(button, "^", &LBlue);
  AddButtonStatus(button, "^", &Blue);
  AddButtonStatus(button, "^", &Blue);
  AddButtonStatus(button, "^", &LBlue);
  button = CreateButton(41, 7);
  AddButtonStatus(button, "^", &LBlue);
  AddButtonStatus(button, "^", &Blue);
  AddButtonStatus(button, "^", &Blue);
  AddButtonStatus(button, "^", &LBlue);
  button = CreateButton(41, 8);
  AddButtonStatus(button, "Enter", &Blue);
  AddButtonStatus(button, "Enter", &LBlue);
  AddButtonStatus(button, "Enter", &Blue);
  AddButtonStatus(button, "Enter", &LBlue);
  button = CreateButton(41, 9);
  AddButtonStatus(button, "<=", &Blue);
  AddButtonStatus(button, "<=", &LBlue);
  AddButtonStatus(button, "<=", &Blue);
  AddButtonStatus(button, "<=", &LBlue);
  button = CreateButton(41, 10);
  AddButtonStatus(button, "Z", &Blue);
  AddButtonStatus(button, "Z", &LBlue);
  AddButtonStatus(button, "z", &Blue);
  AddButtonStatus(button, "z", &LBlue);
  button = CreateButton(41, 11);
  AddButtonStatus(button, "X", &Blue);
  AddButtonStatus(button, "X", &LBlue);
  AddButtonStatus(button, "x", &Blue);
  AddButtonStatus(button, "x", &LBlue);
  button = CreateButton(41, 12);
  AddButtonStatus(button, "C", &Blue);
  AddButtonStatus(button, "C", &LBlue);
  AddButtonStatus(button, "c", &Blue);
  AddButtonStatus(button, "c", &LBlue);
  button = CreateButton(41, 13);
  AddButtonStatus(button, "V", &Blue);
  AddButtonStatus(button, "V", &LBlue);
  AddButtonStatus(button, "v", &Blue);
  AddButtonStatus(button, "v", &LBlue);
  button = CreateButton(41, 14);
  AddButtonStatus(button, "B", &Blue);
  AddButtonStatus(button, "B", &LBlue);
  AddButtonStatus(button, "b", &Blue);
  AddButtonStatus(button, "b", &LBlue);
  button = CreateButton(41, 15);
  AddButtonStatus(button, "N", &Blue);
  AddButtonStatus(button, "N", &LBlue);
  AddButtonStatus(button, "n", &Blue);
  AddButtonStatus(button, "n", &LBlue);
  button = CreateButton(41, 16);
  AddButtonStatus(button, "M", &Blue);
  AddButtonStatus(button, "M", &LBlue);
  AddButtonStatus(button, "m", &Blue);
  AddButtonStatus(button, "m", &LBlue);
  button = CreateButton(41, 17);
  AddButtonStatus(button, ",", &Blue);
  AddButtonStatus(button, ",", &LBlue);
  AddButtonStatus(button, "!", &Blue);
  AddButtonStatus(button, "!", &LBlue);
  button = CreateButton(41, 18);
  AddButtonStatus(button, ".", &Blue);
  AddButtonStatus(button, ".", &LBlue);
  AddButtonStatus(button, ".", &Blue);
  AddButtonStatus(button, ".", &LBlue);
  button = CreateButton(41, 19);
  AddButtonStatus(button, "/", &Blue);
  AddButtonStatus(button, "/", &LBlue);
  AddButtonStatus(button, "?", &Blue);
  AddButtonStatus(button, "?", &LBlue);
  button = CreateButton(41, 20);
  AddButtonStatus(button, "A", &Blue);
  AddButtonStatus(button, "A", &LBlue);
  AddButtonStatus(button, "a", &Blue);
  AddButtonStatus(button, "a", &LBlue);
  button = CreateButton(41, 21);
  AddButtonStatus(button, "S", &Blue);
  AddButtonStatus(button, "S", &LBlue);
  AddButtonStatus(button, "s", &Blue);
  AddButtonStatus(button, "s", &LBlue);
  button = CreateButton(41, 22);
  AddButtonStatus(button, "D", &Blue);
  AddButtonStatus(button, "D", &LBlue);
  AddButtonStatus(button, "d", &Blue);
  AddButtonStatus(button, "d", &LBlue);
  button = CreateButton(41, 23);
  AddButtonStatus(button, "F", &Blue);
  AddButtonStatus(button, "F", &LBlue);
  AddButtonStatus(button, "f", &Blue);
  AddButtonStatus(button, "f", &LBlue);
  button = CreateButton(41, 24);
  AddButtonStatus(button, "G", &Blue);
  AddButtonStatus(button, "G", &LBlue);
  AddButtonStatus(button, "g", &Blue);
  AddButtonStatus(button, "g", &LBlue);
  button = CreateButton(41, 25);
  AddButtonStatus(button, "H", &Blue);
  AddButtonStatus(button, "H", &LBlue);
  AddButtonStatus(button, "h", &Blue);
  AddButtonStatus(button, "h", &LBlue);
  button = CreateButton(41, 26);
  AddButtonStatus(button, "J", &Blue);
  AddButtonStatus(button, "J", &LBlue);
  AddButtonStatus(button, "j", &Blue);
  AddButtonStatus(button, "j", &LBlue);
  button = CreateButton(41, 27);
  AddButtonStatus(button, "K", &Blue);
  AddButtonStatus(button, "K", &LBlue);
  AddButtonStatus(button, "k", &Blue);
  AddButtonStatus(button, "k", &LBlue);
  button = CreateButton(41, 28);
  AddButtonStatus(button, "L", &Blue);
  AddButtonStatus(button, "L", &LBlue);
  AddButtonStatus(button, "l", &Blue);
  AddButtonStatus(button, "l", &LBlue);
  //button = CreateButton(41, 29);
  //AddButtonStatus(button, "/", &Blue);
  //AddButtonStatus(button, "/", &LBlue);
  button = CreateButton(41, 30);
  AddButtonStatus(button, "Q", &Blue);
  AddButtonStatus(button, "Q", &LBlue);
  AddButtonStatus(button, "q", &Blue);
  AddButtonStatus(button, "q", &LBlue);
  button = CreateButton(41, 31);
  AddButtonStatus(button, "W", &Blue);
  AddButtonStatus(button, "W", &LBlue);
  AddButtonStatus(button, "w", &Blue);
  AddButtonStatus(button, "w", &LBlue);
  button = CreateButton(41, 32);
  AddButtonStatus(button, "E", &Blue);
  AddButtonStatus(button, "E", &LBlue);
  AddButtonStatus(button, "e", &Blue);
  AddButtonStatus(button, "e", &LBlue);
  button = CreateButton(41, 33);
  AddButtonStatus(button, "R", &Blue);
  AddButtonStatus(button, "R", &LBlue);
  AddButtonStatus(button, "r", &Blue);
  AddButtonStatus(button, "r", &LBlue);
  button = CreateButton(41, 34);
  AddButtonStatus(button, "T", &Blue);
  AddButtonStatus(button, "T", &LBlue);
  AddButtonStatus(button, "t", &Blue);
  AddButtonStatus(button, "t", &LBlue);
  button = CreateButton(41, 35);
  AddButtonStatus(button, "Y", &Blue);
  AddButtonStatus(button, "Y", &LBlue);
  AddButtonStatus(button, "y", &Blue);
  AddButtonStatus(button, "y", &LBlue);
  button = CreateButton(41, 36);
  AddButtonStatus(button, "U", &Blue);
  AddButtonStatus(button, "U", &LBlue);
  AddButtonStatus(button, "u", &Blue);
  AddButtonStatus(button, "u", &LBlue);
  button = CreateButton(41, 37);
  AddButtonStatus(button, "I", &Blue);
  AddButtonStatus(button, "I", &LBlue);
  AddButtonStatus(button, "i", &Blue);
  AddButtonStatus(button, "i", &LBlue);
  button = CreateButton(41, 38);
  AddButtonStatus(button, "O", &Blue);
  AddButtonStatus(button, "O", &LBlue);
  AddButtonStatus(button, "o", &Blue);
  AddButtonStatus(button, "o", &LBlue);
  button = CreateButton(41, 39);
  AddButtonStatus(button, "P", &Blue);
  AddButtonStatus(button, "P", &LBlue);
  AddButtonStatus(button, "p", &Blue);
  AddButtonStatus(button, "p", &LBlue);

  button = CreateButton(41, 40);
  AddButtonStatus(button, "1", &Blue);
  AddButtonStatus(button, "1", &LBlue);
  AddButtonStatus(button, "1", &Blue);
  AddButtonStatus(button, "1", &LBlue);
  button = CreateButton(41, 41);
  AddButtonStatus(button, "2", &Blue);
  AddButtonStatus(button, "2", &LBlue);
  AddButtonStatus(button, "2", &Blue);
  AddButtonStatus(button, "2", &LBlue);
  button = CreateButton(41, 42);
  AddButtonStatus(button, "3", &Blue);
  AddButtonStatus(button, "3", &LBlue);
  AddButtonStatus(button, "3", &Blue);
  AddButtonStatus(button, "3", &LBlue);
  button = CreateButton(41, 43);
  AddButtonStatus(button, "4", &Blue);
  AddButtonStatus(button, "4", &LBlue);
  AddButtonStatus(button, "4", &Blue);
  AddButtonStatus(button, "4", &LBlue);
  button = CreateButton(41, 44);
  AddButtonStatus(button, "5", &Blue);
  AddButtonStatus(button, "5", &LBlue);
  AddButtonStatus(button, "5", &Blue);
  AddButtonStatus(button, "5", &LBlue);
  button = CreateButton(41, 45);
  AddButtonStatus(button, "6", &Blue);
  AddButtonStatus(button, "6", &LBlue);
  AddButtonStatus(button, "6", &Blue);
  AddButtonStatus(button, "6", &LBlue);
  button = CreateButton(41, 46);
  AddButtonStatus(button, "7", &Blue);
  AddButtonStatus(button, "7", &LBlue);
  AddButtonStatus(button, "7", &Blue);
  AddButtonStatus(button, "7", &LBlue);
  button = CreateButton(41, 47);
  AddButtonStatus(button, "8", &Blue);
  AddButtonStatus(button, "8", &LBlue);
  AddButtonStatus(button, "8", &Blue);
  AddButtonStatus(button, "8", &LBlue);
  button = CreateButton(41, 48);
  AddButtonStatus(button, "9", &Blue);
  AddButtonStatus(button, "9", &LBlue);
  AddButtonStatus(button, "9", &Blue);
  AddButtonStatus(button, "9", &LBlue);
  button = CreateButton(41, 49);
  AddButtonStatus(button, "0", &Blue);
  AddButtonStatus(button, "0", &LBlue);
  AddButtonStatus(button, "0", &Blue);
  AddButtonStatus(button, "0", &LBlue);
}




/////////////////////////////////////////// APPLICATION DRAWING //////////////////////////////////


void DrawEmptyScreen()
{
  int div = 0;
  
    HorizLine(100, 70, 500, 255, 255, 255);
    VertLine(100, 70, 400, 255, 255, 255);
    HorizLine(100, 470, 500, 255, 255, 255);
    VertLine(600, 70, 400, 255, 255, 255);

    HorizLine(101, 120, 499, 63, 63, 63);
    HorizLine(101, 170, 499, 63, 63, 63);
    HorizLine(101, 220, 499, 63, 63, 63);
    HorizLine(101, 270, 499, 63, 63, 63);
    HorizLine(101, 320, 499, 63, 63, 63);
    HorizLine(101, 370, 499, 63, 63, 63);
    HorizLine(101, 420, 499, 63, 63, 63);

    VertLine(150, 71, 399, 63, 63, 63);
    VertLine(200, 71, 399, 63, 63, 63);
    VertLine(250, 71, 399, 63, 63, 63);
    VertLine(300, 71, 399, 63, 63, 63);
    VertLine(350, 71, 399, 63, 63, 63);
    VertLine(400, 71, 399, 63, 63, 63);
    VertLine(450, 71, 399, 63, 63, 63);
    VertLine(500, 71, 399, 63, 63, 63);
    VertLine(550, 71, 399, 63, 63, 63);

  for(div = 0; div < 10; div++)
  {
    VertLine(100 + div * 50 + 10, 265, 10, 63, 63, 63);
    VertLine(100 + div * 50 + 20, 265, 10, 63, 63, 63);
    VertLine(100 + div * 50 + 30, 265, 10, 63, 63, 63);
    VertLine(100 + div * 50 + 40, 265, 10, 63, 63, 63);
  }

  for(div = 0; div < 8; div++)
  {
    HorizLine(345, 70 + div * 50 + 10, 10, 63, 63, 63);
    HorizLine(345, 70 + div * 50 + 20, 10, 63, 63, 63);
    HorizLine(345, 70 + div * 50 + 30, 10, 63, 63, 63);
    HorizLine(345, 70 + div * 50 + 40, 10, 63, 63, 63);
  }
}


void DrawTickMarks()
{
    VertLine(100, 64, 5, 255, 255, 255);
    VertLine(150, 64, 5, 255, 255, 255);
    VertLine(200, 64, 5, 255, 255, 255);
    VertLine(250, 64, 5, 255, 255, 255);
    VertLine(300, 64, 5, 255, 255, 255);
    VertLine(350, 64, 5, 255, 255, 255);
    VertLine(400, 64, 5, 255, 255, 255);
    VertLine(450, 64, 5, 255, 255, 255);
    VertLine(500, 64, 5, 255, 255, 255);
    VertLine(550, 64, 5, 255, 255, 255);
    VertLine(600, 64, 5, 255, 255, 255);
}


void DrawYaxisLabels()
{
  const font_t *font_ptr = &font_dejavu_sans_18;   // 18pt
  char caption[31];
  int i;
  int pixel_brightness;

  // Clear the previous scale first
  rectangle(20, 63, 79, 417, 0, 0, 0);

  // Clear the previous waterfall calibration
  rectangle(610, 63, 5, 417, 0, 0, 0);

  if (scope == true)
  {
    Text(60, 462, "+4", font_ptr, 0, 0, 0, 255, 255, 255);
    Text(60, 416, "+3", font_ptr, 0, 0, 0, 255, 255, 255);
    Text(60, 366, "+2", font_ptr, 0, 0, 0, 255, 255, 255);
    Text(60, 316, "+1", font_ptr, 0, 0, 0, 255, 255, 255);
    Text(72, 266, "0", font_ptr, 0, 0, 0, 255, 255, 255);
    Text(66, 216, "-1", font_ptr, 0, 0, 0, 255, 255, 255);
    Text(66, 166, "-2", font_ptr, 0, 0, 0, 255, 255, 255);
    Text(66, 116, "-3", font_ptr, 0, 0, 0, 255, 255, 255);
    Text(66,  66, "-4", font_ptr, 0, 0, 0, 255, 255, 255);
    return;
  }

  if ((Range20dB == false) && (waterfall == false))       // Full range spectrum
  {
    if (BaselineShift == 0)                     // 0 to -80
    {
      Text(15, 462, "+30 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(15, 412, "+20 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(15, 362, "+10 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(40, 316, "0 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 266, "-10 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 216, "-20 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 166, "-30 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 116, "-40 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 66, "-50 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
    }
    else                                         // -20 to -100
    {
      Text(15, 462, "+10 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(40, 416, "0 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 366, "-10 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 316, "-20 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 266, "-30 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 216, "-40 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 166, "-50 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 116, "-60 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20,  66, "-70 dBm", font_ptr, 0, 0, 0, 255, 255, 255);
    }
    // Draw the waterfall calibration chart
    for (i = 1; i <= 399; i++)
    {
      pixel_brightness = i - (400 + (5 * WaterfallBase)); // this in range -400 to +400, but only 0 to 400 is valid
      pixel_brightness = (255 * pixel_brightness) / (WaterfallRange * 5);  // scale to 0 - 255
      //printf("i = %d, Pixel = %d\n", i, pixel_brightness);
      if ((pixel_brightness < 0) || (pixel_brightness == 255) || (pixel_brightness == 256))
      {
        pixel_brightness = 0;
      }
      if (pixel_brightness > 256)
      {
        pixel_brightness = 255;
      }
      setPixel(610, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(611, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(612, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(613, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(614, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      HorizLine(610, 470 + (5 * WaterfallBase), 5, 255, 255, 255);
    }
  }
  else if ((Range20dB == true) && (waterfall == false))  // 20 dB range spectrum
  {
    snprintf(caption, 17, "%d dBm", BaseLine20dB + 20 - BaselineShift + 30);
    Text(20, 462, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    snprintf(caption, 17, "%d dBm", BaseLine20dB + 15 - BaselineShift + 30);
    Text(20, 366, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    snprintf(caption, 17, "%d dBm", BaseLine20dB + 10 - BaselineShift + 30);
    Text(20, 266, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    snprintf(caption, 17, "%d dBm", BaseLine20dB + 5 - BaselineShift + 30);
    Text(20, 166, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    snprintf(caption, 17, "%d dBm", BaseLine20dB - BaselineShift + 30);
    Text(20,  66, caption, font_ptr, 0, 0, 0, 255, 255, 255);
  }
  else if ((spectrum == false) && (waterfall == true))   // Waterfall only
  {
    Text(35, 462, "0 s", font_ptr, 0, 0, 0, 255, 255, 255);
    if (wfalltimespan > 4)   // only display requested timespan if not too fast
    {
      snprintf(caption, 15, "%3.1f s", (float)wfalltimespan * 0.5);
      Text(25, 266, caption, font_ptr, 0, 0, 0, 255, 255, 255);
      snprintf(caption, 15, "%3.1f s", (float)wfalltimespan);
      Text(25,  66, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    }
  }

  else if ((spectrum == true) && (waterfall == true))   // Mix
  {
    if (BaselineShift == 0)                     // 0 to -80
    {
      Text(48, 462, "0 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 416, "-40 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 366, "-80 dB", font_ptr, 0, 0, 0, 255, 255, 255);
    }
    else                                        // -20 to -100
    {
      Text(30, 462, "-20 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 416, "-60 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20, 366, "-100 dB", font_ptr, 0, 0, 0, 255, 255, 255);
    }
    if (wfalltimespan > 4)   // only display requested timespan if not too fast
    {
      snprintf(caption, 15, "%3.1f s", (float)wfalltimespan * 0.5);
      Text(25, 216, caption, font_ptr, 0, 0, 0, 255, 255, 255);
      snprintf(caption, 15, "%3.1f s", (float)wfalltimespan);
      Text(25,  66, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    }

    // Draw the waterfall calibration chart
    for (i = 301; i <= 399; i++)
    {
      pixel_brightness = (i - 300) - (400 + (5 * WaterfallBase)) / 4; // this in range -400 to +400, but only 0 to 400 is valid
      pixel_brightness =  4 * (255 * pixel_brightness) / (WaterfallRange * 5);  // scale to 0 - 255

      if (pixel_brightness < 0) 
      {
        pixel_brightness = 0;  // Black below gradient line
      }
      if (pixel_brightness > 256)
      {
        pixel_brightness = 255;
      }
      //setPixel(610, 410 - i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(610, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(611, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(612, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(613, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(614, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
    }
    HorizLine(610, 470 + (WaterfallBase * 5) / 4, 5, 255, 255, 255);  // White bar at bottom of gradient line
    HorizLine(610, 470 + (WaterfallBase * 5) / 4 + (WaterfallRange * 5) /4, 5, 0, 0, 0);  // Black bar at top of gradient line
    HorizLine(610, 471 + (WaterfallBase * 5) / 4 + (WaterfallRange * 5) /4, 5, 0, 0, 0);  // Black bar at top of gradient line
  }
}


void DrawSettings()
{
  const font_t *font_ptr = &font_dejavu_sans_18;   // 18pt
  char DisplayText[63];
  int ParamAsInt;
  int line1y = 45;
  int titley = 5;

  // Clear the previous text first
  rectangle(100, 0, 505, 69, 0, 0, 0);

  // Wait for the data to be ready
  while (dataready == false)
  {
    usleep(100);
  }

  if (scope == true)
  {
    if (ADCclkDiv_value < 959) // < 1ms/div
    {
      ParamAsInt = (50000 * ADCclkDiv_value)/48000;
      snprintf(DisplayText, 63, "%d us/div", ParamAsInt);
      Text(320, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);
    }
    else                        // > 1 ms/div
    {
      ParamAsInt = (50000 * ADCclkDiv_value)/48000000;
      snprintf(DisplayText, 63, "%d ms/div", ParamAsInt);
      Text(320, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);
    }
  }
  else
  {
    // Left hand x axis label
    Text(100, line1y, "0", font_ptr, 0, 0, 0, 255, 255, 255);

    // Centre x axis label

    ParamAsInt = 48000000/(2083 * ADCclkDiv_value * 2);
    snprintf(DisplayText, 63, "%d kHz", ParamAsInt);
    Text(320, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);

    // Right hand x axis label
    ParamAsInt = 48000000/(2083 * ADCclkDiv_value);
    snprintf(DisplayText, 63, "%d kHz", ParamAsInt);
    Text(520, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);
  }

  if (strcmp(PlotTitle, "-") != 0)
  {
    TextMid(350, titley, PlotTitle, &font_dejavu_sans_22, 0, 0, 0, 255, 255, 255);
  }
}


void DrawTrace(int xoffset, int prev2, int prev1, int current)
{
  int xpos;
  int previousStep;
  int thisStep;
  int column[401];
  int ypos;
  int ypospix;  // ypos corrected for pixel map  
  int ymax;     // inclusive upper limit of this plot
  int ymin;     // inclusive lower limit of this plot
  int trace_baseline;       // raised to top quarter for waterfall in mix mode

  if (waterfall == true)
  {
    trace_baseline = 301;
  }
  else
  {
    trace_baseline = 1;
  }

  for (ypos = 0; ypos < 401; ypos++)
  {
    column[ypos] = 0;
  }

  xpos = 99 + xoffset;  // we are going to draw the column before the current one

  previousStep = prev1 - prev2;  // positive is going up, ie below prev1
  thisStep = current - prev1; //    positive is going up, ie above prev1

  // Calculate contribution from previous2:

  if (previousStep == 0)
  {
    column[prev1] = 255;
    ymax = prev1;
    ymin = prev1;
  }
   else if (previousStep > 0)  // prev1 higher than prev2
  {
    for (ypos = prev2; ypos < prev1; ypos++)
    {
      column[ypos] = (255 * (ypos - prev2)) / previousStep;
    }
    ymax = prev1;
    ymin = prev2;
  }
  else // previousStep < 0  // prev2 lower than prev1
  {
    for (ypos = prev2; ypos > prev1; ypos--)
    {
      column[ypos] = (255 * (ypos - prev2 )) / previousStep;
    }
    ymax = prev2;
    ymin = prev1;
  }

  // Calculate contribution from current reading:

  if (thisStep == 0)
  {
    column[prev1] = 255;
  }
   else if (thisStep > 0)
  {
    for (ypos = prev1; ypos < current; ypos++)
    {
      column[ypos] = ((255 * (current - ypos)) / thisStep) + column[ypos];
    }
  }
  else // thisStep < 0
  {
    for (ypos = prev1; ypos > current; ypos--)
    {
      column[ypos] = ((255 * (current - ypos )) / thisStep) + column[ypos];
    }
  }

  if (current > ymax)  // Decide the highest extent of the trace
  {
    ymax = current;
  }
  if (current < ymin)  // Decide the lowest extent of the trace
  {
    ymin = current;
  }

  if (column[ypos] > 255)  // Limit the max brightness of the trace to 255
  {
    column[ypos] = 255;
  }

  // Draw the trace in the column

  for(ypos = ymin; ypos <= ymax; ypos++)
  {
    ypospix = ypos + 70;
    setPixel(xpos, ypospix, column[ypos], column[ypos], 0);  
  }

  // Draw the background and grid (except in the active trace) to erase the previous scan

  if (xpos % 50 == 0)  // vertical int graticule
  {
    for(ypos = trace_baseline; ypos < ymin; ypos++)
    {
      setPixel(xpos, ypos + 70, 64, 64, 64);
    }
    for(ypos = 399; ypos > ymax; ypos--)
    {
      setPixel(xpos, ypos + 70, 64, 64, 64);
    }
  }
  else if ((xpos > 344) && (xpos < 356))  // centre vertical graticule marks
  {
    for(ypos = trace_baseline; ypos < ymin; ypos++)
    {
      ypospix = ypos + 70;

      if (ypos % 10 == 0)  // tick mark
      {
        setPixel(xpos, ypospix, 64, 64, 64);
      }
      else
      {
        setPixel(xpos, ypospix, 0, 0, 0);
      }
    }
    for(ypos = 399; ypos > ymax; ypos--)
    {
      ypospix = ypos + 70;

      if (ypos % 10 == 0)  // tick mark
      {
        setPixel(xpos, ypospix, 64, 64, 64);
      }
      else
      {
        setPixel(xpos, ypospix, 0, 0, 0);
      }
    }
  }
  else  // horizontal graticule and open space
  {
    for(ypos = trace_baseline; ypos < ymin; ypos++)  // below the trace
    {
      ypospix = ypos + 70;

      if (ypos % 50 == 0)  // graticule line
      {
        setPixel(xpos, ypospix, 64, 64, 64);
      }
      else
      {
        setPixel(xpos, ypospix, 0, 0, 0);
      }
      if ((xpos % 10 == 0) && (ypos > 195) && (ypos < 205)) // tick mark on x axis
      {
        setPixel(xpos, ypospix, 64, 64, 64);
      }
    }
    for(ypos = 399; ypos > ymax; ypos--)  // above the trace
    {
      ypospix = ypos + 70;

      if (ypos % 50 == 0)  // graticule line
      {
        setPixel(xpos, ypospix, 64, 64, 64);
      }
      else
      {
        setPixel(xpos, ypospix, 0, 0, 0);
      }
      if ((xpos % 10 == 0) && (ypos > 195) && (ypos < 205)) // tick mark on x axis
      {
        setPixel(xpos, ypospix, 64, 64, 64);
      }
    }
  }

  // Draw the markers

  if (markeron == true)
  {
    MarkerGrn(markerx, xpos, markery);
  }
}


static void cleanexit(int calling_exit_code)
{
  exit_code = calling_exit_code;
  printf("Clean Exit Code %d\n", exit_code);
  app_exit = true;
  usleep(1000000);
  printf("Close screen\n");
  closeScreen();
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);

  printf("scans = %d\n", tracecount);
  exit(exit_code);
}


static void terminate(int sig)
{
  printf("Terminating\n");
  app_exit = true;
  usleep(1000000);
  printf("Close screen\n");
  closeScreen();
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);

  printf("scans = %d\n", tracecount);
  exit(0);
}


int main(void)
{
  wscreen = 800;
  hscreen = 480;
  int screenXmax, screenXmin;
  int screenYmax, screenYmin;
  int i;
  int pixel;
  int PeakValueZeroCounter = 0;

  uint64_t nextwebupdate = monotonic_ms() + 1000;
  uint64_t next_spec_paint = monotonic_ms();
  uint64_t next_wfall_paint = monotonic_ms();
  uint64_t check_time;
  uint16_t y4[625];
  bool paint_line;
  int w_index = 0;
  int j;
  int k;
  screen_pixel_t wfparray[513][401];
  int wfall_offset = 0;
  int wfall_height;
  int16_t pixel_brightness;
  uint16_t AverageCount = 0;
  char response[63];

  // Catch sigaction and call terminate
  for (i = 0; i < 16; i++)
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = terminate;
    sigaction(i, &sa, NULL);
  }

  // Sort out display and control.  If touchscreen detected, use that (and set config) with optional web control.
  // If no touchscreen, check for mouse.  If mouse detected, use that and set hdmi720 if hdmi mode not selected
  // Allow optional web control.  If no mouse, set hdmi720 and enable web control.

  // Check the display type in the config file
  strcpy(response, "not_set");
  GetConfigParam(PATH_SCONFIG, "display", response);
  strcpy(DisplayType, response);

  if ((strcmp(DisplayType, "Element14_7") == 0) || (strcmp(DisplayType, "touch2") == 0))
  {
    printf("Touchscreen detected after boot\n");
    touchscreen_present = true;

    // Open the touchscreen
    for (i = 0; i < 7; i++)
    {
      if (openTouchScreen(i) == 1)
      {
        if(getTouchScreenDetails(&screenXmin, &screenXmax, &screenYmin, &screenYmax) == 1) break;
      }
    }

    // Create Touchscreen thread
    pthread_create (&thtouchscreen, NULL, &WaitTouchscreenEvent, NULL);

    if (FindMouseEvent() >= 0)
    {
      mouse_connected = true;
      mouse_active = true;
      printf("Mouse Connected\n");

      // And start the mouse listener thread
      printf("Starting Mouse Thread\n");
      pthread_create (&thmouse, NULL, &WaitMouseEvent, NULL);
    }
  }
  else // No touchscreen detected, check for mouse
  {
    if (FindMouseEvent() >= 0)
    {
      mouse_connected = true;
      mouse_active = true;
      printf("Mouse Connected\n");

      // And start the mouse listener thread
      printf("Starting Mouse Thread\n");
      pthread_create (&thmouse, NULL, &WaitMouseEvent, NULL);
    }
    else // No touchscreen or mouse, so check webcontrol is enabled
    {
      printf("Mouse not Connected\n");

      strcpy(response, "0");  // highlight null responses
      GetConfigParam(PATH_SCONFIG, "webcontrol", response);
      if (strcmp(response, "enabled") != 0)
      {
        SetConfigParam(PATH_SCONFIG, "webcontrol", "enabled");
        webcontrol = true;

        // Start the web click listener
        pthread_create (&thwebclick, NULL, &WebClickListener, NULL);
        printf("Created webclick listener thread\n");
      }
    }
  }
  printf("Completed Screen and Mouse checks\n");

  screenXmax = 799;
  screenXmin = 0;
  wscreen = 800;
  screenYmax = 479;
  screenYmin = 0;
  hscreen = 480;

  // Calculate screen parameters
  scaleXvalue = ((float)(screenXmax - screenXmin)) / wscreen;
  printf ("X Scale Factor = %f\n", scaleXvalue);
  printf ("wscreen = %d\n", wscreen);
  printf ("screenXmax = %d\n", screenXmax);
  printf ("screenXmim = %d\n", screenXmin);
  scaleYvalue = ((float)(screenYmax - screenYmin)) / hscreen;
  printf ("Y Scale Factor = %f\n", scaleYvalue);
  printf ("hscreen = %d\n", hscreen);

  Define_Menu1();
  Define_Menu2();
  Define_Menu3();
  Define_Menu4();
  Define_Menu5();
  Define_Menu6();
  Define_Menu7();
  Define_Menu8();
  Define_Menu9();
  Define_Menu10();
  Define_Menu11();
  Define_Menu12();
  Define_Menu41();

  CheckConfigFile();
  ReadSavedParams();
  CalcSpan();

  // Create Wait Button thread
  pthread_create (&thbutton, NULL, &WaitButtonEvent, NULL);

  // Initialise screen and splash

  initScreen();

  MsgBox4("Starting the Band Viewer", "Profiling FFTs on first use", "Please wait 80 seconds", "No delay next time");

  printf("Profiling FFTs..\n");
  fftwf_import_wisdom_from_filename("/home/pi/.fftwf_wisdom");
  main_fft_init();
  fftwf_export_wisdom_to_filename("/home/pi/.fftwf_wisdom");
  printf("FFTs Done.\n");


  /* Setting up buffers */
  buffer_circular_init(&buffer_circular_iq_main, sizeof(buffer_iqsample_t), 4096*1024);

  // Check Pico Connected
  if (CheckPicoConnect() == 1)
  {
    MsgBox4("No Pico Detected", "Check connections", "", "Touch Screen to Continue");
    wait_touch();
    if (CheckPicoConnect() == 1)
    {
      MsgBox4("Still No Pico Detected", "Exiting from PicoViewer", "", "Touch Screen to Continue");
      wait_touch();
      clearScreen(0, 0, 0);
      cleanexit(129);
    }
  }

  // Serial Thread
  printf("Starting Serial Thread\n");
  if(pthread_create(&serial_thread_obj, NULL, serial_thread, &app_exit))
  {
      fprintf(stderr, "Error creating %s pthread\n", "Serial");
      return 1;
  }
  pthread_setname_np(serial_thread_obj, "Serial");

  // FFT Thread
  printf("Starting FFT Thread\n");
  if(pthread_create(&fft_thread_obj, NULL, fft_thread, &app_exit))
  {
      fprintf(stderr, "Error creating %s pthread\n", "FFT");
      return 1;
  }
  pthread_setname_np(fft_thread_obj, "FFT");

  for(i = 1; i < 511; i++)
  {
    y[i] = 1;
  }

  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  UpdateWindow();     // Draw the buttons

  while(app_exit == false)                                                  // Start of main display loop
  {
    // transfer the data into correct buffer (unnecesary step)
    for (pixel = 0; pixel <= 512; pixel++)
    {
      y3[pixel] = y[pixel];
    }

    activescan = true;

    check_time = monotonic_ms();

    if ((spectrum == true) && (check_time > next_spec_paint))             // Paint spectrum in full or mix modes  
    {
      // Set the time for the next spectrum paint after this one
      next_spec_paint = next_spec_paint + 20;

      // Make sure that we don't build up a time deficit
      if (next_spec_paint < check_time)
      {
        next_spec_paint = check_time + 20;
      }

      if (waterfall == true)  // mix
      {
        for (pixel = 0; pixel <= 512; pixel++)
        {
          if (y3[pixel] < 4)
          {
            y3[pixel] = 4;
          }
        }
      }

      for (pixel = 8; pixel <= 506; pixel++)
      {
        if (waterfall == false)   // full height mode
        {
          DrawTrace((pixel - 6), y3[pixel - 2], y3[pixel - 1], y3[pixel]);
        }
        else                     //  mix mode
        {
          DrawTrace((pixel - 6), (y3[pixel - 2] / 4) + 300, (y3[pixel - 1] / 4) + 300, (y3[pixel] / 4) + 300);
        }

        if ((PeakPlot == true) && (waterfall == false))  // only works without waterfall!
        {
          // draw [pixel - 1] here based on PeakValue[pixel -1]
          if (y3[pixel - 1] > PeakValue[pixel -1])
          {
            PeakValue[pixel - 1] = y3[pixel - 1];
          }
          setPixel(pixel + 93, 70 + PeakValue[pixel - 1], 255, 0, 63);
        }

        while (freeze)
        {
          frozen = true;
          usleep(1000); // Pause to let things happen if CPU is busy
        }
        frozen = false;
      }

      if (waterfall == true) // mix, so draw the dividing line
      {
        HorizLine(101, 370, 499, 63, 63, 63);
      }

      activescan = false;

      if (markeron == true)
      {
        CalculateMarkers();
      }

      if (RequestPeakValueZero == true)
      {
        PeakValueZeroCounter++;
        if (PeakValueZeroCounter > 19)
        {
          memset(PeakValue, 0, sizeof(PeakValue));
          PeakValueZeroCounter = 0;
          RequestPeakValueZero = false;
        }
      }
    }                                                // End of spectrum painting
    else
    {
      usleep(100);                                   // Give the processor a rest
    }

    if (waterfall == true)                           // Paint waterfall
    {
      if (spectrum == true)
      {
        wfall_height = 299;
      }
      else
      {
        wfall_height = 399;
      }

      check_time = monotonic_ms();
      if (check_time > next_wfall_paint)                   // Paint the waterfall if time is right
      {
        // Set the time for the next paint after this one
        if (spectrum == false)            // full screen waterfall
        {
          next_wfall_paint = next_wfall_paint + (wfalltimespan * 25) / 10;
        }
        else                              // only 300 (not 400) lines so paint slower
        {
          next_wfall_paint = next_wfall_paint + (wfalltimespan * 100) / 30;
        }

        // Make sure that we don't build up a time deficit
        if (next_wfall_paint < check_time)
        {
          if (spectrum == false)            // full screen waterfall
          {
            next_wfall_paint = check_time + (wfalltimespan * 25) / 10;
          }
          else                              // only 300 (not 400) lines so paint slower
          {
            next_wfall_paint = check_time + (wfalltimespan * 100) / 30;
          }
        }
        paint_line = true;                // remember that we need to paint on this pass

        if (strcmp(TraceType, "peak") == 0)
        {
          for (j = 8; j <= 506; j++)
          {
            if (y3[j] > y4[j])  // store the peaks
            {
              y4[j] = y3[j];
            }
          }
        }
        else if (strcmp(TraceType, "single") == 0)
        {
          for (j = 8; j <= 506; j++)
          {
            y4[j] = y3[j];
          }
        }
        else if (strcmp(TraceType, "average") == 0)
        {
          {
            for (j = 8; j <= 506; j++)
            {
              y4[j] = y4[j] + y3[j];
              y4[j] = y4[j] / (AverageCount + 1);
            }
            AverageCount = 0;
          }
        }
      }
      else         // If average or peak, add to the average or store the peak, but don't paint the waterfall line
      {
        if (strcmp(TraceType, "peak") == 0)
        {
          for (j = 8; j <= 506; j++)
          {
            if (y3[j] > y4[j])  // store the peaks
            {
              y4[j] = y3[j];
            }
          }
        }
        else if (strcmp(TraceType, "average") == 0)
        {
          for (j = 8; j <= 506; j++)
          {
            y4[j] = y4[j] + y3[j];
          }
          AverageCount++;
        }
        paint_line = false;
      } 

      if(paint_line)                                         // time is right, so paint waterfall
      {
        // Add the current line to the waterfall

        for (j = 8; j <= 506; j++)
        {
          pixel_brightness = y4[j] - (400 + (5 * WaterfallBase));              // this in range -400 to +400, but only 0 to 400 is valid
          y4[j] = 0;                                                           // Zero peak value in preparation for next line
          pixel_brightness = (255 * pixel_brightness) / (WaterfallRange * 5);  // scale pixel brightness to 0 - 255

          if (pixel_brightness < 0)                                            // and limit to 0 - 255
          {
            pixel_brightness = 0;
          }
          if (pixel_brightness > 255)
          {
            pixel_brightness = 255;
          }
          wfparray[j][w_index] = waterfall_map((uint8_t)pixel_brightness);     // Look up colour and store in array
        }

        // Render the waterfall
        for (k = 0; k < wfall_height; k++)
        {
          // start by displaying the line stored at array[0] at the top of the waterfall
          // so waterfall offset needs to be zero

          // in the next frame w_index will be 1, so we need to display that line at the top
          // so waterfall offset needs to be 0 when k is 1

          wfall_offset = w_index - k;
          if (wfall_offset >= wfall_height)
          {
            wfall_offset = wfall_offset - wfall_height;
          }
          if (wfall_offset < 0)
          {
            wfall_offset = wfall_offset + wfall_height;
          }
          for (j = 7; j <= 505; j++)
          {
            //setPixel(j + 94, (409 - wfall_height + wfall_offset), wfparray[j][k].Red, wfparray[j][k].Green, wfparray[j][k].Blue);
            setPixel(j + 94, (70 + wfall_height - wfall_offset), wfparray[j][k].Red, wfparray[j][k].Green, wfparray[j][k].Blue);

            while (freeze)
            {
              frozen = true;
              usleep(100000); // Pause to let things happen if CPU is busy
            }
            frozen = false;
          }
        }

        w_index++;                   // keep track of where we are in the circular buffer
        if (w_index >= wfall_height)
        {
          w_index = 0;
        }
        activescan = false;
      } 
      else
      {
        usleep(100);                                   // Give the processor a rest
      }

    }                                                  // End of spectrum paint section

    tracecount++;

    publish();                                         // 

    if (monotonic_ms() >= nextwebupdate)
    {
      UpdateWeb();
      nextwebupdate = nextwebupdate + 1000;  // Set next update for 1 second after the previous
      if (nextwebupdate < monotonic_ms())    // Check for backlog due to a freeze
      {
        nextwebupdate = monotonic_ms() + 1000;  //Set next update for 1 second ahead
      }
    }
  }                                                   // End of main while loop

  printf("Waiting for the Serial Thread to exit..\n");
  pthread_join(serial_thread_obj, NULL);
  printf("Waiting for FFT Thread to exit..\n");
  pthread_join(fft_thread_obj, NULL);
  printf("All threads caught, exiting..\n");
  pthread_join(thbutton, NULL);
}

