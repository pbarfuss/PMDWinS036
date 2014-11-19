#include <stdint.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

void *oss_audio_open(const char *devpath, uint32_t *oss_audio_rate)
{
    int afd = -1, tmp = 0;

    /* open the sound device */
    if (!devpath) {
        devpath = "/dev/dsp";
    }

    if ((afd = open(devpath, O_WRONLY, 0)) < 0)
    {   /* Opening device failed */
        return NULL;
    }

    tmp = AFMT_S16_NE;
    /* set the sound driver audio format for playback */
    if (ioctl(afd, SNDCTL_DSP_SETFMT, &tmp) < 0)
    {   /* Fatal error */
        goto err;
    }

    /* set the sound driver number of channels for playback */
    tmp = 0;
    if (ioctl(afd, SNDCTL_DSP_STEREO, &tmp) < 0)
    {   /* Fatal error */
        goto err;
    }

    /* set the sound driver number playback rate */
    tmp = *oss_audio_rate;
    if (ioctl(afd, SNDCTL_DSP_SPEED, &tmp) < 0)
    { /* Fatal error */
        goto err;
    }
    *oss_audio_rate = tmp;

    return (void*)((size_t)afd);
err:
    close(afd);
    return NULL;
}

#define WORD2INT(x) ((x) < -32766.5f ? -32767 : ((x) > 32766.5f ? 32767 : lrintf(x)))
void oss_audio_write_flt(void *ao, float *floatbuf, uint32_t floatbufsize)
{
    int snd_fd = (int)((size_t)ao);
    uint32_t i;
    short outbuf[8192];
    for(i=0; i<floatbufsize; i++) {
        outbuf[i] = WORD2INT(floatbuf[i]*32768.0f);
    }
    write(snd_fd, outbuf, floatbufsize*2);
}

void oss_audio_write_s16(void *ao, int16_t *buf, uint32_t bufsize)
{
    int snd_fd = (int)((size_t)ao);
    uint32_t i;
    short outbuf[8192];
    for(i=0; i<bufsize; i++) {
        outbuf[i] = buf[i];
    }
    write(snd_fd, outbuf, bufsize*2);
}

void oss_audio_close(void *ao)
{
    int snd_fd = (int)((size_t)ao);
    close(snd_fd);
}

