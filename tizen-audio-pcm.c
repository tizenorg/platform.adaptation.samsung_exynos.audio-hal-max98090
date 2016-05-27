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

#include "tizen-audio-internal.h"
#include "tizen-audio-impl.h"

audio_return_t _audio_pcm_init(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    ah->device.pcm_in = NULL;
    ah->device.pcm_out = NULL;
    pthread_mutex_init(&ah->device.pcm_lock, NULL);
    ah->device.pcm_count = 0;

    return AUDIO_RET_OK;
}

audio_return_t _audio_pcm_deinit(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    pthread_mutex_destroy(&ah->device.pcm_lock);

    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_open(void *audio_handle, void **pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = NULL;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(sample_spec, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL((period_size > 0), AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL((periods > 0), AUDIO_ERR_PARAMETER);

    if ((audio_ret = _pcm_open(pcm_handle, direction, sample_spec, period_size, periods)))
        return audio_ret;

    ah = (audio_hal_t*)audio_handle;
    ah->device.pcm_count++;
    AUDIO_LOG_INFO("Opening PCM handle 0x%x", *pcm_handle);

    return AUDIO_RET_OK;
}

audio_return_t audio_pcm_start(void *audio_handle, void *pcm_handle)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_start(pcm_handle);

    return audio_ret;
}

audio_return_t audio_pcm_stop(void *audio_handle, void *pcm_handle)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_stop(pcm_handle);

    return audio_ret;
}

audio_return_t audio_pcm_close(void *audio_handle, void *pcm_handle)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = NULL;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    if ((audio_ret = _pcm_close(pcm_handle)))
        return audio_ret;

    pcm_handle = NULL;
    ah = (audio_hal_t*)audio_handle;
    ah->device.pcm_count--;

    AUDIO_LOG_INFO("PCM handle close success (count:%d)", ah->device.pcm_count);

    return audio_ret;
}

audio_return_t audio_pcm_avail(void *audio_handle, void *pcm_handle, uint32_t *avail)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(avail, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_avail(pcm_handle, avail);

    return audio_ret;
}

audio_return_t audio_pcm_write(void *audio_handle, void *pcm_handle, const void *buffer, uint32_t frames)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_write(pcm_handle, buffer, frames);

    return audio_ret;
}

audio_return_t audio_pcm_read(void *audio_handle, void *pcm_handle, void *buffer, uint32_t frames)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_read(pcm_handle, buffer, frames);

    return audio_ret;
}

audio_return_t audio_pcm_get_fd(void *audio_handle, void *pcm_handle, int *fd)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(fd, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_get_fd(pcm_handle, fd);

    return audio_ret;
}

audio_return_t audio_pcm_recover(void *audio_handle, void *pcm_handle, int revents)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_recover(pcm_handle, revents);

    return audio_ret;
}

audio_return_t audio_pcm_get_params(void *audio_handle, void *pcm_handle, uint32_t direction, void **sample_spec, uint32_t *period_size, uint32_t *periods)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(sample_spec, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(period_size, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(periods, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_get_params(pcm_handle, direction, sample_spec, period_size, periods);

    return audio_ret;
}

audio_return_t audio_pcm_set_params(void *audio_handle, void *pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(sample_spec, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(period_size, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(periods, AUDIO_ERR_PARAMETER);

    audio_ret = _pcm_set_params(pcm_handle, direction, sample_spec, period_size, periods);

    return audio_ret;
}