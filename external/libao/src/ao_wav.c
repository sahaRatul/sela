/*
 *
 *  ao_wav.c
 *
 *      Original Copyright (C) Aaron Holtzman - May 1999
 *      Modifications Copyright (C) Stan Seibert - July 2000, July 2001
 *                    Copyright (C) Monty - January 2010
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
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ao/ao.h>

#define WAVE_FORMAT_PCM         0x0001
#define FORMAT_MULAW            0x0101
#define IBM_FORMAT_ALAW         0x0102
#define IBM_FORMAT_ADPCM        0x0103
#define WAVE_FORMAT_EXTENSIBLE  0xfffe

#define WAV_HEADER_LEN 68

#define WRITE_U32(buf, x) *(buf)     = (unsigned char)(x&0xff);\
						  *((buf)+1) = (unsigned char)((x>>8)&0xff);\
						  *((buf)+2) = (unsigned char)((x>>16)&0xff);\
						  *((buf)+3) = (unsigned char)((x>>24)&0xff);

#define WRITE_U16(buf, x) *(buf)     = (unsigned char)(x&0xff);\
						  *((buf)+1) = (unsigned char)((x>>8)&0xff);

#define DEFAULT_SWAP_BUFFER_SIZE 2048

struct riff_struct {
	unsigned char id[4];   /* RIFF */
	unsigned int len;
	unsigned char wave_id[4]; /* WAVE */
};


struct chunk_struct
{
	unsigned char id[4];
	unsigned int len;
};

struct common_struct
{
	unsigned short wFormatTag;
	unsigned short wChannels;
	unsigned int dwSamplesPerSec;
	unsigned int dwAvgBytesPerSec;
	unsigned short wBlockAlign;
	unsigned short wBitsPerSample;
        unsigned short cbSize;
	unsigned short wValidBitsPerSample;
        unsigned int   dwChannelMask;
        unsigned short subFormat;
};

struct wave_header
{
	struct riff_struct   riff;
	struct chunk_struct  format;
	struct common_struct common;
	struct chunk_struct  data;
};


static char *ao_wav_options[] = {"matrix","verbose","quiet","debug"};
static ao_info ao_wav_info =
{
	AO_TYPE_FILE,
	"WAV file output",
	"wav",
	"Aaron Holtzman <aholtzma@ess.engr.uvic.ca>",
	"Sends output to a .wav file",
	AO_FMT_LITTLE,
	0,
	ao_wav_options,
        sizeof(ao_wav_options)/sizeof(*ao_wav_options)
};

typedef struct ao_wav_internal
{
	struct wave_header wave;
} ao_wav_internal;


static int ao_wav_test(void)
{
	return 1; /* File driver always works */
}


static ao_info *ao_wav_driver_info(void)
{
	return &ao_wav_info;
}


static int ao_wav_device_init(ao_device *device)
{
	ao_wav_internal *internal;

	internal = (ao_wav_internal *) malloc(sizeof(ao_wav_internal));

	if (internal == NULL)
		return 0; /* Could not initialize device memory */

	memset(&(internal->wave), 0, sizeof(internal->wave));

	device->internal = internal;
        device->output_matrix = _strdup("L,R,C,LFE,BL,BR,CL,CR,BC,SL,SR");
        device->output_matrix_order = AO_OUTPUT_MATRIX_COLLAPSIBLE;

	return 1; /* Memory alloc successful */
}


static int ao_wav_set_option(ao_device *device, const char *key,
			     const char *value)
{
	return 1; /* No options! */
}

