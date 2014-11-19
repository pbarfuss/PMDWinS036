#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "audio_out.h"

void (*audio_write_flt)(void *snd_fd, float *buf, uint32_t bufsize);
void (*audio_write_s16)(void *snd_fd, int16_t *buf, uint32_t bufsize);
void (*audio_close)(void *snd_fd);

void *audio_open(uint8_t devtype, devpath_t *devpath, uint32_t *samplerate)
{
    void *ao = NULL;
#ifdef __linux__
    if (devtype == 'a') goto try_alsa;
#endif
#if !defined(_WIN32) && !defined(_WIN64)
    if (devtype == 'o') goto try_oss;
#endif
#if (defined(_WIN32) || defined(_WIN64))
    if ((devtype == 's') || (devtype == 'S')) goto try_nt_wdmks;
#ifdef ENABLE_WINMM
    if (devtype == 'm') goto try_winmm;
#endif
#endif
#ifdef __linux__
    if (!ao) {
try_alsa:
        ao = alsa_audio_open(devpath, samplerate, NULL);
        if (ao) {
            audio_write_flt = alsa_audio_write_flt;
            audio_write_s16 = alsa_audio_write_s16;
            audio_close = alsa_audio_close;
            return ao;
        }
    }
#endif
#if !defined(_WIN32) && !defined(_WIN64)
    if (!ao) {
try_oss:
        ao = oss_audio_open(devpath, samplerate);
        if (ao) {
            audio_write_flt = oss_audio_write_flt;
            audio_write_s16 = oss_audio_write_s16;
            audio_close = oss_audio_close;
            return ao;
        }
    }
#endif
#if (defined(_WIN32) || defined(_WIN64))
    if (!ao) {
try_nt_wdmks:
        ao = audio_wdmks_open(devpath, samplerate);
        if (ao) {
            audio_close = audio_wdmks_close; // audio_wdmks_open will already have initialized audio_write for us.   
            return ao;
        }
#ifdef ENABLE_WINMM
    }
    if (!ao) {
try_winmm:
        ao = audio_winmm_open(samplerate);
        if (ao) {
            audio_write_flt = audio_winmm_write_flt;
            audio_write_s16 = audio_winmm_write_s16;
            audio_close = audio_winmm_close;
            return ao;
        }
    }
#endif
#endif
    if (!ao) {
        //audio_wrerr("Tried all sound devices, none were successfully opened. Exiting...\n");
        return NULL;
    }
    return ao;
}

