/*
 *
 *  ao_au.c
 *
 *      Copyright (C) Wil Mahan - May 2001
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifndef _MSC_VER
# include <unistd.h>
#endif
#include <ao/ao.h>
#include <ao/plugin.h>

#define AUDIO_FILE_MAGIC ((uint_32)0x2e736e64)  /* ".snd" */

#define AUDIO_UNKNOWN_SIZE (~0) /* (unsigned) -1 */

/* Format codes (not comprehensive) */
#define AUDIO_FILE_ENCODING_LINEAR_8 (2) /* 8-bit linear PCM */
#define AUDIO_FILE_ENCODING_LINEAR_16 (3) /* 16-bit linear PCM */

#define AU_HEADER_LEN (28)

#define DEFAULT_SWAP_BUFFER_SIZE 2048

/* Write a uint_32 in big-endian order. */
#define WRITE_U32(buf, x) \
	*(buf) = (unsigned char)(((x)>>24)&0xff);\
	*((buf)+1) = (unsigned char)(((x)>>16)&0xff);\
	*((buf)+2) = (unsigned char)(((x)>>8)&0xff);\
	*((buf)+3) = (unsigned char)((x)&0xff);

typedef struct Audio_filehdr {
	uint_32 magic; /* magic number */
	uint_32 hdr_size; /* offset of the data */
	uint_32 data_size; /* length of data (optional) */
	uint_32 encoding; /* data format code */
	uint_32 sample_rate; /* samples per second */
	uint_32 channels; /* number of interleaved channels */
	char info[4]; /* optional text information */
} Audio_filehdr;

static char *ao_au_options[] = {"matrix","verbose","quiet","debug"};
static ao_info ao_au_info =
{
	AO_TYPE_FILE,
	"AU file output",
	"au",
	"Wil Mahan <wtm2@duke.edu>",
	"Sends output to a .au file",
	AO_FMT_BIG,
	0,
	ao_au_options,
        sizeof(ao_au_options)/sizeof(*ao_au_options)
};

typedef struct ao_au_internal
{
	Audio_filehdr au;
} ao_au_internal;


static int ao_au_test(void)
{
	return 1; /* File driver always works */
}


static ao_info *ao_au_driver_info(void)
{
	return &ao_au_info;
}


static int ao_au_device_init(ao_device *device)
{
	ao_au_internal *internal;

	internal = (ao_au_internal *) malloc(sizeof(ao_au_internal));

	if (internal == NULL)
		return 0; /* Could not initialize device memory */

	memset(&(internal->au), 0, sizeof(internal->au));

	device->internal = internal;
        device->output_matrix_order = AO_OUTPUT_MATRIX_FIXED;

	return 1; /* Memory alloc successful */
}


static int ao_au_set_option(ao_device *device, const char *key,
			    const char *value)
{
	return 1; /* No options! */
}


static int ao_au_open(ao_device *device, ao_sample_format *format)
{
	ao_au_internal *internal = (ao_au_internal *) device->internal;
	unsigned char buf[AU_HEADER_LEN];

	/* The AU format is big-endian */
	device->driver_byte_format = AO_FMT_BIG;

	/* Fill out the header */
	internal->au.magic = AUDIO_FILE_MAGIC;
	internal->au.channels = device->output_channels;
	if (format->bits == 8)
		internal->au.encoding = AUDIO_FILE_ENCODING_LINEAR_8;
	else if (format->bits == 16)
		internal->au.encoding = AUDIO_FILE_ENCODING_LINEAR_16;
	else {
		/* Only 8 and 16 bits are supported at present. */
		return 0;
	}
	internal->au.sample_rate = format->rate;
	internal->au.hdr_size = AU_HEADER_LEN;

	/* From the AU specification:  "When audio files are passed
	 * through pipes, the 'data_size' field may not be known in
	 * advance.  In such cases, the 'data_size' should be set
	 * to AUDIO_UNKNOWN_SIZE."
	 */
	internal->au.data_size = AUDIO_UNKNOWN_SIZE;
	/* strncpy(state->au.info, "OGG ", 4); */

	/* Write the header in big-endian order */
	WRITE_U32(buf, internal->au.magic);
	WRITE_U32(buf + 4, internal->au.hdr_size);
	WRITE_U32(buf + 8, internal->au.data_size);
	WRITE_U32(buf + 12, internal->au.encoding);
	WRITE_U32(buf + 16, internal->au.sample_rate);
	WRITE_U32(buf + 20, internal->au.channels);
	strncpy_s (buf + 24, 4, internal->au.info, 4);

	if (fwrite(buf, sizeof(char), AU_HEADER_LEN, device->file)
	    != AU_HEADER_LEN) {
		return 0; /* Error writing header */
	}

        if(!device->inter_matrix){
          /* set up matrix such that users are warned about > stereo playback */
          if(device->output_channels<=2)
            device->inter_matrix=_strdup("L,R");
          //else no matrix, which results in a warning
        }


	return 1;
}


/*
 * play the sample to the already opened file descriptor
 */
static int ao_au_play(ao_device *device, const char *output_samples,
		       uint_32 num_bytes)
{
	if (fwrite(output_samples, sizeof(char), num_bytes,
		   device->file) < num_bytes)
		return 0;
	else
		return 1;
}

static int ao_au_close(ao_device *device)
{
	ao_au_internal *internal = (ao_au_internal *) device->internal;

        off_t size;
	unsigned char buf[4];

	/* Try to find the total file length, including header */
	size = ftell(device->file);

	/* It's not a problem if the lseek() fails; the AU
	 * format does not require a file length.  This is
	 * useful for writing to non-seekable files (e.g.
	 * pipes).
	 */
	if (size > 0) {
		internal->au.data_size = size - AU_HEADER_LEN;

		/* Rewind the file */
		if (fseek(device->file, 8 /* offset of data_size */,
					SEEK_SET) < 0)
		{
			return 1; /* Seek failed; that's okay  */
		}

		/* Fill in the file length */
		WRITE_U32 (buf, internal->au.data_size);
		if (fwrite(buf, sizeof(char), 4, device->file) < 4) {
			return 1; /* Header write failed; that's okay */
		}
	}

	return 1;
}


static void ao_au_device_clear(ao_device *device)
{
	ao_au_internal *internal = (ao_au_internal *) device->internal;

	free(internal);
        device->internal=NULL;
}

const char *ao_au_file_extension(void)
{
	return "au";
}


ao_functions ao_au = {
	ao_au_test,
	ao_au_driver_info,
	ao_au_device_init,
	ao_au_set_option,
	ao_au_open,
	ao_au_play,
	ao_au_close,
	ao_au_device_clear,
	ao_au_file_extension
};
