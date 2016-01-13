#ifndef footizenaudiointernalfoo
#define footizenaudiointernalfoo

/*
 * audio-hal
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <dlog.h>
#include <time.h>
#include <sys/types.h>
#include <asoundlib.h>
#ifdef __USE_TINYALSA__
#include <tinyalsa/asoundlib.h>
#endif
#include <pthread.h>
#include <use-case.h>
#include "tizen-audio.h"

/* Debug */

//#define AUDIO_DEBUG
#define PROPERTY_VALUE_MAX 92
#define BUF_SIZE 1024
#define AUDIO_DUMP_STR_LEN              256
#define AUDIO_DEVICE_INFO_LIST_MAX      16
#ifdef USE_DLOG
#ifdef DLOG_TAG
#undef DLOG_TAG
#endif
#define DLOG_TAG "AUDIO_HAL"
#define AUDIO_LOG_ERROR(...)            SLOG(LOG_ERROR, DLOG_TAG, __VA_ARGS__)
#define AUDIO_LOG_WARN(...)             SLOG(LOG_WARN, DLOG_TAG, __VA_ARGS__)
#define AUDIO_LOG_INFO(...)             SLOG(LOG_INFO, DLOG_TAG, __VA_ARGS__)
#define AUDIO_LOG_DEBUG(...)            SLOG(LOG_DEBUG, DLOG_TAG, __VA_ARGS__)
#define AUDIO_LOG_VERBOSE(...)          SLOG(LOG_DEBUG, DLOG_TAG, __VA_ARGS__)
#else
#define AUDIO_LOG_ERROR(...)            fprintf(stderr, __VA_ARGS__)
#define AUDIO_LOG_WARN(...)             fprintf(stderr, __VA_ARGS__)
#define AUDIO_LOG_INFO(...)             fprintf(stdout, __VA_ARGS__)
#define AUDIO_LOG_DEBUG(...)            fprintf(stdout, __VA_ARGS__)
#define AUDIO_LOG_VERBOSE(...)          fprintf(stdout, __VA_ARGS__)
#endif

#define AUDIO_RETURN_IF_FAIL(expr) do { \
    if (!expr) { \
        AUDIO_LOG_ERROR("%s failed", #expr); \
        return; \
    } \
} while (0)
#define AUDIO_RETURN_VAL_IF_FAIL(expr, val) do { \
    if (!expr) { \
        AUDIO_LOG_ERROR("%s failed", #expr); \
        return val; \
    } \
} while (0)
#define AUDIO_RETURN_NULL_IF_FAIL(expr) do { \
    if (!expr) { \
        AUDIO_LOG_ERROR("%s failed", #expr); \
        return NULL; \
    } \
} while (0)

/* Devices : Normal  */
#define AUDIO_DEVICE_OUT               0x00000000
#define AUDIO_DEVICE_IN                0x80000000
enum audio_device_type {
    AUDIO_DEVICE_NONE                 = 0,

    /* output devices */
    AUDIO_DEVICE_OUT_SPEAKER          = AUDIO_DEVICE_OUT | 0x00000001,
    AUDIO_DEVICE_OUT_RECEIVER         = AUDIO_DEVICE_OUT | 0x00000002,
    AUDIO_DEVICE_OUT_JACK             = AUDIO_DEVICE_OUT | 0x00000004,
    AUDIO_DEVICE_OUT_BT_SCO           = AUDIO_DEVICE_OUT | 0x00000008,
    AUDIO_DEVICE_OUT_AUX              = AUDIO_DEVICE_OUT | 0x00000010,
    AUDIO_DEVICE_OUT_HDMI             = AUDIO_DEVICE_OUT | 0x00000020,
    AUDIO_DEVICE_OUT_ALL              = (AUDIO_DEVICE_OUT_SPEAKER |
                                         AUDIO_DEVICE_OUT_RECEIVER |
                                         AUDIO_DEVICE_OUT_JACK |
                                         AUDIO_DEVICE_OUT_BT_SCO |
                                         AUDIO_DEVICE_OUT_AUX |
                                         AUDIO_DEVICE_OUT_HDMI),
    /* input devices */
    AUDIO_DEVICE_IN_MAIN_MIC          = AUDIO_DEVICE_IN | 0x00000001,
    AUDIO_DEVICE_IN_SUB_MIC           = AUDIO_DEVICE_IN | 0x00000002,
    AUDIO_DEVICE_IN_JACK              = AUDIO_DEVICE_IN | 0x00000004,
    AUDIO_DEVICE_IN_BT_SCO            = AUDIO_DEVICE_IN | 0x00000008,
    AUDIO_DEVICE_IN_ALL               = (AUDIO_DEVICE_IN_MAIN_MIC |
                                         AUDIO_DEVICE_IN_SUB_MIC |
                                         AUDIO_DEVICE_IN_JACK |
                                         AUDIO_DEVICE_IN_BT_SCO),
};

