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
#include <wiringPi.h> // Include new WiringPi library
#include <ads1115.h>
#include <mcp23017.h>

#include "lime.h"
#include "../common/font/font.h"
#include "../common/touch.h"
#include "../common/graphics.h"
#include "../common/timing.h"
#include "../common/buffer/buffer_circular.h"
#include "../common/ffunc.h"
#include "../common/mcp3002.h"
#include "../common/hardware.h"
#include "../common/temperature.h"
#include "../common/siggen.h"
#include "../common/utils.h"
#include "../common/sa_cal.h"
#include "sa_if.h"

#define PI 3.14159265358979323846

pthread_t thbutton;
pthread_t thwebclick;                  //  Listens for mouse clicks from web interface
pthread_t thtouchscreen;               //  listens to the touchscreen   
pthread_t thmouse;                     //  Listens to the mouse
static pthread_t lime_thread_obj;
pthread_t thPanelMonitor;  // ADC for set Frequency and reads panel switch positions
pthread_t thPowerMeasure;  // Synchronises and measures power in Lime Samples

pthread_mutex_t text_lock;

int fd = 0;
int wscreen, hscreen;
float scaleXvalue, scaleYvalue; // Coeff ratio from Screen/TouchArea

char StartApp[63];                     // Startup app on boot
char DisplayType[31];                  // Element14_7, touch2, hdmi, web
int VLCTransform = 0;                  // 0, 90, 180 or 270
int FBOrientation = 0;                 // 0, 90, 180 or 270
int HDMIResolution = 720;              // HDMI Resolution 720/1080
bool TouchInverted = false;            // true if screen mounted upside down
bool KeyLimePiEnabled = true;          // false for release

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

#define PATH_SCONFIG    "/home/pi/portsdown/configs/system_config.txt"
#define PATH_SAIFCONFIG "/home/pi/portsdown/configs/sa_if_config.txt"
#define PATH_SGCONFIG   "/home/pi/portsdown/configs/siggenconfig.txt"
#define PATH_SGSTATE    "/home/pi/tmp/siggen_state.txt"

#define MAX_BUTTON 675
int IndexButtonInArray=0;
button_t ButtonArray[MAX_BUTTON];
int CurrentMenu;
int CallingMenu;
char KeyboardReturn[64];

bool rotate_display = true;

int Inversed = 0;
int  scaledX, scaledY;
char DisplayType[31] = "Element14_7";
int wbuttonsize = 100;
int hbuttonsize = 50;
int rawX, rawY;
int FinishedButton = 0;
int i;
bool freeze = false;
bool frozen = false;
bool normalised = false;
bool normalising = false;
bool activescan = false;
bool finishednormalising = false;
int y[501];               // Actual displayed values on the chart
int norm[501];            // Normalisation corrections
int scaledadresult[501];  // Sensed AD Result
int normleveloffset = 300;

int DispFreq;
int normlevel = -20;
int normlabel = 0;
char PlotTitle[63] = "-";
bool ContScan = false;
bool ModeChanged = true;
bool PortsdownExitRequested = false;
bool app_exit = false;

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

bool Range20dB = false;
int BaseLine20dB = -80;
int8_t BaselineShift = 0;
int ScaleShift = 0;
int NormScaleCorrn = 0;

int WaterfallBase;
int WaterfallRange;
uint16_t wfalltimespan = 0;
char TraceType[10] = "single"; // single, average or peak

bool ShowStatusPage = false;

// Parameters read from Panel

uint8_t band = 1;                        // Band 1 to 8
uint32_t span = 100;                     // Span in integer MHz
uint8_t rawbw = 1;                       // Bandwidth switch position 1 - 12
uint32_t filter_bandwidth = 1000000;     // Bandwidth in integer Hz
float SDRsamplerate = 1024000;           // Hz
int32_t atten = 0;                       // Input attenuator, 0 through -80 dB
int32_t IFatten = 0;                     // IF attenuator, 0 through -30 dB
uint32_t tperdiv;                        // Scan time per division in mS (1 - 5000)
uint8_t samode = 1;                      // Mode selected on front panel
bool automode = true;                    // set to false to force a mode (persistent)
uint8_t opmode = 0;                      // Selected output mode. 0 = TG, 1 = Express, 2 = Lime

uint64_t volatile sample_read_time;
uint64_t volatile smoothed_sample_read_time;
bool peak = false;
bool volatile buffer1_updated;
bool volatile buffer2_updated;

float buffer1[4096];
float buffer2[4096];

float amp_in[4096];


// Define LO Frequency at Ends of Range
int LowFreqVolts = -31363;
float LowFreqLO = 2003.69;
int HiFreqVolts = 31700;
float HiFreqLO = 4576.31;

bool samodeauto = true;                  // enables front panel control of which running app
bool LimeStarted = false;                // delays checking mode until Lime is started so that it can shut down cleanly

bool webcontrol = false;   // Enables webcontrol

int tracecount = 0;  // Used for speed testing
int exit_code;

int yscalenum = 18;       // Numerator for Y scaling fraction
int yscaleden = 30;       // Denominator for Y scaling fraction
int yshift    = 5;        // Vertical shift (pixels) for Y

//int xmin = 30;            // Trigger value for start of scan
int xmin = 90;            // Trigger value for start of scan
int xscalenum = 71;       // Numerator for X scaling fraction was 26
int xscaleden = 40;       // Denominator for X scaling fraction

bool NewBW = true;
bool NewGain = false;
bool NewSpan = false;
bool NewCal  = false;
float gain = 0.90;
uint32_t limegain = 90;
float smoothing_factor;
bool FirstStatusCall = false;

float freq_power = 0;  // range 0.0 to 399.0


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
bool touchscreen_present = false;

bool scanstart = false;

pixelvt_t pixelbuff[1023];
uint64_t now;
uint64_t now2;
int delaypixels = 45;      // Set by scan speed and bandwidth switches to center display
uint64_t show_gain_until;

float FreqGainCal = 0;

int level = 0;
int SG_OutputLevel = 0;
uint64_t DisplayFreq = 437000000;
bool siggen_on = false;
bool ModOn = false;
bool firmware_loaded = false;
bool firmware_loading = false;

// Menus
// 1 Main
// 2 Markers
// 3 Config
// 4 System
// 5 Was Mode
// 6
// 7 Settings
// 14 Sig gen
// 41 Keyboard

///////////////////////////////////////////// FUNCTION PROTOTYPES ///////////////////////////////

void GetConfigParam(char *, char *, char *);
void SetConfigParam(char *, char *, char *);
void ReadSavedParams();
void *WaitTouchscreenEvent(void * arg);
void *WaitMouseEvent(void * arg);
void *WebClickListener(void * arg);
void parseClickQuerystring(char *query_string, int *x_ptr, int *y_ptr);
FFUNC touchscreenClick(ffunc_session_t * session);
void take_snap();
void snap_from_menu();
void do_snapcheck();
int IsImageToBeChanged(int x, int y);
int CheckLimeConnect();
void UpdateWeb();
void Keyboard(char RequestText[63], char InitText[63], int MaxLength);
int openTouchScreen(int);
int getTouchScreenDetails(int *screenXmin, int *screenXmax,int *screenYmin,int *screenYmax);
void TransformTouchMap(int x, int y);
int IsButtonPushed(int NbButton, int x, int y);
int InitialiseButtons();
int AddButton(int x, int y, int w, int h);
int ButtonNumber(int MenuIndex, int Button);
int CreateButton(int MenuIndex, int ButtonPosition);
int AddButtonStatus(int ButtonIndex, char *Text, color_t *Color);
void AmendButtonStatus(int ButtonIndex, int ButtonStatusIndex, char *Text, color_t *Color);
void DrawButton(int ButtonIndex);
void SetButtonStatus(int ButtonIndex, int Status);
int GetButtonStatus(int ButtonIndex);
int getTouchSampleThread(int *rawX, int *rawY, int *rawPressure);
int getTouchSample(int *rawX, int *rawY, int *rawPressure);
void UpdateWindow();
void wait_touch();
void CalculateMarkers();
void CheckForGainChange(int rawX, int rawY);
void SetNormLevel(int button);
void Normalise();
float calcFreqGainCal(int pixelFreq);
void control_SigGen(int button);
void ChangeLabel(int button);
void *WaitButtonEvent(void * arg);
void Define_Menu1();
void Start_Highlights_Menu1();
void Define_Menu2();
void Start_Highlights_Menu2();
void Define_Menu3();
void Define_Menu4();
void Start_Highlights_Menu4();
void Define_Menu5();
void Start_Highlights_Menu5();
void Define_Menu7();
void Start_Highlights_Menu7();
void Define_Menu14();
void Start_Highlights_Menu14();
void Define_Menu41();
void DrawEmptyScreen();
void DrawYaxisLabels();
void DrawSettings();
void DrawTrace(int xoffset, int prev2, int prev1, int current);
int constrainY(int y);
void *PanelMonitor_thread(void *arg);
void cleanexit(int calling_exit_code);
static void terminate(int dummy);


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


void ReadSavedParams()
{
  char response[63];

  strcpy(response, "-20");  // this is the default level for the normalised response
  GetConfigParam(PATH_SAIFCONFIG, "normlevel", response);
  normlevel = atoi(response);

  strcpy(response, "0");  // this is the default label for the normalised response
  GetConfigParam(PATH_SAIFCONFIG, "normlabel", response);
  normlabel = atoi(response);

  strcpy(PlotTitle, "-");  // this is the "do not display" response
  GetConfigParam(PATH_SAIFCONFIG, "title", PlotTitle);

  char PixelMode[63] = "average";
  GetConfigParam(PATH_SAIFCONFIG, "pixelmode", PixelMode);
  if (strcmp(PixelMode, "average") == 0)
  {
    peak = false;
  }
  else if (strcmp(PixelMode, "peak") == 0)
  {
    peak = true;
  }

  strcpy(response, "0");
  GetConfigParam(PATH_SAIFCONFIG, "baselineshift", response);
  BaselineShift = atoi(response);

  strcpy(response, "0");
  GetConfigParam(PATH_SAIFCONFIG, "scaleshift", response);
  ScaleShift = atoi(response);

  GetConfigParam(PATH_SAIFCONFIG, "limegain", response);
  limegain = atoi(response);
  gain = ((float)limegain) / 100.0;

  // System Config File

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "startup", response);
  strcpy(StartApp, response);

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "display", response);
  strcpy (DisplayType, response);

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "vlctransform", response);
  VLCTransform = atoi(response);

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "fborientation", response);
  FBOrientation = atoi(response);

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_SCONFIG, "hdmiresolution", response);
  HDMIResolution = atoi(response);

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

  strcpy(response, "-31363");
  GetConfigParam(PATH_SAIFCONFIG, "lowfreqvolts", response);
  LowFreqVolts = atoi(response);

  strcpy(response, "2003.69");
  GetConfigParam(PATH_SAIFCONFIG, "lowfreqlo", response);
  LowFreqLO = atof(response);

  strcpy(response, "31700");
  GetConfigParam(PATH_SAIFCONFIG, "hifreqvolts", response);
  HiFreqVolts = atoi(response);

  strcpy(response, "4576.31");
  GetConfigParam(PATH_SAIFCONFIG, "hifreqlo", response);
  HiFreqLO = atof(response);

  GetConfigParam(PATH_SAIFCONFIG, "samodesel", response);
  if (strcmp(response, "auto") == 0)
  {
    samodeauto = true;
  }
  else
  {
    samodeauto = false;
  }

  // SigGen parameters

  strcpy(response, "437000000");
  GetConfigParam(PATH_SGCONFIG, "freq", response);
  DisplayFreq = strtoll(response, 0, 0);
  if (DisplayFreq > 2450000000)
  {
    DisplayFreq = 2450000000;
  }
  if (DisplayFreq < 70000000)
  {
    DisplayFreq = 70000000;
  }

  strcpy(response, "0");
  GetConfigParam(PATH_SGCONFIG, "level", response);
  level = atoi(response);
  if (level < 0)
  {
    level = 0;
  }
  if (level > 42)
  {
    level  = 42;
  }
  SG_OutputLevel = CalcOPLevel(DisplayFreq, level, ModOn);
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
  bool left_button_action = false;
  FILE *fp;
  char response[255];
  char mouse_device[255] = "";

  // Look up the mouse event entry
  fp = popen("echo \"0\" | sudo evemu-describe 2>&1 | grep '/dev/input/event' | grep -e 'Mouse' -e 'mouse'", "r");
  if (fp == NULL)
  {
    printf("Failed to run command\n" );
    exit(1);
  }

  // crop the entry to the event address
  while (fgets(response, 65, fp) != NULL)
  {
    if (response[17] == ':')  // event 0 - 9
    {
      strcpyn(mouse_device, response, 17);
    }
    else if (response[18] == ':')  // event 10 - 99
    {
      strcpyn(mouse_device, response, 18);
    }
  }

  // Close the file command
  pclose(fp);

  // Open the mouse device
  if ((fd = open(mouse_device, O_RDONLY)) < 0)
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
        //printf("value %d, type %d, code %d, x_pos %d, y_pos %d\n",ev.value,ev.type,ev.code, x, y);
        mouse_x = x;
        mouse_y = y;
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
        //printf("value %d, type %d, code %d, x_pos %d, y_pos %d\n",ev.value,ev.type,ev.code, x, y);
