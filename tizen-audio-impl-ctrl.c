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
#include <pthread.h>

#include "tizen-audio-internal.h"

#ifdef __MIXER_PARAM_DUMP
static void __dump_mixer_param(char *dump, long *param, int size)
{
    int i, len;

    for (i = 0; i < size; i++) {
        len = sprintf(dump, "%ld", *param);
        if (len > 0)
            dump += len;
        if (i != size -1) {
            *dump++ = ',';
        }

        param++;
    }
    *dump = '\0';
}
#endif

audio_return_t _control_init(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    pthread_mutex_init(&(ah->mixer.mutex), NULL);
    return AUDIO_RET_OK;
}

audio_return_t _control_deinit(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    pthread_mutex_destroy(&(ah->mixer.mutex));
    return AUDIO_RET_OK;
}

audio_return_t _mixer_control_set_param(audio_hal_t *ah, const char* ctl_name, snd_ctl_elem_value_t* param, int size)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    /* TODO. */
    return AUDIO_RET_OK;
}

audio_return_t _mixer_control_get_value(audio_hal_t *ah, const char *ctl_name, int *val)
{
    snd_ctl_t *handle;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_type_t type;

    int ret = 0, count = 0, i = 0;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    pthread_mutex_lock(&(ah->mixer.mutex));

    ret = snd_ctl_open(&handle, ALSA_DEFAULT_CARD, 0);
    if (ret < 0) {
        AUDIO_LOG_ERROR("snd_ctl_open error, %s\n", snd_strerror(ret));
        pthread_mutex_unlock(&(ah->mixer.mutex));
        return AUDIO_ERR_IOCTL;
    }

    // Get Element Info

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&control);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, ctl_name);

    snd_ctl_elem_info_set_id(info, id);
    if (snd_ctl_elem_info(handle, info) < 0) {
        AUDIO_LOG_ERROR("Cannot find control element: %s\n", ctl_name);
        goto close;
    }
    snd_ctl_elem_info_get_id(info, id);

    type = snd_ctl_elem_info_get_type(info);
    count = snd_ctl_elem_info_get_count(info);

    snd_ctl_elem_value_set_id(control, id);

    if (snd_ctl_elem_read(handle, control) < 0) {
        AUDIO_LOG_ERROR("snd_ctl_elem_read failed \n");
        goto close;
}

    switch (type) {
    case SND_CTL_ELEM_TYPE_BOOLEAN:
        *val = snd_ctl_elem_value_get_boolean(control, i);
        break;
    case SND_CTL_ELEM_TYPE_INTEGER:
        for (i = 0; i < count; i++)
        *val = snd_ctl_elem_value_get_integer(control, i);
        break;
    case SND_CTL_ELEM_TYPE_ENUMERATED:
        for (i = 0; i < count; i++)
        *val = snd_ctl_elem_value_get_enumerated(control, i);
        break;
    default:
        AUDIO_LOG_WARN("unsupported control element type\n");
        goto close;
    }

    snd_ctl_close(handle);

#ifdef AUDIO_DEBUG
    AUDIO_LOG_INFO("get mixer(%s) = %d success", ctl_name, *val);
#endif

    pthread_mutex_unlock(&(ah->mixer.mutex));
    return AUDIO_RET_OK;

close:
    AUDIO_LOG_ERROR("Error\n");
    snd_ctl_close(handle);
    pthread_mutex_unlock(&(ah->mixer.mutex));
    return AUDIO_ERR_UNDEFINED;
}

audio_return_t _mixer_control_set_value(audio_hal_t *ah, const char *ctl_name, int val)
{
    snd_ctl_t *handle;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_type_t type;
    int ret = 0, count = 0, i = 0;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(ctl_name, AUDIO_ERR_PARAMETER);

    pthread_mutex_lock(&(ah->mixer.mutex));

    ret = snd_ctl_open(&handle, ALSA_DEFAULT_CARD, 0);
    if (ret < 0) {
        AUDIO_LOG_ERROR("snd_ctl_open error, card: %s: %s", ALSA_DEFAULT_CARD, snd_strerror(ret));
        pthread_mutex_unlock(&(ah->mixer.mutex));
        return AUDIO_ERR_IOCTL;
    }

    // Get Element Info

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&control);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, ctl_name);

    snd_ctl_elem_info_set_id(info, id);
    if (snd_ctl_elem_info(handle, info) < 0) {
        AUDIO_LOG_ERROR("Cannot find control element: %s", ctl_name);
        goto close;
    }
    snd_ctl_elem_info_get_id(info, id);

    type = snd_ctl_elem_info_get_type(info);
    count = snd_ctl_elem_info_get_count(info);

    snd_ctl_elem_value_set_id(control, id);

    snd_ctl_elem_read(handle, control);

    switch (type) {
    case SND_CTL_ELEM_TYPE_BOOLEAN:
        for (i = 0; i < count; i++)
            snd_ctl_elem_value_set_boolean(control, i, val);
        break;
    case SND_CTL_ELEM_TYPE_INTEGER:
        for (i = 0; i < count; i++)
            snd_ctl_elem_value_set_integer(control, i, val);
        break;
    case SND_CTL_ELEM_TYPE_ENUMERATED:
        for (i = 0; i < count; i++)
            snd_ctl_elem_value_set_enumerated(control, i, val);
        break;

    default:
        AUDIO_LOG_WARN("unsupported control element type");
        goto close;
    }

    snd_ctl_elem_write(handle, control);

    snd_ctl_close(handle);

    AUDIO_LOG_INFO("set mixer(%s) = %d success", ctl_name, val);

    pthread_mutex_unlock(&(ah->mixer.mutex));
    return AUDIO_RET_OK;

close:
    AUDIO_LOG_ERROR("Error");
    snd_ctl_close(handle);
    pthread_mutex_unlock(&(ah->mixer.mutex));
    return AUDIO_ERR_UNDEFINED;
}

audio_return_t _mixer_control_set_value_string(audio_hal_t *ah, const char* ctl_name, const char* value)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(ctl_name, AUDIO_ERR_PARAMETER);

    /* TODO. */
    return AUDIO_RET_OK;
}


audio_return_t _mixer_control_get_element(audio_hal_t *ah, const char *ctl_name, snd_hctl_elem_t **elem)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(ctl_name, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(elem, AUDIO_ERR_PARAMETER);

    /* TODO. */
    return AUDIO_RET_OK;
}