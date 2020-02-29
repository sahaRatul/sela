#include "../include/lpc.hpp"

namespace lpc {
SampleGenerator::SampleGenerator(data::LpcEncodedData& encodedData)
    : residues(encodedData.residues)
    , bitsPerSample(encodedData.bitsPerSample)
    , linearPredictor(*(new LinearPredictor(encodedData.quantizedReflectionCoefficients, encodedData.optimalLpcOrder)))
{
}

inline void SampleGenerator::generateSamples()
{
    samples = std::vector<int32_t>(residues.size(), 0);

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
    linearPredictor.dequantizeReflectionCoefficients();
    linearPredictor.generatelinearPredictionCoefficients();
    generateSamples();
    return data::LpcDecodedData(samples, bitsPerSample);
}
}