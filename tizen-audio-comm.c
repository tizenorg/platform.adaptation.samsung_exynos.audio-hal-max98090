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

static audio_return_t __set_message_callback(audio_hal_t *ah, message_cb callback, void *user_data)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(callback, AUDIO_ERR_PARAMETER);

    ah->comm.msg_cb = callback;
    ah->comm.user_data = user_data;

    AUDIO_LOG_DEBUG("message callback is set, callback(%p), user_data(%p)", ah->comm.msg_cb, ah->comm.user_data);

    return audio_ret;
}

static audio_return_t __unset_message_callback(audio_hal_t *ah)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    ah->comm.msg_cb = NULL;
    ah->comm.user_data = NULL;

    AUDIO_LOG_DEBUG("message callback is unset");

    return audio_ret;
}

audio_return_t _audio_comm_init(audio_hal_t *ah)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    ah->comm.msg_cb = NULL;
    ah->comm.user_data = NULL;

    return audio_ret;
}

audio_return_t _audio_comm_deinit(audio_hal_t *ah)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    ah->comm.msg_cb = NULL;
    ah->comm.user_data = NULL;

    return audio_ret;
}

audio_return_t _audio_comm_send_message(audio_hal_t *ah, const char *name, int value)
{
    audio_return_t audio_ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(name, AUDIO_ERR_PARAMETER);

    AUDIO_LOG_DEBUG("send message : name(%s), value(%d)", name, value);
    if (ah->comm.msg_cb) {
        ah->comm.msg_cb(name, value, ah->comm.user_data);
    }

    return audio_ret;
}

audio_return_t audio_add_message_cb(void *audio_handle, message_cb callback, void *user_data)
{
    audio_return_t ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(callback, AUDIO_ERR_PARAMETER);

    /* NOTE: Management of several callbacks could be implemented.
             But we do not care of it for now.*/
    ret = __set_message_callback((audio_hal_t *)audio_handle, callback, user_data);

    return ret;
}

audio_return_t audio_remove_message_cb(void *audio_handle, message_cb callback)
{
    audio_return_t ret = AUDIO_RET_OK;

    AUDIO_RETURN_VAL_IF_FAIL(audio_handle, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(callback, AUDIO_ERR_PARAMETER);

    ret = __unset_message_callback((audio_hal_t *)audio_handle);

    return ret;
}