/*
 *
 *  ao_alsa.c
 *
 *      Copyright (C) Stan Seibert - July 2000, July 2001
 *      Modifications Copyright (C) Monty - January 2010
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
 *  Largely rewritten 2/18/2002 Kevin Cody Jr <kevinc@wuff.dhs.org>
 *
 */

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include <alsa/asoundlib.h>
#include <ao/ao.h>
#include <ao/plugin.h>

/* default 20 millisecond buffer */
#define AO_ALSA_BUFFER_TIME 20000

/* default we be calculated to be 1/4 of the buffer time */
#define AO_ALSA_PERIOD_TIME 0

/* set mmap to default if enabled at compile time, otherwise, mmap isn't
   the default */
#ifdef USE_ALSA_MMIO
#define AO_ALSA_WRITEI snd_pcm_mmap_writei
#define AO_ALSA_ACCESS_MASK SND_PCM_ACCESS_MMAP_INTERLEAVED
#else
#define AO_ALSA_WRITEI snd_pcm_writei
#define AO_ALSA_ACCESS_MASK SND_PCM_ACCESS_RW_INTERLEAVED
#endif

typedef snd_pcm_sframes_t ao_alsa_writei_t(snd_pcm_t *pcm, const void *buffer,
						snd_pcm_uframes_t size);

static char *ao_alsa_options[] = {
	"dev",
	"id",
	"buffer_time",
        "period_time",
	"use_mmap",
        "matrix",
        "verbose",
        "quiet",
        "debug"
};


static ao_info ao_alsa_info =
{
	AO_TYPE_LIVE,
	"Advanced Linux Sound Architecture (ALSA) output",
	"alsa",
	"Bill Currie <bill@taniwha.org>/Kevin Cody, Jr. <kevinc@wuff.dhs.org>",
	"Outputs to the Advanced Linux Sound Architecture version 0.9/1.x",
	AO_FMT_NATIVE,
	35,
	ao_alsa_options,
        sizeof(ao_alsa_options)/sizeof(*ao_alsa_options)
};


typedef struct ao_alsa_internal
{
        snd_pcm_t *pcm_handle;
        unsigned int buffer_time;
        unsigned int period_time;
        snd_pcm_uframes_t period_size;
        int sample_size;
        unsigned int sample_rate;
        snd_pcm_format_t bitformat;
        char *padbuffer;
        int   padoutw;
        char *dev;
        int id;
        ao_alsa_writei_t * writei;
        snd_pcm_access_t access_mask;
        int static_delay;

        /* Local configuration with handle_underrun workaround set for
           PulseAudio ALSA plugin. Will be NULL if the PA ALSA plugin is not
           in use or the workaround is not required. Remove this and the
           associated code once PulseAudio gets fixed. */
        snd_config_t *local_config;
} ao_alsa_internal;


/* determine if parameters are requires for this particular plugin */
int ao_plugin_test()
{
	snd_pcm_t *handle;
	int err;

	/* Use nonblock flag when testing to avoid getting stuck if the device
	   is in use. Try several devices, as 'default' usually means 'stereo only'. */
	err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK,
			   SND_PCM_NONBLOCK);
	if (err != 0)
		return 0; /* Cannot use this plugin with default parameters */
	else {
		snd_pcm_close(handle);
		return 1; /* This plugin works in default mode */
	}
}


/* return the address of the driver info structure */
ao_info *ao_plugin_driver_info(void)
{
	return &ao_alsa_info;
}


/* initialize internal data structures */
int ao_plugin_device_init(ao_device *device)
{
	ao_alsa_internal *internal;

	internal = (ao_alsa_internal *) calloc(1,sizeof(ao_alsa_internal));

	if (internal == NULL)
		return 0;

	internal->buffer_time = AO_ALSA_BUFFER_TIME;
	internal->period_time = AO_ALSA_PERIOD_TIME;
	internal->writei = AO_ALSA_WRITEI;
	internal->access_mask = AO_ALSA_ACCESS_MASK;
        internal->id=-1;

	device->internal = internal;
        device->output_matrix = strdup("L,R,BL,BR,C,LFE,SL,SR");
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

	return 1;
}


