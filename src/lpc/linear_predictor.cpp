#include "../include/lpc.hpp"

namespace lpc {
LinearPredictor::LinearPredictor()
{
    reflectionCoefficients.reserve(MAX_LPC_ORDER);
    optimalLpcOrder = 1;
}

LinearPredictor::LinearPredictor(std::vector<int32_t> quantizedReflectionCoefficients, uint8_t optimalLpcOrder)
    : optimalLpcOrder(optimalLpcOrder)
    , quantizedReflectionCoefficients(quantizedReflectionCoefficients)
{
    reflectionCoefficients.reserve(MAX_LPC_ORDER);
}

void LinearPredictor::dequantizeReflectionCoefficients()
{
    if (optimalLpcOrder <= 1) {
        reflectionCoefficients.push_back(0);
        return;
    }
    reflectionCoefficients.push_back(firstOrderCoefficients[quantizedReflectionCoefficients[0] + 64]);
    reflectionCoefficients.push_back(secondOrderCoefficients[quantizedReflectionCoefficients[1] + 64]);
    for (uint8_t i = 2; i < optimalLpcOrder; i++) {
        reflectionCoefficients.push_back(higherOrderCoefficients[quantizedReflectionCoefficients[i] + 64]);
    }
}

void LinearPredictor::generatelinearPredictionCoefficients()
{
    linearPredictionCoefficients.reserve((size_t)(optimalLpcOrder + 1));
    double linearPredictionCoefficientMatrix[MAX_LPC_ORDER][MAX_LPC_ORDER];
    double lpcTmp[MAX_LPC_ORDER];
    const uint64_t correction = (uint64_t)1 << CORRECTION_FACTOR;

    // Generate LPC matrix
    for (uint8_t i = 0; i < optimalLpcOrder; i++) {
        lpcTmp[i] = reflectionCoefficients[i];
        int32_t i2 = i >> 1;
        int32_t j = 0;
        for (j = 0; j < i2; j++) {
            double tmp = lpcTmp[j];
            lpcTmp[j] += reflectionCoefficients[i] * lpcTmp[i - 1 - j];
            lpcTmp[i - 1 - j] += reflectionCoefficients[i] * tmp;
        }
        if (i % 2 == 1) {
            lpcTmp[j] += lpcTmp[j] * reflectionCoefficients[i];
        }

        for (j = 0; j <= i; j++) {
            linearPredictionCoefficientMatrix[i][j] = -lpcTmp[j];
        }
    }

    // Select optimum order row from matrix
    linearPredictionCoefficients.push_back(0);
    for (int i = 0; i < optimalLpcOrder; i++) {
        linearPredictionCoefficients.push_back((int64_t)(correction * linearPredictionCoefficientMatrix[optimalLpcOrder - 1][i]));
    }
}
}