//        while (image_complete == false)  // Wait for menu to be drawn
//        {
//          usleep(1000);
//        }
        mouse_x = x;
        mouse_y = y;
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
        scaledX = mouse_x;
        scaledY = mouse_y;
        MouseClickForAction = true;
        printf("scaledX = %d, scaledY = %d\n", scaledX, scaledY);
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

  fb2png(); // saves snap to /home/pi/tmp/snapshot.png
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

    if (getTouchSample(&rawX, &rawY, &rawPressure)==0) continue;

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

  TransformTouchMap(x, y);       // Sorts out orientation and approx scaling of the touch map

  if (scaledX <= wscreen / 4)
  {
    return -1;
  }
  else if (scaledX >= 3 * wscreen / 4)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}


/***************************************************************************//**
 * @brief Checks whether a LimeSDR is connected
 *
 * @param 
 *
 * @return 0 if present, 1 if absent (or V1)
*******************************************************************************/

int CheckLimeConnect()
{
  FILE *fp;
  char response[255];
  int responseint = 1;

return 0;

  /* Open the command for reading. */
  fp = popen("LimeUtil --make | grep -q 'LimeSDR' ; echo $?", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(response, 7, fp) != NULL)
  {
    responseint = atoi(response);
  }

  /* close */
  pclose(fp);
  return responseint;
}


