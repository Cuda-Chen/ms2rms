#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>

#include "libmseed.h"

#include "standard_deviation.h"

int
main (int argc, char **argv)
{
  MS3TraceList *mstl        = NULL;
  MS3TraceID *tid           = NULL;
  MS3TraceSeg *seg          = NULL;
  MS3Selections *selections = NULL;

  char *mseedfile = NULL;
  uint32_t flags  = 0;
  int8_t verbose  = 0;
  size_t idx;
  int rv;

  int64_t unpacked;
  uint8_t samplesize;
  char sampletype;
  size_t lineidx;
  size_t lines;
  int col;
  void *sptr;

  /* Simplistic argument parsing */
  if (argc != 4)
  {
    ms_log (2, "Usage: ./ms2rms <mseedfile> <tine window size> <window overlap>\n");
    return -1;
  }
  mseedfile     = argv[1];

  /* Set bit flag to validate CRC */
  flags |= MSF_VALIDATECRC;

  /* Set bit flag to build a record list */
  flags |= MSF_RECORDLIST;

  int hour, minute, second;
}
