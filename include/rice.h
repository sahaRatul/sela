#ifndef _RICE_H_
#define _RICE_H_

#define MAX_RICE_PARAM 20

typedef struct rice_encode_context
{
	uint32_t buffer;
	uint32_t filled;
	uint32_t bits;
} rice_encode_context;

typedef struct rice_decode_context
{
	uint32_t buffer;
	uint32_t filled;
	uint32_t bytes;
} rice_decode_context;

int32_t signed_to_unsigned(const uint32_t data_size,const int32_t *input,uint32_t *output);
int32_t unsigned_to_signed(const uint32_t data_size,const uint32_t *input,int32_t *output);
uint16_t get_opt_rice_param(const uint32_t *data,int32_t data_size,uint32_t *req_bits);
uint32_t rice_encode_block(int16_t param,const uint32_t *input,int32_t size,uint32_t *encoded);
uint32_t rice_decode_block(int16_t param,const uint32_t *encoded,int32_t count,uint32_t *decoded);

#endif
