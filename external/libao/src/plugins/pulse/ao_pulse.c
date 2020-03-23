/***
  This file is part of libao-pulse.

  libao-pulse is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  libao-pulse is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with libao-pulse; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.

 ********************************************************************

 last mod: $Id$

 ********************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/xmalloc.h>
#include <pulse/util.h>

#include <ao/ao.h>
#include <ao/plugin.h>

#define AO_PULSE_BUFFER_TIME 20000

/* Unfortunately libao doesn't allow "const" for these structures... */
static char * ao_pulse_options[] = {
    "server",
    "sink",
    "dev",
    "id",
    "verbose",
    "quiet",
    "matrix",
    "debug",
    "client_name"
    "buffer_time"
};

static ao_info ao_pulse_info = {
    AO_TYPE_LIVE,
    "PulseAudio Output",
    "pulse",
    PACKAGE_BUGREPORT,
    "Outputs to the PulseAudio Sound Server",
    AO_FMT_NATIVE,
    50,
    ao_pulse_options,
    sizeof(ao_pulse_options)/sizeof(*ao_pulse_options)
};

typedef struct ao_pulse_internal {
    struct pa_simple *simple;
    char *server, *sink, *client_name;
    pa_usec_t static_delay;
    pa_usec_t buffer_time;
} ao_pulse_internal;

/* Yes, this is very ugly, but required nonetheless... */
static void disable_sigpipe(void) {
    struct sigaction sa;

    sigaction(SIGPIPE, NULL, &sa);
    if (sa.sa_handler != SIG_IGN) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_IGN;
        sa.sa_flags = SA_RESTART;
        sigaction(SIGPIPE, &sa, NULL);
    }
}

int ao_plugin_test(void) {
    char *p=NULL, t[256], t2[256];
    const char *fn;
    struct pa_simple *s;
    static const struct pa_sample_spec ss = {
        .format = PA_SAMPLE_S16NE,
        .rate = 44100,
        .channels = 2
    };
    size_t allocated = 128;

    disable_sigpipe();

    if (getenv("PULSE_SERVER") || getenv("PULSE_SINK"))
        return 1;

    while (1) {
      p = pa_xmalloc(allocated);

      if (!(fn = pa_get_binary_name(p, allocated))) {
        break;
      }

      if (fn != p || strlen(p) < allocated - 1) {
        snprintf(t, sizeof(t), "libao[%s]", fn);
        snprintf(t2, sizeof(t2), "libao[%s] test", fn);
        break;
      }

      pa_xfree(p);
      allocated *= 2;
    }
    pa_xfree(p);
    p = NULL;

    if (!(s = pa_simple_new(NULL, fn ? t : "libao", PA_STREAM_PLAYBACK, NULL, fn ? t2 : "libao test", &ss, NULL, NULL, NULL)))
        return 0;

    pa_simple_free(s);
    return 1;
}

ao_info *ao_plugin_driver_info(void) {
    return &ao_pulse_info;
}

int ao_plugin_device_init(ao_device *device) {
    ao_pulse_internal *internal;
    assert(device);

    internal = (ao_pulse_internal *) malloc(sizeof(ao_pulse_internal));

    if (internal == NULL)
        return 0;

    internal->simple = NULL;
    internal->server = NULL;
    internal->sink = NULL;
    internal->client_name = NULL;
    internal->buffer_time = AO_PULSE_BUFFER_TIME;

    device->internal = internal;
    device->output_matrix_order = AO_OUTPUT_MATRIX_PERMUTABLE;
    device->output_matrix = strdup("M,L,R,C,BC,BL,BR,LFE,CL,CR,SL,SR,"
                                   "A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,"
                                   "A10,A11,A12,A13,A14,A15,A16,A17,A18,A19,"
                                   "A20,A21,A22,A23,A24,A25,A26,A27,A28,A29,"
                                   "A30,A31");
    return 1;
}

