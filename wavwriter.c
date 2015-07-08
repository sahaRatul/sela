#include <stdio.h>
#include "wavwriter.h"

void initialize_header(wav_header *hdr,int channels,int rate,int bps)
{
	hdr->riffmarker[0]='R';
	hdr->riffmarker[1]='I';
	hdr->riffmarker[2]='F';
	hdr->riffmarker[3]='F';
	
	hdr->filesize=0;
	
	hdr->wavemarker[0]='W';
	hdr->wavemarker[1]='A';
	hdr->wavemarker[2]='V';
	hdr->wavemarker[3]='E';
	
	hdr->format_tag[0]='f';
	hdr->format_tag[1]='m';
	hdr->format_tag[2]='t';
	hdr->format_tag[3]=' ';
	
	hdr->format_length=16;
	hdr->format_type=1;
	hdr->num_channels=(short)channels;
	hdr->sample_rate=rate;
	hdr->bytes_per_sec=(rate*bps*channels)/8;
	hdr->bytes_by_capture=(bps*channels)/8;
	hdr->bits_per_sample=bps;
	
	hdr->data_header[0]='d';
	hdr->data_header[1]='a';
	hdr->data_header[2]='t';
	hdr->data_header[3]='a';
	
	hdr->data_size=0;
}

void write_header(FILE *fp,wav_header *header)
{
	fseek(fp,0,SEEK_SET);
	fwrite(header,sizeof(wav_header),1,fp);
}

void finalize_file(FILE *fp)
{
	int file_size=ftell(fp);
	fseek(fp,4,SEEK_SET);
	fwrite(&file_size,sizeof(int),1,fp);
	fseek(fp,40,SEEK_SET);
	int data_size=file_size-44;
	fwrite(&data_size,sizeof(int),1,fp);
}
