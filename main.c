#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "libmseed.h"

#include "standard_deviation.h"

#define SECONDSINDAY 86400
#define SECONDSINHOUR 3600
#define SECONDSINMINUTE 60
static nstime_t NSECS = 1000000000;
#define TOKENSIZE 5

int
main (int argc, char **argv)
{
  MS3TraceList *mstl        = NULL;
  MS3TraceID *tid           = NULL;
  MS3TraceSeg *seg          = NULL;
  MS3Selections *selections = NULL;

  char *mseedfile     = NULL;
  char *selectionfile = NULL;
  char starttimestr[30];
  char endtimestr[30];
  uint32_t flags = 0;
  int8_t verbose = 0;
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
  windowSize    = atoi (argv[2]);
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

  /* Set bit flag to validate CRC */
  flags |= MSF_VALIDATECRC;

  /* Set bit flag to build a record list */
  flags |= MSF_RECORDLIST;

  /* tokenize the base name of input file test */
  char *ptrToken;
  ptrToken = strtok (temp, delims);
  char *tokens[TOKENSIZE];
  int i = 0;
  while (ptrToken != NULL)
  {
    tokens[i++] = ptrToken;
    ptrToken    = strtok (NULL, delims);
  }

  /* Print the input params for debug use */
  printf ("input file name: %s\n", mseedfile);
  printf ("base name of input file: %s\n", temp);
  printf ("window size: %d seconds\n", windowSize);
  printf ("window overlap: %d percent\n", windowOverlap);
  for (i = 0; i < TOKENSIZE; i++)
  {
    printf ("%s\n", tokens[i]);
  }

  /* Calculate how many segments of this routine */
  int nextTimeStamp = windowSize - (windowSize * windowOverlap / 100);
  int segments      = SECONDSINDAY / nextTimeStamp;
  nstime_t nextTimeStamp_ns = nextTimeStamp * NSECS;

  /* Loop over the segments' count */
  nstime_t start = ms_time2nstime (atoi (tokens[TOKENSIZE - 2]),
                                   atoi (tokens[TOKENSIZE - 1]), 0, 0, 0, 0);
  nstime_t end   = start + (nstime_t) (windowSize * NSECS);
  for (i = 0; i < segments; i++)
  {
    printf ("starttime: %s endtime: %s\n",
            ms_nstime2timestr (start, starttimestr, SEEDORDINAL, NANO),
            ms_nstime2timestr (end, endtimestr, SEEDORDINAL, NANO));

    start += nextTimeStamp_ns;
    end += nextTimeStamp_ns;
  }

  /* Convert seconds to nstime_t */
  int hour = 0, min = 0, sec = 0;
  uint32_t nsec      = 0;
  nstime_t starttime = ms_time2nstime (atoi (tokens[TOKENSIZE - 2]),
                                       atoi (tokens[TOKENSIZE - 1]), hour, min, sec, nsec);
  /*nstime_t endtime   = ms_time2nstime (atoi (tokens[TOKENSIZE - 2]),
                                     atoi (tokens[TOKENSIZE - 1]), hour + 1, min, sec, nsec);*/
  nstime_t endtime = starttime + (nstime_t) (windowSize * NSECS);
  printf ("starttime: %s endtime: %s\n",
          ms_nstime2timestr (starttime, starttimestr, SEEDORDINAL, NANO),
          ms_nstime2timestr (endtime, endtimestr, SEEDORDINAL, NANO));

  /* Create selection for time window */
  MS3Selections testselection;
  MS3SelectTime testselectime;

  testselection.sidpattern[0] = '*';
  testselection.sidpattern[1] = '\0';
  testselection.timewindows   = &testselectime;
  testselection.next          = NULL;
  testselection.pubversion    = 0;

  testselectime.starttime = starttime;
  testselectime.endtime   = endtime;
  testselectime.next      = NULL;

  ms3_printselections (&testselection);

  /* Read all miniSEED into a trace list, limiting to time selections */
  rv = ms3_readtracelist_selection (&mstl, mseedfile, NULL,
                                    &testselection, 0, flags, verbose);
  if (rv != MS_NOERROR)
  {
    ms_log (2, "Cannot read miniSEED from file: %s\n", ms_errorstr (rv));
    return -1;
  }

  /* Traverse trace list structures and print summary information */
  tid = mstl->traces;
  while (tid)
  {
    /* allocate the data array of every trace */
    double *data = NULL;
    uint64_t dataSize;

    ms_log (0, "TraceID for %s (%d), segments: %u\n",
            tid->sid, tid->pubversion, tid->numsegments);

    seg = tid->first;
    while (seg)
    {
      if (!ms_nstime2timestr (seg->starttime, starttimestr, ISOMONTHDAY, NANO) ||
          !ms_nstime2timestr (seg->endtime, endtimestr, ISOMONTHDAY, NANO))
      {
        ms_log (2, "Cannot create time strings\n");
        starttimestr[0] = endtimestr[0] = '\0';
      }

      ms_log (0, "  Segment %s - %s, samples: %" PRId64 ", sample rate: %g\n",
              starttimestr, endtimestr, seg->samplecnt, seg->samprate);

      /* Unpack and print samples for this trace segment */
      if (seg->recordlist && seg->recordlist->first)
      {
        /* Determine sample size and type based on encoding of first record */
        ms_encoding_sizetype (seg->recordlist->first->msr->encoding, &samplesize, &sampletype);

        /* Unpack data samples using record list.
         * No data buffer is supplied, so it will be allocated and assigned to the segment.
         * Alternatively, a user-specified data buffer can be provided here. */
        unpacked = mstl3_unpack_recordlist (tid, seg, NULL, 0, verbose);

        /* malloc the data array */
        dataSize = seg->numsamples;
        data     = (double *)malloc (sizeof (double) * dataSize);
        if (data == NULL)
        {
          printf ("something wrong when malloc data array\n");
          exit (-1);
        }

        if (unpacked != seg->samplecnt)
        {
          ms_log (2, "Cannot unpack samples for %s\n", tid->sid);
        }
        else
        {
          //ms_log (0, "DATA (%" PRId64 " samples) of type '%c':\n", seg->numsamples, seg->sampletype);

          if (sampletype == 'a')
          {
            printf ("%*s",
                    (seg->numsamples > INT_MAX) ? INT_MAX : (int)seg->numsamples,
                    (char *)seg->datasamples);
          }
          else
          {
            lines = (unpacked / 6) + 1;

            for (idx = 0, lineidx = 0; lineidx < lines; lineidx++)
            {
              for (col = 0; col < 6 && idx < seg->numsamples; col++)
              {
                sptr = (char *)seg->datasamples + (idx * samplesize);

                if (sampletype == 'i')
                {
                  //ms_log (0, "%10d  ", *(int32_t *)sptr);
                  data[idx] = (double)(*(int32_t *)sptr);
                }
                else if (sampletype == 'f')
                {
                  //ms_log (0, "%10.8g  ", *(float *)sptr);
                  data[idx] = (double)(*(float *)sptr);
                }
                else if (sampletype == 'd')
                {
                  //ms_log (0, "%10.10g  ", *(double *)sptr);
                  data[idx] = (double)(*(double *)sptr);
                }

                //printf("data[%zu]: %10.10g  ", idx, data[idx]);

                idx++;
              }
              //ms_log (0, "\n");
            }
          }
        }
      }

      seg = seg->next;
    }

    /* print the data samples of every trace */
    printf ("data samples of this trace: %" PRId64 "\n", dataSize);
    /* Calculate the RMS */
    printf ("RMS of this trace: %lf\n", calculateSD (data, dataSize));
    printf ("\n");

    /* clean up the data array in the end of every trace */
    free (data);

    tid = tid->next;
  }

  /* Make sure everything is cleaned up */
  if (mstl)
    mstl3_free (&mstl, 0);
  if (selections)
    ms3_freeselections (selections);

  return 0;
}
