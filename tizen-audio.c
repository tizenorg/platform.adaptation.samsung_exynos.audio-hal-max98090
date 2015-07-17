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

#include "tizen-audio-internal.h"

int audio_get_revision (void)
{
    return AUDIO_REVISION;
}

audio_return_t audio_init (void **userdata, void *platform_data)
{
    audio_mgr_t *am;
    audio_return_t ret = AUDIO_RET_OK;

    if (!(am = malloc(sizeof(audio_mgr_t)))) {
        AUDIO_LOG_ERROR("am malloc failed");
        return AUDIO_ERR_RESOURCE;
    }
    am->platform_data = platform_data;
    memset(&am->cb_intf, 0, sizeof(audio_cb_interface_t));
    if (AUDIO_IS_ERROR((ret = _audio_device_init(am)))) {
        AUDIO_LOG_ERROR("device init failed");
        goto error_exit;
    }
    if (AUDIO_IS_ERROR((ret = _audio_volume_init(am)))) {
        AUDIO_LOG_ERROR("stream init failed");
        goto error_exit;
    }
    if (AUDIO_IS_ERROR((ret = _audio_ucm_init(am)))) {
        AUDIO_LOG_ERROR("ucm init failed");
        goto error_exit;
    }
    if (AUDIO_IS_ERROR((ret = _audio_util_init(am)))) {
        AUDIO_LOG_ERROR("mixer init failed");
        goto error_exit;
    }

    *userdata = (void *)am;
    return AUDIO_RET_OK;

error_exit:
    if (am)
        free(am);

    return ret;
}

audio_return_t audio_deinit (void **userdata)
{
    audio_mgr_t *am = (audio_mgr_t *)*userdata;

    if (am) {
        _audio_device_deinit(am);
        _audio_volume_deinit(am);
        _audio_ucm_deinit(am);
        _audio_util_deinit(am);
        free(am);
        *userdata = NULL;
    }

    return AUDIO_RET_OK;
}

audio_return_t audio_reset_volume (void **userdata)
{
    audio_mgr_t *am = (audio_mgr_t *)*userdata;
    audio_return_t ret = AUDIO_RET_OK;

    if (am) {
        _audio_volume_deinit(am);

        if (AUDIO_IS_ERROR((ret = _audio_volume_init(am)))) {
            AUDIO_LOG_ERROR("stream init failed");
            goto error_exit;
        }
    }

    return AUDIO_RET_OK;

error_exit:
    if (am)
        free(am);
    *userdata = NULL;

    return ret;
}

audio_return_t audio_set_callback (void *userdata, audio_cb_interface_t *cb_interface)
{
    audio_mgr_t *am = (audio_mgr_t *)userdata;

    if (am) {
        memcpy(&am->cb_intf, cb_interface, sizeof(audio_cb_interface_t));
        return AUDIO_RET_OK;
    } else {
        return AUDIO_ERR_PARAMETER;
    }
}


static const unsigned int SAMPLES_PER_PERIOD_DEFAULT         = 1536; /* Frames */
static const unsigned int PERIODS_PER_BUFFER_FASTMODE        = 4;
static const unsigned int PERIODS_PER_BUFFER_DEFAULT         = 6;
static const unsigned int PERIODS_PER_BUFFER_VOIP            = 2;
static const unsigned int PERIODS_PER_BUFFER_PLAYBACK        = 8;
static const unsigned int PERIODS_PER_BUFFER_CAPTURE         = 12;
static const unsigned int PERIODS_PER_BUFFER_VIDEO           = 10;


/* Latency msec */
static const unsigned int PERIOD_TIME_FOR_ULOW_LATENCY_MSEC  = 20;
static const unsigned int PERIOD_TIME_FOR_LOW_LATENCY_MSEC   = 25;
static const unsigned int PERIOD_TIME_FOR_MID_LATENCY_MSEC   = 50;
static const unsigned int PERIOD_TIME_FOR_HIGH_LATENCY_MSEC  = 75;
static const unsigned int PERIOD_TIME_FOR_UHIGH_LATENCY_MSEC = 150;
static const unsigned int PERIOD_TIME_FOR_VOIP_LATENCY_MSEC  = 20;

