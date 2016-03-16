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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tizen-audio-internal.h"

#ifndef __USE_TINYALSA__
/* FIXME : To avoid build warning... */
int _snd_pcm_poll_descriptor(snd_pcm_t *pcm);
#endif
/* #define DEBUG_TIMING */

static device_type_t outDeviceTypes[] = {
    { AUDIO_DEVICE_OUT_SPEAKER, "Speaker" },
    { AUDIO_DEVICE_OUT_JACK, "Headphones" },
    { AUDIO_DEVICE_OUT_BT_SCO, "Bluetooth" },
    { AUDIO_DEVICE_OUT_AUX, "Line" },
    { AUDIO_DEVICE_OUT_HDMI, "HDMI" },
    { 0, 0 },
};

static device_type_t inDeviceTypes[] = {
    { AUDIO_DEVICE_IN_MAIN_MIC, "MainMic" },
    { AUDIO_DEVICE_IN_JACK, "HeadsetMic" },
    { AUDIO_DEVICE_IN_BT_SCO, "BT Mic" },
    { 0, 0 },
};

static const char* mode_to_verb_str[] = {
    AUDIO_USE_CASE_VERB_HIFI,
};

static uint32_t convert_device_string_to_enum(const char* device_str, uint32_t direction)
{
    uint32_t device = 0;

    if (!strncmp(device_str, "builtin-speaker", MAX_NAME_LEN)) {
        device = AUDIO_DEVICE_OUT_SPEAKER;
    } else if (!strncmp(device_str, "builtin-receiver", MAX_NAME_LEN)) {
        device = AUDIO_DEVICE_OUT_RECEIVER;
    } else if ((!strncmp(device_str, "audio-jack", MAX_NAME_LEN)) && (direction == AUDIO_DIRECTION_OUT)) {
        device = AUDIO_DEVICE_OUT_JACK;
    } else if ((!strncmp(device_str, "bt", MAX_NAME_LEN)) && (direction == AUDIO_DIRECTION_OUT)) {
        device = AUDIO_DEVICE_OUT_BT_SCO;
    } else if (!strncmp(device_str, "aux", MAX_NAME_LEN)) {
        device = AUDIO_DEVICE_OUT_AUX;
    } else if (!strncmp(device_str, "hdmi", MAX_NAME_LEN)) {
        device = AUDIO_DEVICE_OUT_HDMI;
    } else if ((!strncmp(device_str, "builtin-mic", MAX_NAME_LEN))) {
        device = AUDIO_DEVICE_IN_MAIN_MIC;
    } else if ((!strncmp(device_str, "audio-jack", MAX_NAME_LEN)) && (direction == AUDIO_DIRECTION_IN)) {
        device = AUDIO_DEVICE_IN_JACK;
    } else if ((!strncmp(device_str, "bt", MAX_NAME_LEN)) && (direction == AUDIO_DIRECTION_IN)) {
        device = AUDIO_DEVICE_IN_BT_SCO;
    } else {
        device = AUDIO_DEVICE_NONE;
    }
    AUDIO_LOG_INFO("device type(%s), enum(0x%x)", device_str, device);
    return device;
}

