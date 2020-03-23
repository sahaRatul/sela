/*
 *
 *  ao_oss.c
 *
 *      Original Copyright (C) Aaron Holtzman - May 1999
 *      Modifications Copyright (C) Stan Seibert - July 2000, June 2001
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
#if defined(__OpenBSD__) || defined(__NetBSD__)
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif
#include <sys/ioctl.h>

#include "ao/ao.h"
#include "ao/plugin.h"

/* default 20 millisecond buffer */
#define AO_OSS_BUFFER_TIME 20000

static char *ao_oss_options[] = {"dsp","dev","id","buffer_time","verbose","quiet","matrix","debug"};
static ao_info ao_oss_info =
{
	AO_TYPE_LIVE,
	"OSS audio driver output ",
	"oss",
	"Aaron Holtzman <aholtzma@ess.engr.uvic.ca>",
	"Outputs audio to the Open Sound System driver.",
	AO_FMT_NATIVE,
	20,
	ao_oss_options,
        sizeof(ao_oss_options)/sizeof(*ao_oss_options)
};


typedef struct ao_oss_internal {
	char *dev;
        int id;
	int fd;
	int buf_size;
	unsigned int buffer_time;
} ao_oss_internal;


/*
 * open either the devfs device or the traditional device and return a
 * file handle.  Also strdup() path to the selected device into
 * *dev_path.  Assumes that *dev_path does not need to be free()'ed
 * initially.
 *
 * If BROKEN_OSS is defined, this opens the device in non-blocking
 * mode at first in order to prevent deadlock caused by ALSA's OSS
 * emulation and some OSS drivers if the device is already in use.  If
 * blocking is non-zero, we remove the blocking flag if possible so
 * that the device can be used for actual output.
 */
int _open_default_oss_device (char **dev_path, int id, int blocking)
{
	int fd;
        char buf[80];

	/* default: first try the devfs path */
        if(id>0){
          sprintf(buf,"/dev/sound/dsp%d",id);
          if(!(*dev_path = strdup(buf)))
            return -1;
        }else{
          if(!(*dev_path = strdup("/dev/sound/dsp")))
            return -1;
        }

#ifdef BROKEN_OSS
	fd = open(*dev_path, O_WRONLY | O_NONBLOCK);
#else
	fd = open(*dev_path, O_WRONLY);
#endif /* BROKEN_OSS */

	/* then try the original dsp path */
	if(fd < 0)
	{
		/* no? then try the traditional path */
		free(*dev_path);
                if(id>0){
                  sprintf(buf,"/dev/dsp%d",id);
                  if(!(*dev_path = strdup(buf)))
                    return -1;
                }else{
                  if(!(*dev_path = strdup("/dev/dsp")))
                    return -1;
                }
#ifdef BROKEN_OSS
		fd = open(*dev_path, O_WRONLY | O_NONBLOCK);
#else
		fd = open(*dev_path, O_WRONLY);
#endif /* BROKEN_OSS */
	}

#ifdef BROKEN_OSS
	/* Now have to remove the O_NONBLOCK flag if so instructed. */
	if (fd >= 0 && blocking) {
		if (fcntl(fd, F_SETFL, 0) < 0) { /* Remove O_NONBLOCK */
			/* If we can't go to blocking mode, we can't use
			   this device */
			close(fd);
			fd = -1;
		}
	}
#endif /* BROKEN_OSS */

	/* Deal with error cases */
	if(fd < 0)
	{
                /* could not open either default device */
		free(*dev_path);
		*dev_path = NULL;
	}

	return fd;
}


int ao_plugin_test()
{
	char *dev_path;
	int fd;

	/* OSS emulation in ALSA will by default cause the open() call
	   to block if the dsp is in use.  This will freeze the default
	   driver detection unless the O_NONBLOCK flag is passed to
	   open().  We cannot use this flag when we actually open the
	   device for writing because then we will overflow the buffer. */
	if ( (fd = _open_default_oss_device(&dev_path, 0, 0)) < 0 )
		return 0; /* Cannot use this plugin with default parameters */
	else {
		free(dev_path);
		close(fd);
		return 1; /* This plugin works in default mode */
	}
}


ao_info *ao_plugin_driver_info(void)
{
	return &ao_oss_info;
}


int ao_plugin_device_init(ao_device *device)
{
	ao_oss_internal *internal;

	internal = (ao_oss_internal *) calloc(1,sizeof(ao_oss_internal));

	if (internal == NULL)
		return 0; /* Could not initialize device memory */

	internal->dev = NULL;
        internal->id = -1;
        internal->buffer_time = AO_OSS_BUFFER_TIME;

	device->internal = internal;
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

	return 1; /* Memory alloc successful */
}

int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
{
	ao_oss_internal *internal = (ao_oss_internal *) device->internal;


	if (!strcmp(key, "dsp") || !strcmp(key, "dev")) {
          /* Free old string in case "dsp" set twice in options */
          free(internal->dev);
          if(!(internal->dev = strdup(value)))
            return 0;
	}
	if (!strcmp(key, "id")) {
          free(internal->dev);
          internal->dev=NULL;
          internal->id=atoi(value);
	}
	if (!strcmp(key, "buffer_time"))
          internal->buffer_time = atoi(value) * 1000;

	return 1;
}

static int ilog(long x){
  int ret=-1;

  while(x>0){
    x>>=1;
    ret++;
  }
  return ret;
}

