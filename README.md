# SELA
###SimplE Lossless Audio
(Part of my ongoing project about writing a lossless audio encoder)

###Main Components
#####*Linear prediction filter (lpc.c)
#####*Golomb Rice compressor and decompressor (rice.c)
#####*Encoder (encoder.c)
#####*Decoder (decoder.c)
#####*Command line player (selaplay.c)

###Build requirements
clang (gcc also works but you will have to tweak the Makefile).
Standard math library for building the encoder and decoder.
POSIX threading and pulseaudio developement libraries for building the command line player.
