#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "wavutils.h"

int check_wav_file(FILE *fp,int32_t * sample_rate,int16_t *channels,int16_t *bits_per_sample)
{
	int16_t fmt_type,bytes_by_capture;
	int32_t file_size,bytes_per_sec,list_size,data_size;
	int32_t fmt_length;
	size_t read;
	char riffmarker[4];
	char wavemarker[4];
	char formatmarker[4];
	char marker[4];

	fseek(fp,0,SEEK_SET);

	read = fread(riffmarker,sizeof(char),4,fp);
	if(strncmp(riffmarker,"RIFF",4))
		return ERR_NO_RIFF_MARKER;

	read = fread(&file_size,sizeof(int32_t),1,fp);

	read = fread(wavemarker,sizeof(char),4,fp);
	if(strncmp(wavemarker,"WAVE",4))
		return ERR_NO_WAVE_MARKER;

	//FORMAT chunk
	read = fread(formatmarker,sizeof(char),4,fp);
	if(strncmp(formatmarker,"fmt",3))
		return ERR_NO_FMT_MARKER;

	read = fread(&fmt_length,sizeof(int32_t),1,fp);
	read = fread(&fmt_type,sizeof(int16_t),1,fp);
	if(fmt_type != 1)
		return ERR_NOT_A_PCM_FILE;
	
	read = fread(channels,sizeof(int16_t),1,fp);
	read = fread(sample_rate,sizeof(int32_t),1,fp);
	read = fread(&bytes_per_sec,sizeof(int32_t),1,fp);
	read = fread(&bytes_by_capture,sizeof(int16_t),1,fp);
	read = fread(bits_per_sample,sizeof(int16_t),1,fp);

	//LIST/DATA chunk
	read = fread(marker,sizeof(char),4,fp);
	if(strncmp(marker,"LIST",4) == 0)
	{
		read = fread(&list_size,sizeof(int32_t),1,fp);
		fseek(fp,list_size,SEEK_CUR);//Skip list chunk
		read = fread(marker,sizeof(char),4,fp);
		if(strncmp(marker,"data",4) == 0)
			read = fread(&data_size,sizeof(int32_t),1,fp);
	}
	else
		read = fread(&data_size,sizeof(int32_t),1,fp);

	return READ_STATUS_OK;
}

void initialize_header(wav_header *hdr,int32_t channels,int32_t rate,int32_t bps)
{
	hdr->riffmarker[0] = 'R';
	hdr->riffmarker[1] = 'I';
	hdr->riffmarker[2] = 'F';
	hdr->riffmarker[3] = 'F';
	
	hdr->filesize = 0;
	
	hdr->wavemarker[0] = 'W';
	hdr->wavemarker[1] = 'A';
	hdr->wavemarker[2] = 'V';
	hdr->wavemarker[3] = 'E';
	
	hdr->format_tag[0] = 'f';
	hdr->format_tag[1] = 'm';
	hdr->format_tag[2] = 't';
	hdr->format_tag[3] = ' ';
	
	hdr->format_length = 16;
	hdr->format_type = 1;
	hdr->num_channels = (int16_t)channels;
	hdr->sample_rate = rate;
	hdr->bytes_per_sec = (rate * bps * channels)/8;
	hdr->bytes_by_capture = (bps * channels)/8;
	hdr->bits_per_sample = bps;
	
	hdr->data_header[0] = 'd';
	hdr->data_header[1] = 'a';
	hdr->data_header[2] = 't';
	hdr->data_header[3] = 'a';
	
	hdr->data_size=0;
}

void write_header(FILE *fp,wav_header *header)
{
	fseek(fp,0,SEEK_SET);
	size_t written = fwrite(header,sizeof(wav_header),1,fp);
}

void finalize_file(FILE *fp)
{
	size_t file_size = ftell(fp);
	fseek(fp,4,SEEK_SET);
	size_t written = fwrite(&file_size,sizeof(int32_t),1,fp);
	fseek(fp,40,SEEK_SET);
	size_t data_size = file_size - 44;
	written = fwrite(&data_size,sizeof(int32_t),1,fp);
}
