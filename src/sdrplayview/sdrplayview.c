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
#include <fcntl.h>

#include "../common/font/font.h"
#include "../common/touch.h"
#include "../common/graphics.h"
#include "../common/timing.h"
#include "../common/hardware.h"
#include "../common/buffer/buffer_circular.h"
#include "../common/ffunc.h"
#include "sdrplayfft.h"
#include "sdrplayview.h"
#include "../common/ffunc.h"

pthread_t thbutton;
pthread_t thwebclick;     //  Listens for mouse clicks from web interface
pthread_t thtouchscreen;  //  listens to the touchscreen   
pthread_t thmouse;        //  Listens to the mouse

int debug_level = 0;                   // 0 minimum, 1 medium, 2 max
int fd = 0;
int hscreen = 480;
int wscreen = 800;
float scaleXvalue, scaleYvalue; // Coeff ratio from Screen/TouchArea
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
char DisplayType[31];


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
typedef struct {
	int x,y,w,h;                          // Position and size of button
	status_t Status[MAX_STATUS];          // Array of text and required colour for each status
	int IndexStatus;                      // The number of valid status definitions.  0 = do not display
	int NoStatus;                         // This is the active status (colour and text)
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
#define PATH_CONFIG "/home/pi/portsdown/configs/sdrplayview_config.txt"

#define MAX_BUTTON 675
#define MAX_BUTTON_ON_MENU 50
#define MAX_MENUS 50

int IndexButtonInArray=0;
button_t ButtonArray[MAX_MENUS][MAX_BUTTON_ON_MENU];

int CurrentMenu = 1;
int CallingMenu = 1;
// char KeyboardReturn[64];

bool NewFreq = false;
bool NewGain = false;
bool NewSpan = false;
bool NewCal  = false;
bool NewPort = false;
float gain;
int8_t BaselineShift = 0;

bool NewData = false;

int scaledX, scaledY;

int wbuttonsize = 100;
int hbuttonsize = 50;
int rawX, rawY;
int i;
bool freeze = false;
bool frozen = false;

bool prepnewscanwidth = false;
bool readyfornewscanwidth = false;

bool activescan = false;
bool PeakPlot = false;
bool PortsdownExitRequested = false;

int scaledadresult[501];  // Sensed AD Result
int PeakValue[513] = {0};
bool RequestPeakValueZero = false;

uint64_t startfreq = 0;    // Hz
uint64_t stopfreq = 0;     // Hz
int64_t freqoffset = 0;   // 0 for none, positive for low side, negative for high side
int rbw = 0;
int reflevel = 99;
char PlotTitle[63] = "-";
bool ContScan = false;

int fft_size = 500;                  // Variable based on scan width
float fft_time_smooth;       // Set for scan width

uint32_t span = 10000;
int limegain = 15;
uint32_t pfreq1 = 146500000;
uint32_t pfreq2 = 437000000;
uint32_t pfreq3 = 748000000;
uint32_t pfreq4 = 1255000000;
uint32_t pfreq5 = 1296000000;

bool app_exit = false;

uint32_t CentreFreq;                     // in Hz
uint32_t SpanWidth;                      // in Hz

int RFgain;
int IFgain;
bool LocalGain;
bool agc = false;
uint8_t decimation_factor = 8;
float SampleRate;

extern uint16_t y3[1250];
extern int force_exit;
bool waterfall;
bool spectrum;

extern pthread_mutex_t histogram;

static pthread_t screen_thread_obj;

static pthread_t sdrplay_fft_thread_obj;

int markerx = 250;
int markery = 15;
bool markeron = false;
int markermode = 7;       // 2 peak, 3, null, 4 man, 7 off
int historycount = 0;
int markerxhistory[10];
int markeryhistory[10];
int manualmarkerx = 250;
bool MarkerRefresh = false;

bool Range20dB = false;
int BaseLine20dB = -80;

int WaterfallBase;
int WaterfallRange;
uint16_t wfalltimespan = 0;
uint8_t Antenna_port;
bool BiasT_volts = false;
char TraceType[10] = "single"; // single, average or peak

// Touch sensing variables
int TouchX;
int TouchY;
int TouchTrigger = 0;
bool touchneedsinitialisation = true;
bool VirtualTouch = false;             // used to simulate a screen touch if a monitored event finishes
int FinishedButton = 0;                // Used to indicate screentouch during TX or RX
int touch_response = 0;                // set to 1 on touch and used to reboot display if it locks up

// Mouse control variables
bool mouse_connected = false;          // Set true if mouse detected at startup
bool mouse_active = false;             // set true after first movement of mouse
bool MouseClickForAction = false;      // set true on left click of mouse
int mouse_x;                           // click x 0 - 799 from left
int mouse_y;                           // click y 0 - 479 from top
bool touchscreen_present = false;      // detected on startup; used to control mouse device address
bool reboot_required = false;          // used after hdmi display change

// Web Control globals
bool webcontrol = false;   // Enables webcontrol

int tracecount = 0;  // Used for speed testing
int exit_code;

int yscalenum = 18;       // Numerator for Y scaling fraction
int yscaleden = 30;       // Denominator for Y scaling fraction
int yshift    = 5;        // Vertical shift (pixels) for Y

int xscalenum = 25;       // Numerator for X scaling fraction
int xscaleden = 20;       // Denominator for X scaling fraction

int FBOrientation = 0;    // 0, 90, 180 or 270
bool TouchInverted = false;            // true if screen mounted upside down
int SnapNumber;           // Number for next snap


///////////////////////////////////////////// FUNCTION PROTOTYPES ///////////////////////////////

void GetConfigParam(char *PathConfigFile, char *Param, char *Value);
void SetConfigParam(char *PathConfigFile, char *Param, char *Value);
void CheckConfigFile();
void ReadSavedParams();
void *WaitTouchscreenEvent(void * arg);
void *WaitMouseEvent(void * arg);
void *WebClickListener(void * arg);
void parseClickQuerystring(char *query_string, int *x_ptr, int *y_ptr);
FFUNC touchscreenClick(ffunc_session_t * session);
void take_snap();
void snap_from_menu();
int checkSnapIndex();
void do_snapcheck();
int IsImageToBeChanged();
int CheckSDRPlay();
void Define_Menus();
void Keyboard(char *RequestText, char *InitText, int MaxLength, char *KeyboardReturn, bool UpperCase);
int openTouchScreen(int NoDevice);
int getTouchScreenDetails(int *screenXmin, int *screenXmax,int *screenYmin,int *screenYmax);
void TransformTouchMap(int x, int y);
void CreateButtons();
int IsMenuButtonPushed();
int AddButton(int x,int y,int w,int h);
int ButtonNumber(int MenuIndex, int Button);
int CreateButton(int MenuIndex, int ButtonPosition);
void AmendButtonStatus(int menu, int button_number, int ButtonStatusIndex, char *Text, color_t *Color);
void DrawButton(int menu, int button_number);
void redrawButton(int menu, int button_number);
int AddButtonStatus(int menu, int button_number, char *Text, color_t *Color);
void SetButtonStatus(int menu, int button_number, int Status);
int GetButtonStatus(int menu, int button_number);
int getTouchSampleThread(int *rawX, int *rawY);
int getTouchSample(int *rawX, int *rawY);
void wait_touch();
void CalculateMarkers();
void SetSpan(int button);
uint32_t ConstrainFreq(uint32_t);
void SetMode(int button);
void SetGain(int button);
void SetWfall(int button);
void SetFreqPreset(int button);
void ShiftFrequency(int button);
void CalcSpan();
void ChangeLabel(int button);
void RedrawDisplay();
void *WaitButtonEvent(void * arg);
void redrawMenu();
void Define_Menu1();
void Highlight_Menu1();
void Define_Menu2();
void Highlight_Menu2();
void Define_Menu3();
void Highlight_Menu3();
void Define_Menu4();
void Highlight_Menu4();
void Define_Menu5();
void Highlight_Menu5();
void Define_Menu6();
void Highlight_Menu6();
void Define_Menu7();
void Highlight_Menu7();
void Define_Menu8();
void Highlight_Menu8();
void Define_Menu9();
void Highlight_Menu9();
void Define_Menu10();
void Highlight_Menu10();
void Define_Menu11();
void Highlight_Menu11();
void Define_Menu12();
void Highlight_Menu12();
void Define_Menu13();
void Highlight_Menu13();
void Define_Menu14();
void Highlight_Menu14();
void Define_Menu41();
void DrawEmptyScreen();
void DrawTickMarks();
void DrawYaxisLabels();
void DrawSettings();
void DrawTrace(int xoffset, int prev2, int prev1, int current);
void cleanexit(int calling_exit_code);
static void terminate(int sig);

///////////////////////////////////////////// SCREEN AND TOUCH UTILITIES ////////////////////////


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
  char ParamWithEquals[255];
  char ErrorMessage1[255];

  if (debug_level == 2)
  {
    printf("Set Config called %s %s %s\n", PathConfigFile , ParamWithEquals, Value);
  }

  if (strlen(Value) == 0)  // Don't write empty values
  {
    snprintf(ErrorMessage1, 63, "Error: Parameter %s in file:", Param);
    MsgBox4(ErrorMessage1, PathConfigFile, "Would have no value. Try again.", "Touch Screen to Continue");
    wait_touch();
    return;
  }

  strcpy(BackupConfigName, PathConfigFile);
  strcat(BackupConfigName, ".bak");
  FILE *fp=fopen(PathConfigFile, "r");
  FILE *fw=fopen(BackupConfigName, "w+");
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");

  if(fp != 0)
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
    //fclose(fp);
    //fclose(fw);
  }
}


/***************************************************************************//**
 * @brief Checks the config file for parameters that have been added after
 * initial release and adds them if required
 *
 * @param void
 *
 * @return void
*******************************************************************************/

void CheckConfigFile()
{
  //char shell_command[255];
  //FILE *fp;
  //int r;

  //sprintf(shell_command, "grep -q 'tracetype=' %s", PATH_CONFIG);
  //fp = popen(shell_command, "r");
  //r = pclose(fp);
  //if (WEXITSTATUS(r) != 0)
  //{
  //  printf("Updating Config File\n");
  //  sprintf(shell_command, "echo tracetype=single >> %s", PATH_CONFIG);
  //  system(shell_command); 
  //} 
}


void ReadSavedParams()
{
  char response[63];

  strcpy(PlotTitle, "-");  // this is the "do not display" response
  GetConfigParam(PATH_CONFIG, "title", PlotTitle);

  strcpy(response, "50000000");
  GetConfigParam(PATH_CONFIG, "centrefreq", response);
  CentreFreq = atoi(response);
  CentreFreq = ConstrainFreq(CentreFreq);

  strcpy(response, "10000");
  GetConfigParam(PATH_CONFIG, "span", response);
  span = atoi(response);

  CalcSpan();

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

  strcpy(response, "25");
  GetConfigParam(PATH_CONFIG, "rfgain", response);
  RFgain = atoi(response);

  strcpy(response, "-30");
  GetConfigParam(PATH_CONFIG, "ifgain", response);
  IFgain = atoi(response);

  strcpy(response, "off");
  GetConfigParam(PATH_CONFIG, "agc", response);
  if (strcmp(response, "on") == 0)
  {
    agc = true;
  }
  else
  {
    agc = false;
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

  strcpy(response, "50000000");
  GetConfigParam(PATH_CONFIG, "pfreq1", response);
  pfreq1 = atoi(response);

  GetConfigParam(PATH_CONFIG, "pfreq2", response);
  pfreq2 = atoi(response);

  GetConfigParam(PATH_CONFIG, "pfreq3", response);
  pfreq3 = atoi(response);

  GetConfigParam(PATH_CONFIG, "pfreq4", response);
  pfreq4 = atoi(response);

  GetConfigParam(PATH_CONFIG, "pfreq5", response);
  pfreq5 = atoi(response);

  strcpy(response, "0");
  GetConfigParam(PATH_CONFIG, "port", response);
  Antenna_port = atoi(response);
  if (Antenna_port > 2)  // unsigned int, so can't be less than 0
  {
    Antenna_port = 0;
  }

  strcpy(response, "off");
  GetConfigParam(PATH_CONFIG, "biast", response);
  if (strcmp(response, "on") == 0)
  {
    BiasT_volts = true;
  }
  else
  {
    BiasT_volts = false;
  }

  strcpy(response, "single");
  GetConfigParam(PATH_CONFIG, "tracetype", response);
  strcpy(TraceType, response);

  printf("TraceType read in as %s\n", TraceType);

  strcpy(response, "0");
  GetConfigParam(PATH_CONFIG, "freqoffset", response);
  freqoffset = strtoll(response, NULL, 10);

  strcpy(response, "0");
  GetConfigParam(PATH_CONFIG, "baselineshift", response);
  BaselineShift = atoi(response);

  strcpy(response, "-80");
  GetConfigParam(PATH_CONFIG, "baseline20db", response);
  BaseLine20dB = atoi(response);

  // System Config File

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "fborientation", response);
  FBOrientation = atoi(response);

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "touch", response);
  if (strcmp(response, "normal") == 0)
  {
    TouchInverted = false;
  }
  else
  {
    TouchInverted = true;
  }

  strcpy(response, "disabled");   
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
}


void *WaitTouchscreenEvent(void * arg)
{
  int TouchTriggerTemp;
  int rawX;
  int rawY;
  while (true)
  {
    TouchTriggerTemp = getTouchSampleThread(&rawX, &rawY);
    TouchX = rawX;
    TouchY = rawY;
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

  if ((fd = open(mouse_event_name, O_RDONLY)) < 0)
  {
    perror("evdev open");
    exit(1);
  }

  struct input_event ev;

  while(app_exit == false)
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
        // printf("value %d, type %d, code %d, x_pos %d, y_pos %d\n",ev.value,ev.type,ev.code, x, y);
        moveCursor(x, y);
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
        // printf("value %d, type %d, code %d, x_pos %d, y_pos %d\n",ev.value,ev.type,ev.code, x, y);
//        while (image_complete == false)  // Wait for menu to be drawn
//        {
//          usleep(1000);
//        }
        moveCursor(x, y);
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
          //printf("value %d, type %d, code %d, right mouse click \n", ev.value, ev.type, ev.code);
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
        scaledX = mouse_x;
        scaledY = mouse_y;
        MouseClickForAction = true;
        //printf("scaledX = %d, scaledY = %d\n", scaledX, scaledY);
      }
      left_button_action = false;
    }
    else
    { 
      //printf("value %d, type %d, code %d\n", ev.value, ev.type, ev.code);
    }
  }
  close(fd);
  return NULL;
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
  char mvcmd[255];
  char echocmd[255];
  char SnapIndexText[255];

  SnapNumber = checkSnapIndex();  // this is the next snap number to be saved.  Needs incrementing before exit

  //printf("pre-fb2png\n");
  fb2png(); // saves snap to /home/pi/tmp/snapshot.png
  //printf("post-fb2png\n");

  usleep(1000);

  system("convert /home/pi/tmp/snapshot.png /home/pi/tmp/screen.jpg");

  sprintf(SnapIndexText, "%d", SnapNumber);
  strcpy(mvcmd, "cp /home/pi/tmp/screen.jpg /home/pi/snaps/snap");
  strcat(mvcmd, SnapIndexText);
  strcat(mvcmd, ".jpg >/dev/null 2>/dev/null");
  system(mvcmd);

  snprintf(echocmd, 50, "echo %d > /home/pi/snaps/snap_index.txt", SnapNumber + 1);
  system (echocmd);
  SnapNumber++;
}


