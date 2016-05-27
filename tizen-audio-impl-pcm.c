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
#include "tizen-audio-impl.h"

#ifdef __USE_TINYALSA__
/* Convert pcm format from pulse to alsa */
static const uint32_t g_format_convert_table[] = {
    [AUDIO_SAMPLE_U8]        = PCM_FORMAT_S8,
    [AUDIO_SAMPLE_S16LE]     = PCM_FORMAT_S16_LE,
    [AUDIO_SAMPLE_S32LE]     = PCM_FORMAT_S32_LE,
    [AUDIO_SAMPLE_S24_32LE]  = PCM_FORMAT_S24_LE
};
#else  /* alsa-lib */
/* FIXME : To avoid build warning... */
int _snd_pcm_poll_descriptor(snd_pcm_t *pcm);
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

static uint32_t __convert_format(audio_sample_format_t format)
{
    return g_format_convert_table[format];
}

#ifdef __USE_TINYALSA__
static struct pcm *__tinyalsa_open_device(audio_pcm_sample_spec_t *ss, size_t period_size, size_t period_count, uint32_t direction)
{
    struct pcm *pcm = NULL;
    struct pcm_config config;

    AUDIO_RETURN_NULL_IF_FAIL(ss);

    config.channels          = ss->channels;
    config.rate              = ss->rate;
    config.period_size       = period_size;
    config.period_count      = period_count;
    config.format            = ss->format;
    config.start_threshold   = period_size;
    config.stop_threshold    = 0xFFFFFFFF;
    config.silence_threshold = 0;

    AUDIO_LOG_INFO("direction %d, channels %d, rate %d, format %d, period_size %d, period_count %d", direction, ss->channels, ss->rate, ss->format, period_size, period_count);

    pcm = pcm_open((direction == AUDIO_DIRECTION_OUT) ? PLAYBACK_CARD_ID : CAPTURE_CARD_ID,
                   (direction == AUDIO_DIRECTION_OUT) ? PLAYBACK_PCM_DEVICE_ID : CAPTURE_PCM_DEVICE_ID,
                   (direction == AUDIO_DIRECTION_OUT) ? PCM_OUT : PCM_IN,
                   &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        AUDIO_LOG_ERROR("Unable to open device (%s)", pcm_get_error(pcm));
        pcm_close(pcm);
        return NULL;
    }

    return pcm;
}

static int __tinyalsa_pcm_recover(struct pcm *pcm, int err)
{
    if (err > 0)
        err = -err;
    if (err == -EINTR)  /* nothing to do, continue */
        return 0;
    if (err == -EPIPE) {
        AUDIO_LOG_INFO("XRUN occurred");
        err = pcm_prepare(pcm);
        if (err < 0) {
            AUDIO_LOG_ERROR("Could not recover from XRUN occurred, prepare failed : %d", err);
            return err;
        }
        return 0;
    }
    if (err == -ESTRPIPE) {
        /* tinyalsa does not support pcm resume, dont't care suspend case */
        AUDIO_LOG_ERROR("Could not recover from suspend : %d", err);
        return err;
    }
    return err;
}
#endif

