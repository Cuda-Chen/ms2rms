#include "min_max.h"

void getMinMaxAndDemean(double *data, uint64_t dataSize,
    double *min, double *max,
    double *minDemean, double *maxDemean, double mean)
{
    *min = *max = data[0];
    uint64_t i;
    for(i = 1; i < dataSize; i++)
    {
        if(*min > data[i])
        {
            *min = data[i];
        }
        if(*max < data[i])
        {
            *max = data[i];
        }
    }

    *minDemean = *min - mean;
    *maxDemean = *max - mean;
}
