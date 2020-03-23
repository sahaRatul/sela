/*
 *
 *  audio_out.c
 *
 *      Original Copyright (C) Aaron Holtzman - May 1999
 *      Modifications Copyright (C) Stan Seibert - July 2000
 *      Modifications Copyright (C) Philipp Schafft - February 2012
 *
 *  This file is part of libao, a cross-platform audio output library.  See
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#if defined HAVE_DLFCN_H && defined HAVE_DLOPEN
# include <dlfcn.h>
#else
static void *dlopen(const char *filename, int flag) {return 0;}
static char *dlerror(void) { return "dlopen: unsupported"; }
static void *dlsym(void *handle, const char *symbol) { return 0; }
static int dlclose(void *handle) { return 0; }
#undef DLOPEN_FLAG
#define DLOPEN_FLAG 0
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
# include <unistd.h>
# include <dirent.h>
#endif

#include "ao/ao.h"
#include "ao_private.h"

/* These should have been set by the Makefile */
#ifndef AO_PLUGIN_PATH
#define AO_PLUGIN_PATH "/usr/local/lib/ao"
#endif
#ifndef SHARED_LIB_EXT
#define SHARED_LIB_EXT ".so"
#endif

/* --- Other constants --- */
#define DEF_SWAP_BUF_SIZE  1024

/* --- Driver Table --- */

typedef struct driver_list {
	ao_functions *functions;
	void *handle;
	struct driver_list *next;
} driver_list;


extern ao_functions ao_null;
extern ao_functions ao_wav;
extern ao_functions ao_raw;
extern ao_functions ao_au;
#ifdef HAVE_SYS_AUDIO_H
extern ao_functions ao_aixs;
#endif
#ifdef HAVE_WMM
extern ao_functions ao_wmm;
#endif
static ao_functions *static_drivers[] = {
	&ao_null, /* Must have at least one static driver! */
	&ao_wav,
	&ao_raw,
	&ao_au,
#ifdef HAVE_SYS_AUDIO_H
	&ao_aixs,
#endif
#ifdef HAVE_WMM
	&ao_wmm,
#endif

	NULL /* End of list */
};

static driver_list *driver_head = NULL;
static ao_config config = {
	NULL /* default_driver */
};

static ao_info **info_table = NULL;
static int driver_count = 0;

/* uses the device messaging and option infrastructure */
static ao_device *ao_global_dummy;
static ao_device ao_global_dummy_storage;
static ao_option *ao_global_options=NULL;

/* ---------- Helper functions ---------- */

/* Clear out all of the library configuration options and set them to
   defaults.   The defaults should match the initializer above. */
static void _clear_config()
{
        memset(ao_global_dummy,0,sizeof(*ao_global_dummy));
        ao_global_dummy = NULL;
        ao_free_options(ao_global_options);
        ao_global_options = NULL;

	free(config.default_driver);
	config.default_driver = NULL;
}


/* Load a plugin from disk and put the function table into a driver_list
   struct. */
static driver_list *_get_plugin(char *plugin_file)
{
        ao_device *device = ao_global_dummy;
	driver_list *dt;
	void *handle;
        char *prompt="";

	handle = dlopen(plugin_file, DLOPEN_FLAG /* See ao_private.h */);

	if (handle) {
                prompt="calloc() failed";
                dt = (driver_list *)calloc(1,sizeof(driver_list));
		if (!dt) return NULL;

		dt->handle = handle;

		dt->functions = (ao_functions *)calloc(1,sizeof(ao_functions));
		if (!(dt->functions)) {
			free(dt);
			return NULL;
		}

                prompt="ao_plugin_test() missing";
		dt->functions->test = dlsym(dt->handle, "ao_plugin_test");
		if (!(dt->functions->test)) goto failed;

                prompt="ao_plugin_driver_info() missing";
		dt->functions->driver_info =
		  dlsym(dt->handle, "ao_plugin_driver_info");
		if (!(dt->functions->driver_info)) goto failed;

                prompt="ao_plugin_device_list() missing";
		dt->functions->device_init =
		  dlsym(dt->handle, "ao_plugin_device_init");
		if (!(dt->functions->device_init )) goto failed;

                prompt="ao_plugin_set_option() missing";
		dt->functions->set_option =
		  dlsym(dt->handle, "ao_plugin_set_option");
		if (!(dt->functions->set_option)) goto failed;

                prompt="ao_plugin_open() missing";
		dt->functions->open = dlsym(dt->handle, "ao_plugin_open");
		if (!(dt->functions->open)) goto failed;

                prompt="ao_plugin_play() missing";
		dt->functions->play = dlsym(dt->handle, "ao_plugin_play");
		if (!(dt->functions->play)) goto failed;

                prompt="ao_plugin_close() missing";
		dt->functions->close = dlsym(dt->handle, "ao_plugin_close");
		if (!(dt->functions->close)) goto failed;

                prompt="ao_plugin_clear() missing";
		dt->functions->device_clear =
		  dlsym(dt->handle, "ao_plugin_device_clear");
		if (!(dt->functions->device_clear)) goto failed;


	} else {
          aerror("Failed to load plugin %s => dlopen() failed\n",plugin_file);
          return NULL;
	}

        adebug("Loaded driver %s\n",dt->functions->driver_info()->short_name);
	return dt;

 failed:
        aerror("Failed to load plugin %s => %s\n",plugin_file,prompt);
	free(dt->functions);
	free(dt);
	return NULL;
}


