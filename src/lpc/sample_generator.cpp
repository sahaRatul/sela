#include "../include/lpc.hpp"

namespace lpc {
SampleGenerator::SampleGenerator(const data::LpcEncodedData& encodedData)
    : residues(encodedData.residues)
    , bitsPerSample(encodedData.bitsPerSample)
    , linearPredictor(LinearPredictor(encodedData.quantizedReflectionCoefficients, encodedData.optimalLpcOrder))
{
}

inline void SampleGenerator::generateSamples(std::vector<int32_t>& samples)
{
    int64_t correction = (int64_t)1 << (CORRECTION_FACTOR - 1);

    samples[0] = residues[0];
    for (size_t i = 1; i <= linearPredictor.optimalLpcOrder; i++) {
        int64_t temp = correction;
        for (size_t j = 1; j <= i; j++) {
            temp -= linearPredictor.linearPredictionCoefficients[j] * samples[i - j];
        }
        samples[i] = residues[i] - (int)(temp >> CORRECTION_FACTOR);
    }

    for (size_t i = linearPredictor.optimalLpcOrder + 1; i < residues.size(); i++) {
        int64_t temp = correction;
        for (size_t j = 0; j <= linearPredictor.optimalLpcOrder; j++)
            temp -= (linearPredictor.linearPredictionCoefficients[j] * samples[i - j]);
        samples[i] = residues[i] - (int)(temp >> CORRECTION_FACTOR);
    }
}

data::LpcDecodedData SampleGenerator::process()
{
    std::vector<int32_t> samples = std::vector<int32_t>(residues.size(), 0);
    linearPredictor.dequantizeReflectionCoefficients();
    linearPredictor.generatelinearPredictionCoefficients();
    generateSamples(samples);
    return data::LpcDecodedData(bitsPerSample, std::move(samples));
}
}