void UpdateWeb()
{
  // Called after any screen update to update the web page if required.

  if(webcontrol == true)
  {
    system("/home/pi/portsdown/scripts/single_screen_grab_for_web.sh &");
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

  int shiftX, shiftY;
  double factorX, factorY;

  // Adjust registration of touchscreen for Waveshare
  shiftX=30; // move touch sensitive position left (-) or right (+).  Screen is 700 wide
  shiftY=-5; // move touch sensitive positions up (-) or down (+).  Screen is 480 high

  factorX=-0.4;  // expand (+) or contract (-) horizontal button space from RHS. Screen is 5.6875 wide
  factorY=-0.3;  // expand or contract vertical button space.  Screen is 8.53125 high

  // Switch axes for normal and waveshare displays
  if(Inversed==0) // Tontec35 or Element14_7
  {
    scaledX = x/scaleXvalue;
    scaledY = hscreen-y/scaleYvalue;
  }
  else //Waveshare (inversed)
  {
    scaledX = shiftX+wscreen-y/(scaleXvalue+factorX);

    if(strcmp(DisplayType, "Waveshare4") != 0) //Check for Waveshare 4 inch
    {
      scaledY = shiftY+hscreen-x/(scaleYvalue+factorY);
    }
    else  // Waveshare 4 inch display so flip vertical axis
    {
      scaledY = shiftY+x/(scaleYvalue+factorY); // Vertical flip for 4 inch screen
    }
  }
}


int IsButtonPushed(int NbButton, int x, int y)
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


int IsMenuButtonPushed(int x, int y)
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


int AddButton(int x, int y, int w, int h)
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
  // Returns the Button Number (0 - 350) from the Menu number and the button position
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
  // Provide Menu number (int 1 - 46), Button Position (0 bottom left, 23 top right)
  // return button number

  // Menus 1 - 20 are classic 15-button menus
  // Menus 21 - 40 are undefined
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

  if ((MenuIndex != 41) && (MenuIndex != 2) && (MenuIndex != 14))   // All except keyboard, Markers, SigGen
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
  else if ((MenuIndex == 2) || (MenuIndex == 14))  // Marker or SigGen Menu
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
    if (ButtonPosition == 5) // Left hand or down arrow
    {
      x = normal_xpos;  
      y = 480 - (5 * 60);
      w = 50;
      h = 50;
    }
    if (ButtonPosition == 6) // Right hand or up arrow
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


int AddButtonStatus(int ButtonIndex, char *Text, color_t *Color)
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


void SetButtonStatus(int ButtonIndex, int Status)
{
  button_t *Button=&(ButtonArray[ButtonIndex]);
  Button->NoStatus=Status;
}


int GetButtonStatus(int ButtonIndex)
{
  button_t *Button=&(ButtonArray[ButtonIndex]);
  return Button->NoStatus;
}


int getTouchSampleOLD(int *rawX, int *rawY, int *rawPressure)
{
	int i;
        /* how many bytes were read */
        size_t rb;
        /* the events (up to 64 at once) */
        struct input_event ev[64];
	//static int Last_event=0; //not used?
	rb=read(fd,ev,sizeof(struct input_event)*64);
	*rawX=-1;*rawY=-1;
	int StartTouch=0;
        for (i = 0;  i <  (rb / sizeof(struct input_event)); i++){
              if (ev[i].type ==  EV_SYN)
		{
                         //printf("Event type is %s%s%s = Start of New Event\n",KYEL,events[ev[i].type],KWHT);
		}
                else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
		{
			StartTouch=1;
                        //printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s1%s = Touch Starting\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
		}
                else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
		{
			//StartTouch=0;
			//printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s0%s = Touch Finished\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
		}
                else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0){
                        //printf("Event type is %s%s%s & Event code is %sX(0)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawX = ev[i].value;
		}
                else if (ev[i].type == EV_ABS  && ev[i].code == 1 && ev[i].value > 0){
                        //printf("Event type is %s%s%s & Event code is %sY(1)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawY = ev[i].value;
		}
                else if (ev[i].type == EV_ABS  && ev[i].code == 24 && ev[i].value > 0){
                        //printf("Event type is %s%s%s & Event code is %sPressure(24)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawPressure = ev[i].value;
		}
		if((*rawX!=-1)&&(*rawY!=-1)&&(StartTouch==1))
		{
			/*if(Last_event-mymillis()>500)
			{
				Last_event=mymillis();
				return 1;
			}*/
			//StartTouch=0;
			return 1;
		}

	}
	return 0;
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
  int markerf;
  int startfreq;
  int stopfreq;


  switch (markermode)
  {
    case 2:  // peak
      maxy = 0;
      //for (i = 50; i < 450; i++)
      for (i = 50; i < 425; i++)  // for limited span
      {
         if((y[i] + 1) > maxy)
         {
           maxy = y[i] + 1;
           xformaxy = i;
         }
      }
    break;

    case 3:  // null
      maxy = 400;
      for (i = 50; i < 425; i++)   // for limited span
      {
         if((y[i] + 1) < maxy)
         {
           maxy = y[i] + 1;
           xformaxy = i;
         }
      }
    break;

    case 4:  // manual
      maxy = 0;

      for (i = (manualmarkerx - 5); i < (manualmarkerx + 6); i++)
      {
         if((y[i] + 1) > maxy)
         {
           maxy = y[i] + 1;
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

  // Blank the Menu title
  rectangle(620, 420, 160, 60, 0, 0, 0);

  // Calculate level and display  
  if (normalised == false)
  {
    if ((band != 7) && (band != 8))   // bands 1 to 6 using attenuator
    {
      if (Range20dB == false)
      {
        markerlev = ((float)(410 - markery) / 5.0) - 150.0 - (float)(ScaleShift + atten);  // in dB for a 0 to -80 screen
      }
      else
      {
        markerlev = ((float)(410 - markery) / 20.0) + (float)(BaseLine20dB - atten);  // in dB for a BaseLine20dB to 20 above screen
      }
    }
    else            // external mixer bands without attenuator, so atten  = 0
    {
      if (Range20dB == false)
      {
        markerlev = ((float)(410 - markery) / 5.0) - 150.0 - (float)(ScaleShift);  // in dB for a 0 to -80 screen
      }
      else
      {
        markerlev = ((float)(410 - markery) / 20.0) + (float)(BaseLine20dB);  // in dB for a BaseLine20dB to 20 above screen
      }
    }
    snprintf(markerlevel, 14, "Mkr %0.1f dBm", markerlev);
  }
  else
  {
    markerlev = ((float)(410 - markery) / 5.0) - 150.0 + (float)(normlabel - normlevel - ScaleShift + NormScaleCorrn);  // in dB
    snprintf(markerlevel, 14, "Mkr %0.1f dB", markerlev);
  }
  Text(640, 450, markerlevel, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);

  // Calculate marker frequency
  startfreq = DispFreq - (span / 2);
  stopfreq  = DispFreq + (span / 2);
  markerf = ((markerx - 100) * (stopfreq - startfreq)) / 500.0 + startfreq; 
  snprintf(markerfreq, 14, "%d MHz", markerf);
  Text(640, 425, markerfreq, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);

  MarkerRefresh = false;  // Unlock screen writes

  publish();
}


void CheckForGainChange(int x, int y)
{
  char ValueToSave[63];

  if (x > 100)                         // out of zone
  {
    return;
  }
  if ((y > 260) && (y < 310))          // Decrement gain
  {
    freeze = true;
    while(! frozen)
    {
      usleep(10);                      // wait till the end of the scan
    }
    printf ("Decrement gain\n");
    switch (limegain)
    {
      case 100:
        limegain = 90;
        break;
      case 90:
        limegain = 70;
        break;
      case 70:
        limegain = 50;
        break;
      case 50:
        limegain = 30;
        break;
    }
    show_gain_until = monotonic_ms() + 5000;  // Show the gain instead of bandwidth for 5 seconds
  }
  else if ((y > 110) && (y < 160))      // Increment gain
  {
    freeze = true;
    while(! frozen)
    {
      usleep(10);                      // wait till the end of the scan
    }
    printf ("Increment gain\n");
    switch (limegain)
    {
      case 90:
        limegain = 100;
        break;
      case 70:
        limegain = 90;
        break;
      case 50:
        limegain = 70;
        break;
      case 30:
        limegain = 50;
        break;
    }
    show_gain_until = monotonic_ms() + 5000;  // Show the gain instead of bandwidth for 5 seconds
  }
  else if ((y > 360) && (y < 410))          // Show lower
  {
    printf ("Show lower\n");
    if (Range20dB == false)
    {
      ScaleShift = ScaleShift - 10;
      snprintf(ValueToSave, 63, "%d", ScaleShift);
      SetConfigParam(PATH_SAIFCONFIG, "scaleshift", ValueToSave);
      printf("ScaleShift set to %d \n", ScaleShift);
    }
    else
    {
      BaseLine20dB = BaseLine20dB + 5;
      DrawYaxisLabels();
      snprintf(ValueToSave, 8, "%d", BaseLine20dB);
      SetConfigParam(PATH_SAIFCONFIG, "baseline20db", ValueToSave);
    }
    DrawYaxisLabels();
    return;
  }
  else if ((y > 10) && (y < 60))      // Show higher
  {
    printf ("Show higher\n");
    if (Range20dB == false)
    {
      ScaleShift = ScaleShift + 10;
      snprintf(ValueToSave, 63, "%d", ScaleShift);
      SetConfigParam(PATH_SAIFCONFIG, "scaleshift", ValueToSave);
      printf("ScaleShift set to %d \n", ScaleShift);
    }
    else
    {
      BaseLine20dB = BaseLine20dB - 5;
      DrawYaxisLabels();
      snprintf(ValueToSave, 8, "%d", BaseLine20dB);
      SetConfigParam(PATH_SAIFCONFIG, "baseline20db", ValueToSave);
    }
    DrawYaxisLabels();
    return;
  }
  else                                 // Out of zone
  {
    return;
  }

  // Store the new gain
  snprintf(ValueToSave, 63, "%d", limegain);
  SetConfigParam(PATH_SAIFCONFIG, "limegain", ValueToSave);
  printf("limegain set to %d \n", limegain);

  // Trigger the gain change
  gain = ((float)limegain) / 100.0;
  NewGain = true;

  DrawSettings();                    // Refresh the gain

  freeze = false;
}


void SetNormLevel(int button)
{
  char RequestText[64];
  char InitText[63];
  bool IsValid = false;

  char ValueToSave[63];
  
  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 4:                                            // Select normalised y Pos
      // Define request string
      strcpy(RequestText, "Enter new Normalised Y pos (Range 0 to -80)");

      // Define initial value 
      snprintf(InitText, 10, "%d", normlevel);

      while (IsValid == false)
      {
        Keyboard(RequestText, InitText, 4);
        if (strlen(KeyboardReturn) == 0)
        {
          IsValid = false;
        }
        else
        {
          normlevel = atoi(KeyboardReturn);
          if ((normlevel >= -80) && (normlevel <= 0))
          {
            IsValid = true;
          }
          else
          {
            IsValid = false;
          }
        }
      }
      snprintf(ValueToSave, 63, "%d", normlevel);
      SetConfigParam(PATH_SAIFCONFIG, "normlevel", ValueToSave);
      printf("Normalisation Level set to: %d\n", normlevel);
      break;
    case 5:                                            // Select normalised y Pos
      // Define request string
      strcpy(RequestText, "Enter new Normalised Y label (Range 100 to -100)");

      // Define initial value 
      snprintf(InitText, 10, "%d", normlabel);

      while (IsValid == false)
      {
        Keyboard(RequestText, InitText, 4);
        if (strlen(KeyboardReturn) == 0)
        {
          IsValid = false;
        }
        else
        {
          normlabel = atoi(KeyboardReturn);
          if ((normlabel >= -100) && (normlabel <= 100))
          {
            IsValid = true;
          }
          else
          {
            IsValid = false;
          }
        }
      }
      snprintf(ValueToSave, 63, "%d", normlabel);
      SetConfigParam(PATH_SAIFCONFIG, "normlabel", ValueToSave);
      printf("Normalisation Label set to: %d\n", normlabel);
      break;
  }

  // Tidy up, paint around the screen and then unfreeze
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop, Ref level and Title
  freeze = false;
}


void Normalise()
{
  normleveloffset = 400 + normlevel * 5;  // 0 = screen bottom, 400 = screen top

  finishednormalising = false;            // Make sure that it is not normalising (ie measuring the baseline)

  NormScaleCorrn = 70 + ScaleShift;       // take account of scaleshift setting

  // Indicate that it is normalising (will get reset on return from function)
  SetButtonStatus(ButtonNumber(CurrentMenu, 4), 2);
  Start_Highlights_Menu1();
  Start_Highlights_Menu7();
  UpdateWindow();

  while (activescan == true)              // Wait for the end of the current scan
  {
    usleep(10);
  }
  
  normalising = true;                     // Set the flag for it to measure the baseline

  while (finishednormalising == false)    // Wait for the end of the measurement scan
  {
    usleep(10);
  }

  normalising = false;                    // Stop it measuring

  normalised = true;                      // and start it calculating for subsequent scans
}


float calcFreqGainCal(int pixelFreq)
{
  int i;
  double factor;
  float gainCorrection = 0;

  // return 0;  uncomment for calibration

  switch ((int)(gain * 100.1))
  {
    case 30:
      gainCorrection = 26.4;
      break;
    case 50:
      gainCorrection = 13.0;
      break;
    case 70:
      gainCorrection = -2.0;
      break;
    case 90:
      gainCorrection = -13.4;
      break;
    case 100:
      gainCorrection = -20.8;
      break;
  }

  //printf("Gain before frequency correction is %f\n", gainCorrection);

  for (i = 0; i < 9; i++)  // only 9 intervals for 10 values
  {

    switch (band)
    {
      case 1:
        if ((pixelFreq >= (int)freqCalPoints1[i]) && (pixelFreq < (int)freqCalPoints1[i + 1]))
        {
          factor = ((float)pixelFreq - freqCalPoints1[i]) / (freqCalPoints1[i + 1] - freqCalPoints1[i]);
          gainCorrection = gainCorrection + (freqCalGain1[i] + (float)(factor) * (freqCalGain1[i + 1] - freqCalGain1[i]));
          return gainCorrection; 
        }
        break;
      case 2:
        if ((pixelFreq >= (int)freqCalPoints2[i]) && (pixelFreq < (int)freqCalPoints2[i + 1]))
        {
          factor = ((float)pixelFreq - freqCalPoints2[i]) / (freqCalPoints2[i + 1] - freqCalPoints2[i]);
          gainCorrection = gainCorrection + (freqCalGain2[i] + (float)(factor) * (freqCalGain2[i + 1] - freqCalGain2[i]));
          return gainCorrection; 
        }
        break;
      case 3:
        if ((pixelFreq >= (int)freqCalPoints3[i]) && (pixelFreq < (int)freqCalPoints3[i + 1]))
        {
          factor = ((float)pixelFreq - freqCalPoints3[i]) / (freqCalPoints3[i + 1] - freqCalPoints3[i]);
          gainCorrection = gainCorrection + (freqCalGain3[i] + (float)(factor) * (freqCalGain3[i + 1] - freqCalGain3[i]));
          return gainCorrection; 
        }
        break;
      case 4:
        if ((pixelFreq >= (int)freqCalPoints4[i]) && (pixelFreq < (int)freqCalPoints4[i + 1]))
        {
          factor = ((float)pixelFreq - freqCalPoints4[i]) / (freqCalPoints4[i + 1] - freqCalPoints4[i]);
          gainCorrection = gainCorrection + (freqCalGain4[i] + (float)(factor) * (freqCalGain4[i + 1] - freqCalGain4[i]));
          return gainCorrection; 
        }
        break;
      case 5:
        if ((pixelFreq >= (int)freqCalPoints5[i]) && (pixelFreq < (int)freqCalPoints5[i + 1]))
        {
          factor = ((float)pixelFreq - freqCalPoints5[i]) / (freqCalPoints5[i + 1] - freqCalPoints5[i]);
          gainCorrection = gainCorrection + (freqCalGain5[i] + (float)(factor) * (freqCalGain5[i + 1] - freqCalGain5[i]));
          return gainCorrection; 
        }
        break;
      case 6:
        if ((pixelFreq >= (int)freqCalPoints6[i]) && (pixelFreq < (int)freqCalPoints6[i + 1]))
        {
          factor = ((float)pixelFreq - freqCalPoints6[i]) / (freqCalPoints6[i + 1] - freqCalPoints6[i]);
          gainCorrection = gainCorrection + (freqCalGain6[i] + (float)(factor) * (freqCalGain6[i + 1] - freqCalGain6[i]));
          return gainCorrection; 
        }
        break;
      case 7:
        if ((pixelFreq >= (int)freqCalPoints7[i]) && (pixelFreq < (int)freqCalPoints7[i + 1]))
        {
          factor = ((float)pixelFreq - freqCalPoints7[i]) / (freqCalPoints7[i + 1] - freqCalPoints7[i]);
          gainCorrection = gainCorrection + (freqCalGain7[i] + (float)(factor) * (freqCalGain7[i + 1] - freqCalGain7[i]));
          return gainCorrection; 
        }
        break;
      case 8:
        if ((pixelFreq >= (int)freqCalPoints8[i]) && (pixelFreq < (int)freqCalPoints8[i + 1]))
        {
          factor = ((float)pixelFreq - freqCalPoints8[i]) / (freqCalPoints8[i + 1] - freqCalPoints8[i]);
          gainCorrection = gainCorrection + (freqCalGain8[i] + (float)(factor) * (freqCalGain8[i + 1] - freqCalGain8[i]));
          return gainCorrection; 
        }
        break;
    }  
  }
  return gainCorrection;
}


void control_SigGen(int button)
{
  char RequestText[63];
  char InitText[63];
  bool IsValid = false;
  char ValueToSave[63];
  char Value[63];

  switch (button)
  {
    case 0:                                                                         // On entry to Sig Gen menu

      if (CheckExpressConnect() != 0)        // DATV Express not detected, so show error message
      {
        // Stop the scan at the end of the current one and wait for it to stop
        freeze = true;
        while(! frozen)
        {
          usleep(10);                                   // wait till the end of the scan
        }

        MsgBox4("DATV Express Source not detected", "Check that the Express output is selected", " ", "Touch screen to continue");
        wait_touch();
        // Tidy up, paint around the screen and then unfreeze
        clearScreen(0, 0, 0);
        DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
        DrawYaxisLabels();  // dB calibration on LHS
        DrawSettings();     // Draw Title
        freeze = false;
      }

      firmware_loading = false;
      firmware_loaded = false;

      // Check if DATV Express Server running.  If not, start it (which loads the firmware)
      if (CheckExpressRunning() != 0)        // DATV Express Server not running
      {
        printf("DATV Express Server not running\n");
        // Amend state
        ModOn = false;
        siggen_on = false;
        SetConfigParam(PATH_SGSTATE, "state", "off");
        SetConfigParam(PATH_SGSTATE, "mod", "off");

        // Change menu to say firmware laoding
        firmware_loading = true;
        firmware_loaded = false;
        Start_Highlights_Menu14();
        UpdateWindow();
        
        if (StartExpressServer() != 0)        // DATV Express Server Error
        {
          // Stop the scan at the end of the current one and wait for it to stop
          freeze = true;
          while(! frozen)
          {
            usleep(10);                                   // wait till the end of the scan
          }

          MsgBox4("DATV Express Firmware failed to load", "Investigation required", " ", "Touch screen to continue");
          wait_touch();

          // Tidy up, paint around the screen and then unfreeze
          clearScreen(0, 0, 0);
          DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
          DrawYaxisLabels();  // dB calibration on LHS
          DrawSettings();     // Draw Title
          freeze = false;
        }
        else
        {
          firmware_loading = false;
          firmware_loaded = true;
        }
      }
      else  // Server already running, so show firmware as loaded
      {
        printf("DATV Express Server is running\n");
        firmware_loading = false;
        firmware_loaded = true;

        // Now check stored state of SigGen
        strcpy(Value, "off");
        GetConfigParam(PATH_SGSTATE, "state", Value);

        if (strcmp(Value, "on") == 0)
        {
          siggen_on = true;
        }

        strcpy(Value, "off");
        GetConfigParam(PATH_SGSTATE, "mod", Value);

        if (strcmp(Value, "on") == 0)
        {
          ModOn = true;
        }
      }
      SG_OutputLevel = CalcOPLevel(DisplayFreq, level, ModOn);
      Start_Highlights_Menu14();
      UpdateWindow();

      break;

    case 1:                                                                                  // SigGen On/off
      if (siggen_on == false)
      {
        if (ModOn == false)
        {
          ExpressOn(DisplayFreq, level);
        }
        else
        {
          ExpressOnWithMod(DisplayFreq, level);
        }
        siggen_on = true;
        SetConfigParam(PATH_SGSTATE, "state", "on");
      }
      else
      {
        ExpressOff();
        siggen_on = false;
        SetConfigParam(PATH_SGSTATE, "state", "off");
      }
      break;

    case 2:                                                                               // Frequency
      // Define request string
      strcpy(RequestText, "Enter frequency in MHz");

      // Define initial value 
      snprintf(InitText, 15, "%g", ((float)(DisplayFreq)) / 1000000);

      freeze = true;
      while(! frozen)
      {
        usleep(10);                                   // wait till the end of the scan
      }

      while (IsValid == false)
      {
        Keyboard(RequestText, InitText, 10);
        if (strlen(KeyboardReturn) < 2)
        {
          IsValid = false;
        }
        else
        {
          DisplayFreq = (uint64_t)(1000000 * atof(KeyboardReturn));
          if ((DisplayFreq >= 70000000) && (DisplayFreq <= 2450000000))
          {
            IsValid = true;
          }
          else
          {
            IsValid = false;
          }
        }
      }
      // Tidy up, paint around the screen and then unfreeze
      clearScreen(0, 0, 0);
      DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
      DrawYaxisLabels();  // dB calibration on LHS
      DrawSettings();     // Draw Title
      freeze = false;

      ChangeExpressFreq(DisplayFreq);
      SG_OutputLevel = CalcOPLevel(DisplayFreq, level, ModOn);

      snprintf(ValueToSave, 63, "%ld", DisplayFreq);
      SetConfigParam(PATH_SGCONFIG, "freq", ValueToSave);
      printf("SigGen Frequency set to: %ld Hz\n", DisplayFreq);
      break;

    case 3:                                                                              // QPSK Mod On/off
      if (ModOn == false)
      {
        // turn on modulation
        ModOn = true;
        if (siggen_on == true)
        {
          ExpressModOn();
        }
        SetConfigParam(PATH_SGSTATE, "mod", "on");
      }
      else
      {
        // turn off modulation
        ModOn = false;
        if (siggen_on == true)
        {
          ExpressModOff();
        }
        SetConfigParam(PATH_SGSTATE, "mod", "off");
      }

      // calculate new real level
      SG_OutputLevel = CalcOPLevel(DisplayFreq, level, ModOn);
      break;
    //case 4:                                           // Does nothing, just shows output level

    case 5:                                                                                 // Reduce output
      level = level - 1;
      if (level < 0)
      {
        level = 0;
      }
      snprintf(ValueToSave, 63, "%d", level);
      SetConfigParam(PATH_SGCONFIG, "level", ValueToSave);
      printf("SigGen Level set to: %d\n", level);

      ChangeExpressLevel(level);

      // calculate new real level
      SG_OutputLevel = CalcOPLevel(DisplayFreq, level, ModOn);
      break;

    case 6:                                                                             // Increase output
      level = level + 1;
      if (level > 42)
      {
        level = 42;
      }
      snprintf(ValueToSave, 63, "%d", level);
      SetConfigParam(PATH_SGCONFIG, "level", ValueToSave);
      printf("SigGen Level set to: %d\n", level);

      ChangeExpressLevel(level);

      // calculate new real level
      SG_OutputLevel = CalcOPLevel(DisplayFreq, level, ModOn);
      break;

    case 7:                                                                              // Load Firmware
      siggen_on = false;
      ModOn = false;
      firmware_loading = true;
      firmware_loaded = false;
      StopExpressServer();
      Start_Highlights_Menu14();
      UpdateWindow();
      StartExpressServer();
      firmware_loaded = true;
      firmware_loading = false;
      break;
  }
}


void ChangeLabel(int button)
{

  char RequestText[64];
  char InitText[63];
  int rawPressure;
  
  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 2:                                                       // Show Parameters
      clearScreen(0, 0, 0);
      FirstStatusCall = true;
      ShowStatusPage = true;

      // Now wait for screen touch
      while(getTouchSample(&rawX, &rawY, &rawPressure)==0)
      {
        usleep(10);                                   // wait without burnout
      }
      ShowStatusPage = false;

      break;

    case 3:                                                       // Was Stop Freq
      break;
    case 4:                                                       // Was Ref Level
      break;
    case 5:                                                       // Was RBW
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

      Keyboard(RequestText, InitText, 40);
  
      if(strlen(KeyboardReturn) > 0)
      {
        strcpy(PlotTitle, KeyboardReturn);
      }
      else
      {
        strcpy(PlotTitle, "-");
      }
      SetConfigParam(PATH_SAIFCONFIG, "title", PlotTitle);
      printf("Plot Title set to: %s\n", KeyboardReturn);
      break;
    case 7:                                                       // Was Set Normalise Level
      break;

    case 16:                                                       // Was Power Meter Title
      break;

    case 12:                                                       // Was External Attenuator
      break;

    case 13:                                                       // Was Cal Factor
      break;
  }

  // Tidy up, paint around the screen and then unfreeze
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Draw Title

  freeze = false;
}


//////////////////////////////////////// DEAL WITH TOUCH EVENTS //////////////////////////////////////


void *WaitButtonEvent(void * arg)
{
  int  rawPressure;

  for (;;)
  {
    while(getTouchSample(&rawX, &rawY, &rawPressure)==0)
    {
      usleep(10);                                   // wait without burnout
    }

    printf("x=%d y=%d\n", rawX, rawY);

    CheckForGainChange(rawX, rawY);                 // Check for touch changing gain or level

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
        case 2:                                            // Settings Menu
          printf("Settings Menu 7 Requested\n");
          CurrentMenu = 7;
          Start_Highlights_Menu7();
          UpdateWindow();
          break;
        case 3:                                            // Markers
          printf("Markers Menu 2 Requested\n");
          CurrentMenu = 2;
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 4:                                            // Normalise
          if (normalised == false)
          {
            Normalise();
            SetButtonStatus(ButtonNumber(CurrentMenu, 4), 1);
            UpdateWindow();
            normalised = true;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
            UpdateWindow();
            normalised = false;
          }
          DrawYaxisLabels();
          break;
        case 5:                                            // Labels
          printf("Labels Menu 3 Requested\n");
          CurrentMenu=3;
          UpdateWindow();
          break;
        case 6:                                            // System
          printf("System Menu 4 Requested\n");
          CurrentMenu=4;
          Start_Highlights_Menu4();
          UpdateWindow();
          break;
        case 7:                                            // Exit to Portsdown
          if(PortsdownExitRequested)
          {
            freeze = true;
            usleep(100000);
            clearScreen(0, 0, 0);
            usleep(1000000);
            cleanexit(207);
          }
          else
          {
            PortsdownExitRequested = true;
            Start_Highlights_Menu1();
            UpdateWindow();
          }
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
          printf("Menu 1 Error\n");
      }
      continue;  // Completed Menu 1 action, go and wait for touch
    }

    if (CurrentMenu == 2)  //  Marker Menu
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
            markeron = true;
            markermode = i;
          }
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 3:                                            // Null
          if ((markeron == false) || (markermode != 3))
          {
            markeron = true;
            markermode = i;
          }
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 4:                                            // Manual
          if ((markeron == false) || (markermode != 4))
          {
            markeron = true;
            markermode = i;
          }
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 5:                                            // Left Arrow
          if(manualmarkerx > 10)
          {
            manualmarkerx = manualmarkerx - 10;
          }
          break;
        case 6:                                            // Right Arrow
          if(manualmarkerx < 490)
          {
            manualmarkerx = manualmarkerx + 10;
          }
          break;
        case 7:                                            // No Markers
          markeron = false;
          markermode = i;
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 8:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu = 1;
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

    if (CurrentMenu == 3)  // Config Menu
    {
      printf("Button Event %d, Entering Menu 3 Case Statement\n",i);
      CallingMenu = 3;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Select Mode Menu
          printf("Mode Menu 5 Requested\n");
          CurrentMenu = 5;
          Start_Highlights_Menu1();
          UpdateWindow();
          break;
        //case 3:                                            // 
        //case 4:                                            // 
        //case 5:                                            // 
        case 6:                                            // Plot Title
          ChangeLabel(i);
          UpdateWindow();          
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu = 1;
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
          printf("Menu 3 Error\n");
      }
      continue;  // Completed Menu 3 action, go and wait for touch
    }

    if (CurrentMenu == 4)  //  System Menu
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
          while (frozen == false)
          {
            usleep(10);
          }
          do_snapcheck();
          initScreen();
          DrawEmptyScreen();
          DrawYaxisLabels();  // dB calibration on LHS
          DrawSettings();     // Draw Title
          UpdateWindow();
          freeze = false;
          break;
        case 3:                                            // Show SA Status
          ChangeLabel(2);
          clearScreen(0, 0, 0);
          DrawEmptyScreen();
          DrawYaxisLabels();  // dB calibration on LHS
          DrawSettings();     // Draw Title
          UpdateWindow();
          break;
        case 4:                                            // Shutdown
          system("sudo shutdown now");
          break;
        case 5:                                            // Exit to Portsdown
          freeze = true;
          usleep(100000);
          clearScreen(0, 0, 0);
          UpdateWeb();
          usleep(1000000);
          cleanexit(207);
          break;
        case 6:                                            // Restart this App
          freeze = true;
          usleep(100000);
          clearScreen(0, 0, 0);
          usleep(1000000);
          cleanexit(170);
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu = 1;
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

    if (CurrentMenu == 5)  // Mode Menu (XY and Power)
    {
      printf("Button Event %d, Entering Menu 5 Case Statement\n",i);
      CallingMenu = 5;
      Start_Highlights_Menu5();
      UpdateWindow();
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Was Power Meter Mode
          break;
        case 3:                                            // XY Mode
          if (ContScan == true)
          {
            ContScan = false;
          }
          ModeChanged = true;
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 4:                                            // Continuous Scan Mode
          if (ContScan == false)
          {
            ContScan = true;
          }
          ModeChanged = true;
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 5:                                            // 
          break;
        case 6:                                            // 
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu = 1;
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

    if (CurrentMenu == 7)  // Settings Menu
    {
      printf("Button Event %d, Entering Menu 7 Case Statement\n",i);
      CallingMenu = 7;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Set Normalised Y position
          SetNormLevel(4);
          UpdateWindow();          
          break;
        case 3:                                            // Set Normalised Y Label
          SetNormLevel(5);
          UpdateWindow();          
          break;
        case 4:                                            // Normalise
          if (normalised == false)
          {
            Normalise();
            SetButtonStatus(ButtonNumber(CurrentMenu, 4), 1);
            UpdateWindow();
            normalised = true;
          }
          else
          {
            SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
            UpdateWindow();
            normalised = false;
          }
          DrawYaxisLabels();
          break;
        case 5:                                            // Select SigGen Menu
          printf("SigGen Menu 14 Requested\n");
          Start_Highlights_Menu14();
          CurrentMenu = 14;
          control_SigGen(0);  // Start-up actions for SigGen
          UpdateWindow();
          break;
        case 6:                                            // Toggle Peak or Avberage
          printf("Peak or Average toggle\n");
          if (peak == true)
          {
            peak = false;
            SetConfigParam(PATH_SAIFCONFIG, "pixelmode", "average");
          }
          else
          {
            peak = true;
            SetConfigParam(PATH_SAIFCONFIG, "pixelmode", "peak");
          }
          Start_Highlights_Menu7();
          UpdateWindow();
          break;
        case 7:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          Start_Highlights_Menu1();
          CurrentMenu = 1;
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
          printf("Menu 7 Error\n");
      }
      continue;  // Completed Menu 7 action, go and wait for touch
    }
    if (CurrentMenu == 14)  //  Sig Gen Menu
    {
      printf("Button Event %d, Entering Menu 14 Case Statement\n",i);
      CallingMenu = 14;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 1:                                            // SigGen On/off
        case 2:                                            // Frequency
        case 3:                                            // QPSK Mod On/off
        //case 4:                                           // Does nothing, just shows output level
        case 5:                                            // Reduce output
        case 6:                                            // Increase output
        case 7:                                            // Load Firmware
          control_SigGen(i);
          Start_Highlights_Menu14();
          UpdateWindow();
          break;
        case 8:                                            // Return to Main Menu
          printf("Main Menu 1 Requested\n");
          CurrentMenu = 1;
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
          printf("Menu 14 Error\n");
      }
      continue;  // Completed Menu 14 action, go and wait for touch
    }
  }
  return NULL;
}