audio_return_t _pcm_open(void **pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods)
{
#ifdef __USE_TINYALSA__
    audio_pcm_sample_spec_t *ss;
    int err;

    ss = (audio_pcm_sample_spec_t *)sample_spec;
    ss->format = __convert_format((audio_sample_format_t)ss->format);

    *pcm_handle = __tinyalsa_open_device(ss, (size_t)period_size, (size_t)periods, direction);
    if (*pcm_handle == NULL) {
        AUDIO_LOG_ERROR("Error opening PCM device");
        return AUDIO_ERR_RESOURCE;
    }

    if ((err = pcm_prepare((struct pcm *)*pcm_handle)) != 0) {
        AUDIO_LOG_ERROR("Error prepare PCM device : %d", err);
    }

#else  /* alsa-lib */
    int err, mode;
    char *device_name = NULL;

    mode =  SND_PCM_NONBLOCK | SND_PCM_NO_AUTO_RESAMPLE | SND_PCM_NO_AUTO_CHANNELS | SND_PCM_NO_AUTO_FORMAT;

    if (direction == AUDIO_DIRECTION_OUT)
        device_name = PLAYBACK_PCM_DEVICE;
    else if (direction == AUDIO_DIRECTION_IN)
        device_name = CAPTURE_PCM_DEVICE;
    else {
        AUDIO_LOG_ERROR("Error get device_name, direction : %d", direction);
        return AUDIO_ERR_RESOURCE;
    }

    if ((err = snd_pcm_open((snd_pcm_t **)pcm_handle, device_name, (direction == AUDIO_DIRECTION_OUT) ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE, mode)) < 0) {
        AUDIO_LOG_ERROR("Error opening PCM device %s : %s", device_name, snd_strerror(err));
        return AUDIO_ERR_RESOURCE;
    }

    if ((err = _pcm_set_params(*pcm_handle, direction, sample_spec, period_size, periods)) != AUDIO_RET_OK) {
        AUDIO_LOG_ERROR("Failed to set pcm parameters : %d", err);
        return err;
    }

    AUDIO_LOG_INFO("PCM device %s", device_name);
#endif

    return AUDIO_RET_OK;
}

