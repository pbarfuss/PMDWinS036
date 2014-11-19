/*
 * ALSA kernel API and common-type definitions
 *
 *  Copyright (c) 2011 by Takashi Iwai <tiwai@suse.de>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 */

#ifndef __ALSA_ASOUND_H
#define __ALSA_ASOUND_H

#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SND_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define SND_BIG_ENDIAN
#else
#error "Unsupported endian..."
#endif

/* protocol version */
#define SNDRV_PROTOCOL_VERSION(major, minor, subminor) \
	(((major) << 16) | ((minor) << 8) | (subminor))

/* PCM interface */
#define SNDRV_PCM_VERSION		SNDRV_PROTOCOL_VERSION(2, 0, 9)

typedef unsigned long snd_pcm_uframes_t;
typedef long snd_pcm_sframes_t;

typedef enum _snd_pcm_stream {
	SND_PCM_STREAM_PLAYBACK = 0,
	SND_PCM_STREAM_CAPTURE,
	SND_PCM_STREAM_LAST = SND_PCM_STREAM_CAPTURE,
} snd_pcm_stream_t;

typedef enum _snd_pcm_access {
	SND_PCM_ACCESS_MMAP_INTERLEAVED = 0,
	SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
	SND_PCM_ACCESS_MMAP_COMPLEX,
	SND_PCM_ACCESS_RW_INTERLEAVED,
	SND_PCM_ACCESS_RW_NONINTERLEAVED,
	SND_PCM_ACCESS_LAST = SND_PCM_ACCESS_RW_NONINTERLEAVED,
} snd_pcm_access_t;

typedef enum _sndrv_pcm_format {
	SND_PCM_FORMAT_UNKNOWN = -1,
	SND_PCM_FORMAT_S8 = 0,	/* signed */
	SND_PCM_FORMAT_U8,	/* unsigned */
	SND_PCM_FORMAT_S16_LE,	/* signed little-endian */
	SND_PCM_FORMAT_S16_BE,	/* signed big-endian */
	SND_PCM_FORMAT_U16_LE,
	SND_PCM_FORMAT_U16_BE,
	SND_PCM_FORMAT_S24_LE,	/* low three bytes */
	SND_PCM_FORMAT_S24_BE,
	SND_PCM_FORMAT_U24_LE,
	SND_PCM_FORMAT_U24_BE,
	SND_PCM_FORMAT_S32_LE,
	SND_PCM_FORMAT_S32_BE,
	SND_PCM_FORMAT_U32_LE,
	SND_PCM_FORMAT_U32_BE,
	SND_PCM_FORMAT_FLOAT_LE,	/* 4-byte float */
	SND_PCM_FORMAT_FLOAT_BE,
	SND_PCM_FORMAT_FLOAT64_LE,	/* 8-byte float */
	SND_PCM_FORMAT_FLOAT64_BE,
	SND_PCM_FORMAT_IEC958_SUBFRAME_LE,	/* IEC-958 subframe */
	SND_PCM_FORMAT_IEC958_SUBFRAME_BE,
	SND_PCM_FORMAT_MU_LAW,
	SND_PCM_FORMAT_A_LAW,
	SND_PCM_FORMAT_IMA_ADPCM,
	SND_PCM_FORMAT_MPEG,
	SND_PCM_FORMAT_GSM,
	SND_PCM_FORMAT_SPECIAL = 31,
	SND_PCM_FORMAT_S24_3LE = 32,	/* in three bytes */
	SND_PCM_FORMAT_S24_3BE,
	SND_PCM_FORMAT_U24_3LE,
	SND_PCM_FORMAT_U24_3BE,
	SND_PCM_FORMAT_S20_3LE,
	SND_PCM_FORMAT_S20_3BE,
	SND_PCM_FORMAT_U20_3LE,
	SND_PCM_FORMAT_U20_3BE,
	SND_PCM_FORMAT_S18_3LE,
	SND_PCM_FORMAT_S18_3BE,
	SND_PCM_FORMAT_U18_3LE,
	SND_PCM_FORMAT_U18_3BE,
	SND_PCM_FORMAT_LAST = SND_PCM_FORMAT_U18_3BE,

#ifdef SND_LITTLE_ENDIAN
	SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_LE,
	SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_LE,
	SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_LE,
	SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_LE,
	SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_LE,
	SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_LE,
	SND_PCM_FORMAT_FLOAT = SND_PCM_FORMAT_FLOAT_LE,
	SND_PCM_FORMAT_FLOAT64 = SND_PCM_FORMAT_FLOAT64_LE,
	SND_PCM_FORMAT_IEC958_SUBFRAME = SND_PCM_FORMAT_IEC958_SUBFRAME_LE,
#endif
#ifdef SND_BIG_ENDIAN
	SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_BE,
	SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_BE,
	SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_BE,
	SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_BE,
	SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_BE,
	SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_BE,
	SND_PCM_FORMAT_FLOAT = SND_PCM_FORMAT_FLOAT_BE,
	SND_PCM_FORMAT_FLOAT64 = SND_PCM_FORMAT_FLOAT64_BE,
	SND_PCM_FORMAT_IEC958_SUBFRAME = SND_PCM_FORMAT_IEC958_SUBFRAME_BE,
#endif
} snd_pcm_format_t;

