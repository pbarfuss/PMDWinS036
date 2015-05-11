CC = gcc
CFLAGS += -Wall -pipe -O2 -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer -fno-asynchronous-unwind-tables -fwrapv

all: libfmgen.a
%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o libfmgen.a

libfmgen.a: e_expf.o opna.o psg.o rhythmdata.o
	$(AR) rc $@ $^

