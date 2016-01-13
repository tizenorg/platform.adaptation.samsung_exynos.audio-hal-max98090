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

#include <stdint.h>

/* Error code */
#define AUDIO_IS_ERROR(ret)             (ret < 0)
typedef enum audio_return {
    AUDIO_RET_OK                        = 0,
    AUDIO_ERR_UNDEFINED                 = (int32_t)0x80001000,
    AUDIO_ERR_RESOURCE                  = (int32_t)0x80001001,
    AUDIO_ERR_PARAMETER                 = (int32_t)0x80001002,
    AUDIO_ERR_IOCTL                     = (int32_t)0x80001003,
    AUDIO_ERR_INVALID_STATE             = (int32_t)0x80001004,
    AUDIO_ERR_INTERNAL                  = (int32_t)0x80001005,

    AUDIO_ERR_NOT_IMPLEMENTED           = (int32_t)0x80001100,
} audio_return_t ;

typedef enum audio_direction {
    AUDIO_DIRECTION_IN,                 /**< Capture */
    AUDIO_DIRECTION_OUT,                /**< Playback */
} audio_direction_t;

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

/* Overall */
typedef struct audio_interface {
    audio_return_t (*init)(void **audio_handle);
    audio_return_t (*deinit)(void *audio_handle);
    /* Volume */
    audio_return_t (*get_volume_level_max)(void *audio_handle, audio_volume_info_t *info, uint32_t *level);
    audio_return_t (*get_volume_level)(void *audio_handle, audio_volume_info_t *info, uint32_t *level);
    audio_return_t (*set_volume_level)(void *audio_handle, audio_volume_info_t *info, uint32_t level);
    audio_return_t (*get_volume_value)(void *audio_handle, audio_volume_info_t *info, uint32_t level, double *value);
    audio_return_t (*get_volume_mute)(void *audio_handle, audio_volume_info_t *info, uint32_t *mute);
    audio_return_t (*set_volume_mute)(void *audio_handle, audio_volume_info_t *info, uint32_t mute);
    /* Routing */
    audio_return_t (*do_route)(void *audio_handle, audio_route_info_t *info);
    audio_return_t (*update_route_option)(void *audio_handle, audio_route_option_t *option);
    audio_return_t (*update_stream_connection_info) (void *audio_handle, audio_stream_info_t *info, uint32_t is_connected);
    /* Buffer Attribute */
    audio_return_t (*get_buffer_attr)(void *audio_handle, uint32_t direction, const char *latency, uint32_t samplerate, int format, uint32_t channels,
                                      uint32_t *maxlength, uint32_t *tlength, uint32_t *prebuf, uint32_t* minreq, uint32_t *fragsize);
    /* Interface of PCM device */
    audio_return_t (*pcm_open)(void *audio_handle, void **pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods);
    audio_return_t (*pcm_start)(void *audio_handle, void *pcm_handle);
    audio_return_t (*pcm_stop)(void *audio_handle, void *pcm_handle);
    audio_return_t (*pcm_close)(void *audio_handle, void *pcm_handle);
    audio_return_t (*pcm_avail)(void *audio_handle, void *pcm_handle, uint32_t *avail);
    audio_return_t (*pcm_write)(void *audio_handle, void *pcm_handle, const void *buffer, uint32_t frames);
    audio_return_t (*pcm_read)(void *audio_handle, void *pcm_handle, void *buffer, uint32_t frames);
    audio_return_t (*pcm_get_fd)(void *audio_handle, void *pcm_handle, int *fd);
    audio_return_t (*pcm_recover)(void *audio_handle, void *pcm_handle, int revents);
    audio_return_t (*pcm_get_params)(void *audio_handle, void *pcm_handle, uint32_t direction, void **sample_spec, uint32_t *period_size, uint32_t *periods);
    audio_return_t (*pcm_set_params)(void *audio_handle, void *pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods);
} audio_interface_t;

audio_return_t audio_init(void **audio_handle);
audio_return_t audio_deinit(void *audio_handle);
audio_return_t audio_get_volume_level_max(void *audio_handle, audio_volume_info_t *info, uint32_t *level);
audio_return_t audio_get_volume_level(void *audio_handle, audio_volume_info_t *info, uint32_t *level);
audio_return_t audio_set_volume_level(void *audio_handle, audio_volume_info_t *info, uint32_t level);
audio_return_t audio_get_volume_value(void *audio_handle, audio_volume_info_t *info, uint32_t level, double *value);
audio_return_t audio_get_volume_mute(void *audio_handle, audio_volume_info_t *info, uint32_t *mute);
audio_return_t audio_set_volume_mute(void *audio_handle, audio_volume_info_t *info, uint32_t mute);
audio_return_t audio_do_route(void *audio_handle, audio_route_info_t *info);
audio_return_t audio_update_route_option(void *audio_handle, audio_route_option_t *option);
audio_return_t audio_update_stream_connection_info(void *audio_handle, audio_stream_info_t *info, uint32_t is_connected);
audio_return_t audio_get_buffer_attr(void *audio_handle, uint32_t direction, const char *latency, uint32_t samplerate, int format, uint32_t channels,
                                     uint32_t *maxlength, uint32_t *tlength, uint32_t *prebuf, uint32_t* minreq, uint32_t *fragsize);
audio_return_t audio_pcm_open(void *audio_handle, void **pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods);
audio_return_t audio_pcm_start(void *audio_handle, void *pcm_handle);
audio_return_t audio_pcm_stop(void *audio_handle, void *pcm_handle);
audio_return_t audio_pcm_close(void *audio_handle, void *pcm_handle);
audio_return_t audio_pcm_avail(void *audio_handle, void *pcm_handle, uint32_t *avail);
audio_return_t audio_pcm_write(void *audio_handle, void *pcm_handle, const void *buffer, uint32_t frames);
audio_return_t audio_pcm_read(void *audio_handle, void *pcm_handle, void *buffer, uint32_t frames);
audio_return_t audio_pcm_get_fd(void *audio_handle, void *pcm_handle, int *fd);
audio_return_t audio_pcm_recover(void *audio_handle, void *pcm_handle, int revents);
audio_return_t audio_pcm_get_params(void *audio_handle, void *pcm_handle, uint32_t direction, void **sample_spec, uint32_t *period_size, uint32_t *periods);
audio_return_t audio_pcm_set_params(void *audio_handle, void *pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods);
#endif
