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
#include <string.h>

#include "tizen-audio-internal.h"

/* ------ dump helper --------  */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

dump_data_t* _audio_dump_new(int length)
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

void _audio_dump_add_str(dump_data_t *dump, const char *fmt, ...)
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

char* _audio_dump_get_str(dump_data_t *dump)
{
    return (dump) ? dump->strbuf : NULL;
}

void _audio_dump_free(dump_data_t *dump)
{
    if (dump) {
        if (dump->strbuf)
            free(dump->strbuf);
        free(dump);
    }
}
/* ------ dump helper --------  */