typedef enum _snd_pcm_subformat {
	SND_PCM_SUBFORMAT_STD = 0,
	SND_PCM_SUBFORMAT_LAST = SND_PCM_SUBFORMAT_STD,
} snd_pcm_subformat_t;

#define SNDRV_PCM_INFO_MMAP		0x00000001
#define SNDRV_PCM_INFO_MMAP_VALID	0x00000002
#define SNDRV_PCM_INFO_DOUBLE		0x00000004
#define SNDRV_PCM_INFO_BATCH		0x00000010
#define SNDRV_PCM_INFO_INTERLEAVED	0x00000100
#define SNDRV_PCM_INFO_NONINTERLEAVED	0x00000200
#define SNDRV_PCM_INFO_COMPLEX		0x00000400
#define SNDRV_PCM_INFO_BLOCK_TRANSFER	0x00010000
#define SNDRV_PCM_INFO_OVERRANGE	0x00020000
#define SNDRV_PCM_INFO_RESUME		0x00040000
#define SNDRV_PCM_INFO_PAUSE		0x00080000
#define SNDRV_PCM_INFO_HALF_DUPLEX	0x00100000
#define SNDRV_PCM_INFO_JOINT_DUPLEX	0x00200000
#define SNDRV_PCM_INFO_SYNC_START	0x00400000
#define SNDRV_PCM_INFO_NO_PERIOD_WAKEUP	0x00800000

typedef enum _snd_pcm_state {
	SND_PCM_STATE_OPEN = 0,
	SND_PCM_STATE_SETUP,
	SND_PCM_STATE_PREPARED,
	SND_PCM_STATE_RUNNING,
	SND_PCM_STATE_XRUN,
	SND_PCM_STATE_DRAINING,
	SND_PCM_STATE_PAUSED,
	SND_PCM_STATE_SUSPENDED,
	SND_PCM_STATE_DISCONNECTED,
	SND_PCM_STATE_LAST = SND_PCM_STATE_DISCONNECTED
} snd_pcm_state_t;

enum {
	SNDRV_PCM_MMAP_OFFSET_DATA = 0x00000000,
	SNDRV_PCM_MMAP_OFFSET_STATUS = 0x80000000,
	SNDRV_PCM_MMAP_OFFSET_CONTROL = 0x81000000,
};

typedef union _snd_pcm_sync_id {
	unsigned char id[16];
	unsigned short id16[8];
	unsigned int id32[4];
} snd_pcm_sync_id_t;

typedef struct snd_pcm_info {
	unsigned int device;
	unsigned int subdevice;
	int stream;
	int card;
	unsigned char id[64];
	unsigned char name[80];
	unsigned char subname[32];
	int dev_class;
	int dev_subclass;
	unsigned int subdevices_count;
	unsigned int subdevices_avail;
	snd_pcm_sync_id_t sync;
	unsigned char reserved[64];
} snd_pcm_info_t;

