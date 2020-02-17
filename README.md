# ms2rms
Calculate standard deviation given a time interval of each trace.

# How to Compile
1. Install [libmseed](https://github.com/iris-edu/libmseed).

2. Type `make` to compile.

# Usage
```
$ ./ms2rms <mseedfile> <time window size> <window overlap>
```
Where:
- `time window size`: measured in seconds. It should always bigger than `0`.
- `window overlap`: measured in percentage. If should always smaller than `100`.
