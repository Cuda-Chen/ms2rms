#ifndef TRAVERSE_H
#define TRAVERSE_H

int traverseTimeWindow (const char *mseedfile, const char *outputFileRMS, const char *outputFileJSON,
                        int windowSize, int windowOverlap, int outputFormatFlag);
int traverseTimeWindowLimited (const char *mseedfile, const char *outputFileRMS, const char *outputFileJSON,
                               int windowSize, int windowOverlap);

#endif