typedef struct device_type {
    uint32_t type;
    const char *name;
} device_type_t;

/* Verbs */
#define AUDIO_USE_CASE_VERB_INACTIVE                "Inactive"
#define AUDIO_USE_CASE_VERB_HIFI                    "HiFi"

/* Modifiers */
#define AUDIO_USE_CASE_MODIFIER_VOICESEARCH              "VoiceSearch"
#define AUDIO_USE_CASE_MODIFIER_CAMCORDING               "Camcording"
#define AUDIO_USE_CASE_MODIFIER_MEDIA                    "Media"
#define AUDIO_USE_CASE_MODIFIER_DUAL_MEDIA               "DualMedia"

#define streq !strcmp
#define strneq strcmp

#define ALSA_DEFAULT_CARD       "max98090"
#define PLAYBACK_PCM_DEVICE     "hw:0,0"
#define CAPTURE_PCM_DEVICE      "hw:0,0"

/* hw:0,0 */
#define PLAYBACK_CARD_ID        0
#define PLAYBACK_PCM_DEVICE_ID  0

/* hw:0,0 */
#define CAPTURE_CARD_ID         0
#define CAPTURE_PCM_DEVICE_ID   0

#define MAX_DEVICES             5
#define MAX_MODIFIERS           5
#define MAX_NAME_LEN           32

/* type definitions */
typedef signed char int8_t;

/* pcm */
typedef struct {
    snd_pcm_format_t format;
    uint32_t rate;
    uint8_t channels;
} audio_pcm_sample_spec_t;

/* Device */
typedef enum audio_device_api {
    AUDIO_DEVICE_API_UNKNOWN,
    AUDIO_DEVICE_API_ALSA,
    AUDIO_DEVICE_API_BLUEZ,
} audio_device_api_t;

typedef struct audio_device_alsa_info {
    char *card_name;
    uint32_t card_idx;
    uint32_t device_idx;
} audio_device_alsa_info_t;

typedef struct audio_device_bluz_info {
    char *protocol;
    uint32_t nrec;
} audio_device_bluez_info_t;

typedef struct audio_device_info {
    audio_device_api_t api;
    audio_direction_t direction;
    char *name;
    uint8_t is_default_device;
    union {
        audio_device_alsa_info_t alsa;
        audio_device_bluez_info_t bluez;
    };
} audio_device_info_t;

typedef enum audio_route_mode {
    VERB_NORMAL,
} audio_route_mode_t;

typedef struct audio_hal_device {
    uint32_t active_in;
    uint32_t active_out;
    snd_pcm_t *pcm_in;
    snd_pcm_t *pcm_out;
    pthread_mutex_t pcm_lock;
    uint32_t pcm_count;
    audio_route_mode_t mode;
} audio_hal_device_t;

/* Stream */
#define AUDIO_VOLUME_LEVEL_MAX 16

typedef enum audio_volume {
    AUDIO_VOLUME_TYPE_SYSTEM,           /**< System volume type */
    AUDIO_VOLUME_TYPE_NOTIFICATION,     /**< Notification volume type */
    AUDIO_VOLUME_TYPE_ALARM,            /**< Alarm volume type */
    AUDIO_VOLUME_TYPE_RINGTONE,         /**< Ringtone volume type */
    AUDIO_VOLUME_TYPE_MEDIA,            /**< Media volume type */
    AUDIO_VOLUME_TYPE_CALL,             /**< Call volume type */
    AUDIO_VOLUME_TYPE_VOIP,             /**< VOIP volume type */
    AUDIO_VOLUME_TYPE_VOICE,            /**< Voice volume type */
    AUDIO_VOLUME_TYPE_MAX,              /**< Volume type count */
} audio_volume_t;

typedef enum audio_gain {
    AUDIO_GAIN_TYPE_DEFAULT,
    AUDIO_GAIN_TYPE_DIALER,
    AUDIO_GAIN_TYPE_TOUCH,
    AUDIO_GAIN_TYPE_AF,
    AUDIO_GAIN_TYPE_SHUTTER1,
    AUDIO_GAIN_TYPE_SHUTTER2,
    AUDIO_GAIN_TYPE_CAMCODING,
    AUDIO_GAIN_TYPE_MIDI,
    AUDIO_GAIN_TYPE_BOOTING,
    AUDIO_GAIN_TYPE_VIDEO,
    AUDIO_GAIN_TYPE_TTS,
    AUDIO_GAIN_TYPE_MAX,
} audio_gain_t;

typedef struct audio_volume_value_table {
    double volume[AUDIO_VOLUME_TYPE_MAX][AUDIO_VOLUME_LEVEL_MAX];
    uint32_t volume_level_max[AUDIO_VOLUME_LEVEL_MAX];
    double gain[AUDIO_GAIN_TYPE_MAX];
} audio_volume_value_table_t;

enum {
    AUDIO_VOLUME_DEVICE_DEFAULT,
    AUDIO_VOLUME_DEVICE_MAX,
};

