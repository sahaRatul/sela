#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "wavutils.h"
#include "apev2.h"

//--------init_state-------//<-- apev2_state
//             |
//             v
//---------init_keys-------//<-- apev2_state,apev2_keys
//             |
//             v
//--------init_header------//<-- apev2_state,apev2_header
//             |
//             v
//---------init_list-------//<-- apev2_state,apev2_item_list
//             |
//             v
//--------wav_to_apev2-----//<-- apev2_state,wav_tags,apev2_item_list,apev2_keys
//             |
//             v
//------finalize_header----//<-- apev2_state,apev2_header,apev2_item_list
//             |
//             v
//---------write_list------//<-- apev2_state,FILE,file_position,apev2_header,apev2_list
//             |
//             v
//---------free_list-------//<-- apev2_state,apev2_list

enum ape_error_codes 
init_apev2(apev2_state *state)
{
	if(state == NULL)
		return APE_STATE_NULL;

	state->ape_init_state = 1;
	state->key_init_state = 0;
	state->header_init_state = 0;
	state->list_init_state = 0;
	state->ape_last_state = APE_STATE_INIT_SUCCESS;

	return APE_STATE_INIT_SUCCESS;
}

enum ape_error_codes 
init_apev2_keys(apev2_state *state,apev2_keys *inst)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->ape_init_state != APE_STATE_INIT_SUCCESS)
		return APE_KEYS_INIT_FAIL;

	//Title key
	inst->title[0] = 'T';
	inst->title[1] = 'i';
	inst->title[2] = 't';
	inst->title[3] = 'l';
	inst->title[4] = 'e';

	//Artist key
	inst->artist[0] = 'A';
	inst->artist[1] = 'r';
	inst->artist[2] = 't';
	inst->artist[3] = 'i';
	inst->artist[4] = 's';
	inst->artist[5] = 't';

	//Album key
	inst->album[0] = 'A';
	inst->album[1] = 'l';
	inst->album[2] = 'b';
	inst->album[3] = 'u';
	inst->album[4] = 'm';

	//Year key
	inst->year[0] = 'Y';
	inst->year[1] = 'e';
	inst->year[2] = 'a';
	inst->year[3] = 'r';

	//Genre key
	inst->genre[0] = 'G';
	inst->genre[1] = 'e';
	inst->genre[2] = 'n';
	inst->genre[3] = 'r';
	inst->genre[4] = 'e';

	//Comment key
	inst->comment[0] = 'C';
	inst->comment[1] = 'o';
	inst->comment[2] = 'm';
	inst->comment[3] = 'm';
	inst->comment[4] = 'e';
	inst->comment[5] = 'n';
	inst->comment[6] = 't';

	state->ape_last_state = APE_KEYS_INIT_SUCCESS;
	return APE_KEYS_INIT_SUCCESS;
}

enum ape_error_codes 
init_apev2_header(apev2_state *state,apev2_hdr_ftr *header)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->ape_last_state != APE_KEYS_INIT_SUCCESS)
		return APE_HEADER_INIT_FAIL;

	header->preamble[0] = 'A';
	header->preamble[1] = 'P';
	header->preamble[2] = 'E';
	header->preamble[3] = 'T';
	header->preamble[4] = 'A';
	header->preamble[5] = 'G';
	header->preamble[6] = 'E';
	header->preamble[7] = 'X';

	header->version_number = 2000;//For apev2

	header->flags = 0b11100000000000000000000000000000;

	for(int32_t i = 0; i < 8; i++)
		header->reserved[i] = 0;

	state->ape_last_state = APE_HEADER_INIT_SUCCESS;
	return APE_HEADER_INIT_SUCCESS;
}

enum ape_error_codes 
init_apev2_item_list(apev2_state *state,apev2_item_list *list)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->ape_last_state != APE_HEADER_INIT_SUCCESS)
		return APE_LIST_INIT_FAIL;

	list->first = NULL;
	list->last = NULL;
	list->item_count = 0;

	state->ape_last_state = APE_LIST_INIT_SUCCESS;
	return APE_LIST_INIT_SUCCESS;
}

enum ape_error_codes 
finalize_apev2_header(apev2_state *state,apev2_hdr_ftr *header,apev2_item_list *list)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->ape_last_state != APE_CONV_SUCCESS)
		return APE_FINALIZE_HEADER_FAIL;

	apev2_tag_item *temp = NULL;
	temp = list->first;
	header->item_count = 0;

	//Count actual number of items present in list
	while(temp != list->last)
	{
		if(temp->val_len == 0)
			temp = temp->next;
		else
		{
			header->item_count++;
			temp = temp->next;
		}
	}

	//Calculate total bytes in tags
	//Fixed size(val_len(4) + flags(4) + terminator(1) = 9)
	header->tag_size = header->item_count * 9;
	
	//Variable size
	temp = list->first;
	while(temp != list->last)
	{
		if(temp->val_len == 0)
			temp = temp->next;
		else
		{
			header->tag_size += temp->key_size;
			header->tag_size += temp->val_len;
			temp = temp->next;
		}
	}

	state->ape_last_state = APE_FINALIZE_HEADER_SUCCESS;
	return APE_FINALIZE_HEADER_SUCCESS;
}