enum sndrv_pcm_hw_param {
	SNDRV_PCM_HW_PARAM_ACCESS = 0,
	SNDRV_PCM_HW_PARAM_FIRST_MASK = SNDRV_PCM_HW_PARAM_ACCESS,
	SNDRV_PCM_HW_PARAM_FORMAT,
	SNDRV_PCM_HW_PARAM_SUBFORMAT,
	SNDRV_PCM_HW_PARAM_LAST_MASK = SNDRV_PCM_HW_PARAM_SUBFORMAT,

	SNDRV_PCM_HW_PARAM_SAMPLE_BITS = 8,
	SNDRV_PCM_HW_PARAM_FIRST_INTERVAL = SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
	SNDRV_PCM_HW_PARAM_FRAME_BITS,
	SNDRV_PCM_HW_PARAM_CHANNELS,
	SNDRV_PCM_HW_PARAM_RATE,
	SNDRV_PCM_HW_PARAM_PERIOD_TIME,
	SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
	SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
	SNDRV_PCM_HW_PARAM_PERIODS,
	SNDRV_PCM_HW_PARAM_BUFFER_TIME,
	SNDRV_PCM_HW_PARAM_BUFFER_SIZE,
	SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
	SNDRV_PCM_HW_PARAM_TICK_TIME,
	SNDRV_PCM_HW_PARAM_LAST_INTERVAL = SNDRV_PCM_HW_PARAM_TICK_TIME
};

#define SNDRV_PCM_HW_PARAMS_NORESAMPLE	(1<<0)
#define SNDRV_PCM_HW_PARAMS_EXPORT_BUFFER	(1<<1)
#define SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP	(1<<2)

typedef struct snd_interval {
	unsigned int min, max;
	unsigned int openmin:1,
		     openmax:1,
		     integer:1,
		     empty:1;
} snd_interval_t;

#define SND_MASK_MAX	256

typedef struct snd_mask {
	u_int32_t bits[SND_MASK_MAX / 32];
} snd_mask_t;

typedef struct snd_pcm_hw_params {
	unsigned int flags;
	snd_mask_t masks[SNDRV_PCM_HW_PARAM_LAST_MASK - 
			       SNDRV_PCM_HW_PARAM_FIRST_MASK + 1];
	snd_mask_t mres[5];
	snd_interval_t intervals[SNDRV_PCM_HW_PARAM_LAST_INTERVAL -
				        SNDRV_PCM_HW_PARAM_FIRST_INTERVAL + 1];
	snd_interval_t ires[9];
	unsigned int rmask;
	unsigned int cmask;
	unsigned int info;
	unsigned int msbits;
	unsigned int rate_num;
	unsigned int rate_den;
	snd_pcm_uframes_t fifo_size;
	unsigned char reserved[64];
} snd_pcm_hw_params_t;

typedef enum _snd_pcm_tstamp {
	SND_PCM_TSTAMP_NONE = 0,
	SND_PCM_TSTAMP_ENABLE,
	SND_PCM_TSTAMP_MMAP = SND_PCM_TSTAMP_ENABLE,
	SND_PCM_TSTAMP_LAST = SND_PCM_TSTAMP_MMAP
} snd_pcm_tstamp_t;

typedef struct snd_pcm_sw_params {
	int tstamp_mode;
	unsigned int period_step;
	unsigned int sleep_min;
	snd_pcm_uframes_t avail_min;
	snd_pcm_uframes_t xfer_align;
	snd_pcm_uframes_t start_threshold;
	snd_pcm_uframes_t stop_threshold;
	snd_pcm_uframes_t silence_threshold;
	snd_pcm_uframes_t silence_size;
	snd_pcm_uframes_t boundary;
	unsigned char reserved[60];
	unsigned int period_event;
} snd_pcm_sw_params_t;

/* XXX this is different from snd_pcm_channel_info_t !! */
struct sndrv_pcm_channel_info {
	unsigned int channel;
	long offset;
	unsigned int first;
	unsigned int step;
};

