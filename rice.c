#include <stdio.h>
#include <malloc.h>

#include "rice.h"

/*signed data to unsigned data*/
int signed_to_unsigned(const unsigned int data_size,const short *input,unsigned short *output)
{
	unsigned int i;
	for(i = 0; i < data_size; i++)
	{
		if(input[i] < 0)
			output[i] = (-2 * input[i]) - 1;
		if(input[i] >= 0)
			output[i] = 2 * input[i];
	}
	return(0);
}

/*unsigned data to signed data*/
int unsigned_to_signed(const unsigned int data_size,const unsigned short *input,short *output)
{
	unsigned int i;
	for(i = 0; i < data_size; i++)
	{
		if((input[i] & 0x01) == 0x01)//Odd number
			output[i] = -((input[i] + 1) >> 1);
		if((input[i] & 0x01) == 0x00)//Even number
			output[i] = input[i] >> 1;
	}
	return(0);
}

/*calculate optimum rice parameter for encoding a block of data*/
unsigned short get_opt_rice_param(const unsigned short *data,int data_size,unsigned int *req_bits)
{
	int i = 0,j;
	unsigned int bits[MAX_RICE_PARAM];
	unsigned int temp;
	unsigned short best_index;

	for(i = 0; i < MAX_RICE_PARAM; i++)
	{
		temp = 0;
		for(j = 0; j < data_size; j++)
		{
			/*data[j]/2^k;*/
			temp += data[j] >> i;
			temp += 1;
			temp += i;
		}
		bits[i] = temp;
	}

	//Get the minimum parameter
	temp = bits[0];
	best_index = 0;
	*req_bits = bits[0];

	for(i = 1; i < MAX_RICE_PARAM; i++)
	{
		if(bits[i] < temp)
		{
			temp = bits[i];
			*req_bits = bits[i];
			best_index = (short)i;
		}
	}
	return best_index;
}

/*Encode a block of data using Golomb-Rice coding*/
unsigned int rice_encode_block(short param,const unsigned short *input,int size,unsigned short *encoded)
{
	int i,j,temp,written = 0;
	unsigned int bits;

	rice_encode_context context;
	rice_encode_context *ctx = &context;

	ctx->buffer = 0;
	ctx->filled = 0;
	ctx->bits = 0;

	for(i = 0; i < size; i++)
	{
		//temp = floor(input[i]/2^param)
		temp = input[i] >> param;

		//Write out 'temp' number of ones
		for(j = 0; j < temp; j++)
		{
			ctx->buffer = ctx->buffer | (0x0001 << ctx->filled);
			ctx->bits++;
			if(ctx->filled == 15)
			{
				encoded[written++] = ctx->buffer;
				ctx->buffer = 0;
				ctx->filled = 0;
			}
			else
				ctx->filled++;
		}

		//Write out a zero bit
		ctx->buffer = ctx->buffer | (0x0000 << ctx->filled);
		ctx->bits++;
		if(ctx->filled == 15)
		{
			encoded[written++] = ctx->buffer;
			ctx->buffer = 0;
			ctx->filled = 0;
		}
		else
			ctx->filled++;

		//Write out the last 'param' bits of input[i]
		for (j = param - 1; j >= 0; j--)
		{
			ctx->buffer = ctx->buffer | (((input[i] >> j) & 0x0001) << ctx->filled);
			ctx->bits++;
			if(ctx->filled == 15)
			{
				encoded[written++] = ctx->buffer;
				ctx->buffer = 0;
				ctx->filled = 0;
			}
			else
				ctx->filled++;
		}
	}

	//Flush buffer if not empty
	if(ctx->filled != 0)
		encoded[written] = ctx->buffer;

	bits = ctx->bits;
	return(bits);
}

/*Decode a block of data encoded using Golomb-Rice coding*/
unsigned int rice_decode_block(short param,const unsigned short *encoded,int count,unsigned short *decoded)
{
	int i = 0,j = 0,q,k = 0;
	short tmp;
	rice_decode_context context;
	rice_decode_context *ctx = &context;
	ctx->filled = 0;

	while(k < count)
	{
		//Count ones until a zero is encountered
		q = 0;
		while(1)
		{
			if(ctx->filled == 0)
			{
				ctx->filled = 16;
				ctx->buffer = encoded[i++];
			}
			tmp = ctx->buffer & 0x0001;
			ctx->buffer = ctx->buffer >> 1;
			ctx->filled--;
			if(tmp == 0)
				break;
			q++;
		}
		decoded[k] = q << param; //x = q * 2^k

		//Read out the last 'param' bits of input
		for(j = param - 1; j >= 0; j--)
		{
			if(ctx->filled == 0)
			{
				ctx->filled = 16;
				ctx->buffer = encoded[i++];
			}
			tmp = ctx->buffer & 0x0001;
			ctx->buffer = ctx->buffer >> 1;
			ctx->filled--;
			decoded[k] = decoded[k] | (tmp << j);
		}
		
		k++;
	}

	return(k);
}
