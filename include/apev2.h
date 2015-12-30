#ifndef _APEV2_H_
#define _APEV2_H_

enum ape_error_codes
{
	APE_STATE_NULL,
	APE_STATE_INIT_SUCCESS,
	APE_STATE_INIT_FAIL,
	APE_KEYS_INIT_SUCCESS,
	APE_KEYS_INIT_FAIL,
	APE_HEADER_INIT_SUCCESS,
	APE_HEADER_INIT_FAIL,
	APE_LIST_INIT_SUCCESS,
	APE_LIST_INIT_FAIL,
	APE_CONV_SUCCESS,
	APE_CONV_FAIL,
	APE_CONV_FAIL_NO_TAGS,
	APE_FINALIZE_HEADER_SUCCESS,
	APE_FINALIZE_HEADER_FAIL,
	APE_FILE_WRITE_SUCCESS,
	APE_FILE_WRITE_FAIL,
	APE_READ_SUCCESS,
	APE_READ_FAIL,
	APE_PRINT_SUCCESS,
	APE_PRINT_FAIL,
	APE_NO_DATA,
	APE_FREE_SUCCESS,
	APE_FREE_FAIL
};

enum ape_tag_items
{
	TITLE,
	ARTIST,
	ALBUM,
	YEAR,
	GENRE,
	COMMENT
};

typedef struct apev2_state
{
	int32_t ape_last_state;
	int32_t ape_init_state;
	int32_t key_init_state;
	int32_t header_init_state;
	int32_t list_init_state;
}apev2_state;

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

enum ape_error_codes 
init_apev2(apev2_state *state);

enum ape_error_codes 
init_apev2_keys(apev2_state *state,apev2_keys *inst);

enum ape_error_codes 
init_apev2_header(apev2_state *state,apev2_hdr_ftr *header);

enum ape_error_codes 
init_apev2_item_list(apev2_state *state,apev2_item_list *list);

enum ape_error_codes 
finalize_apev2_header(apev2_state *state,apev2_hdr_ftr *header,apev2_item_list *list);

enum ape_error_codes 
wav_to_apev2(apev2_state *state,wav_tags *tags,apev2_item_list *list,apev2_keys *key_inst);

enum ape_error_codes 
write_apev2_tags(apev2_state *state,FILE *file,size_t pos,apev2_hdr_ftr *header,apev2_item_list *list);

enum ape_error_codes 
print_apev2_tags(apev2_state *state,apev2_item_list *list);

enum ape_error_codes 
free_apev2_list(apev2_state *state,apev2_item_list *list);

enum ape_error_codes 
read_apev2_tags(apev2_state *state,char *data,int32_t data_size,apev2_keys *keys,apev2_hdr_ftr *header,apev2_item_list *list);

#endif