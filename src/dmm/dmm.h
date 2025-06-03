#ifndef _DMM_H__
#define _DMM_H__

typedef struct
{
  int r,g,b;
} color_t;

typedef struct
{
  char Text[255];
  color_t  Color;
} status_t;

typedef struct
{
  int adc_val;
  float pwr_dBm;
} adc_lookup;


int ad8318_3_underrange = 633;
int ad8318_3_overrange = 161;



#endif /* _DMM_H__ */