int ao_plugin_set_option(ao_device *device, const char *key, const char *value) {
    ao_pulse_internal *internal;
    assert(device && device->internal && key && value);
    internal = (ao_pulse_internal *) device->internal;

    if (!strcmp(key, "server")) {
        free(internal->server);
        internal->server = strdup(value);
    } else if (!strcmp(key, "sink") || !strcmp(key, "dev") || !strcmp(key, "id")) {
        free(internal->sink);
        internal->sink = strdup(value);
    } else if (!strcmp(key, "client_name")) {
        free(internal->client_name);
        internal->client_name = strdup(value);
    }else if (!strcmp(key, "buffer_time")){
      internal->buffer_time = atoi(value) * 1000;
    }

    return 1;
}
int ao_plugin_open(ao_device *device, ao_sample_format *format) {
    char *p=NULL, t[256], t2[256];
    const char *fn = NULL;
    ao_pulse_internal *internal;
    struct pa_sample_spec ss;
    struct pa_channel_map map;
    struct pa_buffer_attr battr;
    size_t allocated = 128;

    assert(device && device->internal && format);

    internal = (ao_pulse_internal *) device->internal;

    if (format->bits == 8)
      ss.format = PA_SAMPLE_U8;
    else if (format->bits == 16)
      ss.format = PA_SAMPLE_S16NE;
#ifdef PA_SAMPLE_S24NE
    else if (format->bits == 24)
      ss.format = PA_SAMPLE_S24NE;
#endif
    else
        return 0;

    if (device->output_channels <= 0 || device->output_channels > PA_CHANNELS_MAX)
      return 0;

    ss.channels = device->output_channels;
    ss.rate = format->rate;

    disable_sigpipe();

    if (internal->client_name) {
        snprintf(t, sizeof(t), "libao[%s]", internal->client_name);
        snprintf(t2, sizeof(t2), "libao[%s] playback stream", internal->client_name);
    } else {
      while (1) {
        p = pa_xmalloc(allocated);

        if (!(fn = pa_get_binary_name(p, allocated))) {
          break;
        }

        if (fn != p || strlen(p) < allocated - 1) {
          fn = pa_path_get_filename(fn);
          snprintf(t, sizeof(t), "libao[%s]", fn);
          snprintf(t2, sizeof(t2), "libao[%s] playback stream", fn);
          break;
        }

        pa_xfree(p);
        allocated *= 2;
      }
      pa_xfree(p);
      p = NULL;
      if (!fn) {
        strcpy(t, "libao");
        strcpy(t2, "libao playback stream");
      }
    }

    if(device->input_map){
      int i;
      pa_channel_map_init(&map);
      map.channels=device->output_channels;

      for(i=0;i<device->output_channels;i++){
        if(device->input_map[i]==-1){
          map.map[i] = PA_CHANNEL_POSITION_INVALID;
        }else{
          map.map[i] = device->input_map[i];
        }
      }
    }

    /* buffering attributes */
    battr.prebuf = battr.minreq = battr.fragsize = -1;

    battr.tlength = (int)(internal->buffer_time * format->rate) / 1000000 *
      ((format->bits+7)/8) * device->output_channels;
    battr.minreq = battr.tlength/4;
    battr.maxlength = battr.tlength+battr.minreq;

    internal->simple = pa_simple_new(internal->server, t, PA_STREAM_PLAYBACK,
                                     internal->sink, t2, &ss,
                                     (device->input_map ? &map : NULL), &battr, NULL);
    if (!internal->simple)
        return 0;

    device->driver_byte_format = AO_FMT_NATIVE;

    internal->static_delay = pa_simple_get_latency(internal->simple, NULL);
    if(internal->static_delay<0) internal->static_delay = 0;

    return 1;
}

int ao_plugin_play(ao_device *device, const char* output_samples, uint_32 num_bytes) {
    assert(device && device->internal);
    ao_pulse_internal *internal = (ao_pulse_internal *) device->internal;

    return pa_simple_write(internal->simple, output_samples, num_bytes, NULL) >= 0;
}


int ao_plugin_close(ao_device *device) {
    assert(device && device->internal);
    ao_pulse_internal *internal = (ao_pulse_internal *) device->internal;

    if(internal->simple){

      /* this is a PulseAudio ALSA bug workaround;
         pa_simple_drain() always takes about 2 seconds, even if
         there's nothing to drain.  Rather than wait for no
         reason, determine the current playback depth, wait
         that long, then kill the stream.  Remove this code
         once Pulse gets fixed. */

      pa_usec_t us = pa_simple_get_latency(internal->simple, NULL);
      if(us<0 || us>1000000){
        pa_simple_drain(internal->simple, NULL);
      }else{
        us -= internal->static_delay;
        if(us>0){
          struct timespec sleep,wake;
          sleep.tv_sec = (int)(us/1000000);
          sleep.tv_nsec = (us-sleep.tv_sec*1000000)*1000;
          while(nanosleep(&sleep,&wake)<0){
            if(errno==EINTR)
              sleep=wake;
            else
              break;
          }
        }
      }

      pa_simple_free(internal->simple);
      internal->simple = NULL;
    }

    return 1;
}

void ao_plugin_device_clear(ao_device *device) {
    assert(device && device->internal);
    ao_pulse_internal *internal = (ao_pulse_internal *) device->internal;

    if(internal->server)
      free(internal->server);
    if(internal->sink)
      free(internal->sink);
    if(internal->client_name)
      free(internal->client_name);
    free(internal);
    device->internal = NULL;
}
