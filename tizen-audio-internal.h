#ifndef footizenaudiointernalfoo
#define footizenaudiointernalfoo

/*
 * audio-hal
 *
 * Copyright (c) 2015 - 2016 Samsung Electronics Co., Ltd. All rights reserved.
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

/* PCM */
typedef struct {
    snd_pcm_format_t format;
    uint32_t rate;
    uint8_t channels;
} audio_pcm_sample_spec_t;

/* Routing */
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

/* Volume */
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

/* UCM */
typedef struct audio_hal_ucm {
    snd_use_case_mgr_t* uc_mgr;
} audio_hal_ucm_t;

/* Mixer */
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

typedef struct audio_hal_comm {
    message_cb msg_cb;
    void *user_data;
} audio_hal_comm_t;

/* Overall */
typedef struct audio_hal {
    audio_hal_device_t device;
    audio_hal_volume_t volume;
    audio_hal_ucm_t ucm;
    audio_hal_mixer_t mixer;
    audio_hal_comm_t comm;
} audio_hal_t;

audio_return_t _audio_volume_init(audio_hal_t *ah);
audio_return_t _audio_volume_deinit(audio_hal_t *ah);
audio_return_t _audio_routing_init(audio_hal_t *ah);
audio_return_t _audio_routing_deinit(audio_hal_t *ah);
audio_return_t _audio_stream_init(audio_hal_t *ah);
audio_return_t _audio_stream_deinit(audio_hal_t *ah);
audio_return_t _audio_pcm_init(audio_hal_t *ah);
audio_return_t _audio_pcm_deinit(audio_hal_t *ah);
audio_return_t _audio_comm_init(audio_hal_t *ah);
audio_return_t _audio_comm_deinit(audio_hal_t *ah);
audio_return_t _audio_comm_send_message(audio_hal_t *ah, const char *name, int value);

typedef struct _dump_data {
    char *strbuf;
    int left;
    char *p;
} dump_data_t;

dump_data_t* _audio_dump_new(int length);
void _audio_dump_add_str(dump_data_t *dump, const char *fmt, ...);
char* _audio_dump_get_str(dump_data_t *dump);
void _audio_dump_free(dump_data_t *dump);

#endif
