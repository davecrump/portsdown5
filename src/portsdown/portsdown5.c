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

#define PATH_PCONFIG "/home/pi/portsdown/configs/portsdown_config.txt"
#define PATH_SCONFIG "/home/pi/portsdown/configs/system_config.txt"

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

#define MAX_BUTTON_ON_MENU 30
#define MAX_MENUS 50

int debug_level = 0;                   // 0 minimum, 1 medium, 2 max
int fd = 0;                            // File Descriptor for touchscreen touch messages

char DisplayType[31];
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
} config_t;

static config_t config = 
{
  .modulation = "S2QPSK",
  .encoding = "H264",
  .modeoutput = "LIMEMINI",
  .format = "4:3",
  .videosource = "CARD",
  .freqoutput = 437.0,
  .symbolrate = 333,
  .fec = 23
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
bool touch_inverted = true;            // true if touchscreen is inverted (PCB print inverted, ribbon on left) 

// Mouse control variables
bool mouse_connected = false;          // Set true if mouse detected at startup
bool mouse_active = false;             // set true after first movement of mouse
bool MouseClickForAction = false;      // set true on left click of mouse
int mouse_x;                           // click x 0 - 799 from left
int mouse_y;                           // click y 0 - 479 from top
bool touchscreen_present = true;       // detected on startup; used to control menu availability
bool reboot_required = false;          // used after hdmi display change

// Web Control globals
bool webcontrol = false;               // Enables remote control of touchscreen functions
char ProgramName[255];                 // used to pass rpidatvgui char string to listener
int *web_x_ptr;                        // pointer
int *web_y_ptr;                        // pointer
int web_x;                             // click x 0 - 799 from left
int web_y;                             // click y 0 - 480 from top
bool webclicklistenerrunning = false;  // Used to only start thread if required
char WebClickForAction[7] = "no";      // no/yes

// Portsdown state variables
char ScreenState[63] = "NormalMenu";

bool test20 = false;
bool test21 = false;

// Threads
pthread_t thtouchscreen;               //  listens to the touchscreen   
pthread_t thmouse;                     //  Listens to the mouse

// ******************* Function Prototypes **********************************//

// Read and write the configuration
void GetConfigParam(char *PathConfigFile, char *Param, char *Value);
void SetConfigParam(char *PathConfigFile, char *Param, char *Value);
void CheckConfigFile();
void ReadSavedParams();
int file_exist (char *filename);

// Handle touch and mouse
int openTouchScreen(int NoDevice);
int getTouchScreenDetails(int *screenXmin,int *screenXmax,int *screenYmin,int *screenYmax);
int getTouchSampleThread(int *rawX, int *rawY);
int getTouchSample(int *rawX, int *rawY);
void *WaitTouchscreenEvent(void * arg);
void *WaitMouseEvent(void * arg);
void handle_mouse();
void wait_touch();

// Actually do useful things
void TransmitStop();
void TransmitStart();

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
void Define_Menu4();
void Highlight_Menu4();
void Define_Menu7();
void Highlight_Menu7();
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
void Define_Menus();

// Action Menu Buttons
void selectTestEquip(int Button);
void selectModulation(int NoButton);
void selectEncoding(int Button);
void selectOutput(int Button);
void selectFormat(int Button);

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

  strcpy(response, "-");  // highlight null responses
  GetConfigParam(PATH_PCONFIG, "modulation", response);
  strcpy(config.modulation, response);

  //strcpy(PlotTitle, "-");  // this is the "do not display" response
  //GetConfigParam(PATH_CONFIG, "title", PlotTitle);
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


int getTouchSampleThread(int *rawX, int *rawY)
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

  if (((strcmp(DisplayType, "Element14_7") == 0) || (touchscreen_present == true))
      && (strcmp(DisplayType, "dfrobot5") != 0))   // Browser or Element14_7, but not dfrobot5
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
        if (touch_inverted == true)
        {
          scaledX = 799 - *rawX;
          scaledY = *rawY;
        }
        else
        {
          scaledX = *rawX;
          scaledY = *rawY;  // ////////////////////////////// This needs testing!!
        }
        printf("7 inch Touchscreen Touch Event: rawX = %d, rawY = %d scaledX = %d, scaledY = %d\n", *rawX, *rawY, scaledX, scaledY);
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
        //*rawPressure = ev[i].value;
      }

      if((*rawX != -1) && (*rawY != -1) && (touchfinished == true))  // 1a
      {
        //printf("DFRobot Touch Event: rawX = %d, rawY = %d, rawPressure = %d\n", *rawX, *rawY, *rawPressure);
        printf("DFRobot Touch Event: rawX = %d, rawY = %d, \n", *rawX, *rawY);
        awaitingtouchstart = true;
        touchfinished = false;
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
      *rawY = web_y;
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
      //*rawPressure = 0;
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

  bool left_button_action = false;

  if ((fd = open("/dev/input/event0", O_RDONLY)) < 0)
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
//	ffunc_run(ProgramName);
  }
  webclicklistenerrunning = false;
  printf("Exiting WebClickListener\n");
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
{
  strcpy(ScreenState, "TXwithMenu");
  system("/home/pi/portsdown/scripts/transmit/tx.sh &");
}