audio_return_t _pcm_start(void *pcm_handle)
{
    int err;

#ifdef __USE_TINYALSA__
    if ((err = pcm_start(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error starting PCM handle : %d", err);
        return AUDIO_ERR_RESOURCE;
    }
#else  /* alsa-lib */
    if ((err = snd_pcm_start(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error starting PCM handle : %s", snd_strerror(err));
        return AUDIO_ERR_RESOURCE;
    }
#endif

    AUDIO_LOG_INFO("PCM handle 0x%x start", pcm_handle);
    return AUDIO_RET_OK;
}

audio_return_t _pcm_stop(void *pcm_handle)
{
    int err;

#ifdef __USE_TINYALSA__
    if ((err = pcm_stop(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error stopping PCM handle : %d", err);
        return AUDIO_ERR_RESOURCE;
    }
#else  /* alsa-lib */
    if ((err = snd_pcm_drop(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error stopping PCM handle : %s", snd_strerror(err));
        return AUDIO_ERR_RESOURCE;
    }
#endif

    AUDIO_LOG_INFO("PCM handle 0x%x stop", pcm_handle);
    return AUDIO_RET_OK;
}

audio_return_t _pcm_close(void *pcm_handle)
{
    int err;

    AUDIO_LOG_INFO("Try to close PCM handle 0x%x", pcm_handle);

#ifdef __USE_TINYALSA__
    if ((err = pcm_close(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error closing PCM handle : %d", err);
        return AUDIO_ERR_RESOURCE;
    }
#else  /* alsa-lib */
    if ((err = snd_pcm_close(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Error closing PCM handle : %s", snd_strerror(err));
        return AUDIO_ERR_RESOURCE;
    }
#endif

    return AUDIO_RET_OK;
}

audio_return_t _pcm_avail(void *pcm_handle, uint32_t *avail)
{
#ifdef __USE_TINYALSA__
    struct timespec tspec;
    unsigned int frames_avail = 0;
    int err;

    err = pcm_get_htimestamp(pcm_handle, &frames_avail, &tspec);
    if (err < 0) {
        AUDIO_LOG_ERROR("Could not get avail and timespec at PCM handle 0x%x : %d", pcm_handle, err);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("avail = %d", frames_avail);
#endif

    *avail = (uint32_t)frames_avail;
#else  /* alsa-lib */
    snd_pcm_sframes_t frames_avail;

    if ((frames_avail = snd_pcm_avail(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("Could not get avail at PCM handle 0x%x : %d", pcm_handle, frames_avail);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("avail = %d", frames_avail);
#endif

    *avail = (uint32_t)frames_avail;
#endif

    return AUDIO_RET_OK;
}

audio_return_t _pcm_write(void *pcm_handle, const void *buffer, uint32_t frames)
{
#ifdef __USE_TINYALSA__
    int err;

    err = pcm_write(pcm_handle, buffer, pcm_frames_to_bytes(pcm_handle, (unsigned int)frames));
    if (err < 0) {
        AUDIO_LOG_ERROR("Failed to write pcm : %d", err);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("_pcm_write = %d", frames);
#endif
#else  /* alsa-lib */
    snd_pcm_sframes_t frames_written;

    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    frames_written = snd_pcm_writei(pcm_handle, buffer, (snd_pcm_uframes_t) frames);
    if (frames_written < 0) {
        AUDIO_LOG_ERROR("Failed to write pcm : %d", frames_written);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("_pcm_write = (%d / %d)", frames_written, frames);
#endif
#endif

    return AUDIO_RET_OK;
}

audio_return_t _pcm_read(void *pcm_handle, void *buffer, uint32_t frames)
{
#ifdef __USE_TINYALSA__
    int err;

    err = pcm_read(pcm_handle, buffer, pcm_frames_to_bytes(pcm_handle, (unsigned int)frames));
    if (err < 0) {
        AUDIO_LOG_ERROR("Failed to read pcm : %d", err);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("audio_pcm_read = %d", frames);
#endif
#else  /* alsa-lib */
    snd_pcm_sframes_t frames_read;

    frames_read = snd_pcm_readi(pcm_handle, buffer, (snd_pcm_uframes_t)frames);
    if (frames_read < 0) {
        AUDIO_LOG_ERROR("Failed to read pcm : %d", frames_read);
        return AUDIO_ERR_IOCTL;
    }

#ifdef DEBUG_TIMING
    AUDIO_LOG_DEBUG("_pcm_read = (%d / %d)", frames_read, frames);
#endif
#endif

    return AUDIO_RET_OK;
}

audio_return_t _pcm_get_fd(void *pcm_handle, int *fd)
{
    /* we use an internal API of the (tiny)alsa library, so it causes warning message during compile */
#ifdef __USE_TINYALSA__
    *fd = _pcm_poll_descriptor((struct pcm *)pcm_handle);
#else  /* alsa-lib */
    *fd = _snd_pcm_poll_descriptor((snd_pcm_t *)pcm_handle);
#endif
    return AUDIO_RET_OK;
}

audio_return_t _pcm_recover(void *pcm_handle, int revents)
{
    int state, err;

    AUDIO_RETURN_VAL_IF_FAIL(pcm_handle, AUDIO_ERR_PARAMETER);

    if (revents & POLLERR)
        AUDIO_LOG_DEBUG("Got POLLERR from ALSA");
    if (revents & POLLNVAL)
        AUDIO_LOG_DEBUG("Got POLLNVAL from ALSA");
    if (revents & POLLHUP)
        AUDIO_LOG_DEBUG("Got POLLHUP from ALSA");
    if (revents & POLLPRI)
        AUDIO_LOG_DEBUG("Got POLLPRI from ALSA");
    if (revents & POLLIN)
        AUDIO_LOG_DEBUG("Got POLLIN from ALSA");
    if (revents & POLLOUT)
        AUDIO_LOG_DEBUG("Got POLLOUT from ALSA");

#ifdef __USE_TINYALSA__
    state = pcm_state(pcm_handle);
    AUDIO_LOG_DEBUG("PCM state is %d", state);

    switch (state) {
        case PCM_STATE_XRUN:
            if ((err = __tinyalsa_pcm_recover(pcm_handle, -EPIPE)) != 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP and XRUN : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;

        case PCM_STATE_SUSPENDED:
            if ((err = __tinyalsa_pcm_recover(pcm_handle, -ESTRPIPE)) != 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP and SUSPENDED : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;

        default:
            pcm_stop(pcm_handle);
            if ((err = pcm_prepare(pcm_handle)) < 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP with pcm_prepare() : %d", err);
                return AUDIO_ERR_IOCTL;
            }
    }
#else  /* alsa-lib */
    state = snd_pcm_state(pcm_handle);
    AUDIO_LOG_DEBUG("PCM state is %s", snd_pcm_state_name(state));

    /* Try to recover from this error */

    switch (state) {
        case SND_PCM_STATE_XRUN:
            if ((err = snd_pcm_recover(pcm_handle, -EPIPE, 1)) != 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP and XRUN : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;

        case SND_PCM_STATE_SUSPENDED:
            if ((err = snd_pcm_recover(pcm_handle, -ESTRPIPE, 1)) != 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP and SUSPENDED : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;

        default:
            snd_pcm_drop(pcm_handle);
            if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
                AUDIO_LOG_ERROR("Could not recover from POLLERR|POLLNVAL|POLLHUP with snd_pcm_prepare() : %d", err);
                return AUDIO_ERR_IOCTL;
            }
            break;
    }
#endif

    AUDIO_LOG_DEBUG("_pcm_recover");
    return AUDIO_RET_OK;
}

audio_return_t _pcm_get_params(void *pcm_handle, uint32_t direction, void **sample_spec, uint32_t *period_size, uint32_t *periods)
{
#ifdef __USE_TINYALSA__
    audio_pcm_sample_spec_t *ss;
    unsigned int _period_size, _buffer_size, _periods, _format, _rate, _channels;
    unsigned int _start_threshold, _stop_threshold, _silence_threshold;
    struct pcm_config *config;

    ss = (audio_pcm_sample_spec_t *)*sample_spec;

    /* we use an internal API of the tiny alsa library, so it causes warning message during compile */
    _pcm_config(pcm_handle, &config);

    *period_size = config->period_size;
    *periods     = config->period_count;
    _buffer_size = config->period_size * config->period_count;
    ss->format   = config->format;
    ss->rate     = config->rate;
    ss->channels = config->channels;
    _start_threshold   = config->start_threshold;
    _stop_threshold    = config->stop_threshold;
    _silence_threshold = config->silence_threshold;

    AUDIO_LOG_DEBUG("_pcm_get_params (handle 0x%x, format %d, rate %d, channels %d, period_size %d, periods %d, buffer_size %d)", pcm_handle, config->format, config->rate, config->channels, config->period_size, config->period_count, _buffer_size);
#else  /* alsa-lib */
    int err;
    audio_pcm_sample_spec_t *ss;
    int dir;
    snd_pcm_uframes_t _period_size, _buffer_size;
    snd_pcm_format_t _format;
    unsigned int _rate, _channels;
    snd_pcm_uframes_t _start_threshold, _stop_threshold, _silence_threshold, _avail_min;
    unsigned int _periods;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;

    ss = (audio_pcm_sample_spec_t *)*sample_spec;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    if ((err = snd_pcm_hw_params_current(pcm_handle, hwparams)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_current() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_get_period_size(hwparams, &_period_size, &dir)) < 0 ||
        (err = snd_pcm_hw_params_get_buffer_size(hwparams, &_buffer_size)) < 0 ||
        (err = snd_pcm_hw_params_get_periods(hwparams, &_periods, &dir)) < 0 ||
        (err = snd_pcm_hw_params_get_format(hwparams, &_format)) < 0 ||
        (err = snd_pcm_hw_params_get_rate(hwparams, &_rate, &dir)) < 0 ||
        (err = snd_pcm_hw_params_get_channels(hwparams, &_channels)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_get_{period_size|buffer_size|periods|format|rate|channels}() failed : %s", err);
        return AUDIO_ERR_PARAMETER;
    }

    *period_size = _period_size;
    *periods     = _periods;
    ss->format   = _format;
    ss->rate     = _rate;
    ss->channels = _channels;

    if ((err = snd_pcm_sw_params_current(pcm_handle, swparams)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_sw_params_current() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_get_start_threshold(swparams, &_start_threshold)) < 0  ||
        (err = snd_pcm_sw_params_get_stop_threshold(swparams, &_stop_threshold)) < 0  ||
        (err = snd_pcm_sw_params_get_silence_threshold(swparams, &_silence_threshold)) < 0  ||
        (err = snd_pcm_sw_params_get_avail_min(swparams, &_avail_min)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_sw_params_get_{start_threshold|stop_threshold|silence_threshold|avail_min}() failed : %s", err);
    }

    AUDIO_LOG_DEBUG("_pcm_get_params (handle 0x%x, format %d, rate %d, channels %d, period_size %d, periods %d, buffer_size %d)", pcm_handle, _format, _rate, _channels, _period_size, _periods, _buffer_size);
#endif

    return AUDIO_RET_OK;
}

audio_return_t _pcm_set_params(void *pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods)
{
#ifdef __USE_TINYALSA__
    /* Parameters are only acceptable in pcm_open() function */
    AUDIO_LOG_DEBUG("_pcm_set_params");
#else  /* alsa-lib */
    int err;
    audio_pcm_sample_spec_t ss;
    snd_pcm_uframes_t _buffer_size;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;

    ss = *(audio_pcm_sample_spec_t *)sample_spec;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    /* Set hw params */
    if ((err = snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_any() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_rate_resample(pcm_handle, hwparams, 0)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_rate_resample() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_access() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    ss.format = __convert_format((audio_sample_format_t)ss.format);
    if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, ss.format)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_format() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_rate(pcm_handle, hwparams, ss.rate, 0)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_rate() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, ss.channels)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_channels(%u) failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_period_size(pcm_handle, hwparams, period_size, 0)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_period_size(%u) failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_periods(%u) failed : %d", periods, err);
        return AUDIO_ERR_PARAMETER;
    }

    _buffer_size = period_size * periods;
    if ((err = snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, _buffer_size)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params_set_buffer_size(%u) failed : %d", periods * periods, err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_hw_params failed : %d", err);
        return AUDIO_ERR_IOCTL;
    }

    /* Set sw params */
    if ((err = snd_pcm_sw_params_current(pcm_handle, swparams)) < 0) {
        AUDIO_LOG_ERROR("Unable to determine current swparams : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_set_tstamp_mode(pcm_handle, swparams, SND_PCM_TSTAMP_ENABLE)) < 0) {
        AUDIO_LOG_ERROR("Unable to enable time stamping : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, 0xFFFFFFFF)) < 0) {
        AUDIO_LOG_ERROR("Unable to set stop threshold : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, period_size / 2)) < 0) {
        AUDIO_LOG_ERROR("Unable to set start threshold : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, 1024)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_sw_params_set_avail_min() failed : %d", err);
        return AUDIO_ERR_PARAMETER;
    }

    if ((err = snd_pcm_sw_params(pcm_handle, swparams)) < 0) {
        AUDIO_LOG_ERROR("Unable to set sw params : %d", err);
        return AUDIO_ERR_IOCTL;
    }

    /* Prepare device */
    if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
        AUDIO_LOG_ERROR("snd_pcm_prepare() failed : %d", err);
        return AUDIO_ERR_IOCTL;
    }

    AUDIO_LOG_DEBUG("_pcm_set_params (handle 0x%x, format %d, rate %d, channels %d, period_size %d, periods %d, buffer_size %d)", pcm_handle, ss.format, ss.rate, ss.channels, period_size, periods, _buffer_size);
#endif

    return AUDIO_RET_OK;
}

/* Generic snd pcm interface APIs */
audio_return_t _pcm_set_hw_params(snd_pcm_t *pcm, audio_pcm_sample_spec_t *sample_spec, uint8_t *use_mmap, snd_pcm_uframes_t *period_size, snd_pcm_uframes_t *buffer_size)
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

audio_return_t _pcm_set_sw_params(snd_pcm_t *pcm, snd_pcm_uframes_t avail_min, uint8_t period_event)
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