void snap_from_menu()
{
  freeze = true; 
  SetButtonStatus(CurrentMenu, 0, 1);  // hide the capture button 
  redrawMenu();                                    // paint the hide

  while(! frozen)                                    // wait till the end of the scan
  {
    usleep(10);
  }

  take_snap();

  SetButtonStatus(CurrentMenu, 0, 0);  // unhide the button
  redrawMenu();                                    // paint the un-hide
  freeze = false;
}


int checkSnapIndex()
{
  char SnapIndex[255];
  FILE *fp;

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

  SnapNumber = atoi(SnapIndex);
  return SnapNumber;
}


void do_snapcheck()
{
  char SnapIndex[255];
  //int SnapNumber;
  int Snap;
  int LastDisplayedSnap = -1;
  int TCDisplay = -1;
  char labelcmd[255];

  clearScreen(0, 0, 0);

  Snap = checkSnapIndex() - 1;

  while (((TCDisplay == 1) || (TCDisplay == -1)) && (SnapNumber != 0))
  {


    if(LastDisplayedSnap != Snap)  // only redraw if not already there
    {
      sprintf(SnapIndex, "%d", Snap);

      system("rm /home/pi/tmp/labelledsnap.jpg >/dev/null 2>/dev/null");

      // Label the snap
      strcpy(labelcmd, "convert /home/pi/snaps/snap");
      strcat(labelcmd, SnapIndex);
      strcat(labelcmd, ".jpg -pointsize 20 -fill white -draw 'text 5,460 \"Snap ");
      strcat(labelcmd, SnapIndex);
      strcat(labelcmd, "\"' /home/pi/tmp/labelledsnap.jpg");
      system(labelcmd);

      // Display it
      img2fb("/home/pi/tmp/labelledsnap.jpg");

      LastDisplayedSnap = Snap;
      UpdateWeb();
    }


    TCDisplay = IsImageToBeChanged();  // check if touch was previous snap, next snap or exit

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
  clearScreen(0, 0, 0);
}


int IsImageToBeChanged()
{
  // Returns -1 for LHS touch, 0 for centre and 1 for RHS

  while((getTouchSample(&rawX, &rawY) == 0))
  {
    usleep(10000);
  }

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


/***************************************************************************//**
 * @brief Checks whether an SDRPlay SDR is connected
 *
 * @param 
 *
 * @return 0 if present, 1 if absent
*******************************************************************************/

int CheckSDRPlay()
{
  FILE *fp;
  char response[255];
  int responseint = 1;

  // Open the command for reading
  fp = popen("lsusb | grep -q '1df7:' ; echo $?", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  // Read the output a line at a time - output it
  while (fgets(response, 7, fp) != NULL)
  {
    responseint = atoi(response);
  }

  pclose(fp);
  return responseint;
}


void Define_Menus()
{
  Define_Menu1();
  Define_Menu2();
  Define_Menu3();
  Define_Menu4();
  Define_Menu5();
  Define_Menu6();
  Define_Menu7();
  Define_Menu8();
  Define_Menu9();
  Define_Menu11();
  Define_Menu12();
  Define_Menu13();
  Define_Menu14();

  Define_Menu41();
}


void Keyboard(char *RequestText, char *InitText, int MaxLength, char *KeyboardReturn, bool UpperCase)
{
  char EditText[63];
  strcpy (EditText, InitText);
  int token;
  int PreviousMenu;
  int i;
  int CursorPos;
  char thischar[2];
  int KeyboardShift;
  int ShiftStatus = 0;
  char KeyPressed[2];
  char PreCuttext[63];
  char PostCuttext[63];
  bool refreshed;
  int rawX;
  int rawY;
  
  // Store away currentMenu
  PreviousMenu = CurrentMenu;

  // Trim EditText to MaxLength
//  if (strlen(EditText) > MaxLength)
//  {
//    strncpy(EditText, &EditText[0], MaxLength);
//    EditText[MaxLength] = '\0';
//  }

  // Set cursor position to next character after EditText
  CursorPos = strlen(EditText);

  // On initial call set Menu to 41
  CurrentMenu = 41;

  refreshed = false;

  // Set up text
  clearScreen(0, 0, 0);
  const font_t       *font_ptr = &font_dejavu_sans_28;
  const font_t *small_font_ptr = &font_dejavu_sans_20;

  // Look up pitch for W (87), the widest caharcter
  int charPitch = font_ptr->characters[87].render_width;

  // Check calling case
  if (UpperCase == true)
  {
    KeyboardShift = 1;
  }
  else
  {
    KeyboardShift = 0;
  }

  for (;;)
  {
    if (!refreshed)
    {
      // Sort Shift changes
      ShiftStatus = 2 - (2 * KeyboardShift); // 0 = Upper, 2 = lower
      for (i = 0; i <= 49; i = i + 1)
      {
        if (ButtonArray[CurrentMenu][i].IndexStatus > 0)  // If button needs to be drawn
        {
          SetButtonStatus(41, i, ShiftStatus);
        }
      }  

      // Display the keys here as they would overwrite the text later
      redrawMenu();

      // Display Instruction Text

      if (font_width_string(font_ptr, RequestText) < 780)  // OK at normal font size
      {
        Text(10, 420 , RequestText, font_ptr, 0, 0, 0, 255, 255, 255);
      }
      else                                                 // Reduce font size
      {
        Text(10, 420 , RequestText, small_font_ptr, 0, 0, 0, 255, 255, 255);
      }

      // Blank out the text line to erase the previous text and cursor
      rectangle(10, 310, 780, 90, 0, 0, 0);

      // Display Text for Editing

      if (strlen(EditText) < 29)                // Text to edit fits on one line
      {
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
            TextMid(i * charPitch, 330, thischar, font_ptr, 0, 0, 0, 255, 255, 255);   // White text
          }
        }

        // Draw the cursor
        rectangle(12 + charPitch * CursorPos, 320, 2, 40, 255, 255, 255);
        publishRectangle(0, 302, 800, 178);
        refreshed = true;
      }
      else                                   // Two editing lines 
      {
        for (i = 1; i <= 28; i = i + 1)
        {
          strncpy(thischar, &EditText[i-1], 1);
          thischar[1] = '\0';
          if (i > MaxLength)
          {
            TextMid(i * charPitch, 360, thischar, font_ptr, 0, 0, 0, 255, 63, 63);    // Red text
          }
          else
          {
            TextMid(i * charPitch, 360, thischar, font_ptr, 0, 0, 0, 255, 255, 255);   // White text
          }
        }

        for (i = 29; i <= strlen(EditText); i = i + 1)
        {
          strncpy(thischar, &EditText[i-1], 1);
          thischar[1] = '\0';
          if (i > MaxLength)
          {
            TextMid((i - 28) * charPitch, 320, thischar, font_ptr, 0, 0, 0, 255, 63, 63);    // Red text
          }
          else
          {
            TextMid((i - 28) * charPitch, 320, thischar, font_ptr, 0, 0, 0, 255, 255, 255);   // White text
          }
        }

        // Draw the cursor
        if (CursorPos < 28)
        {
          rectangle(12 + charPitch * CursorPos, 350, 2, 40, 255, 255, 255);
        }
        else
        {
          rectangle(12 + charPitch * (CursorPos - 28), 310, 2, 40, 255, 255, 255);
        }

        publishRectangle(0, 302, 800, 178);
        refreshed = true;
      }

    }

    // Wait for key press
    if (getTouchSample(&rawX, &rawY) == 0) continue;
    refreshed = false;

    // Check if touch was in edit text area to move cursor
    if ((scaledY > 290) && (scaledY < 420))
    {
      if (strlen(EditText) < 29)                // Text to edit fits on one line
      {
        // Erase the old cursor
        rectangle(12 + charPitch * CursorPos, 320, 2, 40, 0, 0, 0);

        // Calculate the new cursor position
        CursorPos = (scaledX - 0) / charPitch;
        if (CursorPos > strlen(EditText))
        {
          CursorPos = strlen(EditText);
        }

        // Write the new cursor
        rectangle(12 + charPitch * CursorPos, 320, 2, 40, 255, 255, 255);
      }
      else                                     // Text to edit is in 2 lines
      {
        // Erase the old cursor
        if (CursorPos < 28)
        {
          rectangle(12 + charPitch * CursorPos, 350, 2, 40, 0, 0, 0);
        }
        else
        {
          rectangle(12 + charPitch * (CursorPos - 28), 310, 2, 40, 0, 0, 0);
        }

        // Draw the new cursor
        if (scaledY > 355)                    // Top line
        {
          // Calculate the new cursor position
          CursorPos = (scaledX - 0) / charPitch;
          if (CursorPos > 27)
          {
            CursorPos = 27;
          }

          // Write the new cursor
          rectangle(12 + charPitch * CursorPos, 350, 2, 40, 255, 255, 255);
        }
        else                                  // 2nd line
        {
          // Calculate the new cursor position
          CursorPos = (scaledX - 0) / charPitch;
          if (CursorPos > strlen(EditText) - 28)
          {
            CursorPos = strlen(EditText) - 28;
          }

          // Write the new cursor
          rectangle(12 + charPitch * CursorPos, 310, 2, 40, 255, 255, 255);

          CursorPos = CursorPos + 28;
        }
      }
      publishRectangle(0, 310, 800, 80);
      refreshed = true;
    }

    token = IsMenuButtonPushed();
    printf("Keyboard Token %d\n", token);

    // Highlight special keys when touched
    if ((token == 1) || (token == 5) || (token == 8) || (token == 9) || (token == 2) || (token == 3)) // Del, Clear, Enter, backspace, L and R arrows
    {
      SetButtonStatus(41, token, 1);
      redrawButton(41, token);
      usleep(300000);
      SetButtonStatus(41, token, 0);
      redrawButton(41, token);
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
      else if (token == 1)   // del
      {
        if (CursorPos < strlen(EditText))
        {
          // Copy the text to the left of the deleted character
          strncpy(PreCuttext, &EditText[0], CursorPos);
          PreCuttext[CursorPos] = '\0';


          // Copy the text to the right of the deleted character
          strncpy(PostCuttext, &EditText[CursorPos + 1], strlen(EditText));
          PostCuttext[strlen(EditText) - CursorPos + 1] = '\0';

          // Now combine them
          strcat(PreCuttext, PostCuttext);
          strcpy (EditText, PreCuttext);
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
        SetButtonStatus(41, token, ShiftStatus);
        DrawButton(41, token);
        redrawButton(41, token);
        usleep(300000);
        ShiftStatus = 2 - (2 * KeyboardShift); // 0 = Upper, 2 = lower
        SetButtonStatus(41, token, ShiftStatus);
        DrawButton(41, token);
        redrawButton(41, token);

        if (strlen(EditText) < 55) // Don't let it overflow beyond 2 lines
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
        CursorPos = CursorPos + 1;
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

  if (strcmp(DisplayType, "Browser") != 0)      // Touchscreen
  {
    scaledX = x / scaleXvalue;
    scaledY = hscreen - y / scaleYvalue;
  }
  else                                         // Browser control without touchscreen
  {
    scaledX = x;
    scaledY = 480 - y;
  }
}


// This function sets the button position and size for each button, and intialises the buttons as hidden
// Designed for 50 30-button menus

void CreateButtons()
{
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  int normal_width = 120;
  int normal_xpos = 640;

  int menu;
  int button;

  for (menu = 0; menu < 41; menu++)            // All except keyboard
  {
    for (button = 0; button < MAX_BUTTON_ON_MENU; button++)
    {
      if ((menu != 1) && (menu != 2) && (menu != 6))   // All except Main, Span, Markers
      {
        if (button == 0)  // Capture
        {
          x = 0;
          y = 0;
          w = 90;
          h = 55;
        }

        if ((button > 0) && (button < 9))  // 8 right hand buttons
        {
          x = normal_xpos;
          y = 480 - (button * 60);
          w = normal_width;
          h = 50;
        }
      }
      else if ((menu == 1) || (menu == 2))   // Main and Marker Menu
      {
        if (button == 0)  // Capture
        {
          x = 0;
          y = 0;
          w = 90;
          h = 50;
        }

        if ((button > 0) && (button < 5))  // 6 right hand buttons
        {
          x = normal_xpos;
          y = 480 - (button * 60);
          w = normal_width;
          h = 50;
        }
        if (button == 5) // Left hand arrow
        {
          x = normal_xpos;  
          y = 480 - (5 * 60);
          w = 50;
          h = 50;
        }
        if (button == 6) // Right hand arrow
        {
          x = 710;  // = normal_xpos + 50 button width + 20 gap
          y = 480 - (5 * 60);
          w = 50;
          h = 50;
        }
        if ((button > 6) && (button < 10))  // Bottom 3 buttons
        {
          x = normal_xpos;
          y = 480 - ((button - 1) * 60);
          w = normal_width;
          h = 50;
        }
      }
      else if (menu == 6)   // Scan Width Menu
      {
        if (button == 0)  // Capture
        {
          x = 0;
          y = 0;
          w = 90;
          h = 50;
        }

        if ((button >= 1) && (button <= 4))  // Title, 100 kHz, 200 kHz, 500 kHz
        {
          x = normal_xpos;
          y = 480 - (button * 60);
          w = normal_width;
          h = 50;
        }
        if (button == 5) // 1 MHz
        {
          x = normal_xpos;  
          y = 480 - (5 * 60);
          w = 50;
          h = 50;
        }
        if (button == 6) // 2 MHz
        {
          x = 710;  // = normal_xpos + 50 button width + 20 gap
          y = 480 - (5 * 60);
          w = 50;
          h = 50;
        }
        if (button == 7) // 5 MHz
        {
          x = normal_xpos;  
          y = 480 - (6 * 60);
          w = 50;
          h = 50;
        }
        if (button == 8) // 10 MHz
        {
          x = 710;  // = normal_xpos + 50 button width + 20 gap
          y = 480 - (6 * 60);
          w = 50;
          h = 50;
        }
        if ((button == 9) || (button == 10))  // Bottom 2 buttons = Back and Freeze
        {
          x = normal_xpos;
          y = 480 - ((button - 2) * 60);
          w = normal_width;
          h = 50;
        }
      }

      // Write the button position
      ButtonArray[menu][button].x = x;
      ButtonArray[menu][button].y = y;
      ButtonArray[menu][button].w = w;
      ButtonArray[menu][button].h = h;
      ButtonArray[menu][button].IndexStatus = 0;
    }
  }

  // Define Keyboard buttons
  menu = 41; 
  for (button = 0; button < MAX_BUTTON_ON_MENU; button++)
  {
    w = 65;
    h = 59;

    if (button <= 9)  // Space bar and < > - Enter, Bkspc
    {
      switch (button)
      {
      case 0:                   // Space Bar
          y = 0;
          x = 165; // wscreen * 5 / 24;
          w = 296; // wscreen * 9 /24;
          break;
      case 1:                  // del
          y = 0;
          x = 726; //wscreen * 11 / 12;
          break;
      case 2:                  // <
          y = 0;
          x = 528; //wscreen * 8 / 12;
          break;
      case 3:                  // >
          y = 0;
          x = 594; // wscreen * 9 / 12;
          break;
      case 4:                  // -
          y = 0;
          x = 660; // wscreen * 10 / 12;
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
    if ((button >= 10) && (button <= 19))  // ZXCVBNM,./
    {
      y = hscreen/8;
      x = (button - 9) * 66;
    }
    if ((button >= 20) && (button <= 29))  // ASDFGHJKL
    {
      y = 2 * hscreen / 8;
      x = ((button - 19) * 66) - 33;
    }
    if ((button >= 30) && (button <= 39))  // QWERTYUIOP
    {
      y = 3 * hscreen / 8;
      x = (button - 30) * 66;
    }
    if ((button >= 40) && (button <= 49))  // 1234567890
    {
      y = 4 * hscreen / 8;
      x = ((button - 39) * 66) - 33;
    }

    ButtonArray[menu][button].x = x;
    ButtonArray[menu][button].y = y;
    ButtonArray[menu][button].w = w;
    ButtonArray[menu][button].h = h;
    ButtonArray[menu][button].IndexStatus = 0;
  }
}


/***************************************************************************//**
 * @brief Takes the global scaledX and scaled Y and checks against each 
 *        defined button on the current menu
 *
 * @param nil
 *
 * @return int the button number, 0 - 29 or -1 for not pushed
*******************************************************************************/


int IsMenuButtonPushed()
{
  int  i;
  int pushedButton = -1;
  int margin=10;  // was 20

  //printf("x=%d y=%d scaledx %d scaledy %d sxv %f syv %f Button %d\n",x,y,scaledX,scaledY,scaleXvalue,scaleYvalue, NbButton);

  // For each button in the current Menu, check if it has been pushed.
  // If it has been pushed, return the button number.  If nothing valid has been pushed return -1
  // If it has been pushed, do something with the last event time

  for (i = 0; i < MAX_BUTTON_ON_MENU; i++)
  {
    if (ButtonArray[CurrentMenu][i].IndexStatus > 0)  // If button has been defined
    {
      //printf("Button %d, ButtonX = %d, ButtonY = %d\n", i, ButtonArray[i + cmo].x, ButtonArray[i + cmo].y);

      if  ((scaledX <= (ButtonArray[CurrentMenu][i].x + ButtonArray[CurrentMenu][i].w - margin))
        && (scaledX >= ButtonArray[CurrentMenu][i].x + margin)
        && (scaledY <= (ButtonArray[CurrentMenu][i].y + ButtonArray[CurrentMenu][i].h - margin))
        && (scaledY >= ButtonArray[CurrentMenu][i].y + margin))  // and touched
      {
        pushedButton = i;          // Set the button number to return
        break;                 // Break out of loop as button has been found
      }
    }
  }
  // printf("scaledX = %d, scaledY = %d, Menu = %d, PushedButton = %d\n", scaledX, scaledY, CurrentMenu, pushedButton);
  return pushedButton;
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


int AddButtonStatus(int menu, int button_number, char *Text, color_t *Color)
{
  button_t *Button = &(ButtonArray[menu][button_number]);

  strcpy(Button->Status[Button->IndexStatus].Text, Text);
  Button->Status[Button->IndexStatus].Color= *Color;
  return Button->IndexStatus++;
}


void AmendButtonStatus(int menu, int button_number, int ButtonStatusIndex, char *Text, color_t *Color)
{
  button_t *Button=&(ButtonArray[menu][button_number]);
  strcpy(Button->Status[ButtonStatusIndex].Text, Text);
  Button->Status[ButtonStatusIndex].Color=*Color;
}


void DrawButton(int menu, int button_number)
{
  button_t *Button=&(ButtonArray[menu][button_number]);
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
  
  if (label[0] == '\0')  // Deal with empty string
  {
    strcpy(label, " ");
  }

  // Separate button text into 2 lines if required  
  char find = '^';                                  // Line separator is ^
  const char *ptr = strchr(label, find);            // pointer to ^ in string

  if((ptr) && (CurrentMenu != 41))                  // if ^ found then
  {                                                 // 2 lines
    int index = ptr - label;                        // Position of ^ in string
    snprintf(line1, index+1, label);                // get text before ^
    snprintf(line2, strlen(label) - index, label + index + 1);  // and after ^

    // Display the text on the button
    TextMid(Button->x+Button->w/2, Button->y+Button->h*11/16 - 1, line1, &font_dejavu_sans_20,
      Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g,
      Button->Status[Button->NoStatus].Color.b, 255, 255, 255);	
    TextMid(Button->x+Button->w/2, Button->y+Button->h*3/16 - 1, line2, &font_dejavu_sans_20,      
      Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g,
      Button->Status[Button->NoStatus].Color.b, 255, 255, 255);	
    }
  else                                              // One line only
  {
    if (CurrentMenu <= 15)
    {
      TextMid(Button->x+Button->w/2, Button->y+Button->h/2, label, &font_dejavu_sans_20,
      Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g,
      Button->Status[Button->NoStatus].Color.b, 255, 255, 255);
    }
    else  // Keyboard
    {
      TextMid(Button->x+Button->w/2, Button->y+Button->h/2 - hscreen / 64, label, &font_dejavu_sans_28,
      Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g,
      Button->Status[Button->NoStatus].Color.b, 255, 255, 255);
    }
  }
}


void redrawButton(int menu, int button_number)
{
  button_t *Button=&(ButtonArray[menu][button_number]);
  DrawButton(menu, button_number);
  publishRectangle(Button->x, Button->y + 1, Button->w, Button->h);
}


void SetButtonStatus(int menu, int button_number, int Status)
{
  button_t *Button = &(ButtonArray[menu][button_number]);
  Button->NoStatus = Status;
}


int GetButtonStatus(int menu, int button_number)
{
  button_t *Button = &(ButtonArray[menu][button_number]);
  return Button->NoStatus;
}


int getTouchSampleThread(int *rawX, int *rawY)
{
  int i;

  /* how many bytes were read */
  size_t rb;

  /* the events (up to 64 at once) */
  struct input_event ev[64];

  if (strcmp(DisplayType, "Element14_7") == 0)
  {
    // Program flow blocks here until there is a touch event
    rb = read(fd, ev, sizeof(struct input_event) * 64);

    *rawX = -1;
    *rawY = -1;
    int StartTouch = 0;

    for (i = 0;  i <  (rb / sizeof(struct input_event)); i++)
    {
      if (ev[i].type ==  EV_SYN)
      {
        //printf("Event type is %s%s%s = Start of New Event\n", KYEL, events[ev[i].type], KWHT);
      }

      else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
      {
        StartTouch = 1;
        //printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s1%s = Touch Starting\n",
        //        KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
      }

      else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
      {
        StartTouch=0;
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
        //*rawPressure = ev[i].value;
      }

      if((*rawX != -1) && (*rawY != -1) && (StartTouch == 1))  // 1a
      {
        if (TouchInverted == false)
        {
          scaledX = *rawX;
          scaledY = 479 - *rawY;
        }
        else
        {
          scaledX = 799 - *rawX;
          scaledY = *rawY; // ////////////////////////////// This needs testing!!
        }
        //printf("7 inch Touchscreen Touch Event: rawX = %d, rawY = %d scaledX = %d, scaledY = %d\n", *rawX, *rawY, scaledX, scaledY);
        return 1;
      }
    }
  }
  return 0;
}


int getTouchSample(int *rawX, int *rawY)
{
  while (true)
  {
    if (TouchTrigger == 1)
    {
      TouchTrigger = 0;
      //printf("Event (Touch) scaledX = %d, scaledY = %d\n", scaledX, scaledY);
      return 1;
    }
    else if ((webcontrol == true) && (strcmp(WebClickForAction, "yes") == 0))
    {
      *rawX = web_x;
      *rawY = 480 - web_y;
      scaledX = web_x;
      scaledY = 480 - web_y;
      strcpy(WebClickForAction, "no");
      printf("Web rawX = %d, rawY = %d\n", *rawX, *rawY);
      return 1;
    }
    else if (MouseClickForAction == true)
    {
      *rawX = mouse_x;
      *rawY = mouse_y;
      MouseClickForAction = false;
      printf("Mouse rawX = %d, rawY = %d\n", *rawX, *rawY);
      return 1;
    }
    else if (VirtualTouch == true)
    {
      *rawX = web_x;
      *rawY = web_y;
      VirtualTouch = false;
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


void wait_touch()
// Wait for Screen touch, ignore position, but then move on
// Used to let user acknowledge displayed text
{
  int rawX, rawY;
  printf("wait_touch called\n");

  // Check if screen touched, if not, wait 1 ms and check again
  while(getTouchSample(&rawX, &rawY)==0)
  {
    usleep(1000);
  }
  // Screen has been touched
  printf("wait_touch exit\n");
}

void CalculateMarkers()
{
  int maxy = 0;
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
      for (i = 50; i < 450; i++)
      {
         if((y3[i + 6] + 1) > maxy)
         {
           maxy = y3[i + 6] + 1;
           xformaxy = i;
         }
      }
    break;

    case 3:  // null
      maxy = 400;
      for (i = 50; i < 450; i++)
      {
         if((y3[i + 6] + 1) < maxy)
         {
           maxy = y3[i + 6] + 1;
           xformaxy = i;
         }
      }
    break;

    case 4:  // manual
      maxy = 0;
      //maxy = y3[manualmarkerx + 6] + 1;


      for (i = (manualmarkerx - 5); i < (manualmarkerx + 6); i++)
      {
         if((y3[i + 6] + 1) > maxy)
         {
           maxy = y3[i + 6] + 1;
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
    markerlev = ((float)(410 - markery) / 5.0) - 80.0 - BaselineShift;  // in dB for a 0 to -80 screen
  }
  else
  {
    markerlev = ((float)(410 - markery) / 20.0) + (float)(BaseLine20dB - BaselineShift);  // in dB for a BaseLine20dB to 20 above screen
  }
  rectangle(620, 420, 160, 60, 0, 0, 0);  // Blank the Menu title
  snprintf(markerlevel, 14, "Mkr %0.1f dB", markerlev);
  Text(640, 450, markerlevel, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);

  markerf = (float)((((markerx - 100) * (stopfreq - startfreq)) / 500 + startfreq)) / 1000000.0; // value for display
  snprintf(markerfreq, 14, "%0.3f MHz", markerf);
  Text(640, 425, markerfreq, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);

  MarkerRefresh = false;  // Unlock screen writes
}


void SetSpan(int button)
{  
  char ValueToSave[63];

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  prepnewscanwidth = true;
  while(readyfornewscanwidth == false)
  {
    usleep(10);                                   // wait till fft has stopped
  }

  switch (button)
  {
    case 2:                                          // 100 kHz
      span = 100000;
      break;
    case 3:                                          // 200 kHz
      span = 200000;
      break;
    case 4:                                          // 500 kHz
      span = 500000;
      break;
    case 5:                                         // 1000 kHz
      span = 1000000;
      break;
    case 6:                                         // 2000 kHz
      span = 2000000;
      break;
    case 7:                                         // 5000 kHz
      span = 5000000;
      break;
    case 8:                                        // 10000 kHz
      span = 10000000;
      break;
  }

  // Store the new span
  snprintf(ValueToSave, 63, "%d", span);
  SetConfigParam(PATH_CONFIG, "span", ValueToSave);
  printf("Span set to %d Hz\n", span);

  redrawMenu();

  // Trigger the span change
  CalcSpan();
  DrawSettings();       // New labels

  NewSpan = true;

  freeze = false;
}


uint32_t ConstrainFreq(uint32_t CheckFreq)  // Check Freq is within bounds
{
  if (CheckFreq < 1000)
  {
    return 1000;
  }
  if (CheckFreq > 2000000000)
  {
    return 2000000000;
  }
  return CheckFreq;
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


void SetGain(int button)
{  
  char ValueToSave[63];
  int Setgain = 0;

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  if (button < 130)     // RF Gain
  {
    switch (button)
    {
      case 122:
        Setgain = 0;
      break;
      case 123:
        Setgain = 2;
      break;
      case 124:
        Setgain = 4;
      break;
      case 125:
        Setgain = 6;
      break;
      case 126:
        Setgain = 7;
      break;
    }

    RFgain = Setgain;
    snprintf(ValueToSave, 63, "%d", RFgain);
    SetConfigParam(PATH_CONFIG, "rfgain", ValueToSave);
    printf("RFgain set to %d \n", RFgain);
  }
  else   // IF Gain
  {
    switch (button)
    {
      case 132:
        Setgain = 20;
      break;
      case 133:
        Setgain = 30;
      break;
      case 134:
        Setgain = 40;
      break;
      case 135:
        Setgain = 50;
      break;
      case 136:
        Setgain = 59;
      break;
    }

    IFgain = Setgain;
    snprintf(ValueToSave, 63, "%d", IFgain);
    SetConfigParam(PATH_CONFIG, "ifgain", ValueToSave);
    printf("IFgain set to %d \n", IFgain);
  }

  // Trigger the gain change
  NewGain = true;
  freeze = false;
}

void SetWfall(int button)
{
  char ValueToSave[63];
  char RequestText[63];
  char InitText[63];
  char KeyboardReturn[63];

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
        Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
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
        Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
      }
      while ((strlen(KeyboardReturn) == 0) || (atoi(KeyboardReturn) < 1) || (atoi(KeyboardReturn) > 80));

      WaterfallRange = atoi(KeyboardReturn);
      snprintf(ValueToSave, 63, "%d", WaterfallRange);
      SetConfigParam(PATH_CONFIG, "wfallrange", ValueToSave);
      printf("Waterfall Range set to %d dB\n", WaterfallRange);
    break;
    case 12:                                            // Set Waterfall Base
      // Define request string
      strcpy(RequestText, "Enter new waterfall span in seconds (0 for min)");

      // Define initial value
      snprintf(InitText, 25, "%d", wfalltimespan);

      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
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

void SetPort(int button)
{
  char ValueToSave[63];
  char RequestText[63];
  char InitText[63];
  char KeyboardReturn[63];

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 2:                                            // Set Input Port
      // Define request string
      strcpy(RequestText, "Enter Antenna port A, B or C. Use C for Hi-Z");

      // Define initial value
      strcpy(InitText, "A");
      switch (Antenna_port)
      {
        case 0:
          strcpy(InitText, "A");
          break;
        case 1:
          strcpy(InitText, "B");
          break;
        case 2:
          strcpy(InitText, "C");
          break;
      }

      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
      }
      while ((strcmp(KeyboardReturn, "A") != 0) && (strcmp(KeyboardReturn, "B") != 0) && (strcmp(KeyboardReturn, "C") != 0));

      if (strcmp(KeyboardReturn, "B") == 0)
      {
        Antenna_port = 1;
      }
      else if (strcmp(KeyboardReturn, "C") == 0)
      {
        Antenna_port = 2;
      }
      else
      {
        Antenna_port = 0;
      }

      snprintf(ValueToSave, 63, "%d", Antenna_port);
      SetConfigParam(PATH_CONFIG, "port", ValueToSave);
      printf("Antenna port set to %d\n", Antenna_port);
      break;
    case 3:                                            // Set BiasT on/off
      if (BiasT_volts)
      {
        BiasT_volts = false;
        SetConfigParam(PATH_CONFIG, "biast", "off");
        printf("BiasT volts set to off\n");
      }
      else
      {
        BiasT_volts = true;
        SetConfigParam(PATH_CONFIG, "biast", "on");
        printf("BiasT volts set to on\n");
      }

      break;
  }
  NewPort = true;
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawTickMarks();    // tick marks on X axis
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  freeze = false;
}


void SetFreqPreset(int button)
{  
  char ValueToSave[63];
  char RequestText[64];
  char InitText[63];
  char KeyboardReturn[63];

  uint32_t amendfreq = 0;

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
        CentreFreq = pfreq1;
      break;
      case 3:
        CentreFreq = pfreq2;
      break;
      case 4:
        CentreFreq = pfreq3;
      break;
      case 5:
        CentreFreq = pfreq4;
      break;
      case 6:
        CentreFreq = pfreq5;
      break;
    }
    CentreFreq = ConstrainFreq(CentreFreq);

    // Store the new frequency
    snprintf(ValueToSave, 63, "%d", CentreFreq);
    SetConfigParam(PATH_CONFIG, "centrefreq", ValueToSave);
    printf("Centre Freq set to %d Hz\n", CentreFreq);

    // Calculate the new settings
    CalcSpan();

    // Trigger the frequency change
    NewFreq = true;

    while(NewFreq)
    {
      usleep(10);                                   // wait till the frequency has been set
    }

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
    strcpy(RequestText, "Enter new preset frequency in Hz");

    // Define initial value in Hz
    snprintf(InitText, 25, "%d", amendfreq);

    // Ask for the new value
    do
    {
      Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
    }
    while ((strlen(KeyboardReturn) == 0) || (atoi(KeyboardReturn) < 1000) || (atoi(KeyboardReturn) > 2000000000)) ;

    amendfreq = atoi(KeyboardReturn);
    snprintf(ValueToSave, 63, "%d", amendfreq);

    switch (button)
    {
      case 2:
        SetConfigParam(PATH_CONFIG, "pfreq1", ValueToSave);
        printf("Preset Freq 1 set to %d Hz\n", amendfreq);
        pfreq1 = amendfreq;
      break;
      case 3:
        SetConfigParam(PATH_CONFIG, "pfreq2", ValueToSave);
        printf("Preset Freq 2 set to %d Hz\n", amendfreq);
        pfreq2 = amendfreq;
      break;
      case 4:
        SetConfigParam(PATH_CONFIG, "pfreq3", ValueToSave);
        printf("Preset Freq 3 set to %d Hz\n", amendfreq);
        pfreq3 = amendfreq;
      break;
      case 5:
         SetConfigParam(PATH_CONFIG, "pfreq4", ValueToSave);
        printf("Preset Freq 4 set to %d Hz\n", amendfreq);
        pfreq4 = amendfreq;
      break;
      case 6:
        SetConfigParam(PATH_CONFIG, "pfreq5", ValueToSave);
        printf("Preset Freq 5 set to %d Hz\n", amendfreq);
        pfreq5 = amendfreq;
      break;
    }
    CentreFreq = amendfreq;
    CentreFreq = ConstrainFreq(CentreFreq);

    // Store the new frequency
    snprintf(ValueToSave, 63, "%d", CentreFreq);
    SetConfigParam(PATH_CONFIG, "centrefreq", ValueToSave);
    printf("Centre Freq set to %d Hz\n", CentreFreq);

    // Trigger the frequency change
    NewFreq = true;

    // Tidy up, paint around the screen and then unfreeze
    CalcSpan();         // work out start and stop freqs
    clearScreen(0, 0, 0);
    DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
    DrawYaxisLabels();  // dB calibration on LHS
    DrawTickMarks();    // tick marks on X axis
    DrawSettings();     // Start, Stop RBW, Ref level and Title
    freeze = false;
  }
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
      if (CentreFreq > span / 10)                                 // but don't shift into -ve frequency
      {
        CentreFreq = CentreFreq - (span / 10);
      }
    break;
    case 6:                                                       // Right 1/10 span
      CentreFreq = CentreFreq + (span / 10);
    break;
  }

  CentreFreq = ConstrainFreq(CentreFreq);

  // Store the new frequency
  snprintf(ValueToSave, 63, "%d", CentreFreq);
  SetConfigParam(PATH_CONFIG, "centrefreq", ValueToSave);
  printf("Centre Freq set to %d Hz\n", CentreFreq);

  // Calculate the new settings
  CalcSpan();

  // Trigger the frequency change
  NewFreq = true;

  DrawSettings();       // New labels
  freeze = false;
}


void CalcSpan()    // takes centre frequency and span and calculates startfreq and stopfreq and decimation
{
  startfreq = CentreFreq - (span / 2);
  stopfreq =  CentreFreq + (span / 2);

  switch (span)
  {
    case 100000:                                          // 100 kHz
      SampleRate = 3276800;
      decimation_factor = 32;
      fft_size = 512;
      break;
    case 200000:                                          // 200 kHz
      SampleRate = 3276800;
      decimation_factor = 16;
      fft_size = 512;
      break;
    case 500000:                                          // 500 kHz
      SampleRate = 4096000;
      decimation_factor = 8;
      fft_size = 512;
      break;
    case 1000000:                                         // 1000 kHz
      SampleRate = 4096000;
      decimation_factor = 4;
      fft_size = 512;
      break;
    case 2000000:                                         // 2000 kHz
      SampleRate = 4096000;
      decimation_factor = 2;
      fft_size = 512;
      break;
    case 5000000:                                         // 5000 kHz
      SampleRate = 5120000;
      decimation_factor = 1;
      fft_size = 512;
      break;
    case 10000000:                                        // 10000 kHz
      SampleRate = 10000000;                              // Spec says 10.66 MS, but api won't accept more then 10 MS
      decimation_factor = 1;
      fft_size = 500;
      break;
  }

  printf("fft size set to %d\n", fft_size);

  if (waterfall == true)
  {
    fft_time_smooth = 0.0;
  }
   else
  {
    // Set fft smoothing time
    if (Range20dB == false)
    {
      switch (span)
      {
        case 100000:                                          // 100 kHz
          fft_time_smooth = 0.91;
          break;
        case 200000:                                          // 200 kHz
          fft_time_smooth = 0.92;
          break;
        case 500000:                                          // 500 kHz
          fft_time_smooth = 0.93;
          break;
        case 1000000:                                         // 1000 kHz
          fft_time_smooth = 0.945;
           break;
       case 2000000:                                         // 2000 kHz
          fft_time_smooth = 0.95;
          break;
        case 5000000:                                         // 5000 kHz
          fft_time_smooth = 0.95;
          break;
        case 10000000:                                        // 10000 kHz
          fft_time_smooth = 0.96;
          break;
      }
    }
    else
    {
      switch (span)
      {
        case 100000:                                          // 100 kHz
          fft_time_smooth = 0.94;
          break;
        case 200000:                                          // 200 kHz
          fft_time_smooth = 0.94;
          break;
        case 500000:                                          // 500 kHz
          fft_time_smooth = 0.94;
          break;
        case 1000000:                                         // 1000 kHz
          fft_time_smooth = 0.94;
          break;
        case 2000000:                                         // 2000 kHz
          fft_time_smooth = 0.95;
          break;
        case 5000000:                                         // 5000 kHz
          fft_time_smooth = 0.98;
          break;
        case 10000000:                                        // 10000 kHz
          fft_time_smooth = 0.985;
          break;
      }
    }
  }
}

void ChangeLabel(int button)
{
  char RequestText[64];
  char InitText[64];
  char ValueToSave[63];
  char KeyboardReturn[63];

  div_t div_10;
  div_t div_100;
  div_t div_1000;
  div_t div_10000;
  div_t div_100000;
  div_t div_1000000;

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 2:                                                       // Centre Freq
      // If initial value is less than 1 MHz, use Hz
      if (CentreFreq < 1000000)
      {
        // Define request string
        strcpy(RequestText, "Enter new centre frequency in Hz");
        snprintf(InitText, 20, "%d", CentreFreq);

        // Ask for the new value
        do
        {
          Keyboard(RequestText, InitText, 11, KeyboardReturn, true);
        }
        while ((strlen(KeyboardReturn) == 0) || (atoi(KeyboardReturn) < 1000) || (atoi(KeyboardReturn) > 2000000000));

        CentreFreq = atoi(KeyboardReturn);
      }
      else                        // > 1 MHz to use MHz.
      {
        strcpy(RequestText, "Enter new centre frequency in MHz");

        // Define initial value and convert to MHz
        div_10 = div(CentreFreq, 10);
        div_1000000 = div(CentreFreq, 1000000);  //  so div_1000000.quotand div_1000000.rem are MHz and Hz

        if(div_10.rem != 0)  // last character (units) not zero, so make answer of form xxxx.xxxxxx
        {
          snprintf(InitText, 20, "%d.%06d", div_1000000.quot, div_1000000.rem);
        }
        else
        {
          div_100 = div(CentreFreq, 100);
          if(div_100.rem != 0)  // tens not zero, so make answer of form xxxx.xxxxx
          {
            snprintf(InitText, 20, "%d.%05d", div_1000000.quot, div_1000000.rem / 10);
          }
          else
          {
            div_1000 = div(CentreFreq, 1000);
            if(div_1000.rem != 0)  // hundreds not zero, so make answer of form xxxx.xxxx
            {
              snprintf(InitText, 20, "%d.%04d", div_1000000.quot, div_1000000.rem / 100);
            }
            else
            {
              div_10000 = div(CentreFreq, 10000);
              if(div_10000.rem != 0)  // thousands not zero, so make answer of form xxxx.xxx
              {
                snprintf(InitText, 20, "%d.%03d", div_1000000.quot, div_1000000.rem / 1000);
              }
              else
              {
                div_100000 = div(CentreFreq, 100000);
                if(div_100000.rem != 0)  // tens of thousands not zero, so make answer of form xxxx.xx
                {
                  snprintf(InitText, 20, "%d.%02d", div_1000000.quot, div_1000000.rem / 10000);
                }
                else
                {
                  div_1000000 = div(CentreFreq, 1000000);
                  if(div_1000000.rem != 0)  // hundreds of thousands not zero, so make answer of form xxxx.x
                  {
                    snprintf(InitText, 20, "%d.%01d", div_1000000.quot, div_1000000.rem / 100000);
                  }
                  else  // integer MHz, so just xxxx (no dp)
                  {
                    snprintf(InitText, 20, "%d", div_1000000.quot);
                  }
                }
              }
            }
          }
        }
        // Ask for the new value
        do
        {
          Keyboard(RequestText, InitText, 12, KeyboardReturn, true);
        }
        while ((strlen(KeyboardReturn) == 0) || (atof(KeyboardReturn) < 0.001) || (atof(KeyboardReturn) > 2000.0));

        CentreFreq = (int)((1000000 * atof(KeyboardReturn)) + 0.1);
      }

      CentreFreq = ConstrainFreq(CentreFreq);
      snprintf(ValueToSave, 63, "%d", CentreFreq);
      SetConfigParam(PATH_CONFIG, "centrefreq", ValueToSave);
      printf("Centre Freq set to %d Hz\n", CentreFreq);
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

      Keyboard(RequestText, InitText, 30, KeyboardReturn, true);
  
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
      strcpy(RequestText, "Enter offset freq in Hz. 0 for none, -ve for high side");
      snprintf(InitText, 20, "%ld", freqoffset);
 
      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 12, KeyboardReturn, true);
      }
      while (strlen(KeyboardReturn) == 0);

      freqoffset = strtoll(KeyboardReturn, NULL, 10);
      snprintf(ValueToSave, 63, "%ld", freqoffset);
      SetConfigParam(PATH_CONFIG, "freqoffset", ValueToSave);
      printf("Freq Offset set to %ld Hz\n", freqoffset);
      break;

  }

  // Tidy up, paint around the screen and then unfreeze
  CalcSpan();         // work out start and stop freqs
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawTickMarks();    // tick marks on X axis
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
  char ValueToSave[63];

  for (;;)
  {
    while(getTouchSample(&rawX, &rawY) == 0)
    {
      usleep(10);                                   // wait without burnout
    }

    printf("x=%d y=%d at top of WaitButton Event\n", rawX, rawY);

    FinishedButton = 1;
    i = IsMenuButtonPushed();
    if (i == -1)
    {
      continue;  //Pressed, but not on a button so wait for the next touch
      printf("Not a button in Menu %d\n", CurrentMenu);
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
          CurrentMenu=3;
          redrawMenu();
          break;
        case 3:                                            // Markers
          printf("Markers Menu 2 Requested\n");
          CurrentMenu=2;
          redrawMenu();
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
          redrawMenu();
          break;
        case 5:                                            // Left Arrow
        case 6:                                            // Right Arrow
          printf("Shift Frequency Requested\n");
          ShiftFrequency(i);
          //redrawMenu();
          RequestPeakValueZero = true;
          break;
        case 7:                                            // System
          printf("System Menu 4 Requested\n");
          CurrentMenu=4;
          redrawMenu();
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
            redrawMenu();
          }
          break;
        case 9:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
          break;
        default:
          printf("Menu 1 Error\n");
      }
      if(i != 8)
      {
        PortsdownExitRequested = false;
        redrawMenu();
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
            SetButtonStatus(CurrentMenu, 2, 1);
            SetButtonStatus(CurrentMenu, 4, 0);
          }
          else
          {
            markeron = false;
            markermode = 7;
            SetButtonStatus(CurrentMenu, 2, 0);
            SetButtonStatus(CurrentMenu, 4, 0);
          }
          redrawMenu();
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
            SetButtonStatus(CurrentMenu, 3, 1);
          }
          else
          {
            PeakPlot = false;
            RequestPeakValueZero = true;
            SetButtonStatus(CurrentMenu, 3, 0);
          }
          redrawMenu();
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
            SetButtonStatus(CurrentMenu, 2, 0);
            SetButtonStatus(CurrentMenu, 4, 1);
          }
          else
          {
            markeron = false;
            markermode = 7;
            SetButtonStatus(CurrentMenu, 2, 0);
            SetButtonStatus(CurrentMenu, 4, 0);
          }
          redrawMenu();
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
          SetButtonStatus(CurrentMenu, 2, 0);
          SetButtonStatus(CurrentMenu, 3, 0);
          SetButtonStatus(CurrentMenu, 4, 0);
          redrawMenu();
          break;
        case 8:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          redrawMenu();
          break;
        case 9:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 9, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 9, 1);
            freeze = true;
          }
          redrawMenu();
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
          redrawMenu();          
          RequestPeakValueZero = true;
          break;
        case 3:                                            // Frequency Presets
          printf("Frequency Preset Menu 7 Requested\n");
          CurrentMenu = 7;
          redrawMenu();
          RequestPeakValueZero = true;
          break;
        case 4:                                            // Span Width
          printf("Span Width Menu 6 Requested\n");
          CurrentMenu = 6;
          redrawMenu();
          RequestPeakValueZero = true;
          break;
        case 5:                                            // Gain
          printf("Gain Menu 8 Requested\n");
          CurrentMenu = 8;
          redrawMenu();
          RequestPeakValueZero = true;
          break;
        case 6:                                            // Waterfall Config
         printf("Waterfall Config Menu 9 Requested\n");
          CurrentMenu = 9;
          redrawMenu();
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
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
          DrawEmptyScreen();        // Required to set A value, which is not set in DrawTrace
          DrawYaxisLabels();        // dB calibration on LHS
          DrawTickMarks();          // tick marks on X axis
          DrawSettings();           // Start, Stop RBW, Ref level and Title
          redrawMenu();             // Draw the buttons
          freeze = false;           // Restart the scan
          break;
        case 3:                                            // Restart
          freeze = true;
          usleep(100000);
          clearScreen(0, 0, 0);
          usleep(1000000);
          closeScreen();
          cleanexit(150);
          break;
        case 4:                                            // Exit to Portsdown
          freeze = true;
          usleep(100000);
          clearScreen(0, 0, 0);
          usleep(1000000);
          closeScreen();
          cleanexit(129);
          break;
        case 5:                                            // Shutdown
          system("sudo shutdown now");
          break;
        case 6:                                            // Set Freq Presets
          printf("Set Freq Presets Menu 10 Requested\n");
          CurrentMenu = 10;
          redrawMenu();
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
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
          redrawMenu();
          break;
        case 3:                                            // Show 20 dB range
          SetMode(i);
          Range20dB = true;
          printf("20dB Menu 11 Requested\n");
          CurrentMenu = 11;
          redrawMenu();
          break;
        case 4:                                            // Waterfall
        case 5:                                            // Mix
          SetMode(i);
          redrawMenu();
          break;
        case 6:                                            // Config Menu
          printf("Config Menu 9 Requested\n");
          CurrentMenu = 9;
          redrawMenu();
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
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
        case 5:                                            // 1 MHz
        case 6:                                            // 2 MHz
        case 7:                                            // 5 MHz
        case 8:                                            // 10 MHz
          SetSpan(i);
          CurrentMenu = 6;
          redrawMenu();
          break;
        case 9:                                            // Return to Settings Menu
          CurrentMenu=3;
          redrawMenu();
          break;
        case 10:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 9, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 9, 1);
            freeze = true;
          }
          redrawMenu();
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
          redrawMenu();
          break;
        case 7:                                            // Return to Settings Menu
          printf("Settings Menu 3 requested\n");
          CurrentMenu=3;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
          break;
        default:
          printf("Menu 5 Error\n");
      }
      continue;  // Completed Menu 5 action, go and wait for touch
    }

    if (CurrentMenu == 8)  // Gain Menu
    {
      printf("Button Event %d, Entering Menu 8 Case Statement\n",i);
      CallingMenu = 8;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Local RF Gain
          LocalGain = true;
          CurrentMenu = 12;
          redrawMenu();
          break;
        case 3:                                            // Local IF Gain
          LocalGain = true;
          CurrentMenu = 13;
          redrawMenu();
          break;
        case 4:                                            // AGC On/Off
          if (agc == true)
          {
            agc = false;
            SetConfigParam(PATH_CONFIG, "agc", "off");
          }
          else
          {
            agc = true;
            SetConfigParam(PATH_CONFIG, "agc", "on");
          }
          NewGain = true;
          CurrentMenu = 8;
          redrawMenu();
          break;
        case 6:                                            // Show 20dB Lower
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
          CurrentMenu = 8;
          redrawMenu();
          break;
        case 7:                                            // Return to Settings Menu
          printf("Settings Menu 3 requested\n");
          CurrentMenu=3;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
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
        case 2:                                            // Set Waterfall Base
        case 3:                                            // Set waterfall range
          SetWfall(i);
          redrawMenu();
          break;
        case 4:                                            // Set Waterfall TimeSpan
          SetWfall(12);
          redrawMenu();
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
          redrawMenu();
          break;
        case 6:                                            // Advanced Config Menu
          printf("Advanced Config Menu 14 Requested\n");
          CurrentMenu = 14;
          redrawMenu();
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu=1;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
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
          redrawMenu();
          break;
        case 7:                                            // Return to Main Menu
          printf("Settings Menu 1 requested\n");
          CurrentMenu=1;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
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
          printf("Mode set to spectrum \n");
          CalcSpan();
          RedrawDisplay();
          CurrentMenu=1;
          redrawMenu();
          break;
        case 5:                                            // up
          if (BaseLine20dB <= -25)
          {
            BaseLine20dB = BaseLine20dB + 5;
            RedrawDisplay();
            snprintf(ValueToSave, 8, "%d", BaseLine20dB);
            SetConfigParam(PATH_CONFIG, "baseline20db", ValueToSave);
          }
          redrawMenu();
          break;
        case 6:                                            // down
          if (BaseLine20dB >= -75)
          {
            BaseLine20dB = BaseLine20dB - 5;
            RedrawDisplay();
            snprintf(ValueToSave, 8, "%d", BaseLine20dB);
            SetConfigParam(PATH_CONFIG, "baseline20db", ValueToSave);
          }
          redrawMenu();
          break;
        case 7:                                            // Return to Main Menu
          printf("Settings Menu 1 requested\n");
          CurrentMenu=1;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
          break;
        default:
          printf("Menu 11 Error\n");
      }
      continue;  // Completed Menu 10 action, go and wait for touch
    }
    if (CurrentMenu == 12)  // RF Gain Menu
    {
      printf("Button Event %d, Entering Menu 12 Case Statement\n",i);
      CallingMenu = 12;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            //   Max = 0
        case 3:                                            //   -6
        case 4:                                            //   -12
        case 5:                                            //   -18
        case 6:                                            //   -24
          SetGain(120 + i);
          redrawMenu();
          break;
        case 7:                                            // Return to Gains Menu
          printf("Gains Menu 8 Requested\n");
          CurrentMenu = 8;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
          break;
        default:
          printf("Menu 12 Error\n");
      }
      continue;  // Completed Menu 9 action, go and wait for touch
    }

    if (CurrentMenu == 13)  // IF Gain Menu
    {
      printf("Button Event %d, Entering Menu 13 Case Statement\n",i);
      CallingMenu = 13;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            //   -20  50
        case 3:                                            //   -30  25
        case 4:                                            //   -40  20
        case 5:                                            //   -50  10
        case 6:                                            //   -55  0
          SetGain(130 + i);
          redrawMenu();
          break;
        case 7:                                            // Return to Gains Menu
          printf("Gains Menu 8 Requested\n");
          CurrentMenu = 8;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
          break;
        default:
          printf("Menu 13 Error\n");
      }
      continue;  // Completed Menu 13 action, go and wait for touch
    }

    if (CurrentMenu == 14)  // Advanced Config Menu
    {
      printf("Button Event %d, Entering Menu 14 Case Statement\n",i);
      CallingMenu = 14;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Set Antenna Port
        case 3:                                            // Set BiasT volts
          SetPort(i);
          redrawMenu();
          break;
        case 6:                                            // Set display frequency Offset
          ChangeLabel(16);
          redrawMenu();
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 8:
          if (freeze)
          {
            SetButtonStatus(CurrentMenu, 8, 0);
            freeze = false;
          }
          else
          {
            SetButtonStatus(CurrentMenu, 8, 1);
            freeze = true;
          }
          redrawMenu();
          break;
        default:
          printf("Menu 14 Error\n");
      }
      continue;  // Completed Menu 14 action, go and wait for touch
    }


  }
  return NULL;
}