// Takes the global scaledX and scaled Y and checks against each defined button on the current menu.
// returns the button number, 0 - 29 or -1 for not pushed

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

  for (menu = 0; menu < MAX_MENUS; menu++)
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
  }
  
  //if ((CurrentMenu != 38) && (CurrentMenu != 41)) // If not yes/no or the keyboard
  //{
  //  clearScreen();
  //  clearScreen();  // Second clear screen sometimes required on return from fbi images
  //}
  // Draw the backgrounds for the smaller menus
  //if ((CurrentMenu >= 11) && (CurrentMenu <= 40))  // 10-button menus
  //{
  //  rectangle(10, 12, wscreen - 18, hscreen * 2 / 6 + 12, 127, 127, 127);
  //}

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
  //UpdateWeb();
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

  AddButtonStatus(1, 18,"Band/Tvtr^70cm",&Blue);
  AddButtonStatus(1, 18,"Band/Tvtr^70cm",&Green);

  AddButtonStatus(1, 19," Lime Gain^88",&Blue);
  AddButtonStatus(1, 19," Lime Gain^88",&Green);

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
  }
  if (strcmp(config.modeoutput, "IPTSOUT") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^IPTS", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^IPTS", &Green);
  }
  if (strcmp(config.modeoutput, "HDMI") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^HDMI", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^HDMI", &Green);
  }
  if (strcmp(config.modeoutput, "PLUTO") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Pluto", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Pluto", &Green);
  }
  if (strcmp(config.modeoutput, "PLUTOF5OEO") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Pluto (F5OEO)", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Pluto (F5OEO)", &Green);
  }
  if (strcmp(config.modeoutput, "EXPRESS16") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Express 16", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Express 16", &Green);
  }
  if (strcmp(config.modeoutput, "EXPRESS32") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Express 32", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Express 32", &Green);
  }
  if (strcmp(config.modeoutput, "LIMEMINI") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime Mini", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime Mini", &Green);
  }
  if (strcmp(config.modeoutput, "LIMEDVB") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime DVB", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime DVB", &Green);
  }
  if (strcmp(config.modeoutput, "LIMEUSB") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime USB", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime USB", &Green);
  }
  if (strcmp(config.modeoutput, "LIMEXTRX") == 0)
  {
    AmendButtonStatus(1, 22, 0, "Output to^Lime XTRX", &Blue);
    AmendButtonStatus(1, 22, 1, "Output to^Lime XTRX", &Green);
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
}


void Define_Menu2()
{
  strcpy(MenuTitle[2], "Portsdown 5 DATV Receive Menu (8)");

  AddButtonStatus(2, 4, "Return to^Main Menu", &Blue);
}


void Highlight_Menu2()
{

}


void Define_Menu4()
{
  strcpy(MenuTitle[4], "Portsdown 5 File Menu (4)");

  AddButtonStatus(4, 0, "Reboot^System", &Blue);

  AddButtonStatus(4, 1, "Restart^Portsdown", &Blue);

  AddButtonStatus(4, 4, "Return to^Main Menu", &Blue);


}


