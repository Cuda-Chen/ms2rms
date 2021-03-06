#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "traverse.h"

static void
usage ()
{
  printf ("Usage: ./ms2rms [mseedfile] [time window size] [window overlap] [a|r|j]\n\n");
  printf ("## Options ##\n"
          " mseedfile         input miniSEED file\n"
          " time window size  desired time window size, measured in seconds\n"
          "                   and the value should always bigger than 0\n"
          " window overlap    overlap percentage between each window\n"
          "                   and the value should always samller than 100\n"
          " a|r|j             output format indicator\n"
          "                   the flag is described as follows:\n"
          "                   a: all (rms and json)\n"
          "                   r: only rms\n"
          "                   j: only json\n");
  printf ("\nOutput format (rms): \n");
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
  int outputFormatFlag;

  /* Simplistic argument parsing */
  if (argc != 5)
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
  /* Get output file format indicator */
  if (strcmp (argv[4], "a") == 0)
    outputFormatFlag = 0;
  else if (strcmp (argv[4], "r") == 0)
    outputFormatFlag = 1;
  else if (strcmp (argv[4], "j") == 0)
    outputFormatFlag = 2;

  int returnValue = traverseTimeWindow (mseedfile, outputFileRMS, outputFileJSON, windowSize, windowOverlap, outputFormatFlag);
  //int returnValue = traverseTimeWindowLimited (mseedfile, outputFileRMS, outputFileJSON, windowSize, windowOverlap);
  if (returnValue < 0)
  {
    return -1;
  }

  return 0;
}