/* pass application parameters regarding the sound device */
int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
{
	ao_alsa_internal *internal = (ao_alsa_internal *) device->internal;

	if (!strcmp(key, "dev")) {
		if (internal->dev)
			free (internal->dev);
		if (!(internal->dev = strdup(value)))
			return 0;
	}
	else if (!strcmp(key, "id")){
                internal->id = atoi(value);
                if (internal->dev)
                  free (internal->dev);
                internal->dev = NULL;
	}else if (!strcmp(key, "buffer_time"))
		internal->buffer_time = atoi(value) * 1000;
	else if (!strcmp(key, "period_time"))
		internal->period_time = atoi(value);
	else if (!strcmp(key,"use_mmap")) {
		if(!strcmp(value,"yes") || !strcmp(value,"y") || 
			!strcmp(value,"true") || !strcmp(value,"t") ||
			!strcmp(value,"1"))
		{
			internal->writei = snd_pcm_mmap_writei;
			internal->access_mask = SND_PCM_ACCESS_MMAP_INTERLEAVED;
		}
		else {
			internal->writei = snd_pcm_writei;
			internal->access_mask = SND_PCM_ACCESS_RW_INTERLEAVED;
		}
	}

	return 1;
}


/* determine the alsa bitformat for a given bitwidth and endianness */
static inline int alsa_get_sample_bitformat(int bitwidth, int bigendian, ao_device *device)
{
	int ret;

	switch (bitwidth) {
	case 8  : ret = SND_PCM_FORMAT_U8;
		  break;
	case 16 : ret = SND_PCM_FORMAT_S16;
		  break;
	case 24 : ret = SND_PCM_FORMAT_S24;
		  break;
	case 32 : ret = SND_PCM_FORMAT_S32;
		  break;
	default : aerror("invalid bitwidth %d\n", bitwidth);
		  return -1;
	}

	return ret;
}

/* Work around PulseAudio ALSA plugin bug where the PA server forces a
   higher than requested latency, but the plugin does not update its (and
   ALSA's) internal state to reflect that, leading to an immediate underrun
   situation. Inspired by WINE's make_handle_underrun_config.
   Reference: http://mailman.alsa-project.org/pipermail/alsa-devel/2012-July/053391.html
   From Mozilla https://github.com/kinetiknz/cubeb/blob/1aa0058d0729eb85505df104cd1ac072432c6d24/src/cubeb_alsa.c
*/
static snd_config_t *init_local_config_with_workaround(ao_device *device, char const *name){
  char pcm_node_name[80];
  int r;
  snd_config_t * lconf;
  snd_config_t * device_node;
  snd_config_t * type_node;
  snd_config_t * node;
  char const * type_string;

  lconf = NULL;
  snprintf(pcm_node_name,80,"pcm.%s",name);

  if (snd_config == NULL) snd_config_update();

  r = snd_config_copy(&lconf, snd_config);
  if(r<0){
    return NULL;
  }

  r = snd_config_search(lconf, pcm_node_name, &device_node);
  if (r != 0) {
    snd_config_delete(lconf);
    return NULL;
  }

  /* Fetch the PCM node's type, and bail out if it's not the PulseAudio plugin. */
  r = snd_config_search(device_node, "type", &type_node);
  if (r != 0) {
    snd_config_delete(lconf);
    return NULL;
  }

  r = snd_config_get_string(type_node, &type_string);
  if (r != 0) {
    snd_config_delete(lconf);
    return NULL;
  }

  if (strcmp(type_string, "pulse") != 0) {
    snd_config_delete(lconf);
    return NULL;
  }

  /* Don't clobber an explicit existing handle_underrun value, set it only
     if it doesn't already exist. */
  r = snd_config_search(device_node, "handle_underrun", &node);
  if (r != -ENOENT) {
    snd_config_delete(lconf);
    return NULL;
  }

  r = snd_config_imake_integer(&node, "handle_underrun", 0);
  if (r != 0) {
    snd_config_delete(lconf);
    return NULL;
  }

  r = snd_config_add(device_node, node);
  if (r != 0) {
    snd_config_delete(lconf);
    return NULL;
  }

  adebug("PulseAudio ALSA-emulation detected: disabling underrun detection\n");
  return lconf;
}