static audio_return_t set_devices(audio_hal_t *ah, const char *verb, device_info_t *devices, uint32_t num_of_devices)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    uint32_t new_device = 0;
    const char *active_devices[MAX_DEVICES] = {NULL,};
    int i = 0, j = 0, dev_idx = 0;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(devices, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(num_of_devices, AUDIO_ERR_PARAMETER);

    if (num_of_devices > MAX_DEVICES) {
        num_of_devices = MAX_DEVICES;
        AUDIO_LOG_ERROR("error: num_of_devices");
        return AUDIO_ERR_PARAMETER;
    }

    if (devices[0].direction == AUDIO_DIRECTION_OUT) {
        ah->device.active_out &= 0x0;
        if (ah->device.active_in) {
            /* check the active in devices */
            for (j = 0; j < inDeviceTypes[j].type; j++) {
                if (((ah->device.active_in & (~AUDIO_DEVICE_IN)) & inDeviceTypes[j].type))
                    active_devices[dev_idx++] = inDeviceTypes[j].name;
            }
        }
    } else if (devices[0].direction == AUDIO_DIRECTION_IN) {
        ah->device.active_in &= 0x0;
        if (ah->device.active_out) {
            /* check the active out devices */
            for (j = 0; j < outDeviceTypes[j].type; j++) {
                if (ah->device.active_out & outDeviceTypes[j].type)
                    active_devices[dev_idx++] = outDeviceTypes[j].name;
            }
        }
    }

    for (i = 0; i < num_of_devices; i++) {
        new_device = convert_device_string_to_enum(devices[i].type, devices[i].direction);
        if (new_device & AUDIO_DEVICE_IN) {
            for (j = 0; j < inDeviceTypes[j].type; j++) {
                if (new_device == inDeviceTypes[j].type) {
                    active_devices[dev_idx++] = inDeviceTypes[j].name;
                    ah->device.active_in |= new_device;
                }
            }
        } else {
            for (j = 0; j < outDeviceTypes[j].type; j++) {
                if (new_device == outDeviceTypes[j].type) {
                    active_devices[dev_idx++] = outDeviceTypes[j].name;
                    ah->device.active_out |= new_device;
                }
            }
        }
    }

    if (active_devices[0] == NULL) {
        AUDIO_LOG_ERROR("Failed to set device: active device is NULL");
        return AUDIO_ERR_PARAMETER;
    }

    audio_ret = _audio_ucm_set_devices(ah, verb, active_devices);
    if (audio_ret)
        AUDIO_LOG_ERROR("Failed to set device: error = %d", audio_ret);

    return audio_ret;
}

audio_return_t _audio_device_init(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    ah->device.active_in = 0x0;
    ah->device.active_out = 0x0;
    ah->device.pcm_in = NULL;
    ah->device.pcm_out = NULL;
    ah->device.mode = VERB_NORMAL;
    pthread_mutex_init(&ah->device.pcm_lock, NULL);
    ah->device.pcm_count = 0;

    return AUDIO_RET_OK;
}

audio_return_t _audio_device_deinit(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    return AUDIO_RET_OK;
}

static audio_return_t _do_route_ap_playback_capture(audio_hal_t *ah, audio_route_info_t *route_info)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    device_info_t *devices = NULL;
    const char *verb = mode_to_verb_str[VERB_NORMAL];

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(route_info, AUDIO_ERR_PARAMETER);

    devices = route_info->device_infos;

    /* To Do: Set modifiers */
    /* int mod_idx = 0; */
    /* const char *modifiers[MAX_MODIFIERS] = {NULL,}; */

    AUDIO_LOG_INFO("do_route_ap_playback_capture++ ");

    audio_ret = set_devices(ah, verb, devices, route_info->num_of_devices);
    if (audio_ret) {
        AUDIO_LOG_ERROR("Failed to set devices: error = 0x%x", audio_ret);
        return audio_ret;
    }
    ah->device.mode = VERB_NORMAL;

    /* To Do: Set modifiers */
    /*
    if (!strncmp("voice_recognition", route_info->role, MAX_NAME_LEN)) {
        modifiers[mod_idx++] = AUDIO_USE_CASE_MODIFIER_VOICESEARCH;
    } else if ((!strncmp("alarm", route_info->role, MAX_NAME_LEN))||(!strncmp("notifiication", route_info->role, MAX_NAME_LEN))) {
        if (ah->device.active_out &= AUDIO_DEVICE_OUT_JACK)
            modifiers[mod_idx++] = AUDIO_USE_CASE_MODIFIER_DUAL_MEDIA;
        else
            modifiers[mod_idx++] = AUDIO_USE_CASE_MODIFIER_MEDIA;
    } else {
        if (ah->device.active_in)
            modifiers[mod_idx++] = AUDIO_USE_CASE_MODIFIER_CAMCORDING;
        else
            modifiers[mod_idx++] = AUDIO_USE_CASE_MODIFIER_MEDIA;
    }
    audio_ret = _audio_ucm_set_modifiers (ah, verb, modifiers);
    */

    return audio_ret;
}

