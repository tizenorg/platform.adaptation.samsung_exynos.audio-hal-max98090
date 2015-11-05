#ifndef footizenaudiofoo
#define footizenaudiofoo

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define AUDIO_REVISION                  1

/* Error code */

#define AUDIO_IS_ERROR(ret)             (ret < 0)

typedef enum audio_return {
    AUDIO_RET_OK                        = 0,
    AUDIO_ERR_UNDEFINED                 = (int32_t)0x80001000,
    AUDIO_ERR_RESOURCE                  = (int32_t)0x80001001,
    AUDIO_ERR_PARAMETER                 = (int32_t)0x80001002,
    AUDIO_ERR_IOCTL                     = (int32_t)0x80001003,
    AUDIO_ERR_NOT_IMPLEMENTED           = (int32_t)0x80001004,
} audio_return_t ;

/* Direction */
typedef enum audio_direction {
    AUDIO_DIRECTION_IN,                 /**< Capture */
    AUDIO_DIRECTION_OUT,                /**< Playback */
} audio_direction_t;

typedef enum audio_route_flag {
    AUDIO_ROUTE_FLAG_NONE               = 0,
    AUDIO_ROUTE_FLAG_MUTE_POLICY        = 0x00000001,
    AUDIO_ROUTE_FLAG_DUAL_OUT           = 0x00000002,
    AUDIO_ROUTE_FLAG_NOISE_REDUCTION    = 0x00000010,
    AUDIO_ROUTE_FLAG_EXTRA_VOL          = 0x00000020,
    AUDIO_ROUTE_FLAG_NETWORK_WB         = 0x00000040,
    AUDIO_ROUTE_FLAG_BT_WB              = 0x00000100,
    AUDIO_ROUTE_FLAG_BT_NREC            = 0x00000200,
    AUDIO_ROUTE_FLAG_VOICE_COMMAND      = 0x00040000,
} audio_route_flag_t;

typedef enum audio_device_api {
    AUDIO_DEVICE_API_UNKNOWN,
    AUDIO_DEVICE_API_ALSA,
    AUDIO_DEVICE_API_BLUEZ,
} audio_device_api_t;

typedef enum audio_device_param {
    AUDIO_DEVICE_PARAM_NONE,
    AUDIO_DEVICE_PARAM_CHANNELS,
    AUDIO_DEVICE_PARAM_SAMPLERATE,
    AUDIO_DEVICE_PARAM_FRAGMENT_SIZE,
    AUDIO_DEVICE_PARAM_FRAGMENT_NB,
    AUDIO_DEVICE_PARAM_START_THRESHOLD,
    AUDIO_DEVICE_PARAM_USE_MMAP,
    AUDIO_DEVICE_PARAM_USE_TSCHED,
    AUDIO_DEVICE_PARAM_TSCHED_BUF_SIZE,
    AUDIO_DEVICE_PARAM_SUSPEND_TIMEOUT,
    AUDIO_DEVICE_PARAM_ALTERNATE_RATE,
    AUDIO_DEVICE_PARAM_MAX,
} audio_device_param_t;

/* audio format */
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
}   audio_sample_format_t;

/* audio latency */
static const char* AUDIO_LATENCY_LOW  = "low";
static const char* AUDIO_LATENCY_MID  = "mid";
static const char* AUDIO_LATENCY_HIGH = "high";
static const char* AUDIO_LATENCY_VOIP = "voip";

typedef struct audio_device_param_info {
    audio_device_param_t param;
    union {
        int64_t s64_v;
        uint64_t u64_v;
        int32_t s32_v;
        uint32_t u32_v;
    };
} audio_device_param_info_t;

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

typedef struct device_info {
    const char *type;
    uint32_t direction;
    uint32_t id;
} device_info_t;

typedef struct audio_volume_info {
    const char *type;
    const char *gain;
    uint32_t direction;
} audio_volume_info_t ;