static int ao_wav_open(ao_device *device, ao_sample_format *format)
{
	ao_wav_internal *internal = (ao_wav_internal *) device->internal;
	unsigned char buf[WAV_HEADER_LEN];
	int size = 0x7fffffff; /* Use a bogus size initially */

	/* Store information */
	internal->wave.common.wChannels = device->output_channels;
	internal->wave.common.wBitsPerSample = ((format->bits+7)>>3)<<3;
	internal->wave.common.wValidBitsPerSample = format->bits;
	internal->wave.common.dwSamplesPerSec = format->rate;

	memset(buf, 0, WAV_HEADER_LEN);

	/* Fill out our wav-header with some information. */
	strncpy_s(internal->wave.riff.id, 4, "RIFF",4);
	internal->wave.riff.len = size - 8;
	strncpy_s(internal->wave.riff.wave_id, 4, "WAVE",4);

	strncpy_s(internal->wave.format.id, 4, "fmt ",4);
	internal->wave.format.len = 40;

	internal->wave.common.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	internal->wave.common.dwAvgBytesPerSec =
		internal->wave.common.wChannels *
		internal->wave.common.dwSamplesPerSec *
		(internal->wave.common.wBitsPerSample >> 3);

	internal->wave.common.wBlockAlign =
		internal->wave.common.wChannels *
		(internal->wave.common.wBitsPerSample >> 3);
	internal->wave.common.cbSize = 22;
	internal->wave.common.subFormat = WAVE_FORMAT_PCM;
        internal->wave.common.dwChannelMask=device->output_mask;

	strncpy_s(internal->wave.data.id, 4, "data",4);

	internal->wave.data.len = size - WAV_HEADER_LEN;

	strncpy_s(buf, 4, internal->wave.riff.id, 4);
	WRITE_U32(buf+4, internal->wave.riff.len);
	strncpy_s(buf+8, 4, internal->wave.riff.wave_id, 4);
	strncpy_s(buf+12, 4, internal->wave.format.id,4);
	WRITE_U32(buf+16, internal->wave.format.len);
	WRITE_U16(buf+20, internal->wave.common.wFormatTag);
	WRITE_U16(buf+22, internal->wave.common.wChannels);
	WRITE_U32(buf+24, internal->wave.common.dwSamplesPerSec);
	WRITE_U32(buf+28, internal->wave.common.dwAvgBytesPerSec);
	WRITE_U16(buf+32, internal->wave.common.wBlockAlign);
	WRITE_U16(buf+34, internal->wave.common.wBitsPerSample);
	WRITE_U16(buf+36, internal->wave.common.cbSize);
	WRITE_U16(buf+38, internal->wave.common.wValidBitsPerSample);
	WRITE_U32(buf+40, internal->wave.common.dwChannelMask);
	WRITE_U16(buf+44, internal->wave.common.subFormat);
        memcpy(buf+46,"\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71",14);
	strncpy_s(buf+60, 4, internal->wave.data.id, 4);
	WRITE_U32(buf+64, internal->wave.data.len);

	if (fwrite(buf, sizeof(char), WAV_HEADER_LEN, device->file)
	    != WAV_HEADER_LEN) {
		return 0; /* Could not write wav header */
	}

	device->driver_byte_format = AO_FMT_LITTLE;

	return 1;
}


/*
 * play the sample to the already opened file descriptor
 */
static int ao_wav_play(ao_device *device, const char *output_samples,
			uint_32 num_bytes)
{
	if (fwrite(output_samples, sizeof(char), num_bytes,
		   device->file) < num_bytes)
		return 0;
	else
		return 1;

}

static int ao_wav_close(ao_device *device)
{
	ao_wav_internal *internal = (ao_wav_internal *) device->internal;
	unsigned char buf[4];  /* For holding length values */

	long size;

	/* Find how long our file is in total, including header */
	size = ftell(device->file);

	if (size < 0) {
		return 0;  /* Wav header corrupt */
	}

	/* Go back and set correct length info */

	internal->wave.riff.len = size - 8;
	internal->wave.data.len = size - WAV_HEADER_LEN;

	/* Rewind to riff len and write it */
	if (fseek(device->file, 4, SEEK_SET) < 0)
		return 0; /* Wav header corrupt */

	WRITE_U32(buf, internal->wave.riff.len);
	if (fwrite(buf, sizeof(char), 4, device->file) < 4)
	  return 0; /* Wav header corrupt */


	/* Rewind to data len and write it */
	if (fseek(device->file, 64, SEEK_SET) < 0)
		return 0; /* Wav header corrupt */

	WRITE_U32(buf, internal->wave.data.len);
	if (fwrite(buf, sizeof(char), 4, device->file) < 4)
	  return 0; /* Wav header corrupt */


	return 1; /* Wav header correct */
}

static void ao_wav_device_clear(ao_device *device)
{
	ao_wav_internal *internal = (ao_wav_internal *) device->internal;

	free(internal);
        device->internal=NULL;
}

const char *ao_wav_file_extension(void)
{
	return "wav";
}


ao_functions ao_wav = {
	ao_wav_test,
	ao_wav_driver_info,
	ao_wav_device_init,
	ao_wav_set_option,
	ao_wav_open,
	ao_wav_play,
	ao_wav_close,
	ao_wav_device_clear,
	ao_wav_file_extension
};
