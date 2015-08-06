#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "rice.h"

/*
*Golomb-Rice algorithm is optimized to work with unsigned numbers
*This function maps all positive numbers to even postive numbers
*and negative numbers to odd positive numbers
*/
int32_t signed_to_unsigned(const uint32_t data_size,const int32_t *input,uint32_t *output)
{
	for(uint32_t i = 0; i < data_size; i++)
		(input[i] < 0) ? (output[i] = (-(input[i] << 1)) - 1) : (output[i] = input[i] << 1);
	return(0);
}

/*
*This function does exactly the reverse of the above function
*Maps all odd integers to neagtive numbers
*and even integers to positive numbers
*/
int32_t unsigned_to_signed(const uint32_t data_size,const uint32_t *input,int32_t *output)
{
	for(uint32_t i = 0; i < data_size; i++)
		((input[i] & 0x01) == 0x01) ? (output[i] = -((input[i] + 1) >> 1)) : (output[i] = input[i] >> 1);
	return(0);
}

/*calculate optimum rice parameter for encoding a block of data*/
uint16_t get_opt_rice_param(const uint32_t *data,int32_t data_size,uint32_t *req_bits)
{
	int32_t i = 0,j;
	uint32_t bits[MAX_RICE_PARAM];
	uint32_t temp;
	uint16_t best_index;

	for(i = 0; i < MAX_RICE_PARAM; i++)
	{
		temp = 0;
		for(j = 0; j < data_size; j++)
		{
			//data[j]/2^k;
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
			best_index = (int16_t)i;
		}
	}
	return best_index;
}

/*
*Encode a block of data using Golomb-Rice coding
*For more details visit
*http://michael.dipperstein.com/rice/index.html
*/
uint32_t rice_encode_block(int16_t param,const uint32_t *input,int32_t size,uint32_t *encoded)
{
	int32_t i,j,temp,written = 0;
	uint32_t bits;

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
			if(ctx->filled == 31)
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
		if(ctx->filled == 31)
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
			if(ctx->filled == 31)
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
uint32_t rice_decode_block(int16_t param,const uint32_t *encoded,int32_t count,uint32_t *decoded)
{
	int32_t i = 0,j = 0,q,k = 0;
	int16_t tmp;
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
				ctx->filled = 32;
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
				ctx->filled = 32;
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
