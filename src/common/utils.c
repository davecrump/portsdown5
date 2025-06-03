#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"

#define DELIM "."

/***************************************************************************//**
 * @brief safely copies n characters of instring to outstring without overflow
 *
 * @param *outstring
 * @param *instring
 * @param n int number of characters to copy.  Max value is the outstring array size -1
 *
 * @return void
*******************************************************************************/
void strcpyn(char *outstring, char *instring, int n)
{
  //printf("\ninstring= -%s-, instring length = %d, desired length = %d\n", instring, strlen(instring), strnlen(instring, n));
  
  n = strnlen(instring, n);
  int i;
  for (i = 0; i < n; i = i + 1)
  {
    //printf("i = %d input character = %c\n", i, instring[i]);
    outstring[i] = instring[i];
  }
  outstring[n] = '\0'; // Terminate the outstring
  //printf("i = %d input character = %c\n", n, instring[n]);
  //printf("i = %d input character = %c\n\n", (n + 1), instring[n + 1]);

  //for (i = 0; i < n; i = i + 1)
  //{
  //  printf("i = %d output character = %c\n", i, outstring[i]);
  //}  
  //printf("i = %d output character = %c\n", n, outstring[n]);
  //printf("i = %d output character = %c\n", (n + 1), outstring[n + 1]);

  //printf("outstring= -%s-, length = %d\n\n", outstring, strlen(outstring));
}


/***************************************************************************//**
 * @brief Checks if a string is a valid IP address
 *
 * @param char *ip_str
 *
 * @return true if valid, false if invalid
*******************************************************************************/

bool is_valid_ip(char *ip_str) 
{ 
  int num;
  int dots = 0; 
  char *ptr; 
  
  if (ip_str == NULL) 
  {
    return false; 
  }

  ptr = strtok(ip_str, DELIM); 
  if (ptr == NULL) 
  {
    return false; 
  }
  
  while (ptr)
  { 
    // after parsing string, it must contain only digits
    if (valid_digit(ptr) == false) 
    {
      return false; 
    }  
    num = atoi(ptr); 
  
    // check for valid numbers
    if (num >= 0 && num <= 255)
    { 
      // parse remaining string
      ptr = strtok(NULL, DELIM); 
      if (ptr != NULL)
      {
        ++dots; 
      }
    }
    else
    {
      return false; 
    }
  }

  // valid IP string must contain 3 dots
  if (dots != 3)
  { 
    return false;
  }
  return true;
} 


/***************************************************************************//**
 * @brief Checks if a string contains only digits
 *
 * @param char *ip_str
 *
 * @return true if valid, false if invalid
*******************************************************************************/

bool valid_digit(char *ip_str) 
{ 
  while (*ip_str) 
  { 
    if (*ip_str >= '0' && *ip_str <= '9')
    { 
      ++ip_str; 
    }
    else
    {
      return false; 
    }
  } 
  return true; 
} 

