int getTouchSampleThread(int *rawX, int *rawY)
{
  int i;
  size_t rb;                       // how many bytes were read
  struct input_event ev[128];      // the events (up to 128 at once)
  int StartTouch = 0;
  int FinishTouch = 0;

  *rawX = -1;                      // Start with invalid values
  *rawY = -1;

  //debug_level = 3;  // uncomment for touchscreen diagnostics

  if (debug_level == 3)
  {
    printf("\n***************Waiting for next Touch*************** \n\n");
  }

  while (FinishTouch == 0)     // keep listening until touch has finished,; exit with most recent x and y
  {
    // Program flow blocks here until there is a touch event
    rb = read(fd, ev, sizeof(struct input_event) * 64);

    if (debug_level == 3)
    {
      printf("\n*** %ld bytes read.  Input event size %ld bytes, so %ld events:\n", rb, sizeof(struct input_event), (rb / sizeof(struct input_event)));
    }

    for (i = 0;  i <  (rb / sizeof(struct input_event)); i++)  // For each input event:
    {
      if (ev[i].type ==  EV_SYN)
      {
        if (debug_level == 3)
        {
            printf("Event type is %s%s%s = Start of New Event\n", KYEL, events[ev[i].type], KWHT);
        }
      }
      else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
      {
        StartTouch = 1;
        if (debug_level == 3)
        {
          printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s1%s = Touch Starting\n",
                  KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, KWHT);
        }
      }
      else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
      {
        FinishTouch = 1;
        if (debug_level == 3)
        {
          printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s0%s = Touch Finished\n",
                  KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, KWHT);
        }
      }
      else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0)
      {
        ValidX = ev[i].value;
        if (debug_level == 3)
        {
          printf("Event type is %s%s%s & Event code is %sX(0)%s & Event value is %s%d%s\n",
                  KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, ev[i].value, KWHT);
        }
      }
      else if (ev[i].type == EV_ABS  && ev[i].code == 1 && ev[i].value > 0)
      {
        ValidY = ev[i].value;
        if (debug_level == 3)
        {
          printf("Event type is %s%s%s & Event code is %sY(1)%s & Event value is %s%d%s\n",
                  KYEL, events[ev[i].type], KWHT, KYEL, KWHT, KYEL, ev[i].value, KWHT);
        }
      }

      if (debug_level == 3)
      {
        printf(" At end of Event %d, ValidX = %d, ValidY = %d, StartTouch = %d, FinishTouch = %d\n", i, ValidX, ValidY, StartTouch, FinishTouch);
      }

      if((ValidX != -1) && (ValidY != -1) && (FinishTouch == 1))  // Check for valid touch criteria
      {
        *rawX = ValidX;
        ValidX = -1;
        *rawY = ValidY;
        ValidY = -1;
        if (debug_level == 3)
        {
          printf("\nValid Touchscreen Touch Event: rawX = %d, rawY = %d\n", *rawX, *rawY);
        }

        // Inversion required if toucscreen has been inverted.  Needs selecting on touchscreen orientation
        if(touch_inverted == true)
        {
          *rawX = 800 - *rawX;
          *rawY = 480 - *rawY;
        }

        return 1;
      }
    }
  }
  return 0;
}


