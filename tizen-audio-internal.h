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

/* Devices : Normal  */
enum audio_device_type {
    AUDIO_DEVICE_NONE                 = 0,

    /* output devices */
    AUDIO_DEVICE_OUT_SPEAKER          = 0x00000001,
    AUDIO_DEVICE_OUT_RECEIVER         = 0x00000002,
    AUDIO_DEVICE_OUT_JACK             = 0x00000004,
    AUDIO_DEVICE_OUT_BT_SCO           = 0x00000008,
    AUDIO_DEVICE_OUT_AUX              = 0x00000010,
    AUDIO_DEVICE_OUT_HDMI             = 0x00000020,
    AUDIO_DEVICE_OUT_ALL              = (AUDIO_DEVICE_OUT_SPEAKER |
                                         AUDIO_DEVICE_OUT_RECEIVER |
                                         AUDIO_DEVICE_OUT_JACK |
                                         AUDIO_DEVICE_OUT_BT_SCO |
                                         AUDIO_DEVICE_OUT_AUX |
                                         AUDIO_DEVICE_OUT_HDMI),
    /* input devices */
    AUDIO_DEVICE_IN_MAIN_MIC          = 0x80000001,
    AUDIO_DEVICE_IN_SUB_MIC           = 0x80000002,
    AUDIO_DEVICE_IN_JACK              = 0x80000004,
    AUDIO_DEVICE_IN_BT_SCO            = 0x80000008,
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

#define PERIODSZ_PLAYBACK 2048
#define BUFFERSZ_PLAYBACK 4096

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
typedef enum audio_route_mode{
    VERB_NORMAL,

/* Add verb mode if you need */

/*   VERB_CALL, */
/*   VERB_VOIP */

} audio_route_mode_t;

typedef struct audio_device_mgr {
    uint32_t active_in;
    uint32_t active_out;
    uint32_t route_flag;
    snd_pcm_t *pcm_in;
    snd_pcm_t *pcm_out;
    pthread_mutex_t pcm_lock;
    uint32_t pcm_count;
    audio_route_mode_t mode;
} audio_device_mgr_t;


/* Stream */

#define AUDIO_VOLUME_LEVEL_MAX 16

typedef struct audio_volume_value_table {
    double volume[AUDIO_VOLUME_TYPE_MAX][AUDIO_VOLUME_LEVEL_MAX];
    uint32_t volume_level_max[AUDIO_VOLUME_LEVEL_MAX];
    double gain[AUDIO_GAIN_TYPE_MAX];
} audio_volume_value_table_t;

enum {
    AUDIO_VOLUME_DEVICE_DEFAULT,
    AUDIO_VOLUME_DEVICE_MAX,
};

typedef struct audio_volume_mgr {
    uint32_t volume_level[AUDIO_VOLUME_TYPE_MAX];
    audio_volume_value_table_t *volume_value_table;
} audio_volume_mgr_t;

typedef struct audio_ucm_mgr {
    snd_use_case_mgr_t* uc_mgr;
} audio_ucm_mgr_t;

typedef struct audio_mixer_mgr {
    snd_mixer_t *mixer;
    pthread_mutex_t mutex;
    struct {
        snd_ctl_elem_value_t *value;
        snd_ctl_elem_id_t *id;
        snd_ctl_elem_info_t *info;
    } control;
} audio_mixer_mgr_t;

/* Overall */

typedef struct audio_mgr {
    void *platform_data;
    audio_cb_interface_t cb_intf;
    audio_device_mgr_t device;
    audio_volume_mgr_t volume;
    audio_ucm_mgr_t ucm;
    audio_mixer_mgr_t mixer;
} audio_mgr_t;

typedef struct {
    unsigned short      is_open; /* if is_open is true, open device; else close device.*/
    unsigned short      is_headphone;
    unsigned int        is_downlink_mute;
    unsigned int        is_uplink_mute;
} device_ctrl_t;

typedef struct samplerate_ctrl {
    unsigned int samplerate; /* change samplerate.*/
} set_samplerate_t;

audio_return_t _audio_volume_init (audio_mgr_t *am);
audio_return_t _audio_volume_deinit (audio_mgr_t *am);

audio_return_t _audio_device_init (audio_mgr_t *am);
audio_return_t _audio_device_deinit (audio_mgr_t * am);
audio_return_t _audio_ucm_init (audio_mgr_t *am);
audio_return_t _audio_ucm_deinit (audio_mgr_t *am);
void _audio_ucm_get_device_name (audio_mgr_t *am, const char *use_case, audio_direction_t direction, const char **value);
#define _audio_ucm_update_use_case _audio_ucm_set_use_case
audio_return_t _audio_ucm_set_use_case (audio_mgr_t *am, const char *verb, const char *devices[], const char *modifiers[]);
audio_return_t _audio_ucm_set_devices (audio_mgr_t *am, const char *verb, const char *devices[]);
audio_return_t _audio_ucm_set_modifiers (audio_mgr_t *am, const char *verb, const char *modifiers[]);
int _audio_ucm_fill_device_info_list (audio_mgr_t *am, audio_device_info_t *device_info_list, const char *verb);
int _voice_pcm_open(audio_mgr_t *am);
int _voice_pcm_close(audio_mgr_t *am, uint32_t direction);
audio_return_t _audio_ucm_get_verb (audio_mgr_t *am, const char **value);
audio_return_t _audio_ucm_reset_use_case (audio_mgr_t *am);
audio_return_t _audio_util_init (audio_mgr_t *am);
audio_return_t _audio_util_deinit (audio_mgr_t *am);
audio_return_t _audio_mixer_control_set_param(audio_mgr_t *am, const char* ctl_name, snd_ctl_elem_value_t* value, int size);
audio_return_t _audio_mixer_control_set_value(audio_mgr_t *am, const char *ctl_name, int val);
audio_return_t _audio_mixer_control_set_value_string(audio_mgr_t *am, const char* ctl_name, const char* value);
audio_return_t _audio_mixer_control_get_value(audio_mgr_t *am, const char *ctl_name, int *val);
audio_return_t _audio_mixer_control_get_element(audio_mgr_t *am, const char *ctl_name, snd_hctl_elem_t **elem);
audio_return_t _audio_pcm_set_sw_params(snd_pcm_t *pcm, snd_pcm_uframes_t avail_min, uint8_t period_event);
audio_return_t _audio_pcm_set_hw_params(snd_pcm_t *pcm, audio_pcm_sample_spec_t *sample_spec, uint8_t *use_mmap, snd_pcm_uframes_t *period_size, snd_pcm_uframes_t *buffer_size);
uint32_t _convert_format(audio_sample_format_t format);
#endif
