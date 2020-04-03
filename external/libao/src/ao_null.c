/*
 *
 *  ao_null.c
 *
 *      Original Copyright (C) Aaron Holtzman - May 1999
 *      Modifications Copyright (C) Stan Seibert - July 2000, July 2001
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

#include <stdio.h>
#include <string.h>
#include <ao/ao.h>

static char *ao_null_options[] = {
  "debug","verbose","matrix","quiet"
};
static ao_info ao_null_info = {
	AO_TYPE_LIVE,
	"Null output",
	"null",
	"Stan Seibert <volsung@asu.edu>",
	"This driver does nothing.",
	AO_FMT_NATIVE,
	0,
	ao_null_options,
        sizeof(ao_null_options)/sizeof(*ao_null_options)
};


typedef struct ao_null_internal {
	unsigned long byte_counter;
} ao_null_internal;


static int ao_null_test(void)
{
	return 1; /* Null always works */
}


static ao_info *ao_null_driver_info(void)
{
	return &ao_null_info;
}


static int ao_null_device_init(ao_device *device)
{
	ao_null_internal *internal;

	internal = (ao_null_internal *) malloc(sizeof(ao_null_internal));

	if (internal == NULL)
		return 0; /* Could not initialize device memory */

	internal->byte_counter = 0;

	device->internal = internal;
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

	return 1; /* Memory alloc successful */
}


static int ao_null_set_option(ao_device *device, const char *key,
			      const char *value)
{
	ao_null_internal *internal = (ao_null_internal *) device->internal;

	return 1;
}



static int ao_null_open(ao_device *device, ao_sample_format *format)
{
	/* Use whatever format the client requested */
	device->driver_byte_format = device->client_byte_format;

        if(!device->inter_matrix){
          /* by default, we want inter == in */
          if(format->matrix)
            device->inter_matrix = strdup(format->matrix);
        }

	return 1;
}


static int ao_null_play(ao_device *device, const char *output_samples,
			uint_32 num_bytes)
{
	ao_null_internal *internal = (ao_null_internal *)device->internal;

	internal->byte_counter += num_bytes;

	return 1;
}


static int ao_null_close(ao_device *device)
{
	ao_null_internal *internal = (ao_null_internal *) device->internal;

	adebug("%ld bytes sent to null device.\n", internal->byte_counter);

	return 1;
}


static void ao_null_device_clear(ao_device *device)
{
	ao_null_internal *internal = (ao_null_internal *) device->internal;

	free(internal);
        device->internal=NULL;
}


ao_functions ao_null = {
	ao_null_test,
	ao_null_driver_info,
	ao_null_device_init,
	ao_null_set_option,
	ao_null_open,
	ao_null_play,
	ao_null_close,
	ao_null_device_clear
};
