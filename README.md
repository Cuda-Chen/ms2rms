# ms2rms
Calculate standard deviation given a time interval of each trace.

# How to Compile
1. Clone this repo by THIS COMMAND: `git clone https://github.com/Cuda-Chen/ms2rms.git`
2. Type `make` to compile.

## Dependencies
- [libmseed](https://github.com/iris-edu/libmseed)

# Usage
```
$ ./ms2rms [mseedfile] [time window size] [window overlap]
```
Where:
- `time window size`: measured in seconds. It should always bigger than `0`.
- `window overlap`: measured in percentage. It should always smaller than `100`.

# Output Format
## .rms
```
<time stamp of the first window>,<station>,<network>,<channel>,<location>,<CR><LF>
<time difference between this window to the first window>,<mean>,<SD>,<min>,<max>,<minDemean>,<maxDemean>,<CR><LF>
<time difference between this window to the first window>,<mean>,<SD>,<min>,<max>,<minDemean>,<maxDemean>,<CR><LF>
...
```

# Note
- This program can ONLY accept single channel record.
Multiple channels result in incorrect output.
- The `rms` and `mean` value are rounded to hundrendth place.
