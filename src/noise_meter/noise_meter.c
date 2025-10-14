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

//#include "timing.h"
#include "lime.h"
#include "fft.h"
#include "../common/font/font.h"
#include "../common/touch.h"
#include "../common/graphics.h"
#include "../common/timing.h"
#include "../common/utils.h"
#include "../common/buffer/buffer_circular.h"
#include "../common/ffunc.h"
#include "../common/hardware.h"

#define PI 3.14159265358979323846

pthread_t thbutton;
pthread_t thMeter_Movement;
pthread_t thwebclick;     //  Listens for mouse clicks from web interface
pthread_t thtouchscreen;  //  listens to the touchscreen   
pthread_t thmouse;                     //  Listens to the mouse
//static pthread_t fft_thread_obj;

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

#define PATH_SCONFIG "/home/pi/portsdown/configs/system_config.txt"
#define PATH_CONFIG "/home/pi/portsdown/configs/noise_meter_config.txt"
#define PATH_ICONFIG "/home/pi/portsdown/configs/sa_if_config.txt"

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

char StartApp[63];                     // Startup app on boot
char DisplayType[31];                  // Element14_7, touch2, hdmi, web
int VLCTransform = 0;                  // 0, 90, 180 or 270
int FBOrientation = 0;                 // 0, 90, 180 or 270
int HDMIResolution = 720;              // HDMI Resolution 720/1080
bool TouchInverted = false;            // true if screen mounted upside down
bool KeyLimePiEnabled = true;          // false for release

bool Calibrated = false;

bool CalibrateBLRequested = false;
bool BLCalibrated = false;

int scaledX, scaledY;

bool touch_inverted = false;
bool mouse_active = false;             // set true after first movement of mouse

bool MouseClickForAction = false;      // set true on left click of mouse
int mouse_x;                           // click x 0 - 799 from left
int mouse_y;                           // click y 0 - 479 from bottom
bool image_complete = true;            // prevents mouse image buffer from being copied until image is complete
bool mouse_connected = false;          // Set true if mouse detected at startup

int wbuttonsize = 100;
int hbuttonsize = 50;
int rawX, rawY;
int FinishedButton = 0;
int i;
bool freeze = false;
bool frozen = false;
bool PeakPlot = false;
bool PortsdownExitRequested = false;

int startfreq = 0;
int stopfreq = 0;
int rbw = 0;
char PlotTitle[63] = "-";
bool ContScan = false;

int centrefreq = 437000;
int span = 5120;
int limegain = 90;
int nf_bandwidth = 5000;
int pfreq1 = 146500;
int pfreq2 = 437000;
int pfreq3 = 748000;
int pfreq4 = 1255000;
int pfreq5 = 2409000;
char mode[31];           // absolute or differential or carrier or abscarrier
int smoothing;           // 1 to 1000
float noiseSmoothingFactor = 0.9;  // = 1.0 - (1 / (smoothing + 1))
float carrierSmoothingFactor;
float baseline;          // 0 to -80.0
int historyspan;         // default 100 seconds
int refstart;            // 0 to 100%
int refend;              // 0 to 100%
int sigstart;            // 0 to 100%
int sigend;              // 0 to 100%
int refstartpixel;       // 1 to 249
int refendpixel;         // 1 to 249
int sigstartpixel;       // 1 to 249
int sigendpixel;         // 1 to 249
int peakPixel;           // 1 to 249
int peakLine;           // 1 to 249

int levelhistory[255];   // 
int levelhistoryindex = 0;

float MAIN_SPECTRUM_TIME_SMOOTH;

int ScansforLevel = 10;
bool CalibrateRequested = false;
int meter_deflection = 0;
bool ModeChanged = true;
int MeterScale = 1;
float ActiveZero = 0.0;
float ActiveFSD = 30.0;

bool webcontrol = false;   // Enables webcontrol on a Portsdown 4

bool LargeNumbers = false;  // Enables large numbers instead of parameters
bool LargeNumbersDisplayed = false;  // Used to clear screen area when large numbers are toggled off

bool samodeauto = true;                  // enables front panel control of which running app


int tracecount = 0;  // Used for speed testing
int exit_code;

int yscalenum = 18;       // Numerator for Y scaling fraction
int yscaleden = 30;       // Denominator for Y scaling fraction
int yshift    = 5;        // Vertical shift (pixels) for Y

int xscalenum = 25;       // Numerator for X scaling fraction
int xscaleden = 20;       // Denominator for X scaling fraction

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
bool FalseTouch = false;     // used to simulate a screen touch if a monitored event finishes

char DisplayType[31];
bool touchscreen_present = false;

///////////////////////////////////////////// FUNCTION PROTOTYPES ///////////////////////////////

void GetConfigParam(char *, char *, char *);
void SetConfigParam(char *, char *, char *);
int CheckWebCtlExists();
void ReadSavedParams();
void *WaitTouchscreenEvent(void * arg);
void *WaitMouseEvent(void * arg);
void *WebClickListener(void * arg);
void parseClickQuerystring(char *query_string, int *x_ptr, int *y_ptr);
FFUNC touchscreenClick(ffunc_session_t * session);
void take_snap();
void snap_from_menu();
void do_snapcheck();
int IsImageToBeChanged(int, int);
void Keyboard(char RequestText[63], char InitText[63], int MaxLength);
int openTouchScreen(int);
int getTouchScreenDetails(int*, int* ,int* ,int*);
void TransformTouchMap(int, int);
int IsButtonPushed(int, int, int);
int IsMenuButtonPushed(int, int);
void ChangeSmallMeterScale(int);
void ToggleLargeNumbers();
int InitialiseButtons();
int AddButton(int, int, int, int);
int ButtonNumber(int, int);
int CreateButton(int, int);
int AddButtonStatus(int, char *, color_t *);
void AmendButtonStatus(int, int, char *, color_t *);
void DrawButton(int);
void SetButtonStatus(int ,int);
int getTouchSampleThread(int *rawX, int *rawY, int *rawPressure);
int getTouchSample(int *rawX, int *rawY, int *rawPressure);
void UpdateWindow();
void wait_touch();
void SetSpanWidth(int);
void SetLimeGain(int);
void AdjustLimeGain(int);
void SetBaseline();
void SetSmoothing();
void SetHistSpan();
void SetFreqLimits(int button);
void SetFreqPreset(int);
void ShiftFrequency(int);
void CalcSpan();
void ChangeLabel(int);
void RedrawDisplay();
void *WaitButtonEvent(void * arg);
void Define_Menu1();
void Start_Highlights_Menu1();
void Define_Menu2();
void Start_Highlights_Menu2();
void Define_Menu3();
void Define_Menu4();
void Define_Menu5();
void Start_Highlights_Menu5();
void Define_Menu6();
void Start_Highlights_Menu6();
void Define_Menu7();
void Start_Highlights_Menu7();
void Define_Menu8();
void Define_Menu9();
void Define_Menu10();
void Start_Highlights_Menu10();
void Define_Menu41();
void DrawEmptyScreen();  
void DrawYaxisLabels();  
void DrawSettings();
void DrawTrace(int, int, int, int);
void DrawHistTrace(int, int, int, int);
void DrawMeterArc();
void DrawMeterTicks(int, int);
void Draw5MeterLabels();
void *MeterMovement(void * arg);
void CalibrateSystem();
static void cleanexit(int);
static void terminate(int sig);


//////////////////////////////////////////// SA bits /////////////////////////////////////////

static bool app_exit = false;

extern double frequency_actual_rx;
extern double bandwidth;
extern int y[260];

