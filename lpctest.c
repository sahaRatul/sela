#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lpc.h"

#define BLOCK_SIZE 2048
#define SHORT_MAX 32767

int32_t check_wav_file(FILE *fp,int32_t *sample_rate,int16_t *channels,int16_t *bps);

int main(int argc,char **argv)
{

	if(argc < 3)
	{
		fprintf(stderr,"Usage : %s <input.wav> <output.wav>\n",argv[0]);
		return -1;
	}

	FILE *infile = fopen(argv[1],"rb");
	FILE *outfile = fopen(argv[2],"wb");
	if(infile == NULL || outfile == NULL)
	{
		fprintf(stderr,"Error open input or output file.Exiting.......\n");
		return -1;
	}
	else
	{
		fprintf(stderr,"Input : %s\n",argv[1]);
		fprintf(stderr,"Output : %s\n",argv[2]);
	}

	int32_t sample_rate;
	int16_t channels,bps;
	const int8_t Q = 35;
	int64_t corr = ((int64_t)1) << Q;
	
	check_wav_file(infile,&sample_rate,&channels,&bps);
	int32_t is_wav = check_wav_file(infile,&sample_rate,&channels,&bps);

	switch(is_wav)
	{
		case 1:
			fprintf(stderr,"WAV file detected.\n");
			break;
		case -1:
			fprintf(stderr,"RIFF header not found. Exiting......\n");
			return -1;
		case -2:
			fprintf(stderr,"WAVE header not found. Exiting......\n");
			return -1;
		case -3:
			fprintf(stderr,"Not a 16 bit/sample wav file. Exiting......\n");
			return -1;
		default:
			fprintf(stderr,"Some error occured. Exiting.......\n");
	}

	fprintf(stderr,"Sampling Rate : %d Hz\n",sample_rate);
	fprintf(stderr,"Bits per sample : %d\n",bps);
	fprintf(stderr,"Channels : %d\n",channels);
	fseek(infile,0,SEEK_SET);
	int8_t header[44];
	fread(header,sizeof(int8_t),44,infile);
	fwrite(header,sizeof(int8_t),44,outfile);

	int32_t read_size = BLOCK_SIZE * channels;
	int16_t *buffer = (int16_t *)malloc(sizeof(int16_t)*read_size);
	int16_t *rcv_buffer = (int16_t *)malloc(sizeof(int16_t)*read_size);

	while(feof(infile) == 0)
	{
		size_t read = fread(buffer,sizeof(int16_t),read_size,infile);
		int32_t samples_per_channel = read/channels;

		for(int32_t i = 0; i < channels; i++)
		{
			int16_t samples[BLOCK_SIZE];
			for(int32_t j = 0; j < samples_per_channel; j++)
				samples[j] = buffer[channels * j + i];

			int32_t int_samples[BLOCK_SIZE];
			for(int32_t j = 0; j < samples_per_channel; j++)
				int_samples[j] = samples[j];

			double qtz_samples[BLOCK_SIZE];
			for(int32_t j = 0; j < samples_per_channel; j++)
				qtz_samples[j] = ((double)samples[j])/SHORT_MAX;

			double autocorr[MAX_LPC_ORDER + 1];
			acf(qtz_samples,samples_per_channel,MAX_LPC_ORDER,1,autocorr);

			double ref[MAX_LPC_ORDER];
			uint8_t opt_lpc_order = compute_ref_coefs(autocorr,MAX_LPC_ORDER,ref);

			//Qtz reflection
			int32_t q_ref_coeffs[MAX_LPC_ORDER];
			qtz_ref_cof(ref,opt_lpc_order,q_ref_coeffs);

			//Dequantize
			dqtz_ref_cof(q_ref_coeffs,opt_lpc_order,ref);

			double lpc[MAX_LPC_ORDER+1];
			double lpc_mat[MAX_LPC_ORDER][MAX_LPC_ORDER];
			levinson(NULL,opt_lpc_order,ref,lpc_mat);
			
			for(int32_t j = 0; j < opt_lpc_order; j++)
				lpc[j+1] = lpc_mat[opt_lpc_order - 1][j];
			lpc[0] = 0;

			int64_t int_lpc[MAX_LPC_ORDER+1];
			for(int32_t j = 0; j <= opt_lpc_order; j++)
				int_lpc[j] =  (int64_t)corr * lpc[j];

			int32_t residues[BLOCK_SIZE];
			int32_t rcv_samples[BLOCK_SIZE];
			calc_residue(int_samples,samples_per_channel,opt_lpc_order,Q,int_lpc,residues);

			calc_signal(residues,samples_per_channel,opt_lpc_order,Q,int_lpc,rcv_samples);

			int mismatch_cnt = 0;
			for(int32_t i = 0; i < samples_per_channel; i++)
				if(int_samples[i] != rcv_samples[i])
					mismatch_cnt++;

			if(mismatch_cnt != 0)
				fprintf(stderr,"Mismatch %d!\n",mismatch_cnt);

			for(int32_t j = 0; j < samples_per_channel; j++)
				rcv_buffer[channels * j + i] = (int16_t)rcv_samples[j];
		}
		fwrite(rcv_buffer,sizeof(int16_t),read,outfile);
	}


	free(buffer);
	fclose(infile);
	fclose(outfile);
	return 0;
}

int32_t check_wav_file(FILE *fp,int32_t *sample_rate,int16_t *channels,int16_t *bps)
{
	fseek(fp,0,SEEK_SET);
	char header[44];
	fread(header,sizeof(char),44,fp);
	
	char *riffmarker = (char *)header;
	char *wavemarker = (char *)(header + 8);

	/*Check RIFF header*/
	if(strncmp(riffmarker,"RIFF",4) != 0)
		return -1;

	/*Check WAVE Header*/
	if(strncmp(wavemarker,"WAVE",4) != 0)
		return -2;

	/*Sample Rate*/
	*sample_rate = *((int32_t *)(header + 24));

	/*Channels*/
	*channels = *((int16_t *)(header + 22));

	/*Bits per sample*/
	*bps = *((int16_t *)(header + 34));
	if(*bps != 16)
		return -3;

	return 1;	
}
