#include <stdio.h>
#include <malloc.h>

#include "lpc.h"
#include "wavwriter.h"

#define SAMPLES 200
#define SHORT_MAX 32767

int main(int argc, char **argv) 
{
	int i;
	short p_ord = 20,opt_order;
	size_t elements;

	if(argc < 3)
	{
		fprintf(stderr,"Usage : %s <input.wav> <output.wav>\n",argv[0]);
		fprintf(stderr,"Supports 16 bit mono wav files only.\n");
		return -1;
	}
	
	FILE *infile = fopen(argv[1],"rb");
	if(infile == NULL)
	{
		fprintf(stderr,"Error while opening input file");
		return -1;
	}
	FILE *outfile = fopen(argv[2],"wb");
	if(infile == NULL)
	{
		fprintf(stderr,"Error while opening output file");
		return -1;
	}

	char header[44];
	size_t bytes = fread(header,sizeof(char),44,infile);
	(void)bytes;

	int *sample_rate = (int *)(header + 24);
	fprintf(stderr,"Sample rate = %d Hz\n",*sample_rate);
	short *channels = (short *)(header + 22);
	if(*channels != 1)
	{
		fprintf(stderr,"Not a mono wav file.");
		return -1;
	}
	fprintf(stderr,"Channels = %d\n",*channels);
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
	
	short *samples = (short *)malloc(sizeof(short)*SAMPLES);
	double *qsamples = (double *)malloc(sizeof(double)*SAMPLES);
	short *esamples = (short *)malloc(sizeof(short)*SAMPLES);

	while(1)
	{
		if(feof(infile))
			break;

		//Read Samples from audio
		elements = fread(samples,sizeof(short),SAMPLES,infile);
		
		//Quantize samples
		for(i = 0; i < elements; i++)
			qsamples[i] = (double)samples[i]/SHORT_MAX;
		
		//Get autocorrelation data
		double rxx[p_ord+1];
		acf(qsamples,elements,p_ord,1,rxx);

		//get lpc coefficients
		double lpc_mat[MAX_LPC_ORDER][MAX_LPC_ORDER];
		opt_order = compute_lpc_coefs_est(rxx,p_ord,lpc_mat);
		double lpc[opt_order];
		for(i = 0; i < opt_order; i++)
			lpc[i] = lpc_mat[opt_order - 1][i];

		//Calculate residue signal
		double residue[elements];
		calc_residue(qsamples,elements,lpc,opt_order,residue);

		//Get back original
		double est_rcv[elements];
		for(i = 0; i < elements; i++)
			est_rcv[i] = 0;
		calc_original(residue,elements,lpc,opt_order,est_rcv);
		
		//Upscale samples
		for(i = 0; i < elements; i++)
			esamples[i] = est_rcv[i]*SHORT_MAX;

		//Write estimates to output
		fwrite(esamples,sizeof(short),elements,outfile);
		
		#ifdef __DEBUG__
		
			fprintf(stderr,"\n");
			for(i = 0; i < elements; i++)
				fprintf(stderr,"Original[%d] = %f\n",i+1,qsamples[i]);
			
			fprintf(stderr,"\n");
			for(i = 0; i < p_ord; i++)
				fprintf(stderr,"rxx[%d] = %f\n",i+1,rxx[i]);
			
			fprintf(stderr,"\n");
			for(i = 0; i < opt_order; i++)
				fprintf(stderr,"lpc[%d] = %f\n",i+1,lpc[i]);
			
			fprintf(stderr,"\n");
			for(i = 0; i < elements; i++)
				fprintf(stderr,"Residue[%d] = %f\n",i+1,residue[i]);
			
			fprintf(stderr,"\n");
			for(i = 0; i < elements; i++)
				fprintf(stderr,"Received[%d] = %f\n",i+1,est_rcv[i]);
			
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
	
	return 0;
}