static audio_return_t _do_route_voip(audio_hal_t *ah, device_info_t *devices, int32_t num_of_devices)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    const char *verb = mode_to_verb_str[VERB_NORMAL];

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(devices, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("do_route_voip++");

    audio_ret = set_devices(ah, verb, devices, num_of_devices);
    if (audio_ret) {
        AUDIO_LOG_ERROR("Failed to set devices: error = 0x%x", audio_ret);
        return audio_ret;
    }
    /* FIXME. If necessary, set VERB_VOIP */
    ah->device.mode = VERB_NORMAL;

    /* TO DO: Set modifiers */
    return audio_ret;
}

static audio_return_t _do_route_reset(audio_hal_t *ah, uint32_t direction)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    const char *active_devices[MAX_DEVICES] = {NULL,};
    int i = 0, dev_idx = 0;

    /* FIXME: If you need to reset, set verb inactive */
    /* const char *verb = NULL; */
    /* verb = AUDIO_USE_CASE_VERB_INACTIVE; */

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("do_route_reset++, direction(0x%x)", direction);

    if (direction == AUDIO_DIRECTION_OUT) {
        ah->device.active_out &= 0x0;
        if (ah->device.active_in) {
            /* check the active in devices */
            for (i = 0; i < inDeviceTypes[i].type; i++) {
                if (((ah->device.active_in & (~AUDIO_DEVICE_IN)) & inDeviceTypes[i].type)) {
                    active_devices[dev_idx++] = inDeviceTypes[i].name;
                    AUDIO_LOG_INFO("added for in : %s", inDeviceTypes[i].name);
                }
            }
        }
    } else {
        ah->device.active_in &= 0x0;
        if (ah->device.active_out) {
            /* check the active out devices */
            for (i = 0; i < outDeviceTypes[i].type; i++) {
                if (ah->device.active_out & outDeviceTypes[i].type) {
                    active_devices[dev_idx++] = outDeviceTypes[i].name;
                    AUDIO_LOG_INFO("added for out : %s", outDeviceTypes[i].name);
                }
            }
        }
    }

    if (active_devices[0] == NULL) {
        AUDIO_LOG_DEBUG("active device is NULL, no need to update.");
        return AUDIO_RET_OK;
    }

    audio_ret = _audio_ucm_set_devices(ah, mode_to_verb_str[ah->device.mode], active_devices);
    if (audio_ret)
        AUDIO_LOG_ERROR("Failed to set device: error = %d", audio_ret);

    return audio_ret;
}

#define LOOPBACK_ARG_LATENCY_MSEC      40
#define LOOPBACK_ARG_ADJUST_TIME_SEC   3
audio_return_t audio_do_route(void *audio_handle, audio_route_info_t *info)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = (audio_hal_t *)audio_handle;
    device_info_t *devices = NULL;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(info, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("role:%s", info->role);

    devices = info->device_infos;

    if (!strncmp("voip", info->role, MAX_NAME_LEN)) {
        audio_ret = _do_route_voip(ah, devices, info->num_of_devices);
        if (AUDIO_IS_ERROR(audio_ret)) {
            AUDIO_LOG_WARN("set voip route return 0x%x", audio_ret);
        }
    } else if (!strncmp("reset", info->role, MAX_NAME_LEN)) {
        audio_ret = _do_route_reset(ah, devices->direction);
        if (AUDIO_IS_ERROR(audio_ret)) {
            AUDIO_LOG_WARN("set reset return 0x%x", audio_ret);
        }
    } else {
        /* send latency and adjust time for loopback */
        if (!strncmp("loopback", info->role, MAX_NAME_LEN)) {
            _audio_comm_send_message(ah, "loopback::latency", LOOPBACK_ARG_LATENCY_MSEC);
            _audio_comm_send_message(ah, "loopback::adjust_time", LOOPBACK_ARG_ADJUST_TIME_SEC);
        }
        /* need to prepare for "alarm","notification","emergency","voice-information","voice-recognition","ringtone" */
        audio_ret = _do_route_ap_playback_capture(ah, info);
        if (AUDIO_IS_ERROR(audio_ret)) {
            AUDIO_LOG_WARN("set playback route return 0x%x", audio_ret);
        }
    }
    return audio_ret;
}

