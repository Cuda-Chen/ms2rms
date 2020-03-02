#ifndef MIN_MAX_H
#define MIN_MAX_H

#include <stdint.h>

void getMinMaxAndDemean(double *data, uint64_t dataSize, 
    double *min, double *max,
    double *minDemean, double *maxDemdea, double mean);

#endif
