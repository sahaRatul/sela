# SELA
### SimplE Lossless Audio
A simplified lossless audio codec written for final year project.

### Block Diagrams
![Encoder](https://cloud.githubusercontent.com/assets/12273725/8868411/c24585e6-31f5-11e5-937a-e3c11c632704.png)
![Decoder](https://cloud.githubusercontent.com/assets/12273725/8868418/cbb6a1dc-31f5-11e5-91f6-8290766baa34.png)

### Main Components
- Linear prediction filter (core/lpc.c)
- Golomb-Rice compressor and decompressor (core/rice.c)
- apev2 metadata reader/writer (apev2.c)
- Encoder (core/encoder.c)
- Decoder (core/decoder.c)
- Command line player (player/selaplay.c)
- Matlab implementation of Linear prediction filter can be found in 'tests/matlab-tests' folder

### Build requirements
- gcc (You can use clang if you modify the ```CC``` variable in the Makefile)
- GNU make
- Standard math library for building the encoder and decoder.
- POSIX threading and pulseaudio/libao developement libraries for building the command line player.
Note : On Windows you will need cygwin to build the command line player. Otherwise MinGW is sufficient.

### Build instructions
- cd to the directory
- type ```make all``` to build the encoder & decoder
- type ```make selaplay_ao``` or ```make selaplay_pulse``` to build the player using either libao/pulseaudio libraries
- type ```make lpctest && make ricetest && make apev2test``` to build the tests
- type ```make wavdiff``` to build the diff utility for .wav files

### References
- Linear Prediction
  - [Wikipedia](https://en.wikipedia.org/wiki/Linear_prediction)
  - [Digital Signal Processing by John G. Proakis & Dimitris G. Monolakis](http://www.amazon.com/Digital-Signal-Processing-4th-Edition/dp/0131873741)
  - [A detailed pdf](http://www.ece.ucsb.edu/Faculty/Rabiner/ece259/digital%20speech%20processing%20course/lectures_new/Lecture%2013_winter_2012_6tp.pdf)
- Golomb-Rice lossless compression algorithm
  - [Wikipedia](https://en.wikipedia.org/wiki/Golomb_coding)
  - [Here is an implementation](http://michael.dipperstein.com/rice/index.html)
- [FLAC overview](https://xiph.org/flac/documentation_format_overview.html)
- [Paper on shorten, the original open source lossless codec](ftp://svr-ftp.eng.cam.ac.uk/pub/reports/robinson_tr156.ps.Z)
- ISO/IEC 14496 Part 3, Subpart 11 (Audio Lossless Coding)

### License : MIT