/* setup alsa data format and buffer geometry */
static inline int alsa_set_hwparams(ao_device *device,
                                    ao_sample_format *format)
{
	ao_alsa_internal *internal  = (ao_alsa_internal *) device->internal;
	snd_pcm_hw_params_t   *params;
	int err;
	unsigned int rate = internal->sample_rate = format->rate;

	/* allocate the hardware parameter structure */
	snd_pcm_hw_params_alloca(&params);

	/* fetch all possible hardware parameters */
	err = snd_pcm_hw_params_any(internal->pcm_handle, params);
	if (err < 0){
          adebug("snd_pcm_hw_params_any() failed.\n"
                 "        Device exists but no matching hardware?\n");
          return err;
        }

	/* set the access type */
	err = snd_pcm_hw_params_set_access(internal->pcm_handle,
			params, internal->access_mask);
	if (err < 0){
          adebug("snd_pcm_hw_params_set_access() failed.\n");
          return err;
        }

	/* set the sample bitformat */
	err = snd_pcm_hw_params_set_format(internal->pcm_handle,
			params, internal->bitformat);
	if (err < 0){

          /* the device may support a greater bit-depth than the one
             requested. */
          switch(internal->bitformat){
          case SND_PCM_FORMAT_U8:
            if (!snd_pcm_hw_params_set_format(internal->pcm_handle,
                                              params, SND_PCM_FORMAT_S16)){
              adebug("snd_pcm_hw_params_set_format() unable to open %d bit playback.\n",format->bits);
              adebug("snd_pcm_hw_params_set_format() using 16 bit playback instead.\n");
              format->bits = 16;
              internal->bitformat=SND_PCM_FORMAT_S16;
              break;
            }
          case SND_PCM_FORMAT_S16:
            if (!snd_pcm_hw_params_set_format(internal->pcm_handle,
                                              params, SND_PCM_FORMAT_S24)){
              adebug("snd_pcm_hw_params_set_format() unable to open %d bit playback.\n",format->bits);
              adebug("snd_pcm_hw_params_set_format() using 24 bit playback instead.\n");
              format->bits = 24;
              internal->bitformat=SND_PCM_FORMAT_S24;
              break;
            }
          case SND_PCM_FORMAT_S24:
            /* There are packed and unpacked '24 bit' formats. Try the packed. */
            /* Alsa requires explicit endianness assignment, so try both */
            if (!snd_pcm_hw_params_set_format(internal->pcm_handle,
                                              params, SND_PCM_FORMAT_S24_3LE)){
              device->driver_byte_format = AO_FMT_LITTLE;
              internal->bitformat=SND_PCM_FORMAT_S24_3LE;
              break;
            }
            if (!snd_pcm_hw_params_set_format(internal->pcm_handle,
                                              params, SND_PCM_FORMAT_S24_3BE)){
              device->driver_byte_format = AO_FMT_BIG;
              internal->bitformat=SND_PCM_FORMAT_S24_3BE;
              break;
            }
            if (!snd_pcm_hw_params_set_format(internal->pcm_handle,
                                              params, SND_PCM_FORMAT_S32)){
              adebug("snd_pcm_hw_params_set_format() unable to open %d bit playback.\n",format->bits);
              adebug("snd_pcm_hw_params_set_format() using 32 bit playback instead.\n");
              format->bits = 32;
              internal->bitformat=SND_PCM_FORMAT_S32;
              break;
            }
          case SND_PCM_FORMAT_S32:
            adebug("snd_pcm_hw_params_set_format() failed.\n");
            return err;
          }
        }

	/* set the number of channels */
	err = snd_pcm_hw_params_set_channels(internal->pcm_handle,
			params, (unsigned int)device->output_channels);
	if (err < 0){
          adebug("snd_pcm_hw_params_set_channels() failed.\n");
          return err;
        }

	/* set the sample rate */
	err = snd_pcm_hw_params_set_rate_near(internal->pcm_handle,
			params, &rate, 0);
	if (err < 0){
          adebug("snd_pcm_hw_params_set_rate_near() failed.\n");
          return err;
        }
	if (rate > 1.05 * format->rate || rate < 0.95 * format->rate) {
          awarn("sample rate %i not supported "
                "by the hardware, using %u\n", format->rate, rate);
	}

	/* set the time per hardware sample transfer */
        if(internal->period_time==0)
          internal->period_time=internal->buffer_time/4;

	err = snd_pcm_hw_params_set_period_time_near(internal->pcm_handle,
			params, &(internal->period_time), 0);
	if (err < 0){
          adebug("snd_pcm_hw_params_set_period_time_near() failed.\n");
          return err;
        }

	/* set the length of the hardware sample buffer in microseconds */
        /* some plug devices have very high minimum periods; don't
           allow a buffer size small enough that it's ~ guaranteed to
           skip */
        if(internal->buffer_time<internal->period_time*3)
          internal->buffer_time=internal->period_time*3;
	err = snd_pcm_hw_params_set_buffer_time_near(internal->pcm_handle,
			params, &(internal->buffer_time), 0);
	if (err < 0){
          adebug("snd_pcm_hw_params_set_buffer_time_near() failed.\n");
          return err;
        }

	/* commit the params structure to the hardware via ALSA */
	err = snd_pcm_hw_params(internal->pcm_handle, params);
	if (err < 0){
          adebug("snd_pcm_hw_params() failed.\n");
          return err;
        }

	/* save the period size in frames for posterity */
	err = snd_pcm_hw_params_get_period_size(params,
						&(internal->period_size), 0);
	if (err < 0){
          adebug("snd_pcm_hw_params_get_period_size() failed.\n");
          return err;
        }

	return 1;
}


