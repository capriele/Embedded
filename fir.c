/*
 * fir.c
 *
 *  Created on: Dec 24, 2016
 *      Author: albertopetrucci
 */


/* Implementation of FIR filter */

#include <stdlib.h>
#include <string.h>
#include "fir.h"

/* Initialize FIR_filter struct
 * Params:
 *  int length -- number of filter coefficients
 *  double *h  -- array of filter coefficients
 * Returns:
 *  FIR_filter *p -- on success
 *  NULL          -- on error
 */

void FIR_init(FIR_filter *filter) {
	filter->last_filtered_sample = 0;
	filter->valid = 0;
    filter->length = (int)(sizeof(h)/sizeof(double)) ;
    filter->index = 1;
    filter->h = h;
    int i;
    for (i=0; i < filter->length; ++i) {
    		filter->delay_line[i] = 0;
    }
}

void FIR_put_sample(FIR_filter *filter, uint32_t input) {
    filter->delay_line[filter->index-1] = input;
    if(++filter->index > filter->length)
    		filter->index = 1;
}

/* Get next filtered sample of input signsl
 * Params:
 *  FIR_filter *filter -- pointer to filter structure
 *  double input       -- input signl sample
 * Returns:
 *  double result -- filtered sample
 */
int FIR_get_sample(FIR_filter *filter) {
	double result = 0.0;
    int i = 0;

    //QUEUE

    // Nota :
    // n % m == n & (m - 1)
    // where m is a power of 2.
    for (i=0; i < filter->length; ++i) {
    		result += filter->h[i] * filter->delay_line[(filter->index + i) % filter->length];
    }
    /*
    filter->delay_line[filter->count] = input;
    for (i=0; i < filter->length; ++i) {
        result += filter->h[i] * filter->delay_line[(filter->length-1) - i];
    }
    if(++filter->count >= filter->length)
        filter->count = 0;
     */
	filter->last_filtered_sample = (int)result;
    filter->valid = 1;
    return (int)result;
}

/* Free memory, allocated by filter
 * Params:
 *  FIR_filter *filter -- pointer to FIR_filter struct
 */
void FIR_destroy(FIR_filter *filter) {
    free(filter->delay_line);
    free(filter);
}

