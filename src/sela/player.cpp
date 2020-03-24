#include <chrono>
#include <iostream>
#include <thread>

#include "../include/sela/player.hpp"

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
        int16_t* samples = new int16_t[wavFrame.samples.size() * wavFrame.samples[0].size()]; //Alloc
        size_t offset = 0;
        if (wavFrame.samples.size() == 2) { //If number of channels is 2
            //Flattening of n-d array to 1d array done here
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
        } else {
            for (size_t i = 0; i < wavFrame.samples[0].size(); i++) {
                for (size_t j = 0; j < wavFrame.samples.size(); j++) {
                    samples[offset] = (uint16_t)wavFrame.samples[j][i];
                    offset++;
                }
            }
            if (transformCount.load() == 100) {
                std::unique_lock<std::mutex> lock(mutex);
                condVar.wait(lock);
            }
        }
        transformCount.store(transformCount.load() + 1);
        audioPackets.push_back(data::AudioPacket((char*)samples, wavFrame.samples.size() * wavFrame.samples[0].size() * sizeof(int16_t)));
    }
}

void Player::play(const file::WavFile& wavFile)
{
    audioPackets.reserve(wavFile.wavChunk.dataSubChunk.wavFrames.size());
    transformCount.store(0);
    size_t currentProgress = 0;

    initializeAo();
    setAoFormat(wavFile.wavChunk.formatSubChunk);

    //Start transformer thread
    std::thread transformThread(&Player::transform, this, std::ref(wavFile.wavChunk.dataSubChunk.wavFrames));

    //Start print thread
    std::thread printThread(&Player::printProgress, this, std::ref(currentProgress), wavFile.wavChunk.dataSubChunk.wavFrames.size());

    //Wait for 1 ms on this thread
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    //Audio loop
    for (size_t i = 0; i < wavFile.wavChunk.dataSubChunk.wavFrames.size(); i++) {
        currentProgress++;
        ao_play(dev, audioPackets[i].audio, (uint32_t)audioPackets[i].bufferSize);
        delete[] audioPackets[i].audio; //Free for L28
        transformCount.store(transformCount.load() - 1);
        if (transformCount.load() < 10) {
            condVar.notify_one();
        }
    }

    //Wait for worker threads to finish
    transformThread.join();
    printThread.join();

    //Close AO
    destroyAo();
}

void Player::printProgress(size_t& current, size_t total)
{
    const size_t barWidth = 40;
    std::string output;
    while (current != total) {
        double progress = (double)current / total;
        output = "[";
        size_t pos = (int32_t)(barWidth * progress);
        for (size_t i = 0; i < barWidth; ++i) {
            if (i < pos)
                output += "=";
            else if (i == pos)
                output += ">";
            else
                output += " ";
        }
        output += ("] " + std::to_string(uint32_t(progress * 100)) + "% (" + std::to_string(current) + "/" + std::to_string(total) + ")\r");

        std::cout << output << std::flush;
        //Wait for 100 ms on this thread to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << output << std::endl;
}

void Player::destroyAo()
{
    ao_close(dev);
    ao_shutdown();
}
}