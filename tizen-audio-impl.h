#ifndef footizenaudioimplfoo
#define footizenaudioimplfoo

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

/* PCM */
audio_return_t _pcm_open(void **pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods);
audio_return_t _pcm_start(void *pcm_handle);
audio_return_t _pcm_stop(void *pcm_handle);
audio_return_t _pcm_close(void *pcm_handle);
audio_return_t _pcm_avail(void *pcm_handle, uint32_t *avail);
audio_return_t _pcm_write(void *pcm_handle, const void *buffer, uint32_t frames);
audio_return_t _pcm_read(void *pcm_handle, void *buffer, uint32_t frames);
audio_return_t _pcm_get_fd(void *pcm_handle, int *fd);
audio_return_t _pcm_recover(void *pcm_handle, int revents);
audio_return_t _pcm_get_params(void *pcm_handle, uint32_t direction, void **sample_spec, uint32_t *period_size, uint32_t *periods);
audio_return_t _pcm_set_params(void *pcm_handle, uint32_t direction, void *sample_spec, uint32_t period_size, uint32_t periods);
audio_return_t _pcm_set_sw_params(snd_pcm_t *pcm, snd_pcm_uframes_t avail_min, uint8_t period_event);
audio_return_t _pcm_set_hw_params(snd_pcm_t *pcm, audio_pcm_sample_spec_t *sample_spec, uint8_t *use_mmap, snd_pcm_uframes_t *period_size, snd_pcm_uframes_t *buffer_size);

/* Control */
audio_return_t _control_init(audio_hal_t *ah);
audio_return_t _control_deinit(audio_hal_t *ah);
audio_return_t _mixer_control_set_param(audio_hal_t *ah, const char* ctl_name, snd_ctl_elem_value_t* value, int size);
audio_return_t _mixer_control_set_value(audio_hal_t *ah, const char *ctl_name, int val);
audio_return_t _mixer_control_set_value_string(audio_hal_t *ah, const char* ctl_name, const char* value);
audio_return_t _mixer_control_get_value(audio_hal_t *ah, const char *ctl_name, int *val);
audio_return_t _mixer_control_get_element(audio_hal_t *ah, const char *ctl_name, snd_hctl_elem_t **elem);

/* UCM  */
audio_return_t _ucm_init(audio_hal_t *ah);
audio_return_t _ucm_deinit(audio_hal_t *ah);
#define _ucm_update_use_case _ucm_set_use_case
audio_return_t _ucm_set_use_case(audio_hal_t *ah, const char *verb, const char *devices[], const char *modifiers[]);
audio_return_t _ucm_set_devices(audio_hal_t *ah, const char *verb, const char *devices[]);
audio_return_t _ucm_set_modifiers(audio_hal_t *ah, const char *verb, const char *modifiers[]);
audio_return_t _ucm_get_verb(audio_hal_t *ah, const char **value);
audio_return_t _ucm_reset_use_case(audio_hal_t *ah);

#endif