static const uint32_t g_size_table[] = {
    [AUDIO_SAMPLE_U8]        = 1,
    [AUDIO_SAMPLE_ULAW]      = 1,
    [AUDIO_SAMPLE_ALAW]      = 1,
    [AUDIO_SAMPLE_S16LE]     = 2,
    [AUDIO_SAMPLE_S16BE]     = 2,
    [AUDIO_SAMPLE_FLOAT32LE] = 4,
    [AUDIO_SAMPLE_FLOAT32BE] = 4,
    [AUDIO_SAMPLE_S32LE]     = 4,
    [AUDIO_SAMPLE_S32BE]     = 4,
    [AUDIO_SAMPLE_S24LE]     = 3,
    [AUDIO_SAMPLE_S24BE]     = 3,
    [AUDIO_SAMPLE_S24_32LE]  = 4,
    [AUDIO_SAMPLE_S24_32BE]  = 4
};

int _sample_spec_valid(uint32_t rate, audio_sample_format_t format, uint32_t channels)
{
    if ((rate <= 0                 ||
        rate > (48000U*4U)         ||
        channels <= 0              ||
        channels > 32U             ||
        format >= AUDIO_SAMPLE_MAX ||
        format <  AUDIO_SAMPLE_U8))
        return 0;

    AUDIO_LOG_ERROR("hal-latency - _sample_spec_valid() -> return true");

    return 1;
}

uint32_t _audio_usec_to_bytes(uint64_t t, uint32_t rate, audio_sample_format_t format, uint32_t channels)
{
    uint32_t ret = (uint32_t) (((t * rate) / 1000000ULL)) * (g_size_table[format] * channels);
    AUDIO_LOG_DEBUG("hal-latency - return %d", ret);
    return ret;
}

