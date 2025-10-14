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
#include "../common/ffunc.h"
#include "../common/utils.h"
#include "lmrx_utils.h"

#define PI 3.14159265358979323846

#define PATH_PCONFIG "/home/pi/portsdown/configs/portsdown_config.txt"
#define PATH_SCONFIG "/home/pi/portsdown/configs/system_config.txt"
#define PATH_LMCONFIG "/home/pi/portsdown/configs/longmynd_config.txt"
#define PATH_STREAM_PRESETS "/home/pi/portsdown/configs/stream_presets.txt"

typedef struct {int r, g, b;} color_t;    // Colour (used for buttons)

typedef struct {
	char Text[255];                       // Button text
	color_t  Color;                       // Button colour
} status_t;

#define MAX_STATUS 5                      // 5 options for each button

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
color_t Red   = {.r = 255, .g = 0  , .b = 0  };
color_t Black = {.r = 0  , .g = 0  , .b = 0  };

bool invert_display = true;

#define MAX_BUTTON_ON_MENU 50
#define MAX_MENUS 50

int debug_level = 0;                   // 0 minimum, 1 medium, 2 max
int fd = 0;                            // File Descriptor for touchscreen touch messages

char StartApp[63];                     // Startup app on boot
char DisplayType[31];                  // Element14_7, touch2, hdmi, web
int VLCTransform = 0;                  // 0, 90, 180 or 270
int FBOrientation = 0;                 // 0, 90, 180 or 270
int HDMIResolution = 720;              // HDMI Resolution 720/1080
bool TouchInverted = false;            // true if screen mounted upside down
bool KeyLimePiEnabled = true;          // false for release

int hscreen = 480;
int wscreen = 800;
  // -25 keeps right hand side symmetrical with left hand side
int wbuttonsize = 155; //(wscreen-25)/5;
int hbuttonsize = 67; // hscreen/6 * 5 / 6;

typedef struct
{
  char modulation[63];
  char encoding[63];
  char modeoutput[63];
  char format[63];
  char videosource[63];
  float freqoutput;
  uint16_t symbolrate;
  uint16_t fec;
  uint16_t limegain;
} config_t;

static config_t config = 
{
  .modulation = "S2QPSK",
  .encoding = "H264",
  .modeoutput = "LIMEMINI",
  .format = "4:3",
  .videosource = "TestCard",
  .freqoutput = 437.00,
  .symbolrate = 333,
  .fec = 23,
  .limegain = 88
};

int IndexButtonInArray=0;
button_t ButtonArray[MAX_MENUS][MAX_BUTTON_ON_MENU];
char MenuTitle[MAX_MENUS][63];
int CurrentMenu = 1;
int CallingMenu = 1;

// User interaction variables
int scaledX = 0;                       // x and y valid after touch, mouse click, web click or Virtual touch
int scaledY = 0;                       // bottom left is always 0, 0.  Top right 799, 479
bool app_exit = false;                 // To ensure clean shutdown

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
bool webcontrol = false;               // Enables remote control of touchscreen functions
char ProgramName[255];                 // used to pass application name char string to listener
int *web_x_ptr;                        // pointer
int *web_y_ptr;                        // pointer
int web_x;                             // click x 0 - 799 from left
int web_y;                             // click y 0 - 480 from top
bool webclicklistenerrunning = false;  // Used to only start thread if required
char WebClickForAction[7] = "no";      // no/yes

// Portsdown state variables
char ScreenState[63] = "NormalMenu";
bool boot_to_tx = false;
bool boot_to_rx = false;

bool test20 = false;
bool test21 = false;

// Lime gain calibration for 0 to -21 dB
int limeGainSet[25] = {100, 99, 97, 96, 95,
                        93, 92, 90, 89, 88,
                        0,  85, 83, 81, 80,
                        79, 78, 76, 75, 74,
                        73, 71};

// LongMynd RX Parameters. [0] is current.
int LMRXfreq[22];           // Integer frequency in kHz 0 current, 1-10 q, 11-20 t, 21 second tuner current
int LMRXsr[22];             // Symbol rate in K. 0 current, 1-6 q, 6-12 t, 13 second tuner current
int LMRXqoffset;            // Offset in kHz
int LMRXqtimeout;           // Tuner timeout for QO-100 in milliseconds
int LMRXttimeout;            // Tuner timout for terrestrial in milliseconds
int LMRXqscanwidth;         // Tuner scan width for QO-100 as % of SR
int LMRXtscanwidth;         // Tuner scan width for terrestrial as % of SR
int LMRXqvidchan;           // Video Channel for QO-100
int LMRXtvidchan;           // Video Channel for terrestrial
char LMRXinput[2];          // Input a or b
char LMRXudpip[20];         // UDP IP address
char LMRXudpport[10];       // UDP IP port
char LMRXmode[10];          // sat or terr
char LMRXaudio[15];         // rpi or usb or hdmi
char LMRXvolts[7];          // off, v or h
char RXmod[7];              // DVB-S or DVB-T
bool VLCResetRequest = false; // Set on touchsscreen request for VLC to be reset  
int  CurrentVLCVolume = 256;  // Read from config file
char player[63];            // vlc or ffplay                   

// LongMynd RX Received Parameters for display
bool timeOverlay = false;    // Display time overlay on received metadata and snaps
time_t t;                    // current time

// Stream Player
bool amendStreamPreset = false;        // Set to amend a stream preset
int activeStream;                      // Stream currently ready to be played
char streamURL[21][63];                // stream URL read from config file
char streamLabel[21][63];              // stream Button Label read from config file
char StreamKey[9][63];                 // transmit stream name-key
bool StreamerStoreTrigger = false;     // Set true to enable transmit stream to be changed

// Threads
pthread_t thtouchscreen;               //  listens to the touchscreen
pthread_t thwebclick;                  //  Listens for clicks from web interface
pthread_t thmouse;                     //  Listens to the mouse
pthread_t thbutton;                    //  Listens during receive

// ******************** Menus for Reference *********************************

// 1  Main Menu
// 2  (Apps Menu)
// 3  Config Menu
// 4  File MenuDATV Tools
// 5  DATV Tools
// 6  System Config
// 7  Test Equipment
// 8  Receive
// 9
// 10
// 11 TX Modulation
// 12 TX Encoding
// 13 TX Output Device
// 14 TX Video Format
// 15 TX Video Source
// 16 TX Frequency
// 17 TX Symbol Rate
// 18 TX FEC
// 19 TX Band
// 20 SDR Gain
// 21 Start-up App
// 22 Display and Control
// 23 Receive Config
// 24 Receive Presets
// 25 Stream Player
// 26 Stream TX presets
// 32 Decision of 2
// 33 Decision of 3

// 41 Alpha Keyboard


// ******************* External functions for reference *********************

// *************** hardware.c: ***************

// int FindMouseEvent();
// void GetIPAddr(char IPAddress[18]);
// void GetIPAddr2(char IPAddress[18]);
// void Get_wlan0_IPAddr(char IPAddress[18]);

// *************** utils.c: ***************

// void strcpyn(char *outstring, char *instring, int n);
// bool is_valid_ip(char *ip_str);
// bool valid_digit(char *ip_str);

// ******************* Function Prototypes **********************************

// Read and write the configuration
void GetConfigParam(char *PathConfigFile, char *Param, char *Value);
void SetConfigParam(char *PathConfigFile, char *Param, char *Value);
void CheckConfigFile();
void ReadSavedParams();
void ReadLMRXPresets();
void ReadStreamPresets();
int file_exist (char *filename);

// Handle touch and mouse
int openTouchScreen(int NoDevice);
int getTouchScreenDetails(int *screenXmin,int *screenXmax,int *screenYmin,int *screenYmax);
int getTouchSampleThread(int *rawX, int *rawY);
int getTouchSample(int *rawX, int *rawY);
void *WaitTouchscreenEvent(void * arg);
void *WaitMouseEvent(void * arg);
void handle_mouse();
void *WebClickListener(void * arg);
void parseClickQuerystring(char *query_string, int *x_ptr, int *y_ptr);
FFUNC touchscreenClick(ffunc_session_t * session);
void *WaitButtonLMRX(void * arg);
void wait_touch();

// Actually do useful things
void TransmitStop();
void TransmitStart();
void playStreamFFPlay();
void playStreamVLC();
void ShowTXVideo(bool start);

// Create the Menus
int IsMenuButtonPushed();
void CreateButtons();
int AddButtonStatus(int menu, int button_number, char *Text, color_t *Color);
void SetButtonStatus(int menu, int button_number, int Status);
void SelectInGroupOnMenu(int menu, int firstButton, int lastButton, int NumberButton, int Status);
void SelectFromGroupOnMenu5(int menu, int firstButton, int Status, char *selection,
  char *value1, char *value2, char *value3, char *value4, char *value5);
void SelectFromGroupOnMenu4(int menu, int firstButton, int Status, char *selection,
  char *value1, char *value2, char *value3, char *value4);
void SelectFromGroupOnMenu3(int menu, int firstButton, int Status, char *selection,
  char *value1, char *value2, char *value3);
void SelectFromGroupOnMenu2(int menu, int firstButton, int Status, char *selection,
  char *value1, char *value2);
void SelectFromGroupOnMenu1(int menu, int firstButton, int Status, char *selection,
  char *value1);
int GetButtonStatus(int menu, int button_number);
void AmendButtonStatus(int menu, int button_number, int ButtonStatusIndex, char *Text, color_t *Color);
void DrawButton(int menu, int button_number);
void redrawButton(int menu, int button_number);
void ShowTitle();
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
void Define_Menu11();
void Highlight_Menu11();
void Define_Menu12();
void Highlight_Menu12();
void Define_Menu13();
void Highlight_Menu13();
void Define_Menu14();
void Highlight_Menu14();
void Define_Menu15();
void Highlight_Menu15();
void Define_Menu16();
void Highlight_Menu16();
void Define_Menu17();
void Highlight_Menu17();
void Define_Menu18();
void Highlight_Menu18();
void Define_Menu19();
void Highlight_Menu19();
void Define_Menu20();
void Highlight_Menu20();
void Define_Menu21();
void Highlight_Menu21();
void Define_Menu22();
void Highlight_Menu22();
void Define_Menu23();
void Highlight_Menu23();
void Define_Menu24();
void Highlight_Menu24();
void Define_Menu25();
void Highlight_Menu25();
void Define_Menu26();
void Highlight_Menu26();
void Define_Menu32();
void Highlight_Menu32();
void Define_Menu33();
void Highlight_Menu33();
void Define_Menu41();
void Define_Menus();
void Keyboard(char *RequestText, char *InitText, int MaxLength, char *KeyboardReturn, bool UpperCase);
int Decision2(char *TitleText, char *QText, char *Button1Text, char *Button2Text, bool CancelButton);
int Decision3(char *TitleText, char *QText, char *Button1Text, char *Button2Text, char *Button3Text, bool CancelButton);

// Action Menu Buttons
void selectTestEquip(int Button);
void selectModulation(int NoButton);
void selectEncoding(int Button);
void selectOutput(int Button);
void selectFormat(int Button);
void selectVideosource(int Button);
void selectFreqoutput(int Button);
void selectFEC(int Button);
void selectBand(int Button);
void selectGain(int Button);
void SelectStartApp(int Button);
void togglewebcontrol();
void togglehdmiresolution();
void togglescreenorientation();
void SelectLMFREQ(int NoButton);
void SelectLMSR(int NoButton);
void toggleLMRXmode();
void ChangeLMRXIP();
void ChangeLMRXPort();
void ChangeLMRXOffset();
void ChangeLMRXPlayer();
void ChangeLMTST();
void ChangeLMSW();
void ChangeLMChan();
void ChangeLMPresetFreq(int NoButton);
void ChangeLMPresetSR(int NoButton);
void toggleLMRXinput();
void CycleLNBVolts();
void CycleLMRXaudio();
void AdjustVLCVolume(int adjustment);
void ChangeStreamPreset(int NoButton);
void ToggleAmendStreamPreset();
void CheckforUpdate();
void SelectStreamerAction(int NoButton);
void AmendStreamerPreset(int NoButton);
void SelectStreamer(int NoButton);
void ToggleAmendStreamerPreset();

// Useful stuff

void InfoScreen();
void checkTunerSettings();
void SeparateStreamKey(char streamkey[127], char streamname[63], char key[63]);

// Make things happen
void waitForScreenAction();
static void cleanexit(int exit_code);
static void terminate(int dummy);

 
// ************** End of Function Prototypes ******************************* //

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
  char ErrorMessage1[255];
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");
  int file_test = 0;

  file_test = file_exist(PathConfigFile);  // Display error if file does not exist
  if (file_test == 1)
  {
    usleep(100000);           // Pause to let screen initialise
    //setBackColour(0, 0, 0);   // Overwrite Portsdown Logo
    //MsgBox4("Error: Config File not found:", PathConfigFile, "Restore manually or by factory reset", 
    //        "Touch Screen to Continue");
    //wait_touch();
    strcpy(Value, " ");
    return;
  }

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
    if (debug_level == 2)
    {
      printf("Get Config reads %s for %s and returns %s\n", PathConfigFile, Param, Value);
    }
  }
  else
  {
    printf("Config file not found \n");
  }
  fclose(fp);

  if (strlen(Value) == 0)  // Display error if parameter undefined
  {  
    usleep(100000);           // Pause to let screen initialise
    //setBackColour(0, 0, 0);   // Overwrite Portsdown Logo

    snprintf(ErrorMessage1, 63, "Error: Undefined parameter %s in file:", Param);

    //MsgBox4(ErrorMessage1, PathConfigFile, "Restore manually or by factory reset", 
//            "Touch Screen to Continue");
    //wait_touch();
  }
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
    //setBackColour(0, 0, 0);   // Overwrite Portsdown Logo
    snprintf(ErrorMessage1, 63, "Error: Parameter %s in file:", Param);
    //MsgBox4(ErrorMessage1, PathConfigFile, "Would have no value. Try again.", 
    //        "Touch Screen to Continue");
    //wait_touch();
    return;
  }

  strcpy(BackupConfigName, PathConfigFile);
  strcat(BackupConfigName, ".bak");
  FILE *fp=fopen(PathConfigFile, "r");
  FILE *fw=fopen(BackupConfigName, "w+");
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

  // Portsdown Config File

  strcpy(response, "-");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "modulation", response);
  strcpy(config.modulation, response);

  strcpy(response, "-");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "encoding", response);
  strcpy(config.encoding, response);

  strcpy(response, "-");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "modeoutput", response);
  strcpy(config.modeoutput, response);

  strcpy(response, "-");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "format", response);
  strcpy(config.format, response);

  strcpy(response, "-");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "videosource", response);
  strcpy(config.videosource, response);

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "freqoutput", response);
  config.freqoutput = atof(response);

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "symbolrate", response);
  config.symbolrate = atoi(response);

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "fec", response);
  config.fec = atoi(response);

  // Band?

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "limegain", response);
  config.limegain = atoi(response);

  // System Config File

  strcpy(response, "0");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "startup", response);
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
  GetConfigParam(PATH_SCONFIG, "player", response);
  strcpy(player, response);

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

  strcpy(response, "disabled");   
  GetConfigParam(PATH_SCONFIG, "keylimepi", response);
  if (strcmp(response, "enabled") == 0)
  {
    KeyLimePiEnabled = true;
  }
  else
  {
    KeyLimePiEnabled = false;
  }

  // Read the LongMynd RX presets
  ReadLMRXPresets();

  // Read the Stream presets
  ReadStreamPresets();
}


/***************************************************************************//**
 * @brief Reads the Presets from longmynd_config.txt and formats them for
 *        Display and switching
 *
 * @param None.  Works on global variables
 *
 * @return void
*******************************************************************************/

void ReadLMRXPresets()
{
  int n;
  char Value[15] = "";
  char Param[20];

  // Mode: sat or terr
  GetConfigParam(PATH_LMCONFIG, "mode", LMRXmode);

  // UDP output IP address:
  GetConfigParam(PATH_LMCONFIG, "udpip", LMRXudpip);
  
  // UDP output port:
  GetConfigParam(PATH_LMCONFIG, "udpport", LMRXudpport);

  // Audio output port: (rpi or usb or hdmi)
  GetConfigParam(PATH_LMCONFIG, "audio", LMRXaudio);

  // QO-100 LNB Offset:
  GetConfigParam(PATH_LMCONFIG, "qoffset", Value);
  LMRXqoffset = atoi(Value);

  // QO-100 Tuner Timeout time
  GetConfigParam(PATH_LMCONFIG, "tstimeout", Value);
  LMRXqtimeout = atoi(Value);

  // Terrestrial Tuner Timeout time
  GetConfigParam(PATH_LMCONFIG, "tstimeout1", Value);
  LMRXttimeout = atoi(Value);

  // QO-100 Tuner scan width
  GetConfigParam(PATH_LMCONFIG, "scanwidth", Value);
  LMRXqscanwidth = atoi(Value);

  // Terrestrial Tuner scan width
  GetConfigParam(PATH_LMCONFIG, "scanwidth1", Value);
  LMRXtscanwidth = atoi(Value);

  // Video Channel for QO-100
  GetConfigParam(PATH_LMCONFIG, "chan", Value);
  LMRXqvidchan = atoi(Value);

  // Video Channel for terrestrial
  GetConfigParam(PATH_LMCONFIG, "chan1", Value);
  LMRXtvidchan = atoi(Value);

  if (strcmp(LMRXmode, "sat") == 0)
  {
    // Input: a or b
    GetConfigParam(PATH_LMCONFIG, "input", LMRXinput);

    // Start up frequency
    GetConfigParam(PATH_LMCONFIG, "freq0", Value);
    LMRXfreq[0] = atoi(Value);

    // Start up SR
    GetConfigParam(PATH_LMCONFIG, "sr0", Value);
    LMRXsr[0] = atoi(Value);
  }
  else    // Terrestrial
  {
    // Input: a or b
    GetConfigParam(PATH_LMCONFIG, "input1", LMRXinput);

    // Start up frequency
    GetConfigParam(PATH_LMCONFIG, "freq1", Value);
    LMRXfreq[0] = atoi(Value);

    // Start up SR
    GetConfigParam(PATH_LMCONFIG, "sr1", Value);
    LMRXsr[0] = atoi(Value);
  }

  // Frequencies
  for(n = 1; n < 11; n = n + 1)
  {
    // QO-100
    snprintf(Param, 15, "qfreq%d", n);
    GetConfigParam(PATH_LMCONFIG, Param, Value);
    LMRXfreq[n] = atoi(Value);

    // Terrestrial
    snprintf(Param, 15, "tfreq%d", n);
    GetConfigParam(PATH_LMCONFIG, Param, Value);
    LMRXfreq[n + 10] = atoi(Value);
  }

  // Symbol Rates
  for(n = 1; n <= 10; n = n + 1)
  {
    // QO-100
    snprintf(Param, 15, "qsr%d", n);
    GetConfigParam(PATH_LMCONFIG, Param, Value);
    LMRXsr[n] = atoi(Value);

    // Terrestrial
    snprintf(Param, 15, "tsr%d", n);
    GetConfigParam(PATH_LMCONFIG, Param, Value);
    LMRXsr[n + 10] = atoi(Value);
  }

  // Modulation
  GetConfigParam(PATH_LMCONFIG, "rxmod", Value);
  if (strcmp(Value, "dvbt") == 0)
  {
    strcpy(RXmod, "DVB-T");
  }
  else
  {
    strcpy(RXmod, "DVB-S");
  }

  // Time Overlay
  strcpy(Value, "off");
  GetConfigParam(PATH_PCONFIG, "timeoverlay", Value);
  if (strcmp(Value, "off") == 0)
  {
    timeOverlay = false;
  }
  else
  {
    timeOverlay = true;
  }
}


/***************************************************************************//**
 * @brief Reads the Presets from stream presests from stream_presets.txt 
 *        and formats them for Display and switching
 *
 * @param None.  Works on global variables
 *
 * @return void
*******************************************************************************/

void ReadStreamPresets()
{
  int i;
  char response[63];
  char param[31];

  strcpy(response, "1");  // Default
  GetConfigParam(PATH_STREAM_PRESETS, "selectedstream", response);
  activeStream = atoi(response);

  for (i = 1; i <= 20; i++)
  {
    snprintf(param, 31, "stream%d", i);
    GetConfigParam(PATH_STREAM_PRESETS, param, streamURL[i]);
    snprintf(param, 31, "label%d", i);
    GetConfigParam(PATH_STREAM_PRESETS, param, streamLabel[i]);
  }

  // Read in Streamer Transmit presets
  
  strcpy(response, "1");  // Default
  GetConfigParam(PATH_PCONFIG, "streamkey", response);
  strcpy(StreamKey[0], response);

  for(i = 1; i < 9; i = i + 1)
  {
    // Streamname-key
    snprintf(param, 15, "streamkey%d", i);
    GetConfigParam(PATH_STREAM_PRESETS, param, response);
    strcpy(StreamKey[i], response);
  }
}


/***************************************************************************//**
 * @brief Checks if a file exists
 *
 * @param nil
 *
 * @return 0 if exists, 1 if not
*******************************************************************************/

