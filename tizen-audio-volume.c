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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <vconf.h>
#include <iniparser.h>

#include "tizen-audio-internal.h"

#define VOLUME_INI_DEFAULT_PATH     SYSCONFDIR"/multimedia/mmfw_audio_volume.ini" /* SYSCONFDIR is defined at .spec */
#define VOLUME_INI_TEMP_PATH        "/opt/system/mmfw_audio_volume.ini"
#define VOLUME_VALUE_MAX            (1.0f)
#define GAIN_VALUE_MAX              (1.0f)

static const char *g_volume_vconf[AUDIO_VOLUME_TYPE_MAX] = {
    "file/private/sound/volume/system",         /* AUDIO_VOLUME_TYPE_SYSTEM */
    "file/private/sound/volume/notification",   /* AUDIO_VOLUME_TYPE_NOTIFICATION */
    "file/private/sound/volume/alarm",          /* AUDIO_VOLUME_TYPE_ALARM */
    "file/private/sound/volume/ringtone",       /* AUDIO_VOLUME_TYPE_RINGTONE */
    "file/private/sound/volume/media",          /* AUDIO_VOLUME_TYPE_MEDIA */
    "file/private/sound/volume/call",           /* AUDIO_VOLUME_TYPE_CALL */
    "file/private/sound/volume/voip",           /* AUDIO_VOLUME_TYPE_VOIP */
    "file/private/sound/volume/voice",          /* AUDIO_VOLUME_TYPE_VOICE */
};

static const char *__get_volume_type_string_by_idx(uint32_t vol_type_idx)
{
    switch (vol_type_idx) {
    case AUDIO_VOLUME_TYPE_SYSTEM:          return "system";
    case AUDIO_VOLUME_TYPE_NOTIFICATION:    return "notification";
    case AUDIO_VOLUME_TYPE_ALARM:           return "alarm";
    case AUDIO_VOLUME_TYPE_RINGTONE:        return "ringtone";
    case AUDIO_VOLUME_TYPE_MEDIA:           return "media";
    case AUDIO_VOLUME_TYPE_CALL:            return "call";
    case AUDIO_VOLUME_TYPE_VOIP:            return "voip";
    case AUDIO_VOLUME_TYPE_VOICE:           return "voice";
    default:                                return "invalid";
    }
}

static uint32_t __get_volume_idx_by_string_type(const char *vol_type)
{
    if (!strncmp(vol_type, "system", strlen(vol_type)) || !strncmp(vol_type, "0", strlen(vol_type)))
        return AUDIO_VOLUME_TYPE_SYSTEM;
    else if (!strncmp(vol_type, "notification", strlen(vol_type)) || !strncmp(vol_type, "1", strlen(vol_type)))
        return AUDIO_VOLUME_TYPE_NOTIFICATION;
    else if (!strncmp(vol_type, "alarm", strlen(vol_type)) || !strncmp(vol_type, "2", strlen(vol_type)))
        return AUDIO_VOLUME_TYPE_ALARM;
    else if (!strncmp(vol_type, "ringtone", strlen(vol_type)) || !strncmp(vol_type, "3", strlen(vol_type)))
        return AUDIO_VOLUME_TYPE_RINGTONE;
    else if (!strncmp(vol_type, "media", strlen(vol_type)) || !strncmp(vol_type, "4", strlen(vol_type)))
        return AUDIO_VOLUME_TYPE_MEDIA;
    else if (!strncmp(vol_type, "call", strlen(vol_type)) || !strncmp(vol_type, "5", strlen(vol_type)))
        return AUDIO_VOLUME_TYPE_CALL;
    else if (!strncmp(vol_type, "voip", strlen(vol_type)) || !strncmp(vol_type, "6", strlen(vol_type)))
        return AUDIO_VOLUME_TYPE_VOIP;
    else if (!strncmp(vol_type, "voice", strlen(vol_type)) || !strncmp(vol_type, "7", strlen(vol_type)))
        return AUDIO_VOLUME_TYPE_VOICE;
    else
        return AUDIO_VOLUME_TYPE_MEDIA;
}

