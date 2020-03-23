#include "../include/sela/player.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace sela {

void Player::initializeAo()
{
    ao_initialize();
    driver = ao_default_driver_id();
}

void Player::setAoFormat(const data::WavFormatSubChunk& format)
{
    ao_format.bits = format.bitsPerSample;
    ao_format.rate = format.sampleRate;
    ao_format.channels = format.numChannels;
    ao_format.byte_format = AO_FMT_NATIVE;
    ao_format.matrix = 0;
    dev = ao_open_live(driver, &ao_format, NULL);
}

void Player::transform(const std::vector<data::WavFrame>& wavFrames)
{
    for (data::WavFrame wavFrame : wavFrames) {
        int16_t* samples = new int16_t[wavFrame.samples.size() * wavFrame.samples[0].size()];
        size_t offset = 0;
        for (size_t i = 0; i < wavFrame.samples[0].size(); i++) {
            samples[offset] = (uint16_t)wavFrame.samples[0][i];
            offset++;
            samples[offset] = (uint16_t)wavFrame.samples[1][i];
            offset++;
        }
        if (transformCount.load() == 100) {
            std::unique_lock<std::mutex> lock(mutex);
            condVar.wait(lock);
        }
        transformCount.store(transformCount.load() + 1);
        audioPackets.push_back(data::AudioPacket((char*)samples, wavFrame.samples.size() * wavFrame.samples[0].size() * sizeof(int16_t)));
    }
}

void Player::play(const file::WavFile& wavFile)
{
    audioPackets.reserve(wavFile.wavChunk.dataSubChunk.wavFrames.size());
    transformCount.store(0);

    initializeAo();
    setAoFormat(wavFile.wavChunk.formatSubChunk);

    //Start transformer thread
    std::thread transformThread = std::thread(&Player::transform, this, std::ref(wavFile.wavChunk.dataSubChunk.wavFrames));

    //Wait for 1 ms on this thread
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    //Transform data to proper format
    for (size_t i = 0; i < wavFile.wavChunk.dataSubChunk.wavFrames.size(); i++) {
        ao_play(dev, audioPackets[i].audio, (uint32_t)audioPackets[i].bufferSize);
        delete[] audioPackets[i].audio;
        transformCount.store(transformCount.load() - 1);
        if (transformCount.load() < 10) {
            condVar.notify_one();
        }
    }

    transformThread.join();
    destroyAo();
}

void Player::destroyAo()
{
    ao_close(dev);
    ao_shutdown();
}
}