/* If *name is a valid driver name, return its driver number.
   Otherwise, test all of available live drivers until one works. */
static int _find_default_driver_id (const char *name)
{
	int def_id;
	int id;
	ao_info *info;
	driver_list *dl = driver_head;
        ao_device *device = ao_global_dummy;

        adebug("Testing drivers to find playback default...\n");
	if ( name == NULL || (def_id = ao_driver_id(name)) < 0 ) {
		/* No default specified. Find one among available drivers. */
		def_id = -1;

		id = 0;
		while (dl != NULL) {

			info = dl->functions->driver_info();
                        adebug("...testing %s\n",info->short_name);
			if ( info->type == AO_TYPE_LIVE &&
			     info->priority > 0 && /* Skip static drivers */
			     dl->functions->test() ) {
				def_id = id; /* Found a usable driver */
                                adebug("OK, using driver %s\n",info->short_name);
				break;
			}

			dl = dl->next;
			id++;
		}
	}

	return def_id;
}


/* Convert the static drivers table into a linked list of drivers. */
static driver_list* _load_static_drivers(driver_list **end)
{
        ao_device *device = ao_global_dummy;
	driver_list *head;
	driver_list *driver;
	int i;

	/* insert first driver */
	head = driver = calloc(1,sizeof(driver_list));
	if (driver != NULL) {
		driver->functions = static_drivers[0];
		driver->handle = NULL;
		driver->next = NULL;
                adebug("Loaded driver %s (built-in)\n",driver->functions->driver_info()->short_name);

		i = 1;
		while (static_drivers[i] != NULL) {
                  driver->next = calloc(1,sizeof(driver_list));
			if (driver->next == NULL)
				break;

			driver->next->functions = static_drivers[i];
			driver->next->handle = NULL;
			driver->next->next = NULL;

			driver = driver->next;
                        adebug("Loaded driver %s (built-in)\n",driver->functions->driver_info()->short_name);
			i++;
		}
	}

	if (end != NULL)
		*end = driver;

	return head;
}


/* Load the dynamic drivers from disk and append them to end of the
   driver list.  end points the driver_list node to append to. */
static void _append_dynamic_drivers(driver_list *end)
{
#ifdef HAVE_DLOPEN
	struct dirent *plugin_dirent;
	char *ext;
	struct stat statbuf;
	DIR *plugindir;
	driver_list *plugin;
	driver_list *driver = end;
        ao_device *device = ao_global_dummy;

	/* now insert any plugins we find */
	plugindir = opendir(AO_PLUGIN_PATH);
        adebug("Loading driver plugins from %s...\n",AO_PLUGIN_PATH);
	if (plugindir != NULL) {
          while ((plugin_dirent = readdir(plugindir)) != NULL) {
            char fullpath[strlen(AO_PLUGIN_PATH) + 1 + strlen(plugin_dirent->d_name) + 1];
            snprintf(fullpath, sizeof(fullpath), "%s/%s",
                     AO_PLUGIN_PATH, plugin_dirent->d_name);
            if (!stat(fullpath, &statbuf) &&
                S_ISREG(statbuf.st_mode) &&
                (ext = strrchr(plugin_dirent->d_name, '.')) != NULL) {
              if (strcmp(ext, SHARED_LIB_EXT) == 0) {
                plugin = _get_plugin(fullpath);
                if (plugin) {
                  driver->next = plugin;
                  plugin->next = NULL;
                  driver = driver->next;
                }
              }
            }
          }

          closedir(plugindir);
	}
#endif
}


/* Compare two drivers based on priority
   Used as compar function for qsort() in _make_info_table() */
static int _compar_driver_priority (const driver_list **a,
				    const driver_list **b)
{
	return memcmp(&((*b)->functions->driver_info()->priority),
		      &((*a)->functions->driver_info()->priority),
		      sizeof(int));
}


/* Make a table of driver info structures for ao_driver_info_list(). */
static ao_info ** _make_info_table (driver_list **head, int *driver_count)
{
	driver_list *list;
	int i;
	ao_info **table;
	driver_list **drivers_table;

	*driver_count = 0;

	/* Count drivers */
	list = *head;
	i = 0;
	while (list != NULL) {
		i++;
		list = list->next;
	}


	/* Sort driver_list */
	drivers_table = (driver_list **) calloc(i, sizeof(driver_list *));
	if (drivers_table == NULL)
		return (ao_info **) NULL;
	list = *head;
	*driver_count = i;
	for (i = 0; i < *driver_count; i++, list = list->next)
		drivers_table[i] = list;
	qsort(drivers_table, i, sizeof(driver_list *),
			(int(*)(const void *, const void *))
			_compar_driver_priority);
	*head = drivers_table[0];
	for (i = 1; i < *driver_count; i++)
		drivers_table[i-1]->next = drivers_table[i];
	drivers_table[i-1]->next = NULL;


	/* Alloc table */
	table = (ao_info **) calloc(i, sizeof(ao_info *));
	if (table != NULL) {
		for (i = 0; i < *driver_count; i++)
			table[i] = drivers_table[i]->functions->driver_info();
	}

	free(drivers_table);

	return table;
}