int32_t write_apev2_item(apev2_keys *key_inst,apev2_tag_item *tag_item,char *data,int32_t data_size,int32_t item_code)
{
	tag_item->val_len = data_size;
	tag_item->terminator = '\0';
	tag_item->item_val = data;
	tag_item->flags = 0b11100000000000000000000000000000;

	switch(item_code)
	{
		case TITLE:
			tag_item->item_key = key_inst->title;
			tag_item->key_size = 5;
			break;
		case ARTIST:
			tag_item->item_key = key_inst->artist;
			tag_item->key_size = 6;
			break;
		case ALBUM:
			tag_item->item_key = key_inst->album;
			tag_item->key_size = 5;
			break;
		case YEAR:
			tag_item->item_key = key_inst->year;
			tag_item->key_size = 4;
			break;
		case GENRE:
			tag_item->item_key = key_inst->genre;
			tag_item->key_size = 5;
			break;
		case COMMENT:
			tag_item->item_key = key_inst->comment;
			tag_item->key_size = 7;
			break;
		default:
			break;
	}

	return 0;
}

enum ape_error_codes 
wav_to_apev2(apev2_state *state,wav_tags *tags,apev2_item_list *list,apev2_keys *key_inst)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->ape_last_state != APE_LIST_INIT_SUCCESS)
		return APE_CONV_FAIL;

	int32_t num_tags = 0;
	int32_t max_tags = 6; //(Max tags supported = 6)

	if(tags->title_present)
		num_tags++;
	if(tags->artist_present)
		num_tags++;
	if(tags->album_present)
		num_tags++;
	if(tags->genre_present)
		num_tags++;
	if(tags->year_present)
		num_tags++;
	if(tags->comment_present)
		num_tags++;

	if(num_tags == 0)
		return APE_CONV_FAIL_NO_TAGS;

	//Generate apev2 link list with 'max_tags' number of items
	for(int32_t i = 0; i < max_tags; i++)
	{
		apev2_tag_item *item = (apev2_tag_item *)malloc(sizeof(apev2_tag_item));

		if(list->first == NULL)
		{
			list->first = item;
			list->last = item;
			list->item_count++;
		}
		else
		{
			list->last->next = item;
			list->last = list->last->next;
			list->item_count++;
		}

		item->next = NULL;
	}

	//Title
	apev2_tag_item *temp = list->first;
	write_apev2_item(key_inst,temp,tags->title,tags->title_size,TITLE);

	//Artist
	temp = temp->next;
	write_apev2_item(key_inst,temp,tags->artist,tags->artist_size,ARTIST);

	//Album
	temp = temp->next;
	write_apev2_item(key_inst,temp,tags->album,tags->album_size,ALBUM);

	//Genre
	temp = temp->next;
	write_apev2_item(key_inst,temp,tags->genre,tags->genre_size,GENRE);
	
	//Year
	temp = temp->next;
	write_apev2_item(key_inst,temp,tags->year,tags->year_size,YEAR);

	//Comment
	temp = temp->next;
	write_apev2_item(key_inst,temp,tags->comment,tags->comment_size,COMMENT);

	assert(temp == list->last);

	state->ape_last_state = APE_CONV_SUCCESS;
	return APE_CONV_SUCCESS;
}

enum ape_error_codes 
write_apev2_tags(apev2_state *state,FILE *file,size_t pos,apev2_hdr_ftr *header,apev2_item_list *list)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->ape_last_state != APE_FINALIZE_HEADER_SUCCESS)
		return APE_FILE_WRITE_FAIL;

	size_t written;
	apev2_tag_item *temp = NULL;

	fseek(file,pos,SEEK_SET);

	//Calculate number of valid tags (<= 5)
	temp = list->first;
	header->item_count = 0;
	while(temp != list->last)
	{
		if(temp->val_len == 0)
			temp = temp->next;
		else
		{
			header->item_count++;
			temp = temp->next;
		}
	}

	//Calculate total bytes in tags
	//Fixed size(val_len(4) + flags(4) + terminator(1) = 9)
	header->tag_size = header->item_count * 9;
	
	//Variable size
	temp = list->first;
	while(temp != list->last)
	{
		if(temp->val_len == 0)
			temp = temp->next;
		else
		{
			header->tag_size += temp->key_size;
			header->tag_size += temp->val_len;
			temp = temp->next;
		}
	}

	//Write header
	{
		written = fwrite(header->preamble,sizeof(char),8,file);
		written = fwrite(&header->version_number,sizeof(int32_t),1,file);
		written = fwrite(&header->tag_size,sizeof(uint32_t),1,file);
		written = fwrite(&header->item_count,sizeof(uint32_t),1,file);
		written = fwrite(&header->flags,sizeof(uint32_t),1,file);
		written = fwrite(header->reserved,sizeof(char),8,file);
	}

	//Write tags
	{
		temp = list->first;
		while(temp != list->last)
		{
			if(temp->val_len == 0)
			{
				temp = temp->next;
				continue;//Skip writing empty tag
			}

			written = fwrite(&temp->val_len,sizeof(int32_t),1,file);
			written = fwrite(&temp->flags,sizeof(int32_t),1,file);
			written = fwrite(temp->item_key,sizeof(char),(size_t)temp->key_size,file);
			written = fwrite(&temp->terminator,sizeof(int8_t),1,file);
			written = fwrite(temp->item_val,sizeof(char),(size_t)temp->val_len,file);

			//Move to next tag
			temp = temp->next;
		}
	}

	(void)written;
	temp = NULL;
	
	state->ape_last_state = APE_FILE_WRITE_SUCCESS;
	return APE_FILE_WRITE_SUCCESS;
}

