#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "lpc.h"
#include "rice.h"

#define BLOCK_SIZE 2048
#define SHORT_MAX 32767

int check_wav_file(FILE *fp,int *sample_rate,short *channels,short *bps);

int main(int argc,char **argv)
{
	FILE *infile = fopen(argv[1],"rb");
	FILE *outfile = fopen(argv[2],"wb");
	if(infile == NULL || outfile == NULL)
	{
		fprintf(stderr,"Error open input or output file.Exiting.......\n");
		return -1;
	}

	int sample_rate;
	short channels,bps;
	int corr = 1 << 20;
	const char Q = 20;
	
	check_wav_file(infile,&sample_rate,&channels,&bps);
	int is_wav = check_wav_file(infile,&sample_rate,&channels,&bps);

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
	char header[44];
	fread(header,sizeof(char),44,infile);
	fwrite(header,sizeof(char),44,outfile);

	int read_size = BLOCK_SIZE * channels;
	short *buffer = (short *)malloc(sizeof(short)*read_size);
	short *rcv_buffer = (short *)malloc(sizeof(short)*read_size);

	while(feof(infile) == 0)
	{
		size_t read = fread(buffer,sizeof(short),read_size,infile);
		int samples_per_channel = read/channels;

		for(int i = 0; i < channels; i++)
		{
			short samples[BLOCK_SIZE];
			for(int j = 0; j < samples_per_channel; j++)
				samples[j] = buffer[channels * j + i];

			int int_samples[BLOCK_SIZE];
			for(int j = 0; j < samples_per_channel; j++)
				int_samples[j] = samples[j];

			double qtz_samples[BLOCK_SIZE];
			for(int j = 0; j < samples_per_channel; j++)
				qtz_samples[j] = ((double)samples[j])/SHORT_MAX;

			double autocorr[MAX_LPC_ORDER + 1];
			acf(qtz_samples,samples_per_channel,MAX_LPC_ORDER,1,autocorr);

			double ref[MAX_LPC_ORDER];
			unsigned char opt_lpc_order = compute_ref_coefs(autocorr,MAX_LPC_ORDER,ref);

			double lpc[MAX_LPC_ORDER+1];
			double lpc_mat[MAX_LPC_ORDER][MAX_LPC_ORDER];
			levinson(NULL,opt_lpc_order,ref,lpc_mat);
			
			for(int j = 0; j < opt_lpc_order; j++)
				lpc[j+1] = lpc_mat[opt_lpc_order - 1][j];
			lpc[0] = 0;

			int int_lpc[MAX_LPC_ORDER+1];
			for(int j = 0; j <= opt_lpc_order; j++)
				int_lpc[j] =  corr * lpc[j];

			short residues[BLOCK_SIZE];
			int rcv_samples[BLOCK_SIZE];
			calc_residue(int_samples,samples_per_channel,opt_lpc_order,Q,int_lpc,residues);

			calc_signal(residues,samples_per_channel,opt_lpc_order,Q,int_lpc,rcv_samples);

			int short_rcv_samples[BLOCK_SIZE];
			for(int j = 0; j < samples_per_channel; j++)
				short_rcv_samples[j] = rcv_samples[j];

			for(int j = 0; j < samples_per_channel; j++)
				rcv_buffer[channels * j + i] = short_rcv_samples[j];
		}
		fwrite(rcv_buffer,sizeof(short),read,outfile);
	}


	free(buffer);
	fclose(infile);
	fclose(outfile);
	return 0;
}

int check_wav_file(FILE *fp,int *sample_rate,short *channels,short *bps)
{
	fseek(fp,0,SEEK_SET);
	char header[44];
	fread(header,sizeof(char),44,fp);

	/*Check RIFF header*/
	if(strncmp(header,"RIFF",4)!=0)
		return -1;

	/*Check WAVE Header*/
	if(strncmp((header+8),"WAVE",4)!=0)
		return -2;

	/*Sample Rate*/
	*sample_rate = *((int *)(header + 24));

	/*Channels*/
	*channels = *((short *)(header + 22));

	/*Bits per sample*/
	*bps = *((short *)(header + 34));
	if(*bps != 16)
		return -3;

	return 1;	
}
