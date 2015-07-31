#ifndef _AO_OUTPUT_H_
#define _AO_OUTPUT_H_

#include "packetqueue.h"
#include "format.h"

int initialize_ao();
int destroy_ao();
int set_ao_format(audio_format *format);
void *play_ao(PacketList *list);

#endif
