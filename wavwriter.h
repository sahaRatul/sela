#ifndef _WAVWRITER_H_
#define _WAVWRITER_H_

#include <stdio.h>
typedef struct wav_header
{
	char riffmarker[4];
	int filesize;
	char wavemarker[4];
	char format_tag[4];
	int format_length;
	short format_type;
	short num_channels;
	int sample_rate;
	int bytes_per_sec;
	short bytes_by_capture;
	short bits_per_sample;
	char data_header[4];
	int data_size;
}wav_header;

void initialize_header(wav_header *hdr,int channels,int rate,int bps);
void write_header(FILE *fp,wav_header *header);
void finalize_file(FILE *fp);

#endif
