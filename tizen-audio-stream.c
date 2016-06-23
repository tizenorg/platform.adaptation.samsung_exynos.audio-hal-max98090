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

