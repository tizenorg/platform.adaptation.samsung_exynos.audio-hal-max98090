/*
 * audio-hal
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved.
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
#include "tizen-audio-impl.h"

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

static uint32_t __convert_device_string_to_enum(const char* device_str, uint32_t direction)
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

static audio_return_t __set_devices(audio_hal_t *ah, const char *verb, device_info_t *devices, uint32_t num_of_devices)
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
        new_device = __convert_device_string_to_enum(devices[i].type, devices[i].direction);
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

    audio_ret = _ucm_set_devices(ah, verb, active_devices);
    if (audio_ret)
        AUDIO_LOG_ERROR("Failed to set device: error = %d", audio_ret);

    return audio_ret;
}

static audio_return_t __update_route_ap_playback_capture(audio_hal_t *ah, audio_route_info_t *route_info)
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

    AUDIO_LOG_INFO("update_route_ap_playback_capture++ ");

    audio_ret = __set_devices(ah, verb, devices, route_info->num_of_devices);
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

static audio_return_t __update_route_voip(audio_hal_t *ah, device_info_t *devices, int32_t num_of_devices)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    const char *verb = mode_to_verb_str[VERB_NORMAL];

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(devices, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("update_route_voip++");

    audio_ret = __set_devices(ah, verb, devices, num_of_devices);
    if (audio_ret) {
        AUDIO_LOG_ERROR("Failed to set devices: error = 0x%x", audio_ret);
        return audio_ret;
    }
    /* FIXME. If necessary, set VERB_VOIP */
    ah->device.mode = VERB_NORMAL;

    /* TO DO: Set modifiers */
    return audio_ret;
}

static audio_return_t __update_route_reset(audio_hal_t *ah, uint32_t direction)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    const char *active_devices[MAX_DEVICES] = {NULL,};
    int i = 0, dev_idx = 0;

    /* FIXME: If you need to reset, set verb inactive */
    /* const char *verb = NULL; */
    /* verb = AUDIO_USE_CASE_VERB_INACTIVE; */

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("update_route_reset++, direction(0x%x)", direction);

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

    if ((audio_ret = _ucm_set_devices(ah, mode_to_verb_str[ah->device.mode], active_devices)))
        AUDIO_LOG_ERROR("failed to _ucm_set_devices(), ret(0x%x)", audio_ret);

    return audio_ret;
}

audio_return_t _audio_routing_init(audio_hal_t *ah)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    ah->device.active_in = 0x0;
    ah->device.active_out = 0x0;
    ah->device.mode = VERB_NORMAL;

    if ((audio_ret = _ucm_init(ah)))
        AUDIO_LOG_ERROR("failed to _ucm_init(), ret(0x%x)", audio_ret);

    return audio_ret;
}

audio_return_t _audio_routing_deinit(audio_hal_t *ah)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    if ((audio_ret = _ucm_deinit(ah)))
        AUDIO_LOG_ERROR("failed to _ucm_deinit(), ret(0x%x)", audio_ret);

    return audio_ret;
}

#define LOOPBACK_ARG_LATENCY_MSEC      40
#define LOOPBACK_ARG_ADJUST_TIME_SEC   3
audio_return_t audio_update_route(void *audio_handle, audio_route_info_t *info)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = (audio_hal_t *)audio_handle;
    device_info_t *devices = NULL;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(info, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("role:%s", info->role);

    devices = info->device_infos;

    if (!strncmp("voip", info->role, MAX_NAME_LEN)) {
        if ((audio_ret = __update_route_voip(ah, devices, info->num_of_devices)))
            AUDIO_LOG_WARN("update voip route return 0x%x", audio_ret);

    } else if (!strncmp("reset", info->role, MAX_NAME_LEN)) {
        if ((audio_ret = __update_route_reset(ah, devices->direction)))
            AUDIO_LOG_WARN("update reset return 0x%x", audio_ret);

    } else {
        /* send latency and adjust time for loopback */
        if (!strncmp("loopback", info->role, MAX_NAME_LEN)) {
            _audio_comm_send_message(ah, "loopback::latency", LOOPBACK_ARG_LATENCY_MSEC);
            _audio_comm_send_message(ah, "loopback::adjust_time", LOOPBACK_ARG_ADJUST_TIME_SEC);
        }
        /* need to prepare for "alarm","notification","emergency","voice-information","voice-recognition","ringtone" */
        if ((audio_ret = __update_route_ap_playback_capture(ah, info)))
            AUDIO_LOG_WARN("update playback route return 0x%x", audio_ret);
    }
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
