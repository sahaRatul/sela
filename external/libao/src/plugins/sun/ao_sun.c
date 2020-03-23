/*
 *
 *  ao_sun.c    Solaris/NetBSD/OpenBSD
 *
 *      Original Copyright (C) Aaron Holtzman - May 1999
 *      Modifications Copyright (C) Stan Seibert - July 2000
 *      and Copyright (C) Christian Weisgerber - March 2001
 *
 *  libao is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  libao is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ********************************************************************

 last mod: $Id$

 ********************************************************************/

#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/audioio.h>

#include <ao/ao.h>
#include <ao/plugin.h>


#ifndef AUDIO_ENCODING_SLINEAR
#define AUDIO_ENCODING_SLINEAR AUDIO_ENCODING_LINEAR	/* Solaris */
#endif

#ifndef AO_SUN_ENV_DEV1
/* SunRay device environment variable, look here first */
#define AO_SUN_ENV_DEV1 "UTAUDIODEV"
#endif

#ifndef AO_SUN_ENV_DEV2
/* Ordinary Sun audio device environment variable, look here second */
#define AO_SUN_ENV_DEV2 "AUDIODEV"
#endif

/* No environment variable, try the default local device */
#ifndef AO_SUN_DEFAULT_DEV
#define AO_SUN_DEFAULT_DEV "/dev/audio"
#endif


static char *ao_sun_options[] = {"dev","id","verbose","quiet","matrix","debug"};
ao_info ao_sun_info = {
	AO_TYPE_LIVE,
	"Sun audio driver output",
	"sun",
	"Christian Weisgerber <naddy@openbsd.org>",
	"Outputs to the sun audio system.",
	AO_FMT_NATIVE,
	20,
	ao_sun_options,
        sizeof(ao_sun_options)/sizeof(*ao_sun_options)
};


typedef struct ao_sun_internal {
	char *dev;
        int id;
	int fd;
} ao_sun_internal;


int ao_plugin_test()
{
	int fd;
        char *dev = NULL;

        dev = getenv(AO_SUN_ENV_DEV1);
        if (!dev) dev = getenv(AO_SUN_ENV_DEV2);
        if (!dev) dev = AO_SUN_DEFAULT_DEV;

	if ( (fd = open(dev, O_WRONLY | O_NONBLOCK)) < 0 )
		return 0; /* Cannot use this plugin with default parameters */
	else {
		close(fd);
		return 1; /* This plugin works in default mode */
	}
}


ao_info *ao_plugin_driver_info(void)
{
	return &ao_sun_info;
}


int ao_plugin_device_init(ao_device *device)
{
	ao_sun_internal *internal;
        char *dev = NULL;

	internal = (ao_sun_internal *) calloc(1,sizeof(ao_sun_internal));

	if (internal == NULL)
		return 0; /* Could not initialize device memory */

        dev = getenv(AO_SUN_ENV_DEV1);
        if (!dev) dev = getenv(AO_SUN_ENV_DEV2);
        if (!dev) dev = AO_SUN_DEFAULT_DEV;
	internal->dev = strdup(dev);

	if (internal->dev == NULL) {
		free(internal);
		return 0;
	}

	device->internal = internal;
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

	return 1; /* Memory alloc successful */
}

int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
{
	ao_sun_internal *internal = (ao_sun_internal *) device->internal;


	if (!strcmp(key, "dev")) {
		/* Free old string in case "dsp" set twice in options */
		free(internal->dev);
		internal->dev = strdup(value);
	}
	if (!strcmp(key, "id")) {
		/* Free old string in case "dsp" set twice in options */
		free(internal->dev);
		internal->dev = NULL;
                internal->id=atoi(value);
	}

	return 1;
}


int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
	ao_sun_internal *internal = (ao_sun_internal *) device->internal;

	audio_info_t info;

        if(internal->dev==NULL){
          char buf[80];
          sprintf(buf,"/dev/sound/%d",internal->id);
          internal->dev=strdup(buf);
        }

	if ( (internal->fd = open(internal->dev, O_WRONLY)) < 0 )
		return 0;

	AUDIO_INITINFO(&info);
#ifdef AUMODE_PLAY	/* NetBSD/OpenBSD */
	info.mode = AUMODE_PLAY;
#endif
	info.play.encoding = AUDIO_ENCODING_SLINEAR;
	info.play.precision = format->bits;
	info.play.sample_rate = format->rate;
	info.play.channels = device->output_channels;

 	if (ioctl(internal->fd, AUDIO_SETINFO, &info) < 0) {
		close(internal->fd);
		return 0;  /* Unsupported audio format */
	}

	device->driver_byte_format = AO_FMT_NATIVE;

        if(!device->inter_matrix){
          /* set up out matrix such that users are warned about > stereo playback */
          if(device->output_channels<=2)
            device->inter_matrix=strdup("L,R");
          //else no matrix, which results in a warning
        }

	return 1;
}


int ao_plugin_play(ao_device *device, const char *output_samples,
		uint_32 num_bytes)
{
	ao_sun_internal *internal = (ao_sun_internal *) device->internal;

	if (write(internal->fd, output_samples, num_bytes) < 0)
		return 0;
	else
		return 1;
}


int ao_plugin_close(ao_device *device)
{
	ao_sun_internal *internal = (ao_sun_internal *) device->internal;

        if(internal->fd>=0)
          close(internal->fd);
        internal->fd=-1;

	return 1;
}


void ao_plugin_device_clear(ao_device *device)
{
	ao_sun_internal *internal = (ao_sun_internal *) device->internal;

        if(internal->dev)
          free(internal->dev);
	free(internal);
        device->internal=NULL;
}
