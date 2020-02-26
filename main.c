#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "libmseed.h"

#include "standard_deviation.h"

#define SECONDSINDAY 86400
#define SECONDSINHOUR 3600
#define SECONDSINMINUTE 60
static nstime_t NSECS = 1000000000;

/*
static void
write2RMS (FILE *file, char *timeStampStr, double mean, double SD)
{
  fprintf (file, "%s,%.2lf,%.2lf\n", timeStampStr, mean, SD);
}
*/

static void
write2RMS (FILE *file, nstime_t timeStamp, double mean, double SD)
{
  int timeStampInSecond = timeStamp / NSECS;
  fprintf (file, "%d,%.2lf,%.2lf\r\n", timeStampInSecond, mean, SD);
}

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
  FILE *fptrRMS;
  FILE *fptrJSON;
  char *outputFileRMS;
  char *outputFileJSON;
  const char *RMSExtension  = ".rms";
  const char *JSONExtension = ".json";

  /* Buffers for storing source id, network, station, location and channel */
  //char sid[LM_SIDLEN];
  char network[11];
  char station[11];
  char location[11];
  char channel[31];

  /* Simplistic argument parsing */
  if (argc != 4)
  {
    ms_log (2, "Usage: ./ms2rms <mseedfile> <time window size> <window overlap>\n");
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
  printf ("temp str size: %ld content: %s\n", tempLen, temp);
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

  /* Open the output files */
  fptrRMS  = fopen (outputFileRMS, "w");
  fptrJSON = fopen (outputFileJSON, "w");
  if (fptrRMS == NULL)
  {
    printf ("Error opening file %s\n", outputFileRMS);
    return -1;
  }
  else if (fptrJSON == NULL)
  {
    printf ("Error opening file %s\n", outputFileJSON);
    return -1;
  }

  /* get the end time of the earliest record */
  MS3Record *msr = 0;
  ms3_readmsr (&msr, mseedfile, NULL, NULL, flags, verbose);
  uint16_t year, yday;
  uint8_t hour, min, sec;
  uint32_t nsec;
  ms_nstime2time (msr3_endtime (msr), &year, &yday, &hour, &min, &sec, &nsec);
  printf ("end time of year and yday of the earliest record: %" PRId16 " %" PRId16 "\n", year, yday);

  /* Parse network, station, location and channel from SID */
  rv = ms_sid2nslc (msr->sid, network, station, location, channel);
  if (rv)
  {
    printf ("Error returned ms_sid2nslc()\n");
    return -1;
  }
  /* Write network, station, location and channel to output JSON file */
  fprintf (fptrJSON, "{\"network\":\"%s\", \
                          \"station\":\"%s\", \
                          \"location\":\"%s\", \
                          \"channel\":\"%s\", \
                          \"data\":[",
           network, station, location, channel);
  if (msr)
    msr3_free (&msr);

  /* Loop over the selected segments */
  nstime_t starttime = ms_time2nstime (year, yday, 0, 0, 0, 0);
  nstime_t endtime   = starttime + (nstime_t) (windowSize * NSECS);
  nstime_t timeStampFirst;
  int i;
  for (i = 0; i < segments; i++)
  {
    printf ("index: %d\n", i);
    /* Record the time stamp of each time interval */
    nstime_t timeStamp;

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

      /* Create time stamp string */
      timeStamp = tid->earliest + (tid->latest - tid->earliest) / 2;
      /* Record the time of the first segment */
      if (i == 0)
      {
        timeStampFirst = timeStamp;
      }
      if (!ms_nstime2timestr (timeStamp,
                              timeStampStr, ISOMONTHDAY, NONE))
      {
        ms_log (2, "Cannot create time stamp strings\n");
        return -1;
      }
      ms_log (0, "Time stamp: %s\n", timeStampStr);

      uint64_t total = 0;
      seg            = tid->first;
      while (seg)
      {
        total += seg->samplecnt;
        seg = seg->next;
      }
      printf ("estimated samples of this trace: %" PRId64 "\n", total);
      //total = 0;

      seg = tid->first;

      /* malloc the data array */
      dataSize = total;
      data     = (double *)malloc (sizeof (double) * dataSize);
      if (data == NULL)
      {
        printf ("something wrong when malloc data array\n");
        exit (-1);
      }
      int64_t index = 0;
      total         = 0;

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
                    data[index] = (double)(*(int32_t *)sptr);
                  }
                  else if (sampletype == 'f')
                  {
                    data[index] = (double)(*(float *)sptr);
                  }
                  else if (sampletype == 'd')
                  {
                    data[index] = (double)(*(double *)sptr);
                  }

                  idx++;
                  index++;
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
      printf ("data samples of this trace: %" PRId64 " index: %" PRId64 "\n", dataSize, index);
      /* Calculate the mean and standard deviation */
      double mean, SD;
      getMeanAndSD (data, dataSize, &mean, &SD);
      printf ("mean: %.2lf standard deviation: %.2lf\n", mean, SD);
      printf ("\n");

      /* Record the header information into .rms file */
      if (i == 0)
      {
        char temp[30];
        if (!ms_nstime2timestr (timeStamp, temp, SEEDORDINAL, NONE))
        {
          ms_log (2, "Cannot create time stamp strings\n");
          return -1;
        }
        fprintf (fptrRMS, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\r\n",
                 temp, station, network, channel, location);
      }

      /* Output timestamp, mean and standard deviation to output files */
      write2RMS (fptrRMS, timeStamp - timeStampFirst, mean, SD);
      //write2RMS(fptrRMS, timeStampStr, mean, SD);
      if (i == segments - 1)
      {
        fprintf (fptrJSON, "{\"timestamp\":\"%s\",\"mean\":%.2lf,\"rms\":%.2lf}",
                 timeStampStr, mean, SD);
      }
      else
      {
        fprintf (fptrJSON, "{\"timestamp\":\"%s\",\"mean\":%.2lf,\"rms\":%.2lf},",
                 timeStampStr, mean, SD);
      }

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

  fprintf (fptrJSON, "]}");

  /* Close the output files */
  fclose (fptrRMS);
  fclose (fptrJSON);

  return 0;
}
