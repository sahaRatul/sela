/*
 *
 *  ao_irix.h
 *
 *      Original Copyright (C) Aaron Holtzman - May 1999
 *      Port to IRIX by Jim Miller, SGI - Nov 1999
 *      Modifications Copyright (C) Stan Seibert - July 2000, July 2001
 *
 *  This file is part of libao, a cross-platform library.  See
 *  README for a history of this source code.
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include <audio.h>

#include <ao/ao.h>

#define AO_IRIX_BUFFER_SIZE 32768

typedef struct ao_irix_internal {
	ALport alport;
	ALconfig alconfig;
	int bytesPerSample;
	int channels;
} ao_irix_internal;

static char *ao_irix_options[] = {"matrix","verbose","quiet","debug"};

static ao_info ao_irix_info =
{
	AO_TYPE_LIVE,
	"Irix audio output ",
	"irix",
	"Jim Miller <???@sgi.com>",
	"Outputs to the IRIX Audio Library.",
	AO_FMT_NATIVE,
	20,
	ao_irix_options,
        sizeof(ao_irix_options)/sizeof(*ao_irix_options)
};

int ao_plugin_test(void)
{
	char *dev_path;
	ALport port;


	if ((port = alOpenPort("libao test", "w", NULL)) == NULL)
		return 0; /* Cannot use this plugin with default parameters */
	else {
		alClosePort(port);
		return 1; /* This plugin works in default mode */
	}
}

ao_info *ao_plugin_driver_info(void)
{
	return &ao_irix_info;
}


int ao_plugin_device_init(ao_device *device)
{
	ao_irix_internal *internal;

	internal = (ao_irix_internal *) malloc(sizeof(ao_irix_internal));

	if (internal == NULL)
		/* Could not allocate memory for device-specific data. */
		return 0;

	internal->alconfig = alNewConfig();
	internal->alport = NULL;
	internal->bytesPerSample = 2;
	internal->channels = 2;

	device->internal = internal;
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

	/* Device-specific initialization was successful. */
	return 1;
}

int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
{
	return 1; /* No options */
}

/* Open the audio device for writing. */
int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
	ao_irix_internal *internal = (ao_irix_internal *) device->internal;
	ALpv params;
	int  dev = AL_DEFAULT_OUTPUT;
	int  wsize = AL_SAMPLE_16;

	if (alSetQueueSize(internal->alconfig, AO_IRIX_BUFFER_SIZE) < 0) {
		aerror("alSetQueueSize failed: %s\n",
			alGetErrorString(oserror()));
		return 0;
	}

	if (alSetChannels(internal->alconfig, device->output_channels) < 0) {
		aerror("alSetChannels(%d) failed: %s\n",
			device->output_channels, alGetErrorString(oserror()));
		return 0;
	}

	internal->channels = device->output_channels;

	if (alSetDevice(internal->alconfig, dev) < 0) {
		aerror("alSetDevice failed: %s\n",
			alGetErrorString(oserror()));
		return 0;
	}

	if (alSetSampFmt(internal->alconfig, AL_SAMPFMT_TWOSCOMP) < 0) {
		aerror("alSetSampFmt failed: %s\n",
			alGetErrorString(oserror()));
		return 0;
	}

	switch (format->bits) {
	case 8:
		internal->bytesPerSample = 1;
		wsize = AL_SAMPLE_8;
		break;

	case 16:
		internal->bytesPerSample = 2;
		wsize = AL_SAMPLE_16;
		break;

	case 24:
		internal->bytesPerSample = 4;
		wsize = AL_SAMPLE_24;
		break;

	default:
		aerror("unsupported bit with %d\n",
			format->bits);
		break;
	}

	if (alSetWidth(internal->alconfig, wsize) < 0) {
		aerror("alSetWidth failed: %s\n",
			alGetErrorString(oserror()));
		alClosePort(internal->alport);
		return 0;
	}

	internal->alport = alOpenPort("libao", "w", internal->alconfig);
	if (internal->alport == NULL)
	{
		aerror("alOpenPort failed: %s\n",
			alGetErrorString(oserror()));
		return 0;
	}

	params.param = AL_RATE;
	params.value.ll = alDoubleToFixed((double) format->rate);
	if (alSetParams(dev, &params, 1) < 0)
	{
		aerror("alSetParams() failed: %s\n", alGetErrorString(oserror()));
		alClosePort(internal->alport);
		return 0;
	}

	device->driver_byte_format = AO_FMT_NATIVE;
        if(!device->inter_matrix){
          /* set up matrix such that users are warned about > stereo playback */
          if(device->output_channels<=2)
            device->inter_matrix=strdup("L,R");
          //else no matrix, which results in a warning
        }

	return 1;
}

/* Play the sampled audio data to the already opened device. */
int ao_plugin_play(ao_device *device, const char *output_samples,
		uint_32 num_bytes)
{
	uint_32 num_frames;
	ao_irix_internal *internal = (ao_irix_internal *) device->internal;

	num_frames = num_bytes;
	num_frames /= internal->bytesPerSample;
	num_frames /= internal->channels;

	alWriteFrames(internal->alport, output_samples, num_frames);

	return 1; /* FIXME: Need to check if the above function failed */
}

int ao_plugin_close(ao_device *device)
{
	ao_irix_internal *internal = (ao_irix_internal *) device->internal;

        if(internal->alport)
          alClosePort(internal->alport);
        internal->alport=NULL;
	return 1;
}

void ao_plugin_device_clear(ao_device *device)
{
	ao_irix_internal *internal = (ao_irix_internal *) device->internal;

        if(internal->alconfig)
          alFreeConfig(internal->alconfig);
	free(internal);
        device->internal=NULL;
}
