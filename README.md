# ms2rms
Calculate standard deviation given a time interval of each trace.

# How to Compile
1. Clone this repo by THIS COMMAND: `git clone https://github.com/Cuda-Chen/ms2rms.git --recursive`
2. Type `make` to compile.

# Usage
```
$ ./ms2rms <mseedfile> <time window size> <window overlap>
```
Where:
- `time window size`: measured in seconds. It should always bigger than `0`.
- `window overlap`: measured in percentage. If should always smaller than `100`.

# Output Format
## .rms
```
<time stamp of the first window>,<station>,<network>,<channel>,<location>,<CR><LF>
<time difference between this window to the first window>,<mean>,<SD>,<min>,<max>,<minDemean>,<maxDemean>,<CR><LF>
<time difference between this window to the first window>,<mean>,<SD>,<min>,<max>,<minDemean>,<maxDemean>,<CR><LF>
...
```

# Note
It `libmseed` directory is empty run this command in project's root directory:
`git submodule update --init`
