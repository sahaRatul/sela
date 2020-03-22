#include "../include/sela/player.hpp"

namespace sela
{

void Player::initializeAo()
{
    ao_initialize();
    driver = ao_default_driver_id();
}

void Player::setAoFormat(const data::WavFormatSubChunk &format)
{
    ao_format.bits = format.bitsPerSample;
    ao_format.rate = format.sampleRate;
    ao_format.channels = format.numChannels;
    ao_format.byte_format = AO_FMT_NATIVE;
    ao_format.matrix = 0;
    dev = ao_open_live(driver, &ao_format, NULL);
}

void Player::play(file::WavFile wavFile)
{
    initializeAo();
    setAoFormat(wavFile.wavChunk.formatSubChunk);
    audioPackets.reserve(wavFile.wavChunk.dataSubChunk.wavFrames.size());

    const size_t bytesPerSample = wavFile.wavChunk.formatSubChunk.bitsPerSample / 8;
    for (data::WavFrame wavFrame : wavFile.wavChunk.dataSubChunk.wavFrames)
    {
        int16_t *samples = new int16_t[wavFrame.samples.size() * wavFrame.samples[0].size()];
        size_t offset = 0;
        const size_t bufferSize = wavFrame.samples[0].size() * wavFrame.samples.size() * bytesPerSample;
        for (size_t i = 0; i < wavFrame.samples[0].size(); i++)
        {
            samples[offset] = (uint16_t)wavFrame.samples[0][i];
            offset++;
            samples[offset] = (uint16_t)wavFrame.samples[1][i];
            offset++;
        }
        audioPackets.push_back(data::AudioPacket((char *)samples, wavFrame.samples.size() * wavFrame.samples[0].size() * sizeof(int16_t)));
    }

    //Transform data to proper format
    for (data::AudioPacket audioPacket : audioPackets)
    {
        ao_play(dev, audioPacket.audio, audioPacket.bufferSize);
        delete[] audioPacket.audio;
    }

    destroyAo();
}

void Player::destroyAo()
{
    ao_close(dev);
    ao_shutdown();
}
} // namespace sela