audio_return_t audio_update_stream_connection_info(void *audio_handle, audio_stream_info_t *info, uint32_t is_connected)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = (audio_hal_t *)audio_handle;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(info, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("role:%s, direction:%u, idx:%u, is_connected:%d", info->role, info->direction, info->idx, is_connected);

    return audio_ret;
}

audio_return_t audio_update_route_option(void *audio_handle, audio_route_option_t *option)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = (audio_hal_t *)audio_handle;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(option, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("role:%s, name:%s, value:%d", option->role, option->name, option->value);

    return audio_ret;
}

#ifdef __USE_TINYALSA__
static struct pcm *__tinyalsa_open_device(audio_pcm_sample_spec_t *ss, size_t period_size, size_t period_count, uint32_t direction)
{
    struct pcm *pcm = NULL;
    struct pcm_config config;

    AUDIO_RETURN_NULL_IF_FAIL(ss);

    config.channels          = ss->channels;
    config.rate              = ss->rate;
    config.period_size       = period_size;
    config.period_count      = period_count;
    config.format            = ss->format;
    config.start_threshold   = period_size;
    config.stop_threshold    = 0xFFFFFFFF;
    config.silence_threshold = 0;

    AUDIO_LOG_INFO("direction %d, channels %d, rate %d, format %d, period_size %d, period_count %d", direction, ss->channels, ss->rate, ss->format, period_size, period_count);

    pcm = pcm_open((direction == AUDIO_DIRECTION_OUT) ? PLAYBACK_CARD_ID : CAPTURE_CARD_ID,
                   (direction == AUDIO_DIRECTION_OUT) ? PLAYBACK_PCM_DEVICE_ID : CAPTURE_PCM_DEVICE_ID,
                   (direction == AUDIO_DIRECTION_OUT) ? PCM_OUT : PCM_IN,
                   &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        AUDIO_LOG_ERROR("Unable to open device (%s)", pcm_get_error(pcm));
        pcm_close(pcm);
        return NULL;
    }

    return pcm;
}
#endif