/* Return the driver struct corresponding to particular driver id
   number. */
static driver_list *_get_driver(int driver_id) {
	int i = 0;
	driver_list *driver = driver_head;

	if (driver_id < 0) return NULL;

	while (driver && (i < driver_id)) {
		i++;
		driver = driver->next;
	}

	if (i == driver_id)
		return driver;

	return NULL;
}


/* Check if driver_id is a valid id number */
static int _check_driver_id(int driver_id)
{
	int i = 0;
	driver_list *driver = driver_head;

	if (driver_id < 0) return 0;

	while (driver && (i <= driver_id)) {
		driver = driver->next;
		i++;
	}

	if (i == (driver_id + 1))
		return 1;

	return 0;
}


/* helper function to convert a byte_format of AO_FMT_NATIVE to the
   actual byte format of the machine, otherwise just return
   byte_format */
static int _real_byte_format(int byte_format)
{
	if (byte_format == AO_FMT_NATIVE) {
		if (ao_is_big_endian())
			return AO_FMT_BIG;
		else
			return AO_FMT_LITTLE;
	} else
		return byte_format;
}


/* Create a new ao_device structure and populate it with data */
static ao_device* _create_device(int driver_id, driver_list *driver,
				 ao_sample_format *format, FILE *file)
{
	ao_device *device;

	device = calloc(1,sizeof(ao_device));

	if (device != NULL) {
		device->type = driver->functions->driver_info()->type;
		device->driver_id = driver_id;
		device->funcs = driver->functions;
		device->file = file;
		device->machine_byte_format =
		  ao_is_big_endian() ? AO_FMT_BIG : AO_FMT_LITTLE;
		device->client_byte_format =
		  _real_byte_format(format->byte_format);
		device->swap_buffer = NULL;
		device->swap_buffer_size = 0;
		device->internal = NULL;
                device->output_channels = format->channels;
                device->inter_permute = NULL;
                device->output_matrix = NULL;
	}

	return device;
}


/* Expand the swap buffer in this device if it is smaller than
   min_size. */
static int _realloc_swap_buffer(ao_device *device, int min_size)
{
	void *temp;

	if (min_size > device->swap_buffer_size) {
		temp = realloc(device->swap_buffer, min_size);
		if (temp != NULL) {
			device->swap_buffer = temp;
			device->swap_buffer_size = min_size;
			return 1; /* Success, realloc worked */
		} else
			return 0; /* Fail to realloc */
	} else
		return 1; /* Success, no need to realloc */
}


static void _buffer_zero(char *target,int och,int bytewidth,int ochannels,int bytes){
  int i = och*bytewidth;
  int stride = bytewidth*ochannels;
  switch(bytewidth){
  case 1:
    while(i<bytes){
      ((unsigned char *)target)[i] = 128; /* 8 bit PCM is unsigned in libao */
      i+=stride;
    }
    break;
  case 2:
    while(i<bytes){
      target[i] = 0;
      target[i+1] = 0;
      i+=stride;
    }
    break;
  case 3:
    while(i<bytes){
      target[i] = 0;
      target[i+1] = 0;
      target[i+2] = 0;
      i+=stride;
    }
    break;
  case 4:
    while(i<bytes){
      target[i] = 0;
      target[i+1] = 0;
      target[i+2] = 0;
      target[i+3] = 0;
      i+=stride;
    }
    break;
  }
}

static void _buffer_permute_swap(char *target,int och,int bytewidth,int ochannels,int bytes,
                                 char *source,int ich, int ichannels){
  int o = och*bytewidth;
  int i = ich*bytewidth;
  int ostride = bytewidth*ochannels;
  int istride = bytewidth*ichannels;
  switch(bytewidth){
  case 2:
    while(o<bytes){
      target[o] = source[i+1];
      target[o+1] = source[i];
      o+=ostride;
      i+=istride;
    }
    break;
  case 3:
    while(o<bytes){
      target[o] = source[i+2];
      target[o+1] = source[i+1];
      target[o+2] = source[i];
      o+=ostride;
      i+=istride;
    }
    break;
  case 4:
    while(o<bytes){
      target[o] = source[i+3];
      target[o+1] = source[i+2];
      target[o+2] = source[i+1];
      target[o+3] = source[i];
      o+=ostride;
      i+=istride;
    }
    break;
  }
}

