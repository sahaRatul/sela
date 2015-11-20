#ifndef _PULSE_OUTPUT_H_
#define _PULSE_OUTPUT_H_

#include "packetqueue.h"
#include "format.h"

int32_t initialize_pulse(audio_format *format);
void *pulse_play(PacketList *list);
int32_t destroy_pulse();

#endif
