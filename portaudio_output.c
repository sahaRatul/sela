#include <portaudio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "packetqueue.h"
#include "format.h"

static PacketList *pkt_list;
static PaStream *stream;

static int 
port_audio_callback(const void *in_buffer,void *out_buffer,unsigned long frames_per_buffer,const PaStreamCallbackTimeInfo *timeinfo,PaStreamCallbackFlags statusFlags,void *userdata)
{
	fprintf(stderr, "In callback.\n");
	(void)in_buffer;
	PacketList *list = (PacketList *)userdata;
	PacketNode *node = PacketQueueGet(list);

	int16_t *out = (int16_t *)out_buffer;
	int16_t *temp = (int16_t *)node->packet;

	for(int32_t i = 0; i < frames_per_buffer; i++)
		out[i] = temp[i];

	//frames_per_buffer = node->packet_size/sizeof(int16_t);

	//Clean up packets after playing
	//free(node->packet);
	//node->packet = NULL;
	//free(node);
	//node = NULL;

	return 0;
}

int32_t initialize_portaudio(PacketList *list)
{
	pkt_list = list;
	PaError err = Pa_Initialize();
	if(err != paNoError)
	{
		fprintf(stderr,"PortAudio error : %s\n",Pa_GetErrorText(err));
		return -1;
	}
	return 0;
}

int32_t set_portaudio_format(audio_format *format)
{
	PaError err;
	PaSampleFormat sample_format;

	if(format->bits_per_sample == 16)
		sample_format = paInt16;
	err = Pa_OpenDefaultStream(&stream,0,format->num_channels,sample_format,format->sample_rate,4096,port_audio_callback,(void *)&pkt_list);
	if(err != paNoError)
	{
		fprintf(stderr,"PortAudio error : %s\n",Pa_GetErrorText(err));
		return -1;
	}
	return 0;
}

void *portaudio_play()
{
	PaError err = Pa_StartStream(stream);
	if(err != paNoError)
	{
		fprintf(stderr,"PortAudio error : %s\n",Pa_GetErrorText(err));
		return(0);
	}
	return(0);
}

int32_t destroy_portaudio()
{
	PaError err = Pa_Terminate();
	if(err != paNoError)
	{
		fprintf(stderr,"PortAudio error : %s\n",Pa_GetErrorText(err));
		return -1;
	}
	return 0;
}