static void _buffer_permute(char *target,int och,int bytewidth,int ochannels,int bytes,
                            char *source,int ich, int ichannels){
  int o = och*bytewidth;
  int i = ich*bytewidth;
  int ostride = bytewidth*ochannels;
  int istride = bytewidth*ichannels;
  switch(bytewidth){
  case 1:
    while(o<bytes){
      target[o] = source[i];
      o+=ostride;
      i+=istride;
    }
    break;
  case 2:
    while(o<bytes){
      target[o] = source[i];
      target[o+1] = source[i+1];
      o+=ostride;
      i+=istride;
    }
    break;
  case 3:
    while(o<bytes){
      target[o] = source[i];
      target[o+1] = source[i+1];
      target[o+2] = source[i+2];
      o+=ostride;
      i+=istride;
    }
    break;
  case 4:
    while(o<bytes){
      target[o] = source[i];
      target[o+1] = source[i+1];
      target[o+2] = source[i+2];
      target[o+3] = source[i+3];
      o+=ostride;
      i+=istride;
    }
    break;
  }
}


/* Swap and copy the byte order of samples from the source buffer to
   the target buffer. */
static void _swap_samples(char *target_buffer, char* source_buffer,
			  uint_32 num_bytes)
{
	uint_32 i;

	for (i = 0; i < num_bytes; i += 2) {
		target_buffer[i] = source_buffer[i+1];
		target_buffer[i+1] = source_buffer[i];
	}
}

/* the channel locations we know right now. code below assumes U is in slot 0, X in 1, M in 2 */
static char *mnemonics[]={
  "X",
  "M",
  "L","C","R","CL","CR","SL","SR","BL","BC","BR","LFE",
  "A1","A2","A3","A4","A5","A6","A7","A8","A9","A10",
  "A11","A12","A13","A14","A15","A16","A17","A18","A19","A20",
  "A21","A22","A23","A24","A25","A26","A27","A28","A29","A30",
  "A31","A32",NULL
};

/* Check the requested matrix string for syntax and mnemonics */
static char *_sanitize_matrix(int maxchannels, char *matrix, ao_device *device){
  if(matrix){
    char *ret = calloc(strlen(matrix)+1,1); /* can only get smaller */
    char *p=matrix;
    int count=0;

    if(!ret)
      return NULL;

    while(count<maxchannels){
      char *h,*t;
      int m=0;

      /* trim leading space */
      while(*p && isspace(*p))p++;

      /* search for seperator */
      h=p;
      while(*h && *h!=',')h++;

      /* trim trailing space */
      t=h;
      while(t>p && isspace(*(t-1)))t--;

      while(mnemonics[m]){
        if(t-p && !strncmp(mnemonics[m],p,t-p) &&
           strlen(mnemonics[m])==t-p){
          if(count)strcat(ret,",");
          strcat(ret,mnemonics[m]);
          break;
        }
        m++;
      }
      if(!mnemonics[m]){
        /* unrecognized channel mnemonic */
        {
          aerror("Unrecognized channel name \"%.*s\" in channel matrix \"%s\"\n",
              t-p, p, matrix);
        }
        free(ret);
        return NULL;
      }else
        count++;
      if(!*h)break;
      p=h+1;
    }
    return ret;
  }else
    return NULL;
}

static int _find_channel(int needle, char *haystack){
  char *p=haystack;
  int count=0;

  /* X does not map to anything, including X! */
  if(needle==0) return -1;

  while(1){
    char *h;
    int m=0;

    /* search for seperator */
    h=p;
    while(*h && *h!=',')h++;

    while(mnemonics[m]){
      if(!strncmp(mnemonics[needle],p,h-p) &&
         strlen(mnemonics[needle])==h-p)break;
      m++;
    }
    if(mnemonics[m])
      return count;
    count++;
    if(!*h)break;
    p=h+1;
  }
  return -1;
}

static void _free_map(char **m){
  char **in=m;
  while(m && *m){
    free(*m);
    m++;
  }
  if(in)free(in);
}

static char **_tokenize_matrix(char *matrix){
  char **ret=NULL;
  char *p=matrix;
  int count=0;
  while(1){
    char *h,*t;

    /* trim leading space */
    while(*p && isspace(*p))p++;

    /* search for seperator */
    h=p;
    while(*h && *h!=',')h++;

    /* trim trailing space */
    t=h;
    while(t>p && isspace(*(t-1)))t--;

    count++;
    if(!*h)break;
    p=h+1;
  }

  ret = calloc(count+1,sizeof(*ret));
  if(!ret)
    return NULL;

  p=matrix;
  count=0;
  while(1){
    char *h,*t;

    /* trim leading space */
    while(*p && isspace(*p))p++;

    /* search for seperator */
    h=p;
    while(*h && *h!=',')h++;

    /* trim trailing space */
    t=h;
    while(t>p && isspace(*(t-1)))t--;

    ret[count] = calloc(t-p+1,1);
    if(!ret[count]){
      _free_map(ret);
      return NULL;
    }
    memcpy(ret[count],p,t-p);
    count++;
    if(!*h)break;
    p=h+1;
  }

  return ret;
}