/* setup alsa data transfer behavior */
static inline int alsa_set_swparams(ao_device *device)
{
	ao_alsa_internal *internal  = (ao_alsa_internal *) device->internal;
	snd_pcm_sw_params_t *params;
        snd_pcm_uframes_t boundary;
	int err;

	/* allocate the software parameter structure */
	snd_pcm_sw_params_alloca(&params);

	/* fetch the current software parameters */
	err = snd_pcm_sw_params_current(internal->pcm_handle, params);
	if (err < 0){
          adebug("snd_pcm_sw_params_current() failed.\n");
          return err;
        }

#if 0 /* the below causes more trouble than it cures */
	/* allow transfers to start when there is one period */
	err = snd_pcm_sw_params_set_start_threshold(internal->pcm_handle,
                                                    params, internal->period_size);
	if (err < 0){
          adebug("snd_pcm_sw_params_set_start_threshold() failed.\n");
          //return err;
        }

	/* require a minimum of one full transfer in the buffer */
	err = snd_pcm_sw_params_set_avail_min(internal->pcm_handle, params,
                                              internal->period_size);
	if (err < 0){
          adebug("snd_pcm_sw_params_set_avail_min() failed.\n");
          //return err;
        }
#endif

	/* do not align transfers; this is obsolete/deprecated in ALSA
           1.x where the transfer alignemnt is always 1 (except for
           buggy drivers like VIA 82xx which still demand aligned
           transfers regardless of setting, in violation of the ALSA
           API docs) */
	err = snd_pcm_sw_params_set_xfer_align(internal->pcm_handle, params, 1);
	if (err < 0){
          adebug("snd_pcm_sw_params_set_xfer_align() failed.\n");
          //return err;
        }


        /* get the boundary size */
        err = snd_pcm_sw_params_get_boundary(params,&boundary);
	if (err < 0){
          adebug("snd_pcm_sw_params_get_boundary() failed.\n");
        }else{

          /* force a work-ahead silence buffer; this is a fix, again for
             VIA 82xx, where non-MMIO transfers will buffer into
             period-size transfers, but the last transfer is usually
             undersized and playback falls off the end of the submitted
             data. */
          err = snd_pcm_sw_params_set_silence_size(internal->pcm_handle, params, boundary);
          if (err < 0){
            adebug("snd_pcm_sw_params_set_silence_size() failed.\n");
            //return err;
          }
        }

	/* commit the params structure to ALSA */
	err = snd_pcm_sw_params(internal->pcm_handle, params);
	if (err < 0){
          adebug("snd_pcm_sw_params() failed.\n");
          return err;
        }

	return 1;
}


