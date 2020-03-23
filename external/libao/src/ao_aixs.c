/*
 *
 *  ao_aixs.c    AIX (5.1)
 *
 *      Original Copyright (C) Stefan Tibus - August 2002
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

#ifdef HAVE_SYS_AUDIO_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/audio.h>

#include <ao/ao.h>
#include <ao/plugin.h>


/*
 * default audio device to be used,
 * possible options:
 * /dev/paud0/1 on PCI machines with the Crystal chipset
 * /dev/baud0/1 on MCA machines with the Crystal chipset
 * /dev/acpa0/1 on MCA machines with the ACPA
 */
#ifndef AO_AIX_DEFAULT_DEV
#define AO_AIX_DEFAULT_DEV "/dev/baud0/1"
#define AO_AIX_DEFAULT_DEV2 "/dev/paud0/1"
#define AO_AIX_DEFAULT_DEV3 "/dev/acpa0/1"
#endif


static char *ao_aixs_options[] = {"dev","id","matrix","verbose","quiet","debug"};
ao_info ao_aixs_info = {
  AO_TYPE_LIVE,
  "AIX audio driver output",
  "aixs",
  "Stefan Tibus <sjti@gmx.net>",
  "Outputs to the AIX audio system.",
  AO_FMT_NATIVE,
  20,
  ao_aixs_options,
  sizeof(ao_aixs_options)/sizeof(*ao_aixs_options)
};


typedef struct ao_aixs_internal {
  char *dev;
  int id;
  int fd;
} ao_aixs_internal;


int ao_aixs_test()
{
  int fd;

  fd = open(AO_AIX_DEFAULT_DEV, O_WRONLY);
  if(fd<0)
    fd = open(AO_AIX_DEFAULT_DEV2, O_WRONLY);
  if(fd<0)
    fd = open(AO_AIX_DEFAULT_DEV3, O_WRONLY);
  if(fd<0)
    return 0; /* Cannot use this plugin with default parameters */

  close(fd);
  return 1; /* This plugin works in default mode */
}


ao_info *ao_aixs_driver_info(void)
{
  return &ao_aixs_info;
}


int ao_aixs_device_init(ao_device *device)
{
  ao_aixs_internal *internal;

  internal = (ao_aixs_internal *) calloc(1,sizeof(ao_aixs_internal));

  if (internal == NULL)
    return 0; /* Could not initialize device memory */

  device->internal = internal;
  device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

  return 1; /* Memory alloc successful */
}

int ao_aixs_set_option(ao_device *device, const char *key, const char *value)
{
  ao_aixs_internal *internal = (ao_aixs_internal *) device->internal;

  if (!strcmp(key, "dev")) {
    if(internal->dev)
      free(internal->dev);
    internal->dev = strdup(value);
  }
  if (!strcmp(key, "id")) {
    internal->id = atoi(value);
    if(internal->dev)
      free(internal->dev);
    internal->dev = NULL;
  }

  return 1;
}


int ao_aixs_open(ao_device *device, ao_sample_format *format)
{
	ao_aixs_internal *internal = (ao_aixs_internal *) device->internal;

	audio_init init;
	audio_control control;
	audio_change change;

        if(!internal->dev){
          char buffer[80];
          int fd;

          sprintf(buffer,"/dev/baud%d/1",id);
          fd = open(buffer, O_WRONLY);
          if(fd<0){
            sprintf(buffer,"/dev/paud%d/1",id);
            fd = open(buffer, O_WRONLY);
          }
          if(fd<0){
            sprintf(buffer,"/dev/acpa%d/1",id);
            fd = open(buffer, O_WRONLY);
          }
          if(fd<0) return 0;
          internal->fd = fd;
          internal->dev = strdup(buffer);
        }else
          if ( (internal->fd = open(internal->dev, O_WRONLY)) < 0 )
            return 0;

	init.srate = format->rate;
	init.bits_per_sample = format->bits;
	init.channels = device->output_channels;
	init.mode = AUDIO_PCM;
	init.flags = AUDIO_BIG_ENDIAN | AUDIO_TWOS_COMPLEMENT;
	init.operation = AUDIO_PLAY;

 	if (ioctl(internal->fd, AUDIO_INIT, &init) < 0) {
		close(internal->fd);
		return 0;  /* Unsupported audio format */
	}

	change.balance = 0x3fff0000;
	change.volume = 0x3fff0000;
	change.monitor = AUDIO_IGNORE;
	change.input = AUDIO_IGNORE;
	change.output = AUDIO_OUTPUT_1;

	control.ioctl_request = AUDIO_CHANGE;
	control.position = 0;
	control.request_info = &change;

	if (ioctl(internal->fd, AUDIO_CONTROL, &control) < 0) {
		close(internal->fd);
		return 0;
	}

	control.ioctl_request = AUDIO_START;
	control.request_info = NULL;

	if (ioctl(internal->fd, AUDIO_CONTROL, &control) < 0) {
		close(internal->fd);
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


int ao_aixs_play(ao_device *device, const char *output_samples,
		uint_32 num_bytes)
{
	ao_aixs_internal *internal = (ao_aixs_internal *) device->internal;

	if (write(internal->fd, output_samples, num_bytes) < 0)
		return 0;
	else
		return 1;
}


int ao_aixs_close(ao_device *device)
{
	ao_aixs_internal *internal = (ao_aixs_internal *) device->internal;

        if(internal->fd)
          close(internal->fd);
        internal->fd=-1;
	return 1;
}


void ao_aixs_device_clear(ao_device *device)
{
	ao_aixs_internal *internal = (ao_aixs_internal *) device->internal;

        if(internal->dev)
          free(internal->dev);
	free(internal);
        device->internal = NULL;
}

ao_functions ao_aixs = {
	ao_aixs_test,
	ao_aixs_driver_info,
	ao_aixs_device_init,
	ao_aixs_set_option,
	ao_aixs_open,
	ao_aixs_play,
	ao_aixs_close,
	ao_aixs_device_clear
};

#endif