static unsigned int _matrix_to_channelmask(int ch, char *matrix, char *premap, int **mout){
  unsigned int ret=0;
  char *p=matrix;
  int *perm=(*mout=malloc(ch*sizeof(*mout)));
  int i;
  char **map;

  if(!perm)
    return 0;

  map = _tokenize_matrix(premap);
  if(!map)
    return 0;

  for(i=0;i<ch;i++) perm[i] = -1;
  i=0;

  while(1){
    char *h=p;
    int m=0;

    /* search for seperator */
    while(*h && *h!=',')h++;

    while(map[m]){
      if(h-p && !strncmp(map[m],p,h-p) &&
         strlen(map[m])==h-p)
        break;
      m++;
    }
    /* X is a placeholder, X does not map to X */
    if(map[m] && strcmp(map[m],"X")){
      ret |= (1<<m);
      perm[i] = m;
    }
    if(!*h)break;
    p=h+1;
    i++;
  }

  _free_map(map);
  return ret;
}

static char *_channelmask_to_matrix(unsigned int mask, char *premap){
  int m=0;
  int count=0;
  char buffer[257]={0};
  char **map = _tokenize_matrix(premap);

  if(!map)
    return NULL;

  while(map[m]){
    if(mask & (1<<m)){
      if(count)
        strcat(buffer,",");
      strcat(buffer,map[m]);
      count++;
    }
    m++;
  }
  _free_map(map);
  return _strdup(buffer);
}

static int _channelmask_bits(unsigned int mask){
  int count=0;
  while(mask){
    if(mask&1)count++;
    mask/=2;
  }
  return count;
}

static int _channelmask_maxbit(unsigned int mask){
  int count=0;
  int max=-1;
  while(mask){
    if(mask&1)max=count;
    mask/=2;
    count++;
  }
  return max;
}

static char *_matrix_intersect(char *matrix,char *premap){
  char *p=matrix;
  char buffer[257]={0};
  int count=0;
  char **map = _tokenize_matrix(premap);

  if(!map)
    return NULL;

  while(1){
    char *h=p;
    int m=0;

    /* search for seperator */
    while(*h && *h!=',')h++;

    while(map[m]){
      if(h-p && !strncmp(map[m],p,h-p) &&
         strlen(map[m])==h-p)
        break;
      m++;
    }
    /* X is a placeholder, X does not map to X */
    if(map[m] && strcmp(map[m],"X")){
      if(count)
        strcat(buffer,",");
      strcat(buffer,map[m]);
      count++;
    }

    if(!*h)break;
    p=h+1;
  }

  _free_map(map);
  return _strdup(buffer);
}

static int ao_global_load_options(ao_option *options){
  while (options != NULL) {
    if(!strcmp(options->key,"debug")){
      ao_global_dummy->verbose=2;
    }else if(!strcmp(options->key,"verbose")){
      if(ao_global_dummy->verbose<1)ao_global_dummy->verbose=1;
    }else if(!strcmp(options->key,"quiet")){
      ao_global_dummy->verbose=-1;
    }

    options = options->next;
  }

  return 0;

}

static int ao_device_load_options(ao_device *device, ao_option *options){

  while (options != NULL) {
    if(!strcmp(options->key,"matrix")){
      /* If a driver has a static channel mapping mechanism
         (physically constant channel mapping, or at least an
         unvarying set of constants for mapping channels), the
         output_matrix is already set.  An app/user specified
         output mapping trumps. */
      if(device->output_matrix)
        free(device->output_matrix);
      /* explicitly set the output matrix to the requested
         string; devices must not override. */
      device->output_matrix = _sanitize_matrix(32, options->value, device);
      if(!device->output_matrix){
        aerror("Empty or inavlid output matrix\n");
        return AO_EBADOPTION;
      }
      adebug("Sanitized device output matrix: %s\n",device->output_matrix);
    }else if(!strcmp(options->key,"debug")){
      device->verbose=2;
    }else if(!strcmp(options->key,"verbose")){
      if(device->verbose<1)device->verbose=1;
    }else if(!strcmp(options->key,"quiet")){
      device->verbose=-1;
    }else{
      if (!device->funcs->set_option(device, options->key, options->value)) {
        /* Problem setting options */
        aerror("Driver %s unable to set option %s=%s\n",
               info_table[device->driver_id]->short_name, options->key, options->value);
        return AO_EOPENDEVICE;
      }
    }

    options = options->next;
  }

  return 0;
}