typedef struct snd_pcm_status {
	int state;
	struct timespec trigger_tstamp;
	struct timespec tstamp;
	snd_pcm_uframes_t appl_ptr;
	snd_pcm_uframes_t hw_ptr;
	snd_pcm_sframes_t delay;
	snd_pcm_uframes_t avail;
	snd_pcm_uframes_t avail_max;
	snd_pcm_uframes_t overrange;
	int suspended_state;
	unsigned char reserved[60];
} snd_pcm_status_t;

struct snd_pcm_mmap_status {
	int state;
	int pad1;
	snd_pcm_uframes_t hw_ptr;
	struct timespec tstamp;
	int suspended_state;
};

struct snd_pcm_mmap_control {
	snd_pcm_uframes_t appl_ptr;
	snd_pcm_uframes_t avail_min;
};

#define SNDRV_PCM_SYNC_PTR_HWSYNC	(1<<0)
#define SNDRV_PCM_SYNC_PTR_APPL		(1<<1)
#define SNDRV_PCM_SYNC_PTR_AVAIL_MIN	(1<<2)

struct snd_pcm_sync_ptr {
	unsigned int flags;
	union {
		struct snd_pcm_mmap_status status;
		unsigned char reserved[64];
	} s;
	union {
		struct snd_pcm_mmap_control control;
		unsigned char reserved[64];
	} c;
};

struct snd_xferi {
	snd_pcm_sframes_t result;
	void *buf;
	snd_pcm_uframes_t frames;
};

struct snd_xfern {
	snd_pcm_sframes_t result;
	void **bufs;
	snd_pcm_uframes_t frames;
};


enum {
        SNDRV_PCM_TSTAMP_TYPE_GETTIMEOFDAY = 0,
        SNDRV_PCM_TSTAMP_TYPE_MONOTONIC,
        SNDRV_PCM_TSTAMP_TYPE_LAST = SNDRV_PCM_TSTAMP_TYPE_MONOTONIC,
};

enum {
	SNDRV_PCM_IOCTL_PVERSION = _IOR('A', 0x00, int),
	SNDRV_PCM_IOCTL_INFO = _IOR('A', 0x01, snd_pcm_info_t),
	SNDRV_PCM_IOCTL_TSTAMP = _IOW('A', 0x02, int),
	SNDRV_PCM_IOCTL_TTSTAMP = _IOW('A', 0x03, int),
	SNDRV_PCM_IOCTL_HW_REFINE = _IOWR('A', 0x10, snd_pcm_hw_params_t),
	SNDRV_PCM_IOCTL_HW_PARAMS = _IOWR('A', 0x11, snd_pcm_hw_params_t),
	SNDRV_PCM_IOCTL_HW_FREE = _IO('A', 0x12),
	SNDRV_PCM_IOCTL_SW_PARAMS = _IOWR('A', 0x13, snd_pcm_sw_params_t),
	SNDRV_PCM_IOCTL_STATUS = _IOR('A', 0x20, snd_pcm_status_t),
	SNDRV_PCM_IOCTL_DELAY = _IOR('A', 0x21, snd_pcm_sframes_t),
	SNDRV_PCM_IOCTL_HWSYNC = _IO('A', 0x22),
	SNDRV_PCM_IOCTL_SYNC_PTR = _IOWR('A', 0x23, struct snd_pcm_sync_ptr),
	SNDRV_PCM_IOCTL_CHANNEL_INFO = _IOR('A', 0x32, struct sndrv_pcm_channel_info),
	SNDRV_PCM_IOCTL_PREPARE = _IO('A', 0x40),
	SNDRV_PCM_IOCTL_RESET = _IO('A', 0x41),
	SNDRV_PCM_IOCTL_START = _IO('A', 0x42),
	SNDRV_PCM_IOCTL_DROP = _IO('A', 0x43),
	SNDRV_PCM_IOCTL_DRAIN = _IO('A', 0x44),
	SNDRV_PCM_IOCTL_PAUSE = _IOW('A', 0x45, int),
	SNDRV_PCM_IOCTL_REWIND = _IOW('A', 0x46, snd_pcm_uframes_t),
	SNDRV_PCM_IOCTL_RESUME = _IO('A', 0x47),
	SNDRV_PCM_IOCTL_XRUN = _IO('A', 0x48),
	SNDRV_PCM_IOCTL_FORWARD = _IOW('A', 0x49, snd_pcm_uframes_t),
	SNDRV_PCM_IOCTL_WRITEI_FRAMES = _IOW('A', 0x50, struct snd_xferi),
	SNDRV_PCM_IOCTL_READI_FRAMES = _IOR('A', 0x51, struct snd_xferi),
	SNDRV_PCM_IOCTL_WRITEN_FRAMES = _IOW('A', 0x52, struct snd_xfern),
	SNDRV_PCM_IOCTL_READN_FRAMES = _IOR('A', 0x53, struct snd_xfern),
	SNDRV_PCM_IOCTL_LINK = _IOW('A', 0x60, int),
	SNDRV_PCM_IOCTL_UNLINK = _IO('A', 0x61),
};