static const char *__get_gain_type_string_by_idx(uint32_t gain_type_idx)
{
    switch (gain_type_idx) {
    case AUDIO_GAIN_TYPE_DEFAULT:           return "default";
    case AUDIO_GAIN_TYPE_DIALER:            return "dialer";
    case AUDIO_GAIN_TYPE_TOUCH:             return "touch";
    case AUDIO_GAIN_TYPE_AF:                return "af";
    case AUDIO_GAIN_TYPE_SHUTTER1:          return "shutter1";
    case AUDIO_GAIN_TYPE_SHUTTER2:          return "shutter2";
    case AUDIO_GAIN_TYPE_CAMCODING:         return "camcording";
    case AUDIO_GAIN_TYPE_MIDI:              return "midi";
    case AUDIO_GAIN_TYPE_BOOTING:           return "booting";
    case AUDIO_GAIN_TYPE_VIDEO:             return "video";
    case AUDIO_GAIN_TYPE_TTS:               return "tts";
    default:                                return "invalid";
    }
}

static void __dump_tb(audio_hal_t *ah)
{
    audio_volume_value_table_t *volume_value_table = ah->volume.volume_value_table;
    uint32_t vol_type_idx, vol_level_idx, gain_type_idx;
    const char *gain_type_str[] = {
        "def",          /* AUDIO_GAIN_TYPE_DEFAULT */
        "dial",         /* AUDIO_GAIN_TYPE_DIALER */
        "touch",        /* AUDIO_GAIN_TYPE_TOUCH */
        "af",           /* AUDIO_GAIN_TYPE_AF */
        "shut1",        /* AUDIO_GAIN_TYPE_SHUTTER1 */
        "shut2",        /* AUDIO_GAIN_TYPE_SHUTTER2 */
        "cam",          /* AUDIO_GAIN_TYPE_CAMCODING */
        "midi",         /* AUDIO_GAIN_TYPE_MIDI */
        "boot",         /* AUDIO_GAIN_TYPE_BOOTING */
        "video",        /* AUDIO_GAIN_TYPE_VIDEO */
        "tts",          /* AUDIO_GAIN_TYPE_TTS */
    };
    char dump_str[AUDIO_DUMP_STR_LEN], *dump_str_ptr;

    /* Dump volume table */
    AUDIO_LOG_INFO("<<<<< volume table >>>>>");

    const char *table_str = "volumes";

    AUDIO_LOG_INFO("<< %s >>", table_str);

    for (vol_type_idx = 0; vol_type_idx < AUDIO_VOLUME_TYPE_MAX; vol_type_idx++) {
        const char *vol_type_str = __get_volume_type_string_by_idx(vol_type_idx);

        dump_str_ptr = &dump_str[0];
        memset(dump_str, 0x00, sizeof(char) * sizeof(dump_str));
        snprintf(dump_str_ptr, 8, "%6s:", vol_type_str);
        dump_str_ptr += strlen(dump_str_ptr);

        for (vol_level_idx = 0; vol_level_idx < volume_value_table->volume_level_max[vol_type_idx]; vol_level_idx++) {
            snprintf(dump_str_ptr, 6, "%01.2f ", volume_value_table->volume[vol_type_idx][vol_level_idx]);
            dump_str_ptr += strlen(dump_str_ptr);
        }
        AUDIO_LOG_INFO("%s", dump_str);
    }

    volume_value_table = ah->volume.volume_value_table;

    /* Dump gain table */
    AUDIO_LOG_INFO("<<<<< gain table >>>>>");

    dump_str_ptr = &dump_str[0];
    memset(dump_str, 0x00, sizeof(char) * sizeof(dump_str));

    snprintf(dump_str_ptr, 11, "%10s", " ");
    dump_str_ptr += strlen(dump_str_ptr);

    for (gain_type_idx = 0; gain_type_idx < AUDIO_GAIN_TYPE_MAX; gain_type_idx++) {
        snprintf(dump_str_ptr, 7, "%5s ", gain_type_str[gain_type_idx]);
        dump_str_ptr += strlen(dump_str_ptr);
    }
    AUDIO_LOG_INFO("%s", dump_str);

    dump_str_ptr = &dump_str[0];
    memset(dump_str, 0x00, sizeof(char) * sizeof(dump_str));

    snprintf(dump_str_ptr, 11, "%9s:", table_str);
    dump_str_ptr += strlen(dump_str_ptr);

    for (gain_type_idx = 0; gain_type_idx < AUDIO_GAIN_TYPE_MAX; gain_type_idx++) {
        snprintf(dump_str_ptr, 7, "%01.3f ", volume_value_table->gain[gain_type_idx]);
        dump_str_ptr += strlen(dump_str_ptr);
    }
    AUDIO_LOG_INFO("%s", dump_str);

}

