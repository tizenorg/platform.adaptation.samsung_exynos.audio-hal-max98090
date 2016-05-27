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

/* Audio latency */
static const char* AUDIO_LATENCY_LOW  = "low";
static const char* AUDIO_LATENCY_MID  = "mid";
static const char* AUDIO_LATENCY_HIGH = "high";
static const char* AUDIO_LATENCY_VOIP = "voip";

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

static int __sample_spec_valid(uint32_t rate, audio_sample_format_t format, uint32_t channels)
{
    if ((rate <= 0                 ||
        rate > (48000U*4U)         ||
        channels <= 0              ||
        channels > 32U             ||
        format >= AUDIO_SAMPLE_MAX ||
        format <  AUDIO_SAMPLE_U8))
        return 0;

    AUDIO_LOG_ERROR("hal-latency - __sample_spec_valid() -> return true");

    return 1;
}

static uint32_t __usec_to_bytes(uint64_t t, uint32_t rate, audio_sample_format_t format, uint32_t channels)
{
    uint32_t ret = (uint32_t) (((t * rate) / 1000000ULL)) * (g_size_table[format] * channels);
    AUDIO_LOG_DEBUG("hal-latency - return %d", ret);
    return ret;
}

static uint32_t __sample_size(audio_sample_format_t format)
{
    return g_size_table[format];
}

audio_return_t _audio_stream_init(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    return AUDIO_RET_OK;
}

audio_return_t _audio_stream_deinit(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    return AUDIO_RET_OK;
}

audio_return_t audio_notify_stream_connection_changed(void *audio_handle, audio_stream_info_t *info, uint32_t is_connected)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = (audio_hal_t *)audio_handle;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(info, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_INFO("role:%s, direction:%u, idx:%u, is_connected:%d", info->role, info->direction, info->idx, is_connected);

    return audio_ret;
}

