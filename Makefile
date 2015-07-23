CC = clang
LINKFLAGS = -lm
CFLAGS = -O3 -std=c99
DEBUGFLAGS = -g -std=c99

all: encoder decoder

clean:
	rm -v *.o

encoder: encode.o rice.o lpc.o
	$(CC) encode.o rice.o lpc.o -o encode $(LINKFLAGS)

decoder: decode.o rice.o lpc.o wavwriter.o
	$(CC) decode.o rice.o lpc.o wavwriter.o -o decode $(LINKFLAGS)

selaplay: selaplay.o rice.o lpc.o packetqueue.o pulse_output.o
	$(CC) -o selaplay selaplay.o rice.o lpc.o packetqueue.o pulse_output.o -lm -lpthread `pkg-config --libs libpulse-simple`

d_encoder: encode.c rice.c lpc.c
	$(CC) encode.c rice.c lpc.c $(DEBUGFLAGS) $(LINKFLAGS)

d_decoder: decode.c rice.c lpc.c wavwriter.c
	$(CC) decode.c rice.c lpc.c wavwriter.c $(DEBUGFLAGS) $(LINKFLAGS)

ricetest: ricetest.c rice.c
	$(CC) ricetest.c rice.c $(DEBUGFLAGS) $(LINKFLAGS)

lpctest: lpctest.c lpc.c
	$(CC) -o lpctest lpctest.c lpc.c $(CFLAGS) $(LINKFLAGS)

rice.o: rice.c rice.h
	$(CC) -c rice.c rice.h $(CFLAGS)

lpc.o: lpc.c lpc.h
	$(CC) -c lpc.c lpc.h $(CFLAGS)

packetqueue.o: packetqueue.c packetqueue.h
	$(CC) -c packetqueue.c packetqueue.h $(CFLAGS)

pulse_output.o: pulse_output.c pulse_output.h
	$(CC) -c pulse_output.c pulse_output.h $(CFLAGS)

selaplay.o: selaplay.c
	$(CC) -c selaplay.c $(CFLAGS)

wavwriter.o: wavwriter.c wavwriter.h
	$(CC) -c wavwriter.c wavwriter.h $(CFLAGS)

encode.o: encode.c
	$(CC) -c encode.c $(CFLAGS)

decode.o: decode.c
	$(CC) -c decode.c $(CFLAGS)
