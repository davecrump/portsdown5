
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
  // Check if DATV Express is connected
  if (CheckExpressConnect() == 1)   // Not connected
  {
    MsgBox2("DATV Express Not connected", "Connect it or select another mode");
    wait_touch();
  }

  // Check if the server is already running
  if (CheckExpressRunning() == 1)   // Not running, so start it
  {
    // Make sure the control file is not locked by deleting it
    strcpy(BashText, "sudo rm /tmp/expctrl >/dev/null 2>/dev/null");
    system(BashText);

    // Start the server
    strcpy(BashText, "cd /home/pi/express_server; ");
    strcat(BashText, "sudo nice -n -40 /home/pi/express_server/express_server  >/dev/null 2>/dev/null &");
    system(BashText);
    strcpy(BashText, "cd /home/pi");
    system(BashText);
    MsgBox4("Please wait 5 seconds", "while the DATV Express firmware", "is loaded", " ");
    usleep(5000000);
    responseint = CheckExpressRunning();
    if (responseint == 0)  // Running OK
    {
      MsgBox4(" ", " ", " ", "DATV Express Firmware Loaded");
      usleep(1000000);
    }
    else
    {
      MsgBox4("Failed to load", "DATV Express firmware.", "Please check connections", "and try again");
      wait_touch();
    }
    setBackColour(0, 0, 0);
    clearScreen();
  }
  else
  {
    responseint = 0;
  }
  return responseint;
}


/***************************************************************************//**
 * @brief Called on GUI start to check if the DATV Express Server needs to be started
 *
 * @param nil
 *
 * @return nil
*******************************************************************************/

void CheckExpress()
{
  //Check if DATV Express Required
  if (strcmp(osc, "express") == 0)  // Startup mode is DATV Express
  {
    if (CheckExpressConnect() == 1)   // Not connected
    {
      MsgBox2("DATV Express Not connected", "Connect it now or select another mode");
      wait_touch();
      setBackColour(0, 0, 0);
      clearScreen();
    }
    if (CheckExpressConnect() != 1)   // Connected
    {
      StartExpressServer();
    }
  }
}


/***************************************************************************//**
 * @brief Called to stop the DATV Express Server
 *
 * @param nil
 *
 * @return nil
*******************************************************************************/

void StopExpressServer()      // Output Oscillator
{
  system("sudo killall express_server >/dev/null 2>/dev/null");
  system("sudo rm /tmp/expctrl >/dev/null 2>/dev/null");
}