typedef struct audio_hal_volume {
    uint32_t volume_level[AUDIO_VOLUME_TYPE_MAX];
    audio_volume_value_table_t *volume_value_table;
} audio_hal_volume_t;

typedef struct audio_hal_ucm {
    snd_use_case_mgr_t* uc_mgr;
} audio_hal_ucm_t;

typedef struct audio_hal_mixer {
    snd_mixer_t *mixer;
    pthread_mutex_t mutex;
    struct {
        snd_ctl_elem_value_t *value;
        snd_ctl_elem_id_t *id;
        snd_ctl_elem_info_t *info;
    } control;
} audio_hal_mixer_t;

/* Audio format */
typedef enum audio_sample_format {
    AUDIO_SAMPLE_U8,
    AUDIO_SAMPLE_ALAW,
    AUDIO_SAMPLE_ULAW,
    AUDIO_SAMPLE_S16LE,
    AUDIO_SAMPLE_S16BE,
    AUDIO_SAMPLE_FLOAT32LE,
    AUDIO_SAMPLE_FLOAT32BE,
    AUDIO_SAMPLE_S32LE,
    AUDIO_SAMPLE_S32BE,
    AUDIO_SAMPLE_S24LE,
    AUDIO_SAMPLE_S24BE,
    AUDIO_SAMPLE_S24_32LE,
    AUDIO_SAMPLE_S24_32BE,
    AUDIO_SAMPLE_MAX,
    AUDIO_SAMPLE_INVALID = -1
} audio_sample_format_t;

/* Overall */
typedef struct audio_hal {
    audio_hal_device_t device;
    audio_hal_volume_t volume;
    audio_hal_ucm_t ucm;
    audio_hal_mixer_t mixer;
} audio_hal_t;

typedef struct {
    unsigned short      is_open; /* if is_open is true, open device; else close device.*/
    unsigned short      is_headphone;
    unsigned int        is_downlink_mute;
    unsigned int        is_uplink_mute;
} device_ctrl_t;

typedef struct samplerate_ctrl {
    unsigned int samplerate; /* change samplerate.*/
} set_samplerate_t;

audio_return_t _audio_volume_init(audio_hal_t *ah);
audio_return_t _audio_volume_deinit(audio_hal_t *ah);

audio_return_t _audio_device_init(audio_hal_t *ah);
audio_return_t _audio_device_deinit(audio_hal_t *ah);
audio_return_t _audio_ucm_init(audio_hal_t *ah);
audio_return_t _audio_ucm_deinit(audio_hal_t *ah);
void _audio_ucm_get_device_name(audio_hal_t *ah, const char *use_case, audio_direction_t direction, const char **value);
#define _audio_ucm_update_use_case _audio_ucm_set_use_case
audio_return_t _audio_ucm_set_use_case(audio_hal_t *ah, const char *verb, const char *devices[], const char *modifiers[]);
audio_return_t _audio_ucm_set_devices(audio_hal_t *ah, const char *verb, const char *devices[]);
audio_return_t _audio_ucm_set_modifiers(audio_hal_t *ah, const char *verb, const char *modifiers[]);
int _audio_ucm_fill_device_info_list(audio_hal_t *ah, audio_device_info_t *device_info_list, const char *verb);
audio_return_t _audio_ucm_get_verb(audio_hal_t *ah, const char **value);
audio_return_t _audio_ucm_reset_use_case(audio_hal_t *ah);
audio_return_t _audio_util_init(audio_hal_t *ah);
audio_return_t _audio_util_deinit(audio_hal_t *ah);
audio_return_t _audio_mixer_control_set_param(audio_hal_t *ah, const char* ctl_name, snd_ctl_elem_value_t* value, int size);
audio_return_t _audio_mixer_control_set_value(audio_hal_t *ah, const char *ctl_name, int val);
audio_return_t _audio_mixer_control_set_value_string(audio_hal_t *ah, const char* ctl_name, const char* value);
audio_return_t _audio_mixer_control_get_value(audio_hal_t *ah, const char *ctl_name, int *val);
audio_return_t _audio_mixer_control_get_element(audio_hal_t *ah, const char *ctl_name, snd_hctl_elem_t **elem);
audio_return_t _audio_pcm_set_sw_params(snd_pcm_t *pcm, snd_pcm_uframes_t avail_min, uint8_t period_event);
audio_return_t _audio_pcm_set_hw_params(snd_pcm_t *pcm, audio_pcm_sample_spec_t *sample_spec, uint8_t *use_mmap, snd_pcm_uframes_t *period_size, snd_pcm_uframes_t *buffer_size);
uint32_t _convert_format(audio_sample_format_t format);

typedef struct _dump_data {
    char *strbuf;
    int left;
    char *p;
} dump_data_t;

dump_data_t* dump_new(int length);
void dump_add_str(dump_data_t *dump, const char *fmt, ...);
char* dump_get_str(dump_data_t *dump);
void dump_free(dump_data_t *dump);

#endif
