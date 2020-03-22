#include <ao/ao.h>

#include "../file/wav_file.hpp"
#include "../data/audio_packet.hpp"

namespace sela
{
class Player
{
private:
    ao_sample_format ao_format;
    ao_device *dev;
    int32_t driver;
    std::vector<data::AudioPacket> audioPackets;
    void initializeAo();
    void setAoFormat(const data::WavFormatSubChunk &format);
    void transform(const file::WavFile& wavFile);
    void destroyAo();

public:
    void play(const file::WavFile& wavFile);
};
} // namespace sela