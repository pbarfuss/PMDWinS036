#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h> 
#include "alsa_pcm_api.h"
#define MASK_SIZE (SND_MASK_MAX / 32)
#define MASK_OFS(i) ((i) >> 5)
#define MASK_BIT(i) (1U << ((i) & 31))

static inline
snd_interval_t *hw_param_interval(snd_pcm_hw_params_t *params, int var)
{
    return &params->intervals[var - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL];
}

static inline
const snd_interval_t *hw_param_interval_c(const snd_pcm_hw_params_t *params, int var)
{
    return (const snd_interval_t *)hw_param_interval((snd_pcm_hw_params_t*) params, var);
}

size_t snd_pcm_hw_params_sizeof(void)
{
    return sizeof(snd_pcm_hw_params_t);
}

static void snd_interval_any(snd_interval_t *i)
{
    i->min = 0;
    i->openmin = 0;
    i->max = -1;
    i->openmax = 0;
    i->integer = 0;
    i->empty = 0;
}

static inline int snd_interval_empty(const snd_interval_t *i)
{
    return i->empty;
}

static inline int snd_interval_checkempty(const snd_interval_t *i)
{
    return (i->min > i->max ||
        (i->min == i->max && (i->openmin || i->openmax)));
}

/* Return the minimum value for field PAR. */
int _snd_pcm_hw_param_get_min(const snd_pcm_hw_params_t *params,
                  int var, unsigned int *val, int *dir)
{                 
    const snd_interval_t *i = hw_param_interval_c(params, var);
    if (dir)
        *dir = i->openmin;
    if (val)
        *val = i->min;
    return 0;
}

/* Return the maximum value for field PAR. */
int _snd_pcm_hw_param_get_max(const snd_pcm_hw_params_t *params,
                  int var, unsigned int *val, int *dir)
{
    const snd_interval_t *i = hw_param_interval_c(params, var);
    if (dir)
        *dir = - (int) i->openmax;
    if (val)
        *val = i->max;
    return 0;
}

static void _snd_pcm_hw_param_any(snd_pcm_hw_params_t *params, int var)
{
    snd_interval_any(hw_param_interval(params, var));
    params->cmask |= 1 << var;
    params->rmask |= 1 << var;
}

int snd_pcm_hw_params_any(int snd_fd, snd_pcm_hw_params_t *params)
{
    unsigned int k;
    memset(params, 0, sizeof(*params));
    for (k = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         k <= SNDRV_PCM_HW_PARAM_LAST_MASK; k++) {
        snd_mask_t *mask = &params->masks[k - SNDRV_PCM_HW_PARAM_FIRST_MASK];
        memset(mask, 0xff, sizeof(*mask));
        params->cmask |= 1 << k;
        params->rmask |= 1 << k;
    }
    for (k = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         k <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; k++)
        _snd_pcm_hw_param_any(params, k);
    params->rmask = ~0U;
    params->cmask = 0;
    params->info = ~0U;
    return(ioctl(snd_fd, SNDRV_PCM_IOCTL_HW_REFINE, params));
}

int snd_pcm_hw_params_store(int snd_fd, snd_pcm_hw_params_t *params)
{
    ioctl(snd_fd, SNDRV_PCM_IOCTL_HW_REFINE, params);
    if (ioctl(snd_fd, SNDRV_PCM_IOCTL_HW_PARAMS, params) < 0)
        return -1;
    if (ioctl(snd_fd, SNDRV_PCM_IOCTL_PREPARE) < 0)
        return -1; //-errno;
    return 0;
}

static int hw_param_update_var(int snd_fd, snd_pcm_hw_params_t *params,
                               int var, int changed)
{
    if (changed < 0)
        return changed;
    if (changed) {
        params->cmask |= 1 << var;
        params->rmask |= 1 << var;
    }                                                                                                                                                   
    if (params->rmask) {
        changed = ioctl(snd_fd, SNDRV_PCM_IOCTL_HW_REFINE, params);
        if (changed < 0)
            return changed;
        if (snd_interval_empty(hw_param_interval_c(params, var)))
            return -ENOENT;
    }
    return 0;
}

