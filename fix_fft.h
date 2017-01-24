/*
 * fix_fft.h
 *
 *  Created on: Dec 23, 2016
 *      Author: albertopetrucci
 */

#ifndef FIX_FFT_H_
#define FIX_FFT_H_

#include <stdint.h>
//#include <Energia.h>

#define DEBUG              0     //debug print, 0 = no debug print

/*
Highest valid frequency measured
        |                          FREQ_RESOLUTION
  LOG2N |     5       10       20       40       50      100      200      500
  ===== | ========|========|========|========|========|========|========|========|
    5   |                               600      750     1500     3000     7500
    6   |                      620     1240     1550     3100     6200
    7   |             630     1260     2520     3150     6300    12600
    8   |    635     1270     2540     5080     6350    12700
    9   |   1275     2550     5100    10200    12750
   10   |   2555     5110    10220
Milliseconds for one conversion
           |                          FREQ_RESOLUTION
  LOG2N |     5       10       20       40       50      100      200      500
  ===== | ========|========|========|========|========|========|========|========|
    5   |                               28       22       12       6        3
    6   |                      53       27       22       12       5
    7   |            105       54       29       24       12       9
    8   |    209     108       57       32       24       18
    9   |    215     115       65       40       36
   10   |    229     130       81
*/

#define LOG2N              9     //log base 2 of the number of points, e.g. LOG2N = 8 is 256 points
#define FREQ_RESOLUTION   20     //Frequency resolution of output in Hz
#define ANALOG_IN          6     //analog input pin
#define ANALOG_RESOLUTION 12     //CPU specific - e.g. set to 12 for TM4C123/129 and 14 for MSP432
#define FFT_VECTOR_SIZE  512    //pow(2, LOG2N)
static const int nPts = FFT_VECTOR_SIZE;
static const int hiFreq = FREQ_RESOLUTION * (FFT_VECTOR_SIZE/2 - 1);

typedef struct fft
{
  int in_re[FFT_VECTOR_SIZE];
  int in_im[FFT_VECTOR_SIZE];
  uint16_t out[FFT_VECTOR_SIZE/2];
  uint16_t counter;
  uint8_t valid;
} fft_vector;

inline int FIX_MPY(int a, int b);
int fix_fft(int fr[], int fi[], int m, int inverse);
int fix_fftr(int f[], int m, int inverse);

void fftInit(fft_vector *fft);
void fftAddSample(fft_vector *fft, uint16_t sample);
void calculateFft(fft_vector *fft);

#endif /* FIX_FFT_H_ */
