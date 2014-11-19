#ifndef __AUDIO_OUT_H__
#define __AUDIO_OUT_H__

#include <stdint.h>
#if defined(_WIN32) || defined(_WIN64)
#include "ntapi.h"
#endif

#if defined(_WIN32) || defined(_WIN64)
typedef UNICODE_STRING devpath_t;
#else
typedef const char devpath_t;
#endif

extern void (*audio_write_flt)(void *snd_fd, float *buf, uint32_t bufsize);
extern void (*audio_write_s16)(void *snd_fd, int16_t *buf, uint32_t bufsize);
extern void (*audio_close)(void *snd_fd);

void *audio_open(uint8_t devtype, devpath_t *devpath, uint32_t *samplerate);

void *oss_audio_open(const char *devpath, uint32_t *oss_audio_rate);
void oss_audio_write_flt(void *snd_fd, float *buf, uint32_t bufsize);
void oss_audio_write_s16(void *snd_fd, int16_t *buf, uint32_t bufsize);
void oss_audio_close(void *snd_fd);
void *alsa_audio_open(const char *devpath, uint32_t *samplerate, uint32_t *_ver);
void alsa_audio_write_flt(void *snd_fd, float *buf, uint32_t bufsize);
void alsa_audio_write_s16(void *snd_fd, int16_t *buf, uint32_t bufsize);
void alsa_audio_close(void *snd_fd);

void *CoreAudioOpen(uint32_t rate);
void CoreAudioWriteFlt(void *_ao, float *output_samples, uint32_t num_bytes);
void CoreAudioWriteS16(void *_ao, int16_t *output_samples, uint32_t num_bytes);
void CoreAudioClose(void *_ao);

#if defined(_WIN32) || defined(_WIN64)
void *audio_wdmks_open(UNICODE_STRING *AudioDevicePath, uint32_t *samplerate, uint32_t bufsize);
void audio_wdmks_close(void *wdmks);
void *audio_winmm_open(unsigned int *sfreq);
void audio_winmm_write_flt(void *waveout, float *buf, uint32_t bufsize);
void audio_winmm_write_s16(void *waveout, int16_t *buf, uint32_t bufsize);
void audio_winmm_close(void *waveout);
#if 0
typedef struct _devlist_t {
    void *next;
    ULONG idx;
    UNICODE_STRING devpath;
} devlist_t;
devlist_t *NtWdmKSEnumerateAudioDevices(UNICODE_STRING *KeyName);
#endif
#endif

#endif