static int snd_interval_refine(snd_interval_t *i, const snd_interval_t *v)
{
    int changed = 0;
    if (snd_interval_empty(i))
        return -ENOENT;
    if (i->min < v->min) {
        i->min = v->min;
        i->openmin = v->openmin;
        changed = 1;
    } else if (i->min == v->min && !i->openmin && v->openmin) {
        i->openmin = 1;
        changed = 1;
    }
    if (i->max > v->max) {
        i->max = v->max;
        i->openmax = v->openmax;
        changed = 1;
    } else if (i->max == v->max && !i->openmax && v->openmax) {
        i->openmax = 1;
        changed = 1;
    }
    if (!i->integer && v->integer) {
        i->integer = 1;
        changed = 1;
    }
    if (i->integer) {
        if (i->openmin) {
            i->min++;
            i->openmin = 0;
        }
        if (i->openmax) {
            i->max--;
            i->openmax = 0;
        }
    } else if (!i->openmin && !i->openmax && i->min == i->max)
        i->integer = 1;
    if (snd_interval_checkempty(i)) {
        i->empty = 1;
        return -EINVAL;
    }
    return changed;
}

int _snd_pcm_hw_param_get(const snd_pcm_hw_params_t *params,
                          int var, unsigned int *val, int *dir)
{
    const snd_interval_t *i = hw_param_interval_c(params, var);
    if (snd_interval_empty(i) || !(i->min == i->max || ((i->min + 1 == i->max) && i->openmax)))
        return -EINVAL;
    if (dir)
        *dir = i->openmin;
    if (val)
        *val = i->min;
    return 0;
}

int _snd_pcm_hw_param_set(int snd_fd, snd_pcm_hw_params_t *params, int var,
                          unsigned int val, int dir)
{
    int err = 0;
    snd_pcm_hw_params_t save = *params;
    snd_interval_t *i = hw_param_interval(params, var);
    snd_interval_t t;
    t.empty = 0;
    t.min = t.max = val;
    t.openmin = t.openmax = 0;
    t.integer = 1;
    err = snd_interval_refine(i, &t);
    err = hw_param_update_var(snd_fd, params, var, err);
    if (err)
        *params = save;
    return err;
}

static inline int _snd_pcm_hw_param_get64(const snd_pcm_hw_params_t *params, int type, unsigned long *val, int *dir)
{
    unsigned int _val;
    int err = _snd_pcm_hw_param_get(params, type, &_val, dir);
    *val = _val;
    return err;
}