/* Trick to make alsa-lib/acinclude.m4 happy */
#define SNDRV_PCM_IOCTL_REWIND SNDRV_PCM_IOCTL_REWIND

/* Timer interface */
#define SNDRV_TIMER_VERSION		SNDRV_PROTOCOL_VERSION(2, 0, 5)

typedef enum _snd_timer_class {
	SND_TIMER_CLASS_NONE = -1,
	SND_TIMER_CLASS_SLAVE = 0,
	SND_TIMER_CLASS_GLOBAL,	
	SND_TIMER_CLASS_CARD,
	SND_TIMER_CLASS_PCM,
	SND_TIMER_CLASS_LAST = SND_TIMER_CLASS_PCM
} snd_timer_class_t;

typedef enum _snd_timer_slave_class {
	SND_TIMER_SCLASS_NONE = 0,
	SND_TIMER_SCLASS_APPLICATION,
	SND_TIMER_SCLASS_SEQUENCER,
	SND_TIMER_SCLASS_OSS_SEQUENCER,
	SND_TIMER_SCLASS_LAST = SND_TIMER_SCLASS_OSS_SEQUENCER
} snd_timer_slave_class_t;

/* global timers (device member) */
#define SND_TIMER_GLOBAL_SYSTEM	0
#define SND_TIMER_GLOBAL_RTC		1
#define SND_TIMER_GLOBAL_HPET		2
#define SND_TIMER_GLOBAL_HRTIMER	3

/* info flags */
#define SNDRV_TIMER_FLG_SLAVE		(1<<0)	/* cannot be controlled */

typedef struct snd_timer_id {
	int dev_class;
	int dev_sclass;
	int card;
	int device;
	int subdevice;
} snd_timer_id_t;

typedef struct snd_timer_ginfo {
	snd_timer_id_t tid;
	unsigned int flags;
	int card;
	unsigned char id[64];
	unsigned char name[80];
	unsigned long reserved0;
	unsigned long resolution;
	unsigned long resolution_min;
	unsigned long resolution_max;
	unsigned int clients;
	unsigned char reserved[32];
} snd_timer_ginfo_t;

typedef struct snd_timer_gparams {
	snd_timer_id_t tid;
	unsigned long period_num;
	unsigned long period_den;
	unsigned char reserved[32];
} snd_timer_gparams_t;

typedef struct snd_timer_gstatus {
	snd_timer_id_t tid;
	unsigned long resolution;
	unsigned long resolution_num;
	unsigned long resolution_den;
	unsigned char reserved[32];
} snd_timer_gstatus_t;

typedef struct snd_timer_select {
	snd_timer_id_t id;
	unsigned char reserved[32];
} snd_timer_select_t;

typedef struct snd_timer_info {
	unsigned int flags;
	int card;
	unsigned char id[64];
	unsigned char name[80];
	unsigned long reserved0;
	unsigned long resolution;
	unsigned char reserved[64];
} snd_timer_info_t;

#define SNDRV_TIMER_PSFLG_AUTO		(1<<0)
#define SNDRV_TIMER_PSFLG_EXCLUSIVE	(1<<1)
#define SNDRV_TIMER_PSFLG_EARLY_EVENT	(1<<2)

