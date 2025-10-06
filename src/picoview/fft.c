#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>

#include <pthread.h>
#include <math.h>

// needs sudo apt install libfftw3-dev
#include <fftw3.h>

#include "../common/timing.h"

#define PI 3.14159265358979323846
#define FFT_SIZE 1024

extern bool Range20dB;
extern int BaseLine20dB;
extern int8_t BaselineShift;
extern float smoothing_factor; // 0.0 - 1.0
extern int32_t freqoffset;
extern int adresult[2047];
extern bool scope;
extern bool dataready;
extern bool invert_scope;

static float hanning_window_const[FFT_SIZE];
static float hamming_window_const[FFT_SIZE];

static fftwf_complex* fft_in;
static fftwf_complex* fft_out;
static fftwf_plan fft_plan;

static float fft_data_staging[FFT_SIZE];
static float fft_scaled_data[FFT_SIZE];
int y[515];


void main_fft_init(void)
{
  for(int i = 0; i < FFT_SIZE; i++)
  {
    /* Hanning */
    hanning_window_const[i] = 0.5 * (1.0 - cos(2*M_PI*(((float)i)/FFT_SIZE)));

    /* Hamming */
    hamming_window_const[i] = 0.54 - (0.46 * cos(2*M_PI*(0.5+((2.0*((float)i/(FFT_SIZE-1))+1.0)/2))));
  }

  /* Set up FFTW */
  fft_in = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * FFT_SIZE);
  fft_out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * FFT_SIZE);
  fft_plan = fftwf_plan_dft_1d(FFT_SIZE, fft_in, fft_out, FFTW_FORWARD, FFTW_PATIENT);
  printf(" "); fftwf_print_plan(fft_plan); printf("\n");
}


static void fft_fftw_close(void)
{
  /* De-init fftw */
  fftwf_free(fft_in);
  fftwf_free(fft_out);
  fftwf_destroy_plan(fft_plan);
  fftwf_forget_wisdom();
}


/* FFT Thread */
void *fft_thread(void *arg)
{
  bool *exit_requested = (bool *)arg;
  int i;
  fftw_complex pt;
  double pwr;
  double lpwr;
  double pwr_scale;
  int16_t y_buffer;
  float adcfloat;

  pwr_scale = 1.0 / ((float)FFT_SIZE * (float)FFT_SIZE);

  while(false == *exit_requested)   // Go around once for each input buffer fill
  {
    // Wait for new data
    while ((dataready == false) && (false == *exit_requested))
    {
      usleep(100);
    }
    dataready = false;

    if((*exit_requested))
    {
      break;
    }

    if (scope == false)
    {
      for (i = 0; i < FFT_SIZE; i++)
      {
         adcfloat = 1 * ((float)(adresult[i] - 2048) / 2048);
         fft_in[i][0] = (adcfloat + 0.00048828125) * hanning_window_const[i];
         fft_in[i][1] = (adcfloat + 0.00048828125) * hanning_window_const[i];         
      }

      // Run FFT
      fftwf_execute(fft_plan);

      for (i = 0; i < FFT_SIZE / 2; i++)
      {
        // shift, normalize and convert to dBFS

        pt[0] = fft_out[505 + i - FFT_SIZE / 2][0] / FFT_SIZE;
        pt[1] = fft_out[505 + i - FFT_SIZE / 2][1] / FFT_SIZE;

        pwr = pwr_scale * ((pt[0] * pt[0]) + (pt[1] * pt[1]));
        lpwr = 10.f * log10(pwr + 1.0e-20);

        fft_data_staging[i] = (lpwr * (1.f - smoothing_factor)) + (fft_data_staging[i] * smoothing_factor);

        // Set the scaling and vertical offset
        // 5 pixels per dB.  140 adjusts 0 dBFS to be 0.  baseline shift in dB
        fft_scaled_data[i] = 5 * ((fft_data_staging[i] + 135 ) + (float)BaselineShift);  

        // Correct for the roll-off at the ends of the fft
        //if (i < 46)
        //{
        //  fft_scaled_data[i] = fft_scaled_data[i] + ((46 - i) * 2) / 5;
        //}
        //else if (i > 466)
        //{
        //  fft_scaled_data[i] = fft_scaled_data[i] + ((i - 466) * 2) / 5;
        //}

        if (Range20dB) // Range20dB
        {
          fft_scaled_data[i] = fft_scaled_data[i] - 5 * (80 + BaseLine20dB);  
          fft_scaled_data[i] = 4 * fft_scaled_data[i];
        }

        // Convert to int
        y_buffer = (int16_t)(fft_scaled_data[i]);

        // Make sure that the data is within bounds
        if (y_buffer < 1)
        {
          y[i] = 1;
        }
        else if (y_buffer > 399)
        {
          y[i] = 399;
        }
        else
        {
          y[i] = (uint16_t)y_buffer;
        }
      }
    }
    else                             // Scope - Direct amplitude plot
    {
      for (i = 0; i < FFT_SIZE / 2; i++)
      {
        y[i] = (int)((float)adresult[i] / 10.24);
        if (y[i] < 1)
        {
        y[i] = 1;
        }
        else if (y[i] > 399)
        {
          y[i] = 399;
        }
        else
        {
          y[i] = (uint16_t)y[i];
        }
        if (invert_scope == true)
        {
          y[i] = 400 - y[i];
        }
      }   
    }
    //printf("Max: %f, Min %f\n", int_max, int_min);
  }

  fft_fftw_close();
  printf("fft Thread Closed\n");
  return NULL;
}

