#include <ao/ao.h>
#include <condition_variable>
#include <mutex>

#include "../data/audio_packet.hpp"
#include "../file/wav_file.hpp"

namespace sela {
class Player {
private:
    ao_sample_format ao_format;
    ao_device* dev;
    int32_t driver;
    std::vector<data::AudioPacket> audioPackets;

    std::mutex mutex;
    std::condition_variable condVar;
    std::atomic<size_t> transformCount;

    void initializeAo();
    void setAoFormat(const data::WavFormatSubChunk& format);
    void transform(const std::vector<data::WavFrame>& wavFrames);
    void printProgress(size_t& current, size_t total);
    void destroyAo();

public:
    void play(const file::WavFile& wavFile);
};
} // namespace sela