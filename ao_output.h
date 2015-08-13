#ifndef _AO_OUTPUT_H_
#define _AO_OUTPUT_H_

#include "packetqueue.h"
#include "format.h"

int32_t initialize_ao();
int32_t destroy_ao();
int32_t set_ao_format(audio_format *format);
void *play_ao(PacketList *list);

#endif
