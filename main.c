#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "traverse.h"

static void
usage ()
{
  printf ("Usage: ./ms2rms <mseedfile> <time window size> <window overlap> [-r]\n\n");
  printf ("## Options ##\n"
          " -r        Calculate RMS in the time range of the file\n");
  printf ("\nOutput format: \n");
  printf ("\
<time stamp of the first window>,<station>,<network>,<channel>,<location>,<CR><LF>\n\
<time difference between this window to the first window>,<mean>,<SD>,<min>,<max>,<minDemean>,<maxDemean>,<CR><LF>\n\
<time difference between this window to the first window>,<mean>,<SD>,<min>,<max>,<minDemean>,<maxDemean>,<CR><LF>\n\
...  \
\n");
}

int
main (int argc, char **argv)
{
  char *mseedfile = NULL;
  int windowSize;
  int windowOverlap;
  char *outputFileRMS;
  char *outputFileJSON;
  const char *RMSExtension  = ".rms";
  const char *JSONExtension = ".json";

  /* Simplistic argument parsing */
  if (argc != 4)
  {
    usage ();
    return -1;
  }
  /* Get file name without path */
  mseedfile  = argv[1];
  int len    = strlen (mseedfile);
  char *temp = (char *)malloc (sizeof (char) * (len + 1));
  strncpy (temp, mseedfile, len);
  temp[len] = '\0';
  int l     = 0;
  char *ssc = strstr (temp, "/");
  while (ssc)
  {
    l    = strlen (ssc) + 1;
    temp = &temp[strlen (temp) - l + 2];
    ssc  = strstr (temp, "/");
  }
  size_t tempLen = strlen (temp);
#ifdef DEBUG
  printf ("temp str size: %ld content: %s\n", tempLen, temp);
#endif
  /* Get window size */
  windowSize = atoi (argv[2]);
  /* Get overlap percentage between each window */
  windowOverlap = atoi (argv[3]);
  if (windowSize <= 0)
  {
    printf ("This doesn't make sense because time window size is smaller than zero.\n");
    return -1;
  }
  if (windowOverlap >= 100)
  {
    printf ("This doesn't make sense because if window overlap percentage is bigger of \
equal than 100 will create infinite loop\n");
    return -1;
  }
  /* Create output files (.rms and .json) */
  outputFileRMS  = (char *)malloc (sizeof (char) * (1 + tempLen + strlen (RMSExtension)));
  outputFileJSON = (char *)malloc (sizeof (char) * (1 + tempLen + strlen (JSONExtension)));
  strcpy (outputFileRMS, temp);
  strcat (outputFileRMS, RMSExtension);
  strcpy (outputFileJSON, temp);
  strcat (outputFileJSON, JSONExtension);

  int returnValue = traverseTimeWindow (mseedfile, outputFileRMS, outputFileJSON, windowSize, windowOverlap);
  if (returnValue < 0)
  {
    return -1;
  }

  return 0;
}
