#include <stdio.h>
#include <stdint.h>

#include "wavutils.h"

#define READ_SIZE 4096

int main(int argc,char **argv)
{
	if(argc < 3)
	{
		fprintf(stderr,"This program generates a wav file based on the difference between two wav files\n");
		fprintf(stderr,"The difference is stored in 'diff.wav' file\n");
		fprintf(stderr,"Usage : %s <input1.wav> <input2.wav>\n",argv[0]);
		return -1;
	}

	FILE *infile1 = fopen(argv[1],"rb");
	if(infile1 == NULL)
	{
		fprintf(stderr,"Error while opening 1st input file\n");
		return -1;
	}
		
	FILE *infile2 = fopen(argv[2],"rb");
	if(infile2 == NULL)
	{
		fprintf(stderr,"Error while opening 2nd input file\n");
		fclose(infile1);
		return -1;
	}

	FILE *outfile = fopen("diff.wav","wb");
	if(outfile == NULL)
	{
		fprintf(stderr,"Error while opening output file\n");
		fclose(infile1);
		fclose(infile2);
		return -1;
	}

	int16_t channels1,channels2,bps1,bps2;
	int32_t sample_rate1,sample_rate2;
	size_t read1,read2,written,i;

	int8_t infile1_buffer[READ_SIZE];
	int8_t infile2_buffer[READ_SIZE];
	int8_t diff_buffer[READ_SIZE];
	wav_header hdr;

	//Check whether the input files are wav or not
	int32_t infile1_is_wav = check_wav_file(infile1,&sample_rate1,&channels1,&bps1);
	int32_t infile2_is_wav = check_wav_file(infile2,&sample_rate2,&channels2,&bps2);

	if(infile1_is_wav != READ_STATUS_OK)
	{
		fprintf(stderr,"%s is not a valid wav file.\n",argv[1]);
		fclose(infile1);
		fclose(infile2);
		fclose(outfile);
	}
	if(infile2_is_wav != READ_STATUS_OK)
	{
		fprintf(stderr,"%s is not a valid wav file.\n",argv[2]);
		fclose(infile1);
		fclose(infile2);
		fclose(outfile);
	}

	//Check whether sample rate,channels,bps of input file matches
	if(sample_rate1 != sample_rate2)
		fprintf(stderr,"Sample Rates are not equal mismatch\n");

	if(channels1 != channels2)
		fprintf(stderr,"Number of channels are not equal in %s & %s\n",argv[1],argv[2]);

	if(bps1 != bps2)
		fprintf(stderr,"Bits per sample are not equal in %s & %s\n",argv[1],argv[2]);

	if((sample_rate1 != sample_rate2) || (channels1 != channels2) || (bps1 != bps2))
	{
		fclose(infile1);
		fclose(infile2);
		fclose(outfile);
	}

	initialize_header(&hdr,channels1,sample_rate1,bps1);
	write_header(outfile,&hdr);

	while((feof(infile1) == 0) && (feof(infile2) == 0))
	{
		read1 = fread(infile1_buffer,sizeof(int8_t),READ_SIZE,infile1);
		read2 = fread(infile2_buffer,sizeof(int8_t),READ_SIZE,infile2);

		if(read1 == read2)
		{
			for(i = 0; i < read1; i++)
				diff_buffer[i] = infile1_buffer[i] - infile2_buffer[i];

			written = fwrite(diff_buffer,sizeof(int8_t),read1,outfile);
		}
		else
		{
			if(read1 < read2)
			{
				for(i = 0; i < read1; i++)
					diff_buffer[i] = infile1_buffer[i] - infile2_buffer[i];

				written = fwrite(diff_buffer,sizeof(int8_t),read2,outfile);
			}
			else
			{
				for(i = 0; i < read2; i++)
					diff_buffer[i] = infile1_buffer[i] - infile2_buffer[i];

				written = fwrite(diff_buffer,sizeof(int8_t),read2,outfile);
			}

			break;
		}
	}

	finalize_file(outfile);

	fclose(infile1);
	fclose(infile2);
	fclose(outfile);
	return 0;
}