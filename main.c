#include <stdio.h>
#include <malloc.h>

#include "lpc.h"
#include "wavwriter.h"

#define SAMPLES 200
#define SHORT_MAX 32767

int main(int argc, char **argv) 
{
	int i,j,k;
	const short p_ord = 32;
	short opt_order;
	int elements;

	if(argc < 3)
	{
		fprintf(stderr,"Usage : %s <input.wav> <output.wav>\n",argv[0]);
		return -1;
	}
	
	FILE *infile = fopen(argv[1],"rb");
	if(infile == NULL)
	{
		fprintf(stderr,"Error while opening input file\n");
		return -1;
	}

	FILE *outfile = fopen(argv[2],"wb");
	if(infile == NULL)
	{
		fprintf(stderr,"Error while opening output file\n");
		return -1;
	}

	char header[44];
	size_t bytes = fread(header,sizeof(char),44,infile);
	(void)bytes;

	//Sample Rate
	int *sample_rate = (int *)(header + 24);
	fprintf(stderr,"Sample rate = %d Hz\n",*sample_rate);

	//Channels
	short *channels = (short *)(header + 22);
	fprintf(stderr,"Channels = %d\n",*channels);

	//Bits per sample
	short *bps = (short *)(header + 34);
	if(*bps != 16)
	{
		fprintf(stderr,"Not a 16bit/sample wav file.");
		return -1;	
	}
	fprintf(stderr,"Bits per sample = %d\n",*bps);

	wav_header hdr;
	initialize_header(&hdr,*channels,*sample_rate,*bps);

	fwrite(&hdr,sizeof(wav_header),1,outfile);
	
	short *samples = (short *)malloc(sizeof(short)*SAMPLES*(*channels));
	double *qsamples = (double *)malloc(sizeof(double)*SAMPLES*(*channels));

	double *rsamples = (double *)malloc(sizeof(double)*SAMPLES*(*channels));
	short *esamples = (short *)malloc(sizeof(short)*SAMPLES*(*channels));

	while(1)
	{
		if(feof(infile))
			break;

		//Read Samples from audio
		elements = (int)fread(samples,sizeof(short),(*channels * SAMPLES),infile);
		
		//Quantize samples
		for(i = 0; i < elements; i++)
			qsamples[i] = (double)samples[i]/SHORT_MAX;

		int samples_per_channel = elements/(*channels);
		double *temp = (double *)malloc(sizeof(double)*samples_per_channel);
		for(k = 0; k < *channels; k++)
		{
			//Split channels
			for(j = 0; j < samples_per_channel; j++)
				temp[j] = qsamples[*channels * j + k];

			//Get autocorrelation data
			double rxx[MAX_LPC_ORDER+1];
			acf(temp,samples_per_channel,p_ord,1,rxx);

			//get lpc coefficients
			double lpc_mat[MAX_LPC_ORDER][MAX_LPC_ORDER];
			opt_order = compute_lpc_coefs_est(rxx,p_ord,lpc_mat);
			double *lpc = (double *)malloc(sizeof(double)*opt_order);;
			for(j = 0; j < opt_order; j++)
				lpc[j] = lpc_mat[opt_order - 1][j];

			//Calculate residue signal
			double *residue = (double *)malloc(sizeof(double)*samples_per_channel);;
			calc_residue(temp,samples_per_channel,lpc,opt_order,residue);

			//<--------------ENCODER ENDS------------------>//
			//<--------------DECODER STARTS---------------->//

			//Get back original
			double *est_rcv = (double *)malloc(sizeof(double)*samples_per_channel);;
			calc_original(residue,samples_per_channel,lpc,opt_order,est_rcv);

			//Merge channels
			for(j = 0; j < samples_per_channel; j++)
				rsamples[*channels * j + k] = temp[j];

			free(lpc);
			free(residue);
			free(est_rcv);
		}
		free(temp);

		//Rescale samples
		for(i = 0; i < elements; i++)
			esamples[i] = (short)(rsamples[i]*SHORT_MAX);

		//Write estimates to output
		fwrite(esamples,sizeof(short),elements,outfile);
		
		#ifdef __DEBUG__
		
			fprintf(stderr,"\n");
			for(i = 0; i < elements; i++)
				fprintf(stderr,"Original[%d] = %f\n",i+1,qsamples[i]);
			
			fprintf(stderr,"\n");
			for(i = 0; i < elements; i++)
				fprintf(stderr,"Received[%d] = %f\n",i+1,rsamples[i]);
			
			break;
		
		#endif
	}

	//Finalize output file
	finalize_file(outfile);

	free(samples);
	free(qsamples);
	free(esamples);
	
	fclose(infile);
	fclose(outfile);

	//getchar();

	return 0;
}
