#ifndef _ID3V11_H_
#define _ID3V11_H_

enum error_codes
{
	TAG_WRITE_OK,
	TAG_WRITE_ERROR,
	TAG_READ_OK,
	TAG_READ_ERROR
};

typedef struct id3v1_tag
{
	char id[3];
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	char comment[30];
	uint8_t genre_code; 
}id3v1_tag;

enum error_codes write_metadata(FILE *fp,int64_t position,id3v1_tag *tag_inst);

#endif