/* Open a device.  If this is a live device, file == NULL. */
static ao_device* _open_device(int driver_id, ao_sample_format *format,
			       ao_option *options, FILE *file)
{
	ao_functions *funcs;
	driver_list *driver;
	ao_device *device=NULL;
	int result;
        ao_sample_format sformat=*format;
        sformat.matrix=NULL;

	/* Get driver id */
	if ( (driver = _get_driver(driver_id)) == NULL ) {
		errno = AO_ENODRIVER;
		goto error;
	}

	funcs = driver->functions;

	/* Check the driver type */
	if (file == NULL &&
	    funcs->driver_info()->type != AO_TYPE_LIVE) {

		errno = AO_ENOTLIVE;
		goto error;
	} else if (file != NULL &&
		   funcs->driver_info()->type != AO_TYPE_FILE) {

		errno = AO_ENOTFILE;
		goto error;
	}

	/* Make a new device structure */
	if ( (device = _create_device(driver_id, driver,
				      format, file)) == NULL ) {
		errno = AO_EFAIL;
		goto error;
	}

	/* Initialize the device memory; get static channel mapping */
	if (!funcs->device_init(device)) {
		errno = AO_EFAIL;
		goto error;
	}

	/* Load options */
        errno = ao_device_load_options(device,ao_global_options);
        if(errno) goto error;
        errno = ao_device_load_options(device,options);
        if(errno) goto error;

        /* also sanitize the format input channel matrix */
        if(format->matrix){
          sformat.matrix = _sanitize_matrix(format->channels, format->matrix, device);
          if(!sformat.matrix)
            awarn("Input channel matrix invalid; ignoring.\n");

          /* special-case handling of 'M'. */
          if(sformat.channels==1 && sformat.matrix && !strcmp(sformat.matrix,"M")){
            free(sformat.matrix);
            sformat.matrix=NULL;
          }
        }

        /* If device init was able to declare a static channel mapping
           mechanism, reconcile it to the input now.  Odd drivers that
           need to communicate with a backend device to determine
           channel mapping strategy can still bypass this mechanism
           entirely.  Also, drivers like ALSA may need to adjust
           strategy depending on what device is successfully opened,
           etc, but this still saves work later. */

        if(device->output_matrix && sformat.matrix){
          switch(device->output_matrix_order){
          case AO_OUTPUT_MATRIX_FIXED:
            /* Backend channel ordering is immutable and unused
               channels must still be sent.  Look for the highest
               channel number we are able to map from the input
               matrix. */
            {
              unsigned int mask = _matrix_to_channelmask(sformat.channels,sformat.matrix,
                                                         device->output_matrix,
                                                         &device->input_map);
              int max = _channelmask_maxbit(mask);
              if(max<0){
                aerror("Unable to map any channels from input matrix to output");
                errno = AO_EBADFORMAT;
                goto error;
              }
              device->output_channels = max+1;
              device->output_mask = mask;
              device->inter_matrix = _strdup(device->output_matrix);
            }
            break;

          case AO_OUTPUT_MATRIX_COLLAPSIBLE:
            /* the ordering of channels submitted to the backend is
               fixed, but only the channels in use should be present
               in the audio stream */
            {
              unsigned int mask = _matrix_to_channelmask(sformat.channels,sformat.matrix,
                                                         device->output_matrix,
                                                         &device->input_map);
              int channels = _channelmask_bits(mask);
              if(channels<=0){
                aerror("Unable to map any channels from input matrix to output");
                errno = AO_EBADFORMAT;
                goto error;
              }
              device->output_channels = channels;
              device->output_mask = mask;
              device->inter_matrix = _channelmask_to_matrix(mask,device->output_matrix);
            }
            break;

          case AO_OUTPUT_MATRIX_PERMUTABLE:
            /* The ordering of channels is freeform.  Only the
               channels in use should be sent, and they may be sent in
               any order.  If possible, leave the input ordering
               unchanged */
            {
              unsigned int mask = _matrix_to_channelmask(sformat.channels,sformat.matrix,
                                                         device->output_matrix,
                                                         &device->input_map);
              int channels = _channelmask_bits(mask);
              if(channels<=0){
                aerror("Unable to map any channels from input matrix to output");
                errno = AO_EBADFORMAT;
                goto error;
              }
              device->output_channels = channels;
              device->output_mask = mask;
              device->inter_matrix = _matrix_intersect(sformat.matrix,device->output_matrix);
            }
            break;

          default:
            aerror("Driver backend failed to set output ordering.\n");
            errno = AO_EFAIL;
            goto error;
          }

        }else{
          device->output_channels = sformat.channels;
        }

        /* other housekeeping */
        device->input_channels = sformat.channels;
        device->bytewidth = (sformat.bits+7)>>3;
        device->rate = sformat.rate;

	/* Open the device */
	result = funcs->open(device, &sformat);
	if (!result) {
          errno = AO_EOPENDEVICE;
          goto error;
	}

        /* set up permutation based on finalized inter_matrix mapping */
        /* The only way device->output_channels could be zero here is
           if the driver has opted to ouput no channels (eg, the null
           driver). */
        if(sformat.matrix){
          if(!device->inter_matrix){
            awarn("Driver %s does not support automatic channel mapping;\n"
                 "\tRouting only L/R channels to output.\n\n",
                 info_table[device->driver_id]->short_name);
            device->inter_matrix=_strdup("L,R");
          }
          {

            /* walk thorugh the inter matrix, match channels */
            char *op=device->inter_matrix;
            int count=0;
            device->inter_permute = calloc(device->output_channels,sizeof(int));

            if (!device->inter_permute) {
              errno = AO_EFAIL;
              goto error;
            }
            adebug("\n");

            while(count<device->output_channels){
              int m=0,mm;
              char *h=op;

              if(*op){
                /* find mnemonic offset of inter channel */
                while(*h && *h!=',')h++;
                while(mnemonics[m]){
                  if(!strncmp(mnemonics[m],op,h-op))
                    break;
                  m++;
                }
                mm=m;

                /* find match in input if any */
                device->inter_permute[count] = _find_channel(m,sformat.matrix);
                if(device->inter_permute[count] == -1 && sformat.channels == 1){
                  device->inter_permute[count] = _find_channel(1,sformat.matrix);
                  mm=1;
                }
              }else
                device->inter_permute[count] = -1;

              /* display resulting mapping for now */
              if(device->inter_permute[count]>=0){
                adebug("input %d (%s)\t -> backend %d (%s)\n",
                       device->inter_permute[count], mnemonics[mm],
                       count,mnemonics[m]);
              }else{
                adebug("             \t    backend %d (%s)\n",
                       count,mnemonics[m]);
              }
              count++;
              op=h;
              if(*h)op++;
            }
            {
              char **inch = _tokenize_matrix(sformat.matrix);
              int i,j;
              int unflag=0;
              for(j=0;j<sformat.channels;j++){
                for(i=0;i<device->output_channels;i++)
                  if(device->inter_permute[i]==j)break;
                if(i==device->output_channels){
                  if(inch){
                    adebug("input %d (%s)\t -> none\n",
                           j,inch[j]);
                  }
                  unflag=1;
                }
              }
              _free_map(inch);
              if(unflag)
                awarn("Some input channels are unmapped and will not be used.\n");
            }
            averbose("\n");

          }
        }

        /* if there's no actual permutation to do, release the permutation vector */
        if(device->inter_permute && device->output_channels == device->input_channels){
          int i;
          for(i=0;i<device->output_channels;i++)
            if(device->inter_permute[i]!=i)break;
          if(i==device->output_channels){
            free(device->inter_permute);
            device->inter_permute=NULL;
          }
        }

	/* Resolve actual driver byte format */
	device->driver_byte_format =
		_real_byte_format(device->driver_byte_format);

	/* Only create swap buffer if needed */
        if (device->bytewidth>1 &&
            device->client_byte_format != device->driver_byte_format){
          adebug("swap buffer required:\n");
          adebug("  machine endianness: %d\n",ao_is_big_endian());
          adebug("  device->client_byte_format:%d\n",device->client_byte_format);
          adebug("  device->driver_byte_format:%d\n",device->driver_byte_format);
        }

	if ( (device->bytewidth>1 &&
              device->client_byte_format != device->driver_byte_format) ||
             device->input_channels != device->output_channels ||
             device->inter_permute){

          result = _realloc_swap_buffer(device, DEF_SWAP_BUF_SIZE);

          if (!result) {

            if(sformat.matrix)free(sformat.matrix);
            device->funcs->close(device);
            device->funcs->device_clear(device);
            free(device);
            errno = AO_EFAIL;
            return NULL; /* Couldn't alloc swap buffer */
          }
	}

	/* If we made it this far, everything is OK. */
        if(sformat.matrix)free(sformat.matrix);
	return device;

 error:
        {
          int errtemp = errno;
          if(sformat.matrix)
            free(sformat.matrix);
          ao_close(device);
          errno=errtemp;
        }
        return NULL;
}


