#include "FFT.h"

FFT::FFT(const size_t n) {
    fftLength = n;
    powerOfTwo = static_cast<int>(std::log2(n));

    if (n != (1u << powerOfTwo)) {
        throw std::invalid_argument("FFT length must be a power of two");;
    }

    reverseTable.resize(fftLength, 0);
    cosTable.resize(fftLength, 0.0f);
    sinTable.resize(fftLength, 0.0f);
    spectrumData.resize(fftLength);
    magnitudes.resize(fftLength / 2, 0.0f);

    buildReverseTable();

    for (size_t i = 1; i < fftLength; i++) {
        sinTable[i] = std::sin(-static_cast<float>(std::numbers::pi) / i);
        cosTable[i] = std::cos(-static_cast<float>(std::numbers::pi) / i);
    }

}

void FFT::buildReverseTable() {
    // set up the bit reversing table
    reverseTable[0] = 0;
    for (int limit = 1, bit = fftLength / 2; limit < fftLength; limit <<= 1, bit >>= 1)
        for (int i = 0; i < limit; i++)
            reverseTable[i + limit] = reverseTable[i] + bit;
}

void FFT::applyWindow() {
    constexpr float TWO_PI = 2.0f * static_cast<float>(std::numbers::pi);
    for (size_t i = 0; i < fftLength; i++) {
        float coeff = 0.54f - 0.46f * std::cos(TWO_PI * i / (fftLength - 1));
        spectrumData[i] *= coeff;
    }
}

void FFT::bitReverseSamples() {
    std::vector<std::complex<float>> tmp(fftLength);
    for (size_t i = 0; i < fftLength; i++) {
        tmp[i] = spectrumData[reverseTable[i]];
    }
    spectrumData = std::move(tmp);
}

void FFT::computeFFT() {
    for (size_t halfSize = 1; halfSize < fftLength; halfSize*= 2) {
        float phaseShiftR = cosTable[halfSize];
        float phaseShiftI = sinTable[halfSize];

        float currShiftR = 1.0f;
        float currShiftI = 0.0f;

        for (size_t fftStep = 0; fftStep < halfSize; fftStep++) {
            for (size_t i = fftStep; i < fftLength; i+= 2 * halfSize) {
                size_t off = i + halfSize;

                float tr = (currShiftR * spectrumData[off].real()) - (currShiftI * spectrumData[off].imag());
                float ti = (currShiftR * spectrumData[off].imag()) + (currShiftI * spectrumData[off].real());

                spectrumData[off] = {spectrumData[i].real() - tr, spectrumData[i].imag() - ti};
                spectrumData[i] = {spectrumData[i].real() + tr, spectrumData[i].imag() + ti};

            }

            float tmpR = currShiftR;
            currShiftR = (tmpR * phaseShiftR) - (currShiftI * phaseShiftI);
            currShiftI = (tmpR * phaseShiftI) + (currShiftI * phaseShiftR);
        }
    }
}

void FFT::process(const float* samples) {
    for (size_t i = 0; i < fftLength; i++) {
        spectrumData[i] = {samples[i], 0.0f};
    }

    applyWindow();
    bitReverseSamples();
    computeFFT();

    for (size_t i = 0; i < fftLength / 2; i++) {
        magnitudes[i] = std::sqrt(
            spectrumData[i].real() * spectrumData[i].real() +
            spectrumData[i].imag() * spectrumData[i].imag()
        );
    }
}

float FFT::binToFrequency(size_t bin, float sampleRate) const {
    return static_cast<float>(bin) * sampleRate / static_cast<float>(fftLength);
}