/////////////////////////////////////////////// DEFINE THE BUTTONS ///////////////////////////////

void Define_Menu1()  // Main Menu
{
  int button = 0;

  button = CreateButton(1, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(1, 1);
  AddButtonStatus(button, "Main Menu", &Black);

  button = CreateButton(1, 2);
  AddButtonStatus(button, "Settings", &Blue);

  button = CreateButton(1, 3);
  AddButtonStatus(button, "Markers", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 4);
  AddButtonStatus(button, "Normalise", &Blue);
  AddButtonStatus(button, "Normalised", &Green);
  AddButtonStatus(button, "Normalising", &Red);

  button = CreateButton(1, 5);
  AddButtonStatus(button, "Config", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 6);
  AddButtonStatus(button, "System", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 7);
  AddButtonStatus(button, "Exit to^Portsdown", &DBlue);
  AddButtonStatus(button, "Exit to^Portsdown", &Red);

  button = CreateButton(1, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu1()
{
  if (PortsdownExitRequested)
  {
    SetButtonStatus(ButtonNumber(1, 7), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(1, 7), 0);
  }
}


void Define_Menu2()  //  Markers
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
  AddButtonStatus(button, "Null", &Blue);
  AddButtonStatus(button, "Null", &Green);

  button = CreateButton(2, 4);
  AddButtonStatus(button, "Manual", &Blue);
  AddButtonStatus(button, "Manual", &Green);

  button = CreateButton(2, 5);
  AddButtonStatus(button, "<-", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(2, 6);
  AddButtonStatus(button, "->", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(2, 7);
  AddButtonStatus(button, "Markers^Off", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(2, 8);
  AddButtonStatus(button, "Return to^Main Menu", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(2, 9);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu2()
{
  SetButtonStatus(ButtonNumber(2, 2), 0);
  SetButtonStatus(ButtonNumber(2, 3), 0);
  SetButtonStatus(ButtonNumber(2, 4), 0);

  if ((markeron == true) && (markermode == 2))
  {
    SetButtonStatus(ButtonNumber(2, 2), 1);
  }

  if ((markeron == true) && (markermode == 3))
  {
    SetButtonStatus(ButtonNumber(2, 3), 1);
  }

  if ((markeron == true) && (markermode == 4))
  {
    SetButtonStatus(ButtonNumber(2, 4), 1);
  }
}


void Define_Menu3()  // Config
{
  int button = 0;

  button = CreateButton(3, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(3, 1);
  AddButtonStatus(button, "Config^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 2);
  AddButtonStatus(button, "Mode", &Blue);

  //button = CreateButton(3, 3);
  //AddButtonStatus(button, "Stop^Freq", &Blue);
  //AddButtonStatus(button, " ", &Green);

  //button = CreateButton(3, 4);
  //AddButtonStatus(button, "Reference^Level", &Blue);
  //AddButtonStatus(button, " ", &Green);

  //button = CreateButton(3, 5);
  //AddButtonStatus(button, "Resolution^Bandwidth", &Blue);
  //AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 6);
  AddButtonStatus(button, "Title", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 7);
  AddButtonStatus(button, "Return to^Main Menu", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(3, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Define_Menu4()  // System
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
  AddButtonStatus(button, "Show SA^Status", &Blue);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(4, 4);
  AddButtonStatus(button, "Shutdown^System", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 5);
  AddButtonStatus(button, "Exit to^Portsdown", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 6);
  AddButtonStatus(button, "Re-start^App", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 7);
  AddButtonStatus(button, "Return to^Main Menu", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu4()
{
  // Nothing here
}


void Define_Menu5()  // Mode
{
  int button = 0;

  button = CreateButton(5, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(5, 1);
  AddButtonStatus(button, "Mode^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  //button = CreateButton(5, 2);
  //AddButtonStatus(button, "Power^Meter", &Blue);
  //AddButtonStatus(button, "Power^Meter", &Green);

  button = CreateButton(5, 7);
  AddButtonStatus(button, "Return to^Main Menu", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(5, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu5()
{
  // Nothing here
}


void Define_Menu7()  // Settings
{
  int button = 0;

  button = CreateButton(7, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(7, 1);
  AddButtonStatus(button, "Settings^Menu", &Black);

  button = CreateButton(7, 2);
  AddButtonStatus(button, "Normalised^Y Position", &Blue);

  button = CreateButton(7, 3);
  AddButtonStatus(button, "Normalised^Y Label", &Blue);

  button = CreateButton(7, 4);
  AddButtonStatus(button, "Normalise", &Blue);
  AddButtonStatus(button, "Normalised", &Green);
  AddButtonStatus(button, "Normalising", &Red);

  button = CreateButton(7, 5);
  AddButtonStatus(button, "Signal^Generator", &Blue);

  button = CreateButton(7, 6);
  AddButtonStatus(button, "Pixel Mode^Peak", &Blue);
  AddButtonStatus(button, "Pixel Mode^Average", &Blue);

  button = CreateButton(7, 7);
  AddButtonStatus(button, "Return to^Main Menu", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(7, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu7()
{
  if (peak == true)
  {
    SetButtonStatus(ButtonNumber(7, 6), 0);
  }
  else
  {
    SetButtonStatus(ButtonNumber(7, 6), 1);
  }
}


void Define_Menu14()  //  Sig Gen
{
  int button = 0;

  button = CreateButton(14, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(14, 1);
  AddButtonStatus(button, "Sig Gen^Off", &Black);
  AddButtonStatus(button, "Sig Gen^On", &Red);

  button = CreateButton(14, 2);
  AddButtonStatus(button, "Freq^ ", &Blue);

  button = CreateButton(14, 3);
  AddButtonStatus(button, "DVB-S Mod^Off", &Blue);
  AddButtonStatus(button, "DVB-S Mod^On", &Red);

  button = CreateButton(14, 4);
  AddButtonStatus(button, "Level^ ", &Blue);

  button = CreateButton(14, 5);
  AddButtonStatus(button, "Lev^down", &Blue);

  button = CreateButton(14, 6);
  AddButtonStatus(button, "Lev^up", &Blue);

  button = CreateButton(14, 7);
  AddButtonStatus(button, "Load^Firmware", &Blue);
  AddButtonStatus(button, "Loading^Firmware", &Red);
  AddButtonStatus(button, "Firmware^Loaded", &Green);

  button = CreateButton(14, 8);
  AddButtonStatus(button, "Return to^Main Menu", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(14, 9);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}

void Start_Highlights_Menu14()
{
  char button_text[31];
  if (siggen_on == true)
  {
    SetButtonStatus(ButtonNumber(14, 1), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(14, 1), 0);
  }

  snprintf(button_text, 30, "Freq^%0.3f", (float)(DisplayFreq) / 1000000);
  AmendButtonStatus(ButtonNumber(14, 2), 0, button_text, &Blue);

  if (ModOn == true)
  {
    SetButtonStatus(ButtonNumber(14, 3), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(14, 3), 0);
  }

  snprintf(button_text, 30, "Level^%0.1f dBm", (float)(SG_OutputLevel) / 10);
  AmendButtonStatus(ButtonNumber(14, 4), 0, button_text, &Blue);

  if ((firmware_loaded == false) && (firmware_loading == false))
  {
    SetButtonStatus(ButtonNumber(14, 7), 0);
  }
  else if ((firmware_loading == true) && (firmware_loaded == false))
  {
    SetButtonStatus(ButtonNumber(14, 7), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(14, 7), 2);
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


void DrawYaxisLabels()
{
  const font_t *font_ptr = &font_dejavu_sans_18;   // 18pt
  char caption[31];
  int i;

  // Clear the previous scale first
  rectangle(0, 63, 100, 417, 0, 0, 0);

  if (normalised == false)       // Not normalised
  {
    if ((band != 7) && (band != 8))   //bands that use the attenuator
    {
      for (i = 0; i <= 8; i++)
      {
        snprintf(caption, 20, "%d dBm", ((i + 7) * -10) - atten - ScaleShift);
        Text(10, 462 - (50 * i), caption, font_ptr, 0, 0, 0, 255, 255, 255);
      }
    }
    else         // Bands 7 and 8: using external mixer, so atten = 0
    {
      for (i = 0; i <= 8; i++)
      {
        snprintf(caption, 20, "%d dBm", ((i + 7) * -10) - ScaleShift);
        Text(10, 462 - (50 * i), caption, font_ptr, 0, 0, 0, 255, 255, 255);
      }
    }
  }
  else           // Normalised
  {
    for (i = 0; i <= 8; i++)
    {
      snprintf(caption, 15, "%d dB", ((i + 7) * -10) + normlabel - normlevel - ScaleShift + NormScaleCorrn); 
      Text(10, 462 - (50 * i), caption, font_ptr, 0, 0, 0, 255, 255, 255);
    }
  }

  // Draw the gain increment and decrement buttons
  Text(1, 335, "+", &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);
  Text(1, 185, "-", &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);
  Text(1, 435, "^", &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);
  Text(1, 85,  "v", &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);
}


void DrawSettings()
{
  const font_t *font_ptr = &font_dejavu_sans_22;   // 22pt
  int titley = 5;

  // Clear the previous text first
  rectangle(100, 0, 505, 35, 0, 0, 0);

  if (strcmp(PlotTitle, "-") != 0)
  {
    TextMid(350, titley, PlotTitle, font_ptr, 0, 0, 0, 255, 255, 255);
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
    ypospix = 409 - ypos;  //409 = 479 - 70
    setInvPixel(xpos, ypospix, column[ypos], column[ypos], 0);  
  }

  // Draw the background and grid (except in the active trace) to erase the previous scan

  if (xpos % 50 == 0)  // vertical int graticule
  {
    for(ypos = 1; ypos < ymin; ypos++)
    {
      setInvPixel(xpos, 479 - (70 + ypos), 64, 64, 64);
    }
    for(ypos = 399; ypos > ymax; ypos--)
    {
      setInvPixel(xpos, 479 - (70 + ypos), 64, 64, 64);
    }
  }
  else if ((xpos > 344) && (xpos < 356))  // centre vertical graticule marks
  {
    for(ypos = 1; ypos < ymin; ypos++)
    {
      ypospix = 409 - ypos;  // 409 = 479 - 70

      if (ypos % 10 == 0)  // tick mark
      {
        setInvPixel(xpos, ypospix, 64, 64, 64);
      }
      else
      {
        setInvPixel(xpos, ypospix, 0, 0, 0);
      }
    }
    for(ypos = 399; ypos > ymax; ypos--)
    {
      ypospix = 409 - ypos;  // 409 = 479 - 70

      if (ypos % 10 == 0)  // tick mark
      {
        setInvPixel(xpos, ypospix, 64, 64, 64);
      }
      else
      {
        setInvPixel(xpos, ypospix, 0, 0, 0);
      }
    }
  }
  else  // horizontal graticule and open space
  {
    for(ypos = 1; ypos < ymin; ypos++)  // below the trace
    {
      ypospix = 409 - ypos;  // 409 = 479 - 70

      if (ypos % 50 == 0)  // graticule line
      {
        setInvPixel(xpos, ypospix, 64, 64, 64);
      }
      else
      {
        setInvPixel(xpos, ypospix, 0, 0, 0);
      }
      if ((xpos % 10 == 0) && (ypos > 195) && (ypos < 205)) // tick mark on x axis
      {
        setInvPixel(xpos, ypospix, 64, 64, 64);
      }
    }
    for(ypos = 399; ypos > ymax; ypos--)  // above the trace
    {
      ypospix = 409 - ypos;  // 409 = 479 - 70
      if (ypos % 50 == 0)  // graticule line
      {
        setInvPixel(xpos, ypospix, 64, 64, 64);
      }
      else
      {
        setInvPixel(xpos, ypospix, 0, 0, 0);
      }
      if ((xpos % 10 == 0) && (ypos > 195) && (ypos < 205)) // tick mark on x axis
      {
        setInvPixel(xpos, ypospix, 64, 64, 64);
      }
    }
  }

  // Draw the markers

  if (markeron == true)
  {
    MarkerGrn(markerx, xpos, markery);
  }
  publish();
}


// Takes Y value and constrains it to range 1 - 399, printing error message
int constrainY(int y)
{
  if (y < 1)
  {
    //printf("y was %d, constrained to 1\n", y);
    y = 1;
  }
  else if (y > 399)
  {
    //printf("y was %d, constrained to 399\n", y);
    y = 399;
  }
  return y;
}


void *PanelMonitor_thread(void *arg)
{
  bool *exit_requested = (bool *)arg;

  #define AD_BASE 200
  #define M1_BASE 100
  #define M2_BASE 150

  #define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
  #define BYTE_TO_BINARY(byte)  \
    ((byte) & 0x80 ? '1' : '0'), \
    ((byte) & 0x40 ? '1' : '0'), \
    ((byte) & 0x20 ? '1' : '0'), \
    ((byte) & 0x10 ? '1' : '0'), \
    ((byte) & 0x08 ? '1' : '0'), \
    ((byte) & 0x04 ? '1' : '0'), \
    ((byte) & 0x02 ? '1' : '0'), \
    ((byte) & 0x01 ? '1' : '0') 

  const font_t *font_ptr = &font_dejavu_sans_18;   // 18pt

  int i;
  char FreqText[63];
  char SpanText[63];
  char BWText[63];
  char result[63];
  char ipaddress[255];
  char ADCText[255];
  char M1AText[255];
  char M1BText[255];
  char M2AText[255];
  char M2BText[255];
  char BandText[255];
  char AttenText[255];
  char ModeText[30];
  char OPText[20];
  char OPDispText[255];
  char TempText[255];

  int adcreturn;
  int VoltsRange;
  float LORange;
  float LOFactor;
  float LOatZero;
  float LOFreq;
  char CPUTemp[31];


  uint8_t M1A = 0;
  uint8_t M1B = 0;
  uint8_t M2A = 0;
  uint8_t M2B = 0;
  uint8_t rawspan = 0;
  uint8_t rawtperdiv = 0;
  int LimeTempValue;

  static uint8_t oldrawBW;
  static float oldSDRsamplerate;
  // RF Parameters
  int firstIF = 2050;
  int secondIF = 323;

  // Calculate constants for voltage to frequency conversion
  VoltsRange = HiFreqVolts - LowFreqVolts;
  LORange = HiFreqLO - LowFreqLO;
  LOFactor = LORange / (float)(VoltsRange);
  LOatZero = LowFreqLO - (LowFreqVolts * LOFactor);

  // Set up ADS1115 for Centre Frequency indication
  ads1115Setup (AD_BASE, 0x48);

  // Set ADS1115 gain.  Second parameter  = 0 for gain control
  digitalWrite (AD_BASE + 0, 2);  // Default of 2 is required

  // Set ADS1115 data rate.  Second parameter  = 1 for gain control
  digitalWrite (AD_BASE + 1, 4);  // Default of 4 (128 SPS) is OK

  // Set up first MCP23017 for reading front panel switches
  mcp23017Setup (M1_BASE, 0x20);

  // Set pins GPA0 - 6 and GPB0 - 6 as inputs.  GPBA7 and GPB7 are hard wired as outputs
  for (i = 0; i < 7; i++)
  {
    pinMode (M1_BASE + i, INPUT);
    pinMode (M1_BASE + i + 8, INPUT);
  }

  // Set up second MCP23017 for reading front panel switches
  mcp23017Setup (M2_BASE, 0x21);

  // Set pins GPA0 - 6 and GPB0 - 6 as inputs.  GPBA7 and GPB7 are hard wired as outputs
  for (i = 0; i < 7; i++)
  {
    pinMode (M2_BASE + i, INPUT);
    pinMode (M2_BASE + i + 8, INPUT);
  }

  // Read all the parameters and update display
  do
  {
    // Read ADC voltage and convert to LO frequency
    adcreturn = analogRead (AD_BASE + 4);  // pin =  4 sets diff mode on 0 and 1 
    LOFreq = LOatZero + (float)(adcreturn) * LOFactor;

    // Read Multiplexers
    M1A = 0;
    M1B = 0;
    M2A = 0;
    M2B = 0;

    for (i = 6; i >= 0; i--)
    {
      M1A = M1A * 2;
      if (digitalRead (M1_BASE + i) == HIGH)
      {
        M1A = M1A + 1;
      }

      M1B = M1B * 2;
      if (digitalRead (M1_BASE + i + 8) == HIGH)
      {
        M1B = M1B + 1;
      }

      M2A = M2A * 2;
      if (digitalRead (M2_BASE + i) == HIGH)
      {
        M2A = M2A + 1;
      }

      M2B = M2B * 2;
      if (digitalRead (M2_BASE + i + 8) == HIGH)
      {
        M2B = M2B + 1;
      }
    }

    // Extract Band data
    band = ((M1B & 0x70) >> 4) + 1;  // Bands are 1 to 8

    // Calculate the Centre frequency for the selected band
    switch (band)
    {
      case 1:                                            // LO - 2050
        DispFreq = (int)(LOFreq) - firstIF;
        break;
      case 2:                                            // LO - 323
        DispFreq = (int)(LOFreq) - secondIF;
        break;
      case 3:                                            // 2LO - 323
        DispFreq = (int)(2.0 * LOFreq) - secondIF;
        break;
      case 4:                                            // 3LO - 323
        DispFreq = (int)(3.0 * LOFreq) - secondIF;
        break;
      case 5:                                            // 4LO + 323
        DispFreq = (int)(4.0 * LOFreq) + secondIF;
        break;
      case 6:                                            // 5LO + 2050
        DispFreq = (int)(5.0 * LOFreq) + firstIF;
        break;
      case 7:                                            // 6LO + 2050
        DispFreq = (int)(6.0 * LOFreq) + firstIF;
        break;
      case 8:                                            // 10LO + 20250
        DispFreq = (int)(10.0 * LOFreq) + firstIF;
        break;
      default:
        DispFreq = 0;
        break;
    }

    // Extract the Attenuator Setting
    atten = -10 * (int32_t)((M1A & 0x70) >> 4);  // Input 0x00 to 0x70, output 0 to -70 on steps of 10

    // Extract the IF Attenuator Setting
    IFatten = -30 + (10 * (int32_t)((M2A & 0x40) >> 6)) + (20 * (int32_t)((M2B & 0x40) >> 6));  // M2A and M2B Bit 6 inverted logic

    // Extract the Span
    rawspan = M1A & 0x0F;
    switch (rawspan)
    {
      case 1:                                            // 1 MHz
        span = 1;
        break;
      case 2:                                            // 2 MHz
        span = 2;
        break;
      case 3:                                            // 5 MHz
        span = 5;
        break;
      case 4:                                            // 10 MHz
        span = 10;
        break;
      case 5:                                            // 20 MHz
        span = 20;
        break;
      case 6:                                            // 50 MHz
        span = 50;
        break;
      case 7:                                            // 100 MHz
        span = 100;
        break;
      case 8:                                            // 200 MHz
        span = 200;
        break;
      case 9:                                            // 500 MHz
        span = 500;
        break;
      case 10:                                            // 1000 MHz
        span = 1000;
        break;
      case 11:                                            // 2000 MHz
        span = 2000;
        break;
      case 12:                                            // 5000 MHz
        span = 5000;
        break;
    }

    // Extract the BandWidth
    rawbw = M2B & 0x0F;

    if (rawbw == 1)     // Bandwidth set to auto, so 1/200th of span
    {
      switch (rawspan)
      {
        case 1:                                            // 1 MHz
          filter_bandwidth = 100000;
          SDRsamplerate = 102400;
          break;
        case 2:                                            // 2 MHz
          filter_bandwidth = 100000;
          SDRsamplerate = 102400;
          break;
        case 3:                                            // 5 MHz
          filter_bandwidth = 100000;
          SDRsamplerate = 102400;
          break;
        case 4:                                            // 10 MHz
          filter_bandwidth = 100000;
          SDRsamplerate = 102400;
          break;
        case 5:                                            // 20 MHz
          filter_bandwidth = 100000;
          SDRsamplerate = 102400;
          break;
        case 6:                                            // 50 MHz
          filter_bandwidth = 200000;
          SDRsamplerate = 204800;
          break;
        case 7:                                            // 100 MHz
          filter_bandwidth = 500000;
          SDRsamplerate = 512000;
          break;
        case 8:                                            // 200 MHz
          filter_bandwidth = 1000000;
          SDRsamplerate = 1024000;
          break;
        case 9:                                            // 500 MHz
          filter_bandwidth = 2000000;
          SDRsamplerate = 2048000;
          break;
        case 10:                                            // 1000 MHz
          filter_bandwidth = 5000000;
          SDRsamplerate = 5120000;
          break;
        case 11:                                            // 2000 MHz
          filter_bandwidth = 10000000;
          SDRsamplerate = 10240000;
          break;
        case 12:                                            // 5000 MHz
          filter_bandwidth = 10000000;
          SDRsamplerate = 10240000;
          break;
      }
    }
    else  // manual bandwidth
    {
      switch (rawbw)
      {
        case 1:                                            // 2 kHz  Now auto
          filter_bandwidth = 2000;
          SDRsamplerate = 102400;
          break;
        case 2:                                            // 5 kHz
          filter_bandwidth = 5000;
          SDRsamplerate = 102400;
          break;
        case 3:                                            // 10 kHz
          filter_bandwidth = 1000;
          SDRsamplerate = 102400;
          break;
        case 4:                                            // 20 kHz
          filter_bandwidth = 20000;
          SDRsamplerate = 102400;
          break;
        case 5:                                            // 50 kHz
          filter_bandwidth = 50000;
          SDRsamplerate = 102400;
          break;
        case 6:                                            // 100 kHz
          filter_bandwidth = 100000;
          SDRsamplerate = 102400;
          break;
        case 7:                                            // 200 kHz
          filter_bandwidth = 200000;
          SDRsamplerate = 204800;
          break;
        case 8:                                            // 500 kHz
          filter_bandwidth = 500000;
          SDRsamplerate = 512000;
          break;
        case 9:                                            // 1 MHz
          filter_bandwidth = 1000000;
          SDRsamplerate = 1024000;
          break;
        case 10:                                           // 2 MHz
          filter_bandwidth = 2000000;
          SDRsamplerate = 2048000;
          break;
        case 11:                                           // 5 MHz
          filter_bandwidth = 5000000;
          SDRsamplerate = 5120000;
          break;
        case 12:                                           // 10 MHz
          filter_bandwidth = 10000000;
          SDRsamplerate = 10240000;
          break;
      }
    }
    if ((rawbw != oldrawBW)  || (SDRsamplerate != oldSDRsamplerate))
    {
      NewBW = true;
      oldrawBW = rawbw;
      oldSDRsamplerate = SDRsamplerate;
    }

    // Extract the sweep time per division
    rawtperdiv = M1B & 0x0F;                               // 1 to 12.  1 ms to 5 sec
    switch (rawtperdiv)
    {
      case 1:                                            // 1 ms
        tperdiv = 1;
        break;
      case 2:                                            // 2 ms
        tperdiv = 2;
        break;
      case 3:                                            // 5 ms
        tperdiv = 5;
        break;
      case 4:                                            // 10 ms
        tperdiv = 10;
        break;
      case 5:                                            // 20 ms
        tperdiv = 20;
        break;
      case 6:                                            // 50 ms
        tperdiv = 50;
        break;
      case 7:                                            // 100 ms
        tperdiv = 100;
        break;
      case 8:                                            // 200 ms
        tperdiv = 200;
        break;
      case 9:                                            // 500 ms
        tperdiv = 500;
        break;
      case 10:                                           // 1000 ms
        tperdiv = 1000;
        break;
      case 11:                                           // 2000 ms
        tperdiv = 2000;
        break;
      case 12:                                           // 5000 ms
        tperdiv = 5000;
        break;
    }

    // Extract the mode
    samode = M2A & 0x0F;
    switch (samode)
    {
      case 1:                                            // DATV RX on SA IF
        strcpy(ModeText, "DATV RX on SA IF");
        break;
      case 2:                                            // Direct DATV RX
        strcpy(ModeText, "Direct DATV RX");
        break;
      case 3:                                            // Power Detector
        strcpy(ModeText, "Power Detector");
        break;
      case 4:                                            // Bolo power Meter
        strcpy(ModeText, "Bolo power Meter");
        break;
      case 5:                                            // Direct SDR
        strcpy(ModeText, "Direct SDR");
        break;
      case 6:                                            // SDR fft on SA IF
        strcpy(ModeText, "SDR fft on SA IF");
        break;
      case 7:                                            // Swept SA
        strcpy(ModeText, "Swept SA");
        break;
      case 8:                                            // Swept Tracking Generator
        strcpy(ModeText, "Swept Tracking Generator");
        break;
      case 9:                                            // Zero Span Tracking Generator
        strcpy(ModeText, "Zero Span Tracking Generator");
        break;
    }

    // If sa mode selection is auto, check that mode is valid.  If not, cleanexit to new mode.
    // Make sure that Lime has started first
    if ((samodeauto == true) && (LimeStarted == true))
    {
      switch (samode)
      {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        // case 7:      AM Detector on SA IF
        // case 8:      AM Detector on SA IF for TG
        case 9:         // TG zero span
          printf("\n***** Exiting because Mode Switch has been selected to another Mode ****\n\n");
          cleanexit(170);
          break;
      } 
    }

    // Extract the Output Mode
    opmode = (M2A & 0x30) >> 4;
    switch (opmode)
    {
      case 0:                                            // Tracking Generator
      case 3:
        strcpy(OPText, "Tracking Generator");
        break;
      case 1:                                            // DATV Express
        strcpy(OPText, "DATV Express");
        break;
      case 2:                                            // LimeSDR
        strcpy(OPText, "LimeSDR");
        break;
    }

    if (ShowStatusPage == false)  // Normal operation
    {
      if (freeze == false)        // Dont't write if the keyboard is displayed
      {
        // Clear the text in line 1
        rectangle(100, 35, 505, 34, 0, 0, 0);

        // Write the Span
        snprintf(SpanText, 63, "Span %d MHz", span);
        Text(100, 45, SpanText, font_ptr, 0, 0, 0, 255, 255, 255);

        // Write the Centre Frequency
        snprintf(FreqText, 63, "Centre %d MHz    ", DispFreq);
        TextMid(350, 45, FreqText, font_ptr, 0, 0, 0, 255, 255, 255);

        // Write the Bandwidth or the Gain

        if (monotonic_ms() < show_gain_until)
        {
          snprintf(BWText, 63, "IF Gain %d%%", limegain);
        }
        else
        {
          if (filter_bandwidth > 999000)
          {
            snprintf(BWText, 63, "BW %d MHz", filter_bandwidth / 1000000);
          }
          else
          {
            snprintf(BWText, 63, "BW %d kHz", filter_bandwidth / 1000);
          }
        }
        Text(480, 45, BWText, font_ptr, 0, 0, 0, 255, 255, 255);
      }
    }
    else    // Show diagnostics
    {
      if (FirstStatusCall == true)  // Look up the temperatures
      {
        GetCPUTemp(CPUTemp);
        LimeTempValue = LimeTemp();
        FirstStatusCall = false;  // only do it on initial selection
      }

      // Page Title
      TextMid(400, 450, "KeyLimePi RF Analyser Status", font_ptr, 0, 0, 0, 255, 255, 255);

      // IP Address
      strcpy(ipaddress, "IP Address: ");
      strcpy(result, "Not connected");
      GetIPAddr(result);
      strcat(ipaddress, result);
      Text(50, 420, ipaddress, font_ptr, 0, 0, 0, 255, 255, 255);

      // Frequency ADC
      rectangle(50, 390, 700, 24, 0, 0, 0);
      snprintf(ADCText, 200, "Freq ADC: Raw = %d, Calculated LO Freq = %.2f MHz", adcreturn, LOFreq);
      Text(50, 390, ADCText, font_ptr, 0, 0, 0, 255, 255, 255);

      // Multiplexer 1
      rectangle(50, 360, 700, 24, 0, 0, 0);
      snprintf(M1AText, 50, "Multiplexer 1A: Raw = "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(M1A));
      snprintf(M1BText, 50, "    1B: Raw = "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(M1B));
      strcat(M1AText, M1BText);
      Text(50, 360, M1AText, font_ptr, 0, 0, 0, 255, 255, 255);

      // Multiplexer 2
      rectangle(50, 330, 700, 24, 0, 0, 0);
      snprintf(M2AText, 50, "Multiplexer 2A: Raw = "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(M2A));
      snprintf(M2BText, 50, "    2B: Raw = "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(M2B));
      strcat(M2AText, M2BText);
      Text(50, 330, M2AText, font_ptr, 0, 0, 0, 255, 255, 255);

      // Band, frequency, bandwidth and span
      rectangle(50, 300, 700, 24, 0, 0, 0);
      snprintf(BandText, 70, "Band %d, Frequency %d MHz, Span %d MHz, bandwidth %d kHz", band, DispFreq, span, filter_bandwidth / 1000);
      Text(50, 300, BandText, font_ptr, 0, 0, 0, 255, 255, 255);

      // Time per division, Attenuator and mode
      rectangle(50, 270, 749, 24, 0, 0, 0);
      snprintf(AttenText, 97, "Scan time per div %d ms, Attenuator %d dB, Mode: %s", tperdiv, atten, ModeText);
      Text(50, 270, AttenText, font_ptr, 0, 0, 0, 255, 255, 255);

      // IF Attenuator and Selected Output
      rectangle(50, 235, 700, 29, 0, 0, 0);
      snprintf(OPDispText, 70, "IF Attenuator %d dB, Selected Output: %s", IFatten, OPText);
      Text(50, 240, OPDispText, font_ptr, 0, 0, 0, 255, 255, 255);

      // Temperatures
      rectangle(50, 200, 700, 29, 0, 0, 0);
      snprintf(TempText, 70, "CPU %s,    LimeSDR Temp = %d 'C ", CPUTemp, LimeTempValue);
      Text(50, 210, TempText, font_ptr, 0, 0, 0, 255, 255, 255);

      // Exit message
      TextMid(400, 5, "Touch screen to Exit", font_ptr, 0, 0, 0, 255, 255, 255);
      publish();
    }
    usleep(10000);
  }
  while (false == *exit_requested);
  return NULL;
}

void *PowerMeasure_thread(void *arg)
{
  bool *exit_requested = (bool *)arg;
  uint64_t end_blanking_time;
  uint64_t end_blanking_time_was;
  uint64_t smoothed_end_blanking_time;
  uint64_t smoothed_end_blanking_time_was;
  uint64_t interval;
  uint64_t smoothed_interval;
  //int delta;
  //int64_t deltaEBT;
  int samplepairs;
  bool flybackstatus;
  int64_t scan_offset_time;
  int64_t scan_offset_samples;
  int raw_pixel;
  bool use_buffer1;
  int j;
  int pixel_start_offset;
  float buffer_power;
  float instantaneous_power;
  float lpwr;
  int i_sample_index;
  int j_sample_index;
  int previous_SDRsamplerate;
  int previous_tperdiv;

  show_gain_until = monotonic_ms();

  // Wait here until the front panel has been read.  Makes sense to wait until LimeSDR has started
  while (LimeStarted == false)
  {
    usleep(1000);
  }

  // Initialise smoothing variables  ******  These need to be calulated
  smoothed_interval = 242600000;                  
  smoothed_end_blanking_time = monotonic_ns();
  smoothed_end_blanking_time_was = monotonic_ns() - smoothed_interval;
  end_blanking_time_was = monotonic_ns() - smoothed_interval;

  while(false == *exit_requested) // Main loop - go round once for each full frequency scan
  {
    // Check switch settings and get calibration constants
    switch ((int)(SDRsamplerate / 100))
    {
      case 1024:                              // 100 kHz bandwidth
        switch (tperdiv)
        {
          case 1:                               // 0.001 sec/dev
            samplepairs = 2;
            delaypixels = 45;
            break;
          case 2:                               // 0.002 sec/dev
            samplepairs = 5;
            delaypixels = 45;
            break;
          case 5:                               // 0.005 sec/dev
            samplepairs = 12;
            delaypixels = 45;
            break;
          case 10:                              // 0.01 sec/dev ***
            samplepairs = 24;
            delaypixels = 45;
            break;
          case 20:                              // 0.02 sec/dev ****
            samplepairs = 48;
            delaypixels = 45;
            break;
          case 50:                              // 0.05 sec/dev ****
            samplepairs = 120;
            delaypixels = 0;
            break;
          case 100:                             // 0.1 sec/dev ***
            samplepairs = 240;
            delaypixels = 0;
            break;
          case 200:                             // 0.2 sec/dev ***
            samplepairs = 480;
            delaypixels = 0;
            break;
          case 500:                             // 0.5 sec/dev ***
            samplepairs = 1200;
            delaypixels = 0;
            break;
        }
        break;
      case 2048:                              // 200 kHz bandwidth
        switch (tperdiv)
        {
          case 1:                               // 0.001 sec/dev
            samplepairs = 5;
            delaypixels = 0;
            break;
          case 2:                               // 0.002 sec/dev
            samplepairs = 10;
            delaypixels = 0;
            break;
          case 5:                               // 0.005 sec/dev
            samplepairs = 24;
            delaypixels = 0;
            break;
          case 10:                              // 0.01 sec/dev
            samplepairs = 48;
            delaypixels = 0;
            break;
          case 20:                              // 0.02 sec/dev ***
            samplepairs = 96;
            delaypixels = 40;
            break;
          case 50:                              // 0.05 sec/dev
            samplepairs = 240;
            delaypixels = 0;
            break;
          case 100:                             // 0.1 sec/dev
            samplepairs = 480;
            delaypixels = 0;
            break;
          case 200:                             // 0.2 sec/dev
            samplepairs = 960;
            delaypixels = 0;
            break;
          case 500:                             // 0.5 sec/dev
            samplepairs = 2400;
            delaypixels = 0;
            break;
        }
        break;
      case 5120:                              // 500 kHz bandwidth
        switch (tperdiv)
        {
          case 1:                               // 0.001 sec/dev
            samplepairs = 12;
            delaypixels = 40;
            break;
          case 2:                               // 0.002 sec/dev
            samplepairs = 24;
            delaypixels = 40;
            break;
          case 5:                               // 0.005 sec/dev
            samplepairs = 60;
            delaypixels = 40;
            break;
          case 10:                              // 0.01 sec/dev
            samplepairs = 120;
            delaypixels = 40;
            break;
          case 20:                              // 0.02 sec/dev  ****
            samplepairs = 240;
            delaypixels = 40;
            break;
          case 50:                              // 0.05 sec/dev
            samplepairs = 600;
            delaypixels = 0;
            break;
          case 100:                             // 0.1 sec/dev
            samplepairs = 1200;
            delaypixels = 0;
            break;
          case 200:                             // 0.2 sec/dev
            samplepairs = 2400;
            delaypixels = 0;
            break;
          case 500:                             // 0.5 sec/dev
            samplepairs = 6000;
            delaypixels = 0;
            break;
        }
        break;
      case 10240:                             // 1 MHz bandwidth
        switch (tperdiv)
        {
          case 1:                              // 0.001 sec/dev
            samplepairs = 24;
            delaypixels = 40;
            break;
          case 2:                              // 0.002 sec/dev
            samplepairs = 48;
            delaypixels = 40;
            break;
          case 5:                              // 0.005 sec/dev
            samplepairs = 120;
            delaypixels = 40;
            break;
          case 10:                              // 0.01 sec/dev
            samplepairs = 240;
            delaypixels = 40;
            break;
          case 20:                              // 0.02 sec/dev ***
            samplepairs = 480;
            delaypixels = 40;
            break;
          case 50:                              // 0.05 sec/dev
            samplepairs = 1200;
            delaypixels = 0;
            break;
          case 100:                             // 0.1 sec/dev
            samplepairs = 2400;
            delaypixels = 0;
            break;
          case 200:                             // 0.2 sec/dev
            samplepairs = 4800;
            delaypixels = 0;
            break;
          case 500:                             // 0.5 sec/dev
            samplepairs = 12000;
            delaypixels = 0;
            break;
        }
        break;
      case 20480:                             // 2 MHz bandwidth
        switch (tperdiv)
        {
          case 1:                              // 0.001 sec/dev
            samplepairs = 48;
            delaypixels = 40;
            break;
          case 2:                              // 0.002 sec/dev
            samplepairs = 96;
            delaypixels = 40;
            break;
          case 5:                              // 0.005 sec/dev
            samplepairs = 240;
            delaypixels = 40;
            break;
          case 10:                              // 0.01 sec/dev
            samplepairs = 480;
            delaypixels = 40;
            break;
          case 20:                              // 0.02 sec/dev *****
            samplepairs = 960;
            delaypixels = 40;
            break;
          case 50:                              // 0.05 sec/dev
            samplepairs = 2400;
            delaypixels = 0;
            break;
          case 100:                             // 0.1 sec/dev
            samplepairs = 4800;
            delaypixels = 0;
            break;
          case 200:                             // 0.2 sec/dev
            samplepairs = 9600;
            delaypixels = 0;
            break;
          case 500:                             // 0.5 sec/dev
            samplepairs = 24000;
            delaypixels = 0;
            break;
        }
        break;
      case 51200:                             // 5 MHz bandwidth
        switch (tperdiv)
        {
          case 1:                              // 0.001 sec/dev
            samplepairs = 120;
            delaypixels = 40;
            break;
          case 2:                              // 0.002 sec/dev
            samplepairs = 240;
            delaypixels = 40;
            break;
          case 5:                              // 0.005 sec/dev
            samplepairs = 600;
            delaypixels = 40;
            break;
          case 10:                              // 0.01 sec/dev
            samplepairs = 1200;
            delaypixels = 40;
            break;
          case 20:                              // 0.02 sec/dev ****
            samplepairs = 2400;
            delaypixels = 40;
            break;
          case 50:                              // 0.05 sec/dev
            samplepairs = 6000;
            delaypixels = 0;
            break;
          case 100:                             // 0.1 sec/dev
            samplepairs = 12000;
            delaypixels = 0;
            break;
          case 200:                             // 0.2 sec/dev
            samplepairs = 24000;
            delaypixels = 0;
            break;
          case 500:                             // 0.5 sec/dev
            samplepairs = 60000;
            delaypixels = 0;
            break;
        }
        break;
      case 102400:                            // 10 MHz bandwitch
        switch (tperdiv)
        {
          case 1:                              // 0.001 sec/dev
            samplepairs = 240;
            delaypixels = 40;
            break;
          case 2:                              // 0.002 sec/dev
            samplepairs = 480;
            delaypixels = 40;
            break;
          case 5:                              // 0.005 sec/dev
            samplepairs = 1200;
            delaypixels = 40;
            break;
          case 10:                              // 0.01 sec/dev
            samplepairs = 2400;
            delaypixels = 40;
            break;
          case 20:                              // 0.02 sec/dev
            samplepairs = 4800;
            delaypixels = 40;
            break;
          case 50:                              // 0.05 sec/dev
            samplepairs = 12000;
            delaypixels = 0;
            break;
          case 100:                             // 0.1 sec/dev
            samplepairs = 24000;
            delaypixels = 0;
            break;
          case 200:                             // 0.2 sec/dev
            samplepairs = 48000;
            delaypixels = 0;
            break;
          case 500:                             // 0.5 sec/dev
            samplepairs = 12000;
            delaypixels = 0;
            break;
        }
        break;  
    }              // End of check switch settings

    // Monitor for switch changes

    if (previous_SDRsamplerate != SDRsamplerate)
    {
      NewBW = true;
      previous_SDRsamplerate = SDRsamplerate;
    }

    if (previous_tperdiv != tperdiv)
    {
      //NewBW = true;
      previous_tperdiv = tperdiv;
    }

    // declare all pixels in output array invalid
    for (i = 0; i < 1023; i++)
    {
      pixelbuff[i].valid = false;
    }

    // For testing 20 MHz span, 20 mS/div, 100 kHz bandwidth
    //samplepairs = 48;

    // For testing 20 MHz span, 20 mS/div, 200 kHz bandwidth
    //samplepairs = 96;

    // For testing 20 MHz span, 20 mS/div, 500 kHz bandwidth
    //samplepairs = 240;

    // For testing 20 MHz span, 20 mS/div, 1 MHz bandwidth
    //samplepairs = 480;

    // For testing 20 MHz span, 20 mS/div, 2 MHz bandwidth
    //samplepairs = 960;

    // Wait here for the start of the scan

    do  // Wait here for end of flyback.  Digital Read 6 goes false when flyback has finished
    {
      flybackstatus = digitalRead(6);
      usleep(20);  // one tenth of a pixel at 10 mS/div
    }
    while (flybackstatus == false);

    scanstart = true;

    // Record time.  May need smoothing for refinement
    end_blanking_time = monotonic_ns();

    // Calculate smoothed interval between ebt triggers (approx 24.3 mS)
    interval = end_blanking_time - end_blanking_time_was;
    smoothed_interval = (interval + (19 * smoothed_interval)) / 20;
    //delta = (int)(smoothed_interval - interval);
    end_blanking_time_was = end_blanking_time;

    // Calculate smoothed end_blanking_time 
    smoothed_end_blanking_time = (end_blanking_time + (19 * (smoothed_end_blanking_time_was + smoothed_interval))) / 20;
    smoothed_end_blanking_time_was = smoothed_end_blanking_time;
    //deltaEBT = ((int64_t)smoothed_end_blanking_time - (int64_t)end_blanking_time);

    // Print diagnostics
    //printf("EBT = %ld, interval = %ld, smoothed_interval = %ld, delta = %d nS, smoothedEBT = %ld, deltaEBT = %ld\n", end_blanking_time, interval, smoothed_interval, delta, smoothed_end_blanking_time, deltaEBT);

    // Use next samples, not current
    buffer1_updated = false;
    buffer2_updated = false;

    // Calculate time between last buffer1 sample and ebt
    //scan_offset_time = smoothed_end_blanking_time - smoothed_sample_read_time;
    scan_offset_time = end_blanking_time - sample_read_time;  // smoothed values seem less reliable here
    scan_offset_samples =  (scan_offset_time * SDRsamplerate) / 1000000000;  // as scan offset time is in nS

    // Print diagnostics
    //printf("Scan offset time = %ld, Scan offset samples = %ld\n", scan_offset_time, scan_offset_samples);

    // Error check
    if ((scan_offset_samples > 2039) || (scan_offset_samples < 0))
    {
      scan_offset_samples = 1022; // set to mid value
    }

    pixel_start_offset = scan_offset_samples;
   
    // Read and process the samples
    for (raw_pixel = 0; raw_pixel < 490; raw_pixel++)
    {
      buffer_power = 0;
      instantaneous_power = 0;

      for (j = 0; j < samplepairs ; j = j + 1)   // for each IQ sample pair
      {
        if (pixel_start_offset > 2039)
        {
          pixel_start_offset  = pixel_start_offset - 2040;
        }

        if (pixel_start_offset > 1019)
        {
          use_buffer1 = true;
          i_sample_index = 2 * (pixel_start_offset - 1020);
          j_sample_index = 2 * (pixel_start_offset - 1020) + 1;
        }
        else
        {
          use_buffer1 = false;
          i_sample_index = 2 * (pixel_start_offset);
          j_sample_index = 2 * (pixel_start_offset) + 1;
        }

        if (use_buffer1 == true)
        {
          // Wait for buffer1 to be ready
          while (buffer1_updated == false)
          {
            usleep(10);
          }
          amp_in[i_sample_index] = ((float*)buffer1)[i_sample_index];
          amp_in[j_sample_index] = ((float*)buffer1)[j_sample_index];
        }
        else
        {
          // Wait for buffer2 to be ready
          while (buffer2_updated == false)
          {
            usleep(10);
          }
          amp_in[i_sample_index] = ((float*)buffer2)[i_sample_index];
          amp_in[j_sample_index] = ((float*)buffer2)[j_sample_index];
        }

        if (pixel_start_offset == 1019)  // last entry in buffer2 has been read
        {
          buffer2_updated = false;
        }
        if (pixel_start_offset == 2039)  // last entry in buffer1 has been read
        {
          buffer1_updated = false;
        }

        instantaneous_power = (amp_in[i_sample_index] * amp_in[i_sample_index]) + (amp_in[j_sample_index] * amp_in[j_sample_index]);

        if (peak == true)
        {
          if (instantaneous_power > buffer_power)
          {
            buffer_power = instantaneous_power;
          }
        }
        else
        {
            buffer_power = buffer_power + instantaneous_power;
        }

        pixel_start_offset = pixel_start_offset + 1;

      }  // end of for each sample pair

      if (peak == false)
      {
        buffer_power = buffer_power / samplepairs;
      }

      lpwr = 10.f * log10(buffer_power + 1.0e-20);   // lpwr should be in dB
      freq_power = (5 * lpwr + 390);            // 5 pixels per dB, + a screen offset

      // Correct for Lime Gain, Band and Frequency of each pixel
      freq_power = freq_power + 5 * calcFreqGainCal(DispFreq - (span / 2) + (int)span * ((int)(raw_pixel) - delaypixels) / 500);

      // Correct for IF Mixer (-4 dB) and IF Attenuator
      freq_power = freq_power + 5 * 4 - 5 * IFatten;

      // Convert to int and apply the scaleshift.
      pixelbuff[raw_pixel].val = (int)(freq_power) + (ScaleShift * 5);
      pixelbuff[raw_pixel].valid = true;

//if (raw_pixel == 0)
//{
//  printf("first sample complete at %ld\n", monotonic_ns());
//}
      usleep(10);
    }    // end of for each raw_pixel

now = monotonic_ns();
//printf ("end of processing at %ld nS, time from ebt = %ld nS\n", now, (now - end_blanking_time));


    // If processing finishes early, wait for the start of the flyback pulse
    do  // Digital Read 6 goes true when flyback has started
    {
      flybackstatus = digitalRead(6);
      usleep(20);  // one tenth of a pixel at 10 mS/div
    }
    while (flybackstatus == true);

  } // end of main loop - once per scan
  return NULL;
}


void cleanexit(int calling_exit_code)
{
  app_exit = true;
  usleep(500000);
  clearScreen(0, 0, 0);
  publish();
  exit_code = calling_exit_code;
  printf("Clean Exit Code %d\n", exit_code);
  usleep(1000000);
  closeScreen();
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);
  exit(exit_code);
}


static void terminate(int dummy)
{
  app_exit = true;
  printf("Terminate\n");

  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);
  sprintf(Commnd,"reset");
  system(Commnd);
  printf("scans = %d\n", tracecount);
  clearScreen(0, 0, 0);
  publish();
  exit(207);
}


int main(int argc, char **argv)
{
  wscreen = 800;
  hscreen = 480;
  int screenXmax, screenXmin;
  int screenYmax, screenYmin;
  int i;
  int pixel;
  int displayed_atten = 0;
  //int dispvalue;
  int nextwebupdate = 10;
  char response[63];


  // Catch sigaction and call terminate
  for (i = 0; i < 16; i++)
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = terminate;
    sigaction(i, &sa, NULL);
  }

  // Make sure that text writes do not interfere with each other
  pthread_mutex_init(&text_lock, NULL);

  // Sort out display and control.  If touchscreen detected, use that (and set config) with optional web control.
  // Allow for mouse if touchscreen detected
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

    if (CheckMouse() == 0)
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
    if (CheckMouse() == 0)
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

    // Set Screen parameters
    screenXmax = 799;
    screenXmin = 0;
    wscreen = 800;
    screenYmax = 479;
    screenYmin = 0;
    hscreen = 480;

  // Calculate screen parameters
  scaleXvalue = ((float)screenXmax-screenXmin) / wscreen;
  printf ("X Scale Factor = %f\n", scaleXvalue);
  printf ("wscreen = %d\n", wscreen);
  printf ("screenXmax = %d\n", screenXmax);
  printf ("screenXmim = %d\n", screenXmin);
  scaleYvalue = ((float)screenYmax-screenYmin) / hscreen;
  printf ("Y Scale Factor = %f\n", scaleYvalue);
  printf ("hscreen = %d\n", hscreen);

  Define_Menu1();
  Define_Menu2();
  Define_Menu3();
  Define_Menu4();
  Define_Menu5();

  Define_Menu7();

  Define_Menu14();
  
  Define_Menu41();

  ReadSavedParams();

  ContScan = false;
  ModeChanged = true;
  CurrentMenu = 1;
  CallingMenu = 1;

  // Start Wiring Pi and set up WPi 6, pin 22 as flyback trigger
  wiringPiSetup();
  pinMode(6, INPUT);

  // Create Wait Button thread
  pthread_create (&thbutton, NULL, &WaitButtonEvent, NULL);

  // Initialise direct access to the 7 inch screen
  initScreen();

  /* Setting up buffers */
  buffer_circular_init(&buffer_circular_iq_main, sizeof(buffer_iqsample_t), 4096*1024);

  // Check LimeSDR Connected
  if (CheckLimeConnect() == 1)
  {
    MsgBox4("No LimeSDR Detected", "Check connections", "", "Touch Screen to Continue");
    wait_touch();
    if (CheckLimeConnect() == 1)
    {
      MsgBox4("Still No LimeSDR Detected", "Exiting from BandViewer", "", "Touch Screen to Continue");
      wait_touch();
      cleanexit(129);
    }
  }

  /* LimeSDR Thread */
  if(pthread_create(&lime_thread_obj, NULL, lime_thread, &app_exit))
  {
      fprintf(stderr, "Error creating %s pthread\n", "Lime");
      return 1;
  }

  /* Panel Monitor Thread */
  if(pthread_create(&thPanelMonitor, NULL, PanelMonitor_thread, &app_exit))
  {
      fprintf(stderr, "Error creating %s pthread\n", "Panel");
      return 1;
  }

  // Power measurement thread
  if(pthread_create(&thPowerMeasure, NULL, PowerMeasure_thread, &app_exit))
  {
      fprintf(stderr, "Error creating %s pthread\n", "Power");
      return 1;
  }

  scanstart = false;

  // declare all pixels in output array invalid
  for (i = 0; i < 1023; i++)
  {
    pixelbuff[i].valid = false;
  }

  normalised = false;
  while(app_exit == false)
  {
    {
      if (ModeChanged == true)
      {
        DrawEmptyScreen();
        DrawYaxisLabels();  // dB calibration on LHS
        DrawSettings();     // Draw Title
        CurrentMenu = 1;
        Start_Highlights_Menu1();
        UpdateWindow();     // Draw the buttons
        UpdateWeb();
        tracecount = 0;
        nextwebupdate = 10;
        ModeChanged = false;
      }

      //do  // Wait here for first sample in buffer
      //{
      //  usleep(1);
      //}
      //while (pixelbuff[0].valid == false);

      //pixelbuff[0].valid = false;

//printf("waiting for first sample at %ld\n", monotonic_ns());

      do  // Wait here for first displayed sample in buffer
      {
        usleep(1);
      }
      while ((pixelbuff[0 + delaypixels].valid == false) && (app_exit == false));
      pixelbuff[0 + delaypixels].valid = false;

      activescan = true;

      scaledadresult[0] = pixelbuff[0 + delaypixels].val;

      if (normalised == false)
      {
        y[0] = scaledadresult[0];
      }
      else
      {
        y[0] = scaledadresult[0] + norm[0];
      }

      if (normalising == true)
      {
        norm[0] = normleveloffset - scaledadresult[0];
      }

      y[0] = constrainY(y[0]);

      do  // Wait here for second sample -------------------------------------------------
      {
        usleep(1);
      }
      while ((pixelbuff[1 + delaypixels].valid == false) && (app_exit == false));
      pixelbuff[1 + delaypixels].valid = false;

      scaledadresult[1] = pixelbuff[1 + delaypixels].val;

      if (normalised == false)
      {
        y[1] = scaledadresult[1];
      }
      else
      {
        y[1] = scaledadresult[1] + norm[1];
      }

      if (normalising == true)
      {
        norm[1] = normleveloffset - scaledadresult[1];
      }

      y[1] = constrainY(y[1]);

      //for (pixel = 2; pixel < 480; pixel++)  // Subsequent Samples -----------------------
      //for (pixel = 2; pixel < (490 - delaypixels); pixel++)  // Subsequent Samples -----------------------
      for (pixel = 2; pixel < 430; pixel++)  // Subsequent Samples -----------------------
      {

        do  // Wait here for numbered sample
        {
          usleep(1);
        }
        while ((pixelbuff[pixel + delaypixels].valid == false) && (app_exit == false));
        pixelbuff[pixel + delaypixels].valid = false;

        // Measure and possibly normalise the value

        scaledadresult[pixel] = pixelbuff[pixel + delaypixels].val;

        if (normalised == false)        
        {
          y[pixel] = scaledadresult[pixel];
        }
        else                           // Apply normalisation if required
        {
          y[pixel] = scaledadresult[pixel] + norm[pixel];
        }

        if (normalising == true)         // Measure for normalisation (One scan only)
        {
          norm[pixel] = normleveloffset - scaledadresult[pixel];
        }

        y[pixel] = constrainY(y[pixel]);

        if (pixel > 20)  // do not draw first 20 pixels
        {
          DrawTrace(pixel, y[pixel - 2], y[pixel - 1], y[pixel]);
        }

	    //printf("pixel=%d, prev2=%d, prev1=%d, current=%d\n", pixel, y[pixel - 2], y[pixel - 1], y[pixel]);


if (pixel  == 444)
{
  now2 = monotonic_ns();
  printf("last update at %ld\n", now2);
}

      }

      activescan = false;

      while (freeze)
      {
        frozen = true;
        usleep(1000); // Pause to let things happen if CPU is busy
      }
      frozen = false;

      if (normalising == true)
      {
        finishednormalising = true;
      }

      if (markeron == true)
      {
        CalculateMarkers();
      }

      // Check if attenuator setting has changed
      if (atten != displayed_atten)
      {
        DrawYaxisLabels();
        displayed_atten = atten;
      }

      tracecount++;

      //UpdateWeb();

      if (tracecount >= nextwebupdate)
      {
        // printf("tracecount = %d,  \n", tracecount);
        UpdateWeb();
        usleep(10000);
        nextwebupdate = tracecount + 5;  // 
      }
    }
  }

  printf("Waiting for Button Thread to exit..\n");
  pthread_join(thbutton, NULL);
  pthread_mutex_destroy(&text_lock);
}