audio_return_t audio_pcm_open(void *audio_handle, void **pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods)
{
#ifdef __USE_TINYALSA__
    audio_hal_t *ah;
    audio_pcm_sample_spec_t *ss;
    int err;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(sample_spec, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL((period_size > 0), AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL((periods > 0), AUDIO_ERR_PARAMETER);

    ah = (audio_hal_t *)audio_handle;
    ss = (audio_pcm_sample_spec_t *)sample_spec;
    ss->format = _convert_format((audio_sample_format_t)ss->format);

    *pcm_handle = __tinyalsa_open_device(ss, (size_t)period_size, (size_t)periods, direction);
    if (*pcm_handle == NULL) {
        AUDIO_LOG_ERROR("Error opening PCM device");
        return AUDIO_ERR_RESOURCE;
    }

    if ((err = pcm_prepare((struct pcm *)*pcm_handle)) != 0) {
        AUDIO_LOG_ERROR("Error prepare PCM device : %d", err);
    }

    ah->device.pcm_count++;
    AUDIO_LOG_INFO("Opening PCM handle 0x%x", *pcm_handle);
#else  /* alsa-lib */
    audio_hal_t *ah;
    int err, mode;
    char *device_name = NULL;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(sample_spec, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL((period_size > 0), AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL((periods > 0), AUDIO_ERR_PARAMETER);

    ah = (audio_hal_t *)audio_handle;
    mode =  SND_PCM_NONBLOCK | SND_PCM_NO_AUTO_RESAMPLE | SND_PCM_NO_AUTO_CHANNELS | SND_PCM_NO_AUTO_FORMAT;

    if (direction == AUDIO_DIRECTION_OUT)
        device_name = PLAYBACK_PCM_DEVICE;
    else if (direction == AUDIO_DIRECTION_IN)
        device_name = CAPTURE_PCM_DEVICE;
    else {
        AUDIO_LOG_ERROR("Error get device_name, direction : %d", direction);
        return AUDIO_ERR_RESOURCE;
    }

    if ((err = snd_pcm_open((snd_pcm_t **)pcm_handle, device_name, (direction == AUDIO_DIRECTION_OUT) ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE, mode)) < 0) {
        AUDIO_LOG_ERROR("Error opening PCM device %s : %s", device_name, snd_strerror(err));
        return AUDIO_ERR_RESOURCE;
    }

    if ((err = audio_pcm_set_params(audio_handle, *pcm_handle, direction, sample_spec, period_size, periods)) != AUDIO_RET_OK) {
        AUDIO_LOG_ERROR("Failed to set pcm parameters : %d", err);
        return err;
    }

    ah->device.pcm_count++;
    AUDIO_LOG_INFO("Opening PCM handle 0x%x, PCM device %s", *pcm_handle, device_name);
#endif

    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_start(void *audio_handle, void *pcm_handle)
{
    int err;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

#ifdef __USE_TINYALSA__
    if ((err = pcm_start(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error starting PCM handle : %d", err);
        return AUDIO_ERR_RESOURCE;
    }
#else  /* alsa-lib */
    if ((err = snd_pcm_start(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error starting PCM handle : %s", snd_strerror(err));
        return AUDIO_ERR_RESOURCE;
    }
#endif

    AUDIO_LOG_INFO("PCM handle 0x%x start", pcm_handle);
    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_stop(void *audio_handle, void *pcm_handle)
{
    int err;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

#ifdef __USE_TINYALSA__
    if ((err = pcm_stop(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error stopping PCM handle : %d", err);
        return AUDIO_ERR_RESOURCE;
    }
#else  /* alsa-lib */
    if ((err = snd_pcm_drop(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error stopping PCM handle : %s", snd_strerror(err));
        return AUDIO_ERR_RESOURCE;
    }
#endif

    AUDIO_LOG_INFO("PCM handle 0x%x stop", pcm_handle);
    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_close(void *audio_handle, void *pcm_handle)
{
    audio_hal_t *ah = (audio_hal_t *)audio_handle;
    int err;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("Try to close PCM handle 0x%x", pcm_handle);

#ifdef __USE_TINYALSA__
    if ((err = pcm_close(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error closing PCM handle : %d", err);
        return AUDIO_ERR_RESOURCE;
    }
#else  /* alsa-lib */
    if ((err = snd_pcm_close(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error closing PCM handle : %s", snd_strerror(err));
        return AUDIO_ERR_RESOURCE;
    }
#endif

    pcm_handle = NULL;
    ah->device.pcm_count--;
    AUDIO_LOG_INFO("PCM handle close success (count:%d)", ah->device.pcm_count);

    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_avail(void *audio_handle, void *pcm_handle, uint32_t *avail)
{
#ifdef __USE_TINYALSA__
    struct timespec tspec;
    unsigned int frames_avail = 0;
    int err;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(avail, AUDIO_ERR_PARAMETER);

    err = pcm_get_htimestamp(pcm_handle, &frames_avail, &tspec);
    if (err < 0) {
        AUDIO_LOG_ERROR("Could not get avail and timespec at PCM handle 0x%x : %d", pcm_handle, err);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("avail = %d", frames_avail);
#endif

    *avail = (uint32_t)frames_avail;
#else  /* alsa-lib */
    snd_pcm_sframes_t frames_avail;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(avail, AUDIO_ERR_PARAMETER);

    if ((frames_avail = snd_pcm_avail(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Could not get avail at PCM handle 0x%x : %d", pcm_handle, frames_avail);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("avail = %d", frames_avail);
#endif

    *avail = (uint32_t)frames_avail;
#endif

    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_write(void *audio_handle, void *pcm_handle, const void *buffer, uint32_t frames)
{
#ifdef __USE_TINYALSA__
    int err;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    err = pcm_write(pcm_handle, buffer, pcm_frames_to_bytes(pcm_handle, (unsigned int)frames));
    if (err < 0) {
        AUDIO_LOG_ERROR("Failed to write pcm : %d", err);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("audio_pcm_write = %d", frames);
#endif
#else  /* alsa-lib */
    snd_pcm_sframes_t frames_written;

    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    frames_written = snd_pcm_writei(pcm_handle, buffer, (snd_pcm_uframes_t) frames);
    if (frames_written < 0) {
        AUDIO_LOG_ERROR("Failed to write pcm : %d", frames_written);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("audio_pcm_write = (%d / %d)", frames_written, frames);
#endif
#endif

    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_read(void *audio_handle, void *pcm_handle, void *buffer, uint32_t frames)
{
#ifdef __USE_TINYALSA__
    int err;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    err = pcm_read(pcm_handle, buffer, pcm_frames_to_bytes(pcm_handle, (unsigned int)frames));
    if (err < 0) {
        AUDIO_LOG_ERROR("Failed to read pcm : %d", err);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("audio_pcm_read = %d", frames);
#endif
#else  /* alsa-lib */
    snd_pcm_sframes_t frames_read;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    frames_read = snd_pcm_readi(pcm_handle, buffer, (snd_pcm_uframes_t)frames);
    if (frames_read < 0) {
        AUDIO_LOG_ERROR("Failed to read pcm : %d", frames_read);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("audio_pcm_read = (%d / %d)", frames_read, frames);
#endif
#endif

    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_get_fd(void *audio_handle, void *pcm_handle, int *fd)
{
    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(fd, AUDIO_ERR_PARAMETER);
    /* we use an internal API of the (tiny)alsa library, so it causes warning message during compile */
#ifdef __USE_TINYALSA__
    *fd = _pcm_poll_descriptor((struct pcm *)pcm_handle);
#else  /* alsa-lib */
    *fd = _snd_pcm_poll_descriptor((snd_pcm_t *)pcm_handle);
#endif
    return AUDIO_RET_OK;
}

#ifdef __USE_TINYALSA__
static int __tinyalsa_pcm_recover(struct pcm *pcm, int err)
{
    if (err > 0)
        err = -err;
    if (err == -EINTR)  /* nothing to do, continue */
        return 0;
    if (err == -EPIPE) {
        AUDIO_LOG_INFO("XRUN occurred");
        err = pcm_prepare(pcm);
        if (err < 0) {
            AUDIO_LOG_ERROR("Could not recover from XRUN occurred, prepare failed : %d", err);
            return err;
        }
        return 0;
    }
    if (err == -ESTRPIPE) {
        /* tinyalsa does not support pcm resume, dont't care suspend case */
        AUDIO_LOG_ERROR("Could not recover from suspend : %d", err);
        return err;
    }
    return err;
}
#endif

audio_return_t audio_pcm_recover(void *audio_handle, void *pcm_handle, int revents)
{
    int state, err;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    if (revents & POLLERR)
        AUDIO_LOG_DEBUG("Got POLLERR from ALSA");
    if (revents & POLLNVAL)
        AUDIO_LOG_DEBUG("Got POLLNVAL from ALSA");
    if (revents & POLLHUP)
        AUDIO_LOG_DEBUG("Got POLLHUP from ALSA");
    if (revents & POLLPRI)
        AUDIO_LOG_DEBUG("Got POLLPRI from ALSA");
    if (revents & POLLIN)
        AUDIO_LOG_DEBUG("Got POLLIN from ALSA");
    if (revents & POLLOUT)
        AUDIO_LOG_DEBUG("Got POLLOUT from ALSA");

#ifdef __USE_TINYALSA__
    state = pcm_state(pcm_handle);
    AUDIO_LOG_DEBUG("PCM state is %d", state);

    switch (state) {
        case PCM_STATE_XRUN:
            if ((err = __tinyalsa_pcm_recover(pcm_handle, -EPIPE)) != 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP and XRUN : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;

        case PCM_STATE_SUSPENDED:
            if ((err = __tinyalsa_pcm_recover(pcm_handle, -ESTRPIPE)) != 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP and SUSPENDED : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;

        default:
            pcm_stop(pcm_handle);
            if ((err = pcm_prepare(pcm_handle)) < 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP with pcm_prepare() : %d", err);
                return AUDIO_ERR_IOCTL;
            }
    }
#else  /* alsa-lib */
    state = snd_pcm_state(pcm_handle);
    AUDIO_LOG_DEBUG("PCM state is %s", snd_pcm_state_name(state));

    /* Try to recover from this error */

    switch (state) {
        case SND_PCM_STATE_XRUN:
            if ((err = snd_pcm_recover(pcm_handle, -EPIPE, 1)) != 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP and XRUN : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;

        case SND_PCM_STATE_SUSPENDED:
            if ((err = snd_pcm_recover(pcm_handle, -ESTRPIPE, 1)) != 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP and SUSPENDED : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;

        default:
            snd_pcm_drop(pcm_handle);
            if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP with snd_pcm_prepare() : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;
    }
#endif

    AUDIO_LOG_DEBUG("audio_pcm_recover");
    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_get_params(void *audio_handle, void *pcm_handle, uint32_t direction, void **sample_spec, uint32_t *period_size, uint32_t *periods)
{
#ifdef __USE_TINYALSA__
    audio_pcm_sample_spec_t *ss;
    unsigned int _period_size, _buffer_size, _periods, _format, _rate, _channels;
    unsigned int _start_threshold, _stop_threshold, _silence_threshold;
    struct pcm_config *config;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(sample_spec, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(period_size, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(periods, AUDIO_ERR_PARAMETER);
    ss = (audio_pcm_sample_spec_t *)*sample_spec;

    /* we use an internal API of the tiny alsa library, so it causes warning message during compile */
    _pcm_config(pcm_handle, &config);

    *period_size = config->period_size;
    *periods     = config->period_count;
    _buffer_size = config->period_size * config->period_count;
    ss->format   = config->format;
    ss->rate     = config->rate;
    ss->channels = config->channels;
    _start_threshold   = config->start_threshold;
    _stop_threshold    = config->stop_threshold;
    _silence_threshold = config->silence_threshold;

    AUDIO_LOG_DEBUG("audio_pcm_get_params (handle 0x%x, format %d, rate %d, channels %d, period_size %d, periods %d, buffer_size %d)", pcm_handle, config->format, config->rate, config->channels, config->period_size, config->period_count, _buffer_size);
#else  /* alsa-lib */
    int err;
    audio_pcm_sample_spec_t *ss;
    int dir;
    snd_pcm_uframes_t _period_size, _buffer_size;
    snd_pcm_format_t _format;
    unsigned int _rate, _channels;
    snd_pcm_uframes_t _start_threshold, _stop_threshold, _silence_threshold, _avail_min;
    unsigned int _periods;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(sample_spec, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(period_size, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(periods, AUDIO_ERR_PARAMETER);
    ss = (audio_pcm_sample_spec_t *)*sample_spec;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    if ((err = snd_pcm_hw_params_current(pcm_handle, hwparams)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_current() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_get_period_size(hwparams, &_period_size, &dir)) < 0 ||
        (err = snd_pcm_hw_params_get_buffer_size(hwparams, &_buffer_size)) < 0 ||
        (err = snd_pcm_hw_params_get_periods(hwparams, &_periods, &dir)) < 0 ||
        (err = snd_pcm_hw_params_get_format(hwparams, &_format)) < 0 ||
        (err = snd_pcm_hw_params_get_rate(hwparams, &_rate, &dir)) < 0 ||
        (err = snd_pcm_hw_params_get_channels(hwparams, &_channels)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_get_{period_size|buffer_size|periods|format|rate|channels}() failed : %s", err);
        return AUDIO_ERR_PARAMETER;
    }

    *period_size = _period_size;
    *periods     = _periods;
    ss->format   = _format;
    ss->rate     = _rate;
    ss->channels = _channels;

    if ((err = snd_pcm_sw_params_current(pcm_handle, swparams)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_sw_params_current() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_get_start_threshold(swparams, &_start_threshold)) < 0  ||
        (err = snd_pcm_sw_params_get_stop_threshold(swparams, &_stop_threshold)) < 0  ||
        (err = snd_pcm_sw_params_get_silence_threshold(swparams, &_silence_threshold)) < 0  ||
        (err = snd_pcm_sw_params_get_avail_min(swparams, &_avail_min)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_sw_params_get_{start_threshold|stop_threshold|silence_threshold|avail_min}() failed : %s", err);
    }

    AUDIO_LOG_DEBUG("audio_pcm_get_params (handle 0x%x, format %d, rate %d, channels %d, period_size %d, periods %d, buffer_size %d)", pcm_handle, _format, _rate, _channels, _period_size, _periods, _buffer_size);
#endif

    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_set_params(void *audio_handle, void *pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods)
{
#ifdef __USE_TINYALSA__
    /* Parameters are only acceptable in pcm_open() function */
    AUDIO_LOG_DEBUG("audio_pcm_set_params");
#else  /* alsa-lib */
    int err;
    audio_pcm_sample_spec_t ss;
    snd_pcm_uframes_t _buffer_size;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(sample_spec, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(period_size, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(periods, AUDIO_ERR_PARAMETER);
    ss = *(audio_pcm_sample_spec_t *)sample_spec;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    /* Set hw params */
    if ((err = snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_any() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_rate_resample(pcm_handle, hwparams, 0)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_rate_resample() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_access() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    ss.format = _convert_format((audio_sample_format_t)ss.format);
    if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, ss.format)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_format() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_rate(pcm_handle, hwparams, ss.rate, 0)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_rate() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, ss.channels)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_channels(%u) failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_period_size(pcm_handle, hwparams, period_size, 0)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_period_size(%u) failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_periods(%u) failed : %d", periods, err);
        return AUDIO_ERR_PARAMETER;
    }

    _buffer_size = period_size * periods;
    if ((err = snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, _buffer_size)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_buffer_size(%u) failed : %d", periods * periods, err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params failed : %d", err);
        return AUDIO_ERR_IOCTL;
    }

    /* Set sw params */
    if ((err = snd_pcm_sw_params_current(pcm_handle, swparams)) < 0) {
        AUDIO_LOG_ERROR("Unable to determine current swparams : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_set_tstamp_mode(pcm_handle, swparams, SND_PCM_TSTAMP_ENABLE)) < 0) {
        AUDIO_LOG_ERROR("Unable to enable time stamping : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, 0xFFFFFFFF)) < 0) {
        AUDIO_LOG_ERROR("Unable to set stop threshold : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, period_size / 2)) < 0) {
        AUDIO_LOG_ERROR("Unable to set start threshold : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, 1024)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_sw_params_set_avail_min() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params(pcm_handle, swparams)) < 0) {
        AUDIO_LOG_ERROR("Unable to set sw params : %d", err);
        return AUDIO_ERR_IOCTL;
    }

    /* Prepare device */
    if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_prepare() failed : %d", err);
        return AUDIO_ERR_IOCTL;
    }

    AUDIO_LOG_DEBUG("audio_pcm_set_params (handle 0x%x, format %d, rate %d, channels %d, period_size %d, periods %d, buffer_size %d)", pcm_handle, ss.format, ss.rate, ss.channels, period_size, periods, _buffer_size);
#endif

    return AUDIO_RET_OK;
}
