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
  char *mseedfile = NULL;
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

  int windowSize;
  int windowOverlap;

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
  while (ssc)
  {
    l    = strlen (ssc) + 1;
    temp = &temp[strlen (temp) - l + 2];
    ssc  = strstr (temp, "/");
  }
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

  /* Calculate how many segments of this routine */
  int nextTimeStamp = windowSize - (windowSize * windowOverlap / 100);
  int segments      = SECONDSINDAY / nextTimeStamp;
  printf ("num of segments: %d\n", segments);
  nstime_t nextTimeStamp_ns = nextTimeStamp * NSECS;
  char timeStampStr[30];

  /* get the end time of the earliest record */
  MS3Record *msr = 0;
  ms3_readmsr (&msr, mseedfile, NULL, NULL, flags, verbose);
  uint16_t year, yday;
  uint8_t hour, min, sec;
  uint32_t nsec;
  ms_nstime2time (msr3_endtime (msr), &year, &yday, &hour, &min, &sec, &nsec);
  printf ("end time of year and yday of the earliest record: %" PRId16 " %" PRId16 "\n", year, yday);
  if (msr)
    msr3_free (&msr);

  /* Loop over the selected segments */
  nstime_t starttime = ms_time2nstime (year, yday, 0, 0, 0, 0);
  nstime_t endtime   = starttime + (nstime_t) (windowSize * NSECS);
  int i;
  for (i = 0; i < segments; i++)
  {
    printf ("index: %d\n", i);

    MS3TraceList *mstl        = NULL;
    MS3TraceID *tid           = NULL;
    MS3TraceSeg *seg          = NULL;
    MS3Selections *selections = NULL;

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

    if (!ms_nstime2timestr (starttime + (endtime - starttime) / 2,
                            timeStampStr, ISOMONTHDAY, NANO))
    {
      ms_log (2, "Cannot create time stamp strings\n");
      return -1;
    }
    printf ("Time stamp: %s\n", timeStampStr);

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

      if (!ms_nstime2timestr (tid->earliest, starttimestr, ISOMONTHDAY, NANO_MICRO_NONE) ||
          !ms_nstime2timestr (tid->latest, endtimestr, ISOMONTHDAY, NANO_MICRO_NONE))
      {
        ms_log (2, "Cannot create time strings\n");
        starttimestr[0] = endtimestr[0] = '\0';
      }

      ms_log (0, "TraceID for %s (%d), earliest: %s, latest: %s, segments: %u\n",
              tid->sid, tid->pubversion, starttimestr, endtimestr, tid->numsegments);
      uint64_t total = 0;
      seg            = tid->first;
      while (seg)
      {
        total += seg->numsamples;
        seg = seg->next;
      }
      printf ("estimated samples of this trace: %" PRId64 "\n", total);

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
                    data[idx] = (double)(*(int32_t *)sptr);
                  }
                  else if (sampletype == 'f')
                  {
                    data[idx] = (double)(*(float *)sptr);
                  }
                  else if (sampletype == 'd')
                  {
                    data[idx] = (double)(*(double *)sptr);
                  }

                  //printf("data[%zu]: %10.10g  ", idx, data[idx]);

                  idx++;
                }
              }
            }
          }
        }

        total += seg->numsamples;
        seg = seg->next;
      }

      printf ("total samples of this trace: %" PRId64 "\n", total);
      /* print the data samples of every trace */
      printf ("data samples of this trace: %" PRId64 " index: %" PRId64 "\n", dataSize, idx);
      /* Calculate the RMS */
      printf ("RMS of this trace: %lf\n", calculateSD (data, dataSize));
      printf ("\n");

      /* clean up the data array in the end of every trace */
      free (data);

      tid = tid->next;
    }

    starttime += nextTimeStamp_ns;
    endtime += nextTimeStamp_ns;

    /* Make sure everything is cleaned up */
    if (mstl)
      mstl3_free (&mstl, 0);
    if (selections)
      ms3_freeselections (selections);
  }

  return 0;
}