void redrawMenu()
// Paint each defined button and the title on the current Menu
{
  int i;

  if (markeron == false)
  {
    // Draw a black rectangle where the buttons are to erase them
    rectangle(620, 0, 160, 480, 0, 0, 0);
  }
  else   // But don't erase the marker text
  {
    rectangle(620, 0, 160, 420, 0, 0, 0);
  }


  switch(CurrentMenu)
  {
    case 1:
      Highlight_Menu1();
      break;
    case 2:
      Highlight_Menu2();
      break;
    case 3:
      Highlight_Menu3();
      break;
    case 4:
      Highlight_Menu4();
      break;
    case 5:
      Highlight_Menu5();
      break;
    case 6:
      Highlight_Menu6();
      break;
    case 7:
      Highlight_Menu7();
      break;
    case 8:
      Highlight_Menu8();
      break;
    case 9:
      Highlight_Menu9();
      break;
    case 10:
      Highlight_Menu10();
      break;
    case 11:
      Highlight_Menu11();
      break;
    case 12:
      Highlight_Menu12();
      break;
    case 13:
      Highlight_Menu13();
      break;
    case 14:
      Highlight_Menu14();
      break;
  }

  // Draw each button in turn

  for(i = 0; i <= MAX_BUTTON_ON_MENU - 1; i++)
  {
    if (ButtonArray[CurrentMenu][i].IndexStatus > 0)  // If button needs to be drawn
    {
      DrawButton(CurrentMenu, i);                     // Draw the button
    }
  }

  UpdateWeb();
  publish();
}


