#ifndef _RICE_H_
#define _RICE_H_

#define MAX_RICE_PARAM 20

typedef struct rice_encode_context
{
	unsigned short buffer;
	unsigned short filled;
	unsigned int bits;
}rice_encode_context;

typedef struct rice_decode_context
{
	unsigned short buffer;
	unsigned short filled;
	unsigned int bytes;
}rice_decode_context;

int signed_to_unsigned(const unsigned int data_size,const short *input,unsigned short *output);
int unsigned_to_signed(const unsigned int data_size,const unsigned short *input,short *output);
unsigned short get_opt_rice_param(const unsigned short *data,int data_size,unsigned int *req_bits);
unsigned int rice_encode_block(short param,const unsigned short *input,int size,unsigned short *encoded);
unsigned int rice_decode_block(short param,const unsigned short *encoded,int count,unsigned short *decoded);

#endif
