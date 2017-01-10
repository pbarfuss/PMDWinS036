CC = gcc
CFLAGS += -Wall -pipe -O2 -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer -fno-asynchronous-unwind-tables -fwrapv

all: libaout.a
%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o libaout.a

libaout.a: audio_out.o oss_audio_open.o alsa_audio_open.o alsa_pcm_api.o
	$(AR) rc $@ $^
	ranlib $@ $^