/*
 * open the audio device for writing to
 */
int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
	ao_oss_internal *internal = (ao_oss_internal *) device->internal;
	int tmp;

	/* Open the device driver */

	if (internal->dev != NULL) {
          /* open the user-specified path */
          internal->fd = open(internal->dev, O_WRONLY);

          if(internal->fd < 0) {
            aerror("open(%s) => %s\n",internal->dev,strerror(errno));
            return 0;  /* Cannot open device */
          }

        }else{
          internal->fd = _open_default_oss_device(&internal->dev, internal->id, 1);
          if (internal->fd < 0){
            aerror("open default => %s\n",strerror(errno));
            return 0;  /* Cannot open default device */
          }
        }

        /* try to lower the DSP delay; this ioctl may fail gracefully */
        {

          long bytesperframe=(format->bits+7)/8*device->output_channels*format->rate*(internal->buffer_time/1000000.);
          int fraglog=ilog(bytesperframe);
          int fragment=0x00040000|fraglog,fragcheck;

          fragcheck=fragment;
          int ret=ioctl(internal->fd,SNDCTL_DSP_SETFRAGMENT,&fragment);
          if(ret || fragcheck!=fragment){
            fprintf(stderr,"Could not set DSP fragment size; continuing.\n");
          }
        }

	/* Now set all of the parameters */

#ifdef SNDCTL_DSP_CHANNELS
        /* OSS versions > 2 */
        tmp = device->output_channels;
        if (ioctl(internal->fd,SNDCTL_DSP_CHANNELS,&tmp) < 0 ||
            tmp != device->output_channels) {
          aerror("cannot set channels to %d\n", device->output_channels);
          goto ERR;
        }
#else
        /* OSS versions < 4 */
	switch (device->output_channels)
	{
	case 1: tmp = 0;
		break;
	case 2: tmp = 1;
		break;
	default:aerror("Unsupported number of channels: %d.\n",
                       device->output_channels);
		goto ERR;
	}

	if (ioctl(internal->fd,SNDCTL_DSP_STEREO,&tmp) < 0 ||
			tmp+1 != device->output_channels) {
          aerror("cannot set channels to %d\n",
                 device->output_channels);
          goto ERR;
	}
#endif

	/* To eliminate the need for a swap buffer, we set the device
	   to use whatever byte format the client selected. */
	switch (format->bits)
	{
	case 8: tmp = AFMT_S8;
		break;
        case 16: tmp = device->client_byte_format == AO_FMT_BIG ?
		   AFMT_S16_BE : AFMT_S16_LE;
	        device->driver_byte_format = device->client_byte_format;
	        break;
	default:aerror("Unsupported number of bits: %d.",
			format->bits);
		goto ERR;
	}

	if (ioctl(internal->fd,SNDCTL_DSP_SAMPLESIZE,&tmp) < 0) {
		aerror("cannot set sample size to %d\n",
			format->bits);
		goto ERR;
	}

	tmp = format->rate;
	/* Some cards aren't too accurate with their clocks and set to the
	   exact data rate, but something close.  Fail only if completely out
	   of whack. */
	if (ioctl(internal->fd,SNDCTL_DSP_SPEED, &tmp) < 0
	    || tmp > 1.02 * format->rate || tmp < 0.98 * format->rate) {
		aerror("cannot set rate to %d\n",
			format->rate);
		goto ERR;
	}

	/* this calculates and sets the fragment size */
	internal->buf_size = -1;
	if ((ioctl(internal->fd,SNDCTL_DSP_GETBLKSIZE,
				&(internal->buf_size)) < 0) ||
			internal->buf_size<=0 )
	{
          /* Some versions of the *BSD OSS drivers use a subtly
             different SNDCTL_DSP_GETBLKSIZE ioctl implementation
             which is, oddly, incompatible with the shipped
             declaration in soundcard.h.  This ioctl isn't necessary
             anyway, it's just tuning.  Soldier on without, */

          adebug("cannot get buffer size for device; using a default of 1024kB\n");
          internal->buf_size=1024;
	}

        if(!device->inter_matrix){
          /* set up matrix such that users are warned about > stereo playback */
          if(device->output_channels<=2)
            device->inter_matrix=strdup("L,R");
          //else no matrix, which results in a warning
        }

	return 1; /* Open successful */

 ERR:
	close(internal->fd);
	return 0; /* Failed to open device */
}

/*
 * play the sample to the already opened file descriptor
 */
int ao_plugin_play(ao_device *device, const char *output_samples,
		uint_32 num_bytes)
{
	int ret;
	int send;
	ao_oss_internal *internal = (ao_oss_internal *) device->internal;

	while(num_bytes > 0) {
          send = num_bytes>internal->buf_size?
            internal->buf_size:num_bytes;
          ret = write(internal->fd, output_samples, send);

          if (ret < 0){
            if(errno == EINTR) continue;
            return 0;
          }
          num_bytes-=ret;
          output_samples+=ret;
	}

	return 1;
}


int ao_plugin_close(ao_device *device)
{
	ao_oss_internal *internal = (ao_oss_internal *) device->internal;

        if(internal->fd>=0)
          close(internal->fd);
        internal->fd=-1;

	return 1;
}


void ao_plugin_device_clear(ao_device *device)
{
	ao_oss_internal *internal = (ao_oss_internal *) device->internal;

        if(internal->dev)
          free(internal->dev);
	free(internal);
        device->internal=NULL;
}
