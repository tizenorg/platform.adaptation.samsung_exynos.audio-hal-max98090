/*
 * audio-hal
 *
 * Copyright (c) 2015 - 2016 Samsung Electronics Co., Ltd. All rights reserved.
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

audio_return_t audio_init(void **audio_handle)
{
    audio_hal_t *ah;
    audio_return_t ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);

    if (!(ah = malloc(sizeof(audio_hal_t)))) {
        AUDIO_LOG_ERROR("failed to malloc()");
        return AUDIO_ERR_RESOURCE;
    }
    if ((ret = _audio_volume_init(ah))) {
        AUDIO_LOG_ERROR("failed to _audio_volume_init(), ret(0x%x)", ret);
        goto error_exit;
    }
    if ((ret = _audio_routing_init(ah))) {
        AUDIO_LOG_ERROR("failed to _audio_routing_init(), ret(0x%x)", ret);
        goto error_exit;
    }
    if ((ret = _audio_stream_init(ah))) {
        AUDIO_LOG_ERROR("failed to _audio_stream_init(), ret(0x%x)", ret);
        goto error_exit;
    }
    if ((ret = _audio_pcm_init(ah))) {
        AUDIO_LOG_ERROR("failed to _audio_pcm_init(), ret(0x%x)", ret);
        goto error_exit;
    }
    if ((ret = _audio_comm_init(ah))) {
        AUDIO_LOG_ERROR("failed to _audio_comm_init(), ret(0x%x)", ret);
        goto error_exit;
    }

    *audio_handle = (void *)ah;
    return AUDIO_RET_OK;

error_exit:
    if (ah)
        free(ah);

    return ret;
}

audio_return_t audio_deinit(void *audio_handle)
{
    audio_hal_t *ah = (audio_hal_t *)audio_handle;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    _audio_volume_deinit(ah);
    _audio_routing_deinit(ah);
    _audio_stream_deinit(ah);
    _audio_pcm_deinit(ah);
    _audio_comm_deinit(ah);
    free(ah);
    ah = NULL;

    return AUDIO_RET_OK;
}