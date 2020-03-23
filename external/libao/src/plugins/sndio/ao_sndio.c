/*
 * Copyright (c) 2008 Alexandre Ratchov <alex at caoua.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>
#include <sndio.h>
#include <ao/ao.h>
#include <ao/plugin.h>

static char *ao_sndio_options[] = {
  "verbose",
  "quiet",
  "matrix",
  "debug",
  "dev",
  "id"
};

ao_info ao_sndio_info = {
  AO_TYPE_LIVE,
  "sndio audio output",
  "sndio",
  "Alexandre Ratchov <alex at caoua.org>",
  "Outputs to the sndio library",
  AO_FMT_NATIVE,
  30,
  ao_sndio_options,
  sizeof(ao_sndio_options)/sizeof(*ao_sndio_options)
};

typedef struct ao_sndio_internal
{
  struct sio_hdl *hdl;
  char *dev;
  int id;
} ao_sndio_internal;

int ao_plugin_test()
{
  struct sio_hdl *hdl;

  hdl = sio_open(NULL, SIO_PLAY, 0);
  if (hdl == NULL)
    return 0;
  sio_close(hdl);
  return 1;
}

ao_info *ao_plugin_driver_info(void)
{
  return &ao_sndio_info;
}

int ao_plugin_device_init(ao_device *device)
{
  ao_sndio_internal *internal;
  internal = (ao_sndio_internal *) calloc(1,sizeof(*internal));
  if (internal == NULL)
    return 0;

  internal->id=-1;
  device->internal = internal;
  device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;
  return 1;
}

int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
{
  ao_sndio_internal *internal = (ao_sndio_internal *) device->internal;

  if (!strcmp(key, "dev")) {
    if (internal->dev)
      free (internal->dev);
    if(!value){
      internal->dev=NULL;
    }else{
      if (!(internal->dev = strdup(value)))
        return 0;
    }
  }
  if (!strcmp(key, "id")) {
    if(internal->dev)
      free (internal->dev);
    internal->dev=NULL;
    internal->id=atoi(value);
  }
  return 1;
}

int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
  ao_sndio_internal *internal = (ao_sndio_internal *) device->internal;
  struct sio_par par;

  if(!internal->dev && internal->id>=0){
    char buf[80];
    sprintf(buf,"sun:%d",internal->id);
    internal->dev = strdup(buf);
  }

  internal->hdl = sio_open(internal->dev, SIO_PLAY, 0);
  if (internal->hdl == NULL)
    return 0;

  sio_initpar(&par);
  par.sig = 1;
  par.le = SIO_LE_NATIVE;
  par.bits = format->bits;
  par.rate = format->rate;
  par.pchan = device->output_channels;
  if (!sio_setpar(internal->hdl, &par))
    return 0;
  device->driver_byte_format = AO_FMT_NATIVE;
  if (!sio_start(internal->hdl))
    return 0;

  if(!device->inter_matrix){
    /* set up matrix such that users are warned about > stereo playback */
    if(device->output_channels<=2)
      device->inter_matrix=strdup("L,R");
    //else no matrix, which results in a warning
  }

  return 1;
}

int ao_plugin_play(ao_device *device, const char *output_samples, uint_32 num_bytes)
{
  ao_sndio_internal *internal = (ao_sndio_internal *) device->internal;
  struct sio_hdl *hdl = internal->hdl;

  if (!sio_write(hdl, output_samples, num_bytes))
    return 0;
  return 1;
}

int ao_plugin_close(ao_device *device)
{
  ao_sndio_internal *internal = (ao_sndio_internal *) device->internal;
  struct sio_hdl *hdl = internal->hdl;

  if(hdl)
    if (!sio_stop(hdl))
      return 0;
  return 1;
}

void ao_plugin_device_clear(ao_device *device)
{
  ao_sndio_internal *internal = (ao_sndio_internal *) device->internal;
  struct sio_hdl *hdl = internal->hdl;

  if(hdl)
    sio_close(hdl);
  if(internal->dev)
    free(internal->dev);
  free(internal);
  device->internal=NULL;
}
