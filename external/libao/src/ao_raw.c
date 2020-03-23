/*
 *
 *  ao_raw.c
 *
 *      Copyright (C) Stan Seibert - January 2001, July 2001
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
#include <errno.h>
#include <ao/ao.h>
#include <ao/plugin.h>

static char *ao_raw_options[] = {"byteorder","matrix","verbose","quiet","debug"};
static ao_info ao_raw_info =
{
	AO_TYPE_FILE,
	"RAW sample output",
	"raw",
	"Stan Seibert <indigo@aztec.asu.edu>",
	"Writes raw audio samples to a file",
	AO_FMT_NATIVE,
	0,
	ao_raw_options,
        sizeof(ao_raw_options)/sizeof(*ao_raw_options)
};

typedef struct ao_raw_internal
{
	int byte_order;
} ao_raw_internal;


static int ao_raw_test(void)
{
	return 1; /* Always works */
}


static ao_info *ao_raw_driver_info(void)
{
	return &ao_raw_info;
}


static int ao_raw_device_init(ao_device *device)
{
	ao_raw_internal *internal;

	internal = (ao_raw_internal *) malloc(sizeof(ao_raw_internal));

	if (internal == NULL)
		return 0; /* Could not initialize device memory */

	internal->byte_order = AO_FMT_NATIVE;

	device->internal = internal;
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

	return 1; /* Memory alloc successful */
}

static int ao_raw_set_option(ao_device *device, const char *key,
			      const char *value)
{
	ao_raw_internal *internal = (ao_raw_internal *)device->internal;

	if (!strcmp(key, "byteorder")) {
		if (!strcmp(value, "native"))
			internal->byte_order = AO_FMT_NATIVE;
		else if (!strcmp(value, "big"))
			internal->byte_order = AO_FMT_BIG;
		else if (!strcmp(value, "little"))
			internal->byte_order = AO_FMT_LITTLE;
		else
			return 0; /* Bad option value */
	}

	return 1;
}


static int ao_raw_open(ao_device *device, ao_sample_format *format)
{
	ao_raw_internal *internal = (ao_raw_internal *)device->internal;

	device->driver_byte_format = internal->byte_order;

        //if(!device->inter_matrix){
        ///* by default, inter == in */
        //if(format->matrix)
        //  device->inter_matrix = strdup(format->matrix);
        //}

	return 1;
}


/*
 * play the sample to the already opened file descriptor
 */
static int ao_raw_play(ao_device *device, const char *output_samples,
		       uint_32 num_bytes)
{
	if (fwrite(output_samples, sizeof(char), num_bytes,
		   device->file) < num_bytes)
		return 0;
	else
		return 1;
}


static int ao_raw_close(ao_device *device)
{
	/* No closeout needed */
	return 1;
}


static void ao_raw_device_clear(ao_device *device)
{
	ao_raw_internal *internal = (ao_raw_internal *) device->internal;

	free(internal);
        device->internal=NULL;
}

const char *ao_raw_file_extension(void)
{
	return "raw";
}


ao_functions ao_raw = {
	ao_raw_test,
	ao_raw_driver_info,
	ao_raw_device_init,
	ao_raw_set_option,
	ao_raw_open,
	ao_raw_play,
	ao_raw_close,
	ao_raw_device_clear,
	ao_raw_file_extension
};
