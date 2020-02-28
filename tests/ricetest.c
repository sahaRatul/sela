#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "rice.h"

#define DATA_SIZE 1024

int main(int argc,char **argv)
{
	int i,j = 0;
	int16_t opt_param;
	size_t read;
	uint32_t bits = 1,num_elements = 1;

	int32_t buffer[10] = {200,234,134,156,323,456,234,100,138,334 };
	uint32_t p_buffer[10];

	for(i = 0; i < 10; i++)
		fprintf(stderr,"Input[%d] = %d\n",i,buffer[i]);

	fprintf(stderr,"\n");

	/*Transform everything to positive*/
	signed_to_unsigned(10,buffer,p_buffer);

	/*Calculate optimum rice parameter*/
	opt_param = get_opt_rice_param(p_buffer,10,&bits);
	fprintf(stderr,"Optimum rice parameter : %d\n\n",opt_param);
	fprintf(stderr,"%d bits required.\n\n",bits);

	num_elements = ceil(((float)(bits))/(sizeof(uint32_t) * 8));

	/*Encode*/
	uint32_t encoded[10];
	bits = rice_encode_block(opt_param,p_buffer,10,encoded);
	fprintf(stderr,"%d bits written.\n\n",bits);

	for(i = 0; i < num_elements; i++)
		fprintf(stderr,"Encoded[%d] = %d\n",i,encoded[i]);
	fprintf(stderr,"\n");

	/*Decode*/
	uint32_t d_buffer[10];
	int out_elements = rice_decode_block(opt_param,encoded,10,d_buffer);
	for(i = 0; i < out_elements; i++)
		fprintf(stderr,"Decoded[%d] = %d\n",i,d_buffer[i]);

	/*Transform back to original*/
	int32_t buffer2[10];
	unsigned_to_signed(10,d_buffer,buffer2);

	fprintf(stderr,"\n");

	for(i = 0; i < out_elements; i++)
		fprintf(stderr,"Decompressed[%d] = %d\n",i,buffer2[i]);


	return 0;
}
