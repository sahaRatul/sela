#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "rice.h"
#include "lpc.h"
#include "format.h"
#include "packetqueue.h"

#ifdef __PULSE__
	#include "pulse_output.h"
#elif __AO__
	#include "ao_output.h"
#elif __PORTAUDIO__
	#include "portaudio_output.h"
#endif

#define BLOCK_SIZE 2048

void *playback_func(void *arg);

int main(int argc,char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr,"Usage : %s <input.sela>\n",argv[0]);
		return -1;
	}

	FILE *infile = fopen(argv[1],"rb");
	if(infile == NULL)
	{
		fprintf(stderr,"Error while opening input.\n");
		return -1;
	}

	char magic_number[4];
	size_t read_bytes = fread(magic_number,sizeof(char),4,infile);
	if(strncmp(magic_number,"SeLa",4))
	{
		fprintf(stderr,"Not a sela file.\n");
		return -1;
	}
	else
	{
		fprintf(stderr,"Input : %s\n",argv[1]);
		#ifdef __PULSE__
			fprintf(stderr,"Using pulseaudio output\n");
		#elif __AO__
			fprintf(stderr,"Using Xiph.org libao output\n");
		#elif __PORTAUDIO__
			fprintf(stderr,"Using PortAudio output\n");
		#endif
	}

	//Variables and arrays
	uint8_t channels,curr_channel,rice_param_ref,rice_param_residue,opt_lpc_order;
	int16_t bps;
	const int16_t Q = 35;
	uint16_t num_ref_elements,num_residue_elements,samples_per_channel = 0;
	int32_t sample_rate,i;
	uint32_t temp;
	const uint32_t frame_sync = 0xAA55FF00;
	const int64_t corr = ((int64_t)1) << Q;
	size_t read,written;
	PacketList list;
	audio_format fmt;
	pthread_t play_thread;

	uint32_t compressed_ref[MAX_LPC_ORDER];
	uint32_t compressed_residues[BLOCK_SIZE];
	uint32_t decomp_ref[MAX_LPC_ORDER];
	uint32_t decomp_residues[BLOCK_SIZE];
	int32_t s_ref[MAX_LPC_ORDER];
	int32_t s_residues[BLOCK_SIZE];
	int32_t rcv_samples[BLOCK_SIZE];
	int64_t lpc[MAX_LPC_ORDER + 1];
	double ref[MAX_LPC_ORDER];
	double lpc_mat[MAX_LPC_ORDER][MAX_LPC_ORDER];

	read = fread(&sample_rate,sizeof(int),1,infile);
	read = fread(&bps,sizeof(short),1,infile);
	read = fread(&channels,sizeof(unsigned char),1,infile);

	fprintf(stderr,"Sample rate : %d Hz\n",sample_rate);
	fprintf(stderr,"Bits per sample : %d\n",bps);
	fprintf(stderr,"Channels : %d\n",channels);

	fmt.sample_rate = sample_rate;
	fmt.num_channels = channels;
	fmt.bits_per_sample = bps;

	#ifdef __PULSE__
		initialize_pulse(&fmt);
	#elif __AO__
		initialize_ao();
		set_ao_format(&fmt);
	#elif __PORTAUDIO__
		initialize_portaudio(&list);
		set_portaudio_format(&fmt);
	#endif

	PacketQueueInit(&list);

	//Start playback thread
	pthread_create(&play_thread,NULL,&playback_func,(void *)(&list));

	//Main loop
	while(feof(infile) == 0)
	{
		short *buffer = (short *)malloc(sizeof(short) * BLOCK_SIZE * channels);
		read = fread(&temp,sizeof(int),1,infile);//Read from input

		if(temp == frame_sync)
		{
			for(i = 0; i < channels; i++)
			{
				//Read channel number
				read = fread(&curr_channel,sizeof(unsigned char),1,infile);

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
					lpc[k+1] = (int64_t)(corr * lpc_mat[opt_lpc_order - 1][k]);

				//lossless reconstruction
				calc_signal(s_residues,samples_per_channel,opt_lpc_order,Q,lpc,rcv_samples);

				//Combine samples from all channels
				for(int32_t k = 0; k < samples_per_channel; k++)
					buffer[channels * k + i] = (int16_t)rcv_samples[k];
			}

			PacketNode *node=(PacketNode *)malloc(sizeof(PacketNode));
			node->packet = (char *)buffer;
			node->packet_size = (int16_t)(samples_per_channel * channels * sizeof(int16_t));
			PacketQueuePut(&list,node);//Insert in list
			temp = 0;
		}
		else
			break;
	}
	pthread_join(play_thread,NULL);

	#ifdef __PULSE__
		destroy_pulse();
	#elif __AO__
		destroy_ao();
	#elif __PORTAUDIO__
		destroy_portaudio();
	#endif

	fclose(infile);
	return 0;
}

void *playback_func(void *arg)
{
	PacketList *list=(PacketList *)arg;

	#ifdef __PULSE__
		pulse_play(list);
	#elif __AO__
		play_ao(list);
	#elif __PORTAUDIO__
		portaudio_play();
	#endif

	return NULL;
}
