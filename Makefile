CC = gcc
CFLAGS += -Ifmgen -Wall -pipe -O2 -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer -fno-asynchronous-unwind-tables -fwrapv -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

all: pmdwin
%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o fmgen/*.o libfmgen.a pmdwin

pmdwin: pmd_play.o pmdwin.o md5.o fmgen/libfmgen.a libaout.a
	$(CC) -o $@ $^

fmgen/libfmgen.a:
	make -C fmgen

