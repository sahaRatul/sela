#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "packetqueue.h"

#define MAX_PACKET_COUNT 250
#define MIN_PACKET_COUNT 50

void PacketQueueInit(PacketList *list)
{
	list->first = NULL;
	list->last = NULL;
	list->num_packets = 0;
	list->total_packets_count = 0;
	pthread_cond_init(&list->cond,NULL);
	pthread_mutex_init(&list->mutex,NULL);
}

void PacketQueueDestroy(PacketList *list)
{
	pthread_cond_destroy(&list->cond);
	pthread_mutex_destroy(&list->mutex);
}

void PacketQueuePut(PacketList *list,PacketNode *packet)
{
	pthread_mutex_lock(&list->mutex);
	
	//Lock when number of packets reaches 200
	while(list->num_packets == MAX_PACKET_COUNT)
		pthread_cond_wait(&list->cond,&list->mutex);
	
	if(list->num_packets == 0)
	{
		list->first = packet;
		list->last = packet;
		list->last->next = NULL;
		list->num_packets += 1;
	}
	else
	{
		list->last->next = packet;
		list->last = list->last->next;
		list->num_packets += 1;
	}
	
	list->total_packets_count += 1;
	pthread_mutex_unlock(&list->mutex);
}

PacketNode *PacketQueueGet(PacketList *list)
{
	PacketNode *packet;
	pthread_mutex_lock(&list->mutex);
	packet = list->first;
	list->first = list->first->next;
	list->num_packets -= 1;
	
	//Unlock PacketQueuePut if number of packets is <= MIN_PACKET_COUNT
	if(list->num_packets <= MIN_PACKET_COUNT)
		pthread_cond_signal(&list->cond);
	
	pthread_mutex_unlock(&list->mutex);
	fprintf(stderr,"%3d Packets in queue.\r",list->num_packets);
	return packet;
}
