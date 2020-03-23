/*
 *
 *  ao_arts.c
 *
 *      Copyright (C) Rik Hemsley (rikkus) <rik@kde.org> 2000
 *     Modifications Copyright (C) 2010 Monty <monty@xiph.org>
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
#include <pthread.h>

#include <glib.h>
#include <artsc.h>
#include <ao/ao.h>
#include <ao/plugin.h>

/* we must serialize all aRtsc library access as virtually every
   operation accesses global state */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int server_open_count = 0;

static char *ao_arts_options[] = {"matrix","verbose","quiet","debug","multi"};
static ao_info ao_arts_info =
{
	AO_TYPE_LIVE,
	"aRts output",
	"arts",
	"Monty <monty@xiph.org>",
	"Outputs to the aRts soundserver.",
	AO_FMT_NATIVE,
#ifdef HAVE_ARTS_SUSPENDED
	45,
#else
	15,
#endif
	ao_arts_options,
        sizeof(ao_arts_options)/sizeof(*ao_arts_options)
};

typedef struct ao_arts_internal
{
  arts_stream_t stream;
  int allow_multi;
  int buffersize;
} ao_arts_internal;


int ao_plugin_test()
{
  pthread_mutex_lock(&mutex);

  if (server_open_count || arts_init() == 0) {
    server_open_count++;

#ifdef HAVE_ARTS_SUSPENDED
    if (arts_suspended() == 1) {
      server_open_count--;
      if(!server_open_count)arts_free();
      pthread_mutex_unlock(&mutex);
      return 0;
    }
#endif
    server_open_count--;
    if(!server_open_count)arts_free();
    arts_free();
    pthread_mutex_unlock(&mutex);
    return 1;
  }
  pthread_mutex_unlock(&mutex);
  return 0;
}

ao_info *ao_plugin_driver_info(void)
{
  /* this is a dirty but necessary trick.  aRts's C library for
     clients calls g_thread_init() internally in arts_init() whether
     your app uses glib or not.  This call sets several
     thread-specific keys and stashes glib static data state in the
     calling thread.  Later, ao_close() calls arts_free(), glib is
     dlclose()d, but the keys aren't deleted and this will cause a
     segfault when the thread that originally called arts_init() exits
     and pthreads tries to clean up. In addition, g_thread_init() must
     be called outside of mutextes, and access to arts_init() below is
     and must be locked.

     So we tackle this problem in two ways; one, call g_thread_init()
     here, which will be during ao_initialize().  It is documented
     that ao_initialize() must be called in the app's main
     thread. Second, be sure to link with glib-2.0, which means that
     the glib static context is never unloaded by aRts (this alone is
     currently enough in practice to avoid problems, but that's partly
     by accident. The g_thread_init() here avoids it randomly breaking
     again in the future by following documentation exactly). */

  if (!g_thread_supported ())
    g_thread_init(0);

  return &ao_arts_info;
}


int ao_plugin_device_init(ao_device *device)
{
  ao_arts_internal *internal;

  internal = (ao_arts_internal *) calloc(1,sizeof(ao_arts_internal));

  if (internal == NULL)
    return 0; /* Could not initialize device memory */

  device->internal = internal;
  device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;
  device->output_matrix=strdup("L,R");

  return 1; /* Memory alloc successful */
}


int ao_plugin_set_option(ao_device *device, const char *key, const char *value)
{
  ao_arts_internal *internal = (ao_arts_internal *) device->internal;

  if (!strcmp(key, "multi")) {
    if(!strcmp(value,"yes") || !strcmp(value,"y") ||
       !strcmp(value,"true") || !strcmp(value,"t") ||
       !strcmp(value,"1"))
      {
        internal->allow_multi = 1;
        return 1;
      }
    if(!strcmp(value,"no") || !strcmp(value,"n") ||
       !strcmp(value,"false") || !strcmp(value,"f") ||
       !strcmp(value,"0"))
      {
        internal->allow_multi = 0;
        return 1;
      }
    return 0;
  }
  return 1;
}

