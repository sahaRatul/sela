#include <pthread.h>
#include <malloc.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#include "packetqueue.h"
#include "pulse_output.h"

pa_simple *handle = NULL;
pa_sample_spec audio_spec;
int error;

int initialize_pulse(audio_format *format)
{
	if(format->bits_per_sample==16)
		audio_spec.format=PA_SAMPLE_S16LE;
	audio_spec.rate=format->sample_rate;
	audio_spec.channels=format->num_channels;
	
	handle = pa_simple_new(NULL,NULL, PA_STREAM_PLAYBACK,NULL,"playback",&audio_spec,NULL,NULL,&error);
	return 0;
}

void *pulse_play(PacketList *list)
{
	while(list->num_packets < 50)
		fprintf(stderr,"In loop.\r");
	
	do
	{	
		PacketNode *node=PacketQueueGet(list);
		if((pa_simple_write(handle,node->packet,node->packet_size,&error)) < 0)
			break;
		
		fprintf(stderr,"%03d Packets in queue.\r",list->num_packets);
		if(node==NULL)
			break;
		
		//Clean up packets after playing
		free(node->packet);
		node->packet=NULL;
		free(node);
		node=NULL;
		if(list->num_packets==0)
			break;
	}
	while(1);
	
	fprintf(stderr,"Total %d packets played.\n",list->total_packets_count);
	return NULL;
}

int destroy_pulse()
{
	if (handle)
        pa_simple_free(handle);
    return 0;
}