/* Devices declared in the alsa configuration will usually open
   without error, even if there's no underlying hardware to support
   them, eg, opening a 5.1 surround device on setero hardware.  The
   device won't 'fail' until there's an attempt to configure it. */

static inline int alsa_test_open(ao_device *device,
                                 char *dev,
                                 ao_sample_format *format)
{
  ao_alsa_internal *internal  = (ao_alsa_internal *) device->internal;
  snd_pcm_hw_params_t   *params;
  int err;

  adebug("Trying to open ALSA device '%s'\n",dev);

  internal->local_config = init_local_config_with_workaround(device,dev);
  if(internal->local_config)
    err = snd_pcm_open_lconf(&(internal->pcm_handle), dev,
                             SND_PCM_STREAM_PLAYBACK, 0, internal->local_config);
  else
    err = snd_pcm_open(&(internal->pcm_handle), dev,
                       SND_PCM_STREAM_PLAYBACK, 0);

  if(err){
    adebug("Unable to open ALSA device '%s'\n",dev);
    if(internal->local_config)
      snd_config_delete(internal->local_config);
    internal->local_config=NULL;
    return err;
  }

  /* try to set up hw params */
  err = alsa_set_hwparams(device,format);
  if(err<0){
    adebug("Unable to open ALSA device '%s'\n",dev);
    snd_pcm_close(internal->pcm_handle);
    if(internal->local_config)
      snd_config_delete(internal->local_config);
    internal->local_config=NULL;
    internal->pcm_handle = NULL;
    return err;
  }

  /* try to set up sw params */
  err = alsa_set_swparams(device);
  if(err<0){
    adebug("Unable to open ALSA device '%s'\n",dev);
    snd_pcm_close(internal->pcm_handle);
    if(internal->local_config)
      snd_config_delete(internal->local_config);
    internal->local_config=NULL;
    internal->pcm_handle = NULL;
    return err;
  }

  /* this is a hack and fragile if the exact device detection code
     flow changes!  Nevertheless, this is a useful warning for users.
     Never fail silently if we can help it! */
  if(!strcasecmp(dev,"default")){
    /* default device */
    if(device->output_channels>2){
      awarn("ALSA 'default' device plays only channels 0,1.\n");
    }
  }
  if(!strcasecmp(dev,"default") || !strncasecmp(dev,"plug",4)){
    if(format->bits>16){
      awarn("ALSA '%s' device may only simulate >16 bit playback\n",dev);
    }
  }

  /* success! */
  return 0;
}