static audio_return_t __load_volume_value_table_from_ini(audio_hal_t *ah)
{
    dictionary * dict = NULL;
    uint32_t vol_type_idx, vol_level_idx, gain_type_idx;
    audio_volume_value_table_t *volume_value_table = ah->volume.volume_value_table;
    int size = 0;

    dict = iniparser_load(VOLUME_INI_TEMP_PATH);
    if (!dict) {
        AUDIO_LOG_DEBUG("Use default volume&gain ini file");
        dict = iniparser_load(VOLUME_INI_DEFAULT_PATH);
        if (!dict) {
            AUDIO_LOG_WARN("Loading volume&gain table from ini file failed");
            return AUDIO_ERR_UNDEFINED;
        }
    }

    const char delimiter[] = ", ";
    char *key, *list_str, *token, *ptr = NULL;
    const char *table_str = "volumes";

    /* Load volume table */
    for (vol_type_idx = 0; vol_type_idx < AUDIO_VOLUME_TYPE_MAX; vol_type_idx++) {
        const char *vol_type_str = __get_volume_type_string_by_idx(vol_type_idx);

        volume_value_table->volume_level_max[vol_type_idx] = 0;
        size = strlen(table_str) + strlen(vol_type_str) + 2;
        key = malloc(size);
        if (key) {
            snprintf(key, size, "%s:%s", table_str, vol_type_str);
            list_str = iniparser_getstring(dict, key, NULL);
            if (list_str) {
                token = strtok_r(list_str, delimiter, &ptr);
                while (token) {
                    /* convert dB volume to linear volume */
                    double vol_value = 0.0f;
                    if (strncmp(token, "0", strlen(token)))
                        vol_value = pow(10.0, (atof(token) - 100) / 20.0);
                    volume_value_table->volume[vol_type_idx][volume_value_table->volume_level_max[vol_type_idx]++] = vol_value;
                    token = strtok_r(NULL, delimiter, &ptr);
                }
            } else {
                volume_value_table->volume_level_max[vol_type_idx] = 1;
                for (vol_level_idx = 0; vol_level_idx < AUDIO_VOLUME_LEVEL_MAX; vol_level_idx++) {
                    volume_value_table->volume[vol_type_idx][vol_level_idx] = VOLUME_VALUE_MAX;
                }
            }
            free(key);
        }
    }

    /* Load gain table */
    volume_value_table->gain[AUDIO_GAIN_TYPE_DEFAULT] = GAIN_VALUE_MAX;
    for (gain_type_idx = AUDIO_GAIN_TYPE_DEFAULT + 1; gain_type_idx < AUDIO_GAIN_TYPE_MAX; gain_type_idx++) {
        const char *gain_type_str = __get_gain_type_string_by_idx(gain_type_idx);

        size = strlen(table_str) + strlen("gain") + strlen(gain_type_str) + 3;
        key = malloc(size);
        if (key) {
            snprintf(key, size, "%s:gain_%s", table_str, gain_type_str);
            token = iniparser_getstring(dict, key, NULL);
            if (token) {
                volume_value_table->gain[gain_type_idx] = atof(token);
            } else {
                volume_value_table->gain[gain_type_idx] = GAIN_VALUE_MAX;
            }
            free(key);
        } else {
            volume_value_table->gain[gain_type_idx] = GAIN_VALUE_MAX;
        }
    }

    iniparser_freedict(dict);

    __dump_tb(ah);

    return AUDIO_RET_OK;
}

audio_return_t _audio_volume_init(audio_hal_t *ah)
{
    int i;
    int val = 0;
    audio_return_t audio_ret = AUDIO_RET_OK;
    int init_value[AUDIO_VOLUME_TYPE_MAX] = { 9, 11, 7, 11, 7, 4, 4, 7 };

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    for (i = 0; i < AUDIO_VOLUME_TYPE_MAX; i++) {
        ah->volume.volume_level[i] = init_value[i];
    }

    for (i = 0; i < AUDIO_VOLUME_TYPE_MAX; i++) {
        /* Get volume value string from VCONF */
        if (vconf_get_int(g_volume_vconf[i], &val) < 0) {
            AUDIO_LOG_ERROR("vconf_get_int(%s) failed", g_volume_vconf[i]);
            continue;
        }

        AUDIO_LOG_INFO("read vconf. %s = %d", g_volume_vconf[i], val);
        ah->volume.volume_level[i] = val;
    }

    if (!(ah->volume.volume_value_table = malloc(AUDIO_VOLUME_DEVICE_MAX * sizeof(audio_volume_value_table_t)))) {
        AUDIO_LOG_ERROR("volume_value_table malloc failed");
        return AUDIO_ERR_RESOURCE;
    }

    audio_ret = __load_volume_value_table_from_ini(ah);
    if (audio_ret != AUDIO_RET_OK) {
        AUDIO_LOG_ERROR("gain table load error");
        return AUDIO_ERR_UNDEFINED;
    }

    return audio_ret;
}