void CWOnOff(int onoff)
{
  int rc;

  if((rc = iio_channel_attr_write_bool(tx0_i, "raw", onoff)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }
  if((rc = iio_channel_attr_write_bool(tx0_q, "raw", onoff)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }
}


int plutotx(bool cal)
{
  const char *value;
  long long freq;
  double dBm;
  int rc;
  char URIPLUTO[20] = "ip:";
  
  freq = DisplayFreq;
  dBm = level;
  strcat(URIPLUTO, PlutoIP);

  ctx = iio_create_context_from_uri(URIPLUTO);

  if(ctx == NULL)
  {
    stderrandexit("Connection failed", 0, __LINE__);
  }

  if((value = iio_context_get_attr_value(ctx, "ad9361-phy,model")) != NULL)
  {
    if(strcmp(value, "ad9364"))
    {
      stderrandexit("Pluto is not expanded", 0, __LINE__);
    }
  }
  else
  {
    stderrandexit("Error retrieving phy model", 0, __LINE__);
  }

  phy = iio_context_find_device(ctx, "ad9361-phy");
  dds_core_lpc = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");  
  tx0_i = iio_device_find_channel(dds_core_lpc, "altvoltage0", true);
  tx0_q = iio_device_find_channel(dds_core_lpc, "altvoltage2", true);
  tx1_i = iio_device_find_channel(dds_core_lpc, "altvoltage1", true);
  tx1_q = iio_device_find_channel(dds_core_lpc, "altvoltage3", true);
  tx_chain=iio_device_find_channel(phy, "voltage0", true);
  tx_lo=iio_device_find_channel(phy, "altvoltage1", true);

  if(!phy || !dds_core_lpc || !tx0_i || !tx0_q || !tx_chain || !tx_lo)
  {
    stderrandexit("Error finding device or channel", 0, __LINE__);
  }

  //enable internal TX local oscillator
  if((rc = iio_channel_attr_write_bool(tx_lo, "external", false)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  //disable fastlock feature of TX local oscillator
  if((rc = iio_channel_attr_write_bool(tx_lo, "fastlock_store", false)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  //power on TX local oscillator
  if((rc = iio_channel_attr_write_bool(tx_lo, "powerdown", false)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  //full duplex mode
  if((rc = iio_device_attr_write(phy, "ensm_mode", "fdd")) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  if (cal)
  {
    //calibration mode to auto
    if((rc = iio_device_attr_write(phy, "calib_mode", "auto")) < 0)
    {
      stderrandexit(NULL, rc, __LINE__);
    }
  }
  else
  {
    //calibration mode to manual
    if((rc = iio_device_attr_write(phy, "calib_mode", "manual")) < 0)
    {
      stderrandexit(NULL, rc, __LINE__);
    }
  }

  // Set the hardware gain
  if((rc = iio_channel_attr_write_double(tx_chain, "hardwaregain", dBm - REFTXPWR)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  // Set the TX bandwidth (to 4 MHz)
  if((rc = iio_channel_attr_write_longlong(tx_chain, "rf_bandwidth", FBANDWIDTH)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  if (cal)
  {
    // Set the sampling frequency (to 4 MHz), produces spike and is non-volatile so only do for Cal
    if((rc = iio_channel_attr_write_longlong(tx_chain, "sampling_frequency", FSAMPLING) ) < 0)
    {
      stderrandexit(NULL, rc, __LINE__);
    }
  }

  // Set the I channel 0 DDS scale  0.8 reduces the LSB 3rd harmonic (was 1.0)
  if((rc = iio_channel_attr_write_double(tx0_i, "scale", 0.8)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  // Set the Q channel 0 DDS scale
  if((rc = iio_channel_attr_write_double(tx0_q, "scale", 0.8)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  // Disable the DDSs on Channel 1
  if((rc = iio_channel_attr_write_double(tx1_i, "scale", 0.0)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }
  if((rc = iio_channel_attr_write_double(tx1_q, "scale", 0.0)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  // Set the I channel stimulus frequency (1 MHz)
  if((rc = iio_channel_attr_write_longlong(tx0_i, "frequency", FCW)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  // Set the Q channel stimulus frequency (1 MHz)
  if((rc = iio_channel_attr_write_longlong(tx0_q, "frequency", FCW)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  // Set the I channel phase to 90 degrees for USB
  if((rc = iio_channel_attr_write_longlong(tx0_i, "phase", 90000)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  // Set the Q channel phase to 0 degrees for USB
  if((rc = iio_channel_attr_write_longlong(tx0_q, "phase", 0)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  if (strcmp(osc, "pluto") == 0)
  {
    // Set the LO frequency for USB operation
    if((rc = iio_channel_attr_write_longlong(tx_lo, "frequency", freq - FCW)) < 0)
    {
      stderrandexit(NULL, rc, __LINE__);
    }
  }
  else  // 5th harmonic operation
  {
printf("freq = %lld\n", freq);
   // Set the LO frequency for USB operation
    if((rc = iio_channel_attr_write_longlong(tx_lo, "frequency", (freq - FCW) / 5)) < 0)
    {
      stderrandexit(NULL, rc, __LINE__);
    }
  }
  
  CWOnOff(1); //Turn the output on

  return 0;
}


int PlutoOff()
{
  const char *value;
  int rc;
  char URIPLUTO[20] = "ip:";
  strcat(URIPLUTO, PlutoIP);

  ctx = iio_create_context_from_uri(URIPLUTO);

  if(ctx==NULL)
  {
    // stderrandexit("Connection failed", 0, __LINE__);
    return 1;  // Don't throw an error otherwise it obstructs system exit
  }

  if((value = iio_context_get_attr_value(ctx, "ad9361-phy,model"))!=NULL)
  {
    if(strcmp(value,"ad9364"))
    {
      stderrandexit("Pluto is not expanded",0,__LINE__);
    }
  }
  else
  {
    stderrandexit("Error retrieving phy model",0,__LINE__);
  }

  phy = iio_context_find_device(ctx, "ad9361-phy");
  dds_core_lpc = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");  
  tx0_i = iio_device_find_channel(dds_core_lpc, "altvoltage0", true);
  tx0_q = iio_device_find_channel(dds_core_lpc, "altvoltage2", true);
  tx_chain=iio_device_find_channel(phy, "voltage0", true);
  tx_lo=iio_device_find_channel(phy, "altvoltage1", true);

  if(!phy || !dds_core_lpc || !tx0_i || !tx0_q || !tx_chain || !tx_lo)
  {
    stderrandexit("Error finding device or channel", 0, __LINE__);
  }

  //power off TX local oscillator
  if((rc = iio_channel_attr_write_bool(tx_lo, "powerdown", true)) < 0)
  {
    stderrandexit(NULL, rc, __LINE__);
  }

  CWOnOff(0);  // Make sure I and Q channels are off

  iio_context_destroy(ctx);

  return 0;
}
