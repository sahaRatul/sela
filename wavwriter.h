#ifndef _WAVWRITER_H_
#define _WAVWRITER_H_

#include <stdio.h>

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
}wav_header;

void initialize_header(wav_header *hdr,int32_t channels,int32_t rate,int32_t bps);
void write_header(FILE *fp,wav_header *header);
void finalize_file(FILE *fp);

#endif
