
// Actions
//
// Sig Gen Menu is selected
//
// Title button selected on
//
// Title button selected off
//
// Freq Button is selected

// Mod button selected on

// Mod button selected off

// Level up is selected

// Level down is selected

// Load firmware is selected

// On selection of Sig Gen Menu

//  Initially Express_ready is false.
//    check express connected
//    check firmware loaded (load if required)
//    start express server
//    set expressReady true

//  If Title button is selected on
//    Start Expresss with stored parameters
//    button goes red

//  If Title button is selected off
//    Start Expresss with stored parameters
//    button goes red

//  If freq button selected, bring up keybaord, enter freq in MHz
//    Amend frequency and store
//    change frequency if running

//  If mod selected on
//    if running, stop.
//    store Mod_on
//    restart with mod
//
//    if not running, store Mod_on
//
//  If gain up or down selected
//    increment/decrement gain figure and store
//    if running send command to change
//
//  if load firmware selected
//    stop output
//    kill server
//    check presence of Express
//    reload fiormeare
//    start server




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


/***************************************************************************//**
 * @brief Checks whether the DATV Express is connected
 *
 * @param 
 *
 * @return 0 if present, 1 if absent
*******************************************************************************/

int CheckExpressConnect()
{
  FILE *fp;
  char response[255];
  int responseint;

  /* Open the command for reading. */
  fp = popen("lsusb | grep -q 'CY7C68013' ; echo $?", "r");
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

/***************************************************************************//**
 * @brief Checks whether the DATV Express Server is Running
 *
 * @param 
 *
 * @return 0 if running, 1 if not running
*******************************************************************************/

int CheckExpressRunning()
{
  FILE *fp;
  char response[255];
  int responseint;

  /* Open the command for reading. */
  fp = popen("pgrep -x 'express_server' ; echo $?", "r");
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


/***************************************************************************//**
 * @brief Called to start the DATV Express Server
 *
 * @param 
 *
 * @return 0 if running OK, 1 if not running
*******************************************************************************/

int StartExpressServer()
{
  char BashText[255];
  int responseint;

  // Check if the server is already running
  if (CheckExpressRunning() == 1)   // Not running, so start it
  {
    // Make sure the control file is not locked by deleting it
    strcpy(BashText, "sudo rm /tmp/expctrl >/dev/null 2>/dev/null");
    system(BashText);

    // Start the server
    strcpy(BashText, "cd /home/pi/express_server; ");
    // strcat(BashText, "sudo nice -n -40 /home/pi/express_server/express_server  >/dev/null 2>/dev/null &");
    strcat(BashText, "sudo nice -n -40 /home/pi/express_server/express_server & >/dev/null 2>/dev/null");
    system(BashText);
    strcpy(BashText, "cd /home/pi");
    system(BashText);
    //MsgBox4("Please wait 5 seconds", "while the DATV Express firmware", "is loaded", " ");
    usleep(5000000);
    responseint = CheckExpressRunning();
  }
  else
  {
    responseint = 0;
  }
  return responseint;
}


/***************************************************************************//**
 * @brief Called to stop the DATV Express Server
 *
 * @param nil
 *
 * @return nil
*******************************************************************************/

void StopExpressServer()
{
  system("sudo killall express_server >/dev/null 2>/dev/null");
  system("sudo rm /tmp/expctrl >/dev/null 2>/dev/null");
}


/***************************************************************************//**
 * @brief Calculates output level for given frequency
 *
 * @param uint64_t freq, int level 
 *
 * @return int DisplayLevel in tenths of a dBm
*******************************************************************************/
int CalcOPLevel(uint64_t DisplayFreq, int level, bool ModOn)
{
  uint64_t CalFreq[12];         // Frequency for calibration point
  int      CalLevel[12];        // Output level in tenths of a dBm at level 0

  CalFreq [1] = 70000000;
  CalLevel[1] = -320;
  CalFreq [2] = 120000000;
  CalLevel[2] = -304;
  CalFreq [3] = 230000000;
  CalLevel[3] = -300;
  CalFreq [4] = 435000000;
  CalLevel[4] = -304;
  CalFreq [5] = 650000000;
  CalLevel[5] = -310;
  CalFreq [6] = 800000000;
  CalLevel[6] = -320;
  CalFreq [7] = 1000000000;
  CalLevel[7] = -325;
  CalFreq [8] = 1250000000;
  CalLevel[8] = -331;
  CalFreq [9] = 1500000000;
  CalLevel[9] = -337;
  CalFreq [10] = 2000000000;
  CalLevel[10] = -357;
  CalFreq [11] = 2450000000;
  CalLevel[11] = -365;

  int PointBelow = 0;
  int PointAbove = 0;
  int n = 0;
  float proportion;
  int DisplayLevel;

  // Calculate output level from Osc based on Cal and frequency *********************

  while ((PointAbove == 0) && (n <= 100))
  {
    n = n + 1;
    if (DisplayFreq <= CalFreq[n])
    {
      PointAbove = n;
      PointBelow = n - 1;
    }
  }
  // printf("PointAbove = %d \n", PointAbove);

  if (DisplayFreq == CalFreq[n])
  {
    DisplayLevel = CalLevel[PointAbove];
  }
  else
  {
    proportion = (float)(DisplayFreq - CalFreq[PointBelow])/(CalFreq[PointAbove]- CalFreq[PointBelow]);
    // printf("proportion = %f \n", proportion);
    DisplayLevel = CalLevel[PointBelow] + (CalLevel[PointAbove] - CalLevel[PointBelow]) * proportion;
  }

  // Now adjust if modulation is on  *********************************************************************
  if (ModOn == true)
  {
    DisplayLevel = DisplayLevel - 2;   // Correction for DATV Express mod power (-0.2 dB)
  }
  return (int)(DisplayLevel) + 10 * level;
}


/***************************************************************************//**
 * @brief Start the output on the set frequency and level
 *
 * @param uint64_t freq, int level 
 *
 * @return nil
*******************************************************************************/
void ExpressOn(uint64_t DisplayFreq, int level)
{
  char ExpressCommand[255];
  char LevelText[255];
  char FreqText[255];

  strcpy(ExpressCommand, "echo \"set freq ");
  snprintf(FreqText, 12, "%ld", DisplayFreq);
  strcat(ExpressCommand, FreqText);
  strcat(ExpressCommand, "\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set fec 7/8\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set srate 333000\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set port 0\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set level ");
  snprintf(LevelText, 3, "%d", level);
  strcat(ExpressCommand, LevelText);
  strcat(ExpressCommand, "\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set car on\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set ptt tx\" >> /tmp/expctrl" );
  system(ExpressCommand);
}


/***************************************************************************//**
 * @brief Change the Frequency while running
 *
 * @param uint64_t freq 
 *
 * @return nil
*******************************************************************************/
void ChangeExpressFreq(uint64_t DisplayFreq)
{
  char ExpressCommand[255];
  char FreqText[255];

  strcpy(ExpressCommand, "echo \"set freq ");
  snprintf(FreqText, 12, "%ld", DisplayFreq);
  strcat(ExpressCommand, FreqText);
  strcat(ExpressCommand, "\" >> /tmp/expctrl" );
  system(ExpressCommand);

}


/***************************************************************************//**
 * @brief Change the Level while running
 *
 * @param int level 
 *
 * @return nil
*******************************************************************************/
void ChangeExpressLevel(int level)
{
  char ExpressCommand[255];
  char LevelText[255];

  strcpy(ExpressCommand, "echo \"set level ");
  snprintf(LevelText, 3, "%d", level);
  strcat(ExpressCommand, LevelText);
  strcat(ExpressCommand, "\" >> /tmp/expctrl" );
  system(ExpressCommand);
}



/***************************************************************************//**
 * @brief Start the output on the set frequency and level with QPSK mod
 *
 * @param uint64_t freq, int level 
 *
 * @return nil
*******************************************************************************/
void ExpressOnWithMod(uint64_t DisplayFreq, int level)
{
  char ExpressCommand[255];
  char LevelText[255];
  char FreqText[255];

  strcpy(ExpressCommand, "echo \"set freq ");
  snprintf(FreqText, 12, "%ld", DisplayFreq);
  strcat(ExpressCommand, FreqText);
  strcat(ExpressCommand, "\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set fec 7/8\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set srate 333000\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set port 0\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set level ");
  snprintf(LevelText, 3, "%d", level);
  strcat(ExpressCommand, LevelText);
  strcat(ExpressCommand, "\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set car off\" >> /tmp/expctrl" );
  system(ExpressCommand);

  strcpy(ExpressCommand, "echo \"set ptt tx\" >> /tmp/expctrl" );
  system(ExpressCommand);
}


/***************************************************************************//**
 * @brief Stop the output
 *
 * @param none 
 *
 * @return nil
*******************************************************************************/
void ExpressOff()
{
  char ExpressCommand[255];

  strcpy(ExpressCommand, "echo \"set ptt rx\" >> /tmp/expctrl" );
  system(ExpressCommand);
}


/***************************************************************************//**
 * @brief Set the modulation off whilst SigGen is running
 *
 * @param none 
 *
 * @return nil
*******************************************************************************/
void ExpressModOff()
{
  char ExpressCommand[255];

  strcpy(ExpressCommand, "echo set car on >> /tmp/expctrl" );
  system(ExpressCommand);
}


/***************************************************************************//**
 * @brief Set the modulation on whilst SigGen is running
 *
 * @param none 
 *
 * @return nil
*******************************************************************************/
void ExpressModOn()
{
  char ExpressCommand[255];

  strcpy(ExpressCommand, "echo set car off >> /tmp/expctrl" );
  system(ExpressCommand);
}