/* prepare the audio device for playback */
int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
	ao_alsa_internal *internal  = (ao_alsa_internal *) device->internal;
	int err,prebits;

	/* Get the ALSA bitformat first to make sure it's valid */
	err = alsa_get_sample_bitformat(format->bits,
                                        device->client_byte_format == AO_FMT_BIG,device);
	if (err < 0){
          aerror("Invalid byte format\n");
          return 0;
        }

	internal->bitformat = err;

	/* alsa's endianness will (nearly always) be the same as the application's */
	if (format->bits > 8)
		device->driver_byte_format = device->client_byte_format;

        prebits = format->bits;

	/* Open the ALSA device */
        err=0;
        if(!internal->dev){
          if(internal->id<0){
            char *tmp=NULL;
            /* we don't try just 'default' as it's a plug device that
               will accept any number of channels but usually plays back
               everything as stereo. */
            switch(device->output_channels){
            default:
            case 8:
            case 7:
              err = alsa_test_open(device, tmp="surround71", format);
              break;
            case 4:
            case 3:
              err = alsa_test_open(device, tmp="surround40", format);
              if(err==0)break;
            case 6:
            case 5:
              err = alsa_test_open(device, tmp="surround51", format);
              break;
            case 2:
              err = alsa_test_open(device, tmp="front", format);
            case 1:
              break;
            }

            if(err){
              awarn("Unable to open surround playback.  Trying default device...\n");
              tmp=NULL;
            }

            if(!tmp)
              err = alsa_test_open(device, tmp="default", format);

            internal->dev=strdup(tmp);
          }else{
            char tmp[80];
            sprintf(tmp,"hw:%d",internal->id);
            internal->dev=strdup(tmp);
            err = alsa_test_open(device, internal->dev, format);
          }
        }else
          err = alsa_test_open(device, internal->dev, format);

	if (err < 0) {
          aerror("Unable to open ALSA device '%s' for playback => %s\n",
                 internal->dev, snd_strerror(err));
          return 0;
	}

        internal->padbuffer = 0;
        internal->padoutw = 0;
        if(internal->bitformat == SND_PCM_FORMAT_S24){
          internal->padbuffer = calloc(4096,1);
          internal->padoutw = 32;
        }else if(prebits != format->bits){
          internal->padbuffer = calloc(4096,1);
          internal->padoutw = (format->bits+7)/8;
          format->bits=prebits;
        }

        adebug("Using ALSA device '%s'\n",internal->dev);
        {
          snd_pcm_sframes_t sframes;
          if(snd_pcm_delay (internal->pcm_handle, &sframes)){
            internal->static_delay=0;
          }else{
            internal->static_delay=sframes;
          }
        }

	/* save the sample size in bytes for posterity */
	internal->sample_size = format->bits * device->output_channels / 8;

        if(strcasecmp(internal->dev,"default")){
          if(strncasecmp(internal->dev,"surround",8)){
            if(device->output_channels>2 && device->verbose>=0){
              awarn("No way to determine hardware %d channel mapping of\n"
                    "ALSA device '%s'.\n",device->output_channels, internal->dev);
              if(device->inter_matrix){
                free(device->inter_matrix);
                device->inter_matrix=NULL;
              }
            }
          }
        }

	return 1;
}


/* recover from an alsa exception */
static inline int alsa_error_recovery(ao_alsa_internal *internal, int err, ao_device *device)
{
	if (err == -EPIPE) {
		/* FIXME: underrun length detection */
		adebug("underrun, restarting...\n");
		/* output buffer underrun */
		err = snd_pcm_prepare(internal->pcm_handle);
		if (err < 0)
			return err;
	} else if (err == -ESTRPIPE) {
		/* application was suspended, wait until suspend flag clears */
		while ((err = snd_pcm_resume(internal->pcm_handle)) == -EAGAIN)
			sleep (1);

		if (err < 0) {
			/* unable to wake up pcm device, restart it */
			err = snd_pcm_prepare(internal->pcm_handle);
			if (err < 0)
				return err;
		}
		return 0;
	}

	/* error isn't recoverable */
	return err;
}


static int ao_plugin_playi(ao_device *device, const char *output_samples, 
                           uint_32 num_bytes, int sample_size)
{
	ao_alsa_internal *internal = (ao_alsa_internal *) device->internal;
       	uint_32 len = num_bytes / sample_size;
	char *ptr = (char *) output_samples;
	int err;

        /* the entire buffer might not transfer at once */
        while (len > 0) {
                /* try to write the entire buffer at once */
                err = internal->writei(internal->pcm_handle, ptr, len);

                /* no data transferred or interrupt signal */
                if (err == -EAGAIN || err == -EINTR) continue;

                if (err < 0) {
                        /* this might be an error, or an exception */
                        err = alsa_error_recovery(internal, err, device);
                        if (err < 0) {
                                aerror("write error: %s\n",
                                       snd_strerror(err));
                                return 0;
                        }else continue;
                }

                /* decrement the sample counter */
                len -= err;

                /* adjust the start pointer */
                ptr += err * sample_size;
        }

	return 1;
}

