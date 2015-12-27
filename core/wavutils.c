#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "id3v1_1.h"
#include "wavutils.h"

int32_t check_wav_file
(	
	FILE *fp,
	int32_t * sample_rate,
	int16_t *channels,
	int16_t *bits_per_sample,
	uint32_t *data_size,
	wav_tags *tags
)
{
	int16_t fmt_type,bytes_by_capture,meta_found = 0;
	int32_t file_size,bytes_per_sec,list_size;
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

	//LIST chunk (metadata)
	read = fread(marker,sizeof(char),4,fp);
	if(strncmp(marker,"LIST",4) == 0)
	{
		char tag[200];
		int32_t temp = 0,size = 0;
		read = fread(&list_size,sizeof(int32_t),1,fp);

		temp += fread(marker,sizeof(char),4,fp);
		if(strncmp(marker,"INFO",4) == 0)
		{
			meta_found = 1;
			//strncpy(tags->id,"TAG",3);

			//Inner tag list. id3v1 tags are filled here
			while(temp < list_size)
			{
				temp += fread(marker,sizeof(char),4,fp);
				temp += fread(&size,sizeof(char),4,fp);
				temp += fread(tag,sizeof(char),size,fp);

				if(strncmp(marker,"IART",4) == 0)//Artist
				{
					tags->artist_present = 1;
					tags->artist_size = size;
					tags->artist = (char *)malloc(sizeof(char) * size);
					strncpy(tags->artist,tag,(size_t)size);
				}
				else if(strncmp(marker,"ICRD",4) == 0)//Date
				{
					tags->year_present = 1;
					tags->year_size = size;
					tags->year = (char *)malloc(sizeof(char) * size);
					strncpy(tags->year,tag,(size_t)size);
				}
				else if(strncmp(marker,"INAM",4) == 0)//Title
				{
					tags->title_present = 1;
					tags->title_size = size;
					tags->title = (char *)malloc(sizeof(char) * size);
					strncpy(tags->title,tag,(size_t)size);
				}	
				else if(strncmp(marker,"IPRD",4) == 0)//Album
				{
					tags->album_present = 1;
					tags->album_size = size;
					tags->album = (char *)malloc(sizeof(char) * size);
					strncpy(tags->album,tag,size);
				}
				else if(strncmp(marker,"IGNR",4) == 0)//Genre
				{
					tags->genre_present = 1;
					tags->genre_size = size;
					tags->genre = (char *)malloc(sizeof(char) * size);
					strncpy(tags->genre,tag,size);
				}
				else if(strncmp(marker,"ICMT",4) == 0)//Comment
				{
					tags->comment_present = 1;
					tags->comment_size = size;
					tags->comment = (char *)malloc(sizeof(char) * size);
					strncpy(tags->comment,tag,size);
				}

				while(fgetc(fp) == '\0')//FFmpeg Bug
					temp++;
				fseek(fp,-1,SEEK_CUR);
			}
		}
		else
			fseek(fp,(list_size - 4),SEEK_CUR);
	}
	
	//Data chunk
	read = fread(marker,sizeof(char),4,fp);
	if(strncmp(marker,"data",4) == 0)
		read = fread(data_size,sizeof(int32_t),1,fp);

	(void)read;
	if(meta_found == 1)
		return READ_STATUS_OK_WITH_META;
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

	hdr->data_size = 0;

	return;
}

void init_wav_tags(wav_tags *tags)
{
	tags->title = NULL;
	tags->title_present = 0;
	tags->title_size = 0;

	tags->artist = NULL;
	tags->artist_present = 0;
	tags->artist_size = 0;

	tags->album = NULL;
	tags->album_present = 0;
	tags->album_size = 0;

	tags->genre = NULL;
	tags->genre_present = 0;
	tags->genre_size = 0;

	tags->year = NULL;
	tags->year_present = 0;
	tags->year_size = 0;

	tags->comment = NULL;
	tags->comment_present = 0;
	tags->comment_size = 0;

	return;
}

void destroy_wav_tags(wav_tags *tags)
{
	if(tags->title_present)
		free(tags->title);
	if(tags->artist_present)
		free(tags->artist);
	if(tags->album_present)
		free(tags->album);
	if(tags->genre_present)
		free(tags->genre);
	if(tags->year_present)
		free(tags->year);
	if(tags->comment_present)
		free(tags->comment);

	return;
}

void write_header(FILE *fp,wav_header *header)
{
	fseek(fp,0,SEEK_SET);
	size_t written = fwrite(header,sizeof(wav_header),1,fp);
	(void)written;
	return;
}

void finalize_file(FILE *fp)
{
	size_t file_size = ftell(fp);
	size_t riff_data_size = file_size - 8;
	fseek(fp,4,SEEK_SET);
	size_t written = fwrite(&riff_data_size,sizeof(int32_t),1,fp);
	fseek(fp,40,SEEK_SET);
	size_t data_size = file_size - 44;
	written = fwrite(&data_size,sizeof(int32_t),1,fp);
	(void)written;
	return;
}
