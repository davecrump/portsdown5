#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "temperature.h"

bool temperature_cpu(float *result)
{
  FILE *temperatureFile;
  double T;

  temperatureFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r");

  if(temperatureFile != NULL)
  {
    fscanf(temperatureFile, "%lf", &T);

    fclose(temperatureFile);
    
    *result = T/1000.0;
    return true;
  }

  return false;
}

/***************************************************************************//**
 * @brief Looks up the CPU Temp
 *
 * @param CPUTemp (str) CPU Temp to be passed as a string
 *
 * @return void
*******************************************************************************/

void GetCPUTemp(char CPUTemp[31])
{
  FILE *fp;

  /* Open the command for reading. */
  fp = popen("vcgencmd measure_temp", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(CPUTemp, 20, fp) != NULL)
  {
    //printf("%s", CPUTemp);
  }

  /* close */
  pclose(fp);
}

