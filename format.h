#ifndef _FORMAT_H_
#define _FORMAT_H_

#include <stdint.h>

typedef struct audio_format
{
	int32_t num_channels;
	int32_t sample_rate;
	int32_t bits_per_sample;
} audio_format;

#endif
