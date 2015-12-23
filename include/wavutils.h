#ifndef _WAVUTILS_H_
#define _WAVUTILS_H_

#include <stdio.h>
#include "id3v1_1.h"

typedef struct wav_header
{
	char riffmarker[4];
	int32_t filesize;
	char wavemarker[4];
	char format_tag[4];
	int32_t format_length;
	int16_t format_type;
	int16_t num_channels;
	int32_t sample_rate;
	int32_t bytes_per_sec;
	int16_t bytes_by_capture;
	int16_t bits_per_sample;
	char data_header[4];
	int32_t data_size;
} wav_header;

enum wav_error
{
    ERR_NO_RIFF_MARKER,
    ERR_NO_WAVE_MARKER,
    ERR_NO_FMT_MARKER,
    ERR_NOT_A_PCM_FILE,
    READ_STATUS_OK,
    READ_STATUS_OK_WITH_META
};

int32_t check_wav_file(FILE *fp,int32_t * sample_rate,int16_t *channels,int16_t *bits_per_sample,id3v1_tag *tags);
void initialize_header(wav_header *hdr,int32_t channels,int32_t rate,int32_t bps);
void write_header(FILE *fp,wav_header *header);
void finalize_file(FILE *fp);

#endif