typedef struct snd_timer_params {
	unsigned int flags;
	unsigned int ticks;
	unsigned int queue_size;
	unsigned int reserved0;
	unsigned int filter;
	unsigned char reserved[60];
} snd_timer_params_t;

typedef struct snd_timer_status {
	struct timespec tstamp;
	unsigned int resolution;
	unsigned int lost;
	unsigned int overrun;
	unsigned int queue;
	unsigned char reserved[64];
} snd_timer_status_t;

enum {
	SNDRV_TIMER_IOCTL_PVERSION = _IOR('T', 0x00, int),
	SNDRV_TIMER_IOCTL_NEXT_DEVICE = _IOWR('T', 0x01, snd_timer_id_t),
	SNDRV_TIMER_IOCTL_TREAD = _IOW('T', 0x02, int),
	SNDRV_TIMER_IOCTL_GINFO = _IOWR('T', 0x03, snd_timer_ginfo_t),
	SNDRV_TIMER_IOCTL_GPARAMS = _IOW('T', 0x04, snd_timer_gparams_t),
	SNDRV_TIMER_IOCTL_GSTATUS = _IOWR('T', 0x05, snd_timer_gstatus_t),
	SNDRV_TIMER_IOCTL_SELECT = _IOW('T', 0x10, snd_timer_select_t),
	SNDRV_TIMER_IOCTL_INFO = _IOR('T', 0x11, snd_timer_info_t),
	SNDRV_TIMER_IOCTL_PARAMS = _IOW('T', 0x12, snd_timer_params_t),
	SNDRV_TIMER_IOCTL_STATUS = _IOR('T', 0x14, snd_timer_status_t),
	/* The following four ioctls are changed since 1.0.9 due to confliction */
	SNDRV_TIMER_IOCTL_START = _IO('T', 0xa0),
	SNDRV_TIMER_IOCTL_STOP = _IO('T', 0xa1),
	SNDRV_TIMER_IOCTL_CONTINUE = _IO('T', 0xa2),
	SNDRV_TIMER_IOCTL_PAUSE = _IO('T', 0xa3),
};

typedef struct _snd_timer_read {
	unsigned int resolution;
        unsigned int ticks;
} snd_timer_read_t;

typedef enum _snd_timer_event {
	SND_TIMER_EVENT_RESOLUTION = 0,
	SND_TIMER_EVENT_TICK,
	SND_TIMER_EVENT_START,
	SND_TIMER_EVENT_STOP,
	SND_TIMER_EVENT_CONTINUE,
	SND_TIMER_EVENT_PAUSE,
	SND_TIMER_EVENT_EARLY,
	SND_TIMER_EVENT_SUSPEND,
	SND_TIMER_EVENT_RESUME,
	SND_TIMER_EVENT_MSTART = SND_TIMER_EVENT_START + 10,
	SND_TIMER_EVENT_MSTOP = SND_TIMER_EVENT_STOP + 10,
	SND_TIMER_EVENT_MCONTINUE = SND_TIMER_EVENT_CONTINUE + 10,
	SND_TIMER_EVENT_MPAUSE = SND_TIMER_EVENT_PAUSE + 10,
	SND_TIMER_EVENT_MSUSPEND = SND_TIMER_EVENT_SUSPEND + 10,
	SND_TIMER_EVENT_MRESUME = SND_TIMER_EVENT_RESUME + 10	
} snd_timer_event_t;

typedef struct timeval snd_timestamp_t;
typedef struct timespec snd_htimestamp_t;

typedef struct _snd_timer_tread {
	snd_timer_event_t event;
	snd_htimestamp_t tstamp;
	unsigned int val;
} snd_timer_tread_t;

typedef snd_mask_t snd_pcm_access_mask_t;
typedef snd_mask_t snd_pcm_format_mask_t;
typedef snd_mask_t snd_pcm_subformat_mask_t;
#define SND_PCM_NONBLOCK        0x00000001

#endif /* __ALSA_ASOUND_H */