typedef struct audio_route_info {
    const char *role;
    device_info_t *device_infos;
    uint32_t num_of_devices;
} audio_route_info_t;

typedef struct audio_route_option {
    const char *role;
    const char *name;
    int32_t value;
} audio_route_option_t;

typedef struct audio_stream_info {
    const char *role;
    uint32_t direction;
    uint32_t idx;
} audio_stream_info_t ;

/* Stream */

typedef enum audio_volume {
    AUDIO_VOLUME_TYPE_SYSTEM,           /**< System volume type */
    AUDIO_VOLUME_TYPE_NOTIFICATION,     /**< Notification volume type */
    AUDIO_VOLUME_TYPE_ALARM,            /**< Alarm volume type */
    AUDIO_VOLUME_TYPE_RINGTONE,         /**< Ringtone volume type */
    AUDIO_VOLUME_TYPE_MEDIA,            /**< Media volume type */
    AUDIO_VOLUME_TYPE_CALL,             /**< Call volume type */
    AUDIO_VOLUME_TYPE_VOIP,             /**< VOIP volume type */
    AUDIO_VOLUME_TYPE_VOICE,            /**< Voice volume type */
    AUDIO_VOLUME_TYPE_FIXED,            /**< Volume type for fixed acoustic level */
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

/* Overall */

typedef struct audio_cb_interface {
    audio_return_t (*load_device)(void *platform_data, audio_device_info_t *device_info, audio_device_param_info_t *params);
    audio_return_t (*open_device)(void *platform_data, audio_device_info_t *device_info, audio_device_param_info_t *params);
    audio_return_t (*close_all_devices)(void *platform_data);
    audio_return_t (*close_device)(void *platform_data, audio_device_info_t *device_info);
    audio_return_t (*unload_device)(void *platform_data, audio_device_info_t *device_info);
} audio_cb_interface_t;

typedef struct audio_interface {
    audio_return_t (*init)(void **userdata, void *platform_data);
    audio_return_t (*deinit)(void **userdata);
    audio_return_t (*reset_volume)(void **userdata);
    audio_return_t (*get_volume_level_max)(void *userdata, audio_volume_info_t *info, uint32_t *level);
    audio_return_t (*get_volume_level)(void *userdata, audio_volume_info_t *info, uint32_t *level);
    audio_return_t (*set_volume_level)(void *userdata, audio_volume_info_t *info, uint32_t level);
    audio_return_t (*get_volume_value)(void *userdata, audio_volume_info_t *info, uint32_t level, double *value);
    audio_return_t (*get_volume_mute)(void *userdata, audio_volume_info_t *info, uint32_t *mute);
    audio_return_t (*set_volume_mute)(void *userdata, audio_volume_info_t *info, uint32_t mute);
    audio_return_t (*do_route)(void *userdata, audio_route_info_t *info);
    audio_return_t (*update_route_option)(void *userdata, audio_route_option_t *option);
    audio_return_t (*update_stream_connection_info) (void *userdata, audio_stream_info_t *info, uint32_t is_connected);
    audio_return_t (*alsa_pcm_open)(void *userdata, void **pcm_handle, char *device_name, uint32_t direction, int mode);
    audio_return_t (*alsa_pcm_close)(void *userdata, void *pcm_handle);

    /* Interface of PCM device */
    audio_return_t (*pcm_open)(void *userdata, void **pcm_handle, void *sample_spec, uint32_t direction);
    audio_return_t (*pcm_start)(void *userdata, void *pcm_handle);
    audio_return_t (*pcm_stop)(void *userdata, void *pcm_handle);
    audio_return_t (*pcm_close)(void *userdata, void *pcm_handle);
    audio_return_t (*pcm_avail)(void *userdata, void *pcm_handle, uint32_t *avail);
    audio_return_t (*pcm_write)(void *userdata, void *pcm_handle, const void *buffer, uint32_t frames);
    audio_return_t (*pcm_read)(void *userdata, void *pcm_handle, void *buffer, uint32_t frames);
    audio_return_t (*pcm_get_fd)(void *userdata, void *pcm_handle, int *fd);
    audio_return_t (*pcm_recover)(void *userdata, void *pcm_handle, int err);
    audio_return_t (*pcm_get_params)(void *userdata, void *pcm_handle, uint32_t direction, void **sample_spec, uint32_t *period_size, uint32_t *periods);
    audio_return_t (*pcm_set_params)(void *userdata, void *pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods);

    audio_return_t (*get_buffer_attr)(void *userdata, uint32_t direction, const char *latency, uint32_t samplerate, audio_sample_format_t format, uint32_t channels, uint32_t *maxlength, uint32_t *tlength, uint32_t *prebuf, uint32_t* minreq, uint32_t *fragsize);
    audio_return_t (*set_callback)(void *userdata, audio_cb_interface_t *cb_interface);
} audio_interface_t;

int audio_get_revision (void);
audio_return_t audio_init (void **userdata, void *platform_data);
audio_return_t audio_deinit (void **userdata);
audio_return_t audio_reset_volume (void **userdata);
audio_return_t audio_get_volume_level_max (void *userdata, audio_volume_info_t *info, uint32_t *level);
audio_return_t audio_get_volume_level (void *userdata, audio_volume_info_t *info, uint32_t *level);
audio_return_t audio_set_volume_level (void *userdata, audio_volume_info_t *info, uint32_t level);
audio_return_t audio_get_volume_value (void *userdata, audio_volume_info_t *info, uint32_t level, double *value);
audio_return_t audio_get_volume_mute (void *userdata, audio_volume_info_t *info, uint32_t *mute);
audio_return_t audio_set_volume_mute (void *userdata, audio_volume_info_t *info, uint32_t mute);
audio_return_t audio_do_route (void *userdata, audio_route_info_t *info);
audio_return_t audio_update_route_option (void *userdata, audio_route_option_t *option);
audio_return_t audio_update_stream_connection_info (void *userdata, audio_stream_info_t *info, uint32_t is_connected);
audio_return_t audio_alsa_pcm_open (void *userdata, void **pcm_handle, char *device_name, uint32_t direction, int mode);
audio_return_t audio_alsa_pcm_close (void *userdata, void *pcm_handle);
audio_return_t audio_pcm_open (void *userdata, void **pcm_handle, void *sample_spec, uint32_t direction);
audio_return_t audio_pcm_start (void *userdata, void *pcm_handle);
audio_return_t audio_pcm_stop (void *userdata, void *pcm_handle);
audio_return_t audio_pcm_close (void *userdata, void *pcm_handle);
audio_return_t audio_pcm_avail (void *userdata, void *pcm_handle, uint32_t *avail);
audio_return_t audio_pcm_write (void *userdata, void *pcm_handle, const void *buffer, uint32_t frames);
audio_return_t audio_pcm_read (void *userdata, void *pcm_handle, void *buffer, uint32_t frames);
audio_return_t audio_pcm_get_fd(void *userdata, void *pcm_handle, int *fd);
audio_return_t audio_pcm_recover(void *userdata, void *pcm_handle, int err);
audio_return_t audio_pcm_get_params(void *userdata, void *pcm_handle, uint32_t direction, void **sample_spec, uint32_t *period_size, uint32_t *periods);
audio_return_t audio_pcm_set_params(void *userdata, void *pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods);
audio_return_t audio_get_buffer_attr(void *userdata, uint32_t direction, const char *latency, uint32_t samplerate, audio_sample_format_t format, uint32_t channels, uint32_t *maxlength, uint32_t *tlength, uint32_t *prebuf, uint32_t* minreq, uint32_t *fragsize);
audio_return_t audio_set_callback (void *userdata, audio_cb_interface_t *cb_interface);
#endif