static pthread_t screen_thread_obj;
static pthread_t lime_thread_obj;
static pthread_t fft_thread_obj;

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
  char response[63]="0";

  strcpy(PlotTitle, "-");  // this is the "do not display" response
  GetConfigParam(PATH_CONFIG, "title", PlotTitle);

  GetConfigParam(PATH_CONFIG, "centrefreq", response);
  centrefreq = atoi(response);

  GetConfigParam(PATH_CONFIG, "span", response);
  span = atoi(response);

  CalcSpan();

  GetConfigParam(PATH_CONFIG, "limegain", response);
  limegain = atoi(response);
  gain = ((float)limegain) / 100.0;

  strcpy(response, "146500");
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

  strcpy(response, "absolute");
  GetConfigParam(PATH_CONFIG, "mode", mode);

  GetConfigParam(PATH_CONFIG, "smoothing", response);
  smoothing = atoi(response);
  noiseSmoothingFactor = 1.0 - (1.0 / (float)(smoothing + 1));
  carrierSmoothingFactor = 1.0 - (1.0 / ((float)(20 * smoothing) + 0.001));

  strcpy(response, "-80.0");
  GetConfigParam(PATH_CONFIG, "baseline", response);
  baseline = atof(response);

  strcpy(response, "100");
  GetConfigParam(PATH_CONFIG, "historyspan", response);
  historyspan = atoi(response);

  strcpy(response, "5");
  GetConfigParam(PATH_CONFIG, "refstart", response);
  refstart = atoi(response);
  refstartpixel = (refstart * 248 / 100) + 4;

  strcpy(response, "45");
  GetConfigParam(PATH_CONFIG, "refend", response);
  refend = atoi(response);
  refendpixel = (refend * 248 / 100) + 4;

  strcpy(response, "55");
  GetConfigParam(PATH_CONFIG, "sigstart", response);
  sigstart = atoi(response);
  sigstartpixel = (sigstart * 248 / 100) + 4;

  strcpy(response, "95");
  GetConfigParam(PATH_CONFIG, "sigend", response);
  sigend = atoi(response);
  sigendpixel = (sigend * 248 / 100) + 4;

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

  GetConfigParam(PATH_ICONFIG, "samodesel", response);
  if (strcmp(response, "auto") == 0)
  {
    samodeauto = true;
  }
  else
  {
    samodeauto = false;
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

  if (scaledY >= hscreen/2)
  {
    return 0;
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
  //setForeColour(255, 255, 255);    // White text
  //setBackColour(0, 0, 0);          // on Black
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
      //setForeColour(255, 255, 255);    // White text
      //setBackColour(0, 0, 0);          // on Black
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
        usleep(300000);
        ShiftStatus = 2 - (2 * KeyboardShift); // 0 = Upper, 2 = lower
        SetButtonStatus(ButtonNumber(41, token), ShiftStatus);
        DrawButton(ButtonNumber(41, token));
        publish();

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

  // Check for small meter scale "buttons"
  if ((scaledX <= 600) && (scaledX >= 350)
   && (scaledY <= 470) && (scaledY >= 270))
  {
    ChangeSmallMeterScale(scaledX);
  }

  // Check for Large Number select/deselct
  if ((scaledX <= 300) && (scaledX >= 100)
   && (scaledY <= 270) && (scaledY >= 70))
  {
    ToggleLargeNumbers();
  } 

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


void ChangeSmallMeterScale(int scaledX)
{
  // Decode touch
  if (scaledX >= 475)
  {
    MeterScale = MeterScale + 1;
  }
  else
  {
    MeterScale = MeterScale - 1;
  }

  // Limit scales
  if (MeterScale < 1)
  {
    MeterScale = 1;
  }
  if (MeterScale > 6)
  {
    MeterScale = 6;
  }

  // Select scales
  switch (MeterScale)
  {
    case 1:                   // 0 - 30
      ActiveZero = 0.0;
      ActiveFSD = 30.0;
    break;
    case 2:                   // 20 - 30
      ActiveZero = 20.0;
      ActiveFSD = 30.0;
    break;
    case 3:                   // 10 - 20
      ActiveZero = 10.0;
      ActiveFSD = 20.0;
    break;
    case 4:                   // 0 - 10
      ActiveZero = 0.0;
      ActiveFSD = 10.0;
    break;
    case 5:                   // 0 - 5
      ActiveZero = 0.0;
      ActiveFSD = 5.0;
    break;
    case 6:                   // -5 - 5
      ActiveZero = -5.0;
      ActiveFSD = 5.0;
    break;
  }
  ModeChanged = true;
  DrawYaxisLabels();
} 


void ToggleLargeNumbers()
{
  if (LargeNumbers == true)
  {
    LargeNumbers = false;
  }
  else
  {
    LargeNumbers = true;
  }
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

  if ((MenuIndex != 41) && (MenuIndex != 1) && (MenuIndex != 6))   // All except Main, keyboard, Span
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
  else if (MenuIndex == 1)   // Main Menu
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

    if ((ButtonPosition > 0) && (ButtonPosition < 6))  // 6 right hand buttons
    {
      x = normal_xpos;
      y = 480 - (ButtonPosition * 60);
      w = normal_width;
      h = 50;
    }
    if (ButtonPosition == 6) // 10
    {
      x = normal_xpos;  
      y = 480 - (6 * 60);
      w = 50;
      h = 50;
    }
    if (ButtonPosition == 7) // 20
    {
      x = 710;  // = normal_xpos + 50 button width + 20 gap
      y = 480 - (6 * 60);
      w = 50;
      h = 50;
    }
    if ((ButtonPosition > 7) && (ButtonPosition < 10))  // Bottom 2 buttons
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

  // Draw a black rectangle where the buttons are to erase them
  rectangle(620, 0, 160, 480, 0, 0, 0);

  // Draw each button in turn
  first = ButtonNumber(CurrentMenu, 0);
  last = ButtonNumber(CurrentMenu + 1 , 0) - 1;
  for(i = first; i <= last; i++)
  {
    if (ButtonArray[i].IndexStatus > 0)  // If button needs to be drawn
    {
      DrawButton(i);                     // Draw the button
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
      span = 512;
    break;
    case 3:
      span = 1024;
    break;
    case 4:
      span = 2048;
    break;
    case 5:
      span = 5120;
    break;
    case 6:
      span = 10240;
    break;
    case 7:
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
  Calibrated = false;
}


void SetLimeGain(int button)
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
      limegain = 100;
    break;
    case 3:
      limegain = 90;
    break;
    case 4:
      limegain = 70;
    break;
    case 5:
      limegain = 50;
    break;
    case 6:
      limegain = 30;
    break;
  }

  // Store the new gain
  snprintf(ValueToSave, 63, "%d", limegain);
  SetConfigParam(PATH_CONFIG, "limegain", ValueToSave);
  printf("limegain set to %d \n", limegain);

  // Trigger the gain change
  gain = ((float)limegain) / 100.0;
  NewGain = true;
  Calibrated = false;
  freeze = false;
}


void AdjustLimeGain(int button)
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
    case 3:      // up
      if (limegain >= 95)
      {
        limegain = 100;
      }
      else
      {
        limegain = limegain + 5;
      }
    break;
    case 5:      // down
      if (limegain <= 5)
      {
        limegain = 0;
      }
      else
      {
        limegain = limegain - 5;
      }
    break;
  }

  // Store the new gain
  snprintf(ValueToSave, 63, "%d", limegain);
  SetConfigParam(PATH_CONFIG, "limegain", ValueToSave);
  printf("limegain set to %d \n", limegain);

  // Trigger the gain change
  gain = ((float)limegain) / 100.0;
  NewGain = true;
  Calibrated = false;
  freeze = false;
}


void SetBaseline()
{
  char ValueToSave[63];
  char RequestText[64];
  char InitText[63];
  float newBaseline;

  // Don't do anything (so return) unless "absolute" or "absolute carrier mode is selected
  if ((strcmp(mode, "absolute") != 0) && (strcmp(mode, "abscarrier") != 0))
  {
    return;
  }

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  // Define request string
  strcpy(RequestText, "Enter the new Baseline (range 0 to -80.0) in dB");

  // Define initial value
  snprintf(InitText, 10, "%.1f", baseline);
 
  // Ask for the new value
  do
  {
    Keyboard(RequestText, InitText, 10);
    newBaseline = atof(KeyboardReturn);
  }
  while ((newBaseline > 0) || (newBaseline < -80.0));

  baseline = newBaseline;
  snprintf(ValueToSave, 63, "%.1f", newBaseline);

  // Store the new Baseline and recalculate
  SetConfigParam(PATH_CONFIG, "baseline", ValueToSave);
  printf("new baseline set to %.1f \n", baseline);

  // Tidy up, paint around the screen and then unfreeze
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  ModeChanged = true; // Redraw the meter scale
  freeze = false;
}


void SetSmoothing()
{  
  char ValueToSave[63];
  char RequestText[64];
  char InitText[63];
  int newSmoothing;

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  // Define request string
  strcpy(RequestText, "Enter the new number of smoothing scans");

  // Define initial value
  snprintf(InitText, 10, "%d", smoothing);
 
  // Ask for the new value
  do
  {
    Keyboard(RequestText, InitText, 10);
    newSmoothing = atoi(KeyboardReturn);
  }
  while ((newSmoothing < 1) || (newSmoothing > 1000));

  smoothing = newSmoothing;
  noiseSmoothingFactor = 1.0 - (1.0 / ((float)smoothing + 0.001));
  carrierSmoothingFactor = 1.0 - (1.0 / ((float)(20 * smoothing) + 0.001));
  snprintf(ValueToSave, 63, "%d", newSmoothing);

  // Store the new smoothing
  SetConfigParam(PATH_CONFIG, "smoothing", ValueToSave);
  printf("new smoothing set to %d \n", smoothing);

  // Tidy up, paint around the screen and then unfreeze
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  ModeChanged = true; // Redraw the meter scale
  UpdateWindow();     // Redraw the menu
  freeze = false;
}


void SetHistSpan()
{  
  char ValueToSave[63];
  char RequestText[64];
  char InitText[63];
  int newhistoryspan;

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  // Define request string
  strcpy(RequestText, "Enter the time span (sec) for the history");

  // Define initial value
  snprintf(InitText, 10, "%d", historyspan);
 
  // Ask for the new value
  do
  {
    Keyboard(RequestText, InitText, 10);
    newhistoryspan = atoi(KeyboardReturn);
  }
  while ((newhistoryspan < 1) || (newhistoryspan > 10000));

  historyspan = newhistoryspan;
  snprintf(ValueToSave, 63, "%d", newhistoryspan);

  // Store the new span
  SetConfigParam(PATH_CONFIG, "historyspan", ValueToSave);
  printf("new history span set to %d \n", historyspan);

  // Tidy up, paint around the screen and then unfreeze
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  ModeChanged = true; // Redraw the meter scale
  UpdateWindow();     // Redraw the menu
  freeze = false;
}