enum ape_error_codes 
read_apev2_tags(apev2_state *state,char *data,int32_t data_size,apev2_keys *keys,apev2_hdr_ftr *header,apev2_item_list *list)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->ape_last_state != APE_LIST_INIT_SUCCESS)
		return APE_READ_FAIL;

	//Read tags
	{
		//Match header id
		if(strncmp(data,"APETAGEX",8) == 0)
			strncpy(header->preamble,data,8);
		else
			return -1;

		//Match version
		if(*((int32_t *)(data + 8)) == 2000)
			header->version_number = *((int32_t *)(data + 8));
		else
			return -1;

		//Tag size
		header->tag_size = *((uint32_t *)(data + 12));
		if(header->tag_size != (data_size - 32))
			return -1;

		//Tag items count
		header->item_count = *((uint32_t *)(data + 16));

		//Flags
		header->flags = *((uint32_t *)(data + 20));

		//Reserved
		strncpy(header->reserved,(data + 24),8);
		if(strncmp(header->reserved,"\0\0\0\0\0\0\0\0",8) != 0)
			return -1;//Some weird shit happened
	}

	uint32_t curr_index = 32;
	//Now read the tags
	//(Absolutely zero error handling here. Proceed with caution!!)
	{
		list->item_count = 0;//Just in case
		for(int32_t i = 0; i < header->item_count; i++)
		{
			apev2_tag_item *tag = (apev2_tag_item *)malloc(sizeof(apev2_tag_item));
			if(list->first == NULL)
			{
				list->first = tag;
				list->last = tag;
				tag->next = NULL;
				list->item_count++;
			}
			else
			{
				list->last->next = tag;
				list->last = list->last->next;
				list->item_count++;
			}

			tag->val_len = *(data + curr_index);
			curr_index += 4;
			tag->flags = *(data + curr_index);
			curr_index += 4;
			tag->item_key = (data + curr_index);
			int32_t j = 0;

			while(*(data + curr_index + j) != '\0')
				j++;

			tag->key_size = j;
			curr_index += ++j;
			tag->item_val = (data + curr_index);
			curr_index += tag->val_len;
		}
	}

	return APE_READ_SUCCESS;
}

enum ape_error_codes 
print_apev2_tags(apev2_state *state,apev2_item_list *list)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->ape_init_state != 1)
		return APE_PRINT_FAIL;
	if(list->item_count <= 0)
		return APE_NO_DATA;

	//int32_t val_len = 0;
	int32_t tag_count = list->item_count;

	apev2_tag_item *temp = list->first;
	while(tag_count != 0)
	{
		if(strncmp(temp->item_key,"Title",5) == 0)
			fprintf(stderr,"Title : %s\n",temp->item_val);
		else if(strncmp(temp->item_key,"Artist",6) == 0)
			fprintf(stderr,"Artist : %s\n",temp->item_val);
		else if(strncmp(temp->item_key,"Album",5) == 0)
			fprintf(stderr,"Album : %s\n",temp->item_val);
		else if(strncmp(temp->item_key,"Genre",5) == 0)
			fprintf(stderr,"Genre : %s\n",temp->item_val);
		else if(strncmp(temp->item_key,"Year",4) == 0)
			fprintf(stderr,"Year : %d\n",atoi(temp->item_val));
		else if(strncmp(temp->item_key,"Comment",7) == 0)
			fprintf(stderr,"Comment : %s\n",temp->item_val);

		temp = temp->next;
		tag_count--;
	}

	return APE_PRINT_SUCCESS;
}

enum ape_error_codes 
free_apev2_list(apev2_state *state,apev2_item_list *list)
{
	if(state == NULL)
		return APE_STATE_NULL;
	if(state->list_init_state == 0)
		return APE_FREE_FAIL;
	if(list->item_count <= 0)
		return APE_NO_DATA;

	apev2_tag_item *temp = list->first;
	apev2_tag_item *x = NULL;

	while(list->item_count != 0)
	{
		x = temp;
		temp = temp->next;
		free(x);
		list->item_count--;
	}

	return APE_FREE_SUCCESS;
}