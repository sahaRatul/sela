#ifndef _APEV2_H_
#define _APEV2_H_

#include "wavutils.h"

enum tag_items
{
	TITLE,
	ARTIST,
	ALBUM,
	YEAR,
	GENRE,
	COMMENT
};

typedef struct apev2_keys
{
	char title[5];
	char artist[6];
	char album[6];
	char year[4];
	char genre[5];
	char comment[6];
}apev2_keys;

typedef struct apev2_hdr_ftr
{
	char preamble[8];
	int32_t version_number;
	uint32_t tag_size;
	uint32_t item_count;
	int32_t flags;
	char reserved[8];
}apev2_hdr_ftr;

typedef struct apev2_tag_item
{
	int32_t val_len;
	int32_t flags;
	char *item_key;
	int32_t key_size;//Not to be written in final tag
	char terminator;
	char *item_val;
	struct apev2_tag_item *next;//Next tag 
}apev2_tag_item;

typedef struct apev2_item_list
{
	apev2_tag_item *first;
	apev2_tag_item *last;
	int32_t item_count;
}apev2_item_list;

void init_apev2_keys(apev2_keys *inst);
void init_apev2_header(apev2_hdr_ftr *header);
void init_apev2_item_list(apev2_item_list *list);
void finalize_apev2_header(apev2_hdr_ftr *header,apev2_item_list *list);
void print_apev2_tags(apev2_item_list *list);
void free_apev2_list(apev2_item_list *list);
int32_t write_apev2_item(apev2_keys *key_inst,apev2_tag_item *tag_item,char *data,int32_t data_size,int32_t item_code);
int32_t wav_to_apev2(wav_tags *tags, apev2_item_list *list,apev2_keys *key_inst);
int32_t write_apev2_tags(FILE *file,size_t pos,apev2_hdr_ftr *header,apev2_item_list *list);
int32_t read_apev2_tags(char *data,int32_t data_size,apev2_keys *keys,apev2_hdr_ftr *header,apev2_item_list *list);

#endif