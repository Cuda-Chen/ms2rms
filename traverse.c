#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "libmseed.h"

#include "min_max.h"
#include "standard_deviation.h"

#define SECONDSINDAY 86400
#define SECONDSINHOUR 3600
#define SECONDSINMINUTE 60
static nstime_t NSECS = 1000000000;

static void
write2RMS (FILE *file, nstime_t timeStamp, double mean, double SD,
           double min, double max, double minDemean, double maxDemean)
{
  int timeStampInSecond = timeStamp / NSECS;
  fprintf (file, "%d,%.2lf,%.2lf,%.2lf,%.2lf,%.2lf,%.2lf\r\n", timeStampInSecond, mean, SD,
           min, max, minDemean, maxDemean);
}

int traverseTimeWindow(const char *mseedfile, const char *outputFileRMS, const char *outputFileJSON,
                        int windowSize, int windowOverlap)
{
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

  FILE *fptrRMS;
  FILE *fptrJSON;

  /* Buffers for storing source id, network, station, location and channel */
  //char sid[LM_SIDLEN];
  char network[11];
  char station[11];
  char location[11];
  char channel[31];


  /* Set bit flag to validate CRC */
  flags |= MSF_VALIDATECRC;

  /* Set bit flag to build a record list */
  flags |= MSF_RECORDLIST;

  /* Calculate how many segments of this routine */
  int nextTimeStamp = windowSize - (windowSize * windowOverlap / 100);
  int segments      = SECONDSINDAY / nextTimeStamp;
#ifdef DEBUG
  printf ("num of segments: %d\n", segments);
#endif
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
#ifdef DEBUG
  printf ("end time of year and yday of the earliest record: %" PRId16 " %" PRId16 "\n", year, yday);
#endif

  /* Parse network, station, location and channel from SID */
  rv = ms_sid2nslc (msr->sid, network, station, location, channel);
  if (rv)
  {
    printf ("Error returned ms_sid2nslc()\n");
    return -1;
  }
  if (msr)
    msr3_free (&msr);

  /* Loop over the selected segments */
  nstime_t starttime = ms_time2nstime (year, yday, 0, 0, 0, 0);
  nstime_t endtime   = starttime + (nstime_t) (windowSize * NSECS);
  nstime_t timeStampFirst;
  int i, counter = 0;
  for (i = 0; i < segments; i++)
  {
#ifdef DEBUG
    printf ("index: %d\n", i);
#endif
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

#ifdef DEBUG
    ms3_printselections (&testselection);
#endif

    /* Read all miniSEED into a trace list, limiting to time selections */
    rv = ms3_readtracelist_selection (&mstl, mseedfile, NULL,
                                      &testselection, 0, flags, verbose);

    /* If there are some records in this time window */
    if (rv == MS_NOERROR)
    {
      counter++;
    }
    else if (rv == MS_NOTSEED)
    {
#ifdef DEBUG
      ms_log (1, "Seems this interval has no data or there is no miniSEED data\n");
#endif
      starttime += nextTimeStamp_ns;
      endtime += nextTimeStamp_ns;

      /* Make sure everything is cleaned up */
      if (mstl)
        mstl3_free (&mstl, 0);
      if (selections)
        ms3_freeselections (selections);

      continue;
    }
    else
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
      /* Record the sampling rate of each trace */
      double samplingRate;

      if (!ms_nstime2timestr (tid->earliest, starttimestr, ISOMONTHDAY, NANO_MICRO_NONE) ||
          !ms_nstime2timestr (tid->latest, endtimestr, ISOMONTHDAY, NANO_MICRO_NONE))
      {
        ms_log (2, "Cannot create time strings\n");
        starttimestr[0] = endtimestr[0] = '\0';
      }

#ifdef DEBUG
      ms_log (0, "TraceID for %s (%d), earliest: %s, latest: %s, segments: %u\n",
              tid->sid, tid->pubversion, starttimestr, endtimestr, tid->numsegments);
#endif

      /* Create time stamp string */
      timeStamp = tid->earliest + (tid->latest - tid->earliest) / 2;
      /* Record the time of the first segment */
      if (counter == 1)
      {
        timeStampFirst = timeStamp;
      }
      if (!ms_nstime2timestr (timeStamp,
                              timeStampStr, ISOMONTHDAY, NONE))
      {
        ms_log (2, "Cannot create time stamp strings\n");
        return -1;
      }
#ifdef DEBUG
      ms_log (0, "Time stamp: %s\n", timeStampStr);
#endif

      /* Record the header information into .rms and .json file */
      if (counter == 1)
      {
        char temp[30];
        if (!ms_nstime2timestr (timeStamp, temp, SEEDORDINAL, NONE))
        {
          ms_log (2, "Cannot create time stamp strings\n");
          return -1;
        }
        fprintf (fptrRMS, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\r\n",
                 temp, station, network, channel, location);

        fprintf (fptrJSON, "{\"network\":\"%s\",\"station\":\"%s\",\"location\":\"%s\",\"channel\":\"%s\",\"data\":[",
                 network, station, location, channel);
      }

      uint64_t total = 0;
      seg            = tid->first;
      samplingRate = seg->samprate;
      while (seg)
      {
        total += seg->samplecnt;
        seg = seg->next;
      }
#ifdef DEBUG
      printf ("estimated samples of this trace: %" PRId64 "\n", total);
#endif

      seg = tid->first;

      /* malloc the data array */
      dataSize = total;
      data     = (double *)malloc (sizeof (double) * dataSize);
      if (data == NULL)
      {
        printf ("something wrong when malloc data array\n");
        exit (-1);
      }

      /* If the duration of this trace is smaller than 20 seconds */
      if (total * samplingRate < 20)
      {
        printf ("Number of data of this trace is smaller than 20 * %lf\n", samplingRate);
        /* ignore this trace */
        counter--;
        free (data);
        tid = tid->next;
        continue;
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

#ifdef DEBUG
        ms_log (0, "  Segment %s - %s, samples: %" PRId64 ", sample rate: %g\n",
                starttimestr, endtimestr, seg->samplecnt, seg->samprate);
#endif

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

#ifdef DEBUG
      printf ("total samples of this trace: %" PRId64 "\n", total);
      /* print the data samples of every trace */
      printf ("data samples of this trace: %" PRId64 " index: %" PRId64 "\n", dataSize, index);
#endif
      /* If total < samplingRate, ignore this trace */
/*
      if (total < 20 * samplingRate)
      {
        printf ("Number of data of this trace is smaller than 20 * %lf\n", samplingRate);
        
        free (data);
        tid = tid->next;
        continue;
      }*/

      /* Calculate the mean and standard deviation */
      double mean, SD;
      getMeanAndSD (data, dataSize, &mean, &SD);
#ifdef DEBUG
      printf ("mean: %.2lf standard deviation: %.2lf\n", mean, SD);
      printf ("\n");
#endif
      /* Calculate the min and max with all and demain */
      double min, max, minDemean, maxDemean;
      getMinMaxAndDemean (data, dataSize, &min, &max,
                          &minDemean, &maxDemean, mean);

      /* Output timestamp, mean and standard deviation to output files */
      write2RMS (fptrRMS, timeStamp - timeStampFirst, mean, SD,
                 min, max, minDemean, maxDemean);
      //write2RMS(fptrRMS, timeStampStr, mean, SD);
      if (i == segments - 1)
      {
        fprintf (fptrJSON, "{\"timestamp\":\"%s\",\"mean\":%.2lf,\"rms\":%.2lf}",
                 timeStampStr, mean, SD);
        fprintf (fptrJSON, "]}");
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

  /* Close the output files */
  fclose (fptrRMS);
  fclose (fptrJSON);

  return 0;
}