void SetFreqLimits(int button)
{
  char ValueToSave[63];
  char RequestText[64];
  char InitText[63];
  int newLimit;

  // Stop the scan at the end of the current one and wait for it to stop
  freeze = true;
  while(! frozen)
  {
    usleep(10);                                   // wait till the end of the scan
  }

  switch (button)
  {
    case 2:                                            // Set Lower Reference
      // Define request string
      strcpy(RequestText, "Enter lower span % for the reference band");

      // Define initial value
      snprintf(InitText, 10, "%d", refstart);
 
      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
        newLimit = atoi(KeyboardReturn);
      }
      while ((newLimit < 0) || (newLimit > 100));

      refstart = newLimit;
      snprintf(ValueToSave, 63, "%d", refstart);

      // Store the new lower reference start
      SetConfigParam(PATH_CONFIG, "refstart", ValueToSave);
      printf("new refstart set to %d \n", refstart);
      refstartpixel = (refstart * 248 / 100) + 4;
    break;
    case 3:                                            // Set Upper Reference
      // Define request string
      strcpy(RequestText, "Enter upper span % for the reference band");

      // Define initial value
      snprintf(InitText, 10, "%d", refend);
 
      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
        newLimit = atoi(KeyboardReturn);
      }
      while ((newLimit < 0) || (newLimit > 100));

      refend = newLimit;
      snprintf(ValueToSave, 63, "%d", refend);

      // Store the new upper reference end
      SetConfigParam(PATH_CONFIG, "refend", ValueToSave);
      printf("new refend set to %d \n", refend);
      refendpixel = (refend * 248 / 100) + 4;
    break;
    case 4:                                            // Set Lower Signal
      // Define request string
      strcpy(RequestText, "Enter lower span % for the signal band");

      // Define initial value
      snprintf(InitText, 10, "%d", sigstart);
 
      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
        newLimit = atoi(KeyboardReturn);
      }
      while ((newLimit < 0) || (newLimit > 100));

      sigstart = newLimit;
      snprintf(ValueToSave, 63, "%d", sigstart);

      // Store the new signal start
      SetConfigParam(PATH_CONFIG, "sigstart", ValueToSave);
      printf("new sigstart set to %d \n", sigstart);
      sigstartpixel = (sigstart * 248 / 100) + 4;
    break;
    case 5:                                            // Set Upper Signal
      // Define request string
      strcpy(RequestText, "Enter upper span % for the signal band");

      // Define initial value
      snprintf(InitText, 10, "%d", sigend);
 
      // Ask for the new value
      do
      {
        Keyboard(RequestText, InitText, 10);
        newLimit = atoi(KeyboardReturn);
      }
      while ((newLimit < 0) || (newLimit > 100));

      sigend = newLimit;
      snprintf(ValueToSave, 63, "%d", sigend);

      // Store the new signal end
      SetConfigParam(PATH_CONFIG, "sigend", ValueToSave);
      printf("new sigend set to %d \n", refend);
      sigendpixel = (sigend * 248 / 100) + 4;
    break;
  }

  // Tidy up, paint around the screen and then unfreeze
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  ModeChanged = true; // Redraw the meter scale
  UpdateWindow();     // Redraw the menu
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
    Calibrated = false;
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
      snprintf(InitText, 31, "%d.%03d", div_1000.quot, div_1000.rem);
    }
    else
    {
      div_100 = div(amendfreq, 100);
      if(div_100.rem != 0)  // last but one character not zero, so make answer of form xxx.xx
      {
        snprintf(InitText, 31, "%d.%02d", div_1000.quot, div_1000.rem / 10);
      }
      else
      {
        if(div_1000.rem != 0)  // last but two character not zero, so make answer of form xxx.x
        {
          snprintf(InitText, 31, "%d.%d", div_1000.quot, div_1000.rem / 100);
        }
        else  // integer MHz, so just xxx (no dp)
        {
          snprintf(InitText, 31, "%d", div_1000.quot);
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
    Calibrated = false;
  }
  ModeChanged = true;
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
  Calibrated = false;
  ModeChanged = true;
}


void CalcSpan()    // takes centre frequency and span and calulates startfreq and stopfreq
{
  startfreq = centrefreq - (span * 125) / 256;
  stopfreq =  centrefreq + (span * 125) / 256;
  frequency_actual_rx = 1000.0 * (float)(centrefreq);
  bandwidth = (float)(span * 1000);
  nf_bandwidth = span * 250 / 256;   //nf bandwidth in kHz

  // set a sensible time constant for the fft display
  if (bandwidth >= 2048000)
  {
    MAIN_SPECTRUM_TIME_SMOOTH =  0.98;
  }
  else
  {
    MAIN_SPECTRUM_TIME_SMOOTH =  0.90;
  }

  // Set levelling time for Noise power Measurement
  switch (span)
  {
    case 512:                                            // 500 kHz use 15
      ScansforLevel = 15;
      break;
    case 1024:                                            // 1 MHz use 10
      ScansforLevel = 10;
      break;
    case 2048:                                            // 2 MHz use 10
      ScansforLevel = 10;
      break;
    case 5120:                                            // 5 MHz use 10
      ScansforLevel = 10;
      break;
    case 10240:                                            // 10 MHz use 10
      ScansforLevel = 10;
      break;
    case 20480:                                            // 20 MHz use 10
      ScansforLevel = 10;
      break;
    default:
      ScansforLevel = 10;
  }
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
        snprintf(InitText, 31, "%d.%03d", div_1000.quot, div_1000.rem);
      }
      else
      {
        div_100 = div(centrefreq, 100);
        if(div_100.rem != 0)  // last but one character not zero, so make answer of form xxx.xx
        {
          snprintf(InitText, 31, "%d.%02d", div_1000.quot, div_1000.rem / 10);
        }
        else
        {
          if(div_1000.rem != 0)  // last but two character not zero, so make answer of form xxx.x
          {
            snprintf(InitText, 31, "%d.%d", div_1000.quot, div_1000.rem / 100);
          }
          else  // integer MHz, so just xxx (no dp)
          {
            snprintf(InitText, 10, "%d", div_1000.quot);
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
      ModeChanged = true;
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
  }

  // Tidy up, paint around the screen and then unfreeze
  CalcSpan();         // work out start and stop freqs
  clearScreen(0, 0, 0);
  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  ModeChanged = true;
  UpdateWindow();     // Redraw the menu
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
          CurrentMenu=3;
          UpdateWindow();
          break;
        case 3:                                            // Set-up
          printf("Set-up Menu 2 Requested\n");
          CurrentMenu = 2;
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 4:                                            // Mode
          printf("Mode Menu 5 Requested\n");
          CurrentMenu = 5;
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 5:                                            // Left Arrow
        case 6:                                            // Right Arrow
          printf("Shift Frequency Requested\n");
          ShiftFrequency(i);
          break;
        case 7:                                            // System
          printf("System Menu 4 Requested\n");
          CurrentMenu=4;
          UpdateWindow();
          break;
        case 8:                                            // Exit to Portsdown
          if(PortsdownExitRequested)
          {
            freeze = true;
            usleep(100000);
            //setBackColour(0, 0, 0);
            clearScreen(0, 0, 0);
            publish();
            UpdateWeb();
            usleep(1000000);
            closeScreen();
            cleanexit(207);
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

    if (CurrentMenu == 2)  // Set-up Menu
    {
      printf("Button Event %d, Entering Menu 2 Case Statement\n",i);
      CallingMenu = 2;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Set Baseline
          SetBaseline();
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 3:                                            // Lime Gain Up
        case 5:                                            // Lime Gain Down
          BLCalibrated = false;
          AdjustLimeGain(i);
          BLCalibrated = false;
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 4:                                            // No action, show Lime Gain
          break;
        case 6:                                            // Establish baseline in absolute noise mode
          if (strcmp(mode, "absolute") == 0)
          {
            CalibrateBLRequested = true;
            Start_Highlights_Menu2();
            UpdateWindow();
          }
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
          break;
        case 3:                                            // Frequency Presets
          printf("Frequency Preset Menu 7 Requested\n");
          CurrentMenu = 7;
          Start_Highlights_Menu7();
          UpdateWindow();
          break;
        case 4:                                            // Span Width
          printf("Span Width Menu 6 Requested\n");
          CurrentMenu = 6;
          Start_Highlights_Menu6();
          UpdateWindow();
          break;
        case 5:                                            // Set-up Menu
          printf("Set-up Menu 2 Requested\n");
          CurrentMenu=2;
          Start_Highlights_Menu2();
          UpdateWindow();
          break;
        case 6:                                            // Config Menu
          printf("Config Menu 9 Requested\n");
          CurrentMenu=9;
          //Start_Highlights_Menu9();
          UpdateWindow();          
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
          DrawMeterArc();
          DrawMeterTicks(5, 1);
          Draw5MeterLabels();
          freeze = false;           // Restart the scan
          break;
        case 3:                                            // Restart Noise Meter
          freeze = true;
          usleep(100000);
          //setBackColour(0, 0, 0);
          clearScreen(0, 0, 0);
          usleep(1000000);
          closeScreen();
          cleanexit(147);
          break;
        case 4:                                            // Exit to Portsdown
          freeze = true;
          usleep(100000);
          //setBackColour(0, 0, 0);
          clearScreen(0, 0, 0);
          usleep(1000000);
          closeScreen();
          cleanexit(207);
          break;
        case 5:                                            // Shutdown
          system("sudo shutdown now");
          break;
        case 6:                                            // ReCal Lime
          NewCal = true;
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
        case 2:                                            // Absolute noise measurement
          strcpy(mode, "absolute");
          SetConfigParam(PATH_CONFIG, "mode", "absolute");
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 3:                                            // Differential noise measurement
          strcpy(mode, "differential");
          SetConfigParam(PATH_CONFIG, "mode", "differential");
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 4:                                            // Carrier over Noise Level Measurement
          strcpy(mode, "carrier");
          SetConfigParam(PATH_CONFIG, "mode", "carrier");
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 5:                                            // Absolute Carrier Level Measurement
          strcpy(mode, "abscarrier");
          SetConfigParam(PATH_CONFIG, "mode", "abscarrier");
          Start_Highlights_Menu5();
          UpdateWindow();
          break;
        case 6:                                            // Config Menu
          printf("Config Menu 9 Requested\n");
          CurrentMenu = 9;
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
        case 2:                                            // 0.5
        case 3:                                            // 1
        case 4:                                            // 2
        case 5:                                            // 5
        case 6:                                            // 10
        case 7:                                            // 20
          SetSpanWidth(i);
          CurrentMenu = 6;
          Start_Highlights_Menu6();
          UpdateWindow();
          break;
        case 8:                                            // Return to Settings Menu
          CurrentMenu=3;
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

    if (CurrentMenu == 8)  // 2nd Config Menu
    {
      printf("Button Event %d, Entering Menu 8 Case Statement\n",i);
      CallingMenu = 8;
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
        case 3:                                            // Set Title
          ChangeLabel(6);
          break;
        case 4:                                            // Set Smoothing
          SetSmoothing();
          break;
        case 5:                                            // Set the history span
          SetHistSpan();
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
          printf("Menu 8 Error\n");
      }
      continue;  // Completed Menu 8 action, go and wait for touch
    }


    if (CurrentMenu == 9)  // First Config Menu
    {
      printf("Button Event %d, Entering Menu 9 Case Statement\n",i);
      CallingMenu = 9;
      switch (i)
      {
        case 0:                                            // Capture Snap
          snap_from_menu();
          break;
        case 2:                                            // Freq Limits for differential
        case 3:                                            // 
        case 4:                                            // 
        case 5:                                            //
          SetFreqLimits(i); 
          break;
        case 6:                                            // 
          printf("2nd Config Menu 8 Requested\n");
          CurrentMenu=8;
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
  AddButtonStatus(button, "Freq and^Bandwidth", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(1, 3);
  AddButtonStatus(button, "Set-up", &Blue);
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


void Define_Menu2()                                         // Set-up Menu
{
  int button = 0;

  button = CreateButton(2, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(2, 1);
  AddButtonStatus(button, "Set-up^Menu", &Black);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(2, 2);
  AddButtonStatus(button, "Set^Baseline", &Blue);
  AddButtonStatus(button, "Set^Baseline", &Grey);

  button = CreateButton(2, 3);
  AddButtonStatus(button, "Gain Up", &Blue);
  AddButtonStatus(button, "Gain Up", &Grey);

  button = CreateButton(2, 4);
  AddButtonStatus(button, "Gain", &Black);

  button = CreateButton(2, 5);
  AddButtonStatus(button, "Gain Down", &Blue);
  AddButtonStatus(button, "Gain Down", &Grey);

  button = CreateButton(2, 6);
  AddButtonStatus(button, "Establish^Baseline", &Blue);
  AddButtonStatus(button, "Measuring", &Green);
  AddButtonStatus(button, "Baseline^Established", &Green);
  AddButtonStatus(button, "Establish^Baseline", &Grey);

  button = CreateButton(2, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(2, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu2()
{
  char ButtText[31];
  snprintf(ButtText, 30, "Gain=%d%%", limegain);
  AmendButtonStatus(ButtonNumber(CurrentMenu, 4), 0, ButtText, &Black);

  if (Calibrated == true)  // 
  {
    SetButtonStatus(ButtonNumber(2, 2), 1);
    SetButtonStatus(ButtonNumber(2, 3), 1);
    SetButtonStatus(ButtonNumber(2, 5), 1);
    SetButtonStatus(ButtonNumber(2, 6), 2);
  }
  else
  {
    SetButtonStatus(ButtonNumber(2, 2), 0);
    SetButtonStatus(ButtonNumber(2, 3), 0);
    SetButtonStatus(ButtonNumber(2, 5), 0);

    if (CalibrateRequested == true)
    {
      SetButtonStatus(ButtonNumber(2, 6), 1);
    }
    else
    {
      SetButtonStatus(ButtonNumber(2, 6), 0);
    }
  }
  if ((strcmp(mode, "differential") == 0) || (strcmp(mode, "carrier") == 0) || (strcmp(mode, "abscarrier") == 0))
  {
    SetButtonStatus(ButtonNumber(2, 2), 1);
    SetButtonStatus(ButtonNumber(2, 6), 3);

    if (strcmp(mode, "abscarrier") == 0)
    {
      SetButtonStatus(ButtonNumber(2, 2), 0);  // Allow baseline to be established for absolute carrier mode
    }
  }
  else
  {
    if (CalibrateBLRequested == true)
    {
      SetButtonStatus(ButtonNumber(2, 2), 0);
      SetButtonStatus(ButtonNumber(2, 6), 1);
    }
    else if (BLCalibrated == true)
    {
      SetButtonStatus(ButtonNumber(2, 2), 0);
      SetButtonStatus(ButtonNumber(2, 6), 2);
    }
    else
    {
      SetButtonStatus(ButtonNumber(2, 2), 0);
      SetButtonStatus(ButtonNumber(2, 6), 0);
    }
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
  AddButtonStatus(button, "Set-up^Menu", &Blue);

  button = CreateButton(3, 6);
  AddButtonStatus(button, "Config^Menu", &Blue);

  button = CreateButton(3, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(3, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
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
  AddButtonStatus(button, "Re-start^Noise Meter", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 4);
  AddButtonStatus(button, "Exit to^Portsdown", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 5);
  AddButtonStatus(button, "Shutdown^System", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 6);
  AddButtonStatus(button, "Re-cal^LimeSDR", &Blue);
  AddButtonStatus(button, " ", &Green);

  button = CreateButton(4, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(4, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
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
  AddButtonStatus(button, "Absolute^Noise", &Blue);
  AddButtonStatus(button, "Absolute^Noise", &Green);

  button = CreateButton(5, 3);
  AddButtonStatus(button, "Differential^Noise", &Blue);
  AddButtonStatus(button, "Differential^Noise", &Green);

  button = CreateButton(5, 4);
  AddButtonStatus(button, "Carrier/^Noise Level", &Blue);
  AddButtonStatus(button, "Carrier/^Noise Level", &Green);

  button = CreateButton(5, 5);
  AddButtonStatus(button, "Carrier^Abs Level", &Blue);
  AddButtonStatus(button, "Carrier^Abs Level", &Green);

  button = CreateButton(5, 6);
  AddButtonStatus(button, "Config^Menu", &Blue);

  button = CreateButton(5, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(5, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu5()
{
  if (strcmp(mode, "absolute") == 0)
  {
    SetButtonStatus(ButtonNumber(5, 2), 1);
    SetButtonStatus(ButtonNumber(5, 3), 0);
    SetButtonStatus(ButtonNumber(5, 4), 0);
    SetButtonStatus(ButtonNumber(5, 5), 0);
  }
  if (strcmp(mode, "differential") == 0)
  {
    SetButtonStatus(ButtonNumber(5, 2), 0);
    SetButtonStatus(ButtonNumber(5, 3), 1);
    SetButtonStatus(ButtonNumber(5, 4), 0);
    SetButtonStatus(ButtonNumber(5, 5), 0);
  }
  if (strcmp(mode, "carrier") == 0)
  {
    SetButtonStatus(ButtonNumber(5, 2), 0);
    SetButtonStatus(ButtonNumber(5, 3), 0);
    SetButtonStatus(ButtonNumber(5, 4), 1);
    SetButtonStatus(ButtonNumber(5, 5), 0);
  }
  if (strcmp(mode, "abscarrier") == 0)
  {
    SetButtonStatus(ButtonNumber(5, 2), 0);
    SetButtonStatus(ButtonNumber(5, 3), 0);
    SetButtonStatus(ButtonNumber(5, 4), 0);
    SetButtonStatus(ButtonNumber(5, 5), 1);
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
  AddButtonStatus(button, "500 kHz", &Blue);
  AddButtonStatus(button, "500 kHz", &Green);

  button = CreateButton(6, 3);
  AddButtonStatus(button, "1 MHz", &Blue);
  AddButtonStatus(button, "1 MHz", &Green);

  button = CreateButton(6, 4);
  AddButtonStatus(button, "2 MHz", &Blue);
  AddButtonStatus(button, "2 MHz", &Green);

  button = CreateButton(6, 5);
  AddButtonStatus(button, "5 MHz", &Blue);
  AddButtonStatus(button, "5 MHz", &Green);

  button = CreateButton(6, 6);
  AddButtonStatus(button, "10", &Blue);
  AddButtonStatus(button, "10", &Green);

  button = CreateButton(6, 7);
  AddButtonStatus(button, "20", &Blue);
  AddButtonStatus(button, "20", &Green);

  button = CreateButton(6, 8);
  AddButtonStatus(button, "Back to^Settings", &DBlue);

  button = CreateButton(6, 9);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
}


void Start_Highlights_Menu6()
{
  if (span == 512)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 2), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 2), 0);
  }
  if (span == 1024)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 3), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 3), 0);
  }
  if (span == 2048)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 4), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 4), 0);
  }
  if (span == 5120)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 5), 0);
  }
  if (span == 10240)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 6), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 6), 0);
  }
  if (span == 20480)
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 7), 1);
  }
  else
  {
    SetButtonStatus(ButtonNumber(CurrentMenu, 7), 0);
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


void Define_Menu8()                                          // 2nd Config Menu
{
  int button = 0;

  button = CreateButton(8, 0);
  AddButtonStatus(button, "Capture^Snap", &DGrey);
  AddButtonStatus(button, " ", &Black);

  button = CreateButton(8, 1);
  AddButtonStatus(button, "2nd Config^Menu", &Black);

  button = CreateButton(8, 2);
  AddButtonStatus(button, "Set Freq^Presets", &Blue);

  button = CreateButton(8, 3);
  AddButtonStatus(button, "Set^Title", &Blue);
  //AddButtonStatus(button, " ", &Green);

  button = CreateButton(8, 4);
  AddButtonStatus(button, "Set^Smoothing", &Blue);

  button = CreateButton(8, 5);
  AddButtonStatus(button, "History^Span", &Blue);

  //button = CreateButton(8 6);
  //AddButtonStatus(button, "Set^Config", &Blue);

  button = CreateButton(8, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(8, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
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
  AddButtonStatus(button, "Set Lower^Reference", &Blue);

  button = CreateButton(9, 3);
  AddButtonStatus(button, "Set Upper^Reference", &Blue);

  button = CreateButton(9, 4);
  AddButtonStatus(button, "Set Lower^Signal", &Blue);

  button = CreateButton(9, 5);
  AddButtonStatus(button, "Set Upper^Signal", &Blue);

  button = CreateButton(9, 6);
  AddButtonStatus(button, "More^Config", &Blue);

  button = CreateButton(9, 7);
  AddButtonStatus(button, "Return to^Main Menu", &DBlue);

  button = CreateButton(9, 8);
  AddButtonStatus(button, "Freeze", &Blue);
  AddButtonStatus(button, "Unfreeze", &Green);
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
  clearScreen(0, 0, 0);

  // Spectrum
  HorizLine(100, 270, 250, 255, 255, 255);
  VertLine(100, 270, 200, 255, 255, 255);
  HorizLine(100, 470, 250, 255, 255, 255);
  VertLine(350, 270, 200, 255, 255, 255);

  HorizLine(101, 320, 249, 63, 63, 63);
  HorizLine(101, 370, 249, 63, 63, 63);
  HorizLine(101, 420, 249, 63, 63, 63);

  VertLine(150, 271, 199, 63, 63, 63);
  VertLine(200, 271, 199, 63, 63, 63);
  VertLine(250, 271, 199, 63, 63, 63);
  VertLine(300, 271, 199, 63, 63, 63);

  // Timeline
  HorizLine(350, 40,  250, 255, 255, 255);
  VertLine (350, 40,  200, 255, 255, 255);
  HorizLine(350, 240, 250, 255, 255, 255);
  VertLine (600, 40,  200, 255, 255, 255);

  HorizLine(351,  80, 249, 63, 63, 63);
  HorizLine(351, 120, 249, 63, 63, 63);
  HorizLine(351, 160, 249, 63, 63, 63);
  HorizLine(351, 200, 249, 63, 63, 63);

  VertLine(400, 41, 199, 63, 63, 63);
  VertLine(450, 41, 199, 63, 63, 63);
  VertLine(500, 41, 199, 63, 63, 63);
  VertLine(550, 41, 199, 63, 63, 63);
}


void DrawYaxisLabels()
{
  const font_t *font_ptr = &font_dejavu_sans_18;   // 18pt
  char labeltext[31];

  // Clear the previous scales first
  rectangle( 25, 263, 65, 217, 0, 0, 0);
  rectangle(320,  31, 29, 220, 0, 0, 0);

  // Spectrum
  Text(48, 462, "0 dB", font_ptr, 0, 0, 0, 255, 255, 255);
  Text(30, 416, "-20 dB", font_ptr, 0, 0, 0, 255, 255, 255);
  Text(30, 366, "-40 dB", font_ptr, 0, 0, 0, 255, 255, 255);
  Text(30, 316, "-60 dB", font_ptr, 0, 0, 0, 255, 255, 255);
  Text(30, 266, "-80 dB", font_ptr, 0, 0, 0, 255, 255, 255);

  // History
  snprintf(labeltext, 14, "%d", (int)(ActiveZero + 4 * (ActiveFSD - ActiveZero) / 5));
  Text(325, 194, labeltext, font_ptr, 0, 0, 0, 255, 255, 255);
  snprintf(labeltext, 14, "%d", (int)(ActiveZero + 3 * (ActiveFSD - ActiveZero) / 5));
  Text(325, 154, labeltext, font_ptr, 0, 0, 0, 255, 255, 255);
  snprintf(labeltext, 14, "%d", (int)(ActiveZero + 2 * (ActiveFSD - ActiveZero) / 5));
  Text(325, 114, labeltext, font_ptr, 0, 0, 0, 255, 255, 255);
  snprintf(labeltext, 14, "%d", (int)(ActiveZero + (ActiveFSD - ActiveZero) / 5));
  Text(325,  74, labeltext, font_ptr, 0, 0, 0, 255, 255, 255);
  snprintf(labeltext, 14, "%d", (int)ActiveZero);
  Text(325,  34, labeltext, font_ptr, 0, 0, 0, 255, 255, 255);
}


void DrawSettings()
{
  const font_t *font_ptr = &font_dejavu_sans_18;   // 18pt
  char DisplayText[63];
  float ParamAsFloat;
  int line1y = 245;
  int line2y = 215;
  int titley = 5;

  // Title at bottom of screen
  rectangle(100, 0, 505, 30, 0, 0, 0);                                // Clear previous text
  if (strcmp(PlotTitle, "-") != 0)
  {
    TextMid(350, titley, PlotTitle, &font_dejavu_sans_22, 0, 0, 0, 255, 255, 255);
  }

  // Centre Freq beneath specturum
  ParamAsFloat = (float)centrefreq / 1000.0;
  snprintf(DisplayText, 63, "Centre Freq %5.2f MHz", ParamAsFloat);
  rectangle(100, line1y - 5, 250, 25, 0, 0, 0);                        // Clear previous text
  Text(100, line1y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);

  // Bandwidth and History Span in bottom left quadrant - only display if Large Numbers not displayed
  if (LargeNumbers == false)
  {
    // Display bandwidth
    rectangle(100, line2y - 5, 250, 25, 0, 0, 0);     // Clear previous text
    if ((nf_bandwidth > 0) && (nf_bandwidth < 1000))  // valid and less than 1000 kHz
    {
      ParamAsFloat = (float)nf_bandwidth;
      snprintf(DisplayText, 63, "Bandwidth %5.1f kHz", ParamAsFloat);
      Text(100, line2y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);
    }
    else if (nf_bandwidth >= 1000)                  // valid and greater 1000 kHz
    {
      ParamAsFloat = (float)nf_bandwidth / 1000.0;
      snprintf(DisplayText, 63, "Bandwidth %5.1f MHz", ParamAsFloat);
      Text(100, line2y, DisplayText, font_ptr, 0, 0, 0, 255, 255, 255);
    }

    // Display History scan width
    rectangle(100, 55, 200, 22, 0, 0, 0);  // Blank area
    snprintf(DisplayText, 30, "History span %d sec", historyspan);
    //setBackColour(0, 0, 0);
    Text(100, 60, DisplayText, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);
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
    //setPixelNoA(xpos, ypospix, column[ypos], column[ypos], 0);
    setPixel(xpos, 479 - ypospix, column[ypos], column[ypos], 0);  
  }

  // Draw the background and grid (except in the active trace) to erase the previous scan

  if (xpos % 50 == 0)  // vertical int graticule
  {
    for(ypos = 201; ypos < ymin; ypos++)
    {
      //setPixelNoAGra(xpos, 479 - (70 + ypos));
      setPixel(xpos, (70 + ypos), 64, 64, 64);
    }
    for(ypos = 399; ypos > ymax; ypos--)
    {
      //setPixelNoAGra(xpos, 479 - (70 + ypos));
      setPixel(xpos, (70 + ypos), 64, 64, 64);
    }
  }
  else  // horizontal graticule and open space
  {
    for(ypos = 201; ypos < ymin; ypos++)  // below the trace
    {
      ypospix = 409 - ypos;  // 409 = 479 - 70

      if (ypos % 50 == 0)  // graticule line
      {
        //setPixelNoAGra(xpos, ypospix);
        setPixel(xpos, 479 - ypospix, 64, 64, 64);
      }
      else
      {
        if (((strcmp(mode, "differential") == 0) || (strcmp(mode, "carrier") == 0)) && 
            (((xpos > 97 + refstartpixel ) && (xpos < 97 + refendpixel)) ||
            ((xpos > 97 + sigstartpixel) && (xpos < 97 + sigendpixel)))) // Show measurement frequencies
        {
          //setPixelNoAGra(xpos, ypospix);
          setPixel(xpos, 479 - ypospix, 64, 64, 64);
          if (((strcmp(mode, "carrier") == 0) || (strcmp(mode, "abscarrier") == 0)) && (xpos == peakLine + 98))  // Draw carrier marker line
          {
            //setPixel(xpos, ypospix, 0, 255, 0);
            setPixel(xpos, 479 - ypospix, 0, 255, 0);
          }
        }
        else if ((strcmp(mode, "abscarrier") == 0) &&
                ((xpos > 97 + sigstartpixel) && (xpos < 97 + sigendpixel))) // Show measurement frequencies
        {
          //setPixelNoAGra(xpos, ypospix);
          setPixel(xpos, 479 - ypospix, 64, 64, 64);

          if (((strcmp(mode, "carrier") == 0) || (strcmp(mode, "abscarrier") == 0)) && (xpos == peakLine + 98))  // Draw carrier marker line
          {
            //setPixelNoA(xpos, ypospix, 0, 255, 0);
            setPixel(xpos, 479 - ypospix, 0, 255, 0);
          }
        }
        else
        {
          //setPixelNoABlk(xpos, ypospix);
          setPixel(xpos, 479 - ypospix, 0, 0, 0);
        }
      }
    }
    for(ypos = 399; ypos > ymax; ypos--)  // above the trace
    {
      ypospix = 409 - ypos;  // 409 = 479 - 70
      if (ypos % 50 == 0)  // graticule line
      {
        //setPixelNoAGra(xpos, ypospix);
        setPixel(xpos, 479 - ypospix, 64, 64, 64);
      }
      else
      {
        //setPixelNoABlk(xpos, ypospix);
        setPixel(xpos, 479 - ypospix, 0, 0, 0);
      }
    }
  }
}


void DrawHistTrace(int xoffset, int prev2, int prev1, int current)
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

  //xpos = 99 + xoffset;  // we are going to draw the column before the current one
  xpos = 349 + xoffset;  // we are going to draw the column before the current one

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
    //ypospix = 409 - ypos;  //409 = 479 - 70
    ypospix = 639 - ypos;  //639 = 479 - 70 + 230
    //setPixelNoA(xpos, ypospix, column[ypos], column[ypos], 0);  
    setPixel(xpos, 479 - ypospix, column[ypos], column[ypos], 0);  
  }

  // Draw the background and grid (except in the active trace) to erase the previous scan

  if (xpos % 50 == 0)  // vertical int graticule
  {
    for(ypos = 201; ypos < ymin; ypos++)
    {
      //setPixelNoAGra(xpos, 479 - (70 + ypos));
      //setPixelNoAGra(xpos, 479 - (70 + ypos - 230));
      setPixel(xpos, 70 + ypos - 230, 64, 64, 64);
    }
    for(ypos = 399; ypos > ymax; ypos--)
    {
      //setPixelNoAGra(xpos, 479 - (70 + ypos));
      //setPixelNoAGra(xpos, 479 - (70 + ypos - 230));
      setPixel(xpos, 70 + ypos - 230, 64, 64, 64);
    }
  }
  else  // horizontal graticule and open space
  {
    for(ypos = 201; ypos < ymin; ypos++)  // below the trace
    {
      //ypospix = 409 - ypos;  // 409 = 479 - 70
      ypospix = 639 - ypos;  // 639 = 479 - 70 + 230

      if (ypos % 40 == 0)  // graticule line
      {
        //setPixelNoAGra(xpos, ypospix);
        setPixel(xpos, 479 - ypospix, 64, 64, 64);
      }
      else
      {
        //setPixelNoABlk(xpos, ypospix);
        setPixel(xpos, 479 - ypospix, 0, 0, 0);
      }
    }
    for(ypos = 399; ypos > ymax; ypos--)  // above the trace
    {
      //ypospix = 409 - ypos;  // 409 = 479 - 70
      ypospix = 639 - ypos;  // 639 = 479 - 70 + 230
      if (ypos % 40 == 0)  // graticule line
      {
        //setPixelNoAGra(xpos, ypospix);
        setPixel(xpos, 479 - ypospix, 64, 64, 64);
      }
      else
      {
        //setPixelNoABlk(xpos, ypospix);
        setPixel(xpos, 479 - ypospix, 0, 0, 0);
      }
    }
  }
}


void DrawMeterArc()
{
  // Draws an anti-aliased meter arc

  float x;
  float y;
  int previous_x = 364; // Left most position of arc - 1
  float current_sin;
  float current_cos;
  float arc_angle;
  int arc_deflection;
  float contrast;

  for (arc_deflection = 0; arc_deflection < 998; arc_deflection++)
  {
    arc_angle = (float)(arc_deflection) * 2 * PI / 4000.0 - PI / 4.0;
    current_sin = sin(arc_angle);
    current_cos = cos(arc_angle);

    x = 475 + 156 * current_sin;
    y = 290 + 156 * current_cos;

    if ((x - (float)previous_x) >= 1.0)  // new pixel pair required
    {
      // Work out contrast for lower pixel and paint
      contrast = (y - (int)y) * 255;
      setPixel(previous_x + 1, (int)y, contrast, contrast, contrast);

      // work out contrast for upper pixel and paint
      contrast = (1 - (y - (int)y)) * 255;
      setPixel(previous_x + 1, ((int)y - 1), contrast, contrast, contrast);

      previous_x = previous_x + 1;
    }
  }
}


void DrawMeterTicks(int major_ticks, int minor_ticks)
{
  float tick_deflection= 0;
  int x1;
  int x2;
  int y1;
  int y2;
  float current_sin;
  float current_cos;
  int major_tick_number;
  int minor_tick_number;

//  float major_tick_length = 20.0;
//  float minor_tick_length = 10.0;
  float major_tick_length = 10.0;
  float minor_tick_length = 5.0;

  // Draw the major ticks.  Start at zero
  for (major_tick_number = 0; major_tick_number <= major_ticks; major_tick_number++)
  {
    tick_deflection = (major_tick_number * 1000) / major_ticks;  // for majors

    current_sin = sin((float)tick_deflection * 2.0 * PI / 4000.0 - PI / 4.0);
    current_cos = cos((float)tick_deflection * 2.0 * PI / 4000.0 - PI / 4.0);

//    x1 = (int)(350.0 + 312.0 * current_sin);
//    y1 = (int)(90.0 + 312.0 * current_cos);
//    x2 = (int)(350.0 + (312.0 + major_tick_length) * current_sin);
//    y2 = (int)(90.0 + (312.0 + major_tick_length) * current_cos);
    x1 = (int)(475.0 + 156.0 * current_sin);
    y1 = (int)(290.0 + 156.0 * current_cos);
    x2 = (int)(475.0 + (156.0 + major_tick_length) * current_sin);
    y2 = (int)(290.0 + (156.0 + major_tick_length) * current_cos);

    DrawAALine(x1, y1, x2, y2, 0, 0, 0, 255, 255, 255);

    // Draw the minor ticks.  Start at one.  Don't draw for last major tick
    if (major_tick_number < major_ticks)
    {
      for (minor_tick_number = 1; minor_tick_number <= minor_ticks; minor_tick_number++)
      {
        tick_deflection = tick_deflection + (minor_tick_number * 1000) / major_ticks / (minor_ticks + 1);

        current_sin = sin((float)tick_deflection * 2.0 * PI / 4000.0 - PI / 4.0);
        current_cos = cos((float)tick_deflection * 2.0 * PI / 4000.0 - PI / 4.0);

//        x1 = (int)(350.0 + 312.0 * current_sin);
//        y1 = (int)(90.0 + 312.0 * current_cos);
//        x2 = (int)(350.0 + (312.0 + minor_tick_length) * current_sin);
//        y2 = (int)(90.0 + (312.0 + minor_tick_length) * current_cos);
        x1 = (int)(475.0 + 156.0 * current_sin);
        y1 = (int)(290.0 + 156.0 * current_cos);
        x2 = (int)(475.0 + (156.0 + minor_tick_length) * current_sin);
        y2 = (int)(290.0 + (156.0 + minor_tick_length) * current_cos);

        DrawAALine(x1, y1, x2, y2, 0, 0, 0, 255, 255, 255);
      }
    }
  }
}


void Draw5MeterLabels()
{
  char labeltext1[15];
  char labeltext2[15];
  char labeltext3[15];
  char labeltext4[15];
  char labeltext5[15];
  char labeltext6[15];
  const font_t *font_ptr = &font_dejavu_sans_18;   // 18pt
  //setBackColour(0, 0, 0);

  // Define the labels
  snprintf(labeltext1, 14, "%d", (int)ActiveZero);
  snprintf(labeltext2, 14, "%d", (int)(ActiveZero + (ActiveFSD - ActiveZero) / 5));
  snprintf(labeltext3, 14, "%d", (int)(ActiveZero + 2 * (ActiveFSD - ActiveZero) / 5));
  snprintf(labeltext4, 14, "%d", (int)(ActiveZero + 3 * (ActiveFSD - ActiveZero) / 5));
  snprintf(labeltext5, 14, "%d", (int)(ActiveZero + 4 * (ActiveFSD - ActiveZero) / 5));
  snprintf(labeltext6, 14, "%d", (int)ActiveFSD);

  // Clear the old labels
  rectangle(351, 432, 40, 30, 0, 0, 0);
  rectangle(378, 454, 40, 25, 0, 0, 0);
  rectangle(430, 465, 40, 15, 0, 0, 0);
  rectangle(482, 465, 40, 15, 0, 0, 0);
  rectangle(532, 454, 40, 25, 0, 0, 0);
  rectangle(575, 432, 40, 30, 0, 0, 0);

  // Write the new labels
  TextMid(366, 432, labeltext1, font_ptr, 0, 0, 0, 255, 255, 255);
  TextMid(398, 454, labeltext2, font_ptr, 0, 0, 0, 255, 255, 255);
  TextMid(448, 462, labeltext3, font_ptr, 0, 0, 0, 255, 255, 255);  // y was 465
  TextMid(500, 462, labeltext4, font_ptr, 0, 0, 0, 255, 255, 255);  // y was 465
  TextMid(550, 454, labeltext5, font_ptr, 0, 0, 0, 255, 255, 255);
  TextMid(593, 432, labeltext6, font_ptr, 0, 0, 0, 255, 255, 255);
}


void *MeterMovement(void * arg)
{
  bool *exit_requested = (bool *)arg;
  int current_meter_deflection = 0;
  int meter_move;
  float meter_angle;
  float current_sin;
  float current_cos;
  float previous_sin;
  float previous_cos;

  while((false == *exit_requested))
  {
    while (freeze)  // Do not run if display is frozen
    {
      usleep(20000);
    }

    // Check Meter deflection is within bounds
    if (meter_deflection > 1050)
    {
      meter_deflection = 1050;
    }
    if (meter_deflection < -50)
    {
      meter_deflection = -50;
    }


    if (meter_deflection != current_meter_deflection )        
    {
      // Physics
      meter_move = (meter_deflection - current_meter_deflection) / 5;  // Reduce movement speed
      if ((abs(meter_move) < 200) && (abs(meter_move) > 50))           // Slow as nearing position
      {
         meter_move = meter_move / 2;
      }

      if (meter_move > 200)                                            // Limit max speed
      {
        meter_move = 200;
      }

      if ((meter_deflection - current_meter_deflection) != 0)                                     // Only draw if needed
      {
        if (abs(meter_deflection - current_meter_deflection) > 5)                                 // Large move
        {
          meter_angle = (float)(current_meter_deflection + meter_move) * 2.0 * PI / 4000.0 - PI / 4.0;
          current_meter_deflection = current_meter_deflection + meter_move;
        }
        else                                                                                      // last few degrees
        {
          meter_angle = (float)(meter_deflection) * 2.0 * PI / 4000.0 - PI / 4.0;
          current_meter_deflection = meter_deflection;
        }
        current_sin = sin(meter_angle);
        current_cos = cos(meter_angle);

        // Overwrite previous position
        DrawAALine(475, 290, 475 + (int)(153.0 * previous_sin), 290 + (int)(153.0 * previous_cos), 0, 0, 0, 0, 0, 0);

        // Write new position
        DrawAALine(475, 290, 475 + (int)(153.0 * current_sin), 290 + (int)(153.0 * current_cos), 0, 0, 0, 255, 255, 255);

        // Set up for overwrite next time
        previous_sin = current_sin;
        previous_cos = current_cos;
      }
    }
    usleep(20000);  // 50 Hz refresh rate
  }
  return NULL;
}


void CalibrateSystem()
{
  CalibrateRequested = true;
}


static void cleanexit(int calling_exit_code)
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
  printf("Terminating\n");
  usleep(1000000);
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);
  printf("scans = %d\n", tracecount);
  exit(0);
}