/* play num_bytes of audio data */
int ao_plugin_play(ao_device *device, const char *output_samples, 
		uint_32 num_bytes)
{
  ao_alsa_internal *internal = (ao_alsa_internal *) device->internal;
  int endianp = ao_is_big_endian();

  /* the bit padding should be at a higher layer where we're doing
     other permutation/swizzling, but for now only ALSA has need of
     this, and moving it up will require a plugin API change */

  if(internal->padbuffer){
    /* pad and forward ~ a page at a time; must not hang on fractional frames */
    int ibytewidth = internal->sample_size / device->output_channels;
    int obytewidth = internal->padoutw;
    int istride = internal->sample_size;
    int ostride = obytewidth*device->output_channels;

    while(num_bytes >= internal->sample_size){
      int oframes = 4096/(obytewidth*device->output_channels);
      int iframes = num_bytes/internal->sample_size;
      int frames = oframes<iframes ? oframes : iframes;
      int obytes = frames * obytewidth * device->output_channels;
      int ibytes = frames * ibytewidth * device->output_channels;
      int i,j;

      /* copy */
      for(j=0;j<ibytewidth;j++){
        const char *s = output_samples + j;
        char *d = internal->padbuffer + (endianp ? j : obytewidth-ibytewidth+j);
        for(i=0;i<frames*device->output_channels;i++){
          *d = *s;
          s+=ibytewidth;
          d+=obytewidth;
        }
      }

      /* pad */
      for(;j<obytewidth;j++){
        char *d = internal->padbuffer + (endianp ? j : j-ibytewidth);
        for(i=0;i<frames*device->output_channels;i++){
          *d = 0;
          d+=obytewidth;
        }
      }

      if(!ao_plugin_playi(device,internal->padbuffer,obytes,obytewidth*device->output_channels))
        return 0;

      num_bytes-=frames*internal->sample_size;
      output_samples+=frames*internal->sample_size;
    }
    return 1;
  }else
    return ao_plugin_playi(device,output_samples,num_bytes,internal->sample_size);
}


/* close the audio device */
int ao_plugin_close(ao_device *device)
{
	ao_alsa_internal *internal;

	if (device) {
          if ((internal = (ao_alsa_internal *) device->internal)) {
            if (internal->pcm_handle) {

              /* this is a PulseAudio ALSA emulation bug workaround;
                 snd_pcm_drain always takes about 2 seconds, even if
                 there's nothing to drain.  Rather than wait for no
                 reason, determine the current playback depth, wait
                 that long, then kill the stream.  Remove this code
                 once Pulse gets fixed. */

              snd_pcm_sframes_t sframes;
              if(snd_pcm_delay (internal->pcm_handle, &sframes)){
                snd_pcm_drain(internal->pcm_handle);
              }else{
                double s = (double)(sframes - internal->static_delay)/internal->sample_rate;
                if(s>1){
                  /* something went wrong; fall back */
                  snd_pcm_drain(internal->pcm_handle);
                }else{
                  if(s>0){
                    struct timespec sleep,wake;
                    sleep.tv_sec = (int)s;
                    sleep.tv_nsec = (s-sleep.tv_sec)*1000000000;
                    while(nanosleep(&sleep,&wake)<0){
                      if(errno==EINTR)
                        sleep=wake;
                      else
                        break;
                    }
                  }
                }
              }
              snd_pcm_close(internal->pcm_handle);
              if(internal->local_config)
                snd_config_delete(internal->local_config);
              internal->local_config=NULL;
              internal->pcm_handle=NULL;
            }
          } else
            awarn("ao_plugin_close called with uninitialized ao_device->internal\n");
	} else
          awarn("ao_plugin_close called with uninitialized ao_device\n");

	return 1;
}


/* free the internal data structures */
void ao_plugin_device_clear(ao_device *device)
{
	ao_alsa_internal *internal;

	if (device) {
          if ((internal = (ao_alsa_internal *) device->internal)) {
            if (internal->dev)
              free (internal->dev);
            else
              awarn("ao_plugin_device_clear called with uninitialized ao_device->internal->dev\n");
            if (internal->padbuffer)
              free (internal->padbuffer);
            free(internal);
            device->internal=NULL;
          } else
            awarn("ao_plugin_device_clear called with uninitialized ao_device->internal\n");
	} else
          awarn("ao_plugin_device_clear called with uninitialized ao_device\n");
}