/* ---------- Public Functions ---------- */

/* -- Library Setup/Teardown -- */

static ao_info ao_dummy_info=
  { 0,0,0,0,0,0,0,0,0 };
static ao_info *ao_dummy_driver_info(void){
  return &ao_dummy_info;
}
static ao_functions ao_dummy_funcs=
  { 0, &ao_dummy_driver_info, 0,0,0,0,0,0,0};

void ao_initialize(void)
{
	driver_list *end;
        ao_global_dummy = &ao_global_dummy_storage;
        ao_global_dummy->funcs = &ao_dummy_funcs;

	/* Read config files */
	ao_read_config_files(&config);
        ao_global_load_options(ao_global_options);

	if (driver_head == NULL) {
		driver_head = _load_static_drivers(&end);
		_append_dynamic_drivers(end);
	}

	/* Create the table of driver info structs */
	info_table = _make_info_table(&driver_head, &driver_count);
}

void ao_shutdown(void)
{
	driver_list *driver = driver_head;
	driver_list *next_driver;

	if (!driver_head) return;

	free(info_table);
	info_table = NULL;

	/* unload and free all the drivers */
	while (driver) {
		if (driver->handle) {

		  dlclose(driver->handle);
		  free(driver->functions); /* DON'T FREE STATIC FUNC TABLES */
		}
		next_driver = driver->next;
		free(driver);
		driver = next_driver;
	}

        _clear_config();
	/* NULL out driver_head or ao_initialize() won't work */
	driver_head = NULL;
}


