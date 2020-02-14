#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "libmseed.h"

/*#include "standard_deviation.h"*/

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

  int year;
  int yday;
  int windowSize;
  int windowOverlap;
  nstime_t windowSize_in_ns;
  nstime_t windowOverlap_in_ns;
  const char *delims = ".";

  /* Simplistic argument parsing */
  if (argc != 4)
  {
    ms_log (2, "Usage: ./ms2rms <mseedfile> <time window size> <window overlap>\n");
    return -1;
  }
  mseedfile  = argv[1];
  int len    = strlen (mseedfile);
  char *temp = (char *)malloc (sizeof (char) * (len + 1));
  strncpy (temp, mseedfile, len);
  temp[len] = '\0';
  int l     = 0;
  char *ssc = strstr (temp, "/");
  do
  {
    l    = strlen (ssc) + 1;
    temp = &temp[strlen (temp) - l + 2];
    ssc  = strstr (temp, "/");
  } while (ssc);
  /* Convert windowSize and windowOverlap to HH:MM:SS
   * format so that ms_time2nstime() will like it */
  windowSize    = atoi (argv[2]);
  windowOverlap = atoi (argv[3]);

  /* Set bit flag to validate CRC */
  flags |= MSF_VALIDATECRC;

  /* Set bit flag to build a record list */
  flags |= MSF_RECORDLIST;

  printf ("input file name: %s\n", mseedfile);
  printf ("base name of input file: %s\n", temp);
  printf ("window size: %d\n", windowSize);
  printf ("window overlap: %d\n", windowOverlap);

  /* tokenize the base name of input file test */
  char *ptrToken;
  ptrToken = strtok(temp, delims);
  char *tokens[5];
  int i = 0;
  while(ptrToken != NULL)
  {
    tokens[i++] = ptrToken;
    ptrToken = strtok(NULL, delims);
  }

  for(i = 0; i < 5; i++)
  {
    printf("%s\n", tokens[i]);
  }

  return 0;
}