uint32_t _audio_sample_size(audio_sample_format_t format)
{
    return g_size_table[format];
}
audio_return_t audio_get_buffer_attr(void                  *userdata,
                                     audio_latency_t       latency,
                                     uint32_t              samplerate,
                                     audio_sample_format_t format,
                                     uint32_t              channels,
                                     uint32_t              *maxlength,
                                     uint32_t              *tlength,
                                     uint32_t              *prebuf,
                                     uint32_t              *minreq,
                                     uint32_t              *fragsize)
{
    assert(userdata);
    assert(maxlength);
    assert(tlength);
    assert(prebuf);
    assert(minreq);
    assert(fragsize);

    AUDIO_LOG_DEBUG("hal-latency - audio_get_buffer_attr(latency:%d, samplerate:%d, format:%d, channels:%d)", latency, samplerate, format, channels);

    audio_mgr_t *am = (audio_mgr_t *)userdata;

    uint32_t period_time        = 0,
             sample_per_period  = 0,
             periods_per_buffer = 0;

    if (_sample_spec_valid(samplerate, format, channels) == 0) {
        return AUDIO_ERR_PARAMETER;
    }

    switch (latency) {
    case AUDIO_IN_LATENCY_LOW:
        AUDIO_LOG_DEBUG("AUDIO_IN_LATENCY_LOW");
        period_time        = PERIOD_TIME_FOR_LOW_LATENCY_MSEC;
        sample_per_period  = (samplerate * period_time) / 1000;
        periods_per_buffer = PERIODS_PER_BUFFER_FASTMODE;
        *prebuf            = 0;
        *minreq            = -1;
        *tlength           = -1;
        *maxlength         = -1;
        *fragsize          = sample_per_period * _audio_sample_size(format);
        break;

    case AUDIO_IN_LATENCY_MID:
        AUDIO_LOG_DEBUG("AUDIO_IN_LATENCY_MID");
        period_time        = PERIOD_TIME_FOR_MID_LATENCY_MSEC;
        sample_per_period  = (samplerate * period_time) / 1000;
        periods_per_buffer = PERIODS_PER_BUFFER_DEFAULT;
        *prebuf            = 0;
        *minreq            = -1;
        *tlength           = -1;
        *maxlength         = -1;
        *fragsize          = _audio_usec_to_bytes(100000, samplerate, format, channels);
        break;

    case AUDIO_IN_LATENCY_HIGH:
        AUDIO_LOG_DEBUG("AUDIO_IN_LATENCY_HIGH");
        period_time        = PERIOD_TIME_FOR_HIGH_LATENCY_MSEC;
        sample_per_period  = (samplerate * period_time) / 1000;
        periods_per_buffer = PERIODS_PER_BUFFER_CAPTURE;
        *prebuf            = 0;
        *minreq            = -1;
        *tlength           = -1;
        *maxlength         = -1;
        *fragsize          = sample_per_period * _audio_sample_size(format);
        break;

    case AUDIO_IN_LATENCY_VOIP:
        AUDIO_LOG_DEBUG("AUDIO_IN_LATENCY_VOIP");
        period_time        = PERIOD_TIME_FOR_VOIP_LATENCY_MSEC;
        sample_per_period  = (samplerate * period_time) / 1000;
        periods_per_buffer = PERIODS_PER_BUFFER_VOIP;
        *prebuf            = 0;
        *minreq            = -1;
        *tlength           = -1;
        *maxlength         = -1;
        *fragsize          = sample_per_period * _audio_sample_size(format);
        break;

    case AUDIO_OUT_LATENCY_LOW:
        AUDIO_LOG_DEBUG("AUDIO_OUT_LATENCY_LOW");
        period_time        = PERIOD_TIME_FOR_LOW_LATENCY_MSEC;
        sample_per_period  = (samplerate * period_time) / 1000;
        periods_per_buffer = PERIODS_PER_BUFFER_FASTMODE;
        *prebuf            = (samplerate / 100) * _audio_sample_size(format) * channels;
        *minreq            = -1;
        *tlength           = (samplerate / 10) * _audio_sample_size(format) * channels;
        *maxlength         = -1;
        *fragsize          = 0;
        break;

    case AUDIO_OUT_LATENCY_MID:
        AUDIO_LOG_DEBUG("AUDIO_OUT_LATENCY_MID");
        period_time        = PERIOD_TIME_FOR_MID_LATENCY_MSEC;
        sample_per_period  = (samplerate * period_time) / 1000;
        periods_per_buffer = PERIODS_PER_BUFFER_DEFAULT;
        *prebuf            = -1;
        *minreq            = -1;
        *tlength           = (uint32_t) _audio_usec_to_bytes(350000, samplerate, format, channels);
        *maxlength         = -1;
        *fragsize          = -1;
        break;

    case AUDIO_OUT_LATENCY_HIGH:
        AUDIO_LOG_DEBUG("AUDIO_OUT_LATENCY_HIGH");
        period_time        = PERIOD_TIME_FOR_HIGH_LATENCY_MSEC;
        sample_per_period  = (samplerate * period_time) / 1000;
        periods_per_buffer = PERIODS_PER_BUFFER_PLAYBACK;
        *prebuf            = 0;
        *minreq            = -1;
        *tlength           = (uint32_t) _audio_usec_to_bytes(500000, samplerate, format, channels);
        *maxlength         = -1;
        *fragsize          = -1;
        break;

    case AUDIO_OUT_LATENCY_VOIP:
        AUDIO_LOG_DEBUG("AUDIO_OUT_LATENCY_VOIP");
        period_time        = PERIOD_TIME_FOR_VOIP_LATENCY_MSEC;
        sample_per_period  = (samplerate * period_time) / 1000;
        periods_per_buffer = PERIODS_PER_BUFFER_VOIP;
        *prebuf            = -1;
        *minreq            = _audio_usec_to_bytes(20000, samplerate, format, channels);
        *tlength           = _audio_usec_to_bytes(100000, samplerate, format, channels);
        *maxlength         = -1;
        *fragsize          = 0;
        break;

    default:
        AUDIO_LOG_ERROR("hal-latency - The latency(%d) is undefined", latency);
        return AUDIO_ERR_UNDEFINED;
    }

    AUDIO_LOG_INFO("hal-latency - return attr --> prebuf:%d, minreq:%d, tlength:%d, maxlength:%d, fragsize:%d", *prebuf, *minreq, *tlength, *maxlength, *fragsize);
    return AUDIO_RET_OK;
}
