#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "rice.h"
#include "lpc.h"
#include "wavutils.h"
#include "apev2.h"
#define BLOCK_SIZE 2048

int main(int argc,char **argv)
{
	fprintf(stderr,"SimplE Lossless Audio Decoder\n");
	fprintf(stderr,"Copyright (c) 2015-2016. Ratul Saha\n");
	fprintf(stderr,"Released under MIT license\n\n");
	
	if(argc < 3)
	{
		fprintf(stderr,"Usage\n%s input.sela output.wav\n",argv[0]);
		return -1;
	}

	FILE *infile = fopen(argv[1],"rb");
	FILE *outfile = fopen(argv[2],"wb");
	fprintf(stderr,"Input : %s\n",argv[1]);
	fprintf(stderr, "Output : %s\n",argv[2]);

	char magic_number[4];
	size_t read_bytes = fread(magic_number,sizeof(char),4,infile);
	if(strncmp(magic_number,"SeLa",4))
	{
		fprintf(stderr,"Not a sela file.\n");
		fclose(infile);
		fclose(outfile);
		return -1;
	}

	//Variables
	int8_t percent = 0;
	uint8_t channels,curr_channel,rice_param_ref,rice_param_residue,opt_lpc_order;
	int16_t bps,meta_present = 0;
	const int16_t Q = 35;
	uint16_t num_ref_elements,num_residue_elements,samples_per_channel = 0;
	int32_t sample_rate,i;
	int32_t frame_sync_count = 0;
	uint32_t temp;
	uint32_t seconds,estimated_frames;
	const uint32_t frame_sync = 0xAA55FF00;
	const uint32_t metadata_sync = 0xAA5500FF;
	const int64_t corr = ((int64_t)1) << Q;
	size_t read,written;

	//Arrays
	int32_t s_ref[MAX_LPC_ORDER];
	int32_t s_residues[BLOCK_SIZE];
	int32_t rcv_samples[BLOCK_SIZE];
	int64_t lpc[MAX_LPC_ORDER + 1];
	uint32_t compressed_ref[MAX_LPC_ORDER];
	uint32_t compressed_residues[BLOCK_SIZE];
	uint32_t decomp_ref[MAX_LPC_ORDER];
	uint32_t decomp_residues[BLOCK_SIZE];
	double ref[MAX_LPC_ORDER];
	double lpc_mat[MAX_LPC_ORDER][MAX_LPC_ORDER];
	
	//Metadata structures
	apev2_state read_state;
	apev2_keys keys_inst;
	apev2_item_list ape_read_list;
	apev2_hdr_ftr read_header;

	//Initialise state
	init_apev2(&read_state);

	//Initialise apev2 keys
	init_apev2_keys(&read_state,&keys_inst);

	//Init apev2 header
	init_apev2_header(&read_state,&read_header);

	//Init apev2 list
	init_apev2_item_list(&read_state,&ape_read_list);

	//Read media info from input file
	read = fread(&sample_rate,sizeof(int32_t),1,infile);
	read = fread(&bps,sizeof(int16_t),1,infile);
	read = fread(&channels,sizeof(int8_t),1,infile);
	read = fread(&estimated_frames,sizeof(uint32_t),1,infile);

	//Read metadata if present
	read = fread(&temp,sizeof(int32_t),1,infile);
	if(temp == metadata_sync)
	{
		meta_present = 1;
		fread(&temp,sizeof(int32_t),1,infile);
		char *metadata = (char *)malloc(sizeof(char) * temp);
		fread(metadata,sizeof(char),(size_t)temp,infile);
		read_apev2_tags(&read_state,metadata,temp,&keys_inst,&read_header,&ape_read_list);
	}
	else
		fseek(infile,-4,SEEK_CUR);//No tags. Rewind 4 bytes

	fprintf(stderr,"\nStream Information\n");
	fprintf(stderr,"------------------\n");
	fprintf(stderr,"Sample rate : %d Hz\n",sample_rate);
	fprintf(stderr,"Bits per sample : %d\n",bps);
	fprintf(stderr,"Channels : %d",channels);

	if(channels == 1)
		fprintf(stderr,"(Monoaural)\n");
	else if(channels == 2)
		fprintf(stderr,"(Stereo)\n");
	else
		fprintf(stderr,"\n");

	fprintf(stderr,"\nMetadata\n");
	fprintf(stderr,"--------\n");
	if(meta_present == 0)
		fprintf(stderr,"No metadata found\n");
	else
		print_apev2_tags(&read_state,&ape_read_list);

	wav_header hdr;
	initialize_header(&hdr,channels,sample_rate,bps);
	write_header(outfile,&hdr);

	int16_t *buffer = (int16_t *)calloc((BLOCK_SIZE * channels),sizeof(int16_t));

	//Main loop
	while(feof(infile) == 0)
	{
		read = fread(&temp,sizeof(int32_t),1,infile);//Read from input
		if(temp == frame_sync)
		{
			frame_sync_count++;
			for(i = 0; i < channels; i++)
			{
				//Read channel number
				read = fread(&curr_channel,sizeof(uint8_t),1,infile);

				//Read rice_param,lpc_order,encoded lpc_coeffs from input
				read = fread(&rice_param_ref,sizeof(uint8_t),1,infile);
				read = fread(&num_ref_elements,sizeof(uint16_t),1,infile);
				read = fread(&opt_lpc_order,sizeof(uint8_t),1,infile);
				read = fread(compressed_ref,sizeof(uint32_t),num_ref_elements,infile);

				//Read rice_param,num_of_residues,encoded residues from input
				read = fread(&rice_param_residue,sizeof(uint8_t),1,infile);
				read = fread(&num_residue_elements,sizeof(uint16_t),1,infile);
				read = fread(&samples_per_channel,sizeof(uint16_t),1,infile);
				read = fread(compressed_residues,sizeof(uint32_t),num_residue_elements,infile);

				//Decode compressed reflection coefficients and residues
				rice_decode_block(rice_param_ref,compressed_ref,opt_lpc_order,decomp_ref);
				rice_decode_block(rice_param_residue,compressed_residues,samples_per_channel,decomp_residues);

				//unsigned to signed
				unsigned_to_signed(opt_lpc_order,decomp_ref,s_ref);
				unsigned_to_signed(samples_per_channel,decomp_residues,s_residues);

				//Dequantize reflection coefficients
				dqtz_ref_cof(s_ref,opt_lpc_order,ref);

				//Generate lpc coefficients
				levinson(NULL,opt_lpc_order,ref,lpc_mat);
				lpc[0] = 0;
				for(int32_t k = 0; k < opt_lpc_order; k++)
					lpc[k + 1] = (int64_t)(corr * lpc_mat[opt_lpc_order - 1][k]);

				for(int32_t k = opt_lpc_order; k < MAX_LPC_ORDER; k++)
					lpc[k + 1] = 0;

				//lossless reconstruction
				calc_signal(s_residues,samples_per_channel,opt_lpc_order,Q,lpc,rcv_samples);

				//Combine samples from all channels
				for(int32_t k = 0; k < samples_per_channel; k++)
					buffer[channels * k + i] = (int16_t)rcv_samples[k];
			}
			written = fwrite(buffer,sizeof(int16_t),(samples_per_channel * channels),outfile);
			temp = 0;

			//Percentage bar print
			fprintf(stderr,"\r");
			percent = ((float)frame_sync_count/(float)estimated_frames) * 100;
			fprintf(stderr,"[");
			for(i = 0; i < (percent >> 2); i++)
				fprintf(stderr,"=");
			for(i = 0; i < (25 - (percent >> 2)); i++)
				fprintf(stderr," ");
			fprintf(stderr,"]");
		}
		else
			break;
	}

	fprintf(stderr,"\n");
	seconds = ((uint32_t)(frame_sync_count * BLOCK_SIZE)/(sample_rate));
	fprintf(stderr,"\nStatistics\n");
	fprintf(stderr,"----------\n");
	fprintf(stderr,"%d frames decoded. %dmin %dsec of audio\n",
		frame_sync_count,(seconds/60),(seconds%60));
	finalize_file(outfile);

	//Cleanup
	free(buffer);
	fclose(infile);
	fclose(outfile);
	free_apev2_list(&read_state,&ape_read_list);

	return 0;
}
