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
#include <pthread.h>

#include "tizen-audio-internal.h"

audio_return_t _audio_util_init(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    pthread_mutex_init(&(ah->mixer.mutex), NULL);
    return AUDIO_RET_OK;
}

audio_return_t _audio_util_deinit(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    pthread_mutex_destroy(&(ah->mixer.mutex));
    return AUDIO_RET_OK;
}

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

audio_return_t _audio_mixer_control_set_param(audio_hal_t *ah, const char* ctl_name, snd_ctl_elem_value_t* param, int size)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    /* TODO. */
    return AUDIO_RET_OK;
}

audio_return_t audio_mixer_control_get_value(audio_hal_t *ah, const char *ctl_name, int *val)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    audio_ret = _audio_mixer_control_get_value(ah, ctl_name, val);
    return audio_ret;
}

audio_return_t _audio_mixer_control_get_value(audio_hal_t *ah, const char *ctl_name, int *val)
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

audio_return_t _audio_mixer_control_set_value(audio_hal_t *ah, const char *ctl_name, int val)
{
    snd_ctl_t *handle;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_type_t type;

    char *card_name = NULL;
    int ret = 0, count = 0, i = 0;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(ctl_name, AUDIO_ERR_PARAMETER);

    pthread_mutex_lock(&(ah->mixer.mutex));

    ret = snd_ctl_open(&handle, ALSA_DEFAULT_CARD, 0);
    if (ret < 0) {
        AUDIO_LOG_ERROR("snd_ctl_open error, card: %s: %s", card_name, snd_strerror(ret));
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

audio_return_t _audio_mixer_control_set_value_string(audio_hal_t *ah, const char* ctl_name, const char* value)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(ctl_name, AUDIO_ERR_PARAMETER);

    /* TODO. */
    return AUDIO_RET_OK;
}


audio_return_t _audio_mixer_control_get_element(audio_hal_t *ah, const char *ctl_name, snd_hctl_elem_t **elem)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(ctl_name, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(elem, AUDIO_ERR_PARAMETER);

    /* TODO. */
    return AUDIO_RET_OK;
}

#ifdef __USE_TINYALSA__
/* Convert pcm format from pulse to alsa */
static const uint32_t g_format_convert_table[] = {
    [AUDIO_SAMPLE_U8]        = PCM_FORMAT_S8,
    [AUDIO_SAMPLE_S16LE]     = PCM_FORMAT_S16_LE,
    [AUDIO_SAMPLE_S32LE]     = PCM_FORMAT_S32_LE,
    [AUDIO_SAMPLE_S24_32LE]  = PCM_FORMAT_S24_LE
};
#else  /* alsa-lib */
/* Convert pcm format from pulse to alsa */
static const uint32_t g_format_convert_table[] = {
    [AUDIO_SAMPLE_U8]        = SND_PCM_FORMAT_U8,
    [AUDIO_SAMPLE_ALAW]      = SND_PCM_FORMAT_A_LAW,
    [AUDIO_SAMPLE_ULAW]      = SND_PCM_FORMAT_MU_LAW,
    [AUDIO_SAMPLE_S16LE]     = SND_PCM_FORMAT_S16_LE,
    [AUDIO_SAMPLE_S16BE]     = SND_PCM_FORMAT_S16_BE,
    [AUDIO_SAMPLE_FLOAT32LE] = SND_PCM_FORMAT_FLOAT_LE,
    [AUDIO_SAMPLE_FLOAT32BE] = SND_PCM_FORMAT_FLOAT_BE,
    [AUDIO_SAMPLE_S32LE]     = SND_PCM_FORMAT_S32_LE,
    [AUDIO_SAMPLE_S32BE]     = SND_PCM_FORMAT_S32_BE,
    [AUDIO_SAMPLE_S24LE]     = SND_PCM_FORMAT_S24_3LE,
    [AUDIO_SAMPLE_S24BE]     = SND_PCM_FORMAT_S24_3BE,
    [AUDIO_SAMPLE_S24_32LE]  = SND_PCM_FORMAT_S24_LE,
    [AUDIO_SAMPLE_S24_32BE]  = SND_PCM_FORMAT_S24_BE
};
#endif

uint32_t _convert_format(audio_sample_format_t format)
{
    return g_format_convert_table[format];
}

/* Generic snd pcm interface APIs */
audio_return_t _audio_pcm_set_hw_params(snd_pcm_t *pcm, audio_pcm_sample_spec_t *sample_spec, uint8_t *use_mmap, snd_pcm_uframes_t *period_size, snd_pcm_uframes_t *buffer_size)
{
    audio_return_t ret = AUDIO_RET_OK;
    snd_pcm_hw_params_t *hwparams;
    int err = 0;
    int dir;
    unsigned int val = 0;
    snd_pcm_uframes_t _period_size = period_size ? *period_size : 0;
    snd_pcm_uframes_t _buffer_size = buffer_size ? *buffer_size : 0;
    uint8_t _use_mmap = use_mmap && *use_mmap;
    uint32_t channels = 0;

    AUDIO_RETURN_VAL_IF_FAIL(pcm, AUDIO_ERR_PARAMETER);

    snd_pcm_hw_params_alloca(&hwparams);

    /* Skip parameter setting to null device. */
    if (snd_pcm_type(pcm) == SND_PCM_TYPE_NULL)
        return AUDIO_ERR_IOCTL;

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&hwparams);

    /* Fill it in with default values. */
    if (snd_pcm_hw_params_any(pcm, hwparams) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_any() : failed! - %s\n", snd_strerror(err));
        goto error;
    }

    /* Set the desired hardware parameters. */

    if (_use_mmap) {

        if (snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_MMAP_INTERLEAVED) < 0) {

            /* mmap() didn't work, fall back to interleaved */

            if ((ret = snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
                AUDIO_LOG_DEBUG("snd_pcm_hw_params_set_access() failed: %s", snd_strerror(ret));
                goto error;
            }

            _use_mmap = 0;
        }

    } else if ((ret = snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        AUDIO_LOG_DEBUG("snd_pcm_hw_params_set_access() failed: %s", snd_strerror(ret));
        goto error;
    }
    AUDIO_LOG_DEBUG("setting rate - %d", sample_spec->rate);
    err = snd_pcm_hw_params_set_rate(pcm, hwparams, sample_spec->rate, 0);
    if (err < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_rate() : failed! - %s\n", snd_strerror(err));
    }

    err = snd_pcm_hw_params(pcm, hwparams);
    if (err < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params() : failed! - %s\n", snd_strerror(err));
        goto error;
    }

    /* Dump current param */

    if ((ret = snd_pcm_hw_params_current(pcm, hwparams)) < 0) {
        AUDIO_LOG_INFO("snd_pcm_hw_params_current() failed: %s", snd_strerror(ret));
        goto error;
    }

    if ((ret = snd_pcm_hw_params_get_period_size(hwparams, &_period_size, &dir)) < 0 ||
        (ret = snd_pcm_hw_params_get_buffer_size(hwparams, &_buffer_size)) < 0) {
        AUDIO_LOG_INFO("snd_pcm_hw_params_get_{period|buffer}_size() failed: %s", snd_strerror(ret));
        goto error;
    }

    snd_pcm_hw_params_get_access(hwparams, (snd_pcm_access_t *) &val);
    AUDIO_LOG_DEBUG("access type = %s\n", snd_pcm_access_name((snd_pcm_access_t)val));

    snd_pcm_hw_params_get_format(hwparams, &sample_spec->format);
    AUDIO_LOG_DEBUG("format = '%s' (%s)\n",
                    snd_pcm_format_name((snd_pcm_format_t)sample_spec->format),
                    snd_pcm_format_description((snd_pcm_format_t)sample_spec->format));

    snd_pcm_hw_params_get_subformat(hwparams, (snd_pcm_subformat_t *)&val);
    AUDIO_LOG_DEBUG("subformat = '%s' (%s)\n",
                    snd_pcm_subformat_name((snd_pcm_subformat_t)val),
                    snd_pcm_subformat_description((snd_pcm_subformat_t)val));

    snd_pcm_hw_params_get_channels(hwparams, &channels);
    sample_spec->channels = (uint8_t)channels;
    AUDIO_LOG_DEBUG("channels = %d\n", sample_spec->channels);

    if (buffer_size)
        *buffer_size = _buffer_size;

    if (period_size)
        *period_size = _period_size;

    if (use_mmap)
        *use_mmap = _use_mmap;

    return AUDIO_RET_OK;

error:
    return AUDIO_ERR_RESOURCE;
}

audio_return_t _audio_pcm_set_sw_params(snd_pcm_t *pcm, snd_pcm_uframes_t avail_min, uint8_t period_event)
{
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t boundary;
    int err;

    AUDIO_RETURN_VAL_IF_FAIL(pcm, AUDIO_ERR_PARAMETER);

    snd_pcm_sw_params_alloca(&swparams);

    if ((err = snd_pcm_sw_params_current(pcm, swparams)) < 0) {
        AUDIO_LOG_WARN("Unable to determine current swparams: %s\n", snd_strerror(err));
        goto error;
    }
    if ((err = snd_pcm_sw_params_set_period_event(pcm, swparams, period_event)) < 0) {
        AUDIO_LOG_WARN("Unable to disable period event: %s\n", snd_strerror(err));
        goto error;
    }
    if ((err = snd_pcm_sw_params_set_tstamp_mode(pcm, swparams, SND_PCM_TSTAMP_ENABLE)) < 0) {
        AUDIO_LOG_WARN("Unable to enable time stamping: %s\n", snd_strerror(err));
        goto error;
    }
    if ((err = snd_pcm_sw_params_get_boundary(swparams, &boundary)) < 0) {
        AUDIO_LOG_WARN("Unable to get boundary: %s\n", snd_strerror(err));
        goto error;
    }
    if ((err = snd_pcm_sw_params_set_stop_threshold(pcm, swparams, boundary)) < 0) {
        AUDIO_LOG_WARN("Unable to set stop threshold: %s\n", snd_strerror(err));
        goto error;
    }
    if ((err = snd_pcm_sw_params_set_start_threshold(pcm, swparams, (snd_pcm_uframes_t) avail_min)) < 0) {
        AUDIO_LOG_WARN("Unable to set start threshold: %s\n", snd_strerror(err));
        goto error;
    }
    if ((err = snd_pcm_sw_params_set_avail_min(pcm, swparams, avail_min)) < 0) {
        AUDIO_LOG_WARN("snd_pcm_sw_params_set_avail_min() failed: %s", snd_strerror(err));
        goto error;
    }
    if ((err = snd_pcm_sw_params(pcm, swparams)) < 0) {
        AUDIO_LOG_WARN("Unable to set sw params: %s\n", snd_strerror(err));
        goto error;
    }
    return AUDIO_RET_OK;
error:
    return err;
}

/* ------ dump helper --------  */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

dump_data_t* dump_new(int length)
{
    dump_data_t* dump = NULL;

    if ((dump = malloc(sizeof(dump_data_t)))) {
        memset(dump, 0, sizeof(dump_data_t));
        if ((dump->strbuf = malloc(length))) {
            dump->p = &dump->strbuf[0];
            dump->left = length;
        } else {
            free(dump);
            dump = NULL;
        }
    }

    return dump;
}

void dump_add_str(dump_data_t *dump, const char *fmt, ...)
{
    int len;
    va_list ap;

    if (!dump)
        return;

    va_start(ap, fmt);
    len = vsnprintf(dump->p, dump->left, fmt, ap);
    va_end(ap);

    dump->p += MAX(0, len);
    dump->left -= MAX(0, len);
}

char* dump_get_str(dump_data_t *dump)
{
    return (dump) ? dump->strbuf : NULL;
}

void dump_free(dump_data_t *dump)
{
    if (dump) {
        if (dump->strbuf)
            free(dump->strbuf);
        free(dump);
    }
}
/* ------ dump helper --------  */
