/*
 *
 *  ao_esd.c
 *
 *      Copyright (C) Stan Seibert - July 2000, July 2001
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
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include <esd.h>
#include <ao/ao.h>
#include <ao/plugin.h>

extern char **environ;

static char *ao_esd_options[] = {"server","host","matrix","verbose","quiet","debug","client_name"};
static ao_info ao_esd_info =
{
	AO_TYPE_LIVE,
	"ESounD output",
	"esd",
	"Stan Seibert <volsung@asu.edu>",
	"Outputs to the Enlightened Sound Daemon.",
	AO_FMT_NATIVE,
	40,
	ao_esd_options,
        sizeof(ao_esd_options)/sizeof(*ao_esd_options)
};


typedef struct ao_esd_internal
{
	int sock;
	char *host;
        char *client_name;
        char bugbuffer[4096];
        int bugfill;
        int bits;
} ao_esd_internal;

/* An old favorite from the UNIX-hater's handbook.
   two things worth noting:
   1) 'not found' returns -1 with a zero errno
   2) removed strings are freed
*/

int portable_unsetenv(char *name){
  char **p = environ;
  if(name){
    if(strchr(name,'=')){
      errno=EINVAL;
      return -1;
    }
    while(*p){
      char *pos = strchr(*p,'=');
      if((pos && !strncmp(name,*p,(pos-*p))) ||
         (!pos && !strcmp(name,*p))){
        /* assume we can free it */
        free(*p);
        /* scrunch.  Not actually O bigger than moving last into the
           slot as we need to scan the whole environment anyway */
        do{
          *p = *(p+1);
          p++;
        }while(*p);
        return 0;
      }
      p++;
    }
  }
  errno = 0;
  return -1; /* not found */
}

int ao_plugin_test()
{
	int sock;

	/* don't wake up the beast while detecting */
	putenv(strdup("ESD_NO_SPAWN=1"));
	sock = esd_open_sound(NULL);
        portable_unsetenv("ESD_NO_SPAWN");
	if (sock < 0)
		return 0;
	if (esd_get_standby_mode(sock) != ESM_RUNNING) {
		esd_close(sock);
		return 0;
	}

	esd_close(sock);
	return 1;
}


ao_info *ao_plugin_driver_info(void)
{
	return &ao_esd_info;
}


int ao_plugin_device_init(ao_device *device)
{
	ao_esd_internal *internal;

	internal = (ao_esd_internal *) calloc(1,sizeof(ao_esd_internal));

	if (internal == NULL)
		return 0; /* Could not initialize device memory */

	internal->host = NULL;
	internal->client_name = NULL;
        internal->sock = -1;
	device->internal = internal;
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;
        device->output_matrix=strdup("L,R");

	return 1; /* Memory alloc successful */
}

int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
{
	ao_esd_internal *internal = (ao_esd_internal *) device->internal;

	if (!strcmp(key, "host") || !strcmp(key, "server")) {
		if(internal->host) free(internal->host);
		internal->host = strdup(value);
	} else if (!strcmp(key, "client_name")) {
		if(internal->client_name) free(internal->client_name);
		internal->client_name = strdup(value);
	}

	return 1;
}

int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
	ao_esd_internal *internal = (ao_esd_internal *) device->internal;
	int esd_bits;
	int esd_channels;
	int esd_mode = ESD_STREAM;
	int esd_func = ESD_PLAY;
	int esd_format;

	switch (format->bits)
	{
	case 8  :
          esd_bits = ESD_BITS8;
          internal->bits = 8;
          break;
	case 16 :
          esd_bits = ESD_BITS16;
          internal->bits = 16;
          break;
	default :
          return 0;
	}
	switch (device->output_channels)
	{
	case 1 : esd_channels = ESD_MONO;
		 break;
	case 2 : esd_channels = ESD_STEREO;
		 break;
	default: return 0;
	}

	esd_format = esd_bits | esd_channels | esd_mode | esd_func;

	internal->sock = esd_play_stream(esd_format, format->rate,
					 internal->host,
					 internal->client_name == NULL ?
                                         "libao output" : internal->client_name);
	if (internal->sock < 0)
		return 0; /* Could not contact ESD server */

	device->driver_byte_format = AO_FMT_NATIVE;

	return 1;
}

/* ESD has a longstanding bug where it won't properly handle any input
   frame size other than ESD_BUF_SIZE, which is defined in esd.h to be
   4096.  The bug is lines 317/318 in esound/players.c; the lines
   perform an unneeded/incorrect short-read test that abort playback
   of any nonblocking read that returns not-4096 bytes.  Simply
   commenting out these two lines corrects the problem, but even if
   it's fixed today, ESD is dead and any already deployed daemons are
   unlilely to ever be fixed.

   So... only ever write 4096 byte chunks.  Ever. */

int write4096(int fd, const char *output_samples){
  int num_bytes = 4096;
  while (num_bytes > 0) {
    /* the loop always makes sure at least a complete block is written
       out almost always, almost instantly-- there's no other way to
       deal.  Except to fix esd of course. */
    ssize_t ret = write(fd, output_samples, num_bytes);
    if(ret<0){
      switch(errno){
      case EAGAIN:
      case EINTR:
        break;
      default:
        return ret;
      }
    }else{
      output_samples += ret;
      num_bytes -= ret;
    }
  }
  return 0;
}

int ao_plugin_play(ao_device *device, const char* output_samples,
                   uint_32 num_bytes)
{
  ao_esd_internal *internal = (ao_esd_internal *) device->internal;

  if(internal->bugfill){
    int addto = internal->bugfill + num_bytes;
    if(addto>4096) addto = 4096;
    addto -= internal->bugfill;
    if(addto){
      memcpy(internal->bugbuffer + internal->bugfill, output_samples, addto);
      output_samples += addto;
      internal->bugfill += addto;
      num_bytes -= addto;
    }
  }

  if(internal->bugfill==4096){
    if(write4096(internal->sock,internal->bugbuffer))
      return 0;
    internal->bugfill=0;
  }

  while(num_bytes>=4096){
    if(write4096(internal->sock,output_samples))
      return 0;
    output_samples += 4096;
    num_bytes -= 4096;
  }

  if(num_bytes){
    memcpy(internal->bugbuffer, output_samples, num_bytes);
    internal->bugfill = num_bytes;
  }
  return 1;
}

int ao_plugin_close(ao_device *device)
{
  ao_esd_internal *internal = (ao_esd_internal *) device->internal;

  if(internal->bugfill && internal->sock != -1){

    if(internal->bugfill<4096){
      switch(internal->bits){
      case 8:
        memset(internal->bugbuffer + internal->bugfill, 128, 4096-internal->bugfill);
        break;
      default:
        memset(internal->bugbuffer + internal->bugfill, 0, 4096-internal->bugfill);
        break;
      }
    }

    write4096(internal->sock,internal->bugbuffer);
    internal->bugfill = 0;
  }

  if(internal->sock != -1)
    esd_close(internal->sock);
  internal->sock=-1;
  return 1;
}


void ao_plugin_device_clear(ao_device *device)
{
  ao_esd_internal *internal = (ao_esd_internal *) device->internal;

  if(internal->host) free(internal->host);
  if(internal->client_name) free(internal->client_name);
  free(internal);
  device->internal=NULL;
}
