#include <cmath>

#include "../include/lpc.hpp"

namespace lpc {
ResidueGenerator::ResidueGenerator(data::LpcDecodedData& data)
    : samples(data.samples)
    , bitsPerSample(data.bitsPerSample)
{
}

inline void ResidueGenerator::quantizeSamples()
{
    quantizedSamples.reserve(samples.size());
    for (int32_t sample : samples) {
        quantizedSamples.push_back((double)sample / quantizationFactor);
    }
}

inline void ResidueGenerator::generateAutoCorrelation()
{
    double sum = 0;
    double mean = 0;
    autocorrelationFactors.reserve(MAX_LPC_ORDER + 1);

    // Generate Mean of samples
    for (double sample : quantizedSamples) {
        sum += sample;
    }
    mean = sum / quantizedSamples.size();

    // Generate autocorrelation coefficients
    for (size_t i = 0; i <= MAX_LPC_ORDER; i++) {
        autocorrelationFactors.push_back(0.0);
        for (size_t j = i; j < quantizedSamples.size(); j++) {
            autocorrelationFactors[i] += (quantizedSamples[j] - mean) * (quantizedSamples[j - i] - mean);
        }
    }

    // Normalise the coefficients
    for (size_t i = 1; i <= MAX_LPC_ORDER; i++) {
        autocorrelationFactors[i] /= autocorrelationFactors[0];
    }
    autocorrelationFactors[0] = 1.0;
}

inline void ResidueGenerator::generateReflectionCoefficients()
{
    double error;
    double gen[2][MAX_LPC_ORDER];

    for (size_t i = 0; i < MAX_LPC_ORDER; i++) {
        gen[0][i] = gen[1][i] = autocorrelationFactors[i + 1];
    }

    error = autocorrelationFactors[0];
    linearPredictor.reflectionCoefficients.push_back(-gen[1][0] / error);
    error += gen[1][0] * linearPredictor.reflectionCoefficients[0];

    for (size_t i = 1; i < MAX_LPC_ORDER; i++) {
        for (size_t j = 0; j < MAX_LPC_ORDER - i; j++) {
            gen[1][j] = gen[1][j + 1] + linearPredictor.reflectionCoefficients[i - 1] * gen[0][j];
            gen[0][j] = gen[1][j + 1] * linearPredictor.reflectionCoefficients[i - 1] + gen[0][j];
        }
        linearPredictor.reflectionCoefficients.push_back(-gen[1][0] / error);
        error += gen[1][0] * linearPredictor.reflectionCoefficients[i];
    }
}

inline void ResidueGenerator::generateoptimalLpcOrder()
{
    for (int64_t i = MAX_LPC_ORDER - 1; i >= 0; i--) {
        if (abs(linearPredictor.reflectionCoefficients[i]) > 0.05) {
            linearPredictor.optimalLpcOrder = (uint8_t)(i + 1);
            break;
        }
    }
}

inline void ResidueGenerator::quantizeReflectionCoefficients()
{
    linearPredictor.quantizedReflectionCoefficients.reserve((size_t)linearPredictor.optimalLpcOrder);

    if (linearPredictor.quantizedReflectionCoefficients.capacity() > 0) {
        linearPredictor.quantizedReflectionCoefficients.push_back((int32_t)floor(64 * (-1 + (SQRT2 * sqrt(linearPredictor.reflectionCoefficients[0] + 1)))));
    }
    if (linearPredictor.quantizedReflectionCoefficients.capacity() > 1) {
        linearPredictor.quantizedReflectionCoefficients.push_back((int32_t)floor(64 * (-1 + (SQRT2 * sqrt(-linearPredictor.reflectionCoefficients[1] + 1)))));
    }
    for (size_t i = 2; i < linearPredictor.optimalLpcOrder; i++) {
        linearPredictor.quantizedReflectionCoefficients.push_back((int32_t)floor(64 * linearPredictor.reflectionCoefficients[i]));
    }
}

inline void ResidueGenerator::generateResidues()
{
    residues.reserve(samples.size());
    int64_t correction = (int64_t)1 << (CORRECTION_FACTOR - 1);
    residues.push_back(samples[0]);

    for (size_t i = 1; i <= (size_t)linearPredictor.optimalLpcOrder; i++) {
        int64_t temp = correction;
        for (int j = 1; (size_t)j <= i; j++) {
            temp += linearPredictor.linearPredictionCoefficients[j] * samples[i - j];
        }
        residues.push_back(samples[i] - (int)(temp >> CORRECTION_FACTOR));
    }

    for (size_t i = linearPredictor.optimalLpcOrder + 1; i < samples.size(); i++) {
        int64_t temp = correction;
        for (size_t j = 0; j <= linearPredictor.optimalLpcOrder; j++) {
            temp += (linearPredictor.linearPredictionCoefficients[j] * samples[i - j]);
        }
        residues.push_back(samples[i] - (int)(temp >> CORRECTION_FACTOR));
    }
}

data::LpcEncodedData ResidueGenerator::process()
{
    quantizeSamples();
    generateAutoCorrelation();
    generateReflectionCoefficients();
    generateoptimalLpcOrder();
    quantizeReflectionCoefficients();
    linearPredictor.dequantizeReflectionCoefficients();
    linearPredictor.generatelinearPredictionCoefficients();
    generateResidues();
    return data::LpcEncodedData(linearPredictor.optimalLpcOrder, bitsPerSample,
        linearPredictor.quantizedReflectionCoefficients, residues);
}

}