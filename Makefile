CC = gcc
CFLAGS += -Ifmgen -Wall -pipe -O2 -fno-math-errno -fno-omit-frame-pointer -fno-asynchronous-unwind-tables

# Uncomment exactly one of the following:
+AUDIO_DRV = oss_audio.o
# AUDIO_DRV = alsa_pcm_api.o
# CFLAGS += -DUSE_ALSA=1
# AUDIO_DRV = wave_out.o

all: pmdwin
%.o: %.c
   $(CC) $(CFLAGS) -c $<

clean:
    rm *.o fmgen/*.o libfmgen.a pmdwin

pmdwin: pmd_play.o pmdwin.o table.o getopt.o lfg.o $(AUDIO_DRV) fmgen.o
   $(CC) -o $@ $^


libfmgen.a: fmgen/e_expf.o fmgen/opna.o fmgen/psg.o fmgen/rhythmdata.o
   $(AR) rc $@ $^



