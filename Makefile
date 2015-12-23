CC = gcc
LINKFLAGS = -lm
INCDIR = -I include/
CFLAGS = -O3 -std=c99
DEBUGFLAGS = -g -std=c99

all: encoder decoder

clean:
	rm -v *.o

expt_enc: core/encode_expt.c core/rice.c core/lpc.c core/wavutils.c
	$(CC) $(INCDIR) core/encode_expt.c core/rice.c core/lpc.c core/wavutils.c -o expt_encode $(CFLAGS) $(LINKFLAGS)

encoder: encode.o rice.o lpc.o wavutils.o id3v1_1.o
	$(CC) $(INCDIR) encode.o rice.o lpc.o wavutils.o id3v1_1.o -o encode $(LINKFLAGS)

decoder: decode.o rice.o lpc.o wavutils.o
	$(CC) $(INCDIR) decode.o rice.o lpc.o wavutils.o -o decode $(LINKFLAGS)

selaplay_pulse: p_selaplay.o rice.o lpc.o packetqueue.o pulse_output.o
	$(CC) $(INCDIR) -o selaplay selaplay.o rice.o lpc.o packetqueue.o pulse_output.o -lm -lpthread -lpulse-simple -lpulse
	
selaplay_ao: a_selaplay.o rice.o lpc.o packetqueue.o ao_output.o
	$(CC) $(INCDIR) -o selaplay selaplay.o rice.o lpc.o packetqueue.o ao_output.o -lm -lpthread -lao

selaplay_debug: player/selaplay.c core/rice.c core/lpc.c player/packetqueue.c player/pulse_output.c
	$(CC) $(INCDIR) -D__PULSE__ player/selaplay.c core/rice.c core/lpc.c player/packetqueue.c player/pulse_output.c $(DEBUGFLAGS) -lm -lpulse -lpulse-simple -lpthread

d_encoder: core/encode.c core/rice.c core/lpc.c core/wavutils.c core/id3v1_1.c
	$(CC) $(INCDIR) core/encode.c core/rice.c core/lpc.c core/wavutils.c core/id3v1_1.c $(DEBUGFLAGS) $(LINKFLAGS)

d_decoder: core/decode.c core/rice.c core/lpc.c core/wavutils.c
	$(CC) $(INCDIR) core/decode.c core/rice.c core/lpc.c core/wavutils.c $(DEBUGFLAGS) $(LINKFLAGS)

ricetest: tests/ricetest.c core/rice.c
	$(CC) $(INCDIR) -o ricetest tests/ricetest.c core/rice.c $(CFLAGS) $(LINKFLAGS)

lpctest: tests/lpctest.c core/lpc.c
	$(CC) $(INCDIR) -o lpctest tests/lpctest.c core/lpc.c $(CFLAGS) $(LINKFLAGS)

wavdiff: wavdiff.o wavutils.o
	$(CC) $(INCDIR) -o wavdiff wavdiff.o wavutils.o

rice.o: core/rice.c
	$(CC) $(INCDIR) -c core/rice.c $(CFLAGS)

lpc.o: core/lpc.c
	$(CC) $(INCDIR) -c core/lpc.c $(CFLAGS)

packetqueue.o: player/packetqueue.c
	$(CC) $(INCDIR) -c player/packetqueue.c $(CFLAGS)

pulse_output.o: player/pulse_output.c
	$(CC) $(INCDIR) -c player/pulse_output.c $(CFLAGS)
	
ao_output.o: player/ao_output.c
	$(CC) $(INCDIR) -c player/ao_output.c $(CFLAGS)

p_selaplay.o: player/selaplay.c
	$(CC) $(INCDIR) -c player/selaplay.c -D__PULSE__ $(CFLAGS)
	
a_selaplay.o: player/selaplay.c
	$(CC) $(INCDIR) -c player/selaplay.c -D__AO__ $(CFLAGS)

wavutils.o: core/wavutils.c
	$(CC) $(INCDIR) -c core/wavutils.c $(CFLAGS)

encode.o: core/encode.c
	$(CC) $(INCDIR) -c core/encode.c $(CFLAGS)

decode.o: core/decode.c
	$(CC) $(INCDIR) -c core/decode.c $(CFLAGS)

id3v1_1.o: core/id3v1_1.c
	$(CC) $(INCDIR) -c core/id3v1_1.c $(CFLAGS)

wavdiff.o: utils/wavdiff.c
	$(CC) $(INCDIR) -c utils/wavdiff.c $(CFLAGS)