int file_exist (char *filename)
{
  if (access(filename, R_OK) == 0) 
  {
    // file exists
    return 0;
  }
  else
  {
    // file doesn't exist
    return 1;
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

/*
Input device name: "ADS7846 Touchscreen"
Supported events:
  Event type 0 (Sync)
  Event type 1 (Key)
    Event code 330 (Touch)
  Event type 3 (Absolute)
    Event code 0 (X)
     Value      0
     Min        0
     Max     4095
    Event code 1 (Y)
     Value      0
     Min        0
     Max     4095
    Event code 24 (Pressure)
     Value      0
     Min        0
     Max      255
*/

int getTouchScreenDetails(int *screenXmin,int *screenXmax,int *screenYmin,int *screenYmax)
{
  unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
  char name[256] = "Unknown";
  int abs[6] = {0};

  ioctl(fd, EVIOCGNAME(sizeof(name)), name);
  if (debug_level >= 1)
  {
    printf("Input device name: \"%s\"\n", name);
  }

  memset(bit, 0, sizeof(bit));
  ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
  if (debug_level >= 1)
  {
    printf("Supported events:\n");
  }

  int i, j, k;
  int IsAtouchDevice = 0;
  for (i = 0; i < EV_MAX; i++)
  {
    if (test_bit(i, bit[0]))
    {
      if (debug_level >= 1)
      {
        printf("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
      }
      if (!i) continue;
      ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
      for (j = 0; j < KEY_MAX; j++)
      {
        if (test_bit(j, bit[i]))
        {
          if (debug_level >= 1)
          {
            printf("    Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
          }
          if (j == 330)
          {
            IsAtouchDevice = 1;
          }
          if (i == EV_ABS)
          {
            ioctl(fd, EVIOCGABS(j), abs);
            for (k = 0; k < 5; k++)
            {
              if ((k < 3) || abs[k])
              {
                if (debug_level >= 1)
                {
                  printf("     %s %6d\n", absval[k], abs[k]);
                }
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
        printf("7 inch Touchscreen Touch Event: rawX = %d, rawY = %d scaledX = %d, scaledY = %d\n", *rawX, *rawY, scaledX, scaledY);
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
      printf("Event (Touch) scaledX = %d, scaledY = %d\n", scaledX, scaledY);
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
  printf("Exiting WebClickListener\n");
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
    //printf("After Parse: x: %d, y: %d\n", x, y);

    if((x >= 0) && (y >= 0))
    {
      web_x = x;                 // web_x is a global int
      web_y = y;                 // web_y is a global int
      strcpy(WebClickForAction, "yes");
      //printf("Web Click Event x: %d, y: %d\n", web_x, web_y);
    }
  }
  else
  {
    ffunc_write_out(session, "Status: 400 Bad Request\r\n");
    ffunc_write_out(session, "Content-Type: text/plain\r\n\r\n");
    ffunc_write_out(session, "%s\n", "payload not found.");
  }
}


void *WaitButtonLMRX(void * arg)
{
  int rawX, rawY;
  int count_time_ms;
  FinishedButton = 1; // Start with Parameters on

  while((FinishedButton == 1) || (FinishedButton = 2))
  {
    while(getTouchSample(&rawX, &rawY) == 0);  // Wait here for touch

    if((scaledX <= 5 * wscreen / 40)  &&  (scaledY <= 2 * hscreen / 12) && (strcmp(DisplayType, "hdmi") != 0)) // Bottom left
    {
      printf("In snap zone, so take snap.\n");
      system("/home/pi/portsdown/scripts/snap2.sh");
    }
    else if((scaledX <= 5 * wscreen / 40)  && (scaledY >= 10 * hscreen / 12) && (strcmp(DisplayType, "hdmi") != 0))  // Top left
    {
      printf("In restart VLC zone, so set for reset.\n");
      VLCResetRequest = true;
    }
    else if ((scaledX <= 15 * wscreen / 40) && (scaledY <= 10 * hscreen / 12) && (scaledY >= 2 * hscreen / 12) && (strcmp(DisplayType, "hdmi") != 0))
    {
      printf("In parameter zone, so toggle parameter view.\n");
      if (FinishedButton == 2)  // Toggle parameters on/off 
      {
        FinishedButton = 1; // graphics on
      }
      else
      {
        FinishedButton = 2; // graphics off
      }
    }
    else if((scaledX >= 35 * wscreen / 40) && (scaledY >= 8 * hscreen / 12) && (strcmp(DisplayType, "hdmi") != 0))  // Top Right
    {
      printf("Volume Up.\n");
      AdjustVLCVolume(51);
    }
    else if((scaledX >= 35 * wscreen / 40) && (scaledY >= 4 * hscreen / 12) && (scaledY < 8 * hscreen / 12) && (strcmp(DisplayType, "hdmi") != 0))  // mid Right
    {
      printf("Volume Mute.\n");
      AdjustVLCVolume(-512);
    }
    else if((scaledX >= 35 * wscreen / 40) && (scaledY < 4 * hscreen / 12) && (strcmp(DisplayType, "hdmi") != 0))  // Bottom Right
    {
      printf("Volume Down.\n");
      AdjustVLCVolume(-51);
    }
    else
    {
      // Close VLC to reduce processor load first
      system("/home/pi/portsdown/scripts/receive/lmvlcclose.sh");

      // kill the receive process
      system("sudo killall longmynd > /dev/null 2>/dev/null");

      printf("Receive processes stopped.\n");
      FinishedButton = 0;  // Exit receive
      touch_response = 1;
      count_time_ms = 0;

      // wait here to make sure that touch_response is set back to 0
      // If not, restart GUI
      while ((touch_response == 1) && (count_time_ms < 500))
      {
        usleep(1000);
        count_time_ms = count_time_ms + 1;
      }
      count_time_ms = 0;
      while ((touch_response == 1) && (count_time_ms < 2500))
      {
        usleep(1000);
        count_time_ms = count_time_ms + 1;
      }

      if (touch_response == 1) // count_time has elapsed and still no reponse
      {
        clearScreen(0, 0, 0);
        exit(129);                 // Restart the GUI
      }
      return NULL;
    }
  }
  return NULL;
}


void wait_touch()
// Wait for Screen touch, ignore position, but then move on
// Used to let user acknowledge displayed text
{
  int rawX, rawY;
  printf("wait_touch called\n");

  // Check if screen touched, if not, wait 0.1s and check again

  while(getTouchSample(&rawX, &rawY) == 0)
  {
    usleep(100000);
  }
  // Screen has been touched
  printf("wait_touch exit\n");
}


void TransmitStop()
{
  system("/home/pi/portsdown/scripts/transmit/tx_stop.sh &");
}


void TransmitStart()
{   // Legacy code for testing LimeXTRX
  if ((strcmp(config.modeoutput, "LIMEXTRX") == 0)  || (strcmp(config.modeoutput, "LIMEMINING") == 0))
  {
    strcpy(ScreenState, "TXwithMenu");
    system("/home/pi/portsdown/scripts/transmit/tx.sh &");
  }
  else  // new code
  {
    strcpy(ScreenState, "TXwithMenu");
    system("/home/pi/portsdown/scripts/transmit/tx_rf.sh &");
    system("/home/pi/portsdown/scripts/transmit/tx_video.sh &");
  }
}


void LMRX(int NoButton)
{
  #define PATH_SCRIPT_LMRXMER     "/home/pi/portsdown/scripts/receive/lmmer.sh 2>&1"
  #define PATH_SCRIPT_LMRXUDP     "/home/pi/portsdown/scripts/receive/lmudp.sh 2>&1"
  #define PATH_SCRIPT_LMRXVLCUDP  "/home/pi/portsdown/scripts/receive/lmvlcudp.sh"
  #define PATH_SCRIPT_LMRXVLCUDP2 "/home/pi/portsdown/scripts/receive/lmvlcudp2.sh"

  //Local parameters:

  FILE *fp;
  int num;
  int ret;
  int fd_status_fifo;
  int TUNEFREQ;

  float MER;
  float refMER;
  int MERcount = 0;
  float FREQ;
  int STATE = 0;
  int SR;
  char Value[63];
  bool Overlay_displayed = false;
  char LinuxCommand[255];

  // To be Global Paramaters:

  char status_message_char[14];
  char stat_string[255];
  char udp_string[63];
  char MERtext[63];
  char MERNtext[63];
  char STATEtext[63];
  char FREQtext[63];
  char SRtext[63];
  char ServiceProvidertext[255] = " ";
  char Servicetext[255] = " ";

  char Itext[63] = " ";
  char Qtext[63] = " ";
  int  IQ[1024][2];
  int  IQindex = 0;
  int  nullsarray[100];
  int  nullsindex = 0;
  float avmag = 50.0;
  int  i;
  char NullRatiotext[63] = " ";
  char FECtext[63] = " ";
  char Modulationtext[63] = " ";
  char Encodingtext[63] = " ";
  char vlctext[255];
  char AGCtext[255];
  char AGC1text[255];
  char AGC2text[255];
  uint16_t AGC1 = 0;
  uint16_t AGC2 = 3200;
  float MERThreshold = 0;
  int Parameters_currently_displayed = 1;  // 1 for displayed, 0 for blank
  float previousMER = 0;
  int FirstLock = 0;  // set to 1 on first lock, and 2 after parameter fade
  clock_t LockTime;
  bool webupdate_this_time = true;   // Only update web on alternate MER changes
  char LastServiceProvidertext[255] = " ";
  bool FirstReceive = true;
  char TIMEtext[63];

  int Qvalue, Ivalue, Itextvalue, Qtextvalue;

  uint16_t TunerPollCount = 0;
  bool TunerFound = false;

  char ExtraText[63];
  char line5[255];

  // Set globals
  FinishedButton = 1;
  VLCResetRequest = false;

  memset(IQ, 0, sizeof IQ);  // zero IQ array

  // Display the correct background
  if (strcmp(DisplayType, "hdmi") == 0)
  {
    clearScreen(0, 0, 0);    // do nothing?
  }
  else if ((NoButton == 1) || (NoButton == 5))   // diagnostics, LNB autoset and hdmi modes
  {
    switch (FBOrientation)
    {
    case 0:
      strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black.png ");
      break;
    case 90:
      strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black90.png ");
      break;
    case 180:
      strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black180.png ");
      break;
    case 270:
      strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black270.png ");
      break;
    }
    //strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black.png ");
    strcat(LinuxCommand, ">/dev/null 2>/dev/null");
    system(LinuxCommand);
    strcpy(LinuxCommand, "(sleep 1; sudo killall -9 fbi >/dev/null 2>/dev/null) &");
    system(LinuxCommand);
  }
  else if ((NoButton == 0) || (NoButton == 2))   // VLC modes
  {
    switch (FBOrientation)
    {
    case 0:
      strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_overlay.png ");
      break;
    case 90:
      strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_overlay90.png ");
      break;
    case 180:
      strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_overlay180.png ");
      break;
    case 270:
      strcpy(LinuxCommand, "sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_overlay270.png ");
      break;
    }

    Overlay_displayed = true;
    strcat(LinuxCommand, ">/dev/null 2>/dev/null");
    system(LinuxCommand);
    strcpy(LinuxCommand, "(sleep 1; sudo killall -9 fbi >/dev/null 2>/dev/null) &");
    system(LinuxCommand);
    UpdateWeb();
  }
  else // MER display modes
  {
    clearScreen(0, 0, 0);
  }

  // Initialise and calculate the text display
  const font_t *font_ptr = &font_dejavu_sans_28;
  int txtht =  font_ptr->ascent;
  int txttot =  font_ptr->height;
  int txtdesc = font_ptr->height - font_ptr->ascent;
  int linepitch = (14 * txtht) / 10;

  // Initialise the MER display
  int bar_height;
  int bar_centre = hscreen * 0.30;
  int ls = wscreen * 38 / 40;
  int wdth = (wscreen * 2 / 40) - 1;

  // Create Wait Button thread
  pthread_create (&thbutton, NULL, &WaitButtonLMRX, NULL);

  // case 0 = DVB-S/S2 VLCFFUDP
  // case 1 = DVB-S/S2 Receive Diagnostics
  // case 2 = DVB-S/S2 VLCFFUDP2 No scan
  // case 3 = DVB-S/S2 UDP
  // case 4 = Beacon MER
  // case 5 = Autoset LNB LO Freq
  // case 6 = DVB-T VLC
  // case 7 = not used
  // case 8 = not used
  // case 9 = DVB-T UDP

  switch (NoButton)
  {
  case 0:
  case 2:

    if (NoButton == 0)
    {
      fp=popen(PATH_SCRIPT_LMRXVLCUDP, "r");
    }
    
    if (NoButton == 2)
    {
      fp=popen(PATH_SCRIPT_LMRXVLCUDP2, "r");
   }
    
    if(fp==NULL) printf("Process error\n");

    printf("STARTING VLC with FFMPEG RX\n");

    /* Open status FIFO for read only  */
    ret = mkfifo("longmynd_status_fifo", 0666);
    fd_status_fifo = open("longmynd_status_fifo", O_RDONLY); 

    // Set the status fifo to be non-blocking on empty reads
    fcntl(fd_status_fifo, F_SETFL, O_NONBLOCK);

    if (fd_status_fifo < 0)
    {
      printf("Failed to open status fifo\n");
    }
    printf("Listening, ret = %d\n", ret);

    while ((FinishedButton == 1) || (FinishedButton == 2)) // 1 is captions on, 2 is off
    {
      if (VLCResetRequest == true)
      {
        system("/home/pi/portsdown/scripts/receive/lmvlcreset.sh &");
        VLCResetRequest = false;
        FirstLock = 2;
        FinishedButton = 1;
      }

      num = read(fd_status_fifo, status_message_char, 1);

      if (num < 0)  // no character to read
      {
        usleep(500);
        if (TunerFound == false)
        {
          TunerPollCount = TunerPollCount + 1;

          if (TunerPollCount == 15)  // Tuner not responding
          {
            strcpy(line5, "Waiting for Tuner to Respond");
            Text(wscreen * 6 / 40, hscreen - 1 * linepitch, line5, font_ptr, 0, 0, 0, 255, 255, 255);
          }       
          // printf("TPC = %d\n", TunerPollCount);
          if (TunerPollCount > 2500)  // Maybe PicoTuner has locked up so reset USB bus power
          {
            strcpy(line5, "Resetting USB Bus                            ");
            Text(wscreen * 6 / 40, hscreen - 1 * linepitch, line5, font_ptr, 0, 0, 0, 255, 255, 255);
            system("sudo killall vlc");
            system("sudo uhubctl -R -a 2");
            MsgBox4(" ", "Touch Centre of Screen to Return to Receiver", " ", " ");
            wait_touch();
          }
        }
      }
      else // there was a character to read
      {
        status_message_char[num]='\0';
        // if (num>0) printf("%s\n",status_message_char);
        
        if (strcmp(status_message_char, "$") == 0)
        {
          TunerFound = true;

          if (Overlay_displayed == true)
          {
            clearScreen(0, 0, 0);
            Overlay_displayed = false;
          }

          if ((stat_string[0] == '1') && (stat_string[1] == ','))  // Decoder State
          {
            // Return State as an integer and a string
            STATE = LMDecoderState(stat_string, STATEtext);
          }

          if ((stat_string[0] == '6') && (stat_string[1] == ','))  // Frequency
          {
            strcpy(FREQtext, stat_string);
            chopN(FREQtext, 2);
            FREQ = atof(FREQtext);
            if (strcmp(LMRXmode, "sat") == 0)
            {
              FREQ = FREQ + LMRXqoffset;
            }
            FREQ = FREQ / 1000;
            snprintf(FREQtext, 15, "%.3f MHz", FREQ);
            //if ((TabBandLO[CurrentBand] < -0.5) || (TabBandLO[CurrentBand] > 0.5))      // band not direct
            //{
            //  strcat(FREQtext, " ");
            //  strcat(FREQtext, TabBandLabel[CurrentBand]);                             // so add band label
            //}
          }

          if ((stat_string[0] == '9') && (stat_string[1] == ','))  // SR in S
          {
            strcpy(SRtext, stat_string);
            chopN(SRtext, 2);
            SR = atoi(SRtext) / 1000;
            snprintf(SRtext, 15, "%d kS", SR);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '3'))  // Service Provider
          {
            strcpy(ServiceProvidertext, stat_string);
            chopN(ServiceProvidertext, 3);

            // Force VLC reset if service provider has changed
            if ((strlen(ServiceProvidertext) > 1) && (strlen(LastServiceProvidertext) > 1))
            {
              if (strcmp(ServiceProvidertext, LastServiceProvidertext) != 0)  // Service provider has changed
              {
                system("/home/pi/portsdown/scripts/receive/lmvlcreset.sh &");           // so reset VLC
                strcpy(LastServiceProvidertext, ServiceProvidertext);
              }
            }

            // deal with first receive case (no reset required)
            if ((FirstReceive == true) && (strlen(ServiceProvidertext) > 1))
            {
              strcpy(LastServiceProvidertext, ServiceProvidertext);
              FirstReceive = false;
            }
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '4'))  // Service
          {
            strcpy(Servicetext, stat_string);
            chopN(Servicetext, 3);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '8'))  // MODCOD
          {
            MERThreshold = LMLookupMODCOD(stat_string, STATE, FECtext, Modulationtext);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '7'))  // Video and audio encoding
          {
            if (LMLookupVidEncoding(stat_string, ExtraText) == 0)
            {
              strcpy(Encodingtext, ExtraText);
            }
            strcpy(ExtraText, "");

            if (LMLookupAudEncoding(stat_string, ExtraText) == 0)
            {
              strcat(Encodingtext, ExtraText);
            }
          }

          if ((stat_string[0] == '2') && (stat_string[1] == '6'))  // AGC1 Setting
          {
            strcpy(AGC1text, stat_string);
            chopN(AGC1text, 3);
            AGC1 = atoi(AGC1text);
          }

          if ((stat_string[0] == '2') && (stat_string[1] == '7'))  // AGC2 Setting
          {
            strcpy(AGC2text, stat_string);
            chopN(AGC2text, 3);
            AGC2 = atoi(AGC2text);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '2'))  // MER
          {
            if (FinishedButton == 1)  // Parameters requested to be displayed
            {

              // If they weren't displayed before, set the previousMER to 0 
              // so they get displayed and don't have to wait for an MER change
              if (Parameters_currently_displayed != 1)
              {
                previousMER = 0;
              }
              Parameters_currently_displayed = 1;
              strcpy(MERtext, stat_string);
              chopN(MERtext, 3);
              MER = atof(MERtext)/10;
              if (MER > 51)  // Trap spurious MER readings
              {
                MER = 0;
              }
              snprintf(MERtext, 24, "MER %.1f (%.1f needed)", MER, MERThreshold);
              snprintf(AGCtext, 24, "RF Input Level %d dB", CalcInputPwr(AGC1, AGC2));

              rectangle(wscreen * 1 / 40, hscreen - 1 * linepitch - txtdesc, wscreen * 30 / 40, txttot, 0, 0, 0);
              Text(wscreen * 1 / 40, hscreen - 1 * linepitch, STATEtext, font_ptr, 0, 0, 0, 255, 255, 255);
              rectangle(wscreen * 1 / 40, hscreen - 2 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              Text(wscreen * 1 / 40, hscreen - 2 * linepitch, FREQtext, font_ptr, 0, 0, 0, 255, 255, 255);
              rectangle(wscreen * 1 / 40, hscreen - 3 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              Text(wscreen * 1 / 40, hscreen - 3 * linepitch, SRtext, font_ptr, 0, 0, 0, 255, 255, 255);
              rectangle(wscreen * 1 / 40, hscreen - 4 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              Text(wscreen * 1 / 40, hscreen - 4 * linepitch, Modulationtext, font_ptr, 0, 0, 0, 255, 255, 255);
              rectangle(wscreen * 1 / 40, hscreen - 5 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              Text(wscreen * 1 / 40, hscreen - 5 * linepitch, FECtext, font_ptr, 0, 0, 0, 255, 255, 255);
              rectangle(wscreen * 1 / 40, hscreen - 6 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              Text(wscreen * 1 / 40, hscreen - 6 * linepitch, ServiceProvidertext, font_ptr, 0, 0, 0, 255, 255, 255);
              rectangle(wscreen * 1 / 40, hscreen - 7 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              Text(wscreen * 1 / 40, hscreen - 7 * linepitch, Servicetext, font_ptr, 0, 0, 0, 255, 255, 255);
              rectangle(wscreen * 1 / 40, hscreen - 8 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              Text(wscreen * 1 / 40, hscreen - 8 * linepitch, Encodingtext, font_ptr, 0, 0, 0, 255, 255, 255);

              rectangle(wscreen * 1 / 40, hscreen - 10 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              if (AGC1 < 1)  // Low input level
              {
                Text(wscreen * 1 / 40, hscreen - 10 * linepitch, AGCtext, font_ptr, 0, 0, 0, 255, 63, 63); // Red
              }
              else
              {
                Text(wscreen * 1 / 40, hscreen - 10 * linepitch, AGCtext, font_ptr, 0, 0, 0, 255, 255, 255); // White
              }

              rectangle(wscreen * 1 / 40, hscreen - 9 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
              if (MER < MERThreshold + 0.1)
              {
                Text(wscreen * 1 / 40, hscreen - 9 * linepitch, MERtext, font_ptr, 0, 0, 0, 255, 63, 63);  // Red
              }
              else  // Auto-hide the parameter display after 5 seconds
              {
                if (FirstLock == 0) // This is the first time MER has exceeded threshold
                {
                  FirstLock = 1;
                  LockTime = clock();  // Set first lock time
                }
                if ((clock() > LockTime + 600000) && (FirstLock == 1))  // About 5s since first lock
                {
                  FinishedButton = 2; // Hide parameters
                  FirstLock = 2;      // and stop it trying to hide them again
                }
                Text(wscreen * 1 / 40, hscreen - 9 * linepitch, MERtext, font_ptr, 0, 0, 0, 255, 255, 255);  // Wht
              }
              publish();

              // Only change VLC overlayfile if MER has changed
              if (MER != previousMER)
              {

                // Strip trailing line feeds from text strings
                ServiceProvidertext[strlen(ServiceProvidertext) - 1] = '\0';
                Servicetext[strlen(Servicetext) - 1] = '\0';

                // Build string for VLC
                strcpy(vlctext, STATEtext);
                strcat(vlctext, "%n");
                strcat(vlctext, FREQtext);
                strcat(vlctext, "%n");
                strcat(vlctext, SRtext);
                strcat(vlctext, "%n");
                strcat(vlctext, Modulationtext);
                strcat(vlctext, "%n");
                strcat(vlctext, FECtext);
                strcat(vlctext, "%n");
                strcat(vlctext, ServiceProvidertext);
                strcat(vlctext, "%n");
                strcat(vlctext, Servicetext);
                strcat(vlctext, "%n");
                strcat(vlctext, Encodingtext);
                strcat(vlctext, "%n");
                strcat(vlctext, MERtext);
                strcat(vlctext, "%n");
                strcat(vlctext, AGCtext);
                strcat(vlctext, "%n");
                if (timeOverlay == true)
                {
                  // Retrieve the current time
                  t = time(NULL);
                  strftime(TIMEtext, sizeof(TIMEtext), "%H:%M %d %b %Y", gmtime(&t));
                  strcat(vlctext, TIMEtext);
                }
                else
                {
                  strcat(vlctext, ".");
                }
                strcat(vlctext, "%nTouch Left to Hide Overlay%nTouch Centre to Exit");

                FILE *fw=fopen("/home/pi/tmp/vlc_temp_overlay.txt","w+");
                if(fw!=0)
                {
                  fprintf(fw, "%s\n", vlctext);
                }
                fclose(fw);

                // Copy temp file to file to be read by VLC to prevent file collisions
                system("cp /home/pi/tmp/vlc_temp_overlay.txt /home/pi/tmp/vlc_overlay.txt");

                previousMER = MER;
              }

              Text(wscreen * 1 / 40, hscreen - 11 * linepitch, "Touch Centre to exit", font_ptr, 0, 0, 0, 255, 255, 255);
              Text(wscreen * 1 / 40, hscreen - 12 * linepitch, "Touch Lower left for image capture", font_ptr, 0, 0, 0, 255, 255, 255);
            
            }
            else
            {
              if (Parameters_currently_displayed == 1)
              {
                clearScreen(0, 0, 0);
                Parameters_currently_displayed = 0;

                FILE *fw=fopen("/home/pi/tmp/vlc_overlay.txt","w+");
                if(fw!=0)
                {
                  fprintf(fw, " ");
                }
                fclose(fw);
              }
            }
            // Update web on alternate MER changes (to reduce processor workload)
            if (webupdate_this_time == true)
            {
              //refreshMouseBackground();
              //draw_cursor_foreground(mouse_x, mouse_y);
              UpdateWeb();
              webupdate_this_time = false;
            }
            else
            {
              webupdate_this_time = true;
            }
          }
          stat_string[0] = '\0';
        }
        else
        {
          strcat(stat_string, status_message_char);
        }
      }
    }

    close(fd_status_fifo); 
    usleep(1000);
    pclose(fp);
    touch_response = 0; 
    break;


  case 1:  // DVB-S/S2 Receive Diagnostics
    clearScreen(0, 0, 0);

    fp=popen(PATH_SCRIPT_LMRXUDP, "r");
    if(fp==NULL) printf("Process error\n");

    printf("STARTING Receive Diagnostics\n");

    // Open status FIFO
    ret = mkfifo("longmynd_status_fifo", 0666);
    fd_status_fifo = open("longmynd_status_fifo", O_RDONLY); 
    if (fd_status_fifo < 0)
    {
      printf("Failed to open status fifo\n");
    }

    printf("FinishedButton = %d\n", FinishedButton);

    while ((FinishedButton == 1) || (FinishedButton == 2)) // 1 is captions on, 2 is off
    {
      
      // printf("%s", "Start Read\n");

      num = read(fd_status_fifo, status_message_char, 1);
      // printf("%s Num= %d \n", "End Read", num);
      if (num >= 0 )
      {
        status_message_char[num]='\0';
        //if (num>0) printf("%s\n",status_message_char);
 
        if (strcmp(status_message_char, "$") == 0)
        {

          if ((stat_string[0] == '1') && (stat_string[1] == ','))  // Decoder State
          {
            // Return State as an integer and a string
            STATE = LMDecoderState(stat_string, STATEtext);
          }
          if ((stat_string[0] == '1') && (stat_string[1] == '5'))  // Null Ratio
          {
            strcpy(NullRatiotext, stat_string);
            chopN(NullRatiotext, 3);
            nullsarray[nullsindex] = atoi(NullRatiotext);
            nullsindex++;
            if (nullsindex > 99)
            {
              nullsindex = 0;
            }
          }

          if ((stat_string[0] == '6') && (stat_string[1] == ','))  // Frequency
          {
            strcpy(FREQtext, stat_string);
            chopN(FREQtext, 2);
            FREQ = atof(FREQtext);
            if (strcmp(LMRXmode, "sat") == 0)
            {
              FREQ = FREQ + LMRXqoffset;
            }
            FREQ = FREQ / 1000;
            snprintf(FREQtext, 15, "%.3f MHz", FREQ);
          }

          if ((stat_string[0] == '7') && (stat_string[1] == ','))  // I voltage
          {
            strcpy(Itext, stat_string);
            chopN(Itext, 2);
            Itextvalue = atoi(Itext);
            if (Itextvalue < 128)
            {
              Ivalue = Itextvalue;
            }
            else
            {
              Ivalue = Itextvalue - 255;
            }
          }

          if ((stat_string[0] == '8') && (stat_string[1] == ','))  // Q Voltage
          {
            strcpy(Qtext, stat_string);
            chopN(Qtext, 2);
            Qtextvalue = atoi(Qtext);
            if (Qtextvalue < 128)
            {
              Qvalue = Qtextvalue;
            }
            else
            {
              Qvalue = Qtextvalue - 255;
            }
            //printf("I %d  Q %d\n", Ivalue, Qvalue);
            //snprintf(udp_string, 15, "I %s, Q %s", Itext, Qtext);
            IQ[IQindex][0] = Ivalue;
            IQ[IQindex][1] = Qvalue;
            IQindex++;
            if (IQindex > 1023)
            {
              IQindex = 0;
            }
            //setPixel(600 + Ivalue, 300 + Qvalue, 255, 255, 128);
          }

          if ((stat_string[0] == '9') && (stat_string[1] == ','))  // SR in S
          {
            strcpy(SRtext, stat_string);
            chopN(SRtext, 2);
            SR = atoi(SRtext) / 1000;
            snprintf(SRtext, 15, "%d kS", SR);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '3'))  // Service Provider
          {
            strcpy(ServiceProvidertext, stat_string);
            chopN(ServiceProvidertext, 3);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '4'))  // Service
          {
            strcpy(Servicetext, stat_string);
            chopN(Servicetext, 3);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '8'))  // MODCOD
          {
            MERThreshold = LMLookupMODCOD(stat_string, STATE, FECtext, Modulationtext);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '7'))  // Video and audio encoding
          {
            if (LMLookupVidEncoding(stat_string, ExtraText) == 0)
            {
              strcpy(Encodingtext, ExtraText);
            }
            strcpy(ExtraText, "");

            if (LMLookupAudEncoding(stat_string, ExtraText) == 0)
            {
              strcat(Encodingtext, ExtraText);
            }
          }

          if ((stat_string[0] == '2') && (stat_string[1] == '6'))  // AGC1 Setting
          {
            strcpy(AGC1text, stat_string);
            chopN(AGC1text, 3);
            AGC1 = atoi(AGC1text);
          }

          if ((stat_string[0] == '2') && (stat_string[1] == '7'))  // AGC2 Setting
          {
            strcpy(AGC2text, stat_string);
            chopN(AGC2text, 3);
            AGC2 = atoi(AGC2text);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '2'))  // MER
          {
            strcpy(MERtext, stat_string);
            chopN(MERtext, 3);
            MER = atof(MERtext)/10;

            if (MER > 51)  // Trap spurious MER readings
            {
              MER = 0;
              strcpy(MERNtext, " ");
            }
            else if (MER >= 10)
            {
              snprintf(MERNtext, 10, "%.1f", MER);
            }
            else
            {
              snprintf(MERNtext, 10, "%.1f  ", MER);
            }
            snprintf(MERtext, 24, "MER %.1f (%.1f needed)", MER, MERThreshold);
            snprintf(AGCtext, 24, "RF Input Level %d dB", CalcInputPwr(AGC1, AGC2));

            // Clear the screen without publishing
            rectangle(0, 0, 799, 480, 0, 0, 0);

            Text(wscreen * 1 / 40, hscreen - 1 * linepitch, STATEtext, font_ptr, 0, 0, 0, 255, 255, 255);
            Text(wscreen * 1 / 40, hscreen - 2 * linepitch, FREQtext, font_ptr, 0, 0, 0, 255, 255, 255);
            Text(wscreen * 1 / 40, hscreen - 3 * linepitch, SRtext, font_ptr, 0, 0, 0, 255, 255, 255);
            Text(wscreen * 1 / 40, hscreen - 4 * linepitch, Modulationtext, font_ptr, 0, 0, 0, 255, 255, 255);
            Text(wscreen * 1 / 40, hscreen - 5 * linepitch, FECtext, font_ptr, 0, 0, 0, 255, 255, 255);
            Text(wscreen * 1 / 40, hscreen - 6 * linepitch, ServiceProvidertext, font_ptr, 0, 0, 0, 255, 255, 255);
            Text(wscreen * 1 / 40, hscreen - 7 * linepitch, Servicetext, font_ptr, 0, 0, 0, 255, 255, 255);
            Text(wscreen * 1 / 40, hscreen - 8 * linepitch, Encodingtext, font_ptr, 0, 0, 0, 255, 255, 255);

            if (AGC1 < 1)  // Low input level
            {
              Text(wscreen * 1 / 40, hscreen - 10 * linepitch, AGCtext, font_ptr, 0, 0, 0, 255, 63, 63);
            }
            else
            {
              Text(wscreen * 1 / 40, hscreen - 10 * linepitch, AGCtext, font_ptr, 0, 0, 0, 255, 255, 255);
            }

            if (MER < MERThreshold + 0.1)
            {
              Text(wscreen * 1 / 40, hscreen - 9 * linepitch, MERtext, font_ptr, 0, 0, 0, 255, 63, 63);
            }
            else
            {
              Text(wscreen * 1 / 40, hscreen - 9 * linepitch, MERtext, font_ptr, 0, 0, 0, 255, 255, 255);
            }

            Text(wscreen * 1 / 40, hscreen - 12 * linepitch, "Touch Centre to Exit", font_ptr, 0, 0, 0, 255, 255, 255);

            // Draw the Constellation
            rectangle(523, 173, 255, 255, 0, 0, 0);
            HorizLine(523, 300, 255, 255, 255, 255);
            VertLine (650, 173, 255, 255, 255, 255);

            float mag = 0;
            float magsum = 0;
            float nullstotal = 0;
            float magerrorpower = 0;
            float phase;
            float phaseerrorpower = 0;
            char magMERtext[63];
            char phaseMERtext[63];

            for (i = 0; i < 1024; i++)
            {
              setPixel(650 + IQ[i][0], 300 + IQ[i][1], 255, 255, 128);
              mag = sqrt(IQ[i][0] * IQ[i][0] + IQ[i][0] * IQ[i][0]);
              magsum = magsum + mag; 
              magerrorpower = magerrorpower + (avmag - mag) * (avmag - mag);

              if (IQ[i][0] == 0)
              {
                phase = PI / 4;
              }
              else
              {
                if (IQ[i][1] == 0)
                {
                  phase = 0;
                }
                else
                {
                  if ((IQ[i][0] > 0) && (IQ[i][1] > 0))  // top right quadrant
                  {
                    phase = atan((float)IQ[i][1] / (float)IQ[i][0]);
                  }
                  else if ((IQ[i][0] < 0) && (IQ[i][1] > 0))  // top left quadrant
                  {
                    phase = atan((float)IQ[i][1] / (-1 * (float)IQ[i][0]));
                  }
                  else if ((IQ[i][0] > 0) && (IQ[i][1] < 0))  // lower right quadrant
                  {
                    phase = atan((-1 * (float)IQ[i][1]) / (float)IQ[i][0]);
                  }
                  else if ((IQ[i][0] < 0) && (IQ[i][1] < 0))  // lower left quadrant
                  {
                    phase = atan((float)IQ[i][1] / (float)IQ[i][0]);
                  }
                }
              }
              phaseerrorpower = phaseerrorpower + ((phase - (PI / 4)) * (phase - (PI / 4)));
            }

            avmag = magsum/1024; // for use next time

            // Now calculate MER components (QPSK only)
            if (strcmp(FECtext, "QPSK") == 0)
            {
              magerrorpower = magerrorpower / 1024; // on a scale of 0 to 100 squared
              magerrorpower = 10 * log10(magerrorpower / 100);
              snprintf(magMERtext, 30, "Amplitude MER %0.1f dB", 14.5 - magerrorpower);
              Text(wscreen * 21 / 40, hscreen - 10 * linepitch, magMERtext, font_ptr, 0, 0, 0, 255, 255, 255);

              phaseerrorpower = phaseerrorpower / 1024;  // On a scale of 0 to PI/2 squared
              phaseerrorpower = 10 * log10(phaseerrorpower / ((PI / 2) * (PI / 2)));
              snprintf(phaseMERtext, 30, "Phase MER %0.1f dB", -6.5 - phaseerrorpower);
              Text(wscreen * 21 / 40, hscreen - 11 * linepitch, phaseMERtext, font_ptr, 0, 0, 0, 255, 255, 255);
            }

            for (i = 0; i < 100; i++)
            {
              nullstotal = nullstotal + nullsarray[i];
            }
            snprintf(NullRatiotext, 15, "Nulls %0.1f%%", nullstotal / 100);
            Text(wscreen * 1 / 40, hscreen - 11 * linepitch, NullRatiotext, font_ptr, 0, 0, 0, 255, 255, 255);

            publish();            
            UpdateWeb();
          }
          stat_string[0] = '\0';
        }
        else
        {
          strcat(stat_string, status_message_char);
        }
      }
      else
      {
        FinishedButton = 0;
      }
    } 
    close(fd_status_fifo); 
    usleep(1000);

    printf("Stopping receive process\n");
    pclose(fp);

    system("sudo killall lmudp.sh >/dev/null 2>/dev/null");
    touch_response = 0; 
    break;

  case 3:  // DVB-S/S2 UDP Output
    snprintf(udp_string, 63, "UDP Output to %s:%s", LMRXudpip, LMRXudpport);
    fp=popen(PATH_SCRIPT_LMRXUDP, "r");
    if(fp==NULL) printf("Process error\n");

    printf("STARTING UDP Player RX\n");

    // Open status FIFO
    ret = mkfifo("longmynd_status_fifo", 0666);
    fd_status_fifo = open("longmynd_status_fifo", O_RDONLY); 
    if (fd_status_fifo < 0)
    {
      printf("Failed to open status fifo\n");
    }

    printf("FinishedButton = %d\n", FinishedButton);

    while ((FinishedButton == 1) || (FinishedButton == 2)) // 1 is captions on, 2 is off
    {
      // printf("%s", "Start Read\n");

      num = read(fd_status_fifo, status_message_char, 1);
      // printf("%s Num= %d \n", "End Read", num);
      if (num >= 0 )
      {
        status_message_char[num]='\0';
        //if (num>0) printf("%s\n",status_message_char);
 
        if (strcmp(status_message_char, "$") == 0)
        {

          if ((stat_string[0] == '1') && (stat_string[1] == ','))  // Decoder State
          {
            // Return State as an integer and a string
            STATE = LMDecoderState(stat_string, STATEtext);
          }

          if ((stat_string[0] == '6') && (stat_string[1] == ','))  // Frequency
          {
            strcpy(FREQtext, stat_string);
            chopN(FREQtext, 2);
            FREQ = atof(FREQtext);
            if (strcmp(LMRXmode, "sat") == 0)
            {
              FREQ = FREQ + LMRXqoffset;
            }
            FREQ = FREQ / 1000;
            snprintf(FREQtext, 15, "%.3f MHz", FREQ);
          }

          if ((stat_string[0] == '9') && (stat_string[1] == ','))  // SR in S
          {
            strcpy(SRtext, stat_string);
            chopN(SRtext, 2);
            SR = atoi(SRtext) / 1000;
            snprintf(SRtext, 15, "%d kS", SR);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '3'))  // Service Provider
          {
            strcpy(ServiceProvidertext, stat_string);
            chopN(ServiceProvidertext, 3);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '4'))  // Service
          {
            strcpy(Servicetext, stat_string);
            chopN(Servicetext, 3);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '8'))  // MODCOD
          {
            MERThreshold = LMLookupMODCOD(stat_string, STATE, FECtext, Modulationtext);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '7'))  // Video and audio encoding
          {
            if (LMLookupVidEncoding(stat_string, ExtraText) == 0)
            {
              strcpy(Encodingtext, ExtraText);
            }
            strcpy(ExtraText, "");

            if (LMLookupAudEncoding(stat_string, ExtraText) == 0)
            {
              strcat(Encodingtext, ExtraText);
            }
          }

          if ((stat_string[0] == '2') && (stat_string[1] == '6'))  // AGC1 Setting
          {
            strcpy(AGC1text, stat_string);
            chopN(AGC1text, 3);
            AGC1 = atoi(AGC1text);
          }

          if ((stat_string[0] == '2') && (stat_string[1] == '7'))  // AGC2 Setting
          {
            strcpy(AGC2text, stat_string);
            chopN(AGC2text, 3);
            AGC2 = atoi(AGC2text);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '2'))  // MER
          {
            strcpy(MERtext, stat_string);
            chopN(MERtext, 3);
            MER = atof(MERtext)/10;

            if (MER > 51)  // Trap spurious MER readings
            {
              MER = 0;
              strcpy(MERNtext, " ");
            }
            else if (MER >= 10)
            {
              snprintf(MERNtext, 10, "%.1f", MER);
            }
            else
            {
              snprintf(MERNtext, 10, "%.1f  ", MER);
            }
            snprintf(MERtext, 24, "MER %.1f (%.1f needed)", MER, MERThreshold);
            snprintf(AGCtext, 24, "RF Input Level %d dB", CalcInputPwr(AGC1, AGC2));

            rectangle(wscreen * 1 / 40, hscreen - 1 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 1 * linepitch, STATEtext, font_ptr, 0, 0, 0, 255, 255, 255);
            rectangle(wscreen * 1 / 40, hscreen - 2 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 2 * linepitch, FREQtext, font_ptr, 0, 0, 0, 255, 255, 255);
            rectangle(wscreen * 1 / 40, hscreen - 3 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 3 * linepitch, SRtext, font_ptr, 0, 0, 0, 255, 255, 255);
            rectangle(wscreen * 1 / 40, hscreen - 4 * linepitch - txtdesc, wscreen * 17 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 4 * linepitch, Modulationtext, font_ptr, 0, 0, 0, 255, 255, 255);
            rectangle(wscreen * 1 / 40, hscreen - 5 * linepitch - txtdesc, wscreen * 17 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 5 * linepitch, FECtext, font_ptr, 0, 0, 0, 255, 255, 255);
            rectangle(wscreen * 1 / 40, hscreen - 6 * linepitch - txtdesc, wscreen * 17 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 6 * linepitch, ServiceProvidertext, font_ptr, 0, 0, 0, 255, 255, 255);
            rectangle(wscreen * 1 / 40, hscreen - 7 * linepitch - txtdesc, wscreen * 17 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 7 * linepitch, Servicetext, font_ptr, 0, 0, 0, 255, 255, 255);
            rectangle(wscreen * 1 / 40, hscreen - 8 * linepitch - txtdesc, wscreen * 17 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 8 * linepitch, Encodingtext, font_ptr, 0, 0, 0, 255, 255, 255);

            rectangle(wscreen * 1 / 40, hscreen - 10 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
            if (AGC1 < 1)  // Low input level
            {
              Text(wscreen * 1 / 40, hscreen - 10 * linepitch, AGCtext, font_ptr, 0, 0, 0, 255, 63, 63);
            }
            else
            {
              Text(wscreen * 1 / 40, hscreen - 10 * linepitch, AGCtext, font_ptr, 0, 0, 0, 255, 255, 255);
            }

            rectangle(wscreen * 1 / 40, hscreen - 9 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
            if (MER < MERThreshold + 0.1)
            {
              Text(wscreen * 1 / 40, hscreen - 9 * linepitch, MERtext, font_ptr, 0, 0, 0, 255, 63, 63);
            }
            else
            {
              Text(wscreen * 1 / 40, hscreen - 9 * linepitch, MERtext, font_ptr, 0, 0, 0, 255, 255, 255);
            }

            Text(wscreen * 1 / 40, hscreen - 11 * linepitch, udp_string, font_ptr, 0, 0, 0, 255, 255, 255);
            Text(wscreen * 1 / 40, hscreen - 12 * linepitch, "Touch Centre to Exit", font_ptr, 0, 0, 0, 255, 255, 255);

            // Display large MER number
            LargeText(wscreen * 18 / 40, hscreen * 19 / 48, 5, MERNtext, &font_dejavu_sans_32, 0, 0, 0, 255, 255, 255);
            //refreshMouseBackground();
            //draw_cursor_foreground(mouse_x, mouse_y);
            publish();            
            UpdateWeb();
          }
          stat_string[0] = '\0';
        }
        else
        {
          strcat(stat_string, status_message_char);
        }
      }
      else
      {
        FinishedButton = 0;
      }
    } 
    close(fd_status_fifo); 
    usleep(1000);

    printf("Stopping receive process\n");
    pclose(fp);

    system("sudo killall lmudp.sh >/dev/null 2>/dev/null");
    touch_response = 0; 
    break;

  case 4:  // MER Display
    snprintf(udp_string, 63, "UDP Output to %s:%s", LMRXudpip, LMRXudpport);
    fp=popen(PATH_SCRIPT_LMRXMER, "r");
    if(fp==NULL) printf("Process error\n");

    printf("STARTING MER Display\n");

    // Open status FIFO
    ret = mkfifo("longmynd_status_fifo", 0666);
    fd_status_fifo = open("longmynd_status_fifo", O_RDONLY); 
    if (fd_status_fifo < 0)
    {
      printf("Failed to open status fifo\n");
    }

    while ((FinishedButton == 1) || (FinishedButton == 2)) // 1 is captions on, 2 is off
    {
      num = read(fd_status_fifo, status_message_char, 1);

      if (num >= 0 )
      {
        status_message_char[num]='\0';
        if (strcmp(status_message_char, "$") == 0)
        {

          if ((stat_string[0] == '1') && (stat_string[1] == ','))  // Decoder State
          {
            // Return State as an integer and a string
            STATE = LMDecoderState(stat_string, STATEtext);
            strcat(STATEtext, " MER:");
          }

          if ((stat_string[0] == '2') && (stat_string[1] == '6'))  // AGC1 Setting
          {
            strcpy(AGC1text, stat_string);
            chopN(AGC1text, 3);
            AGC1 = atoi(AGC1text);
          }

          if ((stat_string[0] == '2') && (stat_string[1] == '7'))  // AGC2 Setting
          {
            strcpy(AGC2text, stat_string);
            chopN(AGC2text, 3);
            AGC2 = atoi(AGC2text);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '2'))  // MER
          {
            strcpy(MERtext, stat_string);
            chopN(MERtext, 3);
            MER = atof(MERtext)/10;
            if (MER > 51)  // Trap spurious MER readings
            {
              MER = 0;
              strcpy(MERtext, " ");
            }
            else if (MER >= 10)
            {
              snprintf(MERtext, 10, "%.1f", MER);
            }
            else
            {
              snprintf(MERtext, 10, "%.1f  ", MER);
            }

            snprintf(AGCtext, 24, "RF Input Level %d dB", CalcInputPwr(AGC1, AGC2));

            // Set up for the tuning bar

            if ((MER > 0.2) && (MERcount < 9))  // Wait for a valid MER
            {
              MERcount = MERcount + 1;
            }
            if (MERcount == 9)                 // Third valid MER
            {
              refMER = MER;
              MERcount = 10;
            }

            rectangle(0, 0, 800, 480, 0, 0, 0);  // Clear the back buffer

            if (AGC1 < 1)  // Low input level
            {
              Text(wscreen * 1 / 40, hscreen - 11 * linepitch, AGCtext, font_ptr, 0, 0, 0, 255, 63, 63); // red
            }
            else
            {
              Text(wscreen * 1 / 40, hscreen - 11 * linepitch, AGCtext, font_ptr, 0, 0, 0, 255, 255, 255); // red
            }

            Text(wscreen * 1 / 40, hscreen - 1 * linepitch, STATEtext, font_ptr, 0, 0, 0, 255, 255, 255);

            LargeText(wscreen * 1 / 40, hscreen - 9 * linepitch, 4, MERtext, &font_dejavu_sans_72, 0, 0, 0, 255, 255, 255);

            Text(wscreen * 1 / 40, hscreen - 12 * linepitch, "Touch Centre to Exit", font_ptr, 0, 0, 0, 255, 255, 255);

            if (MERcount == 10)
            {
              bar_height = hscreen * (MER - refMER) / 8; // zero is reference
              // valid pixels are 1 to wscreen (800) and 1 to hscreen (480)

              rectangle(ls - wdth, bar_centre -1, wdth, 2, 255, 255, 255); // White Reference line
              if (MER > refMER)  // Green rectangle
              {
                if ((bar_centre + bar_height ) > hscreen)  // off the top
                {
                  bar_height = hscreen - bar_centre;
                }
                rectangle(ls, bar_centre, wdth, bar_height, 0, 255, 0); // Green bar
              }
              else              // Red rectangle
              {
                if ((bar_centre + bar_height ) < 1)  // off the bottom
                {
                  bar_height = 1 - bar_centre;
                }
                rectangle(ls, bar_centre + bar_height, wdth, 0 - bar_height, 255, 0, 0); // Red bar
              }
            }
            publish();
            UpdateWeb();
          }
          stat_string[0] = '\0';
        }
        else
        {
          strcat(stat_string, status_message_char);
        }
      }
      else
      {
        FinishedButton = 0;
      }
    } 
    close(fd_status_fifo); 
    usleep(1000);

    printf("Stopping receive process\n");
    pclose(fp);

    system("sudo killall lmmer.sh >/dev/null 2>/dev/null");
    touch_response = 0; 
    break;

  case 5:  // AutoSet LNB Frequency

    clearScreen(0, 0, 0);
    fp=popen(PATH_SCRIPT_LMRXMER, "r");
    if(fp==NULL) printf("Process error\n");

    printf("STARTING Autoset LNB LO Freq\n");
    int oldLMRXqoffset = LMRXqoffset;
    LMRXqoffset = 0;

    // Open status FIFO
    ret = mkfifo("longmynd_status_fifo", 0666);
    fd_status_fifo = open("longmynd_status_fifo", O_RDONLY); 
    if (fd_status_fifo < 0)
    {
      printf("Failed to open status fifo\n");
    }
    while ((FinishedButton == 1) || (FinishedButton == 2)) // 1 is captions on, 2 is off
    {
      num = read(fd_status_fifo, status_message_char, 1);
      if (num >= 0 )
      {
        status_message_char[num]='\0';
        if (strcmp(status_message_char, "$") == 0)
        {

          if ((stat_string[0] == '1') && (stat_string[1] == ','))  // Decoder State
          {
            // Return State as an integer and a string
            STATE = LMDecoderState(stat_string, STATEtext);
          }

          if ((stat_string[0] == '6') && (stat_string[1] == ','))  // Frequency
          {
            strcpy(FREQtext, stat_string);
            chopN(FREQtext, 2);
            FREQ = atof(FREQtext);
            TUNEFREQ = atoi(FREQtext);
            if (strcmp(LMRXmode, "sat") == 0)
            {
              FREQ = FREQ + LMRXqoffset;
            }
            FREQ = FREQ / 1000;
            snprintf(FREQtext, 15, "%.3f MHz", FREQ);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '2'))  // MER
          {
            strcpy(MERtext, stat_string);
            chopN(MERtext, 3);
            MER = atof(MERtext)/10;
            if (MER > 51)  // Trap spurious MER readings
            {
              MER = 0;
            }
            snprintf(MERtext, 24, "MER %.1f", MER);

            // Set up for Frequency capture

            if ((MER > 0.2) && (MERcount < 9))  // Wait for a valid MER
            {
              MERcount = MERcount + 1;
            }
            if (MERcount == 9)                 // Third valid MER
            {
              refMER = MER;
              MERcount = 10;
            }

            rectangle(wscreen * 1 / 40, hscreen - 1 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 1 * linepitch, STATEtext, font_ptr, 0, 0, 0, 255, 255, 255);
            rectangle(wscreen * 1 / 40, hscreen - 3 * linepitch - txtdesc, wscreen * 19 / 40, txttot, 0, 0, 0);
            Text(wscreen * 1 / 40, hscreen - 3 * linepitch, MERtext, font_ptr, 0, 0, 0, 255, 255, 255);

            Text(wscreen * 1.0 / 40.0, hscreen - 4.5 * linepitch, "Previous LNB Offset", font_ptr, 0, 0, 0, 255, 255, 255);
            snprintf(FREQtext, 15, "%d kHz", oldLMRXqoffset);
            Text(wscreen * 1.0 / 40.0, hscreen - 5.5 * linepitch, FREQtext, font_ptr, 0, 0, 0, 255, 255, 255);
       
            // Make sure that the Tuner frequency is sensible
            if ((TUNEFREQ < 143000) || (TUNEFREQ > 2650000))
            {
              TUNEFREQ = 0;
            }

            if ((MERcount == 10) && (LMRXqoffset == 0) && (TUNEFREQ != 0))
            {
              Text(wscreen * 1.0 / 40.0, hscreen - 7.0 * linepitch, "Calculated New LNB Offset", font_ptr, 0, 0, 0, 255, 255, 255);
              LMRXqoffset = 10491500 - TUNEFREQ;
              snprintf(FREQtext, 15, "%d kHz", LMRXqoffset);
              Text(wscreen * 1.0 / 40.0, hscreen - 8.0 * linepitch, FREQtext, font_ptr, 0, 0, 0, 255, 255, 255);
              Text(wscreen * 1.0 / 40.0, hscreen - 9.0 * linepitch, "Saved to memory card", font_ptr, 0, 0, 0, 255, 255, 255);
              snprintf(Value, 15, "%d", LMRXqoffset);
              SetConfigParam(PATH_LMCONFIG, "qoffset", Value);
            }
            if ((MERcount == 10) && (LMRXqoffset != 0)) // Done, so just display results
            {
              Text(wscreen * 1.0 / 40.0, hscreen - 7.0 * linepitch, "Calculated New LNB Offset", font_ptr, 0, 0, 0, 255, 255, 255);
              snprintf(FREQtext, 15, "%d kHz", LMRXqoffset);
              Text(wscreen * 1.0 / 40.0, hscreen - 8.0 * linepitch, FREQtext, font_ptr, 0, 0, 0, 255, 255, 255);
            }
            Text(wscreen * 1.0 / 40.0, hscreen - 11.5 * linepitch, "Touch Centre of screen to exit", font_ptr, 0, 0, 0, 255, 255, 255);

            publish();
            UpdateWeb();
          }
          stat_string[0] = '\0';

        }
        else
        {
          strcat(stat_string, status_message_char);
        }
      }
      else
      {
        FinishedButton = 0;
      }
    } 
    close(fd_status_fifo); 
    usleep(1000);

    printf("Stopping receive process\n");
    pclose(fp);

    system("sudo killall lmmer.sh >/dev/null 2>/dev/null");
    touch_response = 0; 
    break;
  }
  system("sudo killall longmynd >/dev/null 2>/dev/null");
  system("sudo killall vlc >/dev/null 2>/dev/null");
  pthread_join(thbutton, NULL);
}


void playStreamFFPlay()
{
  int rawX;
  int rawY;

  MsgBox4(" ", " ", "Starting stream player", " ");

  system("/home/pi/portsdown/scripts/playstream/ffplay_stream_player.sh >/dev/null 2>/dev/null &");

  // Wait for touch
  while(getTouchSample(&rawX, &rawY) == 0)
  {
    usleep(10000);
  }
  system("sudo killall ffplay");
}


void playStreamVLC()
{
  int rawX;
  int rawY;

  MsgBox4(" ", " ", "Starting stream player", " ");

  system("/home/pi/portsdown/scripts/playstream/vlc_stream_player.sh >/dev/null 2>/dev/null &");

  // Wait for touch
  while(getTouchSample(&rawX, &rawY) == 0)
  {
    usleep(10000);
  }
  system("/home/pi/portsdown/scripts/playstream/vlc_stream_player_stop.sh >/dev/null 2>/dev/null &");
}


/***************************************************************************//**
 * @brief Clears the screen and then dispklays the current TX video from UDP 
 *
 * @param bool start, true to start the process, false to stop
 *
 * @return null
*******************************************************************************/

void ShowTXVideo(bool start)
{
  if (start == true)
  {
    clearScreen(0, 0, 0);
    // Code here to show PiCam, Test Card or "not available"
    MsgBox4(" ", "Not implemented yet", " ", " ");
  }
  else
  {
    // Code here to stop stuff being shown
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


// This function sets the button position and size for each button, and intialises the buttons as hidden
// Designed for 50 30-button menus

void CreateButtons()
{
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  int menu;
  int button;

  for (menu = 0; menu < 41; menu++)            // All except keyboard
  {
    for (button = 0; button < MAX_BUTTON_ON_MENU; button++)
    {
      if (button < 25)  // Bottom 4 rows
      {
        x = (button % 5) * wbuttonsize + 20;  // % operator gives the remainder of the division
        y = (button / 5) * hbuttonsize + 20;
        w = wbuttonsize * 0.9;
        h = hbuttonsize * 0.9;
      }
      else if (button == 25)  // TX button
      {
        x = (button % 5) * wbuttonsize *1.7 + 20;    // % operator gives the remainder of the division
        y = (button / 5) * hbuttonsize + 20;
        w = wbuttonsize * 0.9;
        h = hbuttonsize * 1.2;
      }
      else if ((button == 26) || (button == 27) || (button == 28) || (button == 29)) // RX/M1, M2 and M3 buttons
      {
        //x = ((button + 1) % 5) * wbuttonsize + 20;  // % operator gives the remainder of the division
        x = ((button) % 5) * wbuttonsize + 20;  // % operator gives the remainder of the division
        y = (button / 5) * hbuttonsize + 20;
        w = wbuttonsize * 0.9;
        h = hbuttonsize * 1.2;
      }
      else
      {
        x = 0;
        y = 0;
        w = 0;
        h = 0;
      }
      ButtonArray[menu][button].x = x;
      ButtonArray[menu][button].y = y;
      ButtonArray[menu][button].w = w;
      ButtonArray[menu][button].h = h;
      ButtonArray[menu][button].IndexStatus = 0;
    }
  }

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

int AddButtonStatus(int menu, int button_number, char *Text, color_t *Color)
{
  button_t *Button = &(ButtonArray[menu][button_number]);

  strcpy(Button->Status[Button->IndexStatus].Text, Text);
  Button->Status[Button->IndexStatus].Color= *Color;
  return Button->IndexStatus++;
}


//void SetButtonStatus(int ButtonIndex,int Status)
void SetButtonStatus(int menu, int button_number, int Status)
{
  button_t *Button = &(ButtonArray[menu][button_number]);
  Button->NoStatus = Status;
}


// int GetButtonStatus(int ButtonIndex)
int GetButtonStatus(int menu, int button_number)
{
  button_t *Button = &(ButtonArray[menu][button_number]);
  return Button->NoStatus;
}


//void SelectInGroupOnMenu(int Menu, int StartButton, int StopButton, int NumberButton, int Status)
void SelectInGroupOnMenu(int menu, int firstButton, int lastButton, int NumberButton, int Status)
{
  int i;

  for(i = firstButton; i <= lastButton ; i++)
  {
    if(i == NumberButton)
    {
      SetButtonStatus(menu, i, Status);
    }
    else
    {
      SetButtonStatus(menu, i, 0);
    }
  }
}


void SelectFromGroupOnMenu5(int menu, int firstButton, int Status, char *selection,
  char *value1, char *value2, char *value3, char *value4, char *value5)
{
  int match = 0;
  int newStatus = 0;

  if (strcmp(selection, value1) == 0)
  {
    match = 0;
    newStatus = Status;
  }
  else if (strcmp(selection, value2) == 0)
  {
    match = 1;
    newStatus = Status;
  }
  else if (strcmp(selection, value3) == 0)
  {
    match = 2;
    newStatus = Status;
  }
  else if (strcmp(selection, value4) == 0)
  {
    match = 3;
    newStatus = Status;
  }
  else if (strcmp(selection, value5) == 0)
  {
    match = 4;
    newStatus = Status;
  }

  SelectInGroupOnMenu(menu, firstButton, firstButton + 4, firstButton + match, newStatus);
}


void SelectFromGroupOnMenu4(int menu, int firstButton, int Status, char *selection,
  char *value1, char *value2, char *value3, char *value4)
{
  int match = 0;
  int newStatus = 0;

  if (strcmp(selection, value1) == 0)
  {
    match = 0;
    newStatus = Status;
  }
  else if (strcmp(selection, value2) == 0)
  {
    match = 1;
    newStatus = Status;
  }
  else if (strcmp(selection, value3) == 0)
  {
    match = 2;
    newStatus = Status;
  }
  else if (strcmp(selection, value4) == 0)
  {
    match = 3;
    newStatus = Status;
  }

  SelectInGroupOnMenu(menu, firstButton, firstButton + 3, firstButton + match, newStatus);
}


void SelectFromGroupOnMenu3(int menu, int firstButton, int Status, char *selection,
  char *value1, char *value2, char *value3)
{
  int match = 0;
  int newStatus = 0;

  if (strcmp(selection, value1) == 0)
  {
    match = 0;
    newStatus = Status;
  }
  else if (strcmp(selection, value2) == 0)
  {
    match = 1;
    newStatus = Status;
  }
  else if (strcmp(selection, value3) == 0)
  {
    match = 2;
    newStatus = Status;
  }

  SelectInGroupOnMenu(menu, firstButton, firstButton + 2, firstButton + match, newStatus);
}


void SelectFromGroupOnMenu2(int menu, int firstButton, int Status, char *selection,
  char *value1, char *value2)
{
  int match = 0;
  int newStatus = 0;

  if (strcmp(selection, value1) == 0)
  {
    match = 0;
    newStatus = Status;
  }
  else if (strcmp(selection, value2) == 0)
  {
    match = 1;
    newStatus = Status;
  }

  SelectInGroupOnMenu(menu, firstButton, firstButton + 1, firstButton + match, newStatus);
}


void SelectFromGroupOnMenu1(int menu, int firstButton, int Status, char *selection,
  char *value1)
{
  int match = 0;
  int newStatus = 0;

  if (strcmp(selection, value1) == 0)
  {
    match = 0;
    newStatus = Status;
  }

  SelectInGroupOnMenu(menu, firstButton, firstButton, firstButton + match, newStatus);
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
  
    // Draw green overlay half-button.  Menu 1, 2 lines and Button status = 0 only
    if ((CurrentMenu == 1) && (Button->NoStatus == 0))
    {
      // Draw the green box
      rectangle(Button->x, Button->y + 1, Button->w, Button->h/2,
        Button->Status[1].Color.r,
        Button->Status[1].Color.g,
        Button->Status[1].Color.b);

      // Display the text
      TextMid(Button->x+Button->w/2, Button->y+Button->h*3/16 - 1, line2, &font_dejavu_sans_20,
      Button->Status[1].Color.r, Button->Status[1].Color.g,
      Button->Status[1].Color.b, 255, 255, 255);	
    }
  }
  else                                              // One line only
  {
    if ((CurrentMenu <= 9) && (CurrentMenu != 4))
    {
      TextMid(Button->x+Button->w/2, Button->y+Button->h/2, label, &font_dejavu_sans_28,
      Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g,
      Button->Status[Button->NoStatus].Color.b, 255, 255, 255);
    }
    else if (CurrentMenu == 41)  // Keyboard
    {
      TextMid(Button->x+Button->w/2, Button->y+Button->h/2 - hscreen / 64, label, &font_dejavu_sans_28,
      Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g,
      Button->Status[Button->NoStatus].Color.b, 255, 255, 255);
    }
    else // fix text size at 20
    {
      TextMid(Button->x+Button->w/2, Button->y+Button->h/2, label, &font_dejavu_sans_20,
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


void ShowTitle()
{
  const font_t *font_ptr = &font_dejavu_sans_28;
  int txtht =  font_ptr->ascent;
  int linepitch = (12 * txtht) / 10;
  int linenumber = 1;

  if (CurrentMenu == 1)               // Main menu Black on White
  {
    TextMid(wscreen / 2.0, hscreen - linenumber * linepitch, MenuTitle[CurrentMenu], &font_dejavu_sans_28,
      255, 255, 255, 0, 0, 0);
  }
  else                               // All others white on black
  {
    TextMid(wscreen / 2.0, hscreen - linenumber * linepitch, MenuTitle[CurrentMenu], &font_dejavu_sans_28,
      0, 0, 0, 255, 255, 255);
  }
}


void redrawMenu()
// Paint each defined button and the title on the current Menu
{
  int i;

  // Set the background colour
  if (CurrentMenu == 1)           // Main Menu White
  {
    clearBackBuffer(255, 255, 255);
  }
  else                            // All others Black
  {
    clearBackBuffer(0, 0, 0);
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
//    case 10:
//      Highlight_Menu10();
//      break;
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
    case 15:
      Highlight_Menu15();
      break;
    case 16:
      Highlight_Menu16();
      break;
    case 17:
      Highlight_Menu17();
      break;
    case 18:
      Highlight_Menu18();
      break;
    case 19:
      Highlight_Menu19();
      break;
    case 20:
      Highlight_Menu20();
      break;
    case 21:
      Highlight_Menu21();
      break;
    case 22:
      Highlight_Menu22();
      break;
    case 23:
      Highlight_Menu23();
      break;
    case 24:
      Highlight_Menu24();
      break;
    case 25:
      Highlight_Menu25();
      break;
    case 26:
      Highlight_Menu26();
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

  // Show the title and any required text
  ShowTitle();
  //if ((CurrentMenu == 4) || (CurrentMenu == 33))  // File or update menus
  //{
  //  ShowMenuText();
  //}
  //refreshMouseBackground();
  //draw_cursor_foreground(mouse_x, mouse_y);
  UpdateWeb();
  //image_complete = true;
  publish();
}


void Define_Menu1()
{
  strcpy(MenuTitle[1], "Portsdown 5 DATV Transceiver Main Menu");

  AddButtonStatus(1, 0, "Shutdown^System", &Blue);
  AddButtonStatus(1, 0, "Shutdown^System", &Blue);
  AddButtonStatus(1, 0, "Really^Shutdown?", &Red);
  AddButtonStatus(1, 0, "Really^Shutdown?", &Red);

  AddButtonStatus(1, 1, "Band^Viewer", &Blue);
  AddButtonStatus(1, 1, "Band^Viewer", &Blue);

  AddButtonStatus(1, 2, "Test^Equipment", &Blue);
  AddButtonStatus(1, 2, "Test^Equipment", &Blue);

  AddButtonStatus(1, 3, "Locator^Calculator", &Blue);
  AddButtonStatus(1, 3, "Locator^Calculator", &Blue);

  AddButtonStatus(1, 4, "Stream^Viewer", &Blue);
  AddButtonStatus(1, 4, "Stream^Viewer", &Blue);

  AddButtonStatus(1, 5, "Preset 1^146_333", &Blue);
  AddButtonStatus(1, 5, "Preset 1^146_333", &Green);

  AddButtonStatus(1, 6, "Preset 2^437_Lime", &Blue);
  AddButtonStatus(1, 6, "Preset 2^437_Lime", &Green);

  AddButtonStatus(1, 7, "Preset 3^1255_HD", &Blue);
  AddButtonStatus(1, 7, "Preset 3^1255_HD", &Green);

  AddButtonStatus(1, 8, "Preset 4^437-Pluto", &Blue);
  AddButtonStatus(1, 8, "Preset 4^437-Pluto", &Green);

  AddButtonStatus(1, 9, "Store^Preset", &Blue);
  AddButtonStatus(1, 9, "Store^Preset", &Blue);
  AddButtonStatus(1, 9, "Store^Preset", &Red);
  AddButtonStatus(1, 9, "Store^Preset", &Red);

  AddButtonStatus(1, 10, "EasyCap^Comp Vid", &Blue);
  AddButtonStatus(1, 10, "EasyCap^Comp Vid", &Green);

  AddButtonStatus(1, 11, "Caption^On", &Blue);
  AddButtonStatus(1, 11, "Caption^On", &Green);

  AddButtonStatus(1, 12, "Audio^Auto", &Blue);
  AddButtonStatus(1, 12, "Audio^Auto", &Green);

  AddButtonStatus(1, 13, "Atten^None", &Blue);
  AddButtonStatus(1, 13, "Atten^None", &Green);

  AddButtonStatus(1, 14, "Att Level^-10.00", &Blue);
  AddButtonStatus(1, 14, "Att Level^-10.00", &Green);

  AddButtonStatus(1, 15, " Freq ^437 MHz", &Blue);
  AddButtonStatus(1, 15, " Freq ^437 MHz", &Green);

  AddButtonStatus(1, 16, "Sym Rate^333", &Blue);
  AddButtonStatus(1, 16, "Sym Rate^333", &Green);

  AddButtonStatus(1, 17, "  FEC  ^2/3", &Blue);
  AddButtonStatus(1, 17, "  FEC  ^2/3", &Green);

  AddButtonStatus(1, 18, "Band/Tvtr^70cm",&Blue);
  AddButtonStatus(1, 18, "Band/Tvtr^70cm",&Green);

  AddButtonStatus(1, 19, "Lime Gain^88",&Blue);
  AddButtonStatus(1, 19, "Lime Gain^88",&Green);

  AddButtonStatus(1, 20, "Modulation^ ", &Blue);
  AddButtonStatus(1, 20, "Modulation^QPSK", &Green);

  AddButtonStatus(1, 21, "Encoding^H264", &Blue);
  AddButtonStatus(1, 21, "Encoding^H264", &Green);
  AddButtonStatus(1, 21, "Encoding^H264", &Red);
  AddButtonStatus(1, 21, "Encoding^H264", &Red);

  AddButtonStatus(1, 22, "Output to^Lime Mini", &Blue);
  AddButtonStatus(1, 22, "Output to^Lime Mini", &Green);

  AddButtonStatus(1, 23, "Format^16:9", &Blue);
  AddButtonStatus(1, 23, "Format^16:9", &Green);

  AddButtonStatus(1, 24, "Source^Pi Cam", &Blue);
  AddButtonStatus(1, 24, "Source^Pi Cam", &Green);

  AddButtonStatus(1, 25,"Transmit^Select",&Blue);
  AddButtonStatus(1, 25,"Transmit^Select",&Blue);
  AddButtonStatus(1, 25,"Transmit^ON",&Red);
  AddButtonStatus(1, 25,"Transmit^ON",&Red);

  AddButtonStatus(1, 26,"Receive^Menu",&Blue);
  AddButtonStatus(1, 26,"Receive^Menu",&Blue);

  AddButtonStatus(1, 27,"DATV Tools^Menu",&Blue);
  AddButtonStatus(1, 27,"DATV Tools^Menu",&Blue);

  AddButtonStatus(1, 28,"Config^Menu",&Blue);
  AddButtonStatus(1, 28,"Config^Menu",&Blue);

  AddButtonStatus(1, 29,"File^Menu",&Blue);
  AddButtonStatus(1, 29,"File^Menu",&Blue);

}


void Highlight_Menu1()
{
  char freqLabel[63];
  char srLabel[63];
  char limegainLabel[63];
  bool isLime = false;
  char fecLabel[63];

  // Display Correct Modulation on Button 20
  if (strcmp(config.modulation, "DVBS") == 0)
  {
    AmendButtonStatus(1, 20, 0, "Modulation^DVB-S", &Blue);
    AmendButtonStatus(1, 20, 1, "Modulation^DVB-S", &Green);
  }
  if (strcmp(config.modulation, "S2QPSK") == 0)
  {
    AmendButtonStatus(1, 20, 0, "Modulation^DVB-S2 QPSK", &Blue);
    AmendButtonStatus(1, 20, 1, "Modulation^DVB-S2 QPSK", &Green);
  }
  if (strcmp(config.modulation, "8PSK") == 0)
  {
    AmendButtonStatus(1, 20, 0, "Modulation^DVB-S2 8PSK", &Blue);
    AmendButtonStatus(1, 20, 1, "Modulation^DVB-S2 8PSK", &Green);
  }
  if (strcmp(config.modulation, "16APSK") == 0)
  {
    AmendButtonStatus(1, 20, 0, "Modulation^S2 16APSK", &Blue);
    AmendButtonStatus(1, 20, 1, "Modulation^S2 16APSK", &Green);
  }
  if (strcmp(config.modulation, "32APSK") == 0)
  {
    AmendButtonStatus(1, 20, 0, "Modulation^S2 32APSK", &Blue);
    AmendButtonStatus(1, 20, 1, "Modulation^S2 32APSK", &Green);
  }

  // Display Correct Encoding on Button 21
  if (strcmp(config.encoding, "IPTS in") == 0)
  {
    AmendButtonStatus(1, 21, 0, "Encoding^IPTS in", &Blue);
    AmendButtonStatus(1, 21, 1, "Encoding^IPTS in", &Green);
  }
  if (strcmp(config.encoding, "IPTS in H264") == 0)
  {
    AmendButtonStatus(1, 21, 0, "Encoding^IPTS in H264", &Blue);
    AmendButtonStatus(1, 21, 1, "Encoding^IPTS in H264", &Green);
  }
  if (strcmp(config.encoding, "IPTS in H265") == 0)
  {
    AmendButtonStatus(1, 21, 0, "Encoding^IPTS in H265", &Blue);
    AmendButtonStatus(1, 21, 1, "Encoding^IPTS in H265", &Green);
  }
  if (strcmp(config.encoding, "MPEG-2") == 0)
  {
    AmendButtonStatus(1, 21, 0, "Encoding^MPEG-2", &Blue);
    AmendButtonStatus(1, 21, 1, "Encoding^MPEG-2", &Green);
  }
  if (strcmp(config.encoding, "H264") == 0)
  {
    AmendButtonStatus(1, 21, 0, "Encoding^H264", &Blue);
    AmendButtonStatus(1, 21, 1, "Encoding^H264", &Green);
  }
  if (strcmp(config.encoding, "H265") == 0)
  {
    AmendButtonStatus(1, 21, 0, "Encoding^H265", &Blue);
    AmendButtonStatus(1, 21, 1, "Encoding^H265", &Green);
  }
  if (strcmp(config.encoding, "H266") == 0)
  {
    AmendButtonStatus(1, 21, 0, "Encoding^H266", &Blue);
    AmendButtonStatus(1, 21, 1, "Encoding^H266", &Green);
  }
  if (strcmp(config.encoding, "TS File") == 0)
  {
    AmendButtonStatus(1, 21, 0, "Encoding^from TS File", &Blue);
    AmendButtonStatus(1, 21, 1, "Encoding^from TS File", &Green);
  }

  // Display Correct Output Device on Button 22
  if (strcmp(config.modeoutput, "STREAMER") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Streamer", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Streamer", &Green);
    isLime = false;
  }
  if (strcmp(config.modeoutput, "IPTSOUT") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^IPTS", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^IPTS", &Green);
    isLime = false;
  }
  if (strcmp(config.modeoutput, "HDMI") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^HDMI", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^HDMI", &Green);
    isLime = false;
  }
  if (strcmp(config.modeoutput, "PLUTO") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Pluto", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Pluto", &Green);
    isLime = false;
  }
  if (strcmp(config.modeoutput, "PLUTOF5OEO") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Pluto (F5OEO)", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Pluto (F5OEO)", &Green);
    isLime = false;
  }
  if (strcmp(config.modeoutput, "EXPRESS16") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Express 16", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Express 16", &Green);
    isLime = false;
  }
  if (strcmp(config.modeoutput, "EXPRESS32") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Express 32", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Express 32", &Green);
    isLime = false;
  }
  if (strcmp(config.modeoutput, "LIMEMINI") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime Mini", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime Mini", &Green);
    isLime = true;
  }
  if (strcmp(config.modeoutput, "LIMEDVB") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime DVB", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime DVB", &Green);
    isLime = true;
  }
  if (strcmp(config.modeoutput, "LIMEUSB") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime USB", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime USB", &Green);
    isLime = true;
  }
  if (strcmp(config.modeoutput, "LIMEXTRX") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime XTRX", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime XTRX", &Green);
    isLime = true;
  }
  if (strcmp(config.modeoutput, "LIMEMINING") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime Mini NG", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime Mini NG", &Green);
    isLime = true;
  }

  // Display Correct Output Format on Button 23
  if (strcmp(config.format, "4:3") == 0)
  {
    AmendButtonStatus(1, 23, 0, "Format^4:3", &Blue);
    AmendButtonStatus(1, 23, 1, "Format^4:3", &Green);
  }
  if (strcmp(config.format, "16:9") == 0)
  {
    AmendButtonStatus(1, 23, 0, "Format^16:9", &Blue);
    AmendButtonStatus(1, 23, 1, "Format^16:9", &Green);
  }
  if (strcmp(config.format, "720p") == 0)
  {
    AmendButtonStatus(1, 23, 0, "Format^720p", &Blue);
    AmendButtonStatus(1, 23, 1, "Format^720p", &Green);
  }
  if (strcmp(config.format, "1080p") == 0)
  {
    AmendButtonStatus(1, 23, 0, "Format^1080p", &Blue);
    AmendButtonStatus(1, 23, 1, "Format^1080p", &Green);
  }

  // Display source on button 24
  if (strcmp(config.videosource, "PiCam") == 0)
  {
    AmendButtonStatus(1, 24, 0, "Source^Pi Cam", &Blue);
    AmendButtonStatus(1, 24, 1, "Source^Pi Cam", &Green);
  }
  if (strcmp(config.videosource, "WebCam") == 0)
  {
    AmendButtonStatus(1, 24, 0, "Source^Web Cam", &Blue);
    AmendButtonStatus(1, 24, 1, "Source^Web Cam", &Green);
  }
  if (strcmp(config.videosource, "ATEMUSB") == 0)
  {
    AmendButtonStatus(1, 24, 0, "Source^ATEM USB", &Blue);
    AmendButtonStatus(1, 24, 1, "Source^ATEM USB", &Green);
  }
  if (strcmp(config.videosource, "CAMLINK4K") == 0)
  {
    AmendButtonStatus(1, 24, 0, "Source^CamLink 4K", &Blue);
    AmendButtonStatus(1, 24, 1, "Source^CamLink 4K", &Green);
  }
  if (strcmp(config.videosource, "EasyCap") == 0)
  {
    AmendButtonStatus(1, 24, 0, "Source^EasyCap", &Blue);
    AmendButtonStatus(1, 24, 1, "Source^EasyCap", &Green);
  }
  if (strcmp(config.videosource, "TestCard") == 0)
  {
    AmendButtonStatus(1, 24, 0, "Source^Test Card", &Blue);
    AmendButtonStatus(1, 24, 1, "Source^Test Card", &Green);
  }
  
  // Display frequency on button 15
  snprintf(freqLabel, 60, "Freq^%.2f", config.freqoutput);
  AmendButtonStatus(1, 15, 0, freqLabel, &Blue);
  AmendButtonStatus(1, 15, 1, freqLabel, &Green);

  // Display SR on button 16
  snprintf(srLabel, 60, "Sym Rate^%d", config.symbolrate);
  AmendButtonStatus(1, 16, 0, srLabel, &Blue);
  AmendButtonStatus(1, 16, 1, srLabel, &Green);

  // Display FEC on button 17
  switch(config.fec)
  {
    case 14:
      strcpy(fecLabel, "FEC^1/4");
      break;
    case 13:
      strcpy(fecLabel, "FEC^1/3");
      break;
    case 12:
      strcpy(fecLabel, "FEC^1/2");
      break;
    case 35:
      strcpy(fecLabel, "FEC^3/5");
      break;
    case 23:
      strcpy(fecLabel, "FEC^2/3");
      break;
    case 34:
      strcpy(fecLabel, "FEC^3/4");
      break;
    case 56:
      strcpy(fecLabel, "FEC^5/6");
      break;
    case 78:
      strcpy(fecLabel, "FEC^7/8");
      break;
    case 89:
      strcpy(fecLabel, "FEC^8/9");
      break;
    case 910:
      strcpy(fecLabel, "FEC^9/10");
      break;
  }
  AmendButtonStatus(1, 17, 0, fecLabel, &Blue);
  AmendButtonStatus(1, 17, 1, fecLabel, &Green);

  // Display Band on button 18
  //snprintf(srLabel, 60, "Sym Rate^%d", config.symbolrate);
  //AmendButtonStatus(1, 16, 0, srLabel, &Blue);
  //AmendButtonStatus(1, 16, 1, srLabel, &Green);

  // Display Gain on button 19
  if (isLime)
  {
    snprintf(limegainLabel, 60, "Lime Gain^%d", config.limegain);
    AmendButtonStatus(1, 19, 0, limegainLabel, &Blue);
    AmendButtonStatus(1, 19, 1, limegainLabel, &Green);
  }
  else
  {
    AmendButtonStatus(1, 19, 0, "Gain", &Blue);
    AmendButtonStatus(1, 19, 1, "Gain", &Green);
  }
}


