# SELA
###SimplE Lossless Audio
(Part of my ongoing project about writing a lossless audio encoder)

###Block Diagrams
![Encoder](https://cloud.githubusercontent.com/assets/12273725/8868411/c24585e6-31f5-11e5-937a-e3c11c632704.png)
![Decoder](https://cloud.githubusercontent.com/assets/12273725/8868418/cbb6a1dc-31f5-11e5-91f6-8290766baa34.png)

###Main Components
#####Linear prediction filter (lpc.c)
#####Golomb Rice compressor and decompressor (rice.c)
#####Encoder (encoder.c)
#####Decoder (decoder.c)
#####Command line player (selaplay.c)

###Build requirements
-clang (gcc also works but you will have to tweak the Makefile).
-Standard math library for building the encoder and decoder.
-POSIX threading and pulseaudio developement libraries for building the command line player.