void Highlight_Menu4()
{

}


void Define_Menu7()
{
  strcpy(MenuTitle[7], "Portsdown 5 Test Equipment Menu (7)");

  AddButtonStatus(7, 15, "LimeSDR^BandViewer", &Blue);
  AddButtonStatus(7, 15, "LimeSDR^BandViewer", &Green);

  AddButtonStatus(7, 16, "LimeSDR NG^BandViewer", &Blue);
  AddButtonStatus(7, 16, "LimeSDR NG^BandViewer", &Green);

  //AddButtonStatus(7, 17, "720p^720x1280", &Blue);
  //AddButtonStatus(7, 17, "720p^720x1280", &Green);

  //AddButtonStatus(7, 18, "1080p^1080x1920", &Blue);
  //AddButtonStatus(7, 18, "1080p^1080x1920", &Green);

  AddButtonStatus(7, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu7()
{

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

  AddButtonStatus(13, 7, "HDMI^Source", &Blue);
  AddButtonStatus(13, 7, "HDMI^Source", &Green);

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

  AddButtonStatus(13, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu13()
{
  SelectFromGroupOnMenu3(13, 5, 1, config.modeoutput, "STREAMER", "IPTSOUT", "HDMI");
  SelectFromGroupOnMenu4(13, 10, 1, config.modeoutput, "PLUTO", "PLUTOF5OEO", "EXPRESS16", "EXPRESS32");
  SelectFromGroupOnMenu4(13, 15, 1, config.modeoutput, "LIMEMINI", "LIMEDVB", "LIMEUSB", "LIMEXTRX");
}

void Define_Menu14()
{
  strcpy(MenuTitle[14], "Video Format Selection Menu (14)");

  AddButtonStatus(14, 15, "4:3^576x768", &Blue);
  AddButtonStatus(14, 15, "4:3^576x768", &Green);

  AddButtonStatus(14, 16, "16:9^576x1024", &Blue);
  AddButtonStatus(14, 16, "16:9^576x1024", &Green);

  AddButtonStatus(14, 17, "720p^720x1280", &Blue);
  AddButtonStatus(14, 17, "720p^720x1280", &Green);

  AddButtonStatus(14, 18, "1080p^1080x1920", &Blue);
  AddButtonStatus(14, 18, "1080p^1080x1920", &Green);

  AddButtonStatus(14, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu14()
{
  SelectFromGroupOnMenu4(14, 15, 1, config.format, "4:3", "16:9", "720p", "1080p");
}

void Define_Menu15()
{
  strcpy(MenuTitle[15], "Video Source Selection Menu (15)");

  AddButtonStatus(15, 15, "Pi Camera", &Blue);
  AddButtonStatus(15, 15, "Pi Camera", &Green);

  AddButtonStatus(15, 16, "Web Cam", &Blue);
  AddButtonStatus(15, 16, "Web Cam", &Green);

  AddButtonStatus(15, 17, "ATEM USB", &Blue);
  AddButtonStatus(15, 17, "ATEM USB", &Green);

  //AddButtonStatus(15, 18, "1080p^1080x1920", &Blue);
  //AddButtonStatus(15, 18, "1080p^1080x1920", &Green);

  AddButtonStatus(15, 4, "Return to^Main Menu", &DBlue);
}


void Highlight_Menu15()
{
  SelectFromGroupOnMenu3(15, 15, 1, config.videosource, "PiCam", "WebCam", "ATEMUSB");
}

// PiCam    WebCam ATEMUSB 
// CompVid  HDMI
// TestCard Contest


void Define_Menus()
{
  Define_Menu1();
  Define_Menu2();
  Define_Menu4();
  Define_Menu7();
  Define_Menu11();
  Define_Menu12();
  Define_Menu13();
  Define_Menu14();
  Define_Menu15();
}

void selectTestEquip(int Button)   // Test Equipment
{
  switch(Button)
  {
    case 15:                             // LimeSDR BandViewer
      cleanexit(136);
      break;
    case 16:                             // LimeSDR NG BandViewer
      cleanexit(151);
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
    case 15:
      strcpy(config.videosource, "PiCam");
      break;
    case 16:
      strcpy(config.videosource, "WebCam");
      break;
    case 17:
      strcpy(config.videosource, "ATEMUSB");
      break;
   // case 18:
   //   strcpy(config.format, "1080p");
   //   break;
  }

  SetConfigParam(PATH_PCONFIG, "videosource", config.videosource);
}


void waitForScreenAction()
{
  int i;
  bool PresetStoreTrigger = false;
  bool shutdownRequested = false;
  int rawX = 0;
  int rawY = 0;

  // Wait for a screen touch and act on its position
  // Start the main loop for the Touchscreen
  for (;;)
  {

    //if ((strcmp(ScreenState, "RXwithImage") != 0) && (strcmp(ScreenState, "VideoOut") != 0)
    // && (boot_to_tx == false) && (boot_to_rx == false))  // Don't wait for touch if returning from recieve or booting to TX or rx
    //{
      // Wait here until screen touched
      if (getTouchSample(&rawX, &rawY) == 0) continue;
    //}

    // Handle contexts first

    if (strcmp(ScreenState, "TXwithImage") == 0)
    {
      SetButtonStatus(1, 25, 0);
      redrawMenu();
      TransmitStop();
      strcpy(ScreenState, "NormalMenu");
      continue;  // All reset, and Menu displayed so go back and wait for next touch
     }

    // Now Sort TXwithMenu:
    if (strcmp(ScreenState, "TXwithMenu") == 0)
    {
      SetButtonStatus(1, 25, 0);
      redrawButton(1, 25);
      TransmitStop();
      strcpy(ScreenState, "NormalMenu");
      continue;  // All reset, and Menu displayed so go back and wait for next touch
    }


    {                                           // Normal context
      i = IsMenuButtonPushed();


      // Deal with boot to tx or boot to rx
//      if (boot_to_tx == true)
//      {
//        CurrentMenu = 1;
//        i = 20;
//        boot_to_tx = false;
//      }
//      if (boot_to_rx == true)
//      {
//        CurrentMenu = 8;
//        i = 0;
//        boot_to_rx = false;
//      }

      if (i == -1)
      {
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
          cleanexit(136);
          break;
        case 2:                                                      // Test Equipment
          printf("MENU 7 \n");                                       // Modulation Menu
          CurrentMenu = 7;
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
          //Start_Highlights_Menu1();
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
          if (strcmp(ScreenState, "NormalMenu") == 0)
          {
            TransmitStart();
            SetButtonStatus(1, 25, 2);
            redrawButton(1, 25);
          }
          break;
        case 27:
          printf("MENU 2 \n");         // Tools Menu
          CurrentMenu = 2;
          //Start_Highlights_Menu2();
          redrawMenu();
          break;
        case 29:
          printf("MENU 4 \n");         // File Menu
          CurrentMenu = 4;
          //Start_Highlights_Menu4();
          redrawMenu();
          break;
        }
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
          //Start_Highlights_Menu1();
          redrawMenu();
          break;
        }
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
        case 4:                        // Back to Main Menu
          printf("MENU 1 \n");
          CurrentMenu = 1;
          //Start_Highlights_Menu1();
          redrawMenu();
          break;
        }
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
        case 15:
        case 16:
        //case 17:
        //case 18:
        //case 19:
          selectTestEquip(i);          // Select test equip
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
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
          selectOutput(i);             // Select Output Device
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
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
        case 15:
        case 16:
        case 17:
        //case 18:
          selectVideosource(i);             // Select Video Source
          redrawMenu();                // Show 
          usleep(500000);
          CurrentMenu = 1;
          redrawMenu();                // and return to menu 1
          break;
        }
      }
    }
  }
}


static void cleanexit(int exit_code)
{
  //strcpy(ModeInput, "DESKTOP"); // Set input so webcam reset script is not called
  //TransmitStop();
  //clearScreen(0, 0, 0);
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
  printf("Terminate called\n");

  printf("Set app_exit true\n");
  app_exit = true;
  usleep(500000);

  //clearScreen(0, 0, 0);
  system("reset");                     // Move the cursor to the top of the screen
  //closeScreen();
  exit(1);
}


int main(int argc, char **argv)
{
  int NoDeviceEvent = 0;
  int i;
  int screenXmax, screenXmin;
  int screenYmax, screenYmin;
  char response[63];

  // Catch sigaction and call terminate
  for (i = 0; i < 16; i++)
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = terminate;
    sigaction(i, &sa, NULL);
  }

  CurrentMenu = 1;

  // Sort out display and control.  If touchscreen detected, use that (and set config) with optional web control.
  // If no touchscreen, check for mouse.  If mouse detected, use that and set hdmi720 if hdmi mode not selected
  // Allow optional web control.  If no mouse, set hdmi720 and enable web control.

  // Check the display type in the config file
  strcpy(response, "Element14_7");
  GetConfigParam(PATH_SCONFIG, "display", DisplayType);
  strcpy(DisplayType, response);

  // Check for presence of touchscreen
  for(NoDeviceEvent = 0; NoDeviceEvent < 7; NoDeviceEvent++)
  {
    if (openTouchScreen(NoDeviceEvent) == 1)
    {
      if(getTouchScreenDetails(&screenXmin, &screenXmax, &screenYmin, &screenYmax)==1) break;
    }
  }

  if(NoDeviceEvent != 7)  // Touchscreen detected
  {
    printf("Touchscreen detected\n");
    touchscreen_present = true;

    // Set correct entry in config file if required.  No need to reboot
    if ((strcmp(DisplayType, "Element14_7") != 0) && (strcmp(DisplayType, "dfrobot5") != 0))
    {
      strcpy(DisplayType, "Element14_7");
      SetConfigParam(PATH_SCONFIG, "display", "Element14_7");
    }

    // Create Touchscreen thread
    pthread_create (&thtouchscreen, NULL, &WaitTouchscreenEvent, NULL);
  }
  else // No touchscreen detected, check for mouse
  {
    if (CheckMouse() == 0)
    {
      mouse_connected = true;
      mouse_active = true;
      printf("Mouse Connected\n");

      // If display not previously set to hdmi, correct it
      if ((strcmp(DisplayType, "hdmi") != 0) && (strcmp(DisplayType, "hdmi480") != 0)
        && (strcmp(DisplayType, "hdmi720") != 0) && (strcmp(DisplayType, "hdmi1080") != 0))
      {
        strcpy(DisplayType, "hdmi720");
        SetConfigParam(PATH_SCONFIG, "display", "hdmi720");
        // need a linux command here to edit the cmdline.txt file and a reboot
        //system ("/home/pi/rpidatv/scripts/set_display_config.sh");
        //system ("sudo reboot now");
      }

      // And start the mouse listener thread
      printf("Starting Mouse Thread\n");
      pthread_create (&thmouse, NULL, &WaitMouseEvent, NULL);
    }
    else // No touchscreen or mouse, so check webcontrol is enabled
    {
      printf("Mouse not Connected\n");

      SetConfigParam(PATH_SCONFIG, "webcontrol", "enabled");
      webcontrol = true;

      // If display not previously set to hdmi, correct it
      if ((strcmp(DisplayType, "hdmi") != 0) && (strcmp(DisplayType, "hdmi480") != 0)
        && (strcmp(DisplayType, "hdmi720") != 0) && (strcmp(DisplayType, "hdmi1080") != 0))
      {
        strcpy(DisplayType, "hdmi720");
        SetConfigParam(PATH_SCONFIG, "display", "hdmi720");
        // need a linux command here to edit the cmdline.txt file and a reboot
        //system ("/home/pi/rpidatv/scripts/set_display_config.sh");
        //system ("sudo reboot now");
      }
    }
  }
  printf("Completed Screen and Mouse checks\n");

  //usleep(5000000);                      // Let graphics settle

  initScreen();
  CreateButtons();

  Define_Menus();

  ReadSavedParams();

  clearScreen(255, 255, 255);

  redrawMenu();
  publish();

  printf("Waiting for button press on touchscreen\n");
  waitForScreenAction();

  // Flow should never reach here

  printf("Reached end of main()\n");
}