void Define_Menu2()
{
  strcpy(MenuTitle[2], "Portsdown 5 DATV Receive Menu (8)");

  AddButtonStatus(2, 4, "Return to^Main Menu", &Blue);
}


void Highlight_Menu2()
{

}


void Define_Menu3()
{
  strcpy(MenuTitle[3], "Portsdown 5 Config Menu (3)");

  AddButtonStatus(3, 0, "Check for^Update", &Blue);

  AddButtonStatus(3, 1, "System^Config", &Blue);

  AddButtonStatus(3, 2, "Disp/Ctrl^Config", &Blue);

  AddButtonStatus(3, 4, "Return to^Main Menu", &Blue);

  AddButtonStatus(3, 20, "Set Stream^Outputs", &Blue);

}


void Highlight_Menu3()
{

}


void Define_Menu4()
{
  strcpy(MenuTitle[4], "Portsdown 5 File Menu (4)");

  AddButtonStatus(4, 0, "Reboot^System", &Blue);

  AddButtonStatus(4, 1, "Restart^Portsdown", &Blue);

  AddButtonStatus(4, 2, "Info", &Blue);

  AddButtonStatus(4, 3, "KN", &Blue);

  AddButtonStatus(4, 4, "Return to^Main Menu", &Blue);


}


void Highlight_Menu4()
{

}

