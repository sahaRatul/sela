#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ao/ao.h>

#include "packetqueue.h"
#include "ao_output.h"

static ao_sample_format *ao_format;
static ao_device *dev;
static int32_t driver;
static int8_t silence[1024];

int32_t initialize_ao()
{
	ao_initialize();
	driver = ao_default_driver_id();
	ao_format = (ao_sample_format *)malloc(sizeof(ao_sample_format));

	for(uint32_t i = 0; i < 1024; i++)
		silence[i] = 0;

	return 0;
}

int32_t destroy_ao()
{
	ao_close(dev);
	free(ao_format);
	ao_shutdown();
	return 0;
}

int32_t set_ao_format(audio_format *format)
{
	ao_format->bits = format->bits_per_sample;
	ao_format->rate = format->sample_rate;
	ao_format->channels = format->num_channels;
	ao_format->byte_format = AO_FMT_NATIVE;
	ao_format->matrix = 0;
	dev = ao_open_live(driver,ao_format,NULL);
	return 0;
}

void *play_ao(PacketList *list)
{
	//Play silence until there are 50 packets in queue
	while(list->num_packets < 50)
		ao_play(dev,silence,1024);

	do
	{
		PacketNode *node = PacketQueueGet(list);
		ao_play(dev,node->packet,node->packet_size);

		if(node == NULL)
			break;

		//Clean up packets after playing
		free(node->packet);
		node->packet = NULL;
		free(node);
		node = NULL;
		if(list->num_packets == 0)
			break;
	}
	while(1);
	fprintf(stderr,"Total %d packets played.\n",list->total_packets_count);

	return NULL;
}
