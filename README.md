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

# Note
`git submodule update --init`