/* -- Device Setup/Playback/Teardown -- */
int ao_append_option(ao_option **options, const char *key, const char *value)
{
	ao_option *op, *list;

	op = calloc(1,sizeof(ao_option));
	if (op == NULL) return 0;

	op->key = _strdup(key);
	op->value = _strdup(value?value:"");
	op->next = NULL;

	if ((list = *options) != NULL) {
		list = *options;
		while (list->next != NULL) list = list->next;
		list->next = op;
	} else {
		*options = op;
	}


	return 1;
}

int ao_append_global_option(const char *key, const char *value)
{
  return ao_append_option(&ao_global_options,key,value);
}

void ao_free_options(ao_option *options)
{
	ao_option *rest;

	while (options != NULL) {
		rest = options->next;
		free(options->key);
		free(options->value);
		free(options);
		options = rest;
	}
}


ao_device *ao_open_live (int driver_id, ao_sample_format *format,
			ao_option *options)
{
	return _open_device(driver_id, format, options, NULL);
}


ao_device *ao_open_file (int driver_id, const char *filename, int overwrite,
			 ao_sample_format *format, ao_option *options)
{
	FILE *file;
	ao_device *device;

	if (strcmp("-", filename) == 0)
		file = stdout;
	else {

		if (!overwrite) {
			/* Test for file existence */
			file = fopen(filename, "r");
			if (file != NULL) {
				fclose(file);
				errno = AO_EFILEEXISTS;
				return NULL;
			}
		}


		file = fopen(filename, "w");
	}


	if (file == NULL) {
		errno = AO_EOPENFILE;
		return NULL;
	}

	device = _open_device(driver_id, format, options, file);

	if (device == NULL) {
		fclose(file);
		/* errno already set by _open_device() */
		return NULL;
	}

	return device;
}


int ao_play(ao_device *device, char* output_samples, uint_32 num_bytes)
{
	char *playback_buffer;

	if (device == NULL)
	  return 0;

	if (device->swap_buffer != NULL) {
          int out_bytes = num_bytes*device->output_channels/device->input_channels;
          if (_realloc_swap_buffer(device, out_bytes)) {
            int i;
            int swap = (device->bytewidth>1 &&
                        device->client_byte_format != device->driver_byte_format);
            for(i=0;i<device->output_channels;i++){
              int ic = device->inter_permute ? device->inter_permute[i] : i;
              if(ic==-1){
                _buffer_zero(device->swap_buffer,i,device->bytewidth,device->output_channels,
                             out_bytes);
              }else if(swap){
                _buffer_permute_swap(device->swap_buffer,i,device->bytewidth,device->output_channels,
                                     out_bytes, output_samples, ic, device->input_channels);
              }else{
                _buffer_permute(device->swap_buffer,i,device->bytewidth,device->output_channels,
                                out_bytes, output_samples, ic, device->input_channels);
              }
            }
            playback_buffer = device->swap_buffer;
            num_bytes = out_bytes;
          } else
            return 0; /* Could not expand swap buffer */
	} else
          playback_buffer = output_samples;

	return device->funcs->play(device, playback_buffer, num_bytes);
}


int ao_close(ao_device *device)
{
	int result;

	if (device == NULL)
		result = 0;
	else {
		result = device->funcs->close(device);
		device->funcs->device_clear(device);

		if (device->file) {
			fclose(device->file);
			device->file = NULL;
		}

		if (device->swap_buffer != NULL)
			free(device->swap_buffer);
                if (device->output_matrix != NULL)
                        free(device->output_matrix);
                if (device->input_map != NULL)
                        free(device->input_map);
                if (device->inter_matrix != NULL)
                        free(device->inter_matrix);
                if (device->inter_permute != NULL)
                        free(device->inter_permute);
		free(device);
	}

	return result;
}


/* -- Driver Information -- */

int ao_driver_id(const char *short_name)
{
	int i;
	driver_list *driver = driver_head;

	i = 0;
	while (driver) {
		if (strcmp(short_name,
			   driver->functions->driver_info()->short_name) == 0)
			return i;
		driver = driver->next;
		i++;
	}

	return -1; /* No driver by that name */
}


int ao_default_driver_id (void)
{
	/* Find the default driver in the list of loaded drivers */

	return _find_default_driver_id(config.default_driver);
}


ao_info *ao_driver_info(int driver_id)
{
	driver_list *driver;

	if ( (driver = _get_driver(driver_id)) ) {
		if (driver->functions->driver_info != NULL)
			return driver->functions->driver_info();
		else
			return NULL;
	} else
		return NULL;
}


ao_info **ao_driver_info_list(int *count)
{
	*count = driver_count;
	return info_table;
}

const char *ao_file_extension(int driver_id)
{
	driver_list *driver;

	if ( (driver = _get_driver(driver_id)) ) {
		if (driver->functions->file_extension != NULL)
			return driver->functions->file_extension();
		else
			return NULL;
	} else
		return NULL;
}

/* -- Miscellaneous -- */

/* Stolen from Vorbis' lib/vorbisfile.c */
int ao_is_big_endian(void)
{
	static uint_16 pattern = 0xbabe;
	return 0[(volatile unsigned char *)&pattern] == 0xba;
}