audio_return_t audio_get_buffer_attr(void                  *audio_handle,
                                     uint32_t              direction,
                                     const char            *latency,
                                     uint32_t              samplerate,
                                     audio_sample_format_t format,
                                     uint32_t              channels,
                                     uint32_t              *maxlength,
                                     uint32_t              *tlength,
                                     uint32_t              *prebuf,
                                     uint32_t              *minreq,
                                     uint32_t              *fragsize)
{
    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(latency, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(maxlength, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(tlength, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(prebuf, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(minreq, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(fragsize, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_DEBUG("hal-latency - audio_get_buffer_attr(direction:%d, latency:%s, samplerate:%d, format:%d, channels:%d)", direction, latency, samplerate, format, channels);

    uint32_t period_time        = 0,
             sample_per_period  = 0;

    if (__sample_spec_valid(samplerate, format, channels) == 0) {
        return AUDIO_ERR_PARAMETER;
    }

    if (direction == AUDIO_DIRECTION_IN) {
        if (!strcmp(latency, AUDIO_LATENCY_LOW)) {
            AUDIO_LOG_DEBUG("AUDIO_DIRECTION_IN, AUDIO_LATENCY_LOW");
            period_time        = PERIOD_TIME_FOR_LOW_LATENCY_MSEC;
            sample_per_period  = (samplerate * period_time) / 1000;
            *prebuf            = 0;
            *minreq            = -1;
            *tlength           = -1;
            *maxlength         = -1;
            *fragsize          = sample_per_period * __sample_size(format);
        } else if (!strcmp(latency, AUDIO_LATENCY_MID)) {
            AUDIO_LOG_DEBUG("AUDIO_DIRECTION_IN, AUDIO_LATENCY_MID");
            period_time        = PERIOD_TIME_FOR_MID_LATENCY_MSEC;
            sample_per_period  = (samplerate * period_time) / 1000;
            *prebuf            = 0;
            *minreq            = -1;
            *tlength           = -1;
            *maxlength         = -1;
            *fragsize          = sample_per_period * __sample_size(format);
        } else if (!strcmp(latency, AUDIO_LATENCY_HIGH)) {
            AUDIO_LOG_DEBUG("AUDIO_DIRECTION_IN, AUDIO_LATENCY_HIGH");
            period_time        = PERIOD_TIME_FOR_HIGH_LATENCY_MSEC;
            sample_per_period  = (samplerate * period_time) / 1000;
            *prebuf            = 0;
            *minreq            = -1;
            *tlength           = -1;
            *maxlength         = -1;
            *fragsize          = sample_per_period * __sample_size(format);
        } else if (!strcmp(latency, AUDIO_LATENCY_VOIP)) {
            AUDIO_LOG_DEBUG("AUDIO_DIRECTION_IN, AUDIO_LATENCY_VOIP");
            period_time        = PERIOD_TIME_FOR_VOIP_LATENCY_MSEC;
            sample_per_period  = (samplerate * period_time) / 1000;
            *prebuf            = 0;
            *minreq            = -1;
            *tlength           = -1;
            *maxlength         = -1;
            *fragsize          = sample_per_period * __sample_size(format);
        } else {
            AUDIO_LOG_ERROR("hal-latency - The latency(%s) is undefined", latency);
            return AUDIO_ERR_UNDEFINED;
        }
    } else {  /* AUDIO_DIRECTION_OUT */
        if (!strcmp(latency, AUDIO_LATENCY_LOW)) {
            AUDIO_LOG_DEBUG("AUDIO_DIRECTION_OUT, AUDIO_LATENCY_LOW");
            period_time        = PERIOD_TIME_FOR_LOW_LATENCY_MSEC;
            sample_per_period  = (samplerate * period_time) / 1000;
            *prebuf            = 0;
            *minreq            = -1;
            *tlength           = (samplerate / 10) * __sample_size(format) * channels;  /* 100ms */
            *maxlength         = -1;
            *fragsize          = 0;
        } else if (!strcmp(latency, AUDIO_LATENCY_MID)) {
            AUDIO_LOG_DEBUG("AUDIO_DIRECTION_OUT, AUDIO_LATENCY_MID");
            period_time        = PERIOD_TIME_FOR_MID_LATENCY_MSEC;
            sample_per_period  = (samplerate * period_time) / 1000;
            *prebuf            = 0;
            *minreq            = -1;
            *tlength           = (uint32_t) __usec_to_bytes(200000, samplerate, format, channels);
            *maxlength         = -1;
            *fragsize          = -1;
        } else if (!strcmp(latency, AUDIO_LATENCY_HIGH)) {
            AUDIO_LOG_DEBUG("AUDIO_DIRECTION_OUT, AUDIO_LATENCY_HIGH");
            period_time        = PERIOD_TIME_FOR_HIGH_LATENCY_MSEC;
            sample_per_period  = (samplerate * period_time) / 1000;
            *prebuf            = 0;
            *minreq            = -1;
            *tlength           = (uint32_t) __usec_to_bytes(400000, samplerate, format, channels);
            *maxlength         = -1;
            *fragsize          = -1;
        } else if (!strcmp(latency, AUDIO_LATENCY_VOIP)) {
            AUDIO_LOG_DEBUG("AUDIO_DIRECTION_OUT, AUDIO_LATENCY_VOIP");
            period_time        = PERIOD_TIME_FOR_VOIP_LATENCY_MSEC;
            sample_per_period  = (samplerate * period_time) / 1000;
            *prebuf            = 0;
            *minreq            = __usec_to_bytes(20000, samplerate, format, channels);
            *tlength           = __usec_to_bytes(100000, samplerate, format, channels);
            *maxlength         = -1;
            *fragsize          = 0;
        } else {
            AUDIO_LOG_ERROR("hal-latency - The latency(%s) is undefined", latency);
            return AUDIO_ERR_UNDEFINED;
        }
    }

    AUDIO_LOG_INFO("hal-latency - return attr --> prebuf:%d, minreq:%d, tlength:%d, maxlength:%d, fragsize:%d", *prebuf, *minreq, *tlength, *maxlength, *fragsize);
    return AUDIO_RET_OK;
}