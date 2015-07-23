#include <stdio.h>
#include <malloc.h>
#include <assert.h>

#include "rice.h"
#include "lpc.h"
#include "wavwriter.h"

#define BLOCK_SIZE 2048
#define SHORT_MAX 32767

int main(int argc,char **argv)
{
	if(argc < 3)
	{
		fprintf(stderr,"Usage : %s <input.sela> <output.wav>\n",argv[0]);
		return -1;
	}

	FILE *infile = fopen(argv[1],"rb");
	FILE *outfile = fopen(argv[2],"wb");

	if(infile == NULL || outfile == NULL)
	{
		fprintf(stderr,"Error while opening input/output.");
		return -1;
	}
	else
	{
		fprintf(stderr,"Input : %s\n",argv[1]);
		fprintf(stderr,"output : %s\n",argv[2]);
	}

	int sample_rate,temp,i,j = 0;
	unsigned char opt_lpc_order;
	const int frame_sync = 0xAA55FF00;
	const int corr = 1 << 20;
	unsigned int frame_count = 0;
	const short Q = 20;
	short bps;
	unsigned short num_ref_elements,num_residue_elements,samples_per_channel;
	unsigned char channels,curr_channel,rice_param_ref,rice_param_residue;
	wav_header hdr;

	signed short signed_decomp_ref[MAX_LPC_ORDER];
	unsigned short compressed_ref[MAX_LPC_ORDER];
	unsigned short compressed_residues[BLOCK_SIZE];
	unsigned short decomp_ref[MAX_LPC_ORDER];
	short s_ref[MAX_LPC_ORDER];
	short s_rcv_samples[BLOCK_SIZE];
	unsigned short decomp_residues[BLOCK_SIZE];
	short s_residues[BLOCK_SIZE];
	int rcv_samples[BLOCK_SIZE];
	int lpc[MAX_LPC_ORDER + 1];
	double ref[MAX_LPC_ORDER];
	double lpc_mat[MAX_LPC_ORDER][MAX_LPC_ORDER];

	fread(&sample_rate,sizeof(int),1,infile);
	fread(&bps,sizeof(short),1,infile);
	fread(&channels,sizeof(unsigned char),1,infile);

	initialize_header(&hdr,channels,sample_rate,bps);
	write_header(outfile,&hdr);

	fprintf(stderr,"Sample rate : %d Hz\n",sample_rate);
	fprintf(stderr,"Bits per sample : %d\n",bps);
	fprintf(stderr,"Channels : %d\n",channels);

	short *buffer = (short *)malloc(sizeof(short) * BLOCK_SIZE * channels);

	//Main loop
	while(feof(infile) == 0)
	{
		fread(&temp,sizeof(int),1,infile);//Read from input

		if(temp == frame_sync)
		{
			for(i = 0; i < channels; i++)
			{
				fread(&curr_channel,sizeof(unsigned char),1,infile);

				//Read rice_params,bytes,encoded lpc_coeffs from input
				fread(&rice_param_ref,sizeof(unsigned char),1,infile);
				fread(&num_ref_elements,sizeof(unsigned short),1,infile);
				fread(&opt_lpc_order,sizeof(unsigned char),1,infile);
				fread(compressed_ref,sizeof(unsigned short),num_ref_elements,infile);

				//Read rice_params,bytes,encoded residues from input
				fread(&rice_param_residue,sizeof(unsigned char),1,infile);
				fread(&num_residue_elements,sizeof(unsigned short),1,infile);
				fread(&samples_per_channel,sizeof(unsigned short),1,infile);
				fread(compressed_residues,sizeof(unsigned short),num_residue_elements,infile);

				//Decode compressed reflection coefficients and residues
				char decoded_lpc_num = rice_decode_block(rice_param_ref,compressed_ref,opt_lpc_order,decomp_ref);
				assert(decoded_lpc_num == opt_lpc_order);
				short decomp_residues_num = rice_decode_block(rice_param_residue,compressed_residues,samples_per_channel,decomp_residues);
				assert(decomp_residues_num == samples_per_channel);

				//unsigned to signed
				unsigned_to_signed(opt_lpc_order,decomp_ref,s_ref);
				unsigned_to_signed(samples_per_channel,decomp_residues,s_residues);

				//Dequantize reflection coefficients
				dqtz_ref_cof(s_ref,opt_lpc_order,Q,ref);

				//Generate lpc coefficients
				levinson(NULL,opt_lpc_order,ref,lpc_mat);
				lpc[0] = 0;
				for(int k = 0; k < opt_lpc_order; k++)
					lpc[k+1] = corr * lpc_mat[opt_lpc_order - 1][k];

				//Generate original
				calc_signal(s_residues,samples_per_channel,opt_lpc_order,Q,lpc,rcv_samples);

				//Copy_to_short
				for(int k = 0; k < samples_per_channel; k++)
					s_rcv_samples[k] = rcv_samples[k];

				for(int k = 0; k < samples_per_channel; k++)
					buffer[channels * k + i] = s_rcv_samples[k];
			}

			fwrite(buffer,sizeof(unsigned short),(samples_per_channel * channels),outfile);//Write to output
		}
		else
		{
			fprintf(stderr,"Sync lost\n");
			break;
		}
	}
	finalize_file(outfile);

	free(buffer);
	fclose(infile);
	fclose(outfile);
	return 0;
}
