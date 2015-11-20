#ifndef _PORTOUTPUT_H_
#define _PORTOUTPUT_H_

int32_t initialize_portaudio(PacketList *list);
int32_t set_portaudio_format(audio_format *format);
void *portaudio_play();
int32_t destroy_portaudio();

#endif