/////////////////////////////////////////////// DEFINE THE BUTTONS ///////////////////////////////

void Define_Menu1()                                  // Main Menu

{
  AddButtonStatus(1, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(1, 0, " ", &Black);

  AddButtonStatus(1, 1, "Main Menu", &Black);
  AddButtonStatus(1, 1, " ", &Green);

  AddButtonStatus(1, 2, "Settings", &Blue);
  AddButtonStatus(1, 2, " ", &Green);

  AddButtonStatus(1, 3, "Markers", &Blue);
  AddButtonStatus(1, 3, " ", &Green);

  AddButtonStatus(1, 4, "Mode", &Blue);
  AddButtonStatus(1, 4, "", &Green);

  AddButtonStatus(1, 5, "<-", &Blue);
  AddButtonStatus(1, 5, " ", &Green);

  AddButtonStatus(1, 6, "->", &Blue);
  AddButtonStatus(1, 6, " ", &Green);

  AddButtonStatus(1, 7, "System", &Blue);
  AddButtonStatus(1, 7, " ", &Green);

  AddButtonStatus(1, 8, "Exit to^Portsdown", &DBlue);
  AddButtonStatus(1, 8, "Exit to^Portsdown", &Red);

  AddButtonStatus(1, 9, "Freeze", &Blue);
  AddButtonStatus(1, 9, "Unfreeze", &Green);
}


void Highlight_Menu1()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  if (PortsdownExitRequested)
  {
    SetButtonStatus(1, 8, 1);
  }
  else
  {
    SetButtonStatus(1, 8, 0);
  }
}