audio_return_t _audio_volume_deinit(audio_hal_t *ah)
{
    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    if (ah->volume.volume_value_table) {
        free(ah->volume.volume_value_table);
        ah->volume.volume_value_table = NULL;
    }

    return AUDIO_RET_OK;
}

audio_return_t audio_get_volume_level_max(void *audio_handle, audio_volume_info_t *info, uint32_t *level)
{
    audio_hal_t *ah = (audio_hal_t *)audio_handle;
    audio_volume_value_table_t *volume_value_table;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(ah->volume.volume_value_table, AUDIO_ERR_PARAMETER);

    /* Get max volume level by device & type */
    volume_value_table = ah->volume.volume_value_table;
    *level = volume_value_table->volume_level_max[__get_volume_idx_by_string_type(info->type)];

    AUDIO_LOG_DEBUG("get_[%s] volume_level_max: %d", info->type, *level);

    return AUDIO_RET_OK;
}

audio_return_t audio_get_volume_level(void *audio_handle, audio_volume_info_t *info, uint32_t *level)
{
    audio_hal_t *ah = (audio_hal_t *)audio_handle;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    *level = ah->volume.volume_level[__get_volume_idx_by_string_type(info->type)];

    AUDIO_LOG_INFO("get [%s] volume_level: %d, direction(%d)", info->type, *level, info->direction);

    return AUDIO_RET_OK;
}

audio_return_t audio_get_volume_value(void *audio_handle, audio_volume_info_t *info, uint32_t level, double *value)
{
    audio_hal_t *ah = (audio_hal_t *)audio_handle;
    audio_volume_value_table_t *volume_value_table;
    char dump_str[AUDIO_DUMP_STR_LEN] = {0,};

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    AUDIO_RETURN_VAL_IF_FAIL(ah->volume.volume_value_table, AUDIO_ERR_PARAMETER);

    /* Get basic volume by device & type & level */
    volume_value_table = ah->volume.volume_value_table;
    if (volume_value_table->volume_level_max[__get_volume_idx_by_string_type(info->type)] < level)
        *value = VOLUME_VALUE_MAX;
    else
        *value = volume_value_table->volume[__get_volume_idx_by_string_type(info->type)][level];
    *value *= volume_value_table->gain[AUDIO_GAIN_TYPE_DEFAULT]; /* need to fix getting gain via audio_info_t */

    AUDIO_LOG_DEBUG("get_volume_value:%d(%s)=>%f %s", level, info->type, *value, &dump_str[0]);

    return AUDIO_RET_OK;
}

audio_return_t audio_set_volume_level(void *audio_handle, audio_volume_info_t *info, uint32_t level)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = (audio_hal_t *)audio_handle;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    /* Update volume level */
    ah->volume.volume_level[__get_volume_idx_by_string_type(info->type)] = level;
    AUDIO_LOG_INFO("set [%s] volume_level: %d, direction(%d)", info->type, level, info->direction);

    /* set mixer related to H/W volume if needed */

    return audio_ret;
}

audio_return_t audio_get_volume_mute(void *audio_handle, audio_volume_info_t *info, uint32_t *mute)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = (audio_hal_t *)audio_handle;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);

    /* TODO. Not implemented */

    return audio_ret;
}

audio_return_t audio_set_volume_mute(void *audio_handle, audio_volume_info_t *info, uint32_t mute)
{
    audio_return_t audio_ret = AUDIO_RET_OK;
    audio_hal_t *ah = (audio_hal_t *)audio_handle;

    AUDIO_RETURN_VAL_IF_FAIL(ah, AUDIO_ERR_PARAMETER);
    /* TODO. Not implemented */

    return audio_ret;
}
