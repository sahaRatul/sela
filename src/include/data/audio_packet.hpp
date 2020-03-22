#ifndef _AUDIO_PACKET_H_
#define _AUDIO_PACKET_H_

namespace data
{
class AudioPacket
{
public:
    char *audio;
    const size_t bufferSize;
    AudioPacket(char *audio, const size_t bufferSize) : audio(audio), bufferSize(bufferSize) {}
};
} // namespace data

#endif