void Define_Menu5()
{
  strcpy(MenuTitle[5], "Portsdown 5 DATV Tools Menu (5)");

  AddButtonStatus(5, 5, "IPTS^Monitor", &Blue);

  AddButtonStatus(5, 4, "Return to^Main Menu", &Blue);


}


void Highlight_Menu5()
{

}


void Define_Menu6()
{
  strcpy(MenuTitle[6], "System Config Menu (6)");

  AddButtonStatus(6, 0, "Start-up^App", &Blue);

  AddButtonStatus(6, 4, "Return to^Main Menu", &Blue);
}


void Highlight_Menu6()
{

}



void Define_Menu7()
{
  strcpy(MenuTitle[7], "Portsdown 5 Test Equipment Menu (7)");

  AddButtonStatus(7, 20, "KeyLimePi^BandViewer", &Blue);
  AddButtonStatus(7, 20, "KeyLimePi^BandViewer", &Grey);

  AddButtonStatus(7, 21, "KeyLimePi^NF Meter", &Blue);
  AddButtonStatus(7, 21, "KeyLimePi^NF Meter", &Grey);

  AddButtonStatus(7, 22, "KeyLimePi^Noise Meter", &Blue);
  AddButtonStatus(7, 22, "KeyLimePi^Noise Meter", &Grey);

  AddButtonStatus(7, 23, "KeyLimePi^Power Meter", &Blue);
  AddButtonStatus(7, 23, "KeyLimePi^Power Meter", &Grey);

  AddButtonStatus(7, 15, "LimeSDR^BandViewer", &Blue);
  AddButtonStatus(7, 15, "LimeSDR^BandViewer", &Grey);

  AddButtonStatus(7, 16, "LimeSDR NG^BandViewer", &Blue);
  AddButtonStatus(7, 16, "LimeSDR NG^BandViewer", &Grey);

  AddButtonStatus(7, 17, "LimeSDR^NF Meter", &Blue);
  AddButtonStatus(7, 17, "LimeSDR^NF Meter", &Grey);

  AddButtonStatus(7, 18, "LimeSDR NG^NF Meter", &Blue);
  AddButtonStatus(7, 18, "LimeSDR NG^NF Meter", &Grey);

  AddButtonStatus(7, 19, "LimeSDR^Noise Meter", &Blue);
  AddButtonStatus(7, 19, "LimeSDR^Noise Meter", &Grey);

  AddButtonStatus(7, 10, "Signal^Generator", &Blue);
  AddButtonStatus(7, 10, "Signal^Generator", &Grey);

  AddButtonStatus(7, 11, "Frequency^Sweeper", &Blue);
  AddButtonStatus(7, 11, "Frequency^Sweeper", &Grey);

  AddButtonStatus(7, 12, "DMM^Display", &Blue);
  AddButtonStatus(7, 12, "DMM^ Display", &Grey);

  AddButtonStatus(7, 13, "AD Power^Detector", &Blue);
  AddButtonStatus(7, 13, "AD Power^Detector", &Grey);

  AddButtonStatus(7, 14, "PicoViewer^ ", &Blue);
  AddButtonStatus(7, 14, "PicoViewer^ ", &Grey);

  AddButtonStatus(7, 0, "Switch to^KeyLimePi SA", &Blue);
  AddButtonStatus(7, 0, "Switch to^KeyLimePi SA", &Grey);

  AddButtonStatus(7, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu7()
{
  int i;

  for (i = 15; i < 20; i++)
  {
    SetButtonStatus(7, i, 0);
  }
  for (i = 20; i < 24; i++)
  {
    SetButtonStatus(7, i, 0);
  }

  if (KeyLimePiEnabled == false)
  {
    SetButtonStatus(7, 0, 1);
    SetButtonStatus(7, 20, 1);
    SetButtonStatus(7, 21, 1);
    SetButtonStatus(7, 22, 1);
    SetButtonStatus(7, 23, 1);
  }
  else
  {
    SetButtonStatus(7, 0, 0);
    SetButtonStatus(7, 15, 1);
    SetButtonStatus(7, 16, 1);
    SetButtonStatus(7, 17, 1);
    SetButtonStatus(7, 18, 1);
  }
}


void Define_Menu8()
{
  strcpy(MenuTitle[8], "Portsdown Receiver Menu (8)"); 

  // Bottom Row, Menu 8

  AddButtonStatus(8, 0, "RECEIVE", &Blue);
  AddButtonStatus(8, 0, "RECEIVE", &Grey);

  AddButtonStatus(8, 1, "Receive^Diagnostics", &Blue);
  AddButtonStatus(8, 1, "Receive^Diagnostics", &Grey);

  AddButtonStatus(8, 2, "RX DVB-S2^Exact Freq", &Blue);
  AddButtonStatus(8, 2, "RX DVB-S2^Exact Freq", &Grey);

  AddButtonStatus(8, 3, "Play to^UDP Stream", &Blue);
  AddButtonStatus(8, 3, "Play to^UDP Stream", &Grey);

  AddButtonStatus(8, 4, "Beacon^MER", &Blue);
  AddButtonStatus(8, 4, "Beacon^MER", &Grey);
  AddButtonStatus(8, 4, "Band Viewer^on RX freq", &Blue);
  AddButtonStatus(8, 4, "Band Viewer^on RX freq", &Grey);

  // 2nd Row, Menu 8.  

  AddButtonStatus(8, 5, " ", &Blue);
  AddButtonStatus(8, 5, " ", &Green);

  AddButtonStatus(8, 6, " ", &Blue);
  AddButtonStatus(8, 6, " ", &Green);

  AddButtonStatus(8, 7, " ", &Blue);
  AddButtonStatus(8, 7, " ", &Green);

  AddButtonStatus(8, 8, " ", &Blue);
  AddButtonStatus(8, 8, " ", &Green);

  AddButtonStatus(8, 9, " ", &Blue);
  AddButtonStatus(8, 9, " ", &Green);

  // 3rd line up Menu 8

  AddButtonStatus(8, 10, " ", &Blue);
  AddButtonStatus(8, 10, " ", &Green);

  AddButtonStatus(8, 11, " ", &Blue);
  AddButtonStatus(8, 11, " ", &Green);

  AddButtonStatus(8, 12, " ", &Blue);
  AddButtonStatus(8, 12, " ", &Green);

  AddButtonStatus(8, 13," ",&Blue);
  AddButtonStatus(8, 13," ",&Green);

  AddButtonStatus(8, 14," ",&Blue);
  AddButtonStatus(8, 14," ",&Green);

  // 4th line up Menu 8

  AddButtonStatus(8, 15, " ", &Blue);
  AddButtonStatus(8, 15, " ", &Green);

  AddButtonStatus(8, 16, " ", &Blue);
  AddButtonStatus(8, 16, " ", &Green);

  AddButtonStatus(8, 17, " ", &Blue);
  AddButtonStatus(8, 17, " ", &Green);

  AddButtonStatus(8, 18, " ", &Blue);
  AddButtonStatus(8, 18, " ", &Green);

  AddButtonStatus(8, 19, " ", &Blue);
  AddButtonStatus(8, 19, " ", &Green);

  // 5th line up Menu 8

  AddButtonStatus(8, 20, " ", &Blue);
  AddButtonStatus(8, 20, " ", &Green);

  AddButtonStatus(8, 21, " ", &Blue);
  AddButtonStatus(8, 21, " ", &Green);

  AddButtonStatus(8, 22, " ", &Blue);
  AddButtonStatus(8, 22, " ", &Green);

  AddButtonStatus(8, 23, " ", &Blue);
  AddButtonStatus(8, 23, " ", &Green);

  AddButtonStatus(8, 24, " ", &Blue);
  AddButtonStatus(8, 24, " ", &Green);

  // - Top of Menu 8

  AddButtonStatus(8, 25, "QO-100", &Blue);
  AddButtonStatus(8, 25, "Terre^strial", &Blue);

  AddButtonStatus(8, 27, "EXIT", &Blue);
  AddButtonStatus(8, 27, "EXIT", &Red);

  AddButtonStatus(8, 28, "Config", &Blue);
  AddButtonStatus(8, 28, "Config", &Green);

  AddButtonStatus(8, 29, "DVB-S/S2", &Blue);
  AddButtonStatus(8, 29, "DVB-T/T2", &Blue);
}


void Highlight_Menu8()
{
  int indexoffset = 0;
  char LMBtext[11][21];
  char LMBStext[31];
  int i;
  int FreqIndex;
  div_t div_10;
  div_t div_100;
  div_t div_1000;

  if (strcmp(RXmod, "DVB-S") == 0)
  {
    strcpy(MenuTitle[8], "Portsdown DVB-S/S2 Receiver Menu (8)"); 
    SetButtonStatus(8, 1, 0);
    SetButtonStatus(8, 1, 0);
    SetButtonStatus(8, 1, 0);
  }
  else
  {
    strcpy(MenuTitle[8], "Portsdown DVB-T/T2 Receiver Menu (8)"); 
    SetButtonStatus(8, 1, 1);
    SetButtonStatus(8, 1, 1);
    SetButtonStatus(8, 1, 1);
  }

  // Sort Beacon MER Button / BandViewer Button

  if (strcmp(LMRXmode, "terr") == 0)
  {
    indexoffset = 10;
//    if((CheckLimeMiniConnect() == 0) || (CheckLimeUSBConnect() == 0) || (CheckAirspyConnect() == 0) || (CheckRTL() == 0)
//        || (CheckSDRPlay() == 0) || (CheckPlutoIPConnect() == 0))
    {
      SetButtonStatus(8, 4, 2);
    }
//    else  // Grey out BandViewer Button
//    {
      SetButtonStatus(8, 4, 3);
//    }
  }
  else     // QO-100
  {
    if (strcmp(RXmod, "DVB-T") == 0)  // Grey out for DVB-T
    {
      SetButtonStatus(8, 4, 1);
    }
    else                             // DVB-S or S2
    {
      SetButtonStatus(8, 4, 0);
    }
  }

  // Freq buttons

  for(i = 1; i <= 10; i = i + 1)
  {
    if (i <= 5)
    {
      FreqIndex = i + 5 + indexoffset;
    }
    else
    {
      FreqIndex = i + indexoffset - 5;    
    }
    div_10 = div(LMRXfreq[FreqIndex], 10);
    div_1000 = div(LMRXfreq[FreqIndex], 1000);

    if(div_10.rem != 0)  // last character not zero, so make answer of form xxx.xxx
    {
      snprintf(LMBtext[i], 15, "%d.%03d", div_1000.quot, div_1000.rem);
    }
    else
    {
      div_100 = div(LMRXfreq[FreqIndex], 100);

      if(div_100.rem != 0)  // last but one character not zero, so make answer of form xxx.xx
      {
        snprintf(LMBtext[i], 15, "%d.%02d", div_1000.quot, div_1000.rem / 10);
      }
      else
      {
        if(div_1000.rem != 0)  // last but two character not zero, so make answer of form xxx.x
        {
          snprintf(LMBtext[i], 15, "%d.%d", div_1000.quot, div_1000.rem / 100);
        }
        else  // integer MHz, so just xxx.0
        {
          snprintf(LMBtext[i], 15, "%d.0", div_1000.quot);
        }
      }
    }
    if (i == 5)
    {
      strcat(LMBtext[i], "^Keyboard");
    }
    else
    {
      strcat(LMBtext[i], "^MHz");
    }

    AmendButtonStatus(8, i + 4, 0, LMBtext[i], &Blue);
    AmendButtonStatus(8, i + 4, 1, LMBtext[i], &Green);
  }

  if ( LMRXfreq[0] == LMRXfreq[6 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 5, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[7 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 6, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[8 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 7, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[9 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 8, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[10 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 9, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[1 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 10, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[2 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 11, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[3 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 12, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[4 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 13, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[5 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 5, 14, 14, 1);
  }

  // SR buttons

  if (strcmp(LMRXmode, "terr") == 0)
  {
    indexoffset = 10;
  }
  else
  {
    indexoffset = 0;
  }

  if (strcmp(RXmod, "DVB-S") == 0)  // DVB-S/S2
  {
    for(i = 1; i <= 10; i = i + 1)
    {
      snprintf(LMBStext, 21, "SR^%d", LMRXsr[i + indexoffset]);

      if (i == 5)                                  // keyboard
      {
       snprintf(LMBStext, 21, "SR %d^Keyboard", LMRXsr[i + indexoffset]);
      }

      AmendButtonStatus(8, 14 + i, 0, LMBStext, &Blue);
      AmendButtonStatus(8, 14 + i, 1, LMBStext, &Green);
    }
  }
  else                             // DVB-T
  {
    for(i = 1; i <= 10; i = i + 1)
    {
      snprintf(LMBStext, 21, "Bandwidth^%d kHz", LMRXsr[i + indexoffset]);

      if (i == 5)                                  // keyboard
      {
       snprintf(LMBStext, 21, "BW %d^Keyboard", LMRXsr[i + indexoffset]);
      }

      AmendButtonStatus(8, 14 + i, 0, LMBStext, &Blue);
      AmendButtonStatus(8, 14 + i, 1, LMBStext, &Green);
    }
  }

  if ( LMRXsr[0] == LMRXsr[1 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 15, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[2 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 16, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[3 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 17, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[4 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 18, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[5 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 19, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[6 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 20, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[7 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 21, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[8 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 22, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[9 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 23, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[10 + indexoffset] )
  {
    SelectInGroupOnMenu(8, 15, 24, 24, 1);
  }

  if (strcmp(LMRXmode, "sat") == 0)
  {
    strcpy(LMBStext, "QO-100 (");
    strcat(LMBStext, LMRXinput);
    strcat(LMBStext, ")^ ");
    AmendButtonStatus(8, 25, 0, LMBStext, &Blue);
  }
  else
  {
    strcpy(LMBStext, " ^Terrestrial (");
    strcat(LMBStext, LMRXinput);
    strcat(LMBStext, ")");
    AmendButtonStatus(8, 25, 0, LMBStext, &Blue);
  }
}


void Define_Menu9()
{
  strcpy(MenuTitle[9], "Transmit Control Menu (9)");

  AddButtonStatus(9, 4, "Stop^Transmit", &DBlue);

  AddButtonStatus(9, 15, " Freq ^437 MHz", &Blue);
  AddButtonStatus(9, 15, " Freq ^437 MHz", &Green);
  AddButtonStatus(9, 15, " Freq ^437 MHz", &Grey);

  AddButtonStatus(9, 19, "Lime Gain^88",&Blue);
  AddButtonStatus(9, 19, "Lime Gain^88",&Green);
  AddButtonStatus(9, 19, "Lime Gain^88",&Grey);

  AddButtonStatus(9, 20, "DVB-S^QPSK", &Blue);
  AddButtonStatus(9, 20, "DVB-S^QPSK", &Green);
  AddButtonStatus(9, 20, "DVB-S^QPSK", &Grey);

  AddButtonStatus(9, 23, "Format^16:9", &Blue);
  AddButtonStatus(9, 23, "Format^16:9", &Green);

  AddButtonStatus(9, 24, "Source^Pi Cam", &Blue);
  AddButtonStatus(9, 24, "Source^Pi Cam", &Green);

  AddButtonStatus(9, 25, "Transmit^Selected",&Red);
  AddButtonStatus(9, 25, "Transmit^Off",&Blue);

  AddButtonStatus(9, 26, "RF On",&Red);
  AddButtonStatus(9, 26, "RF Off",&Blue);

  AddButtonStatus(9, 27, "PTT On",&Red);
  AddButtonStatus(9, 27, "PTT Off",&Blue);

  AddButtonStatus(9, 28, "Show^Video",&Blue);
}


void Highlight_Menu9()
{
  char freqLabel[63];
  char limegainLabel[63];

  // Display frequency on button 15
  snprintf(freqLabel, 60, "Freq^%.2f", config.freqoutput);
  AmendButtonStatus(9, 15, 0, freqLabel, &Blue);
  AmendButtonStatus(9, 15, 1, freqLabel, &Green);
  AmendButtonStatus(9, 15, 2, freqLabel, &Grey);
  SetButtonStatus(9, 15, 2);

  snprintf(limegainLabel, 60, "Lime Gain^%d", config.limegain);
  AmendButtonStatus(9, 19, 0, limegainLabel, &Blue);
  AmendButtonStatus(9, 19, 1, limegainLabel, &Green);
  AmendButtonStatus(9, 19, 2, limegainLabel, &Grey);
  SetButtonStatus(9, 19, 2);

  // Display Correct Encoding on Button 21
  if (strcmp(config.encoding, "IPTS in") == 0)
  {
    AmendButtonStatus(9, 21, 0, "Encoding^IPTS in", &Blue);
    AmendButtonStatus(9, 21, 1, "Encoding^IPTS in", &Green);
    AmendButtonStatus(9, 21, 2, "Encoding^IPTS in", &Grey);
  }
  if (strcmp(config.encoding, "IPTS in H264") == 0)
  {
    AmendButtonStatus(9, 21, 0, "Encoding^IPTS in H264", &Blue);
    AmendButtonStatus(9, 21, 1, "Encoding^IPTS in H264", &Green);
    AmendButtonStatus(9, 21, 2, "Encoding^IPTS in H264", &Grey);
  }
  if (strcmp(config.encoding, "IPTS in H265") == 0)
  {
    AmendButtonStatus(9, 21, 0, "Encoding^IPTS in H265", &Blue);
    AmendButtonStatus(9, 21, 1, "Encoding^IPTS in H265", &Green);
    AmendButtonStatus(9, 21, 2, "Encoding^IPTS in H265", &Grey);
  }
  if (strcmp(config.encoding, "MPEG-2") == 0)
  {
    AmendButtonStatus(9, 21, 0, "Encoding^MPEG-2", &Blue);
    AmendButtonStatus(9, 21, 1, "Encoding^MPEG-2", &Green);
    AmendButtonStatus(9, 21, 2, "Encoding^MPEG-2", &Grey);
  }
  if (strcmp(config.encoding, "H264") == 0)
  {
    AmendButtonStatus(9, 21, 0, "Encoding^H264", &Blue);
    AmendButtonStatus(9, 21, 1, "Encoding^H264", &Green);
    AmendButtonStatus(9, 21, 2, "Encoding^H264", &Grey);
  }
  if (strcmp(config.encoding, "H265") == 0)
  {
    AmendButtonStatus(9, 21, 0, "Encoding^H265", &Blue);
    AmendButtonStatus(9, 21, 1, "Encoding^H265", &Green);
    AmendButtonStatus(9, 21, 2, "Encoding^H265", &Grey);
  }
  if (strcmp(config.encoding, "H266") == 0)
  {
    AmendButtonStatus(9, 21, 0, "Encoding^H266", &Blue);
    AmendButtonStatus(9, 21, 1, "Encoding^H266", &Green);
    AmendButtonStatus(9, 21, 2, "Encoding^H266", &Grey);
  }
  if (strcmp(config.encoding, "TS File") == 0)
  {
    AmendButtonStatus(9, 21, 0, "Encoding^from TS File", &Blue);
    AmendButtonStatus(9, 21, 1, "Encoding^from TS File", &Green);
    AmendButtonStatus(9, 21, 2, "Encoding^from TS File", &Grey);
  }
  SetButtonStatus(9, 21, 2);

  // Display Correct Output Format on Button 23
  if (strcmp(config.format, "4:3") == 0)
  {
    AmendButtonStatus(9, 23, 0, "Format^4:3", &Blue);
    AmendButtonStatus(9, 23, 1, "Format^4:3", &Green);
    AmendButtonStatus(9, 23, 2, "Format^4:3", &Grey);
  }
  if (strcmp(config.format, "16:9") == 0)
  {
    AmendButtonStatus(9, 23, 0, "Format^16:9", &Blue);
    AmendButtonStatus(9, 23, 1, "Format^16:9", &Green);
    AmendButtonStatus(9, 23, 2, "Format^16:9", &Grey);
  }
  if (strcmp(config.format, "720p") == 0)
  {
    AmendButtonStatus(9, 23, 0, "Format^720p", &Blue);
    AmendButtonStatus(9, 23, 1, "Format^720p", &Green);
    AmendButtonStatus(9, 23, 2, "Format^720p", &Grey);
  }
  if (strcmp(config.format, "1080p") == 0)
  {
    AmendButtonStatus(9, 23, 0, "Format^1080p", &Blue);
    AmendButtonStatus(9, 23, 1, "Format^1080p", &Green);
    AmendButtonStatus(9, 23, 2, "Format^1080p", &Grey);
  }
  SetButtonStatus(9, 23, 2);

  // Display source on button 24
  if (strcmp(config.videosource, "PiCam") == 0)
  {
    AmendButtonStatus(9, 24, 0, "Source^Pi Cam", &Blue);
    AmendButtonStatus(9, 24, 1, "Source^Pi Cam", &Green);
    AmendButtonStatus(9, 24, 2, "Source^Pi Cam", &Grey);
  }
  if (strcmp(config.videosource, "WebCam") == 0)
  {
    AmendButtonStatus(9, 24, 0, "Source^Web Cam", &Blue);
    AmendButtonStatus(9, 24, 1, "Source^Web Cam", &Green);
    AmendButtonStatus(9, 24, 2, "Source^Web Cam", &Grey);
  }
  if (strcmp(config.videosource, "ATEMUSB") == 0)
  {
    AmendButtonStatus(9, 24, 0, "Source^ATEM USB", &Blue);
    AmendButtonStatus(9, 24, 1, "Source^ATEM USB", &Green);
    AmendButtonStatus(9, 24, 2, "Source^ATEM USB", &Grey);
  }
  if (strcmp(config.videosource, "TestCard") == 0)
  {
    AmendButtonStatus(9, 24, 0, "Source^Test Card", &Blue);
    AmendButtonStatus(9, 24, 1, "Source^Test Card", &Green);
    AmendButtonStatus(9, 24, 2, "Source^Test Card", &Grey);
  }
  SetButtonStatus(9, 24, 2);
}


void Define_Menu11()
{
  strcpy(MenuTitle[11], "Modulation Selection Menu (11)");

  AddButtonStatus(11, 15, "DVB-S^QPSK", &Blue);
  AddButtonStatus(11, 15, "DVB-S^QPSK", &Green);

  AddButtonStatus(11, 16, "DVB-S2^QPSK", &Blue);
  AddButtonStatus(11, 16, "DVB-S2^QPSK", &Green);

  AddButtonStatus(11, 17, "DVB-S2^8PSK", &Blue);
  AddButtonStatus(11, 17, "DVB-S2^8PSK", &Green);

  AddButtonStatus(11, 18, "DVB-S2^16APSK", &Blue);
  AddButtonStatus(11, 18, "DVB-S2^16APSK", &Green);

  AddButtonStatus(11, 19, "DVB-S2^32APSK", &Blue);
  AddButtonStatus(11, 19, "DVB-S2^32APSK", &Green);

  AddButtonStatus(11, 10, "DVB-T", &Grey);

  AddButtonStatus(11, 11, "Carrier", &Grey);

  AddButtonStatus(11, 5, "Pilots^Off", &Grey);
  AddButtonStatus(11, 5, "Pilots^On", &Grey);

  AddButtonStatus(11, 6, "Long^Frames", &Grey);
  AddButtonStatus(11, 6, "Short^Frames", &Grey);

  AddButtonStatus(11, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu11()
{
  SelectFromGroupOnMenu5(11, 15, 1, config.modulation, "DVBS", "S2QPSK", "8PSK", "16APSK", "32APSK");
}


void Define_Menu12()
{
  strcpy(MenuTitle[12], "Video Encoding Selection Menu (12)");

  AddButtonStatus(12, 10, "IPTS in", &Blue);
  AddButtonStatus(12, 10, "IPTS in", &Green);

  AddButtonStatus(12, 11, "Raw IPTS in^H264", &Blue);
  AddButtonStatus(12, 11, "Raw IPTS in^H264", &Green);

  AddButtonStatus(12, 12, "Raw IPTS in^H265", &Blue);
  AddButtonStatus(12, 12, "Raw IPTS in^H265", &Green);

  AddButtonStatus(12, 15, "MPEG-2", &Blue);
  AddButtonStatus(12, 15, "MPEG-2", &Green);

  AddButtonStatus(12, 16, "H264", &Blue);
  AddButtonStatus(12, 16, "H264", &Green);

  AddButtonStatus(12, 17, "H265", &Blue);
  AddButtonStatus(12, 17, "H265", &Green);

  //AddButtonStatus(12, 18, "H266", &Blue);
  //AddButtonStatus(12, 18, "H266", &Green);

  AddButtonStatus(12, 19, "TS File", &Blue);
  AddButtonStatus(12, 19, "TS File", &Green);

  AddButtonStatus(12, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu12()
{
  SelectFromGroupOnMenu3(12, 10, 1, config.encoding, "IPTS in", "IPTS in H264", "IPTS in H265");
  SelectFromGroupOnMenu5(12, 15, 1, config.encoding, "MPEG-2", "H264", "H265", "H266", "TS File");
}


void Define_Menu13()
{
  strcpy(MenuTitle[13], "Output Device Selection Menu (13)");

  AddButtonStatus(13, 5, "BATC^Streamer", &Blue);
  AddButtonStatus(13, 5, "BATC^Streamer", &Green);

  AddButtonStatus(13, 6, "IPTS^Out", &Blue);
  AddButtonStatus(13, 6, "IPTS^Out", &Green);

  AddButtonStatus(13, 7, "HDMI^Screen", &Blue);
  AddButtonStatus(13, 7, "HDMI^Screen", &Green);

  AddButtonStatus(13, 10, "Pluto^Raw", &Blue);
  AddButtonStatus(13, 10, "Pluto^Raw", &Green);

  AddButtonStatus(13, 11, "Pluto^F5OEO", &Blue);
  AddButtonStatus(13, 11, "Pluto^F5OEO", &Green);

  AddButtonStatus(13, 12, "DATV Express^16 bit", &Blue);
  AddButtonStatus(13, 12, "DATV Express^16 bit", &Green);

  AddButtonStatus(13, 13, "DATV Express^32 bit", &Blue);
  AddButtonStatus(13, 13, "DATV Express^32 bit", &Green);

  AddButtonStatus(13, 15, "LimeSDR^Mini", &Blue);
  AddButtonStatus(13, 15, "LimeSDR^Mini", &Green);

  AddButtonStatus(13, 16, "LimeSDR^DVB Firmware", &Blue);
  AddButtonStatus(13, 16, "LimeSDR^DVB Firmware", &Green);

  AddButtonStatus(13, 17, "LimeSDR^USB", &Blue);
  AddButtonStatus(13, 17, "LimeSDR^USB", &Green);

  AddButtonStatus(13, 18, "LimeSDR^PCI XTRX", &Blue);
  AddButtonStatus(13, 18, "LimeSDR^PCI XTRX", &Green);

  AddButtonStatus(13, 19, "LimeSDR^Mini NG", &Blue);
  AddButtonStatus(13, 19, "LimeSDR^Mini NG", &Green);

  AddButtonStatus(13, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu13()
{
  SelectFromGroupOnMenu3(13, 5, 1, config.modeoutput, "STREAMER", "IPTSOUT", "HDMI");
  SelectFromGroupOnMenu4(13, 10, 1, config.modeoutput, "PLUTO", "PLUTOF5OEO", "EXPRESS16", "EXPRESS32");
  SelectFromGroupOnMenu5(13, 15, 1, config.modeoutput, "LIMEMINI", "LIMEDVB", "LIMEUSB", "LIMEXTRX", "LIMEMINING");
}

void Define_Menu14()
{
  strcpy(MenuTitle[14], "Video Format Selection Menu (14)");

  AddButtonStatus(14, 15, "4:3^768x576", &Blue);
  AddButtonStatus(14, 15, "4:3^768x576", &Green);

  AddButtonStatus(14, 16, "16:9^1024x576", &Blue);
  AddButtonStatus(14, 16, "16:9^1024x576", &Green);

  AddButtonStatus(14, 17, "720p^1280x720", &Blue);
  AddButtonStatus(14, 17, "720p^1280x720", &Green);

  AddButtonStatus(14, 18, "1080p^1920x1080", &Blue);
  AddButtonStatus(14, 18, "1080p^1920x1080", &Green);

  AddButtonStatus(14, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu14()
{
  SelectFromGroupOnMenu4(14, 15, 1, config.format, "4:3", "16:9", "720p", "1080p");
}

void Define_Menu15()
{
  strcpy(MenuTitle[15], "Video Source Selection Menu (15)");

  AddButtonStatus(15, 10, "Test Card", &Blue);
  AddButtonStatus(15, 10, "Test Card", &Green);

  AddButtonStatus(15, 15, "Pi Camera", &Blue);
  AddButtonStatus(15, 15, "Pi Camera", &Green);

  AddButtonStatus(15, 16, "Web Cam", &Blue);
  AddButtonStatus(15, 16, "Web Cam", &Green);

  AddButtonStatus(15, 17, "ATEM USB", &Blue);
  AddButtonStatus(15, 17, "ATEM USB", &Green);

  AddButtonStatus(15, 18, "CamLink 4K^HDMI", &Blue);
  AddButtonStatus(15, 18, "CamLink 4K^HDMI", &Green);

  AddButtonStatus(15, 19, "EasyCap^Comp Vid", &Blue);
  AddButtonStatus(15, 19, "EasyCap^Comp Vid", &Green);

  AddButtonStatus(15, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu15()
{
  SelectFromGroupOnMenu5(15, 15, 1, config.videosource, "PiCam", "WebCam", "ATEMUSB", "CAMLINK4K", "EasyCap");
  SelectFromGroupOnMenu1(15, 10, 1, config.videosource, "TestCard");
}


void Define_Menu16()
{
  strcpy(MenuTitle[16], "Transmit Frequency Selection Menu (16)");

  AddButtonStatus(16, 20, "71 MHz", &Blue);
  AddButtonStatus(16, 20, "71 MHz", &Green);

  AddButtonStatus(16, 21, "146.5 MHz", &Blue);
  AddButtonStatus(16, 21, "146.5 MHz", &Green);

  AddButtonStatus(16, 22, "437 MHz", &Blue);
  AddButtonStatus(16, 22, "437 MHz", &Green);

  AddButtonStatus(16, 23, "1249 MHz", &Blue);
  AddButtonStatus(16, 23, "1249 MHz", &Green);

  AddButtonStatus(16, 24, "1255 MHz", &Blue);
  AddButtonStatus(16, 24, "1255 MHz", &Green);

  AddButtonStatus(16, 15, "435 MHz", &Blue);
  AddButtonStatus(16, 15, "435 MHz", &Green);

  AddButtonStatus(16, 16, "436 MHz", &Blue);
  AddButtonStatus(16, 16, "436 MHz", &Green);

  AddButtonStatus(16, 17, "436.5 MHz", &Blue);
  AddButtonStatus(16, 17, "436.5 MHz", &Green);

  AddButtonStatus(16, 18, "438 MHz", &Blue);
  AddButtonStatus(16, 18, "438 MHz", &Green);

  AddButtonStatus(16, 19, "2395 MHz", &Blue);
  AddButtonStatus(16, 19, "2395 MHz", &Green);

  AddButtonStatus(16, 10, "10494.75^2405.25", &Blue);
  AddButtonStatus(16, 10, "10494.75^2405.25", &Green);

  AddButtonStatus(16, 11, "10495.25^2405.75", &Blue);
  AddButtonStatus(16, 11, "10495.25^2405.75", &Green);

  AddButtonStatus(16, 12, "10495.75^2406.25", &Blue);
  AddButtonStatus(16, 12, "10495.75^2406.25", &Green);

  AddButtonStatus(16, 13, "10496.25^2406.75", &Blue);
  AddButtonStatus(16, 13, "10496.25^2406.75", &Green);

  AddButtonStatus(16, 14, "10496.75^2407.25", &Blue);
  AddButtonStatus(16, 14, "10496.75^2407.25", &Green);

  AddButtonStatus(16, 5, "10497.25^2407.75", &Blue);
  AddButtonStatus(16, 5, "10497.25^2407.75", &Green);

  AddButtonStatus(16, 6, "10497.75^2408.25", &Blue);
  AddButtonStatus(16, 6, "10497.75^2408.25", &Green);

  AddButtonStatus(16, 7, "10498.25^2408.75", &Blue);
  AddButtonStatus(16, 7, "10498.25^2408.75", &Green);

  AddButtonStatus(16, 8, "10498.75^2409.25", &Blue);
  AddButtonStatus(16, 8, "10498.75^2409.25", &Green);

  AddButtonStatus(16, 9, "10499.25^2409.75", &Blue);
  AddButtonStatus(16, 9, "10499.25^2409.75", &Green);

  AddButtonStatus(16, 0, "425 MHz", &Blue);
  AddButtonStatus(16, 0, "425 MHz", &Green);

  AddButtonStatus(16, 1, "431.5 MHz", &Blue);
  AddButtonStatus(16, 1, "431.5 MHz", &Green);

  AddButtonStatus(16, 2, "1256 MHz", &Blue);
  AddButtonStatus(16, 2, "1256 MHz", &Green);

  AddButtonStatus(16, 3, "Keyboard", &Blue);
  AddButtonStatus(16, 3, "Keyboard", &Green);

  AddButtonStatus(16, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu16()
{
  char freqAsText[63];
  snprintf(freqAsText, 60, "%.2f", config.freqoutput);

  SelectFromGroupOnMenu5(16, 20, 1, freqAsText, "71.00", "146.50", "437.00", "1249.00", "1255.00");
  SelectFromGroupOnMenu5(16, 15, 1, freqAsText, "435.00", "436.00", "436.50", "438.00", "2395.00");
  SelectFromGroupOnMenu5(16, 10, 1, freqAsText, "2405.25", "2405.75", "2406.25", "2406.75", "2407.25");
  SelectFromGroupOnMenu5(16, 5, 1, freqAsText, "2407.75", "2408.25", "2408.75", "2409.25", "2409.75");
  SelectFromGroupOnMenu3(16, 0, 1, freqAsText, "425.00", "431.50", "1256.00");
}

void Define_Menu17()
{
  strcpy(MenuTitle[17], "Transmit Symbol Rate Menu (17)");

  AddButtonStatus(17, 20, "125", &Blue);
  AddButtonStatus(17, 20, "125", &Green);

  AddButtonStatus(17, 21, "250", &Blue);
  AddButtonStatus(17, 21, "250", &Green);

  AddButtonStatus(17, 22, "333", &Blue);
  AddButtonStatus(17, 22, "333", &Green);

  AddButtonStatus(17, 23, "500", &Blue);
  AddButtonStatus(17, 23, "500", &Green);

  AddButtonStatus(17, 24, "1000", &Blue);
  AddButtonStatus(17, 24, "1000", &Green);

  AddButtonStatus(17, 15, "1500", &Blue);
  AddButtonStatus(17, 15, "1500", &Green);

  AddButtonStatus(17, 16, "2000", &Blue);
  AddButtonStatus(17, 16, "2000", &Green);

  AddButtonStatus(17, 17, "3000", &Blue);
  AddButtonStatus(17, 17, "3000", &Green);

  AddButtonStatus(17, 18, "4000", &Blue);
  AddButtonStatus(17, 18, "4000", &Green);

  AddButtonStatus(17, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu17()
{
  char SRAsText[63];
  snprintf(SRAsText, 60, "%d", config.symbolrate);

  SelectFromGroupOnMenu5(17, 20, 1, SRAsText, "125", "250", "333", "500", "1000");
  SelectFromGroupOnMenu4(17, 15, 1, SRAsText, "1500", "2000", "3000", "4000");
}


void Define_Menu18()
{
  strcpy(MenuTitle[18], "Transmit FEC Menu (18)");

  AddButtonStatus(18, 20, "FEC 1/4", &Blue);
  AddButtonStatus(18, 20, "FEC 1/4", &Green);
  AddButtonStatus(18, 20, "FEC 1/4", &Grey);

  AddButtonStatus(18, 21, "FEC 1/3", &Blue);
  AddButtonStatus(18, 21, "FEC 1/3", &Green);
  AddButtonStatus(18, 21, "FEC 1/3", &Grey);

  AddButtonStatus(18, 22, "FEC 1/2", &Blue);
  AddButtonStatus(18, 22, "FEC 1/2", &Green);
  AddButtonStatus(18, 22, "FEC 1/2", &Grey);

  AddButtonStatus(18, 23, "FEC 3/5", &Blue);
  AddButtonStatus(18, 23, "FEC 3/5", &Green);
  AddButtonStatus(18, 23, "FEC 3/5", &Grey);

  AddButtonStatus(18, 24, "FEC 2/3", &Blue);
  AddButtonStatus(18, 24, "FEC 2/3", &Green);
  AddButtonStatus(18, 24, "FEC 2/3", &Grey);

  AddButtonStatus(18, 15, "FEC 3/4", &Blue);
  AddButtonStatus(18, 15, "FEC 3/4", &Green);
  AddButtonStatus(18, 15, "FEC 3/4", &Grey);

  AddButtonStatus(18, 16, "FEC 5/6", &Blue);
  AddButtonStatus(18, 16, "FEC 5/6", &Green);
  AddButtonStatus(18, 16, "FEC 5/6", &Grey);

  AddButtonStatus(18, 17, "FEC 7/8", &Blue);
  AddButtonStatus(18, 17, "FEC 7/8", &Green);
  AddButtonStatus(18, 17, "FEC 7/8", &Grey);

  AddButtonStatus(18, 18, "FEC 8/9", &Blue);
  AddButtonStatus(18, 18, "FEC 8/9", &Green);
  AddButtonStatus(18, 18, "FEC 8/9", &Grey);

  AddButtonStatus(18, 19, "FEC 9/10", &Blue);
  AddButtonStatus(18, 19, "FEC 9/10", &Green);
  AddButtonStatus(18, 19, "FEC 9/10", &Grey);

  AddButtonStatus(18, 4, "Return to^Main Menu", &DBlue);
}

void Highlight_Menu18()
{
  char FECText[63];
  int i;

  for (i = 15; i < 25; i++)
  {
    SetButtonStatus(18, i, 0);   // reset all buttons to blue
  }

  snprintf(FECText, 60, "%d", config.fec);
  SelectFromGroupOnMenu5(18, 20, 1, FECText, "14", "13", "12", "35", "23");
  SelectFromGroupOnMenu5(18, 15, 1, FECText, "34", "56", "78", "89", "910");

  // Now grey out illegal FECs
  if (strcmp(config.modulation, "S2QPSK") == 0)
  {
    SetButtonStatus(18, 17, 2);   // 7/8 grey
  }
  else if (strcmp(config.modulation, "8PSK") == 0)
  {
    SetButtonStatus(18, 20, 2);   // 1/4 grey
    SetButtonStatus(18, 21, 2);   // 1/3 grey
    SetButtonStatus(18, 22, 2);   // 1/2 grey
    SetButtonStatus(18, 17, 2);   // 7/8 grey
  }
  else if (strcmp(config.modulation, "16APSK") == 0)
  {
    SetButtonStatus(18, 20, 2);   // 1/4 grey
    SetButtonStatus(18, 21, 2);   // 1/3 grey
    SetButtonStatus(18, 22, 2);   // 1/2 grey
    SetButtonStatus(18, 23, 2);   // 3/5 grey
    SetButtonStatus(18, 17, 2);   // 7/8 grey
  }
  else if (strcmp(config.modulation, "32APSK") == 0)
  {
    SetButtonStatus(18, 20, 2);   // 1/4 grey
    SetButtonStatus(18, 21, 2);   // 1/3 grey
    SetButtonStatus(18, 22, 2);   // 1/2 grey
    SetButtonStatus(18, 23, 2);   // 3/5 grey
    SetButtonStatus(18, 24, 2);   // 2/3 grey
    SetButtonStatus(18, 17, 2);   // 7/8 grey
  }
  else if ((strcmp(config.modulation, "DVBS") == 0) || (strcmp(config.modulation, "DVBT") == 0))
  {
    SetButtonStatus(18, 20, 2);   // 1/4 grey
    SetButtonStatus(18, 21, 2);   // 1/3 grey
    SetButtonStatus(18, 23, 2);   // 3/5 grey
    SetButtonStatus(18, 18, 2);   // 8/9 grey
    SetButtonStatus(18, 19, 2);   // 9/10 grey
  }
}


void Define_Menu19()
{
  strcpy(MenuTitle[19], "Band Selection Menu (19)");

  AddButtonStatus(19, 20, "29 MHz", &Blue);
  AddButtonStatus(19, 20, "29 MHz", &Green);

  AddButtonStatus(19, 21, "51 MHz", &Blue);
  AddButtonStatus(19, 21, "51 MHz", &Green);

  AddButtonStatus(19, 22, "Direct", &Blue);
  AddButtonStatus(19, 22, "Direct", &Green);

  AddButtonStatus(19, 23, "2300 MHz", &Blue);
  AddButtonStatus(19, 23, "2300 MHz", &Green);

  AddButtonStatus(19, 24, "3400 MHz", &Blue);
  AddButtonStatus(19, 24, "3400 MHz", &Green);

  AddButtonStatus(19, 15, "5760 MHz", &Blue);
  AddButtonStatus(19, 15, "5760 MHz", &Green);

  AddButtonStatus(19, 16, "10 GHz", &Blue);
  AddButtonStatus(19, 15, "10 GHz", &Green);

  AddButtonStatus(19, 17, "24 GHz", &Blue);
  AddButtonStatus(19, 17, "24 GHz", &Green);

  AddButtonStatus(19, 18, "47 GHz", &Blue);
  AddButtonStatus(19, 18, "47 GHz", &Green);

  AddButtonStatus(19, 19, "76 GHz", &Blue);
  AddButtonStatus(19, 19, "76 GHz", &Green);

  AddButtonStatus(19, 10, "122 GHz", &Blue);
  AddButtonStatus(19, 10, "122 GHz", &Green);

  AddButtonStatus(19, 10, "134 GHz", &Blue);
  AddButtonStatus(19, 10, "134 GHz", &Green);

  AddButtonStatus(19, 4, "Return to^Main Menu", &DBlue);
}

void Highlight_Menu19()
{
  //char FECsText[63];
  //snprintf(FECsText, 60, "%d", config.fec);

  //SelectFromGroupOnMenu5(19, 20, 1, FECsText, "125", "250", "333", "500", "1000");
  //SelectFromGroupOnMenu4(19, 15, 1, FECsText, "1500", "2000", "3000", "4000");
}


void Define_Menu20()
{
  int i;
  int j;
  char label[31];

  strcpy(MenuTitle[20], "SDR Gain Menu (20)");

  for (j = 4; j >= 1; j--)
  {
    for (i = 0; i < 5; i++)
    {
      snprintf(label, 21, "-%d dB", (4 - j) * 5 + i);
      AddButtonStatus(20, 5 * j + i, label, &Blue);
      AddButtonStatus(20, 5 * j + i, label, &Green);
      AddButtonStatus(20, 5 * j + i, label, &Grey);
    }
  }

  AddButtonStatus(20, 0, "-20 dB", &Blue);
  AddButtonStatus(20, 0, "-20 dB", &Green);
  AddButtonStatus(20, 0, "-20 dB", &Grey);

  AddButtonStatus(20, 1, "-21 dB", &Blue);
  AddButtonStatus(20, 1, "-21 dB", &Green);
  AddButtonStatus(20, 1, "-21 dB", &Grey);

  AddButtonStatus(20, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu20()
{
  int i;
  int j;
  int button;
  int gainReduction;
  char label[31];

  if ((strcmp(config.modeoutput, "LIMEMINI") == 0) || (strcmp(config.modeoutput, "LIMEXTRX") == 0)
   || (strcmp(config.modeoutput, "LIMEMINING") == 0))
  {
    for (j = 4; j >= 0; j--)   // For each row top to bottom
    {
      for (i = 0; i < 5; i++)  // For each button left to right
      {
        button = 5 * j + i;
        gainReduction = (4 - j) * 5 + i;

        if ((button != 2) && (button != 3) && (button != 4))   // exclude keyboard and return buttons
        {
          if (limeGainSet[gainReduction] == 0)    // Not achievable
          {
            //set bare button to grey
            snprintf(label, 21, "-%d dB^impossible", gainReduction);
            AmendButtonStatus(20, button, 0, label, &Blue);
            AmendButtonStatus(20, button, 1, label, &Green);
            AmendButtonStatus(20, button, 2, label, &Grey);
            SetButtonStatus(20, button, 2);
          }
          else
          {
            snprintf(label, 21, "-%d dB^Lime gain %d", gainReduction, limeGainSet[gainReduction]);
            AmendButtonStatus(20, button, 0, label, &Blue);
            AmendButtonStatus(20, button, 1, label, &Green);
            AmendButtonStatus(20, button, 2, label, &Grey);
            if (config.limegain == limeGainSet[gainReduction])
            {
              SetButtonStatus(20, button, 1);
            }
            else
            {
              SetButtonStatus(20, button, 0);
            }
          }
        }
      }
    }
  }
  // Keyboard button?
}


void Define_Menu21()
{
  strcpy(MenuTitle[21], "Start-up App Menu (21)");

  AddButtonStatus(21, 20, "Boot to^Portsdown", &Blue);
  AddButtonStatus(21, 20, "Boot to^Portsdown", &Green);

  AddButtonStatus(21, 21, "Boot to^BandViewer", &Blue);
  AddButtonStatus(21, 21, "Boot to^BandViewer", &Green);

  AddButtonStatus(21, 22, "Boot to^Test Menu", &Blue);
  AddButtonStatus(21, 22, "Boot to^Test Menu", &Green);

  AddButtonStatus(21, 15, "Boot to^Transmit", &Blue);
  AddButtonStatus(21, 15, "Boot to^Transmit", &Green);

  AddButtonStatus(21, 16, "Boot to^Receive", &Blue);
  AddButtonStatus(21, 16, "Boot to^Receive", &Green);

  AddButtonStatus(21, 17, "Boot to^Cmd Prompt", &Blue);
  AddButtonStatus(21, 17, "Boot to^Cmd Prompt", &Green);

  AddButtonStatus(21, 18, "Boot to^Key Lime Pi", &Blue);
  AddButtonStatus(21, 18, "Boot to^Key Lime Pi", &Green);
  AddButtonStatus(21, 18, "Boot to^Key Lime Pi", &Grey);

  AddButtonStatus(21, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu21()
{
  // Reset all to Blue
  SetButtonStatus(21, 15, 0);
  SetButtonStatus(21, 16, 0);
  SetButtonStatus(21, 17, 0);
  SetButtonStatus(21, 18, 0);
  SetButtonStatus(21, 20, 0);
  SetButtonStatus(21, 21, 0);
  SetButtonStatus(21, 22, 0);

  if (strcmp(StartApp, "TX_boot") == 0)
  {
    SetButtonStatus(21, 15, 1);   // 
  }

  if (strcmp(StartApp, "RX_boot") == 0)
  {
    SetButtonStatus(21, 16, 1);   //
  }

  if (strcmp(StartApp, "Null_boot") == 0)
  {
    SetButtonStatus(21, 17, 1);   //
  }

  if (strcmp(StartApp, "SA_boot") == 0)
  {
    SetButtonStatus(21, 18, 1);   //
  }

  if (strcmp(StartApp, "Portsdown_boot") == 0)
  {
    SetButtonStatus(21, 20, 1);   //
  }

  if (strcmp(StartApp, "Bandview_boot") == 0)
  {
    SetButtonStatus(21, 21, 1);   //
  }

  if (strcmp(StartApp, "TestMenu_boot") == 0)
  {
    SetButtonStatus(21, 22, 1);   //
  }

  if (KeyLimePiEnabled == false)
  {
    SetButtonStatus(21, 18, 2);
  }
}


void Define_Menu22()
{
  strcpy(MenuTitle[22], "Display and Control Menu (22)");

  AddButtonStatus(22, 15, "Web Control^Enabled", &Blue);
  AddButtonStatus(22, 15, "Web Control^Disabled", &Blue);

  AddButtonStatus(22, 16, "HDMI Def'n^720", &Blue);
  AddButtonStatus(22, 16, "HDMI Def'n^1080", &Blue);

  AddButtonStatus(22, 17, "Touchscreen^Not Inverted", &Blue);
  AddButtonStatus(22, 17, "Touchscreen^Inverted", &Blue);

  AddButtonStatus(22, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu22()
{
  if (webcontrol == true)
  {
    SetButtonStatus(22, 15, 0);
  }
  else
  {
    SetButtonStatus(22, 15, 1);
  }

  if (HDMIResolution == 720)
  {
    SetButtonStatus(22, 16, 0);
  }
  else
  {
    SetButtonStatus(22, 16, 1);
  }

  if (TouchInverted == false)
  {
    SetButtonStatus(22, 17, 0);
  }
  else
  {
    SetButtonStatus(22, 17, 1);
  }
}


void Define_Menu23()
{
  strcpy(MenuTitle[23], "Receive Configuration Menu (23)");

  // Bottom Row, Menu 23

  AddButtonStatus(23, 0, "UDP^IP", &Blue);

  AddButtonStatus(23, 1, "UDP^Port", &Blue);

  AddButtonStatus(23, 2, "Set Preset^Freqs & SRs", &Blue);

  AddButtonStatus(23, 3, "Player^ffplay", &Blue);
  AddButtonStatus(23, 3, "Player^VLC", &Blue);

  AddButtonStatus(23, 4, "Return to^RX Menu", &DBlue);

  // 2nd Row, Menu 23

  AddButtonStatus(23, 5, "Sat LNB^Offset", &Blue);

  AddButtonStatus(23, 6, "Autoset^LNB Offset", &Blue);
  AddButtonStatus(23, 6, "Autoset^LNB Offset", &Grey);

  AddButtonStatus(23, 7, "Input^A", &Blue);
  AddButtonStatus(23, 7, "Input^B", &Blue);

  AddButtonStatus(23, 8, "LNB Volts^OFF", &Blue);
  AddButtonStatus(23, 8, "LNB Volts^18 Horiz", &Green);
  AddButtonStatus(23, 8, "LNB Volts^13 Vert", &Green);

  AddButtonStatus(23, 9, "Audio out^RPi Jack", &Blue);
  AddButtonStatus(23, 9, "Audio out^USB Dongle", &Blue);
  AddButtonStatus(23, 9, "Audio out^HDMI", &Grey);

  // 3rd Row, Menu 23

  AddButtonStatus(23, 10, "Tuner^Timeout", &Blue);

  AddButtonStatus(23, 11, "Tuner Scan^Width", &Blue);

  AddButtonStatus(23, 12, "TS Video^Channel", &Blue);

  AddButtonStatus(23, 13, "Modulation^DVB-S/S2", &Blue);
  AddButtonStatus(23, 13, "Modulation^DVB-T", &Grey);

  //AddButtonStatus(23, 14, "Show Touch^Overlay", &Blue);

  AddButtonStatus(23, 25, "QO-100", &Blue);
  AddButtonStatus(23, 25, "Terrestrial^ ", &Blue);
}


void Highlight_Menu23()
{
  char LMBtext[63];

  if (strcmp(player, "ffplay") == 0)
  {
    SetButtonStatus(23, 3, 0);
  }
  if (strcmp(player, "vlc") == 0)
  {
    SetButtonStatus(23, 3, 1);
  }


  if (strcmp(LMRXmode, "sat") == 0)
  {
    strcpy(MenuTitle[23], "Portsdown QO-100 Receiver Configuration (23)"); 
    strcpy(LMBtext, "QO-100^Input ");
    strcat(LMBtext, LMRXinput);
    SetButtonStatus(23, 6, 0);
  }
  else
  {
    strcpy(MenuTitle[23], "Portsdown Terrestrial Receiver Configuration (23)"); 
    strcpy(LMBtext, "Terrestrial^Input ");
    strcat(LMBtext, LMRXinput);
    SetButtonStatus(23, 6, 1);
  }
  AmendButtonStatus(23, 7, 0, LMBtext, &Blue);

  if (strcmp(LMRXvolts, "off") == 0)
  {
    SetButtonStatus(23, 8, 0);
  }
  if (strcmp(LMRXvolts, "h") == 0)
  {
    SetButtonStatus(23, 8, 1);
  }
  if (strcmp(LMRXvolts, "v") == 0)
  {
    SetButtonStatus(23, 8, 2);
  }

  if (strcmp(LMRXaudio, "rpi") == 0)
  {
    SetButtonStatus(23, 9, 0);
  }
  else if (strcmp(LMRXaudio, "usb") == 0)
  {
    SetButtonStatus(23, 9, 1);
  }
  else
  {
    SetButtonStatus(23, 9, 2);
  }

  if (strcmp(RXmod, "DVB-S") == 0)
  {
    SetButtonStatus(23, 13, 0);
  }
  else
  {
    SetButtonStatus(23, 13, 1);
  }

  if (strcmp(LMRXmode, "sat") == 0)
  {
    SetButtonStatus(23, 25, 0);
  }
  else
  {
    SetButtonStatus(23, 25, 1);
  }
}


void Define_Menu24()
{
  strcpy(MenuTitle[24], "Receive Presets Menu (24)");

  AddButtonStatus(24, 4, "Return to^RX Menu", &Blue);

  // 2nd Row, Menu 24.  

  AddButtonStatus(24, 5, " ", &Blue);
  AddButtonStatus(24, 5, " ", &Green);

  AddButtonStatus(24, 6, " ", &Blue);
  AddButtonStatus(24, 6, " ", &Green);

  AddButtonStatus(24, 7, " ", &Blue);
  AddButtonStatus(24, 7, " ", &Green);

  AddButtonStatus(24, 8, " ", &Blue);
  AddButtonStatus(24, 8, " ", &Green);

  AddButtonStatus(24, 9, " ", &Blue);
  AddButtonStatus(24, 9, " ", &Green);

  // 3rd line up Menu 24

  AddButtonStatus(24, 10, " ", &Blue);
  AddButtonStatus(24, 10, " ", &Green);

  AddButtonStatus(24, 11, " ", &Blue);
  AddButtonStatus(24, 11, " ", &Green);

  AddButtonStatus(24, 12, " ", &Blue);
  AddButtonStatus(24, 12, " ", &Green);

  AddButtonStatus(24, 13," ",&Blue);
  AddButtonStatus(24, 13," ",&Green);

  AddButtonStatus(24, 14," ",&Blue);
  AddButtonStatus(24, 14," ",&Green);

  // 4th line up Menu 24

  AddButtonStatus(24, 15, " ", &Blue);
  AddButtonStatus(24, 15, " ", &Green);

  AddButtonStatus(24, 16, " ", &Blue);
  AddButtonStatus(24, 16, " ", &Green);

  AddButtonStatus(24, 17, " ", &Blue);
  AddButtonStatus(24, 17, " ", &Green);

  AddButtonStatus(24, 18, " ", &Blue);
  AddButtonStatus(24, 18, " ", &Green);

  AddButtonStatus(24, 19, " ", &Blue);
  AddButtonStatus(24, 19, " ", &Green);

  // 5th line up Menu 24

  AddButtonStatus(24, 20, " ", &Blue);
  AddButtonStatus(24, 20, " ", &Green);

  AddButtonStatus(24, 21, " ", &Blue);
  AddButtonStatus(24, 21, " ", &Green);

  AddButtonStatus(24, 22, " ", &Blue);
  AddButtonStatus(24, 22, " ", &Green);

  AddButtonStatus(24, 23, " ", &Blue);
  AddButtonStatus(24, 23, " ", &Green);

  AddButtonStatus(24, 24, " ", &Blue);
  AddButtonStatus(24, 24, " ", &Green);

  AddButtonStatus(24, 25, "QO-100", &Blue);
  AddButtonStatus(24, 25, "Terrestrial^ ", &Blue);
}


void Highlight_Menu24()
{
  int indexoffset = 0;
  char LMBtext[11][21];
  char LMBStext[31];
  int i;
  int FreqIndex;
  div_t div_10;
  div_t div_100;
  div_t div_1000;


  // Sort Sat/Terr switching

  if (strcmp(LMRXmode, "terr") == 0)
  {
    indexoffset = 10;
    strcpy(MenuTitle[24], "Terrestrial Receive Presets Menu (24)");
  }
  else
  {
    strcpy(MenuTitle[24], "QO-100 Receive Presets Menu (24)");
  }

  // Freq buttons

  for(i = 1; i <= 10; i = i + 1)
  {
    if (i <= 5)
    {
      FreqIndex = i + 5 + indexoffset;
    }
    else
    {
      FreqIndex = i + indexoffset - 5;    
    }
    div_10 = div(LMRXfreq[FreqIndex], 10);
    div_1000 = div(LMRXfreq[FreqIndex], 1000);

    if(div_10.rem != 0)  // last character not zero, so make answer of form xxx.xxx
    {
      snprintf(LMBtext[i], 15, "%d.%03d", div_1000.quot, div_1000.rem);
    }
    else
    {
      div_100 = div(LMRXfreq[FreqIndex], 100);

      if(div_100.rem != 0)  // last but one character not zero, so make answer of form xxx.xx
      {
        snprintf(LMBtext[i], 15, "%d.%02d", div_1000.quot, div_1000.rem / 10);
      }
      else
      {
        if(div_1000.rem != 0)  // last but two character not zero, so make answer of form xxx.x
        {
          snprintf(LMBtext[i], 15, "%d.%d", div_1000.quot, div_1000.rem / 100);
        }
        else  // integer MHz, so just xxx.0
        {
          snprintf(LMBtext[i], 15, "%d.0", div_1000.quot);
        }
      }
    }
    strcat(LMBtext[i], "^MHz");

    AmendButtonStatus(24, i + 4, 0, LMBtext[i], &Blue);
    AmendButtonStatus(24, i + 4, 1, LMBtext[i], &Green);
  }

  if ( LMRXfreq[0] == LMRXfreq[6 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 5, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[7 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 6, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[8 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 7, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[9 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 8, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[10 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 9, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[1 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 10, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[2 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 11, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[3 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 12, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[4 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 13, 1);
  }
  else if ( LMRXfreq[0] == LMRXfreq[5 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 5, 14, 14, 1);
  }

  for(i = 1; i <= 10; i = i + 1)
  {
    snprintf(LMBStext, 21, "SR^%d", LMRXsr[i + indexoffset]);

    AmendButtonStatus(24, 14 + i, 0, LMBStext, &Blue);
    AmendButtonStatus(24, 14 + i, 1, LMBStext, &Green);
  }

  if ( LMRXsr[0] == LMRXsr[1 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 15, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[2 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 16, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[3 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 17, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[4 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 18, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[5 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 19, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[6 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 20, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[7 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 21, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[8 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 22, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[9 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 23, 1);
  }
  else if ( LMRXsr[0] == LMRXsr[10 + indexoffset] )
  {
    SelectInGroupOnMenu(24, 15, 24, 24, 1);
  }
  if (strcmp(LMRXmode, "sat") == 0)
  {
    SetButtonStatus(24, 25, 0);
  }
  else
  {
    SetButtonStatus(24, 25, 1);
  }
}


void Define_Menu25()
{
  int i;

  strcpy(MenuTitle[25], "Stream Player Menu (25)");

  // Bottom Row, Menu 25  

  AddButtonStatus(25, 0, "Play with^VLC", &Blue);

  AddButtonStatus(25, 1, "Play with^FFPlay", &Blue);

  AddButtonStatus(25, 3, "Amend^Preset", &Blue);
  AddButtonStatus(25, 3, "Amend^Preset", &Red);

  AddButtonStatus(25, 4, "Exit to^Main Menu", &DBlue);

  // Rows 2, 3 and 4, Menu 25

  for (i = 1; i <= 20; i++)
  {
    AddButtonStatus(25, i + 4, " ", &Blue);
    AddButtonStatus(25, i + 4, " ", &Green);
  }
}


void Highlight_Menu25()
{
  int i;

  if (amendStreamPreset == true)
  {
    SetButtonStatus(25, 3, 1);
  }
  else
  {
    SetButtonStatus(25, 3, 0);
  }

  for (i = 1; i <= 20; i++)
  {
    AmendButtonStatus(25, 4 + i, 0, streamLabel[i], &Blue);
    AmendButtonStatus(25, 4 + i, 1, streamLabel[i], &Green);

    if (i == activeStream)
    {
      SetButtonStatus(25, 4 + i, 1);
    }
    else
    {
      SetButtonStatus(25, 4 + i, 0);
    }
  }
}


void Define_Menu26()
{
  int i;

  strcpy(MenuTitle[26], "Stream Output Selection Menu (26)");

  // Bottom Row, Menu 26  

  AddButtonStatus(26, 9, "Amend^Preset", &Blue);
  AddButtonStatus(26, 9, "Amend^Preset", &Red);

  AddButtonStatus(26, 4, "Exit to^Main Menu", &DBlue);

  // Rows 2, 3 and 4, Menu 25

  for (i = 1; i < 9; i++)
  {
    if (i < 5)  // Bottom row
    {
      AddButtonStatus(26, i - 1, " ", &Blue);
      AddButtonStatus(26, i - 1, " ", &Green);
    }
    if (i > 4) // Top row
    {
      AddButtonStatus(26, i, " ", &Blue);
      AddButtonStatus(26, i, " ", &Green);
    }
  }
}


void Highlight_Menu26()
{
  int i;
  char streamname[63];
  char key[63];

  for (i = 1; i < 9; i++)
  {
    SeparateStreamKey(StreamKey[i], streamname, key);

    if (i < 5)  // Bottom row
    {
      AmendButtonStatus(26, i - 1, 0, streamname, &Blue);
      AmendButtonStatus(26, i - 1, 1, streamname, &Green);
      if (strcmp(StreamKey[0], StreamKey[i]) == 0)
      {
        SetButtonStatus(26, i - 1, 1);
      }
      else
      {
        SetButtonStatus(26, i - 1, 0);
      }
    }
    if (i > 4) // Top row
    {
      AmendButtonStatus(26, i, 0, streamname, &Blue);
      AmendButtonStatus(26, i, 1, streamname, &Green);
      if (strcmp(StreamKey[0], StreamKey[i]) == 0)
      {
        SetButtonStatus(26, i, 1);
      }
      else
      {
        SetButtonStatus(26, i, 0);
      }
    }
  }
  if (StreamerStoreTrigger == true)
  {
    SetButtonStatus(26, 9, 1);
  }
  else
  {
    SetButtonStatus(26, 9, 0);
  }
}


void Define_Menu32()                          // Decision 2 Menu
{
  AddButtonStatus(32, 4, " ", &Black);

  // 2nd Row, Menu 32.  

  AddButtonStatus(32, 11, " ", &Blue);
  AddButtonStatus(32, 11, " ", &Green);

  AddButtonStatus(32, 13, " ", &Blue);
  AddButtonStatus(32, 13, " ", &Green);
}


void Highlight_Menu32()
{
//
}


void Define_Menu33()                          // Decision 2 Menu
{
  AddButtonStatus(33, 4, " ", &Black);

  // 2nd Row, Menu 33.  

  AddButtonStatus(33, 11, " ", &Blue);
  AddButtonStatus(33, 11, " ", &Green);

  AddButtonStatus(33, 12, " ", &Blue);
  AddButtonStatus(33, 12, " ", &Green);

  AddButtonStatus(33, 13, " ", &Blue);
  AddButtonStatus(33, 13, " ", &Green);
}


void Highlight_Menu33()
{
//
}


void Define_Menu41()
{

  AddButtonStatus(22, 4, "Return to^Main Menu", &Blue);

  strcpy(MenuTitle[41], " "); 

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
  Define_Menu15();
  Define_Menu16();
  Define_Menu17();
  Define_Menu18();
  Define_Menu19();
  Define_Menu20();
  Define_Menu21();
  Define_Menu22();
  Define_Menu23();
  Define_Menu24();
  Define_Menu25();
  Define_Menu26();
  Define_Menu32();
  Define_Menu33();

  Define_Menu41();
}


/***************************************************************************//**
 * @brief Lets the user edit an alpha-numeric string of up to 55 characters
 *
 * @param RequestText (str) Displayed as user question
 * @param InitText (str) Initial text for editing
 * @param MaxLength (str) Max length of returned string
 * @param KeyboardReturn (str) The returned string
 * @param UpperCase (bool) Set to false for a lower case keyboard
 *
 * @return void
*******************************************************************************/

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
  if (strlen(EditText) > MaxLength)
  {
    strncpy(EditText, &EditText[0], MaxLength);
    EditText[MaxLength] = '\0';
  }

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


/***************************************************************************//**
 * @brief Asks a yes/no type question
 *
 * @param TitleText (str) Title for the page
 * @param QText (str) Question Text
 * @param Button1Text (str) Text for button 1
 * @param Button2Text (str) Text for button 2
 * @param CancelButton (bool) show cancel button if true 
 *
 * @return (int) 1 for button 1, 2 for button 2, 0 for Cancel
*******************************************************************************/
int Decision2(char *TitleText, char *QText, char *Button1Text, char *Button2Text, bool CancelButton)
{
  const font_t *font_ptr = &font_dejavu_sans_22;
  int i;
  int response = - 1;
  int rawX;
  int rawY;

  strcpy(MenuTitle[32], TitleText);
  AmendButtonStatus(32, 11, 0, Button1Text, &Blue);
  AmendButtonStatus(32, 11, 1, Button1Text, &Green);
  SetButtonStatus(32, 11, 0);
  AmendButtonStatus(32, 13, 0, Button2Text, &Blue);
  AmendButtonStatus(32, 13, 1, Button2Text, &Green);
  SetButtonStatus(32, 13, 0);

  if (CancelButton == true)
  {
    AmendButtonStatus(32, 4, 0, "Cancel", &DBlue);
  }
  else
  {
    AmendButtonStatus(32, 4, 0, " ", &Black);
  }

  CurrentMenu = 32;
  redrawMenu();
  
  TextMid(400, 350, QText, font_ptr, 0, 0, 0, 255, 255, 255);
  publish();

  while(response < 0)
  {
    if (getTouchSample(&rawX, &rawY) == 0) continue;

    i = IsMenuButtonPushed();

    switch(i)
    {
      case 4:                                   // cancel
        if (CancelButton == true)
        {
          response = 0;
        }
        break;
      case 11:
        response = 1;
        SetButtonStatus(32, 11, 1);
        redrawButton(32, 11);
        usleep(250000);
        break;
      case 13:
        response = 2;
        SetButtonStatus(32, 13, 1);
        redrawButton(32, 13);
        usleep(250000);
        break;
    }
  }

  CurrentMenu = CallingMenu;

  return response;
}


/***************************************************************************//**
 * @brief Asks a question with 3 answers
 *
 * @param TitleText (str) Title for the page
 * @param QText (str) Question Text
 * @param Button1Text (str) Text for button 1
 * @param Button2Text (str) Text for button 2
 * @param Button3Text (str) Text for button 3
 * @param CancelButton (bool) show cancel button if true 
 *
 * @return (int) 1 for button 1, 2 for button 2, 0 for Cancel
*******************************************************************************/
int Decision3(char *TitleText, char *QText, char *Button1Text, char *Button2Text, char *Button3Text, bool CancelButton)
{
  const font_t *font_ptr = &font_dejavu_sans_22;
  int i;
  int response = - 1;
  int rawX;
  int rawY;

  strcpy(MenuTitle[33], TitleText);
  AmendButtonStatus(33, 11, 0, Button1Text, &Blue);
  AmendButtonStatus(33, 11, 1, Button1Text, &Green);
  SetButtonStatus(33, 11, 0);
  AmendButtonStatus(33, 12, 0, Button2Text, &Blue);
  AmendButtonStatus(33, 12, 1, Button2Text, &Green);
  SetButtonStatus(33, 12, 0);
  AmendButtonStatus(33, 13, 0, Button3Text, &Blue);
  AmendButtonStatus(33, 13, 1, Button3Text, &Green);
  SetButtonStatus(33, 13, 0);

  if (CancelButton == true)
  {
    AmendButtonStatus(33, 4, 0, "Cancel", &DBlue);
  }
  else
  {
    AmendButtonStatus(33, 4, 0, " ", &Black);
  }

  CurrentMenu = 33;
  redrawMenu();
  
  TextMid(400, 350, QText, font_ptr, 0, 0, 0, 255, 255, 255);
  publish();

  while(response < 0)
  {
    if (getTouchSample(&rawX, &rawY) == 0) continue;

    i = IsMenuButtonPushed();

    switch(i)
    {
      case 4:                                   // cancel
        if (CancelButton == true)
        {
          response = 0;
        }
        break;
      case 11:
        response = 1;
        SetButtonStatus(33, 11, 1);
        redrawButton(33, 11);
        usleep(250000);
        break;
      case 12:
        response = 2;
        SetButtonStatus(33, 12, 1);
        redrawButton(33, 12);
        usleep(250000);
        break;
      case 13:
        response = 3;
        SetButtonStatus(33, 13, 1);
        redrawButton(33, 13);
        usleep(250000);
        break;
    }
  }

  CurrentMenu = CallingMenu;

  return response;
}


void selectTestEquip(int Button)   // Test Equipment
{
  switch(Button)
  {
    case 0:                              // KeyLimePi SA
      // Check KLP installed.  If not, install it
      if (file_exist("/home/pi/portsdown/bin/sa_sched") == 1) // File does not exist
      {
        MsgBox4("Please wait", "Installing KeyLimePi", "Applications", " ");
        system("/home/pi/portsdown/scripts/set-up_configs/enable_klp.sh");
        clearScreen(0, 0, 0);
      }
      cleanexit(170);
      break;
    case 10:                             // SigGen
      cleanexit(130);
      break;
    case 11:                             // Frequency Sweeper
      cleanexit(139);
      break;
    case 12:                             // DMM Display
      cleanexit(142);
      break;
    case 13:                             // AD Power Meter
      cleanexit(137);
      break;
    case 14:                             // PicoViewer
      cleanexit(181);
      break;
    case 15:                             // LimeSDR BandViewer
      cleanexit(136);
      break;
    case 16:                             // LimeSDR NG BandViewer
      cleanexit(151);
      break;
    case 17:                             // LimeSDR NF Meter
      cleanexit(138);
      break;
    case 18:                             // LimeSDR NG NF Meter
      cleanexit(152);
      break;
    case 19:                             // LimeSDR Noise Meter
      cleanexit(147);
      break;
    case 20:                             // KeyLimePi BandViewer
      cleanexit(175);
      break;
    case 21:                             // KeyLimePi NF Meter
      cleanexit(138);
      break;
    case 22:                             // KeyLimePi Noise Meter
      cleanexit(147);
      break;
    case 23:                             // KeyLimePi Bolo Power Meter
      cleanexit(174);
      break;
  }
}


void selectModulation(int Button)  // Transmitter Modulation
{
  switch(Button)
  {
    case 15:
      strcpy(config.modulation, "DVBS");
      break;
    case 16:
      strcpy(config.modulation, "S2QPSK");
      break;
    case 17:
      strcpy(config.modulation, "8PSK");
      break;
    case 18:
      strcpy(config.modulation, "16APSK");
      break;
    case 19:
      strcpy(config.modulation, "32APSK");
      break;
  }

  SetConfigParam(PATH_PCONFIG, "modulation", config.modulation);
}


void selectEncoding(int Button)  // Transmitter Encoding
{
  switch(Button)
  {
    case 10:
      strcpy(config.encoding, "IPTS in");
      break;
    case 11:
      strcpy(config.encoding, "IPTS in H264");
      break;
    case 12:
      strcpy(config.encoding, "IPTS in H265");
      break;
    case 15:
      strcpy(config.encoding, "MPEG-2");
      break;
    case 16:
      strcpy(config.encoding, "H264");
      break;
    case 17:
      strcpy(config.encoding, "H265");
      break;
//    case 18:
//      strcpy(config.encoding, "H266");
//      break;
    case 19:
      strcpy(config.encoding, "TS File");
      break;
  }

  SetConfigParam(PATH_PCONFIG, "encoding", config.encoding);
}


void selectOutput(int Button)          // Transmitter output device
{
  switch(Button)
  {
    case 5:
      strcpy(config.modeoutput, "STREAMER");
      break;
    case 6:
      strcpy(config.modeoutput, "IPTSOUT");
      break;
    case 7:
      strcpy(config.modeoutput, "HDMI");
      break;
    case 10:
      strcpy(config.modeoutput, "PLUTO");
      break;
    case 11:
      strcpy(config.modeoutput, "PLUTOF5OEO");
      break;
    case 12:
      strcpy(config.modeoutput, "EXPRESS16");
      break;
    case 13:
      strcpy(config.modeoutput, "EXPRESS32");
      break;
    case 15:
      strcpy(config.modeoutput, "LIMEMINI");
      break;
    case 16:
      strcpy(config.modeoutput, "LIMEDVB");
      break;
    case 17:
      strcpy(config.modeoutput, "LIMEUSB");
      break;
    case 18:
      strcpy(config.modeoutput, "LIMEXTRX");
      break;
    case 19:
      strcpy(config.modeoutput, "LIMEMINING");
      break;
  }
  SetConfigParam(PATH_PCONFIG, "modeoutput", config.modeoutput);
}


void selectFormat(int Button)  // Transmitter image format
{
  switch(Button)
  {
    case 15:
      strcpy(config.format, "4:3");
      break;
    case 16:
      strcpy(config.format, "16:9");
      break;
    case 17:
      strcpy(config.format, "720p");
      break;
    case 18:
      strcpy(config.format, "1080p");
      break;
  }

  SetConfigParam(PATH_PCONFIG, "format", config.format);
}


void selectVideosource(int Button)  // Transmitter Video Source
{
  switch(Button)
  {
    case 10:
      strcpy(config.videosource, "TestCard");
      break;
    case 15:
      strcpy(config.videosource, "PiCam");
      break;
    case 16:
      strcpy(config.videosource, "WebCam");
      break;
    case 17:
      strcpy(config.videosource, "ATEMUSB");
      break;
    case 18:
      strcpy(config.videosource, "CAMLINK4K");
      break;
    case 19:
      strcpy(config.videosource, "EasyCap");
      break;
  }

  SetConfigParam(PATH_PCONFIG, "videosource", config.videosource);
}


void selectFreqoutput(int Button)  // Transmitter output Frequency
{
  char freqoutputText[63];

  switch(Button)
  {
    case 20:
      config.freqoutput = 71.0;
      break;
    case 21:
      config.freqoutput = 146.5;
      break;
    case 22:
      config.freqoutput = 437;
      break;
    case 23:
      config.freqoutput = 1249;
      break;
    case 24:
      config.freqoutput = 1255;
      break;
    case 15:
      config.freqoutput = 435;
      break;
    case 16:
      config.freqoutput = 436;
      break;
    case 17:
      config.freqoutput = 436.5;
      break;
    case 18:
      config.freqoutput = 438;
      break;
    case 19:
      config.freqoutput = 2395;
      break;
    case 10:
      config.freqoutput = 2405.25;
      break;
    case 11:
      config.freqoutput = 2405.75;
      break;
    case 12:
      config.freqoutput = 2406.25;
      break;
    case 13:
      config.freqoutput = 2406.75;
      break;
    case 14:
      config.freqoutput = 2407.25;
      break;
    case 5:
      config.freqoutput = 2407.75;
      break;
    case 6:
      config.freqoutput = 2408.25;
      break;
    case 7:
      config.freqoutput = 2408.75;
      break;
    case 8:
      config.freqoutput = 2409.25;
      break;
    case 9:
      config.freqoutput = 2409.75;
      break;
    case 0:
      config.freqoutput = 425;
      break;
    case 1:
      config.freqoutput = 431.5;
      break;
    case 2:
      config.freqoutput = 1256;
      break;
    case 3:
      // keyboard
      //config.freqoutput = 71.0;
      break;
  }

  snprintf(freqoutputText, 60, "%.2f", config.freqoutput);
  SetConfigParam(PATH_PCONFIG, "freqoutput", freqoutputText);
}


void selectSymbolrate(int Button)  // Transmitter symbol rate
{
  char srText[63];

  switch(Button)
  {
    case 20:
      config.symbolrate = 125;
      break;
    case 21:
      config.symbolrate = 250;
      break;
    case 22:
      config.symbolrate = 333;
      break;
    case 23:
      config.symbolrate = 500;
      break;
    case 24:
      config.symbolrate = 1000;
      break;
    case 15:
      config.symbolrate = 1500;
      break;
    case 16:
      config.symbolrate = 2000;
      break;
    case 17:
      config.symbolrate = 3000;
      break;
    case 18:
      config.symbolrate = 4000;
      break;
//    case 10:
//      config.symbolrate = 1614;
//      break;
//    case 11:
//      config.symbolrate = 4164;
//      break;
  }
  snprintf(srText, 60, "%d", config.symbolrate);
  SetConfigParam(PATH_PCONFIG, "symbolrate", srText);
}


void selectFEC(int Button)  // Transmitter FEC
{
  char fecText[63];

  switch(Button)
  {
    case 20:
      config.fec = 14;
      break;
    case 21:
      config.fec = 13;
      break;
    case 22:
      config.fec = 12;
      break;
    case 23:
      config.fec = 35;
      break;
    case 24:
      config.fec = 23;
      break;
    case 15:
      config.fec = 34;
      break;
    case 16:
      config.fec = 56;
      break;
    case 17:
      config.fec = 78;
      break;
    case 18:
      config.fec = 89;
      break;
    case 10:
      config.fec = 910;
      break;
  }
  snprintf(fecText, 60, "%d", config.fec);
  SetConfigParam(PATH_PCONFIG, "fec", fecText);
}


void selectBand(int Button)  // Transmitter Band
{
  char fecText[63];

  switch(Button)
  {
    case 20:
      config.symbolrate = 125;
      break;
    case 21:
      config.symbolrate = 250;
      break;
    case 22:
      config.symbolrate = 333;
      break;
    case 23:
      config.symbolrate = 500;
      break;
    case 24:
      config.symbolrate = 1000;
      break;
    case 15:
      config.symbolrate = 1500;
      break;
    case 16:
      config.symbolrate = 2000;
      break;
    case 17:
      config.symbolrate = 3000;
      break;
    case 18:
      config.symbolrate = 4000;
      break;
//    case 10:
//      config.symbolrate = 1614;
//      break;
//    case 11:
//      config.symbolrate = 4164;
//      break;
  }
  snprintf(fecText, 60, "%d", config.fec);
  SetConfigParam(PATH_PCONFIG, "fec", fecText);
}

void selectGain(int Button)  // SDR Gain
{
  char gainText[63];
  // if (Lime)
  {
    switch(Button)
    {
      case 20:
      case 21:
      case 22:
      case 23:
      case 24:
        config.limegain = limeGainSet[Button - 20];
        break;
      case 15:
      case 16:
      case 17:
      case 18:
      case 19:
        config.limegain = limeGainSet[Button - 10];
        break;
      // case 10:  Not possible
      case 11:
      case 12:
      case 13:
      case 14:
        config.limegain = limeGainSet[Button];
        break;
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
        config.limegain = limeGainSet[Button + 10];
        break;
      case 0:
      case 1:
        config.limegain = limeGainSet[Button + 20];
        break;
    }
    // Keyboard?
  snprintf(gainText, 60, "%d", config.limegain);
  SetConfigParam(PATH_PCONFIG, "limegain", gainText);
  }
//  else if   //Pluto
//  {

//  }
//  else if   //DATV Express
//  {

//  }

}


void SelectStartApp(int Button)
{
  switch(Button)
  {
  case 15:                          
    SetConfigParam(PATH_SCONFIG, "startup", "TX_boot");
    strcpy(StartApp, "TX_boot");
    break;
  case 16:                          
    SetConfigParam(PATH_SCONFIG, "startup", "RX_boot");
    strcpy(StartApp, "RX_boot");
    break;
  case 17:                          
    SetConfigParam(PATH_SCONFIG, "startup", "Null_boot");
    strcpy(StartApp, "Null_boot");
    break;
  case 18:                          
    SetConfigParam(PATH_SCONFIG, "startup", "SA_boot");
    strcpy(StartApp, "SA_boot");
    break;
  case 20:                          
    SetConfigParam(PATH_SCONFIG, "startup", "Portsdown_boot");
    strcpy(StartApp, "Portsdown_boot");
    break;
  case 21:                          
    SetConfigParam(PATH_SCONFIG, "startup", "Bandview_boot");
    strcpy(StartApp, "Bandview_boot");
    break;
  case 22:                          
    SetConfigParam(PATH_SCONFIG, "startup", "TestMenu_boot");
    strcpy(StartApp, "TestMenu_boot");
    break;
   default:
    break;
  }
}


void togglewebcontrol()
{
  if (webcontrol == false)
  {
    SetConfigParam(PATH_SCONFIG, "webcontrol", "enabled");
    webcontrol = true;
    //if (webclicklistenerrunning == false)
    //{
    //  printf("Creating thread as webclick listener is not running\n");
    //  pthread_create (&thwebclick, NULL, &WebClickListener, NULL);
    //  printf("Created webclick listener thread\n");
    //}
  }
  else
  {
    system("cp /home/pi/portsdown/images/web_not_enabled.png /home/pi/tmp/screen.png");
    SetConfigParam(PATH_SCONFIG, "webcontrol", "disabled");
    webcontrol = false;
  }
}


void togglehdmiresolution()
{
  int q;
  // Check user intentions
  q = Decision2("Change HDMI Resolution", "This will cause an immediate reboot",
                 "Immediate^Reboot", "Cancel", false);

  if (q == 1)
  {
    if (HDMIResolution == 720)
    {
      SetConfigParam(PATH_SCONFIG, "hdmiresolution", "1080");
      HDMIResolution = 1080;
    }
    else
    {
      SetConfigParam(PATH_SCONFIG, "hdmiresolution", "720");
      HDMIResolution = 720;
    }

    redrawMenu();                // Show
    system("/home/pi/portsdown/scripts/display_check_on_boot.sh");
    usleep(500000);
    system("sudo reboot now");
  }
}


void togglescreenorientation()
{
  int q;
  // Check user intentions
  q = Decision2("Change Screen Orientation", "This will cause an immediate reboot",
                 "Immediate^Reboot", "Cancel", false);

  if (q == 1)
  {
    if (TouchInverted == false)
    {
      SetConfigParam(PATH_SCONFIG, "touch", "inverted");
      TouchInverted = true;
    }
    else
    {
      SetConfigParam(PATH_SCONFIG, "touch", "normal");
      TouchInverted = false;
    }

    redrawMenu();                // Show 
    system("/home/pi/portsdown/scripts/display_check_on_boot.sh");
    usleep(500000);
    system("sudo reboot now");
  }
}


void SelectLMFREQ(int NoButton)  // LongMynd Frequency
{
  char Value[255];

  NoButton = NoButton - 9; // top row 1 - 5 bottom -4 - 0

  if (NoButton < 1)
  {
    NoButton = NoButton + 10;
  }
  // Buttons 1 - 10

  if (strcmp(LMRXmode, "terr") == 0)
  {
    NoButton = NoButton + 10;
    LMRXfreq[0] = LMRXfreq[NoButton];
    snprintf(Value, 25, "%d", LMRXfreq[0]);
    SetConfigParam(PATH_LMCONFIG, "freq1", Value);
  }
  else // Sat
  {
    LMRXfreq[0] = LMRXfreq[NoButton];
    snprintf(Value, 25, "%d", LMRXfreq[0]);
    SetConfigParam(PATH_LMCONFIG, "freq0", Value);
  }
}


void SelectLMSR(int NoButton)  // LongMynd Symbol Rate
{
  char Value[255];

  NoButton = NoButton - 14;  // Translate to 1 - 10

  if (strcmp(LMRXmode, "terr") == 0)
  {
    NoButton = NoButton + 10;
    LMRXsr[0] = LMRXsr[NoButton];
    snprintf(Value, 15, "%d", LMRXsr[0]);
    SetConfigParam(PATH_LMCONFIG, "sr1", Value);
  }
  else // Sat
  {
    LMRXsr[0] = LMRXsr[NoButton];
    snprintf(Value, 15, "%d", LMRXsr[0]);
    SetConfigParam(PATH_LMCONFIG, "sr0", Value);
  }
}


void toggleLMRXmode()
{
  char Value[255];

  if (strcmp(LMRXmode, "sat") == 0)
  {
    strcpy(LMRXmode, "terr");

    // Read in latest values for use
    GetConfigParam(PATH_LMCONFIG, "freq1", Value);
    LMRXfreq[0] = atoi(Value);
    GetConfigParam(PATH_LMCONFIG, "sr1", Value);
    LMRXsr[0] = atoi(Value);
    GetConfigParam(PATH_LMCONFIG, "input1", Value);
    strcpy(LMRXinput, Value);
  }
  else
  {
    strcpy(LMRXmode, "sat");

    // Read in latest values for use
    GetConfigParam(PATH_LMCONFIG, "freq0", Value);
    LMRXfreq[0] = atoi(Value);
    GetConfigParam(PATH_LMCONFIG, "sr0", Value);
    LMRXsr[0] = atoi(Value);
    GetConfigParam(PATH_LMCONFIG, "input", Value);
    strcpy(LMRXinput, Value);
  }
  SetConfigParam(PATH_LMCONFIG, "mode", LMRXmode);
}


void ChangeLMRXIP()
{
  char RequestText[63];
  char InitText[63];
  char KeyboardReturn[63];
  bool IsValid = false;
  char LMRXIPCopy[31];

  while (IsValid == false)
  {
    strcpy(RequestText, "Enter the new UDP IP Destination for the RX TS");
    strcpyn(InitText, LMRXudpip, 17);
    Keyboard(RequestText, InitText, 17, KeyboardReturn, true);
  
    strcpy(LMRXIPCopy, KeyboardReturn);
    if(is_valid_ip(LMRXIPCopy) == true)
    {
      IsValid = true;
    }
  }
  printf("Receiver UDP IP Destination set to: %s\n", KeyboardReturn);

  // Save IP to Local copy and Config File
  strcpy(LMRXudpip, KeyboardReturn);
  SetConfigParam(PATH_LMCONFIG, "udpip", LMRXudpip);
}


void ChangeLMRXPort()
{
  char RequestText[63];
  char InitText[63];
  char KeyboardReturn[63];
  bool IsValid = false;

  while (IsValid == false)
  {
    strcpy(RequestText, "Enter the new UDP Port Number for the RX TS");
    strcpyn(InitText, LMRXudpport, 10);
    Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
  
    if(strlen(KeyboardReturn) > 0)
    {
      IsValid = true;
    }
  }
  printf("LMRX UDP Port set to: %s\n", KeyboardReturn);

  // Save port to local copy and Config File
  strcpy(LMRXudpport, KeyboardReturn);
  SetConfigParam(PATH_LMCONFIG, "udpport", LMRXudpport);
}


void ChangeLMRXOffset()
{
  char RequestText[63];
  char InitText[63];
  char KeyboardReturn[63];
  bool IsValid = false;

  while (IsValid == false)
  {
    strcpy(RequestText, "Enter the new QO-100 LNB Offset in kHz");
    snprintf(InitText, 10, "%d", LMRXqoffset);
    Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
  
    if((atoi(KeyboardReturn) > 1000000) && (atoi(KeyboardReturn) < 76000000))
    {
      IsValid = true;
    }
  }
  printf("LMRXOffset set to: %s kHz\n", KeyboardReturn);

  // Save offset to Config File
  LMRXqoffset = atoi(KeyboardReturn);
  SetConfigParam(PATH_LMCONFIG, "qoffset", KeyboardReturn);
}


void ChangeLMRXPlayer()
{
  if (strcmp(player, "vlc") == 0)
  {
    strcpy(player, "ffplay");
  }
  else
  {
    strcpy(player, "vlc");
  }
  SetConfigParam(PATH_SCONFIG, "player", player);
}


void ChangeLMTST()
{
  char RequestText[63];
  char InitText[63];
  char KeyboardReturn[63];
  bool IsValid = false;

  if (strcmp(LMRXmode, "sat") == 0)
  {
    strcpy(RequestText, "Enter the new Tuner Timeout for QO-100 in ms");
    snprintf(InitText, 12, "%d", LMRXqtimeout);
  }
  else  //Terrestrial
  {
    strcpy(RequestText, "Enter the new Tuner Timeout for terrestrial in ms");
    snprintf(InitText, 12, "%d", LMRXttimeout);
  }

  while (IsValid == false)
  {
    Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
  
    if (((atoi(KeyboardReturn) >= 500) && (atoi(KeyboardReturn) <= 60000)) || (atoi(KeyboardReturn) == -1))
    {
      IsValid = true;
    }
  }
  printf("Tuner Timeout set to: %s ms\n", KeyboardReturn);

  // Save offset to Config File
  if (strcmp(LMRXmode, "sat") == 0)
  {
    LMRXqtimeout = atoi(KeyboardReturn);
    SetConfigParam(PATH_LMCONFIG, "tstimeout", KeyboardReturn);
  }
  else
  {
    LMRXttimeout = atoi(KeyboardReturn);
    SetConfigParam(PATH_LMCONFIG, "tstimeout1", KeyboardReturn);
  }
}


void ChangeLMSW()
{
  char RequestText[63];
  char InitText[63];
  char KeyboardReturn[63];
  bool IsValid = false;

  if (strcmp(LMRXmode, "sat") == 0)
  {
    snprintf(InitText, 4, "%d", LMRXqscanwidth);
    strcpy(RequestText, "Enter scan half-width as % of SR for QO-100");
  }
  else  //Terrestrial
  {
    snprintf(InitText, 4, "%d", LMRXtscanwidth);
    strcpy(RequestText, "Enter scan half-width as % of SR for terrestrial");
  }

  while (IsValid == false)
  {
    Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
  
    if((atoi(KeyboardReturn) >= 10) && (atoi(KeyboardReturn) <= 500))
    {
      IsValid = true;
    }
  }
  printf("Scan Width set to: %s %% of SR\n", KeyboardReturn);

  // Save scan width to Config File
  if (strcmp(LMRXmode, "sat") == 0)
  {
    LMRXqscanwidth = atoi(KeyboardReturn);
    SetConfigParam(PATH_LMCONFIG, "scanwidth", KeyboardReturn);
  }
  else
  {
    LMRXtscanwidth = atoi(KeyboardReturn);
    SetConfigParam(PATH_LMCONFIG, "scanwidth1", KeyboardReturn);
  }
}


void ChangeLMChan()
{
  char RequestText[63];
  char InitText[63];
  bool IsValid = false;
  char KeyboardReturn[63];
  char IntCheck[63];

  //  Retrieve (1 char) Current Sat or Terr channel
  if (strcmp(LMRXmode, "sat") == 0)
  {
    strcpy(RequestText, "Enter TS Video Channel, 0 for default");
    snprintf(InitText, 4, "%d", LMRXqvidchan);
  }
  else  //Terrestrial
  {
    strcpy(RequestText, "Enter TS Video Channel, 0 for default");
    snprintf(InitText, 4, "%d", LMRXtvidchan);
  }

  while (IsValid == false)
  {
    Keyboard(RequestText, InitText, 10, KeyboardReturn, true);

    if(strcmp(KeyboardReturn, "") == 0)  // Set null to 0
    {
      strcpy(KeyboardReturn, "0");
    }

    snprintf(IntCheck, 9, "%d", atoi(KeyboardReturn));
    if(strcmp(IntCheck, KeyboardReturn) == 0)           // Check answer is an integer
    {
      if((atoi(KeyboardReturn) >= 0) && (atoi(KeyboardReturn) <= 64000))
      {
        IsValid = true;
      }
    }
  }
  printf("Video Channel %s selected\n", KeyboardReturn);

  // Save channel to Config File
  if (strcmp(LMRXmode, "sat") == 0)
  {
    LMRXqvidchan = atoi(KeyboardReturn);
    SetConfigParam(PATH_LMCONFIG, "chan", KeyboardReturn);
  }
  else
  {
    LMRXtvidchan = atoi(KeyboardReturn);
    SetConfigParam(PATH_LMCONFIG, "chan1", KeyboardReturn);
  }
}


void ChangeLMPresetFreq(int NoButton)
{
  char RequestText[63];
  char InitText[63] = " ";
  char PresetNo[3];
  char Param[63];
  div_t div_10;
  div_t div_100;
  div_t div_1000;
  int FreqIndex;
  int CheckValue = 0;
  int Offset_to_Apply = 0;
  char FreqkHz[63];
  char KeyboardReturn[63];

  // Convert button number to frequency array index
  if (CallingMenu == 8)  // Called from receive Menu
  {
    FreqIndex = 10;
  }
  else  // called from LM freq Presets menu 
  {
    if (NoButton < 10)  // second row (5 - 9)
    {
      FreqIndex = NoButton + 1;    // 6 - 10
    }
    else                // third row (10 - 14)
    {
      FreqIndex = NoButton - 9;    // 1 - 5
    }
  }
  if (strcmp(LMRXmode, "terr") == 0) // Add index for second set of freqs
  {
    FreqIndex = FreqIndex + 10;
    strcpy(Param, "tfreq");
  }
  else
  {
    Offset_to_Apply = LMRXqoffset;
    strcpy(Param, "qfreq");
  }

  // Define request string
  strcpy(RequestText, "Enter new receive frequency in MHz");

  // Define initial value and convert to MHz

  if(LMRXfreq[FreqIndex] < 50000)  // below 50 MHz, so set to 146.5
  {
    strcpy(InitText, "146.5");
  }
  else
  {
    div_10 = div(LMRXfreq[FreqIndex], 10);
    div_1000 = div(LMRXfreq[FreqIndex], 1000);

    if(div_10.rem != 0)  // last character not zero, so make answer of form xxx.xxx
    {
      snprintf(InitText, 25, "%d.%03d", div_1000.quot, div_1000.rem);
    }
    else
    {
      div_100 = div(LMRXfreq[FreqIndex], 100);

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

  // Ask for new value
  while ((CheckValue - Offset_to_Apply < 50000) || (CheckValue - Offset_to_Apply > 2600000))
  {
    Keyboard(RequestText, InitText, 10, KeyboardReturn, true);
    CheckValue = (int)((1000 * atof(KeyboardReturn)) + 0.1);
    printf("CheckValue = %d Offset = %d\n", CheckValue, Offset_to_Apply);
  }

  // Write freq to memory
  LMRXfreq[FreqIndex] = CheckValue;

  // Convert to string in kHz
  snprintf(FreqkHz, 25, "%d", CheckValue);

  // Write to Presets File as in-use frequency
  if (strcmp(LMRXmode, "terr") == 0) // Terrestrial
  {
    SetConfigParam(PATH_LMCONFIG, "freq1", FreqkHz);        // Set in-use freq
    FreqIndex = FreqIndex - 10;                             // subtract index for terrestrial freqs
  }
  else                               // Sat
  {
    SetConfigParam(PATH_LMCONFIG, "freq0", FreqkHz);        // Set in-use freq
  }

  // write freq to Stored Presets file
  snprintf(PresetNo, 3, "%d", FreqIndex);
  strcat(Param, PresetNo); 
  SetConfigParam(PATH_LMCONFIG, Param, FreqkHz);
}


void ChangeLMPresetSR(int NoButton)
{
  char RequestText[63];
  char InitText[63];
  char PresetNo[31];
  char Param[7];
  int SRIndex;
  int SRCheck = 0;
  char KeyboardReturn[63];

  // Convert button number to frequency array index

  if (CallingMenu == 8)  // Called from receive Menu
  {
    SRIndex = 5;
  }
  else
  {
    // Correct button numbers to index numbers
    SRIndex = NoButton - 14;
  }

  if (strcmp(LMRXmode, "terr") == 0) // Add index for second set of SRs
  {
    SRIndex = SRIndex + 10;
    strcpy(Param, "tsr");
  }
  else
  {
    strcpy(Param, "qsr");
  }

  snprintf(PresetNo, 14, "%d", SRIndex);

  while ((SRCheck < 30) || (SRCheck > 29999))
  {
    strcpy(RequestText, "Enter new Symbol Rate");

    snprintf(InitText, 14, "%d", LMRXsr[SRIndex]);
    Keyboard(RequestText, InitText, 4, KeyboardReturn, true);
  
    // Check valid value
    SRCheck = atoi(KeyboardReturn);
  }

  // Update stored preset value and in-use value
  LMRXsr[SRIndex] = SRCheck;
  LMRXsr[0] = SRCheck;

  // write SR to Config file for current
  if (strcmp(LMRXmode, "terr") == 0) // terrestrial
  {
    SetConfigParam(PATH_LMCONFIG, "sr1", KeyboardReturn);
    SRIndex = SRIndex - 10;
  }
  else
  {
    SetConfigParam(PATH_LMCONFIG, "sr0", KeyboardReturn);
  }

  // write SR to Presets file for preset
  snprintf(PresetNo, 15, "%d", SRIndex);
  strcat(Param, PresetNo); 
  SetConfigParam(PATH_LMCONFIG, Param, KeyboardReturn);
}


void toggleLMRXinput()
{
  if (strcmp(LMRXinput, "a") == 0)
  {
    strcpy(LMRXinput, "b");
  }
  else
  {
    strcpy(LMRXinput, "a");
  }

  if (strcmp(LMRXmode, "sat") == 0)
  {
    SetConfigParam(PATH_LMCONFIG, "input", LMRXinput);
  }
  else
  {
    SetConfigParam(PATH_LMCONFIG, "input1", LMRXinput);
  }
}


void CycleLNBVolts()
{
  if (strcmp(LMRXvolts, "h") == 0)
  {
    strcpy(LMRXvolts, "v");
  }
  else
  {
    if (strcmp(LMRXvolts, "off") == 0)
    {
      strcpy(LMRXvolts, "h");
    }
    else  // All other cases
    {
      strcpy(LMRXvolts, "off");
    }
  }
  SetConfigParam(PATH_LMCONFIG, "lnbvolts", LMRXvolts);
  strcpy(LMRXvolts, "off");
  GetConfigParam(PATH_LMCONFIG, "lnbvolts", LMRXvolts);
}


void CycleLMRXaudio()
{
  if (strcmp(LMRXaudio, "rpi") == 0)
  {
    strcpy(LMRXaudio, "usb");
  }
  else if (strcmp(LMRXaudio, "usb") == 0)
  {
    strcpy(LMRXaudio, "hdmi");
  }
  else
  {
    strcpy(LMRXaudio, "rpi");
  }
  SetConfigParam(PATH_LMCONFIG, "audio", LMRXaudio);
}


void AdjustVLCVolume(int adjustment)
{
  int VLCVolumePerCent;
  static int premuteVLCVolume;
  static bool muted;
  char VLCVolumeText[63];
  char VolumeMessageText[63];
  char VLCVolumeCommand[255];

  if (adjustment == -512) // toggle mute
  {
    if (muted  == true)
    {
      CurrentVLCVolume = premuteVLCVolume;
      muted = false;
    }
    else
    {
      premuteVLCVolume = CurrentVLCVolume;
      CurrentVLCVolume = 0;
      muted = true;
    }
  }
  else                    // normal up or down
  {
    CurrentVLCVolume = CurrentVLCVolume + adjustment;
    if (CurrentVLCVolume < 0)
    {
      CurrentVLCVolume = 0;
    }
    if (CurrentVLCVolume > 512)
    {
      CurrentVLCVolume = 512;
    }
  }

  snprintf(VLCVolumeText, 62, "%d", CurrentVLCVolume);
  SetConfigParam(PATH_PCONFIG, "vlcvolume", VLCVolumeText);

  snprintf(VLCVolumeCommand, 254, "/home/pi/portsdown/scripts/receive/setvlcvolume.sh %d", CurrentVLCVolume);
  system(VLCVolumeCommand); 

  VLCVolumePerCent = (100 * CurrentVLCVolume) / 512;
  snprintf(VolumeMessageText, 62, "VOLUME %d%%", VLCVolumePerCent);
  // printf("%s\n", VolumeMessageText);
  
  FILE *fw=fopen("/home/pi/tmp/vlc_temp_overlay.txt","w+");
  if(fw!=0)
  {
    fprintf(fw, "%s\n", VolumeMessageText);
  }
  fclose(fw);

  // Copy temp file to file to be read by VLC to prevent file collisions
  system("cp /home/pi/tmp/vlc_temp_overlay.txt /home/pi/tmp/vlc_overlay.txt");
  // Clear the volume caption after 1 second
  system("(sleep 1; echo " " > /home/pi/tmp/vlc_overlay.txt) &");
}





void ChangeStreamPreset(int NoButton)
{
  char KeyboardReturn[31];
  char c;
  char param[31];
  bool valid = false;
  char streamname[31];
  char streamnumber[31];
  int i;

  if(amendStreamPreset == true)
  {
    streamname[0] = '\0';

    // Separate the stream name from the full stream URL
    for (i = 29; i < strlen(streamURL[NoButton - 4]); i++)
    {
      if (i < 58)  // max stream name length 27
      {
        c = streamURL[NoButton - 4][i];
        //printf("character: %c\n", c);
      
        streamname[i - 29] = c;
        streamname[i - 28] = '\0';
      }
    }

    // Ask for the new stream name
    while (valid == false)
    {
      Keyboard("Enter the stream name (lower case)", streamname, 27, KeyboardReturn, false);
      if (strlen(KeyboardReturn) > 3)
      {
        valid = true;
      }
    }

    // Append it to the full url and save
    strcpy(streamURL[NoButton - 4], "rtmp://rtmp.batc.org.uk/live/");
    strcat(streamURL[NoButton - 4], KeyboardReturn);

    snprintf(param, 31, "stream%d", NoButton - 4);
    SetConfigParam(PATH_STREAM_PRESETS, param, streamURL[NoButton - 4]);
    printf ("new stream name = %s\n", streamURL[NoButton - 4]);

    valid = false;

    // Ask for the new stream button label
    while (valid == false)
    {
      Keyboard("Enter the Button Label", streamLabel[NoButton - 4], 27, KeyboardReturn, true);
      if (strlen(KeyboardReturn) > 3)
      {
        valid = true;
      }
    }
    strcpy(streamLabel[NoButton - 4], KeyboardReturn);

    // Save the Label
    snprintf(param, 31, "label%d", NoButton - 4);
    SetConfigParam(PATH_STREAM_PRESETS, param, streamLabel[NoButton - 4]);
    printf ("new Label = %s\n", streamLabel[NoButton - 4]);

    // Select the amended stream
    amendStreamPreset = false;
    activeStream = NoButton - 4;

  }
  else                                // Change the active stream
  {
    activeStream = NoButton - 4;
  }

  snprintf(streamnumber, 15, "%d", activeStream);
  SetConfigParam(PATH_STREAM_PRESETS, "selectedstream", streamnumber);
}


void ToggleAmendStreamPreset()
{
  if (amendStreamPreset == false)
  {
    amendStreamPreset = true;
    activeStream = 0;
  }
  else
  {
    amendStreamPreset = false;
  }
}


void CheckforUpdate()
{
  int choice;
  char CurrentVersion[63] = "unknown";
  char UpdateVersion [63] = "unknown";
  char DevVersion [63] = "unknown";
  int current = 0;
  int update = 0;
  int dev = 0;
  char Banner [255];
  char Button1 [63] = "Update Now";
  char Button2 [63] = "Development^Update";
  char * line = NULL;
  size_t len = 0;
  bool complete = false;
  bool show_dev_menu = false;

  MsgBox4("Checking for updates", " ", " ", " ");

  // Look up current version
  FILE *fp=fopen("/home/pi/portsdown/version_history.txt", "r");
  if(fp != 0)
  {
    while ((getline(&line, &len, fp) != -1) && (complete == false))
    {
      if(strncmp (line, "20", 2) == 0)
      {
        strcpy(CurrentVersion, line);
        CurrentVersion[9] = '\0';
        complete = true;
      }
    }
    fclose(fp);
  }
  current = atoi(CurrentVersion);
  // printf ("Current Version -%s-\n", CurrentVersion);

  // Look up production update version
  system("wget -q https://github.com/BritishAmateurTelevisionClub/portsdown5/raw/refs/heads/main/version_history.txt -O /home/pi/tmp/update_version.txt");
  FILE *fu=fopen("/home/pi/tmp/update_version.txt", "r");
  complete = false;
  if(fu != 0)
  {
    while ((getline(&line, &len, fu) != -1) && (complete == false))
    {
      if(strncmp (line, "20", 2) == 0)
      {
        strcpy(UpdateVersion, line);
        UpdateVersion[9] = '\0';
        complete = true;
      }
    }
    fclose(fu);
  }
  update = atoi(UpdateVersion);
  // printf ("Update Version -%s-\n", UpdateVersion);
  
  // Look up development update version
  system("wget -q https://github.com/davecrump/portsdown5/raw/refs/heads/main/version_history.txt -O /home/pi/tmp/dev_version.txt");
  FILE *fd=fopen("/home/pi/tmp/dev_version.txt", "r");
  complete = false;
  if(fd != 0)
  {
    while ((getline(&line, &len, fd) != -1) && (complete == false))
    {
      if(strncmp (line, "20", 2) == 0)
      {
        strcpy(DevVersion, line);
        DevVersion[9] = '\0';
        complete = true;
      }
    }
    fclose(fd);
  }
  dev = atoi(DevVersion);
  // printf ("Dev Version -%s-\n", DevVersion);

  if (current == update)                      // Up to date
  {
    strcpy(Button1, "Force^Update");
    sprintf(Banner, "Latest version: %s is already in use", CurrentVersion);
  }
  else if ((update == 0) || (dev == 0))       // No internet
  {
    strcpy(Button1, " ");
    strcpy(Button2, " ");
    sprintf(Banner, "Unable to contact GitHub.  Check Internet");
  }
  else if (current > update)                  // Probably Dev Version in use
  {
    strcpy(Button1, "Development^Update");
    strcpy(Button2, "Development^Update");
    sprintf(Banner, "Current version: %s, ahead of production version", CurrentVersion);
  }
  else                                        // Normal update available
  {
    sprintf(Banner, "Current version: %s, Update version: %s", CurrentVersion, UpdateVersion);
  }

  choice = Decision3 ("Software Update Menu", Banner, Button1, Button2, "Don't Update", false);
  switch (choice)
  {
    case 1:
      if (current == update)                      // Up to date, but force update
      {
        system("wget -q https://github.com/BritishAmateurTelevisionClub/portsdown5/raw/main/install_p5.sh -O /home/pi/install_p5.sh");
        system("chmod +x /home/pi/install_p5.sh");
        system("/home/pi/install_p5.sh --update &");
        // printf("/home/pi/install_p5.sh --update &\n");
      }
      else if ((update == 0) || (dev == 0))       // No internet
      {
        CurrentMenu = 3;
        return;
      }
      else if (current > update)                  // Probably Dev Version in use, so show dev menu
      {
        show_dev_menu = true;
      }
      else                                        // Do a Normal update 
      {
        system("wget -q https://github.com/BritishAmateurTelevisionClub/portsdown5/raw/main/install_p5.sh -O /home/pi/install_p5.sh");
        system("chmod +x /home/pi/install_p5.sh");
        system("/home/pi/install_p5.sh --update &");
        // printf("/home/pi/install_p5.sh --update &\n");
      }
      break;
    case 2:
      if (current == update)                      // Up to date
      {
        show_dev_menu = true;
      }
      else if ((update == 0) || (dev == 0))       // No internet
      {
        CurrentMenu = 3;
        return;
      }
      else if (current > update)                  // Probably Dev Version in use, so show dev menu
      {
        show_dev_menu = true;
      }
      else                                        // show dev Menu
      {
        show_dev_menu = true;
      }
      break;
    case 3:
      CurrentMenu = 3;
      return;
      break;
  }

  // If flow gets to here, show_dev_menu should be true
  if (show_dev_menu == false)
  {
    printf("unexpected show_dev_menu == false\n");
    CurrentMenu = 3;
    return;
  }

  // Show dev update menu
  if (current == dev)                      // Up to date
  {
    strcpy(Button1, "Force Dev^Update");
    sprintf(Banner, "Latest Dev version: %s is already in use", DevVersion);
  }
  else if (current > dev)                  // Unusual error
  {
    strcpy(Button1, "Force Dev^Update");
    sprintf(Banner, "Current version: %s, ahead of Dev version", CurrentVersion);
  }
  else                                        // Dev update available
  {
    sprintf(Banner, "Current version: %s, Dev version: %s", CurrentVersion, DevVersion);
  }

  choice = Decision2 ("Development Software Update Menu", Banner, Button1, "Don't Update", false);
  switch (choice)
  {
    case 1:
      system("wget -q https://github.com/davecrump/portsdown5/raw/main/install_p5.sh -O /home/pi/install_p5.sh");
      system("chmod +x /home/pi/install_p5.sh");
      if (current == dev)                      // Up to date with dev, so force dev update
      {
        system("/home/pi/install_p5.sh --update --development &");
        // printf("/home/pi/install_p5.sh --update --development &\n");
      }
      else if (current > dev)                  // Unusual error
      {
        system("/home/pi/install_p5.sh --update --development &");
        // printf("/home/pi/install_p5.sh --update --development &\n");
      }
      else                                        // Dev update available
      {
        system("/home/pi/install_p5.sh --update --development &");
         // printf("/home/pi/install_p5.sh --update --development &\n");
      }
      break;
    case 2:
      CurrentMenu = 3;
      return;
      break;
  }
}


void SelectStreamerAction(int NoButton)
{
  if (StreamerStoreTrigger == false)      // Normal
  {
    SelectStreamer(NoButton);
  }
  else
  {
    AmendStreamerPreset(NoButton);
    StreamerStoreTrigger = false;
  }
}


void AmendStreamerPreset(int NoButton)
{
  int NoPreset;
  char Param[63];
  char Value[255];
  char streamname[63];
  char key[63];
  char Prompt[63];
  char KeyboardReturn[63];

  // Map button numbering
  if(NoButton < 5) // bottom row
  {
    NoPreset = NoButton + 1;
  }
  else  // top row
  {
    NoPreset = NoButton;
  }

  // streamurl is unchanged

  SeparateStreamKey(StreamKey[NoPreset], streamname, key);
  snprintf(Param, 31, "streamkey%d", NoPreset);

  sprintf(Prompt, "Enter the streamname (lower case)");
  Keyboard(Prompt, streamname, 15, KeyboardReturn, false);
  strcpy(streamname, KeyboardReturn);

  sprintf(Prompt, "Enter the stream key (6 characters)");
  Keyboard(Prompt, key, 15, KeyboardReturn, false);
  strcpy(key, KeyboardReturn);
  
  snprintf(Value, 127, "%s-%s", streamname, key);
  SetConfigParam(PATH_STREAM_PRESETS, Param, Value);
  strcpy(StreamKey[NoPreset], Value);

  // Select this new streamer as the in-use streamer
  SetConfigParam(PATH_PCONFIG, "streamkey", StreamKey[NoPreset]);
  strcpy(StreamKey[0], StreamKey[NoPreset]);

  clearScreen(0, 0, 0);
}


void SelectStreamer(int NoButton)
{
  int NoPreset;
  //char Param[255];
  //char Value[255];

  // Map button numbering
  if(NoButton < 4) // bottom row
  {
    NoPreset = NoButton + 1;
  }
  else  // top row
  {
    NoPreset = NoButton;
  }

  // Copy in Streamname-key
  SetConfigParam(PATH_PCONFIG, "streamkey", StreamKey[NoPreset]);
  strcpy(StreamKey[0], StreamKey[NoPreset]);
}


void ToggleAmendStreamerPreset()
{
  if (StreamerStoreTrigger == false)      // Normal
  {
    StreamerStoreTrigger = true;
  }
  else
  {
    StreamerStoreTrigger = false;
  }
}


void InfoScreen()
{
  char IPAddress[18] = "Not connected";
  char IPAddress2[18] = " ";
  char IPAddressW[18] = "Not connected";
  char IPLine[127];

  // Initialise and calculate the text display
  const font_t *font_ptr = &font_dejavu_sans_22;
  int txtht =  font_ptr->ascent;
  int linepitch = (14 * txtht) / 10;
  int linenumber = 1;

 
  GetIPAddr(IPAddress);
  GetIPAddr2(IPAddress2);
  Get_wlan0_IPAddr(IPAddressW);
  strcpy(IPLine, "IP: ");
  strcat(IPLine, IPAddress);
  if (strlen(IPAddress2) > 6)
  {
    strcat(IPLine, "   2nd IP: ");
    strcat(IPLine, IPAddress2);
  }
  strcat(IPLine, "   WLAN: ");
  strcat(IPLine, IPAddressW);

  // Display Text
  clearScreen(0, 0, 0);

  TextMid(wscreen / 2.0, hscreen - linenumber * linepitch, "Portsdown 5 Information Screen", font_ptr, 0, 0, 0, 255, 255, 255);

  linenumber = linenumber + 2;

  //Text(wscreen/40, hscreen - linenumber * linepitch, swversion, font_ptr);

  //Text2(15 * wscreen/25, hscreen - linenumber * linepitch, dateTime, font_ptr);
  linenumber = linenumber + 1;

  Text(wscreen/40, hscreen - linenumber * linepitch, IPLine, font_ptr, 0, 0, 0, 255, 255, 255);
  linenumber = linenumber + 1;

  publish();
  UpdateWeb();

  // Wait for screen touch before exit
  wait_touch();
}

void checkTunerSettings()
{
  int basefreq;

  // First check that an FTDI device or PicoTuner is connected
  if (CheckTuner() != 0)
  {
    MsgBox4("No tuner connected", "Please check that you have either", "a MiniTiouner, PicoTuner or a Knucker connected", "Touch Screen to Continue");
    wait_touch();
  }

  if (strcmp(LMRXmode, "sat") == 0)  // Correct for Sat freqs
  {
    basefreq = LMRXfreq[0] - LMRXqoffset;
  }
  else
  {
    basefreq = LMRXfreq[0];
  }

  if (strcmp(RXmod, "DVB-S") == 0)  // Check DVB-S Settings
  {
    // Frequency
    if ((basefreq < 143500 ) || (basefreq > 2650000))
    {
      MsgBox4("Receive Frequency outside normal range", "of 143.5 MHz to 2650 MHz", "Tuner may not operate", "Touch Screen to Continue");
      wait_touch();
    }
    if ((LMRXsr[0] < 66 ) || (LMRXsr[0] > 8000))
    {
      MsgBox4("Receive SR outside normal range", "of 66 kS to 8 MS", "Tuner may not operate", "Touch Screen to Continue");
      wait_touch();
    }
  }
  else                              // Check DVB-T settings
  {
    // Frequency
    if ((basefreq < 50000 ) || (basefreq > 1000000))
    {
      MsgBox4("Receive Frequency outside normal range", "of 50 MHz to 1000 MHz", "Tuner may not operate", "Touch Screen to Continue");
      wait_touch();
    }
    if ((LMRXsr[0] > 500 ) && (LMRXsr[0] != 1000) && (LMRXsr[0] != 1700) && (LMRXsr[0] != 2000) && (LMRXsr[0] != 4000)
                           && (LMRXsr[0] != 5000) && (LMRXsr[0] != 6000) && (LMRXsr[0] != 7000) && (LMRXsr[0] != 8000))
    {
      MsgBox4("Receive bandwidth outside normal range", "of < 501 kHz or 1, 1.7, 2, 4, 5, 6, 7, or 8 MHz", 
              "Tuner may not operate", "Touch Screen to Continue");
      wait_touch();
    }
  }
}


void SeparateStreamKey(char streamkey[127], char streamname[63], char key[63])
{
  int n;
  char delimiter[1] = "-";
  int AfterDelimiter = 0;
  int keystart;
  int stringkeylength;
  strcpy(streamname, "null");
  strcpy(key, "null");

  stringkeylength = strlen(streamkey);

  for(n = 0; n < stringkeylength ; n = n + 1)  // for each character
  {
    if (AfterDelimiter == 0)                   // if streamname
    {
      streamname[n] = streamkey[n];            // copy character into streamname

      if (n == stringkeylength - 1)            // if no delimiter found
      {
        streamname[n + 1] = '\0';                  // terminate streamname to prevent overflow
      }
    }
    else
    {
      AfterDelimiter = AfterDelimiter + 1;     // if not streamname jump over delimiter
    }
    if (streamkey[n] == delimiter[0])          // if delimiter
    {
      streamname[n] = '\0';                    // end streamname
      AfterDelimiter = 1;                      // set flag
      keystart = n;                            // and note key start point
    }
    if (AfterDelimiter > 1)                    // if key
    {
      key[n - keystart - 1] = streamkey[n];    // copy character into key
    }
    if (n == stringkeylength - 2)              // if end of input string
    {
      key[n - keystart + 1] = '\0';            // end key
    }
  }
}


void waitForScreenAction()
{
  int i;
  bool PresetStoreTrigger = false;
  bool shutdownRequested = false;
  int rawX = 0;
  int rawY = 0;

  // Main loop for the Touchscreen
  for (;;)
  {

    if ((boot_to_tx == false) && (boot_to_rx == false))  // Don't wait for touch if booting to TX or RX
    {
      // Wait here until screen touched
      if (getTouchSample(&rawX, &rawY) == 0) continue;
    }

    // Handle contexts first

    if (strcmp(ScreenState, "TXwithImage") == 0)
    {
      ShowTXVideo(false);
      redrawMenu();
      strcpy(ScreenState, "NormalMenu");
      UpdateWeb();
      continue;  // All reset, and Menu displayed so go back and wait for next touch
     }

    // Now Sort TXwithMenu:
    //if (strcmp(ScreenState, "TXwithMenu") == 0)
    //{
    //  SetButtonStatus(1, 25, 0);
    //  redrawButton(1, 25);
    //  TransmitStop();
    //  strcpy(ScreenState, "NormalMenu");
    //  UpdateWeb();
    //  continue;  // All reset, and Menu displayed so go back and wait for next touch
    //}


    {                                           // Normal context
      i = IsMenuButtonPushed();

      // Deal with boot to tx or boot to rx
      if (boot_to_tx == true)
      {
        CurrentMenu = 1;
        i = 25;
        boot_to_tx = false;
      }
      if (boot_to_rx == true)
      {
        CurrentMenu = 8;
        i = 0;
        boot_to_rx = false;
      }

      if (i == -1)
      {
        printf("Screen touched but not on a button\n");
        continue;  //Pressed, but not on a button so wait for the next touch
      }

      // Now do the reponses for each Menu in turn
      if (CurrentMenu == 1)  // Main Menu
      {
        printf("Button Event %d, Entering Menu 1 Case Statement\n",i);
        CallingMenu = 1;

        // Deal with shutdown request
        if ((i != 0) && (shutdownRequested == true))
        {
          shutdownRequested = false;
          SetButtonStatus(1, 0, 0);
          redrawMenu();
          continue;
        }

        if (((i < 5) || (i > 9)) && (PresetStoreTrigger == true))
        {
          PresetStoreTrigger = false;
          SetButtonStatus(1, 9, 0);
          redrawMenu();
          continue;
        }
        switch (i)
        {
        case 0:                                                      // Shutdown
          if (shutdownRequested == false)
          {
            shutdownRequested = true;
            SetButtonStatus(1, 0, 2);
          }
          else
          {
            system ("sudo shutdown now");
          }
          redrawMenu();
          break;
        case 1:                                                      // BandViewer
          clearScreen(0, 0, 0);
          cleanexit(155);
          break;
        case 2:                                                      // Test Equipment
          printf("MENU 7 \n");                                       // Modulation Menu
          CurrentMenu = 7;
          redrawMenu();
          break;
        case 3:                                                      // Test stub for decision
          int q = Decision3("Menu Title", "To be or not to be? That is the question", "Yes", "Don't^know", "No", true);
          printf("Answer = %d\n", q);
          redrawMenu();
          break;
        case 4:                                                      // Stream Player
          printf("MENU 25 \n");
          CurrentMenu = 25;
          redrawMenu();
          break;
        case 5:
        case 6:
        case 7:
        case 8:
          if (PresetStoreTrigger == false)
          {
            //RecallPreset(i);  // Recall preset
            // and make sure that everything has been refreshed?
          }
          else
          {
            //SavePreset(i);  // Set preset
            PresetStoreTrigger = false;
            SetButtonStatus(1, 9, 0);
          }
          SetButtonStatus(1, 9, 1);
          redrawMenu();
          usleep(500000);
          SetButtonStatus(1, 9, 0); 
          redrawMenu();
          break;
        case 9:                                                      // Store preset
          if (PresetStoreTrigger == false)
          {
            PresetStoreTrigger = true;
            SetButtonStatus(1, 9, 2);
          }
          else
          {
            PresetStoreTrigger = false;
            SetButtonStatus(1, 9, 0);
          }
          redrawMenu();
          break;
        case 15:                                                      // Select Frequency
          printf("MENU 16\n");                                        // Frequency menu
          CurrentMenu = 16;
          redrawMenu();
          break;
        case 16:                                                      // Select SR
          printf("MENU 17\n");                                        // SR menu
          CurrentMenu = 17;
          redrawMenu();
          break;
        case 17:                                                      // Select FEC
          printf("MENU 18\n");                                        // FEC menu
          CurrentMenu = 18;
          redrawMenu();
          break;
        case 18:                                                      // Select Band
          printf("MENU 19\n");                                        // Band menu
          CurrentMenu = 19;
          redrawMenu();
          break;
        case 19:                                                      // Select SDR Gain
          printf("MENU 20\n");                                        // SDR Gain menu
          CurrentMenu = 20;
          redrawMenu();
          break;
        case 20:                                                      // Select Modulation
          printf("MENU 11 \n");                                       // Modulation Menu
          CurrentMenu = 11;
          redrawMenu();
          break;
        case 21:                                                      // Select Encoding
          printf("MENU 12 \n");                                       // Encoding Menu
          CurrentMenu = 12;
          redrawMenu();
          break;
        case 22:                                                      // Select Output device
          printf("MENU 13 \n");                                       // Output Menu
          CurrentMenu = 13;
          redrawMenu();
          break;
        case 23:                                                      // Select Output video format
          printf("MENU 14 \n");                                       // Format Menu
          CurrentMenu = 14;
          redrawMenu();
          break;
        case 24:                                                      // Select Video Source
          printf("MENU 15 \n");                                       // Video Source Menu
          CurrentMenu = 15;
          redrawMenu();
          break;
        case 25:                                                      // Transmit
          TransmitStart();
          SetButtonStatus(9, 25, 2);
          CurrentMenu = 9;
          redrawMenu();
          break;
        case 26:                                                      // Receive Menu
          printf("MENU 8 \n");
          CurrentMenu = 8;
          redrawMenu();
          break;
        case 27:
          printf("MENU 5 \n");         // DATV Tools Menu
          CurrentMenu = 5;
          redrawMenu();
          break;
        case 28:
          printf("MENU 3 \n");         // Config Menu
          CurrentMenu = 3;
          redrawMenu();
          break;
        case 29:
          printf("MENU 4 \n");         // File Menu
          CurrentMenu = 4;
          redrawMenu();
          break;
        }
        continue;
      } 
      if (CurrentMenu == 2)  // Apps Menu
      {
        printf("Button Event %d, Entering Menu 2 Case Statement\n",i);
        CallingMenu = 2;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 3)  // Config Menu
      {
        printf("Button Event %d, Entering Menu 3 Case Statement\n",i);
        CallingMenu = 3;
        switch (i)
        {
        case 0:                        // Check for update
          CheckforUpdate();
          redrawMenu();
          break;
        case 1:                        // System Config Menu
          printf("MENU 6 \n");
          CurrentMenu = 6;
          redrawMenu();
          break;
        case 2:                        // Display/Control Config Menu
          printf("MENU 22 \n");
          CurrentMenu = 22;
          redrawMenu();
          break;
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 20:                        // Set Stream Output Menu
          printf("MENU 26 \n");
          CurrentMenu = 26;
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 4)            // File Menu
      {
        printf("Button Event %d, Entering Menu 4 Case Statement\n",i);
        CallingMenu = 4;
        switch (i)
        {
        case 0:                        // Reboot
          system("sudo reboot now");
          break;
        case 1:                        // Restart Portsdown app
          cleanexit(129);
          break;
        case 2:                        // Show Info Page
          InfoScreen();
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 3:                        // Test Keyboard
          char RequestText[63] = "Test keyboard cursor";
          char InitText[63] = "12345678901234567890";
          char KeyboardReturn[63];

          Keyboard(RequestText, InitText, 63, KeyboardReturn, false);
          printf("\n%s\n\n", KeyboardReturn);
          CurrentMenu = 4;
          redrawMenu();
          break;
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 5)            // DATV Tools Menu
      {
        printf("Button Event %d, Entering Menu 5 Case Statement\n",i);
        CallingMenu = 5;
        switch (i)
        {
        case 5:                        // IPTS Monitor
          //system("sudo reboot now");
          break;
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 6)  // System Config Menu
      {
        printf("Button Event %d, Entering Menu 3 Case Statement\n",i);
        CallingMenu = 3;
        switch (i)
        {
        case 0:                        // Start-up App Menu
          printf("MENU 21 \n");
          CurrentMenu = 21;
          redrawMenu();
          break;
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 7)            // Test Equipment Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 7;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 0:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
          selectTestEquip(i);          // Select test equip
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
        continue;
      }
      if (CurrentMenu == 8)            // Receive Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 8;
        switch (i)
        {
        case 0:                                           // VLC with ffmpeg
        case 1:                                           // OMXPlayer
        case 2:                                           // VLC
        case 3:                                           // UDP Output
        case 4:                                           // Beacon MER
          checkTunerSettings();
          LMRX(i);
          clearScreen(0, 0, 0);
          redrawMenu();
          break;

        case 5:                        // Change Freq
        case 6:
        case 7:
        case 8:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
          SelectLMFREQ(i);
          redrawMenu();
          break;
        case 9:                                           // Change button freq
          ChangeLMPresetFreq(i);
          SelectLMFREQ(i);
          redrawMenu();
          break;
        case 15:                                          // Change SR
        case 16:
        case 17:
        case 18:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
          SelectLMSR(i);
          redrawMenu();
          break;
        case 19:                        // Change SR button value
          ChangeLMPresetSR(i);
          SelectLMSR(i);
          redrawMenu();
          break;
        case 25:                        // Toggle QO-100/Terrestrial
          toggleLMRXmode();
          redrawMenu();
          break;
        case 27:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 28:                        // Receiver Config Menu
          printf("MENU 23 \n");
          CurrentMenu = 23;
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 9)           // Transmitting Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 9;
        switch (i)
        {
        case 4:                        // Stop transmit and back to Main Menu
          TransmitStop();
          strcpy(ScreenState, "NormalMenu");
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 15:                         // Change Frequency
          break;
        case 19:                         // ChangeGain                      
          break;
        case 20:
          //selectModulation(i);         // Select Modulation type
          //redrawMenu();                // Show 
          //usleep(500000);
          //CurrentMenu = 1;
          //redrawMenu();                // and return to menu 1
          break;
        case 23:                        // Change format
          break;
        case 24:                        // Change Source
          break;
        case 26:                        // Toggle RF
          break;
        case 27:                        // Toggle PTT
          break;
        case 28:                        // Show Video
          ShowTXVideo(true);
          strcpy(ScreenState, "TXwithImage");
          break;
        }
        continue;
      }
      if (CurrentMenu == 11)           // Modulation Selection Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 11;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
          selectModulation(i);         // Select Modulation type
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
        continue;
      }
      if (CurrentMenu == 12)           // Encoding Selection Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 12;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 10:
        case 11:
        case 12:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
          selectEncoding(i);            // Select Encoding type
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
        continue;
      }
      if (CurrentMenu == 13)           // Output Device Selection Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 13;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 5:
        case 6:
        case 7:
        case 10:
        case 11:
        case 12:
        case 13:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
          selectOutput(i);             // Select Output Device
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
        continue;
      }
      if (CurrentMenu == 14)           // Video format Selection Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 14;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 15:
        case 16:
        case 17:
        case 18:
          selectFormat(i);             // Select Output format
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
        continue;
      }
      if (CurrentMenu == 15)           // Video Source Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 15;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 10:                       // Test Card
        case 15:                       // Pi Cam
        case 16:                       // Web Cam
        case 17:                       // ATEM USB
        case 18:                       // CAMLINK4K
        case 19:                       // EasyCap
          selectVideosource(i);        // Select Video Source
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
        continue;
      }
      if (CurrentMenu == 16)           // Frequency Selection Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 16;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 0:
        case 1:
        case 2:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
          selectFreqoutput(i);         // Select TX output frequency
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        case 3:                       // Keyboard entered frequency
          // Function here
          break;
        }
        continue;
      }
      if (CurrentMenu == 17)           // SR Selection Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 17;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 15:
        case 16:
        case 17:
        case 18:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
          selectSymbolrate(i);         // Select TX SR
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        case 3:                       // Keyboard entered frequency
          // Function here
          break;
        }
        continue;
      }
      if (CurrentMenu == 18)           // FEC Selection Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 18;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 15:
        case 16:
        case 17:
        case 18:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
          selectFEC(i);                // Select TX FEC
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        case 3:                       // Keyboard entered frequency
          // Function here
          break;
        }
        continue;
      }
      if (CurrentMenu == 19)           // Band Selection Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 19;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 15:
        case 16:
        case 17:
        case 18:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
          selectBand(i);               // Select TX Band
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
        continue;
      }
      if (CurrentMenu == 20)           // SDR Gain Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 20;
        switch (i)
        {
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 0:
        case 1:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
          selectGain(i);               // Select SDR Gain
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
        continue;
      }
      if (CurrentMenu == 21)           // Start-up App Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 21;
        switch (i)
        {
        case 15:                       // Boot to Transmit
        case 16:                       // Boot to Receive
        case 17:                       // Boot to Command Prompt
        case 18:                       // Boot to Key Lime Pi SA
        case 20:                       // Boot to Portsdown
        case 21:                       // Boot to BandViewer
        case 22:                       // Boot to Test Menu
          SelectStartApp(i);           // 
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 22)           // Display and Control Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 22;
        switch (i)
        {
        case 15:                       // Toggle web control
          togglewebcontrol();
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        case 16:                       // Toggle HDMI 720/1080 (reboot)
          togglehdmiresolution();
          redrawMenu();
          break;
        case 17:                       // Toggle screen inverted (reboot)
          togglescreenorientation();
          redrawMenu();
          break;
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 23)           // Receiver Config Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 23;
        switch (i)
        {
        case 0:                                         // Output UDP IP
          ChangeLMRXIP();
          redrawMenu();
          break;
        case 1:                                         // Output UDP port 
          ChangeLMRXPort();
          redrawMenu();
          break;
        case 2:                                         // Adjust Preset Freq and SRs
          printf("MENU 24 \n");
          CurrentMenu = 24;
          redrawMenu();
          break;
        case 3:                                         // Player:  VLC or ffplay 
          ChangeLMRXPlayer();
          redrawMenu();
          break;
        case 4:                                         // Back to Receive Menu
          printf("MENU 8 \n");
          CurrentMenu = 8;
          redrawMenu();
          break;
        case 5:                                         // QO-100 LNB Offset 
          ChangeLMRXOffset();
          redrawMenu();
          break;
        case 6:                                         // Autoset QO-100 LNB Offset 
          if (strcmp(LMRXmode, "sat") == 0)
          {
            checkTunerSettings();
            LMRX(5);
            clearScreen(0, 0, 0);
            redrawMenu();
          }
          break;
        case 7:                                         // Change Tuner input 
          toggleLMRXinput();
          redrawMenu();
          break;
        case 8:                                         // Change LNB Volts
          CycleLNBVolts();
          redrawMenu();
          break;
        case 9:                                         // Change Audio out
          CycleLMRXaudio();
          redrawMenu();
          break;
       case 10:                                         // Tuner Timeout 
          ChangeLMTST();
          redrawMenu();
          break;
        case 11:                                         // Tuner Scan Width 
          ChangeLMSW();
          redrawMenu();
          break;
        case 12:                                         // Tuner TS Video Channel 
          ChangeLMChan();
          redrawMenu();
          break;
        case 25:                        // Toggle QO-100/Terrestrial
          toggleLMRXmode();
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 24)           // Receiver Presets Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 24;
        switch (i)
        {
        case 4:                                          // Back to Receive Menu
          printf("MENU 8 \n");
          CurrentMenu = 8;
          redrawMenu();
          break;
        case 5:                                          // Frequencies 
        case 6:                                          //  
        case 7:                                          //  
        case 8:                                          //  
        case 9:                                          //  
        case 10:                                         //  
        case 11:                                         //  
        case 12:                                         // 
        case 13:                                         // 
        case 14:                                         // 
          ChangeLMPresetFreq(i);
          redrawMenu();
          break;
        case 15:                                         // SRs 
        case 16:                                         //  
        case 17:                                         //  
        case 18:                                         //  
        case 19:                                         //  
        case 20:                                         //  
        case 21:                                         //  
        case 22:                                         // 
        case 23:                                         // 
        case 24:                                         // 
          ChangeLMPresetSR(i);
          redrawMenu();
          break;
        case 25:                        // Toggle QO-100/Terrestrial
          toggleLMRXmode();
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 25)           // Stream Player Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 25;
        switch (i)
        {
        case 0:                                          // Play stream with VLC
          amendStreamPreset = false;
          playStreamVLC();
          redrawMenu();
          break;
        case 1:                                          // Play stream with FFPlay
          amendStreamPreset = false;
          playStreamFFPlay();
          redrawMenu();
          break;
        case 3:                                          // Amend preset
          ToggleAmendStreamPreset();
          redrawMenu();
          break;
        case 4:                                          // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          amendStreamPreset = false;
          redrawMenu();
          break;
        case 5:                                          // Streams
        case 6:                                          //  
        case 7:                                          //  
        case 8:                                          //  
        case 9:                                          //  
        case 10:                                         //  
        case 11:                                         //  
        case 12:                                         // 
        case 13:                                         // 
        case 14:                                         // 
        case 15:                                         // 
        case 16:                                         //  
        case 17:                                         //  
        case 18:                                         //  
        case 19:                                         //  
        case 20:                                         //  
        case 21:                                         //  
        case 22:                                         // 
        case 23:                                         // 
        case 24:                                         // 
          ChangeStreamPreset(i);
          redrawMenu();
          break;
        }
        continue;
      }
      if (CurrentMenu == 26)           // Set Stream Outputs Menu
      {
        printf("Menu %d, Button %d\n", CallingMenu, i);
        CallingMenu = 26;
        switch (i)
        {
        case 4:                                          // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          redrawMenu();
          break;
        case 0:                                          // stream 
        case 1:                                          // stream 
        case 2:                                          // stream 
        case 3:                                          // stream 
        case 5:                                          // stream 
        case 6:                                          // stream
        case 7:                                          // stream
        case 8:                                          // stream
          SelectStreamerAction(i);                       // either select or edit
          redrawMenu();
          break;
        case 9:                                          //  Amend Preset
          ToggleAmendStreamerPreset();
          redrawMenu();
          break;
        }
        continue;
      }
    }
  }
}


static void cleanexit(int exit_code)
{
  //strcpy(ModeInput, "DESKTOP"); // Set input so webcam reset script is not called
  //TransmitStop();
  clearScreen(0, 0, 0);
  // closeScreen();

  printf("Set app_exit true\n");
  app_exit = true;
  usleep(500000);

  printf("Clean Exit with Code %d\n", exit_code);
  char Commnd[255];
  sprintf(Commnd,"stty echo");
  system(Commnd);
  sprintf(Commnd,"reset");
  system(Commnd);
  exit(exit_code);
}


static void terminate(int dummy)
{
  clearScreen(0, 0, 0);
  printf("\nTerminate called\n");

  //printf("Set app_exit true\n");
  app_exit = true;
  usleep(50000);

  //system("reset");                     // Move the cursor to the top of the screen
  //closeScreen();
  exit(1);
}


int main(int argc, char **argv)
{
  int i;
  //int NoDeviceEvent = 0;
  int screenXmax, screenXmin;
  int screenYmax, screenYmin;
  int startupmenu = 1;

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

  // Check startup request
  if (argc > 2 )
  {
	for ( i = 1; i < argc - 1; i += 2 )
    {
      if (strcmp(argv[i], "-b") == 0)
      {
        if (strcmp(argv[i + 1], "tx") == 0)
        {
          boot_to_tx = true;
        }
        else if (strcmp(argv[i + 1], "rx") == 0)
        {
          boot_to_rx = true;
        }
        else
        {
          startupmenu = atoi(argv[i + 1] );
          CurrentMenu = startupmenu;
        }
      }
    }
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
  redrawMenu();
  usleep(1000000);
  redrawMenu();  // Second time over-writes system message

  printf("Waiting for button press on touchscreen\n");
  waitForScreenAction();

  // Flow should never reach here

  printf("Reached end of main()\n");
}

