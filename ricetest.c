#include <stdio.h>
#include <math.h>
#include <malloc.h>

#include "rice.h"

#define DATA_SIZE 1024

int main(int argc,char **argv)
{
	int i,j = 0;
	short opt_param;
	size_t read;
	unsigned int bits = 1,bytes = 1;

	#ifndef __DEBUG__
		FILE *fp = fopen(argv[1],"rb");
		if(fp == NULL)
		{
			fprintf(stderr,"FILE not found, exiting.....\n");
			return -1;
		}

		short *buffer = (short *)malloc(sizeof(short) * DATA_SIZE);//Original buffer
		unsigned short *p_buffer = (unsigned short *)calloc(DATA_SIZE,sizeof(unsigned int));//Positive buffer
		unsigned short *d_buffer = (unsigned short *)calloc(DATA_SIZE,sizeof(unsigned int));//Decoded buffer
		short *buffer2 = (short *)malloc(sizeof(short) * DATA_SIZE);	

		while(1)
		{
			if(feof(fp))
				break;

			read = fread(buffer,sizeof(short),DATA_SIZE,fp);

			/*Transform everything to positive*/
			for(i = 0; i < read; i++)
			{
				if(buffer[i] > 0)
					p_buffer[i] = 2 * buffer[i];
				if(buffer[i] < 0)
					p_buffer[i] = (-2 * buffer[i]) - 1;
			}

			/*Get optimum rice parameter*/
			opt_param = get_opt_rice_param(p_buffer,(read),&bits);

			/*Calculate required bytes to encode data*/
			bytes = ceil(((float)(bits))/(sizeof(unsigned short) * 8));

			/*Encode data*/
			unsigned short *encoded = (unsigned short *)calloc((bytes),sizeof(unsigned short));
			rice_encode_block(opt_param,p_buffer,(read),encoded);

			/*Decode data*/
			rice_decode_block(opt_param,encoded,bytes,d_buffer);
			free(encoded);

			/*Transform back to original data*/
			for(i = 0; i < read; i++)
			{
				if((d_buffer[i] & 0x01) == 0x01)
					buffer2[i] = -((d_buffer[i] + 1) >> 1);
				if((d_buffer[i] & 0x01) == 0x00)
					buffer2[i] = d_buffer[i] >> 1;
			}

			if(j == 1000)
			{
				for(i = 0; i < read; i++)
				{
					fprintf(stderr,"Original[%d] = %d\n",i,buffer[i]);
					fprintf(stderr,"Decompressed[%d] = %d\n",i,buffer2[i]);
					fprintf(stderr,"\n");
				}
			}
			j++;
		}
		
		free(buffer);
		free(p_buffer);
		free(d_buffer);
		free(buffer2);

		fclose(fp);
	#endif

	#ifdef __DEBUG__
		short buffer[10] = {200,234,134,156,323,456,234,100,138,334};
		unsigned short p_buffer[10];

		for(i = 0; i < 10; i++)
			fprintf(stderr,"Input[%d] = %d\n",i,buffer[i]);

		fprintf(stderr,"\n");

		/*Transform everything to positive*/
		for(i = 0; i < 10; i++)
		{
			if(buffer[i] > 0)
				p_buffer[i] = 2 * buffer[i];
			if(buffer[i] < 0)
				p_buffer[i] = (-2 * buffer[i]) - 1;
		}

		/*Calculate optimum rice parameter*/
		opt_param = get_opt_rice_param(p_buffer,10,&bits);
		fprintf(stderr,"Optimum rice parameter : %d\n\n",opt_param);
		fprintf(stderr,"%d bits required.\n\n",bits);

		bytes = ceil(((float)(bits))/(sizeof(unsigned short) * 8));

		/*Encode*/
		unsigned short *encoded = (unsigned short *)calloc(bytes,sizeof(unsigned short));
		bits = rice_encode_block(opt_param,p_buffer,10,encoded);
		fprintf(stderr,"%d bits written.\n\n",bits);

		for(i = 0; i < bytes; i++)
			fprintf(stderr,"Encoded[%d] = %d\n",i,encoded[i]);
		fprintf(stderr,"\n");

		/*Decode*/
		unsigned short d_buffer[10];
		int out_elements = rice_decode_block(opt_param,encoded,bytes,d_buffer);
		for(i = 0; i < out_elements; i++)
			fprintf(stderr,"Decoded[%d] = %d\n",i,d_buffer[i]);
		free(encoded);

		/*Transform back to original*/
		short buffer2[10];
		for(i = 0; i < 10; i++)
		{
			if((d_buffer[i] & 0x01) == 0x01)
				buffer2[i] = -((d_buffer[i] + 1) >> 1);
			if((d_buffer[i] & 0x01) == 0x00)
				buffer2[i] = d_buffer[i] >> 1;
		}

		fprintf(stderr,"\n");

		for(i = 0; i < out_elements; i++)
			fprintf(stderr,"Decompressed[%d] = %d\n",i,buffer2[i]);

	#endif

	return 0;
}
