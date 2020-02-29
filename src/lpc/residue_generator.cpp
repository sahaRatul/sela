#include "../include/lpc.hpp"

namespace lpc {
ResidueGenerator::ResidueGenerator(data::LpcDecodedData& data)
    : samples(data.samples)
    , linearPredictor(*(new LinearPredictor()))
    , bitsPerSample(data.bitsPerSample)
    , quantizationFactor(32767)
{
}

inline void ResidueGenerator::quantizeSamples()
{
}

inline void ResidueGenerator::generateAutoCorrelation()
{
}

inline void ResidueGenerator::generateReflectionCoefficients()
{
}

inline void ResidueGenerator::generateoptimalLpcOrder()
{
}

inline void ResidueGenerator::quantizeReflectionCoefficients()
{
}

inline void ResidueGenerator::generateResidues()
{
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