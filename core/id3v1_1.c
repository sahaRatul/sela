#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "id3v1_1.h"

enum error_codes write_metadata(FILE *fp,int64_t position,id3v1_tag *tag_inst)
{
	size_t written = 0;
	written += fwrite(tag_inst->id,sizeof(char),3,fp);
	written += fwrite(tag_inst->title,sizeof(char),30,fp);
	written += fwrite(tag_inst->artist,sizeof(char),30,fp);
	written += fwrite(tag_inst->album,sizeof(char),30,fp);
	written += fwrite(tag_inst->year,sizeof(char),4,fp);
	written += fwrite(tag_inst->comment,sizeof(char),30,fp);
	written += fwrite(&tag_inst->genre_code,sizeof(uint8_t),1,fp);

	if(written == 128)
		return TAG_WRITE_OK;
	return TAG_WRITE_ERROR;
}