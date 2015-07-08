CC = gcc
CFLAGS = -O3
DEBUGFLAGS = -D__DEBUG__ -g
LINKFLAGS = -lm

all: main.o lpc.o wavwriter.o
	$(CC) main.o lpc.o wavwriter.o -o lpc $(LINKFLAGS)

debug: main.c lpc.c lpc.h wavwriter.c wavwriter.h
	$(CC) $(DEBUGFLAGS) main.c lpc.c lpc.h wavwriter.c wavwriter.h -lm

clean:
	rm *.o

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

lpc.o: lpc.c lpc.h
	$(CC) -c lpc.c lpc.h $(CFLAGS)

wavwriter.o: wavwriter.c wavwriter.h
	$(CC) -c wavwriter.c wavwriter.h $(CFLAGS)
