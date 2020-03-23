/*
 *
 *  ao_roar.c
 *
 *      Copyright (C) Philipp 'ph3-der-loewe' Schafft - 2008-2010
 *      based on ao_esd.c of libao by Stan Seibert - July 2000, July 2001
 *
 *  This file is part of RoarAudio, a cross-platform sound server. See
 *  README for a history of this source code.
 *
 *  This file is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  RoarAudio is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <roaraudio.h>
#include <ao/ao.h>
#include <ao/plugin.h>

#define DEFAULT_CLIENT_NAME "libao client"

static char *ao_roar_options[] = {
  "host", "server",
  "client_name",
  "id", "dev",
  "role"
  "verbose", "quiet", "matrix", "debug"
};

static ao_info ao_roar_info ={
  AO_TYPE_LIVE,
  "RoarAudio output",
  "roar",
  "Philipp 'ph3-der-loewe' Schafft, Based on code by: Stan Seibert <volsung@asu.edu>",
  "Outputs to the RoarAudio Sound Daemon.",
  AO_FMT_NATIVE,
  50,
  ao_roar_options,
  sizeof(ao_roar_options)/sizeof(*ao_roar_options)
};


typedef struct ao_roar_internal {
  struct roar_connection con;
  int con_opened;
  roar_vs_t * vss;
  char * host;
  char * mixer;
  char * client_name;
  int    role;
} ao_roar_internal;

#ifdef ROAR_LIBROAR_CONFIG_WAS_NO_SLP
static int workarounds_save;

static void disable_slp (void) {
  struct roar_libroar_config * config = roar_libroar_get_config();

  workarounds_save = config->workaround.workarounds;

  config->workaround.workarounds |= ROAR_LIBROAR_CONFIG_WAS_NO_SLP;
}

static void reenable_slp (void) {
  struct roar_libroar_config * config = roar_libroar_get_config();

  config->workaround.workarounds = workarounds_save;
}
#else
#define disable_slp()
#define reenable_slp()
#endif

int ao_plugin_test(void) {
  struct roar_connection con;

#ifdef ROAR_FT_FUNC_SIMPLE_CONNECT2
  if ( roar_simple_connect2(&con, NULL, DEFAULT_CLIENT_NAME, ROAR_ENUM_FLAG_NONBLOCK, 0) == -1 )
    return 0;
#else
  disable_slp();
  if ( roar_simple_connect(&con, NULL, DEFAULT_CLIENT_NAME) == -1 ) {
    reenable_slp();
    return 0;
  }
  reenable_slp();
#endif

  if (roar_get_standby(&con)) {
    roar_disconnect(&con);
    return 0;
  }

  roar_disconnect(&con);
  return 1;
}


ao_info * ao_plugin_driver_info(void) {
  return &ao_roar_info;
}

enum errtype {
 ERROR,
 WARN,
 INFO,
 VERBOSE,
 DEBUG
};

#define armsg(et,text) _ao_roar_err_real(device, __LINE__, (et), (text), error)
static inline void _ao_roar_err_real(ao_device * device, long line, enum errtype errtype, const char * text, int error) {
 const char * errmsg = roar_vs_strerr(error);
#define _format "%s: %s"
#define _format_dbg _format " at line %li"

 switch (errtype) {
  case ERROR:
    aerror(_format "\n", text, errmsg);
   break;
  case WARN:
    awarn(_format "\n", text, errmsg);
   break;
  case INFO:
    ainfo(_format "\n", text, errmsg);
   break;
  case VERBOSE:
    averbose(_format_dbg "\n", text, errmsg, line);
   break;
  case DEBUG:
    adebug(_format_dbg "\n", text, errmsg, line);
   break;
 }
}


int ao_plugin_device_init(ao_device * device) {
  ao_roar_internal * internal;

  internal = (ao_roar_internal *) calloc(1,sizeof(ao_roar_internal));

  if (internal == NULL)
    return 0;

  internal->con_opened  = 0;

  internal->vss         = NULL;

  internal->host        = NULL;
  internal->mixer       = NULL;
  internal->client_name = strdup(DEFAULT_CLIENT_NAME);
  internal->role        = ROAR_ROLE_UNKNOWN;

  device->internal = internal;
  device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

  return 1;
}

int ao_plugin_set_option(ao_device * device, const char * key, const char * value) {
  ao_roar_internal * internal = (ao_roar_internal *) device->internal;

  if ( !strcmp(key, "host") || !strcmp(key, "server") ) {
    if(internal->host) free(internal->host);
    internal->host = strdup(value);
  } else if ( !strcmp(key, "id") || !strcmp(key, "dev") ) {
    if(internal->mixer) free(internal->mixer);
    internal->mixer = strdup(value);
  } else if ( !strcmp(key, "client_name") ) {
    if(internal->client_name) free(internal->client_name);
    internal->client_name = strdup(value);
  } else if ( !strcmp(key, "role") ) {
    internal->role = roar_str2role(value);
  }

  return 1;
}

static int ao_roar_con_open (ao_roar_internal * internal) {
  if ( internal->con_opened )
    return 1;

#ifdef ROAR_FT_FUNC_SIMPLE_CONNECT2
  if ( roar_simple_connect2(&(internal->con), internal->host, internal->client_name, ROAR_ENUM_FLAG_NONBLOCK, 0) == -1 )
   return 0;
#else
  if ( internal->host == NULL )
    disable_slp();
  if ( roar_simple_connect(&(internal->con), internal->host, internal->client_name) == -1 ) {
    if ( internal->host == NULL )
      reenable_slp();
    return 0;
  }
  if ( internal->host == NULL )
    reenable_slp();
#endif

  internal->con_opened = 1;

  return 1;
}

#if defined(ROAR_FT_FUNC_LIST_FILTERED) && defined(ROAR_VS_CMD_SET_MIXER)
static int ao_roar_find_mixer (ao_roar_internal * internal) {
  struct roar_stream stream;
  char name[80];
  int id[ROAR_STREAMS_MAX];
  int num;
  int i;
  int ret;

  if ( internal->mixer == NULL )
    return -1;

  // test if mixer is numeric...
  ret = atoi(internal->mixer);

  if ( ret > 0 ) {
    return ret;
  } else if ( ret == 0 && internal->mixer[0] == '0' ) {
    return 0;
  }

  if ( (num = roar_list_filtered(&(internal->con), id, ROAR_STREAMS_MAX, // normal parameters,
                                 ROAR_CMD_LIST_STREAMS,      // normaly hidden by
                                                             // roar_list_streams().
                                 ROAR_CTL_FILTER_DIR,        // extra parameter: filter on /dir/
                                 ROAR_CTL_CMP_EQ,            // extra parameter: ... that must be equal ...
                                 ROAR_DIR_MIXING)) == -1 ) { // extra parameter: to this.
    return -1;
  }

  for (i = 0; i < num; i++) {
    if ( roar_stream_new_by_id(&stream, id[i]) == -1 )
      continue;

    if ( roar_stream_get_name(&(internal->con), &stream, name, sizeof(name)) != 0 )
      continue;

    if ( !strncasecmp(name, internal->mixer, sizeof(name)) )
      return id[i];
  }

  return -1;
}
#endif

int ao_plugin_open(ao_device * device, ao_sample_format * format) {
  ao_roar_internal * internal = (ao_roar_internal *) device->internal;
  struct roar_audio_info info;
  char * map = NULL;
  int codec = ROAR_CODEC_DEFAULT;
  int mixer;
  int error = -1;

  info.rate     = format->rate;
  info.channels = format->channels;
  info.codec    = codec;
  info.bits     = format->bits;

  if ( !ao_roar_con_open(internal) )
    return 0;

  internal->vss = roar_vs_new_from_con(&(internal->con), &error);

  if ( internal->vss == NULL ) {
    armsg(ERROR, "Can not create VS object");
    return 0;
  }

#if defined(ROAR_FT_FUNC_LIST_FILTERED) && defined(ROAR_VS_CMD_SET_MIXER)
  mixer = ao_roar_find_mixer(internal);

  if ( mixer != -1 ) {
    if ( roar_vs_ctl(internal->vss, ROAR_VS_CMD_SET_MIXER, &mixer, &error) == -1 ) {
     roar_vs_close(internal->vss, ROAR_VS_FALSE, NULL);
     internal->vss = NULL;
     armsg(ERROR, "Can not set mixer");
     return 0;
    }
    armsg(WARN, "Installed version of libroar does not support VS CTL, mixer setting is ignored.");
  }
#else
  if ( internal->mixer != NULL ) {
    error = ROAR_ERROR_NOTSUP;
    armsg(WARN, "Installed version of libroar does not support VS CTL, mixer setting is ignored");
  }
#endif

  if ( roar_vs_stream(internal->vss, &info, ROAR_DIR_PLAY, &error) == -1 ) {
    roar_vs_close(internal->vss, ROAR_VS_FALSE, NULL);
    internal->vss = NULL;
    armsg(ERROR, "Can not start playback");
    return 0;
  }

  if ( internal->role != ROAR_ROLE_UNKNOWN )
    if ( roar_vs_role(internal->vss, internal->role, &error) == -1 )
      armsg(WARN, "Can not set stream role");

  device->driver_byte_format = AO_FMT_NATIVE;

  if(!device->inter_matrix){ /* It would be set if an app or user force-sets the mapping; don't overwrite! */
    switch (format->channels) {
    case  1: map = "M";               break;
    case  2: map = "L,R";             break;
    case  3: map = "L,R,C";           break;
    case  4: map = "L,R,BL,BR";       break;
    case  5: map = "L,R,C,BL,BR";     break;
    case  6: map = "L,R,C,LFE,BL,BR"; break;
    }
    /* >6 channels will warn about inability to map */

    if ( map )
      device->inter_matrix = strdup(map);
  }

  return 1;
}

int ao_plugin_play(ao_device * device, const char * output_samples, uint_32 num_bytes) {
  ao_roar_internal * internal = (ao_roar_internal *) device->internal;
  ssize_t ret;

  // TODO: handle short writes.

  ret = roar_vs_write(internal->vss, output_samples, num_bytes, NULL);

  if ( ret == -1 ) {
    return 0;
  } else {
    return 1;
  }
}


int ao_plugin_close(ao_device * device) {
  ao_roar_internal * internal = (ao_roar_internal *) device->internal;

  if(internal->vss != NULL)
    roar_vs_close(internal->vss, ROAR_VS_FALSE, NULL);

  internal->vss = NULL;

  if (internal->con_opened)
   roar_disconnect(&(internal->con));

  internal->con_opened = 0;

  return 1;
}


void ao_plugin_device_clear(ao_device * device) {
  ao_roar_internal * internal = (ao_roar_internal *) device->internal;

  if( internal->host != NULL )
    free(internal->host);

  if( internal->mixer != NULL )
    free(internal->mixer);

  if( internal->client_name != NULL )
    free(internal->client_name);

  free(internal);
  device->internal=NULL;
}