void Define_Menu2()                                         // Marker Menu
{
  AddButtonStatus(2, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(2, 0, " ", &Black);

  AddButtonStatus(2, 1, "Marker^Menu", &Black);
  AddButtonStatus(2, 1, " ", &Green);

  AddButtonStatus(2, 2, "Peak", &Blue);
  AddButtonStatus(2, 2, "Peak", &Green);

  AddButtonStatus(2, 3, "Peak^Hold", &Blue);
  AddButtonStatus(2, 3, "Peak^Hold", &Green);

  AddButtonStatus(2, 4, "Manual", &Blue);
  AddButtonStatus(2, 4, "Manual", &Green);

  AddButtonStatus(2, 5, "<-", &Blue);
  AddButtonStatus(2, 5, "<-", &Grey);

  AddButtonStatus(2, 6, "->", &Blue);
  AddButtonStatus(2, 6, "->", &Grey);

  AddButtonStatus(2, 7, "Markers^Off", &Blue);
  AddButtonStatus(2, 7, " ", &Green);

  AddButtonStatus(2, 8, "Return to^Main Menu", &DBlue);

  AddButtonStatus(2, 9, "Freeze", &Blue);
  AddButtonStatus(2, 9, "Unfreeze", &Green);
}


void Highlight_Menu2()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  if ((markeron == true) && (markermode == 4))  // Manual Markers
  {
    SetButtonStatus(2, 5, 0);
    SetButtonStatus(2, 6, 0);
  }
  else
  {
    SetButtonStatus(2, 5, 1);
    SetButtonStatus(2, 6, 1);
  }
}


