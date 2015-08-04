#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct ape_item_keys
{
	const char title_key[5] = {'T','i','t','l','e'};
	const char artist_key[6] = {'A','r','t','i','s','t'};
	const char album_key[5] = {'A','l','b','u','m'};
	const char year_key[4] = {'Y','e','a','r'};
	const char genre_key[5] = {'G','e','n','r','e'};
	const char composer_key[8] = {'C','o','m','p','o','s','e','r'};
	const char publisher_key[9] = {'P','u','b','l','i','s','h','e','r'};
	const char comment_key[7] = {'C','o','m','m','e','n','t'};
}ape_item_keys;

typedef struct ape_header_footer
{
	const char tag_preamble[8] = {'A','P','E','T','A','G','E','X'};
	const char tag_version[4] = {'2','0','0','0'};
	int32_t item_count = 0;
	int32_t tag_flags = 0;
	const int64_t reserved = 0;
}ape_header_footer;

typedef struct ape_item
{
	int32_t item_size = 0;
	int32_t item_flags = 0;
	int32_t item_key_size = 0;
	char *item_key;
	const char null = 0x00;
	char *item_value;
}ape_item;