int main(void)
{
  //int NoDeviceEvent=0;
  wscreen = 800;
  hscreen = 480;
  int screenXmax, screenXmin;
  int screenYmax, screenYmin;
  int i;
  int pixel;
  int nextwebupdate = 10;
  char response[63];

  int ScanCountforRefresh = 0;

  char dBText[31];
  float meterdB = 30.0;
 
  float baselineSum = 0;
  float passbandNoiseSum = 0;
  int baselineCount = 0;
  bool BLCalibrating = false;
  char ValueToSave[63];
  float passbandNoise;
  float smoothedPassbandNoise;
  float smoothedSignalNoise;
  float smoothedBaseNoise;
  float relativeNoise;
  float signalNoise;
  float baseNoise;
  float signalNoiseSum;
  float baseNoiseSum;

  uint64_t next_history = monotonic_ms();
  int z[255];
  float HistorySum;
  int HistoryCount = 0;
  float meterdBPlot;
  int newHistory;

  int pix2;
  int pix1;
  int pix0;
  int thisPix = 249;

  int peakY;


  // Catch sigaction and call terminate
  for (i = 0; i < 16; i++)
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = terminate;
    sigaction(i, &sa, NULL);
  }

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
  printf("Completed Screen and Mouse checks\n");

  // Set Screen parameters
  screenXmax = 799;
  screenXmin = 0;
  wscreen = 800;
  screenYmax = 479;
  screenYmin = 0;
  hscreen = 480;


  // Calculate screen parameters
  scaleXvalue = ((float)(screenXmax-screenXmin)) / wscreen;
  printf ("X Scale Factor = %f\n", scaleXvalue);
  printf ("wscreen = %d\n", wscreen);
  printf ("screenXmax = %d\n", screenXmax);
  printf ("screenXmim = %d\n", screenXmin);
  scaleYvalue = ((float)(screenYmax-screenYmin)) / hscreen;
  printf ("Y Scale Factor = %f\n", scaleYvalue);
  printf ("hscreen = %d\n", hscreen);
  printf ("screenYmax = %d\n", screenYmax);
  printf ("screenYmin = %d\n", screenYmin);

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
  Define_Menu41();

  ReadSavedParams();

  // Create Wait Button thread
  pthread_create (&thbutton, NULL, &WaitButtonEvent, NULL);

  // Initialise screen and splash

  CurrentMenu = 1;
  Start_Highlights_Menu1();

  initScreen();

  MsgBox4("Starting the Noise Meter", "Profiling FFTs on first use", "Please wait 80 seconds", "No delay next time");

  printf("Profiling FFTs..\n");
  fftwf_import_wisdom_from_filename("/home/pi/.fftwf_wisdom");
  printf(" - Main Band FFT\n");
  main_fft_init();
  fftwf_export_wisdom_to_filename("/home/pi/.fftwf_wisdom");
  printf("FFTs Done.\n");

  /* Setting up buffers */
  buffer_circular_init(&buffer_circular_iq_main, sizeof(buffer_iqsample_t), 4096*1024);

  /* LimeSDR Thread */
  if(pthread_create(&lime_thread_obj, NULL, lime_thread, &app_exit))
  {
    fprintf(stderr, "Error creating %s pthread\n", "Lime");
    return 1;
  }
  pthread_setname_np(lime_thread_obj, "Lime");

  printf("Starting FFT Thread\n");

  /* Band FFT Thread */
  if(pthread_create(&fft_thread_obj, NULL, fft_thread, &app_exit))
  {
    fprintf(stderr, "Error creating %s pthread\n", "FFT");
    return 1;
  }
  pthread_setname_np(fft_thread_obj, "FFT");

  // Zero the y and z value buffers
  for(i = 1; i < 255; i++)
  {
    y[i] = 201;
    z[i] = 201;
  }

  DrawEmptyScreen();  // Required to set A value, which is not set in DrawTrace
  DrawYaxisLabels();  // dB calibration on LHS
  DrawSettings();     // Start, Stop RBW, Ref level and Title
  UpdateWindow();     // Draw the buttons
  publish();

  // Start the meter movement
  if(pthread_create(&thMeter_Movement, NULL, &MeterMovement, &app_exit))
  {
    fprintf(stderr, "Error creating %s pthread\n", "MeterMovement");
    return 1;
  }

  // Force drawing of the meter surround on the first pass
  ModeChanged = true;

  while(true)                                       // Continuous loop for running
  {
    if (CalibrateBLRequested)         // Set baseline for absolute noise measurement when requested
    {
      if (BLCalibrating == false)
      {
        BLCalibrating = true;
        baselineSum = 0;
        baselineCount = 0;
        Start_Highlights_Menu2();
        UpdateWindow();
      }

      if (BLCalibrating)
      {
        baselineCount++;
      }

      if (baselineCount > 20)   // After 20 scans calculate baseline
      {
        baseline = (baselineSum / 237) / 100 - 80.0;          // 237 readings in each scan
        // printf ("Baseline =  %.1f dB\n", baseline);
        snprintf(ValueToSave, 61, "%.1f", baseline);  
        SetConfigParam(PATH_CONFIG, "baseline", ValueToSave);  // Store the new Baseline

        BLCalibrated = true;
        CalibrateBLRequested = false;
        BLCalibrating = false;
        baselineCount = 0;
        Start_Highlights_Menu2();
        UpdateWindow();
      }
    }

    if (strcmp(mode, "absolute") == 0)     // calculate and smooth the measured noise
    {
      passbandNoise = (passbandNoiseSum / 237) / 5 - 80.0;          // 237 readings in each scan
      smoothedPassbandNoise = (passbandNoise * (1.f - noiseSmoothingFactor)) + (smoothedPassbandNoise * noiseSmoothingFactor);
      relativeNoise = smoothedPassbandNoise - baseline;
      passbandNoiseSum = 0;
      meterdB = relativeNoise;
    }
    else if (strcmp(mode, "differential") == 0)
    {
      signalNoise = (signalNoiseSum / (sigendpixel - sigstartpixel - 1))/ 5 - 80.0;          // 
      baseNoise = (baseNoiseSum / (refendpixel - refstartpixel - 1))/ 5 - 80.0;          // 
      smoothedSignalNoise = (signalNoise * (1.f - noiseSmoothingFactor)) + (smoothedSignalNoise * noiseSmoothingFactor);
      smoothedBaseNoise = (baseNoise * (1.f - noiseSmoothingFactor)) + (smoothedBaseNoise * noiseSmoothingFactor);
      relativeNoise = smoothedSignalNoise - smoothedBaseNoise;
      signalNoiseSum = 0;
      baseNoiseSum = 0;
      meterdB = relativeNoise;
    }
    else if (strcmp(mode, "carrier") == 0)
    {
      signalNoise = (signalNoiseSum)/ 5 - 80.0;
      baseNoise = (baseNoiseSum / (refendpixel - refstartpixel - 1))/ 5 - 80.0;          // 
      smoothedSignalNoise = (signalNoise * (1.f - carrierSmoothingFactor)) + (smoothedSignalNoise * carrierSmoothingFactor);
      smoothedBaseNoise = (baseNoise * (1.f - carrierSmoothingFactor)) + (smoothedBaseNoise * carrierSmoothingFactor);
      relativeNoise = smoothedSignalNoise - smoothedBaseNoise;
      signalNoiseSum = 0;
      baseNoiseSum = 0;
      meterdB = relativeNoise;
    }
    else if (strcmp(mode, "abscarrier") == 0)
    {
      signalNoise = (signalNoiseSum)/ 5 - 80.0;
      smoothedSignalNoise = (signalNoise * (1.f - carrierSmoothingFactor)) + (smoothedSignalNoise * carrierSmoothingFactor);
      relativeNoise = smoothedSignalNoise - baseline;
      signalNoiseSum = 0;
      meterdB = relativeNoise;
    }

    ScanCountforRefresh++;

    if (ScanCountforRefresh >= (4 * ScansforLevel))  // Refresh display every 40 scans or so
    {
      ScanCountforRefresh = 0;

      // Draw Meter Surround if required
      if (ModeChanged == true)
      {
        DrawMeterArc();
        DrawMeterTicks(5, 1);
        Draw5MeterLabels();
        
        ModeChanged = false;
      }

      if (meterdB != meterdB)                    // Make sure that meterdB is not NaN
      {
        meterdB = 30.0;
      }

      // Display Lower left quadrant (detailed parameters or large numbers)
      if (LargeNumbers == false)   // display detailed parameters
      {
        // Clear the screen area if required
        if (LargeNumbersDisplayed == true)
        {
          rectangle(100, 55, 200, 185, 0, 0, 0);  // Blank area
          DrawSettings();                         // Redraw Bandwidth
          LargeNumbersDisplayed = false;
        }

        // Display Higher Power Level
        if (strcmp(mode, "absolute") == 0)
        {
          snprintf(dBText, 20, "Noise now %0.1f dB", smoothedPassbandNoise);
        }
        else if (strcmp(mode, "differential") == 0)
        {
          snprintf(dBText, 20, "Signal %0.1f dB", smoothedSignalNoise);
        }
        else if ((strcmp(mode, "carrier") == 0) || (strcmp(mode, "abscarrier") == 0))
        {
          snprintf(dBText, 20, "Carrier %0.1f dB", smoothedSignalNoise);
        }
        rectangle(100, 160, 200, 22, 0, 0, 0);  // Blank area
        Text(100, 165, dBText, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);

        // Display Lower Power Level
        if ((strcmp(mode, "absolute") == 0) || (strcmp(mode, "abscarrier") == 0))
        {
          snprintf(dBText, 20, "Baseline %0.1f dB", baseline);
        }
        else if ((strcmp(mode, "differential") == 0) || (strcmp(mode, "carrier") == 0))
        {
          snprintf(dBText, 20, "Background %0.1f dB", smoothedBaseNoise);
        }
        rectangle(100, 130, 200, 22, 0, 0, 0);  // Blank area
        if ((smoothedBaseNoise < -65.0) && (strcmp(mode, "abscarrier") != 0))           // Warn if outside linear region
        {
          Text(100, 135, dBText, &font_dejavu_sans_18, 0, 0, 0, 255, 127, 127);
        }
        else
        {
          Text(100, 135, dBText, &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);
        }

        // Warn if Lime Gain too low
        rectangle(100, 80, 200, 47, 0, 0, 0);  // Blank area
        if (strcmp(mode, "absolute") == 0)
        {
          if (baseline < -65.0)
          {
            Text(100, 110, "Increase Lime Gain", &font_dejavu_sans_18, 0, 0, 0, 255, 127, 127);
            Text(100,  85, "and reset Baseline", &font_dejavu_sans_18, 0, 0, 0, 255, 127, 127);
          }
        }
        else if ((strcmp(mode, "differential") == 0) || (strcmp(mode, "carrier") == 0))
        {
          if (smoothedBaseNoise < -65.0)
          {
            Text(100, 110, "Increase Lime Gain", &font_dejavu_sans_18, 0, 0, 0, 255, 255, 255);
          }
        }
      }
      else                                       // Clear quadrant and Display large numbers
      {
        snprintf(dBText, 20, "%0.1f", meterdB);
        rectangle(100, 55, 200, 185, 0, 0, 0);  // Blank area
        Text(100, 110, dBText, &font_dejavu_sans_72, 0, 0, 0, 255, 255, 255);
        LargeNumbersDisplayed = true;
      }

      switch (MeterScale)
      {
        case 1:                   // 0 - 30
          meter_deflection = (int)(meterdB * 1000 / 30);
        break;
        case 2:                   // 20 - 30
          meter_deflection = (int)(meterdB * 1000 / 10) - 2000;
        break;
        case 3:                   // 10 - 20
          meter_deflection = (int)(meterdB * 1000 / 10) - 1000;
        break;
        case 4:                   // 0 - 10
          meter_deflection = (int)(meterdB * 1000 / 10);
        break;
        case 5:                   // 0 - 5
          meter_deflection = (int)(meterdB * 1000 / 5);
        break;
        case 6:                   // -5 - +5
          meter_deflection = (int)(meterdB * 1000 / 10) + 500;
        break;
      }

      // Constrain value to < 100 dB
      if ((meterdB > 99.9) || (meterdB < -99.9))
      {
        meterdB = 0;
      }

      if (strcmp(mode, "differential") == 0)
      {
        snprintf(dBText, 20, "Noise Diff %0.1f dB", meterdB);
      }
      else if (strcmp(mode, "absolute") == 0)
      {
        snprintf(dBText, 20, "Noise %0.1f dB", meterdB);
      }
      else
      {
        snprintf(dBText, 20, "Carrier %0.1f dB", meterdB);
      }

      rectangle(360, 250, 280, 40, 0, 0, 0);  // Blank area
      Text(360, 260, dBText, &font_dejavu_sans_28, 0, 0, 0, 255, 255, 255);

      // Now calculate and draw the history timeline

      HistorySum = HistorySum + meterdB;
      HistoryCount = HistoryCount + 1;

      if (monotonic_ms() > next_history)
      {
        // Calculate average noise power since last plot point
        meterdBPlot = HistorySum / HistoryCount;

        // Reset average
        HistorySum = 0;
        HistoryCount = 0;

        // Calculate and constrain y value
        newHistory = (int)(((meterdBPlot - ActiveZero) * 200.0) / (ActiveFSD - ActiveZero));
        if (newHistory < 1)
        {
          newHistory = 1;
        }
        if (newHistory > 199)
        {
          newHistory = 199;
        }

        z[thisPix] = 200 + newHistory;

        // Draw the plot
        for (pixel = 3; pixel < 251; pixel++)  // go from 3 to 251, so that subtract 2 is still valid
        {
          // Make the display move along
          pix2 = pixel - 2 + (thisPix);
          if ((pix2) > 249)
          {
            pix2 = pix2 - 249;
          }
          pix1 = pixel - 1 + (thisPix);
          if ((pix1) > 249)
          {
            pix1 = pix1 - 249;
          }
          pix0 = pixel + (thisPix);
          if (pix0 > 249)
          {
            pix0 = pix0 - 249;
          }

          // Draw the trace, but do not interpolate if it is the right hand edge of the screen
          if (pixel != 250)
          {
            DrawHistTrace((pixel - 1), z[pix2], z[pix1], z[pix0]);
          }
          else
          {
            DrawHistTrace((pixel - 1), z[pix2], z[pix1], z[pix1]);
          }
        }

        // increment the move-along counter
        thisPix++;
        if (thisPix > 249)
        {
          thisPix = 1;
        }

        // Calculate time for next plot
        next_history = next_history + 4 * historyspan;  // interval = historyspan * 1000 ms / 250 pixels
      }
    }

    // Draw the main spectrum
    for (pixel = 4; pixel < 253; pixel++)
    {
      // Draw the spectrum
      DrawTrace((pixel - 2), y[pixel - 2], y[pixel - 1], y[pixel]);

      // Measure passband noise
      if (strcmp(mode, "absolute") == 0)  // Measure noise in total passband
      {
        if (CalibrateBLRequested)              // measure reference passband noise here
        {
          if (((pixel > 3) && (pixel < 122)) || ((pixel > 133) && (pixel < 253)))
          {
             baselineSum = baselineSum + 2 * (y[pixel - 1] - 200);
          }
        }
        else                                     // measure passband noise here
        {
          if (((pixel > 3) && (pixel < 122)) || ((pixel > 133) && (pixel < 253)))
          {
            passbandNoiseSum = passbandNoiseSum + 2 * (y[pixel - 1] - 200);
          }
        }
      }
      else if (strcmp(mode, "differential") == 0)
      {
        // pixels go from 4 to 252

        if ((pixel > refstartpixel) && (pixel < refendpixel))
        {
          baseNoiseSum = baseNoiseSum + 2 * (y[pixel - 1] - 200);
        }
        if ((pixel > sigstartpixel) && (pixel < sigendpixel))
        {
          signalNoiseSum = signalNoiseSum + 2 * (y[pixel - 1] - 200);
        }
      }
      else if ((strcmp(mode, "carrier") == 0) || (strcmp(mode, "abscarrier") == 0))
      {
        // pixels go from 4 to 252

        if ((pixel > refstartpixel) && (pixel < refendpixel))
        {
          baseNoiseSum = baseNoiseSum + 2 * (y[pixel - 1] - 200);
        }
        if ((pixel > sigstartpixel) && (pixel < sigendpixel))
        {
          if (y[pixel - 1] > peakY)
          {
            peakY = y[pixel - 1];
            peakPixel = pixel - 1;
          }
        }
      }

      while (freeze)
      {
        frozen = true;
      }
      frozen = false;
    }

    if ((strcmp(mode, "carrier") == 0) || (strcmp(mode, "abscarrier") == 0))
    {
      signalNoiseSum = signalNoiseSum + 2 * (y[peakPixel - 1] - 200 + 3);  // 0 dB is at pixel 3
      peakLine = peakPixel;
      peakY = 0;
    }
    publish();
    tracecount++;

    if (tracecount >= nextwebupdate)
    {
      // printf("tracecount = %d, Time ms = %llu \n", tracecount, monotonic_ms());
      UpdateWeb();
      usleep(10000);
      nextwebupdate = tracecount + 90;  // About 820 ms between updates
    }
  }

  printf("Waiting for Lime Thread to exit..\n");
  pthread_join(lime_thread_obj, NULL);
  printf("Waiting for FFT Thread to exit..\n");
  pthread_join(fft_thread_obj, NULL);
  printf("Waiting for Screen Thread to exit..\n");
  pthread_join(screen_thread_obj, NULL);
  printf("Waiting for Meter Thread to exit..\n");
  pthread_join(thMeter_Movement, NULL);
  printf("All threads caught, exiting..\n");
  pthread_join(thbutton, NULL);
}