void Define_Menu3()                                           // Settings Menu
{
  AddButtonStatus(3, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(3, 0, " ", &Black);

  AddButtonStatus(3, 1, "Settings^Menu", &Black);
  AddButtonStatus(3, 1, " ", &Green);

  AddButtonStatus(3, 2, "Centre^Freq", &Blue);
  AddButtonStatus(3, 2, " ", &Green);

  AddButtonStatus(3, 3, "Freq^Presets", &Blue);
  AddButtonStatus(3, 3, " ", &Green);

  AddButtonStatus(3, 4, "Span^Width", &Blue);
  AddButtonStatus(3, 4, " ", &Green);

  AddButtonStatus(3, 5, "Gain", &Blue);
  AddButtonStatus(3, 5, " ", &Green);

  AddButtonStatus(3, 6, "Waterfall^Config", &Blue);
  AddButtonStatus(3, 6, " ", &Green);

  AddButtonStatus(3, 7, "Return to^Main Menu", &DBlue);

  AddButtonStatus(3, 8, "Freeze", &Blue);
  AddButtonStatus(3, 8, "Unfreeze", &Green);
}


void Highlight_Menu3()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);
}


void Define_Menu4()                                         // System Menu
{
  AddButtonStatus(4, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(4, 0, " ", &Black);

  AddButtonStatus(4, 1, "System^Menu", &Black);
  AddButtonStatus(4, 1, " ", &Green);

  AddButtonStatus(4, 2, "Snap^Viewer", &Blue);
  AddButtonStatus(4, 2, " ", &Green);

  AddButtonStatus(4, 3, "Re-start^Viewer", &Blue);
  AddButtonStatus(4, 3, " ", &Green);

  AddButtonStatus(4, 4, "Exit to^Portsdown", &Blue);
  AddButtonStatus(4, 4, " ", &Green);

  AddButtonStatus(4, 5, "Shutdown^System", &Blue);
  AddButtonStatus(4, 5, " ", &Green);

  AddButtonStatus(4, 6, "Set Freq^Presets", &Blue);

  AddButtonStatus(4, 7, "Return to^Main Menu", &DBlue);

  AddButtonStatus(4, 8, "Freeze", &Blue);
  AddButtonStatus(4, 8, "Unfreeze", &Green);
}


void Highlight_Menu4()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);
}


void Define_Menu5()                                          // Mode Menu
{
  AddButtonStatus(5, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(5, 0, " ", &Black);

  AddButtonStatus(5, 1, "Mode^Menu", &Black);
  AddButtonStatus(5, 1, " ", &Green);

  AddButtonStatus(5, 2, "Classic^SA Display", &Blue);
  AddButtonStatus(5, 2, " ", &Green);

  AddButtonStatus(5, 3, "20 dB Range^SA Display", &Blue);
  AddButtonStatus(5, 3, " ", &Green);

  AddButtonStatus(5, 4, "Waterfall", &Blue);
  AddButtonStatus(5, 4, "Waterfall", &Green);

  AddButtonStatus(5, 5, "Mix", &Blue);
  AddButtonStatus(5, 5, "Mix", &Green);

  AddButtonStatus(5, 6, "Waterfall^Config", &Blue);

  AddButtonStatus(5, 7, "Return to^Main Menu", &DBlue);

  AddButtonStatus(5, 8, "Freeze", &Blue);
  AddButtonStatus(5, 8, "Unfreeze", &Green);
}

void Highlight_Menu5()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);
}

void Define_Menu6()                                           // Span Menu
{
  AddButtonStatus(6, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(6, 0, " ", &Black);

  AddButtonStatus(6, 1, "Span^Menu", &Black);
  AddButtonStatus(6, 1, " ", &Green);

  AddButtonStatus(6, 2, "100 kHz", &Blue);
  AddButtonStatus(6, 2, "100 kHz", &Green);

  AddButtonStatus(6, 3, "200 kHz", &Blue);
  AddButtonStatus(6, 3, "200 kHz", &Green);

  AddButtonStatus(6, 4, "500 kHz", &Blue);
  AddButtonStatus(6, 4, "500 kHz", &Green);

  AddButtonStatus(6, 5, "1^MHz", &Blue);
  AddButtonStatus(6, 5, "1^MHz", &Green);

  AddButtonStatus(6, 6, "2^MHz", &Blue);
  AddButtonStatus(6, 6, "2^MHz", &Green);

  AddButtonStatus(6, 7, "5^MHz", &Blue);
  AddButtonStatus(6, 7, "5^MHz", &Green);

  AddButtonStatus(6, 8, "10^MHz", &Blue);
  AddButtonStatus(6, 8, "10^MHz", &Green);

  AddButtonStatus(6, 9, "Back to^Settings", &DBlue);

  AddButtonStatus(6, 10, "Freeze", &Blue);
  AddButtonStatus(6, 10, "Unfreeze", &Green);
}

void Highlight_Menu6()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  SetButtonStatus(CurrentMenu, 2, 0);
  SetButtonStatus(CurrentMenu, 3, 0);
  SetButtonStatus(CurrentMenu, 4, 0);
  SetButtonStatus(CurrentMenu, 5, 0);
  SetButtonStatus(CurrentMenu, 6, 0);
  SetButtonStatus(CurrentMenu, 7, 0);
  SetButtonStatus(CurrentMenu, 8, 0);

  switch (span)
  {
    case 100000:                                          // 100 kHz
      SetButtonStatus(CurrentMenu, 2, 1);
      break;
    case 200000:                                          // 200 kHz
      SetButtonStatus(CurrentMenu, 3, 1);
      break;
    case 500000:                                          // 500 kHz
      SetButtonStatus(CurrentMenu, 4, 1);
      break;
    case 1000000:                                         // 1000 kHz
      SetButtonStatus(CurrentMenu, 5, 1);
      break;
    case 2000000:                                         // 2000 kHz
      SetButtonStatus(CurrentMenu, 6, 1);
      break;
    case 5000000:                                         // 5000 kHz
      SetButtonStatus(CurrentMenu, 7, 1);
      break;
    case 10000000:                                        // 10000 kHz
      SetButtonStatus(CurrentMenu, 8, 1);
      break;
  }
}

void Define_Menu7()                                            //Presets Menu
{
  AddButtonStatus(7, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(7, 0, " ", &Black);

  AddButtonStatus(7, 1, "Presets^Menu", &Black);
  AddButtonStatus(7, 1, " ", &Green);

  AddButtonStatus(7, 2, "kHz", &Blue);
  AddButtonStatus(7, 2, "kHz", &Green);

  AddButtonStatus(7, 3, "kHz", &Blue);
  AddButtonStatus(7, 3, "kHz", &Green);

  AddButtonStatus(7, 4, "kHz", &Blue);
  AddButtonStatus(7, 4, "kHz", &Green);

  AddButtonStatus(7, 5, "kHz", &Blue);
  AddButtonStatus(7, 5, "kHz", &Green);

  AddButtonStatus(7, 6, "kHz", &Blue);
  AddButtonStatus(7, 6, "kHz", &Green);

  AddButtonStatus(7, 7, "Back to^Settings", &DBlue);

  AddButtonStatus(7, 8, "Freeze", &Blue);
  AddButtonStatus(7, 8, "Unfreeze", &Green);
}

void Highlight_Menu7()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  char ButtText[15];
  snprintf(ButtText, 14, "%0.2f^MHz", ((float)pfreq1) / 1000000);
  AmendButtonStatus(CurrentMenu, 2, 0, ButtText, &Blue);
  AmendButtonStatus(CurrentMenu, 2, 1, ButtText, &Green);

  snprintf(ButtText, 14, "%0.2f^MHz", ((float)pfreq2) / 1000000);
  AmendButtonStatus(CurrentMenu, 3, 0, ButtText, &Blue);
  AmendButtonStatus(CurrentMenu, 3, 1, ButtText, &Green);

  snprintf(ButtText, 14, "%0.2f^MHz", ((float)pfreq3) / 1000000);
  AmendButtonStatus(CurrentMenu, 4, 0, ButtText, &Blue);
  AmendButtonStatus(CurrentMenu, 4, 1, ButtText, &Green);

  snprintf(ButtText, 14, "%0.2f^MHz", ((float)pfreq4) / 1000000);
  AmendButtonStatus(CurrentMenu, 5, 0, ButtText, &Blue);
  AmendButtonStatus(CurrentMenu, 5, 1, ButtText, &Green);

  snprintf(ButtText, 14, "%0.2f^MHz", ((float)pfreq5) / 1000000);
  AmendButtonStatus(CurrentMenu, 6, 0, ButtText, &Blue);
  AmendButtonStatus(CurrentMenu, 6, 1, ButtText, &Green);

  if (CentreFreq == pfreq1)
  {
    SetButtonStatus(CurrentMenu, 2, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 2, 0);
  }
  if (CentreFreq == pfreq2)
  {
    SetButtonStatus(CurrentMenu, 3, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 3, 0);
  }
  if (CentreFreq == pfreq3)
  {
    SetButtonStatus(CurrentMenu, 4, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 4, 0);
  }
  if (CentreFreq == pfreq4)
  {
    SetButtonStatus(CurrentMenu, 5, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 5, 0);
  }
  if (CentreFreq == pfreq5)
  {
    SetButtonStatus(CurrentMenu, 6, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 6, 0);
  }
}


void Define_Menu8()                                    // Gain Menu
{
  AddButtonStatus(8, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(8, 0, " ", &Black);

  AddButtonStatus(8, 1, "Gain^Menu", &Black);

  AddButtonStatus(8, 2, "RF Gain", &Blue);

  AddButtonStatus(8, 3, "IF Gain", &Blue);

  AddButtonStatus(8, 4, "AGC^Off", &Blue);
  AddButtonStatus(8, 4, "AGC^On", &Blue);

  //AddButtonStatus(8, 5, "Remote^RF Gain", &Blue);

  AddButtonStatus(8, 6, "Show 20dB^Lower", &Blue);
  AddButtonStatus(8, 6, "Show 20dB^Lower", &Green);

  AddButtonStatus(8, 7, "Back to^Settings", &DBlue);

  AddButtonStatus(8, 8, "Freeze", &Blue);
  AddButtonStatus(8, 8, "Unfreeze", &Green);
}


void Highlight_Menu8()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  if (agc == true)
  {
    SetButtonStatus(CurrentMenu, 4, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 4, 0);
  }
  if (BaselineShift == 20)
  {
    SetButtonStatus(CurrentMenu, 6, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 6, 0);
  }
}


void Define_Menu9()                                          // Config Menu
{
  AddButtonStatus(9, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(9, 0, " ", &Black);

  AddButtonStatus(9, 1, "Wfall^Config Menu", &Black);

  AddButtonStatus(9, 2, "Set Wfall^Base Level", &Blue);

  AddButtonStatus(9, 3, "Set Wfall^Range", &Blue);

  AddButtonStatus(9, 4, "Set Wfall^Time Span", &Blue);

  AddButtonStatus(9, 5, "Waterfall^Normal", &Blue);

  AddButtonStatus(9, 6, "Advanced^Config Menu", &Blue);

  AddButtonStatus(9, 7, "Return to^Main Menu", &DBlue);

  AddButtonStatus(9, 8, "Freeze", &Blue);
  AddButtonStatus(9, 8, "Unfreeze", &Green);
}


void Highlight_Menu9()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  char ButtText[31] = "Waterfall";
  if (strcmp(TraceType, "single") == 0)
  {
    snprintf(ButtText, 30, "Waterfall^Normal");
  }
  else if (strcmp(TraceType, "average") == 0)
  {
    snprintf(ButtText, 30, "Waterfall^Average");
  }
  else if (strcmp(TraceType, "peak") == 0)
  {
    snprintf(ButtText, 30, "Waterfall^Peak");
  }
  AmendButtonStatus(CurrentMenu, 5, 0, ButtText, &Blue);
}


void Define_Menu10()                                          // Set Freq Presets Menu
{
  AddButtonStatus(10, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(10, 0, " ", &Black);

  AddButtonStatus(10, 1, "Set Freq^Presets", &Black);

  AddButtonStatus(10, 2, "Preset 1", &Blue);

  AddButtonStatus(10, 3, "Preset 2", &Blue);

  AddButtonStatus(10, 4, "Preset 3", &Blue);

  AddButtonStatus(10, 5, "Preset 4", &Blue);

  AddButtonStatus(10, 6, "Preset 5", &Blue);

  AddButtonStatus(10, 7, "Return to^Main Menu", &DBlue);

  AddButtonStatus(10, 8, "Freeze", &Blue);
  AddButtonStatus(10, 8, "Unfreeze", &Green);
}

void Highlight_Menu10()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  char ButtText[31];
  snprintf(ButtText, 30, "Preset 1^%0.2f MHz", ((float)pfreq1) / 1000000);
  AmendButtonStatus(CurrentMenu, 2, 0, ButtText, &Blue);

  snprintf(ButtText, 30, "Preset 2^%0.2f MHz", ((float)pfreq2) / 1000000);
  AmendButtonStatus(CurrentMenu, 3, 0, ButtText, &Blue);

  snprintf(ButtText, 30, "Preset 3^%0.2f MHz", ((float)pfreq3) / 1000000);
  AmendButtonStatus(CurrentMenu, 4, 0, ButtText, &Blue);

  snprintf(ButtText, 30, "Preset 4^%0.2f MHz", ((float)pfreq4) / 1000000);
  AmendButtonStatus(CurrentMenu, 5, 0, ButtText, &Blue);

  snprintf(ButtText, 30, "Preset 5^%0.2f MHz", ((float)pfreq5) / 1000000);
  AmendButtonStatus(CurrentMenu, 6, 0, ButtText, &Blue);
}


void Define_Menu11()                                          // 20db Range Menu
{
  AddButtonStatus(11, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(11, 0, " ", &Black);

  AddButtonStatus(11, 1, "Control^20 dB Range", &Black);

  AddButtonStatus(11, 2, "Back to^Full Range", &Blue);

  //AddButtonStatus(11, 3, "Preset 2", &Blue);

  //AddButtonStatus(11, 4, "Preset 3", &Blue);

  AddButtonStatus(11, 5, "Up", &Blue);

  AddButtonStatus(11, 6, "Down", &Blue);

  AddButtonStatus(11, 7, "Return to^Main Menu", &DBlue);

  AddButtonStatus(11, 8, "Freeze", &Blue);
  AddButtonStatus(11, 8, "Unfreeze", &Green);
}


void Highlight_Menu11()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);
}