int ao_plugin_open(ao_device *device, ao_sample_format *format)
{
  ao_arts_internal *internal = (ao_arts_internal *) device->internal;
  int errorcode=0;

  if(device->output_channels<1 || device->output_channels>2){
    /* the docs aren't kidding here--- feed it more than 2
       channels and the server simply stops answering; the
       connection freezes. */
    aerror("Cannot handle more than 2 channels\n");
    return 0;
  }

  pthread_mutex_lock(&mutex);
  if(!server_open_count)
    errorcode = arts_init();
  else{
    if(!internal->allow_multi){
      /* multiple-playback access disallowed; it's disallowed by
         default as it tends to crash the aRts server. */
      adebug("Multiple-open access disallowed and playback already in progress.\n");
      pthread_mutex_unlock(&mutex);
      return 0;
    }
  }

  if (0 != errorcode){
    pthread_mutex_unlock(&mutex);
    aerror("Could not connect to server => %s.\n",arts_error_text(errorcode));
    return 0; /* Could not connect to server */
  }

  device->driver_byte_format = AO_FMT_NATIVE;
  internal->stream = arts_play_stream(format->rate,
                                      format->bits,
                                      device->output_channels,
                                      "libao stream");

  if(!internal->stream){
    if(!server_open_count)arts_free();
    pthread_mutex_unlock(&mutex);

    aerror("Could not open audio stream.\n");
    return 0;
  }

  if(arts_stream_set(internal->stream, ARTS_P_BLOCKING, 0)){
    arts_close_stream(internal->stream);
    internal->stream=NULL;
    if(!server_open_count)arts_free();
    pthread_mutex_unlock(&mutex);

    aerror("Could not set audio stream to nonblocking.\n");
    return 0;
  }

  if((internal->buffersize = arts_stream_get(internal->stream, ARTS_P_BUFFER_SIZE))<=0){
    arts_close_stream(internal->stream);
    internal->stream=NULL;
    if(!server_open_count)arts_free();
    pthread_mutex_unlock(&mutex);

    aerror("Could not get audio buffer size.\n");
    return 0;
  }

  server_open_count++;
  pthread_mutex_unlock(&mutex);

  return 1;
}


int ao_plugin_play(ao_device *device, const char *output_samples,
		uint_32 num_bytes)
{
  ao_arts_internal *internal = (ao_arts_internal *) device->internal;
  int spindetect=0;
  int i;

  pthread_mutex_lock(&mutex);

  /* the while loop below is another dirty but servicable hack needed
     for two reasons:

     1) for multiple-stream playback, there is no way to block on
     more than one stream object at a time.  One can neither
     select/poll, nor can we block on multiple writes at a time as
     access to arts_write must be locked globally.  So we run in
     nonblocking mode and write to the server based on audio timing.

     2) Although aRts allegedly delivers errors on write failure, I've
     never observed it actually do so in practice.  Most of the time
     when something goes wrong, it returns a short count or zero, but
     there are also cases where the write simply blocks forever
     because the server logged an error and stopped answering without
     informing the client or dropping the connection.  Again, we have
     to run in nonblocking moda and look for an output pattern that
     indicates the server disappeared out from under us (no successful
     writes over a period that should certainly have starved
     playback) */

  while(1){
    int accwrote=0;

    /* why the multiple rapid-fire writes below?

       aRts in nonblocking mode does not service internal buffering
       state outside of the write call.  Further, the internal buffer
       state appears to be pipelined; although the server may be
       waiting or even starved for data, a non blocking write call
       will often return immediately without actually writing
       anything, regardless of internal buffer fullness.  Several more
       calls (all returning 0, due to the full internal buffer) will
       suddenly cause the internal state to actually flush data to the
       server.  Thus the multiple writes in sequence are a way of
       having the aRts internal state step through the sequence
       necessary to actually submit data to the server.
    */

    for(i=0;i<5;i++){
      int wrote = arts_write(internal->stream, output_samples, num_bytes);
      if(wrote < 0){
        /* although it's vanishingly unlikely that aRtsc will actually
           bother reporting any errors, we might as well be ready for
           one. */
        pthread_mutex_unlock(&mutex);
        aerror("Write error\n");
        return 0;
      }
      accwrote+=wrote;
      num_bytes -= wrote;
      output_samples += wrote;
    }

    if(accwrote)
      spindetect=0;
    else
      spindetect++;

    if(spindetect==100){
        pthread_mutex_unlock(&mutex);
        aerror("Write thread spinning; has the aRts server crashed?\n");
        return 0;
    }

    if(num_bytes>0){
      long wait = internal->buffersize*1000/(device->output_channels*device->bytewidth*device->rate);
      pthread_mutex_unlock(&mutex);
      wait = (wait/8)*1000;
      if(wait<1)wait=1;
      if(wait>500000)wait=500000;
      usleep(wait);
      pthread_mutex_lock(&mutex);
    }else{
      pthread_mutex_unlock(&mutex);
      break;
    }
  }

  return 1;
}


int ao_plugin_close(ao_device *device)
{
  ao_arts_internal *internal = (ao_arts_internal *) device->internal;
  pthread_mutex_lock(&mutex);
  if(internal->stream)
    arts_close_stream(internal->stream);
  internal->stream = NULL;

  server_open_count--;
  if(!server_open_count)arts_free();
  pthread_mutex_unlock(&mutex);

  return 1;
}


void ao_plugin_device_clear(ao_device *device)
{
  ao_arts_internal *internal = (ao_arts_internal *) device->internal;

  if(internal)
    free(internal);
  device->internal=NULL;
}
