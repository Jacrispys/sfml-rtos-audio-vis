//
// Created by delphi on 6/12/26.
//

#ifndef SMFL_RTOS_AUDIO_VIS_FFT_H
#define SMFL_RTOS_AUDIO_VIS_FFT_H
#include <complex>
#include <vector>


class FFT {
public:
    explicit FFT(size_t n);
    void process(const float* samples);
    const std::vector<float>& getMagnitudes() const {return magnitudes;};
    float binToFrequency(size_t bin, float sampleRate) const;

private:
    size_t fftLength;
    int powerOfTwo;
    std::vector<int> reverseTable;
    std::vector<float> cosTable;
    std::vector<float> sinTable;
    std::vector<std::complex<float>> spectrumData;
    std::vector<float> magnitudes;

    void buildReverseTable();
    void applyWindow();
    void bitReverseSamples();
    void computeFFT();
};


#endif //SMFL_RTOS_AUDIO_VIS_FFT_H