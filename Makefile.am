lib_LTLIBRARIES = libtizen-audio.la

libtizen_audio_la_SOURCES = tizen-audio.c \
		tizen-audio-device.c \
		tizen-audio-volume.c \
		tizen-audio-ucm.c \
		tizen-audio-util.c
libtizen_audio_la_LDFLAGS = $(AM_LDFLAGS) -disable-static -avoid-version
libtizen_audio_la_LIBADD = $(AM_LDADD) $(ASOUNDLIB_LIBS) $(VCONF_LIBS) $(DLOG_LIBS) $(INIPARSER_LIBS)
libtizen_audio_la_CFLAGS = $(AM_CFLAGS) $(ASOUNDLIB_CFLAGS) $(VCONF_CFLAGS) $(DLOG_CFLAGS) $(INIPARSER_CFLAGS)
libtizen_audio_la_CFLAGS += -DUSE_DLOG

