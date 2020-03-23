/*
 *
 *  ao_nas.c - Network Audio System output plugin for libao
 *
 *  Copyright (C) 2003 - Antoine Mathys <Antoine.Mathys@unifr.ch>
 *
 *  Based on the XMMS NAS plugin by Willem Monsuwe.
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
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <audio/audiolib.h>
#include <ao/ao.h>
#include <ao/plugin.h>

/*
* Buffer size must be greater than 2.
*/

#define AO_NAS_BUF_SIZE 4096

static char *ao_nas_options[] = {
  "server",    /* NAS server. See nas(1) for format. */
  "host",    /* synonym for server */
  "buf_size", /* Buffer size on server */
  "quiet",
  "verbose",
  "matrix",
  "debug"
};

static ao_info ao_nas_info =
{
	AO_TYPE_LIVE,
	"NAS output",
	"nas",
	"Antoine Mathys <Antoine.Mathys@unifr.ch>",
	"Outputs to the Network Audio System.",
	AO_FMT_NATIVE,
	10,
	ao_nas_options,
        sizeof(ao_nas_options)/sizeof(*ao_nas_options)
};


typedef struct ao_nas_internal
{
  AuServer* aud;
  AuFlowID flow;
  AuDeviceID dev;
  char *host;
  AuUint32 buf_size;
  AuUint32 buf_free;
} ao_nas_internal;

int ao_plugin_test()
{
  AuServer* aud = 0;

  aud = AuOpenServer(0, 0, 0, 0, 0, 0);
  if (!aud)
    return 0;
  else {
    AuCloseServer(aud);
    return 1;
  }
}


ao_info *ao_plugin_driver_info(void)
{
	return &ao_nas_info;
}


int ao_plugin_device_init(ao_device *device)
{
	ao_nas_internal *internal;

	internal = (ao_nas_internal *) malloc(sizeof(ao_nas_internal));

	if (internal == NULL)
		return 0; /* Could not initialize device memory */

	internal->host = NULL;
	internal->buf_size = AO_NAS_BUF_SIZE;
	internal->buf_free = 0;

	device->internal = internal;
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

	return 1; /* Memory alloc successful */
}

int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
{
	ao_nas_internal *internal = (ao_nas_internal *) device->internal;

	if (!strcmp(key, "host") || !strcmp(key, "server")) {
          char *tmp = strdup (value);
 	  if (!tmp) return 0;
 	  if (internal->host) free (internal->host);
 	  internal->host = tmp;
	}
	else if (!strcmp(key, "buf_size")) {
          int tmp = atoi (value);
 	  if (tmp <= 2) return 0;
 	  internal->buf_size = tmp;
	}

	return 1;
}


int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
	ao_nas_internal *internal = (ao_nas_internal *) device->internal;
	unsigned char nas_format;
	AuElement elms[2];

	/* get format */
	switch (format->bits)
	{
	case 8  :
	  nas_format = AuFormatLinearUnsigned8;
	  break;
	case 16 :
	  if (device->machine_byte_format == AO_FMT_BIG)
	    nas_format = AuFormatLinearSigned16MSB;
	  else
	    nas_format = AuFormatLinearSigned16LSB;
	  break;
	default : return 0;
	}

	/* open server */
	internal->aud = AuOpenServer(internal->host, 0, 0, 0, 0, 0);
	if (!internal->aud)
	  return 0; /* Could not contact NAS server */

	/* find physical output device */
	{
	  int i;
	  for (i = 0; i < AuServerNumDevices(internal->aud); i++)
	    if ((AuDeviceKind(AuServerDevice(internal->aud, i)) ==
		 AuComponentKindPhysicalOutput) &&
		(AuDeviceNumTracks(AuServerDevice(internal->aud, i)) ==
		 device->output_channels))
	      break;

	  if ((i == AuServerNumDevices(internal->aud)) ||
	      (!(internal->flow = AuCreateFlow(internal->aud, 0)))) {
	    /* No physical output device found or flow creation failed. */
	    AuCloseServer(internal->aud);
	    return 0;
	  }
	  internal->dev = AuDeviceIdentifier(AuServerDevice(internal->aud, i));
	}

	/* set up flow */
	AuMakeElementImportClient(&elms[0], format->rate,
				  nas_format, device->output_channels, AuTrue,
				  internal->buf_size, internal->buf_size / 2,
				  0, 0);
	AuMakeElementExportDevice(&elms[1], 0, internal->dev,
				  format->rate, AuUnlimitedSamples, 0, 0);
	AuSetElements(internal->aud, internal->flow, AuTrue, 2, elms, 0);
	AuStartFlow(internal->aud, internal->flow, 0);

	device->driver_byte_format = AO_FMT_NATIVE;

        if(!device->inter_matrix){
          /* set up out matrix such that users are warned about > stereo playback */
          if(device->output_channels<=2)
            device->inter_matrix=strdup("L,R");
          //else no matrix, which results in a warning
        }

	return 1;
}

int ao_plugin_play(ao_device *device, const char* output_samples,
		uint_32 num_bytes)
{
	ao_nas_internal *internal = (ao_nas_internal *) device->internal;

	while (num_bytes > 0) {
	  /* Wait for room in buffer */
	  while (internal->buf_free == 0) {
	    AuEvent ev;
	    AuNextEvent(internal->aud, AuTrue, &ev);
	    if (ev.type == AuEventTypeElementNotify) {
	      AuElementNotifyEvent* event = (AuElementNotifyEvent*)(&ev);
	      if (event->kind == AuElementNotifyKindLowWater)
		internal->buf_free = event->num_bytes;
	      else if ((event->kind == AuElementNotifyKindState) &&
		       (event->cur_state == AuStatePause) &&
		       (event->reason != AuReasonUser))
		internal->buf_free = event->num_bytes;
	    }
	  }

	  /* Partial transfer */
	  if (num_bytes > internal->buf_free) {
	    AuWriteElement(internal->aud, internal->flow, 0, internal->buf_free,
			   (AuPointer)output_samples, AuFalse, 0);
	    num_bytes -= internal->buf_free;
	    output_samples += internal->buf_free;
	    internal->buf_free = 0;
	  }

	  /* Final transfer */
	  else {
	    AuWriteElement(internal->aud, internal->flow, 0, num_bytes,
			   (AuPointer)output_samples, AuFalse, 0);
	    internal->buf_free -= num_bytes;
	    break;
	  }
	}

	return 1;
}


int ao_plugin_close(ao_device *device)
{
	ao_nas_internal *internal = (ao_nas_internal *) device->internal;

        if(internal->aud){
          AuStopFlow(internal->aud, internal->flow, 0);
          AuCloseServer(internal->aud);
        }
	return 1;
}


void ao_plugin_device_clear(ao_device *device)
{
	ao_nas_internal *internal = (ao_nas_internal *) device->internal;

        if(internal->host)
          free(internal->host);
	free(internal);
        device->internal=NULL;
}