void Define_Menu12()                                          // RF Gain Menu
{
  AddButtonStatus(12, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(12, 0, " ", &Black);

  AddButtonStatus(12, 1, "RF Gain^Menu", &Black);

  AddButtonStatus(12, 2, "0 dB", &Blue);
  AddButtonStatus(12, 2, "0 dB", &Green);

  AddButtonStatus(12, 3, "-6 dB", &Blue);
  AddButtonStatus(12, 3, "-6 dB", &Green);

  AddButtonStatus(12, 4, "-12 dB", &Blue);
  AddButtonStatus(12, 4, "-12 dB", &Green);

  AddButtonStatus(12, 5, "-18 dB", &Blue);
  AddButtonStatus(12, 5, "-18 dB", &Green);

  AddButtonStatus(12, 6, "-24 dB", &Blue);
  AddButtonStatus(12, 6, "-24 dB", &Green);

  AddButtonStatus(12, 7, "Return to^Gains Menu", &DBlue);

  AddButtonStatus(12, 8, "Freeze", &Blue);
  AddButtonStatus(12, 8, "Unfreeze", &Green);
}


void Highlight_Menu12()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  if (RFgain == 0)
  {
    SetButtonStatus(CurrentMenu, 2, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 2, 0);
  }
  if (RFgain == 2)
  {
    SetButtonStatus(CurrentMenu, 3, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 3, 0);
  }
  if (RFgain == 4)
  {
    SetButtonStatus(CurrentMenu, 4, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 4, 0);
  }
  if (RFgain == 6)
  {
    SetButtonStatus(CurrentMenu, 5, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 5, 0);
  }
  if (RFgain == 7)
  {
    SetButtonStatus(CurrentMenu, 6, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 6, 0);
  }
}


