#Linux Makefile

CC = clang
CFLAGS = -O3 -ansi
DEBUGFLAGS = -D__DEBUG__ -g
LINKFLAGS = -lm
OBJS = lpc.o rice.o wavwriter.o main.o

all: $(OBJS)
	$(CC) -o test_lpc $(OBJS) $(LINKFLAGS)

ricetest: rice.o ricetest.o
	$(CC) -o test_rice rice.o ricetest.o $(LINKFLAGS)

debug: main.c lpc.c lpc.h wavwriter.c wavwriter.h rice.c rice.h
	$(CC) $(DEBUGFLAGS) main.c lpc.c lpc.h rice.c rice.h wavwriter.c wavwriter.h $(LINKFLAGS)

clean:
	rm -f -v $(OBJS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

lpc.o: lpc.c lpc.h
	$(CC) -c lpc.c lpc.h $(CFLAGS)

rice.o: rice.c rice.h
	$(CC) -c rice.c rice.h $(CFLAGS)

ricetest.o: ricetest.c
	$(CC) -c ricetest.c $(CFLAGS)

wavwriter.o: wavwriter.c wavwriter.h
	$(CC) -c wavwriter.c wavwriter.h $(CFLAGS)