static inline unsigned int popcount32(uint32_t mask)
{
    register unsigned int y;
    y = (mask >> 1) &033333333333;
    y = mask - y - ((y >>1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
}

int snd_pcm_hw_params_set_access(int pcm_fd, snd_pcm_hw_params_t *params, snd_pcm_access_t access)
{
    snd_pcm_hw_params_t save = *params;
    snd_mask_t *param = (snd_mask_t*)&params->masks[SNDRV_PCM_HW_PARAM_ACCESS - SNDRV_PCM_HW_PARAM_FIRST_MASK];
    uint32_t i, v = 0;
    int err = 0, changed;
    for (i = 0; i < MASK_SIZE; i++) {
        v += popcount32(param->bits[i]);
    }
    changed = (v != 1);
    v = param->bits[MASK_OFS(access)] & MASK_BIT(access);
    memset(param, 0, sizeof(*param));
    param->bits[MASK_OFS(access)] = v;
    if (changed) {
        params->cmask |= 1 << SNDRV_PCM_HW_PARAM_ACCESS;
        params->rmask |= 1 << SNDRV_PCM_HW_PARAM_ACCESS;
    }
    if (params->rmask) {
        err = ioctl(pcm_fd, SNDRV_PCM_IOCTL_HW_REFINE, params);
        if (err)
            *params = save;
    }
    return err;
}

int snd_pcm_hw_params_set_format(int pcm_fd, snd_pcm_hw_params_t *params, snd_pcm_format_t format)
{
    snd_pcm_hw_params_t save = *params;
    snd_mask_t *param = (snd_mask_t*)&params->masks[SNDRV_PCM_HW_PARAM_FORMAT - SNDRV_PCM_HW_PARAM_FIRST_MASK];
    uint32_t i, v = 0;
    int err = 0, changed;
    for (i = 0; i < MASK_SIZE; i++) {
        v += popcount32(param->bits[i]);
    }
    changed = (v != 1);
    v = param->bits[MASK_OFS(format)] & MASK_BIT(format);
    memset(param, 0, sizeof(*param));
    param->bits[MASK_OFS(format)] = v;
    if (changed) {
        params->cmask |= 1 << SNDRV_PCM_HW_PARAM_FORMAT;
        params->rmask |= 1 << SNDRV_PCM_HW_PARAM_FORMAT;
    }
    if (params->rmask) {
        err = ioctl(pcm_fd, SNDRV_PCM_IOCTL_HW_REFINE, params);
        if (err)
            *params = save;
    }
    return err;
}

int snd_pcm_hw_params_get_access(const snd_pcm_hw_params_t *params, snd_pcm_access_t *access)
{
    snd_mask_t *param = (snd_mask_t*)&params->masks[SNDRV_PCM_HW_PARAM_ACCESS - SNDRV_PCM_HW_PARAM_FIRST_MASK];
    uint32_t i = 0, v = 0;
    for (i = 0; i < MASK_SIZE; i++) {
        v += popcount32(param->bits[i]);
    }
    if (v != 1)
        return -EINVAL;
    if (access) {
        unsigned int access_val = 0;
        for (i = 0; i < MASK_SIZE; i++) {
            if (param->bits[i]) {
                access_val = ffs(param->bits[i]) - 1 + (i << 5);
                break;
            }
        }
        *access = access_val;
    }
    return 0;
}

int snd_pcm_hw_params_get_format(const snd_pcm_hw_params_t *params, snd_pcm_format_t *format)
{
    snd_mask_t *param = (snd_mask_t*)&params->masks[SNDRV_PCM_HW_PARAM_FORMAT - SNDRV_PCM_HW_PARAM_FIRST_MASK];
    uint32_t i = 0, v = 0;
    for (i = 0; i < MASK_SIZE; i++) {
        v += popcount32(param->bits[i]);
    }
    if (v != 1)
        return -EINVAL;
    if (format) {
        unsigned int format_val = 0;
        for (i = 0; i < MASK_SIZE; i++) {
            if (param->bits[i]) {
                format_val = ffs(param->bits[i]) - 1 + (i << 5);
                break;
            }
        }
        *format = format_val;
    }
    return 0;
}

void alsa_init_hwparams(int snd_fd, snd_pcm_access_t access, uint32_t *samplerate, uint32_t frame_size, uint8_t channels)
{
	uint32_t actual_channels;
    snd_pcm_hw_params_t *hwparams;
    *&hwparams = (snd_pcm_hw_params_t *) alloca(snd_pcm_hw_params_sizeof());

    // Initialize hwparams structure.
    snd_pcm_hw_params_any(snd_fd, hwparams);

    // this *must* be set, or else the writei (actually an ioctl(), wtf?) below will fail with -EBADF
    snd_pcm_hw_params_set_access(snd_fd, hwparams, access);
    snd_pcm_hw_params_set_format(snd_fd, hwparams, SND_PCM_FORMAT_S16_LE);

    _snd_pcm_hw_param_set(snd_fd, hwparams, SNDRV_PCM_HW_PARAM_CHANNELS, channels, 0);
	_snd_pcm_hw_param_get(hwparams, SNDRV_PCM_HW_PARAM_CHANNELS, &actual_channels, NULL);
	if ((channels == 1) && (actual_channels == 2)) {
		// This is an offensively dumb hack that works around the fact that the ALSA devs erroneously believe that HDAudio chipsets cannot playback mono.
		// On any other chipset, it's a terrible idea. In general, it's awful. I'm just lazy. That, and I don't use ALSA myself. :P
		*samplerate >>= 1;
	}
    _snd_pcm_hw_param_set(snd_fd, hwparams, SNDRV_PCM_HW_PARAM_RATE, *samplerate, 0);
    _snd_pcm_hw_param_set(snd_fd, hwparams, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, frame_size, 0);
    _snd_pcm_hw_param_set(snd_fd, hwparams, SNDRV_PCM_HW_PARAM_PERIODS, ((frame_size > 1024) ? 8 : 16), 0);
    snd_pcm_hw_params_store(snd_fd, hwparams);
    _snd_pcm_hw_param_get(hwparams, SNDRV_PCM_HW_PARAM_RATE, samplerate, NULL);
}