void Define_Menu13()                                          // IF Gain Menu
{
  AddButtonStatus(13, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(13, 0, " ", &Black);

  AddButtonStatus(13, 1, "IF Gain^Menu", &Black);

  AddButtonStatus(13, 2, "0 dB", &Blue);
  AddButtonStatus(13, 2, "0 dB", &Green);

  AddButtonStatus(13, 3, "-10 dB", &Blue);
  AddButtonStatus(13, 3, "-10 dB", &Green);

  AddButtonStatus(13, 4, "-20 dB", &Blue);
  AddButtonStatus(13, 4, "-20 dB", &Green);

  AddButtonStatus(13, 5, "-30 dB", &Blue);
  AddButtonStatus(13, 5, "-30 dB", &Green);

  AddButtonStatus(13, 6, "-39 dB", &Blue);
  AddButtonStatus(13, 6, "-39 dB", &Green);

  AddButtonStatus(13, 7, "Return to^Gain Menu", &DBlue);

  AddButtonStatus(13, 8, "Freeze", &Blue);
  AddButtonStatus(13, 8, "Unfreeze", &Green);
}


void Highlight_Menu13()
{
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  if (IFgain == 20)
  {
    SetButtonStatus(CurrentMenu, 2, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 2, 0);
  }
  if (IFgain == 30)
  {
    SetButtonStatus(CurrentMenu, 3, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 3, 0);
  }
  if (IFgain == 40)
  {
    SetButtonStatus(CurrentMenu, 4, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 4, 0);
  }
  if (IFgain == 50)
  {
    SetButtonStatus(CurrentMenu, 5, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 5, 0);
  }
  if (IFgain == 59)
  {
    SetButtonStatus(CurrentMenu, 6, 1);
  }
  else
  {
    SetButtonStatus(CurrentMenu, 6, 0);
  }
}

void Define_Menu14()                                          // Advanced Config Menu
{
  AddButtonStatus(14, 0, "Capture^Snap", &DGrey);
  AddButtonStatus(14, 0, " ", &Black);

  AddButtonStatus(14, 1, "Advanced^Config", &Black);

  AddButtonStatus(14, 2, "Antenna^port", &Blue);

  AddButtonStatus(14, 3, "BiasT Volts^Off", &Blue);
  AddButtonStatus(14, 3, "BiasT Volts^On", &Green);

  //AddButtonStatus(14, 4, "-20 dB", &Blue);
  //AddButtonStatus(14, 4, "-20 dB", &Green);

  //AddButtonStatus(14, 5, "-30 dB", &Blue);
  //AddButtonStatus(14, 5, "-30 dB", &Green);

  AddButtonStatus(14, 6, "Set Freq^Offset", &Blue);
  AddButtonStatus(14, 6, "Freq^Offset On", &Green);

  AddButtonStatus(14, 7, "Return to^Main Menu", &DBlue);

  AddButtonStatus(14, 8, "Freeze", &Blue);
  AddButtonStatus(14, 8, "Unfreeze", &Green);
}


void Highlight_Menu14()
{
  char ButtText[31];
  char SnapText[31];

  snprintf(SnapText, 30, "Capture^Snap %d", SnapNumber);
  AmendButtonStatus(CurrentMenu, 0, 0, SnapText, &DGrey);

  switch (Antenna_port)
  {
    case 0:
      strcpy(ButtText, "Antenna^Port A");
      break;
    case 1:
      strcpy(ButtText, "Antenna^Port B");
      break;
    case 2:
      strcpy(ButtText, "Antenna^Port C/Hi Z");
      break;
  }
  AmendButtonStatus(CurrentMenu, 2, 0, ButtText, &Blue);

  if (BiasT_volts)
  {
    SetButtonStatus(14, 3, 1);
  }
  else
  {
    SetButtonStatus(14, 3, 0);
  }

  if (freqoffset == 0)
  {
    SetButtonStatus(14, 6, 0);
  }
  else
  {
    SetButtonStatus(14, 6, 1);
  }
}


void Define_Menu41()
{
  AddButtonStatus(41, 0, "SPACE", &Blue);
  AddButtonStatus(41, 0, "SPACE", &LBlue);
  AddButtonStatus(41, 0, "SPACE", &Blue);
  AddButtonStatus(41, 0, "SPACE", &LBlue);
  AddButtonStatus(41, 1, "del", &Blue);
  AddButtonStatus(41, 1, "del", &LBlue);
  AddButtonStatus(41, 1, "del", &Blue);
  AddButtonStatus(41, 1, "del", &LBlue);
  AddButtonStatus(41, 2, "<", &Blue);
  AddButtonStatus(41, 2, "<", &LBlue);
  AddButtonStatus(41, 2, "<", &Blue);
  AddButtonStatus(41, 2, "<", &LBlue);
  AddButtonStatus(41, 3, ">", &Blue);
  AddButtonStatus(41, 3, ">", &LBlue);
  AddButtonStatus(41, 3, ">", &Blue);
  AddButtonStatus(41, 3, ">", &LBlue);
  AddButtonStatus(41, 4, "-", &Blue);
  AddButtonStatus(41, 4, "-", &LBlue);
  AddButtonStatus(41, 4, "_", &Blue);
  AddButtonStatus(41, 4, "_", &LBlue);
  AddButtonStatus(41, 5, "Clear", &Blue);
  AddButtonStatus(41, 5, "Clear", &LBlue);
  AddButtonStatus(41, 5, "Clear", &Blue);
  AddButtonStatus(41, 5, "Clear", &LBlue);
  AddButtonStatus(41, 6, "^", &LBlue);
  AddButtonStatus(41, 6, "^", &Blue);
  AddButtonStatus(41, 6, "^", &Blue);
  AddButtonStatus(41, 6, "^", &LBlue);
  AddButtonStatus(41, 7, "^", &LBlue);
  AddButtonStatus(41, 7, "^", &Blue);
  AddButtonStatus(41, 7, "^", &Blue);
  AddButtonStatus(41, 7, "^", &LBlue);
  AddButtonStatus(41, 8, "Enter", &Blue);
  AddButtonStatus(41, 8, "Enter", &LBlue);
  AddButtonStatus(41, 8, "Enter", &Blue);
  AddButtonStatus(41, 8, "Enter", &LBlue);
  AddButtonStatus(41, 9, "<=", &Blue);
  AddButtonStatus(41, 9, "<=", &LBlue);
  AddButtonStatus(41, 9, "<=", &Blue);
  AddButtonStatus(41, 9, "<=", &LBlue);

  AddButtonStatus(41, 10, "Z", &Blue);
  AddButtonStatus(41, 10, "Z", &LBlue);
  AddButtonStatus(41, 10, "z", &Blue);
  AddButtonStatus(41, 10, "z", &LBlue);
  AddButtonStatus(41, 11, "X", &Blue);
  AddButtonStatus(41, 11, "X", &LBlue);
  AddButtonStatus(41, 11, "x", &Blue);
  AddButtonStatus(41, 11, "x", &LBlue);
  AddButtonStatus(41, 12, "C", &Blue);
  AddButtonStatus(41, 12, "C", &LBlue);
  AddButtonStatus(41, 12, "c", &Blue);
  AddButtonStatus(41, 12, "c", &LBlue);
  AddButtonStatus(41, 13, "V", &Blue);
  AddButtonStatus(41, 13, "V", &LBlue);
  AddButtonStatus(41, 13, "v", &Blue);
  AddButtonStatus(41, 13, "v", &LBlue);
  AddButtonStatus(41, 14, "B", &Blue);
  AddButtonStatus(41, 14, "B", &LBlue);
  AddButtonStatus(41, 14, "b", &Blue);
  AddButtonStatus(41, 14, "b", &LBlue);
  AddButtonStatus(41, 15, "N", &Blue);
  AddButtonStatus(41, 15, "N", &LBlue);
  AddButtonStatus(41, 15, "n", &Blue);
  AddButtonStatus(41, 15, "n", &LBlue);
  AddButtonStatus(41, 16, "M", &Blue);
  AddButtonStatus(41, 16, "M", &LBlue);
  AddButtonStatus(41, 16, "m", &Blue);
  AddButtonStatus(41, 16, "m", &LBlue);
  AddButtonStatus(41, 17, ",", &Blue);
  AddButtonStatus(41, 17, ",", &LBlue);
  AddButtonStatus(41, 17, "!", &Blue);
  AddButtonStatus(41, 17, "!", &LBlue);
  AddButtonStatus(41, 18, ".", &Blue);
  AddButtonStatus(41, 18, ".", &LBlue);
  AddButtonStatus(41, 18, ".", &Blue);
  AddButtonStatus(41, 18, ".", &LBlue);
  AddButtonStatus(41, 19, "/", &Blue);
  AddButtonStatus(41, 19, "/", &LBlue);
  AddButtonStatus(41, 19, "?", &Blue);
  AddButtonStatus(41, 19, "?", &LBlue);

  AddButtonStatus(41, 20, "A", &Blue);
  AddButtonStatus(41, 20, "A", &LBlue);
  AddButtonStatus(41, 20, "a", &Blue);
  AddButtonStatus(41, 20, "a", &LBlue);
  AddButtonStatus(41, 21, "S", &Blue);
  AddButtonStatus(41, 21, "S", &LBlue);
  AddButtonStatus(41, 21, "s", &Blue);
  AddButtonStatus(41, 21, "s", &LBlue);
  AddButtonStatus(41, 22, "D", &Blue);
  AddButtonStatus(41, 22, "D", &LBlue);
  AddButtonStatus(41, 22, "d", &Blue);
  AddButtonStatus(41, 22, "d", &LBlue);
  AddButtonStatus(41, 23, "F", &Blue);
  AddButtonStatus(41, 23, "F", &LBlue);
  AddButtonStatus(41, 23, "f", &Blue);
  AddButtonStatus(41, 23, "f", &LBlue);
  AddButtonStatus(41, 24, "G", &Blue);
  AddButtonStatus(41, 24, "G", &LBlue);
  AddButtonStatus(41, 24, "g", &Blue);
  AddButtonStatus(41, 24, "g", &LBlue);
  AddButtonStatus(41, 25, "H", &Blue);
  AddButtonStatus(41, 25, "H", &LBlue);
  AddButtonStatus(41, 25, "h", &Blue);
  AddButtonStatus(41, 25, "h", &LBlue);
  AddButtonStatus(41, 26, "J", &Blue);
  AddButtonStatus(41, 26, "J", &LBlue);
  AddButtonStatus(41, 26, "j", &Blue);
  AddButtonStatus(41, 26, "j", &LBlue);
  AddButtonStatus(41, 27, "K", &Blue);
  AddButtonStatus(41, 27, "K", &LBlue);
  AddButtonStatus(41, 27, "k", &Blue);
  AddButtonStatus(41, 27, "k", &LBlue);
  AddButtonStatus(41, 28, "L", &Blue);
  AddButtonStatus(41, 28, "L", &LBlue);
  AddButtonStatus(41, 28, "l", &Blue);
  AddButtonStatus(41, 28, "l", &LBlue);

  //AddButtonStatus(41, 29, "/", &Blue);
  //AddButtonStatus(41, 29, "/", &LBlue);
  //AddButtonStatus(41, 29, "/", &Blue);
  //AddButtonStatus(41, 29, "/", &LBlue);

  AddButtonStatus(41, 30, "Q", &Blue);
  AddButtonStatus(41, 30, "Q", &LBlue);
  AddButtonStatus(41, 30, "q", &Blue);
  AddButtonStatus(41, 30, "q", &LBlue);
  AddButtonStatus(41, 31, "W", &Blue);
  AddButtonStatus(41, 31, "W", &LBlue);
  AddButtonStatus(41, 31, "w", &Blue);
  AddButtonStatus(41, 31, "w", &LBlue);
  AddButtonStatus(41, 32, "E", &Blue);
  AddButtonStatus(41, 32, "E", &LBlue);
  AddButtonStatus(41, 32, "e", &Blue);
  AddButtonStatus(41, 32, "e", &LBlue);
  AddButtonStatus(41, 33, "R", &Blue);
  AddButtonStatus(41, 33, "R", &LBlue);
  AddButtonStatus(41, 33, "r", &Blue);
  AddButtonStatus(41, 33, "r", &LBlue);
  AddButtonStatus(41, 34, "T", &Blue);
  AddButtonStatus(41, 34, "T", &LBlue);
  AddButtonStatus(41, 34, "t", &Blue);
  AddButtonStatus(41, 34, "t", &LBlue);
  AddButtonStatus(41, 35, "Y", &Blue);
  AddButtonStatus(41, 35, "Y", &LBlue);
  AddButtonStatus(41, 35, "y", &Blue);
  AddButtonStatus(41, 35, "y", &LBlue);
  AddButtonStatus(41, 36, "U", &Blue);
  AddButtonStatus(41, 36, "U", &LBlue);
  AddButtonStatus(41, 36, "u", &Blue);
  AddButtonStatus(41, 36, "u", &LBlue);
  AddButtonStatus(41, 37, "I", &Blue);
  AddButtonStatus(41, 37, "I", &LBlue);
  AddButtonStatus(41, 37, "i", &Blue);
  AddButtonStatus(41, 37, "i", &LBlue);
  AddButtonStatus(41, 38, "O", &Blue);
  AddButtonStatus(41, 38, "O", &LBlue);
  AddButtonStatus(41, 38, "o", &Blue);
  AddButtonStatus(41, 38, "o", &LBlue);
  AddButtonStatus(41, 39, "P", &Blue);
  AddButtonStatus(41, 39, "P", &LBlue);
  AddButtonStatus(41, 39, "p", &Blue);
  AddButtonStatus(41, 39, "p", &LBlue);

  AddButtonStatus(41, 40, "1", &Blue);
  AddButtonStatus(41, 40, "1", &LBlue);
  AddButtonStatus(41, 40, "1", &Blue);
  AddButtonStatus(41, 40, "1", &LBlue);
  AddButtonStatus(41, 41, "2", &Blue);
  AddButtonStatus(41, 41, "2", &LBlue);
  AddButtonStatus(41, 41, "2", &Blue);
  AddButtonStatus(41, 41, "2", &LBlue);
  AddButtonStatus(41, 42, "3", &Blue);
  AddButtonStatus(41, 42, "3", &LBlue);
  AddButtonStatus(41, 42, "3", &Blue);
  AddButtonStatus(41, 42, "3", &LBlue);
  AddButtonStatus(41, 43, "4", &Blue);
  AddButtonStatus(41, 43, "4", &LBlue);
  AddButtonStatus(41, 43, "4", &Blue);
  AddButtonStatus(41, 43, "4", &LBlue);
  AddButtonStatus(41, 44, "5", &Blue);
  AddButtonStatus(41, 44, "5", &LBlue);
  AddButtonStatus(41, 44, "5", &Blue);
  AddButtonStatus(41, 44, "5", &LBlue);
  AddButtonStatus(41, 45, "6", &Blue);
  AddButtonStatus(41, 45, "6", &LBlue);
  AddButtonStatus(41, 45, "6", &Blue);
  AddButtonStatus(41, 45, "6", &LBlue);
  AddButtonStatus(41, 46, "7", &Blue);
  AddButtonStatus(41, 46, "7", &LBlue);
  AddButtonStatus(41, 46, "7", &Blue);
  AddButtonStatus(41, 46, "7", &LBlue);
  AddButtonStatus(41, 47, "8", &Blue);
  AddButtonStatus(41, 47, "8", &LBlue);
  AddButtonStatus(41, 47, "8", &Blue);
  AddButtonStatus(41, 47, "8", &LBlue);
  AddButtonStatus(41, 48, "9", &Blue);
  AddButtonStatus(41, 48, "9", &LBlue);
  AddButtonStatus(41, 48, "9", &Blue);
  AddButtonStatus(41, 48, "9", &LBlue);
  AddButtonStatus(41, 49, "0", &Blue);
  AddButtonStatus(41, 49, "0", &LBlue);
  AddButtonStatus(41, 49, "0", &Blue);
  AddButtonStatus(41, 49, "0", &LBlue);
}


/////////////////////////////////////////// APPLICATION DRAWING //////////////////////////////////


void DrawEmptyScreen()
{
  int div = 0;
  
    //clearScreen(0, 0, 0);
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
  char caption[15];
  int i;
  int pixel_brightness;

  // Clear the previous scale first
  rectangle(20, 63, 79, 417, 0, 0, 0);

  // Clear the previous waterfall calibration
  rectangle(610, 63, 5, 417, 0, 0, 0);

  if ((Range20dB == false) && (waterfall == false))       // Full range spectrum
  {
    if (BaselineShift == 0)                     // 0 to -80
    {
      Text(48, 462, "0 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 416, "-10 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 366, "-20 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 316, "-30 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 266, "-40 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 216, "-50 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 166, "-60 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 116, "-70 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30,  66, "-80 dB", font_ptr, 0, 0, 0, 255, 255, 255);
    }
    else                                         // -20 to -100
    {
      Text(30, 462, "-20 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 416, "-30 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 366, "-40 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 316, "-50 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 266, "-60 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 216, "-70 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 166, "-80 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(30, 116, "-90 dB", font_ptr, 0, 0, 0, 255, 255, 255);
      Text(20,  66, "-100 dB", font_ptr, 0, 0, 0, 255, 255, 255);
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
      setPixel(610, i + 70, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(611, i + 70, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(612, i + 70, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(613, i + 70, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(614, i + 70, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      HorizLine(610, 470 + (5 * WaterfallBase), 5, 255, 255, 255);

    }
  }
  else if ((Range20dB == true) && (waterfall == false))  // 20 dB range spectrum
  {
    snprintf(caption, 15, "%d dB", BaseLine20dB + 20 - BaselineShift);
    Text(30, 462, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    snprintf(caption, 15, "%d dB", BaseLine20dB + 15 - BaselineShift);
    Text(30, 366, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    snprintf(caption, 15, "%d dB", BaseLine20dB + 10 - BaselineShift);
    Text(30, 266, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    snprintf(caption, 15, "%d dB", BaseLine20dB + 5 - BaselineShift);
    Text(30, 166, caption, font_ptr, 0, 0, 0, 255, 255, 255);
    snprintf(caption, 15, "%d dB", BaseLine20dB - BaselineShift);
    Text(30,  66, caption, font_ptr, 0, 0, 0, 255, 255, 255);
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
      setPixel(610, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(611, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(612, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(613, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      setPixel(614, 70 + i, waterfall_map(pixel_brightness).Red, waterfall_map(pixel_brightness).Green, waterfall_map(pixel_brightness).Blue);
      HorizLine(610, 470 + (5 * WaterfallBase) / 4, 5, 255, 255, 255);  // White bar at bottom of gradient line
      HorizLine(610, 470 + (5 * WaterfallBase) / 4 + (WaterfallRange * 5) /4, 5, 0, 0, 0);  // Black bar at top of gradient line
      HorizLine(610, 471 + (5 * WaterfallBase) / 4 + (WaterfallRange * 5) /4, 5, 0, 0, 0);  // Black bar at top of gradient line
    }
  }
}

void DrawSettings()
{
  const font_t *font_ptr = &font_dejavu_sans_18;   // 18pt
  char DisplayText[63];
  float ParamAsFloat;
  int line1y = 40;
  int line2y = 15;
  int titley = 5;
  int32_t negfreq;

  // Clear the previous text first
  rectangle(100, 0, 505, 64, 0, 0, 0);

  // Left hand x Axis Label
  if (span / 2 > CentreFreq)
  {
    negfreq = CentreFreq - span / 2;
    snprintf(DisplayText, 63, "%5.2f MHz", (float)negfreq / 1000000.0);
  }
  else
  {
    if (freqoffset >= 0)
    {
      ParamAsFloat = (float)(startfreq + freqoffset) / 1000000.0;
      snprintf(DisplayText, 63, "%5.2f MHz", ParamAsFloat);
    }
    else
    {
      ParamAsFloat = (float)(0 - freqoffset - (int64_t)stopfreq) / 1000000.0;
      snprintf(DisplayText, 63, "%5.2f MHz", ParamAsFloat);
    }
  }
  Text(100, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);
 
  // Centre x Axis Label
  if (freqoffset >= 0)
  {
    ParamAsFloat = (float)(CentreFreq + freqoffset) / 1000000.0;
    snprintf(DisplayText, 63, "%5.2f MHz", ParamAsFloat);
  }
  else
  {
    ParamAsFloat = (float)(0 - freqoffset - (int64_t)CentreFreq) / 1000000.0;
    snprintf(DisplayText, 63, "%5.2f MHz", ParamAsFloat);
  }
  Text(300, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);

  // Right hand x Axis Label
  if (freqoffset >= 0)
  {
    ParamAsFloat = (float)(stopfreq  + freqoffset)/ 1000000.0;
    snprintf(DisplayText, 63, "%5.2f MHz", ParamAsFloat);
  }
  else
  {
    ParamAsFloat = (float)(0 - freqoffset - (int64_t)startfreq)/ 1000000.0;
    snprintf(DisplayText, 63, "%5.2f MHz", ParamAsFloat);
  }
  Text(490, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);

  if (reflevel != 99)                           // valid
  {
    snprintf(DisplayText, 63, "Ref %d dBm", reflevel);
    Text(290, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);
  }

  if (rbw != 0)                                 // valid
  {
    snprintf(DisplayText, 63, "RBW %d kHz", rbw);
    Text(100, line2y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);
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
  int ypospix;              // ypos corrected for pixel map  
  int ymax;                 // inclusive upper limit of this plot
  int ymin;                 // inclusive lower limit of this plot
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

  if (ymax > 399)  // Limit the highest point to 399 (otherwise mix mode paints at pixel 400)
  {
    ymax = 399;
  }

  if (column[ypos] > 255)  // Limit the max brightness of the trace to 255
  {
    column[ypos] = 255;
  }

  // Draw the trace in the column

  for(ypos = ymin; ypos <= ymax; ypos++)
  {
    //ypospix = 409 - ypos;  //409 = 479 - 70
    ypospix = ypos + 70;

    setPixel(xpos, ypospix, column[ypos], column[ypos], 0);  
  }

  // Draw the background and grid (except in the active trace) to erase the previous scan

  if (xpos % 50 == 0)  // vertical int graticule
  {
    for(ypos = trace_baseline; ypos < ymin; ypos++)
    {
      //setPixel(xpos, 479 - (70 + ypos), 64, 64, 64);
      setPixel(xpos, ypos + 70, 64, 64, 64);
    }
    for(ypos = 399; ypos > ymax; ypos--)
    {
      //setPixel(xpos, 479 - (70 + ypos), 64, 64, 64);
      setPixel(xpos, ypos + 70, 64, 64, 64);
    }
  }
  else if ((xpos > 344) && (xpos < 356))  // centre vertical graticule marks
  {
    for(ypos = trace_baseline; ypos < ymin; ypos++)
    {
      //ypospix = 409 - ypos;  // 409 = 479 - 70
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
      //ypospix = 409 - ypos;  // 409 = 479 - 70
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
      //ypospix = 409 - ypos;  // 409 = 479 - 70
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
      //ypospix = 409 - ypos;  // 409 = 479 - 70
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


void cleanexit(int calling_exit_code)
{
  exit_code = calling_exit_code;
  app_exit = true;
  printf("Clean Exit Code %d\n", exit_code);
  usleep(1000000);
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);
  printf("scans = %d\n", tracecount);
  exit(exit_code);
}


static void terminate(int sig)
{
  app_exit = true;
  printf("Terminating in main\n");
  usleep(1000000);
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);

  printf("scans = %d\n", tracecount);
  exit(0);
}


int main(int argc, char **argv)
{
  int screenXmax, screenXmin;
  int screenYmax, screenYmin;
  int i;
  int pixel;
  int PeakValueZeroCounter = 0;
  uint64_t nextwebupdate = monotonic_ms() + 1000;
  uint64_t next_paint = monotonic_ms();
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

  strcpy(ProgramName, argv[0]);        // Used for web click control

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
  initScreen();
  CreateButtons();
  Define_Menus();
  ReadSavedParams();
  checkSnapIndex();
  redrawMenu();


  // Create Wait Button thread
  pthread_create (&thbutton, NULL, &WaitButtonEvent, NULL);

  // Check that an SDRPlay is accessible
  if (CheckSDRPlay() != 0)
  {
    MsgBox4("No SDRPlay detected", "Please ensure that it is", "plugged in to a USB2 socket", " ");
    usleep(2000000);
    wait_touch();     // Wait here till screen is touched
    MsgBox4(" ", " ", " ", " ");
    cleanexit(150);   //  
  }

  // SDR FFT Thread
  if(pthread_create(&sdrplay_fft_thread_obj, NULL, sdrplay_fft_thread, &app_exit))
  {
    fprintf(stderr, "Error creating %s pthread\n", "SDRPLAY_FFT");
    return 1;
  }
  pthread_setname_np(sdrplay_fft_thread_obj, "SDRPLAY_FFT");

  for(i = 1; i < 511; i++)
  {
    y3[i] = 1;
  }
  CalcSpan();

  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawTickMarks();
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  redrawMenu();     // Draw the buttons

  while(true)
  {
    //uint16_t sleep_count = 0;
    while (NewData == false)
    {
      usleep(100);
      //sleep_count++;
    }
    //printf("fft plot slept for %d periods of 100us\n", sleep_count);

    NewData = false;
    activescan = true;
    
    if (spectrum == true)
    {
      for (pixel = 8; pixel <= 506; pixel++)
      {
        //pthread_mutex_lock(&histogram);

        if (waterfall == false)
        {
          DrawTrace((pixel - 6), y3[pixel - 2], y3[pixel - 1], y3[pixel]);
        }
        else             //  mix
        {
          DrawTrace((pixel - 6), (y3[pixel - 2] / 4) + 300, (y3[pixel - 1] / 4) + 300, (y3[pixel] / 4) + 300);
        }

        //pthread_mutex_unlock(&histogram);

        //printf("pixel=%d, prev2=%d, prev1=%d, current=%d\n", pixel, y3[pixel - 2], y3[pixel - 1], y3[pixel]);

        if ((PeakPlot == true) && (waterfall == false))  // only works without waterfall!
        {
          // draw [pixel - 1] here based on PeakValue[pixel -1]
          if (y3[pixel - 1] > PeakValue[pixel -1])
          {
            PeakValue[pixel - 1] = y3[pixel - 1];
          }
          //setPixel(pixel + 93, 409 - PeakValue[pixel - 1], 255, 0, 63);
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
    }

    if (waterfall == true)
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
      if (check_time > next_paint)  // Paint the line with current peaks added in
      {
        //printf("time = %lld, next_paint was %lld, new next_paint = %lld\n", check_time, next_paint, next_paint + (wfalltimespan * 25) / 10);
        // Set the time for the next paint after this one
        if (spectrum == false)            // full screen waterfall
        {
          next_paint = next_paint + (wfalltimespan * 25) / 10;
        }
        else                              // only 300 (not 400) lines so paint slower
        {
          next_paint = next_paint + (wfalltimespan * 100) / 30;
        }

        // Make sure that we don't build up a time deficit
        if (next_paint < check_time)
        {
          next_paint = check_time + (wfalltimespan * 100) / 30;
        }
        paint_line = true;

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

      if(paint_line)
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
        //printf("waterfall render time %lld\n", monotonic_ms());
        //last_output = monotonic_ms();
        w_index++;
        if (w_index >= wfall_height)
        {
          w_index = 0;
        }
        activescan = false;
      }
    }

    publish();
    if (monotonic_ms() >= nextwebupdate)
    {
      UpdateWeb();
      nextwebupdate = nextwebupdate + 1000;  // Set next update for 1 second after the previous
      if (nextwebupdate < monotonic_ms())    // Check for backlog due to a freeze
      {
        nextwebupdate = monotonic_ms() + 1000;  //Set next update for 1 second ahead
      }
    }
    tracecount++;
    //printf("Tracecount = %d\n", tracecount);
  }

  printf("Waiting for SDR Play FFT Thread to exit..\n");
  pthread_join(sdrplay_fft_thread_obj, NULL);
  printf("Waiting for Screen Thread to exit..\n");
  pthread_join(screen_thread_obj, NULL);
  printf("All threads caught, exiting..\n");
  pthread_join(thbutton, NULL);
}

