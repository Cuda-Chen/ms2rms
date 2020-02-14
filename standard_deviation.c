#include <math.h>
#include <stdio.h>

#include "standard_deviation.h"

double
calculateSD (double *data, uint64_t dataSize)
{
  double sum = 0.0, mean, SD = 0.0;
  uint64_t i;
  for (i = 0; i < dataSize; i++)
  {
    sum += data[i];
  }
  mean = sum / (double)dataSize;
  for (i = 0; i < dataSize; i++)
  {
    SD += pow (data[i] - mean, 2);
  }
  printf ("mean %lf\n", mean);
  return sqrt (SD / dataSize);
}

void
testCalculateSD ()
{
  double test[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  printf ("RMS of test array: %lf\n", calculateSD (test, 10));
}
