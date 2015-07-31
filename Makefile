CC = clang
LINKFLAGS = -lm
CFLAGS = -O3 -std=c99
DEBUGFLAGS = -g -std=c99

all: encoder decoder

clean:
	rm -v *.o

encoder: encode.o rice.o lpc.o wavutils.o
	$(CC) encode.o rice.o lpc.o wavutils.o -o encode $(LINKFLAGS)

decoder: decode.o rice.o lpc.o wavutils.o
	$(CC) decode.o rice.o lpc.o wavutils.o -o decode $(LINKFLAGS)

selaplay_pulse: p_selaplay.o rice.o lpc.o packetqueue.o pulse_output.o
	$(CC) -o selaplay selaplay.o rice.o lpc.o packetqueue.o pulse_output.o -lm -lpthread -lpulse-simple -lpulse
	
selaplay_ao: a_selaplay.o rice.o lpc.o packetqueue.o ao_output.o
	$(CC) -o selaplay selaplay.o rice.o lpc.o packetqueue.o ao_output.o -lm -lpthread -lao

d_encoder: encode.c rice.c lpc.c wavutils.c
	$(CC) encode.c rice.c lpc.c wavutils.c $(DEBUGFLAGS) $(LINKFLAGS)

d_decoder: decode.c rice.c lpc.c wavutils.c
	$(CC) decode.c rice.c lpc.c wavutils.c $(DEBUGFLAGS) $(LINKFLAGS)

ricetest: ricetest.c rice.c
	$(CC) -o ricetest ricetest.c rice.c $(CFLAGS) $(LINKFLAGS)

lpctest: lpctest.c lpc.c
	$(CC) -o lpctest lpctest.c lpc.c $(CFLAGS) $(LINKFLAGS)

rice.o: rice.c rice.h
	$(CC) -c rice.c $(CFLAGS)

lpc.o: lpc.c lpc.h
	$(CC) -c lpc.c $(CFLAGS)

packetqueue.o: packetqueue.c packetqueue.h
	$(CC) -c packetqueue.c $(CFLAGS)

pulse_output.o: pulse_output.c pulse_output.h
	$(CC) -c pulse_output.c $(CFLAGS)
	
ao_output.o: ao_output.c ao_output.h
	$(CC) -c ao_output.c $(CFLAGS)

p_selaplay.o: selaplay.c
	$(CC) -c selaplay.c -D__PULSE__ $(CFLAGS)
	
a_selaplay.o: selaplay.c
	$(CC) -c selaplay.c -D__AO__ $(CFLAGS)

wavutils.o: wavutils.c wavutils.h
	$(CC) -c wavutils.c $(CFLAGS)

encode.o: encode.c
	$(CC) -c encode.c $(CFLAGS)

decode.o: decode.c
	$(CC) -c decode.c $(CFLAGS)