// Set the freq here if output active

  if (OutputStatus == 1)
  {
    if (strcmp(osc, "express") == 0)
    {
      strcpy(ExpressCommand, "echo \"set freq ");
      snprintf(FreqText, 12, "%lld", DisplayFreq);
      strcat(ExpressCommand, FreqText);
      strcat(ExpressCommand, "\" >> /tmp/expctrl" );
      system(ExpressCommand);
    }


    // Now send command to change level
    strcpy(ExpressCommand, "echo \"set level ");
    snprintf(LevelText, 3, "%d", level);
    strcat(ExpressCommand, LevelText);
    strcat(ExpressCommand, "\" >> /tmp/expctrl" );
    system(ExpressCommand);

void CalcOPLevel()
{
  int PointBelow = 0;
  int PointAbove = 0;
  int n = 0;
  float proportion;
  float MinAtten;

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
  if ((ModOn == 1) && (strcmp(osc, "express")==0))
  {
    DisplayLevel = DisplayLevel - 2;   // Correction for DATV Express mod power (-0.2 dB)
  }
}

void ExpressOn()
{
  char ExpressCommand[255];
  char LevelText[255];
  char FreqText[255];

  strcpy(ExpressCommand, "echo \"set freq ");
  snprintf(FreqText, 12, "%lld", DisplayFreq);
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

void ExpressOnWithMod()
{
  char ExpressCommand[255];
  char LevelText[255];
  char FreqText[255];

  strcpy(ExpressCommand, "sudo rm videots");
  system(ExpressCommand); 
  strcpy(ExpressCommand, "mkfifo videots");
  system(ExpressCommand); 

  strcpy(ExpressCommand, "echo \"set freq ");
  snprintf(FreqText, 12, "%lld", DisplayFreq);
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

  strcpy(ExpressCommand, "sudo nice -n -30 netcat -u -4 127.0.0.1 1314 < videots &");
  system(ExpressCommand);

  strcpy(ExpressCommand, "/home/pi/rpidatv/bin/avc2ts -b 372783 -m 537044 ");
  strcat(ExpressCommand, "-x 720 -y 576 -f 25 -i 100 -o videots -t 3 -p 255 -s SigGen &");
  system(ExpressCommand);
}


void InitOsc()
// Check the freq is in bounds and start/stop DATV express if req
// Read in amplitude Cal table and hide unused buttons
// Call CalcOPLevel
{
  char Param[256];
  char Value[256];
  char KillExpressSvr[255];
  int n;
  char PointNumber[255];
  ImposeBounds();

  if (strcmp(osc, "express") == 0)
  {
    strcpy(osc_text, "DATV Express");
    printf("Starting DATV Express\n");
    n = StartExpressServer();
    printf("Response from StartExpressServer was %d\n", n);
  }
  else
  {
    strcpy(KillExpressSvr, "echo \"set kill\" >> /tmp/expctrl");
    system(KillExpressSvr);
  }


    // Turn off attenuator if not compatible with mode
  if ((strcmp(osc, "pluto") == 0)   || (strcmp(osc, "pluto5") == 0) 
   || (strcmp(osc, "elcom") == 0)   || (strcmp(osc, "express") == 0) 
   || (strcmp(osc, "lime") == 0)    || (strcmp(osc, "slo") == 0)
   || (strcmp(osc, "adf4153") == 0) || (strcmp(osc, "ad9850") == 0))
  {
    AttenIn = 0;
    SetAtten(0);
  }

  // Turn off modulation if not compatible with mode
  if ((strcmp(osc, "pluto") == 0)   || (strcmp(osc, "pluto5") == 0)  || (strcmp(osc, "elcom") == 0)
   || (strcmp(osc, "adf4351") == 0) || (strcmp(osc, "adf5355") == 0) || (strcmp(osc, "slo") == 0)
   || (strcmp(osc, "adf4153") == 0) || (strcmp(osc, "lime") == 0) || (strcmp(osc, "ad9850") == 0))
  {
    ModOn = 0;
  }


  // Read in amplitude Cal table

   {
    strcpy(Param, osc);
    strcat(Param, "points");
    GetConfigParam(PATH_CAL, Param, Value);
    CalPoints = atoi(Value);
    for ( n = 1; n <= CalPoints; n = n + 1 )
    {
      snprintf(PointNumber, 4, "%d", n);

      strcpy(Param, osc);
      strcat(Param, "freq");
      strcat(Param, PointNumber);
      GetConfigParam(PATH_CAL, Param, Value);
      CalFreq[n] = strtoull(Value, 0, 0);

      strcpy(Param, osc);
      strcat(Param, "lev");
      strcat(Param, PointNumber);
      GetConfigParam(PATH_CAL, Param, Value);
      CalLevel[n] = atoi(Value);
    }
  }

  CalcOPLevel();
}

void SelectOsc(int NoButton)      // Output Oscillator
{
  // Stop or reset DATV Express Server if required
  if (strcmp(osc, "express") == 1)  // mode was DATV Express
  {
    system("sudo killall express_server >/dev/null 2>/dev/null");
    system("sudo rm /tmp/expctrl >/dev/null 2>/dev/null");
  }

  // Start DATV Express if required
  StartExpressServer();
  strcpy(osc, "express");
  strcpy(osc_text, "DATV Express");  

  InitOsc();
}

void ImposeBounds()  // Constrain DisplayFreq and level to physical limits
{
  if (strcmp(osc, "express")==0)
  {
    SourceUpperFreq = 2450000000;
    SourceLowerFreq =   70000000;
    strcpy(osc_text, "DATV Express");  
    if (level > 47)
    {
      level = 47;
    }
    if (level < 0)
    {
      level = 0;
    }
  }


  if (DisplayFreq > SourceUpperFreq)
  {
    DisplayFreq = SourceUpperFreq;
  }
  if (DisplayFreq < SourceLowerFreq)
  {
    DisplayFreq = SourceLowerFreq;
  }
}

void OscStart()
{
  //  Look up which oscillator we are using
  // Then use an if statement for each alternative

  // Set the attenuator if required
  if (AttenIn ==1)
  {
    SetAtten(atten);
  }

  printf("Oscillator Start\n");
  char Startadf4351[255] = "sudo /home/pi/rpidatv/bin/adf4351 ";
  char transfer[255];
  double freqmhz;
  int adf4351_lev = level; // 0 to 3

  if (strcmp(osc, "express")==0)
  {
    if (ModOn == 0)  // Start Express without Mod
    {
      ExpressOn();
    }
    else           // Start Express with Mod
    {
      ExpressOnWithMod();
    }
  }

  OutputStatus = 1;
}

void OscStop()
{
  char expressrx[255];
  printf("Oscillator Stop\n");

  if (strcmp(osc, "express") == 0)
  {
    strcpy( expressrx, "echo \"set ptt rx\" >> /tmp/expctrl" );
    system(expressrx);
    //strcpy( expressrx, "echo \"set car off\" >> /tmp/expctrl" );
    //system(expressrx);
    system("sudo killall netcat >/dev/null 2>/dev/null");
    printf("\nStopping Express output\n");
  }
  printf("Oscillator %s selected \n", osc);


  // Kill the key processes as nicely as possible

  system("sudo killall ffmpeg >/dev/null 2>/dev/null");
  system("sudo killall tcanim >/dev/null 2>/dev/null");
  system("sudo killall avc2ts >/dev/null 2>/dev/null");
  system("sudo killall netcat >/dev/null 2>/dev/null");

  // Then pause and make sure that avc2ts has really been stopped (needed at high SRs)
  usleep(1000);
  system("sudo killall -9 avc2ts >/dev/null 2>/dev/null");
  SetButtonStatus(ButtonNumber(11, 30), 0);
  OutputStatus = 0;
}

        case 32:                                   // Modulation or Cal
          if (strcmp(osc, "express") == 0)
          {
            if (ModOn == 0)
            {
              ModOn = 1;
              if (OutputStatus == 1)  // Oscillator already running
              {
                OscStop();
                OscStart();
              }
              SelectInGroupOnMenu(CurrentMenu, 32, 32, 32, 1);
            }
            else
            {
              ModOn = 0;
              if (OutputStatus == 1)  // Oscillator already running
              {
                OscStop();
                OscStart();
              }
              SelectInGroupOnMenu(CurrentMenu, 32, 32, 32, 0);
            }
            CalcOPLevel();
          }


