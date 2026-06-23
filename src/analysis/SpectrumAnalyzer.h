//
// Created by delphi on 6/12/26.
//

#ifndef SMFL_RTOS_AUDIO_VIS_SPECTRUMANALYZER_H
#define SMFL_RTOS_AUDIO_VIS_SPECTRUMANALYZER_H
#include <chrono>
#include <cstring>
#include <thread>

#include "FFT.h"
#include "../audio/AudioRingBuffer.h"

class SpectrumAnalyzer {
private:
    static constexpr size_t fftLength = 2048;
    FFT fft;
    AudioRingBuffer &ringBuffer;
    std::thread bufferThread;
    std::atomic<bool> running{false};

    std::vector<float> bufferA, bufferB;
    std::atomic<std::vector<float> *> currentBuffer;
    std::vector<float> *writeBuffer;
    float window[fftLength] = {};
    const size_t hopSize = 900;

    void waitForSamples() {
        while (running.load(std::memory_order_relaxed)) {
            if (ringBuffer.canRead(hopSize)) {

                std::memmove(window, window + hopSize, (fftLength - hopSize) * sizeof(float));

                ringBuffer.readSamples(window + (fftLength - hopSize), hopSize);
                consumeSamples(window);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void consumeSamples(const float *samples) {
        fft.process(samples);
        *writeBuffer = fft.getMagnitudes();
        currentBuffer.store(writeBuffer, std::memory_order_release);
        writeBuffer = (writeBuffer == &bufferA) ? &bufferB : &bufferA;
    }

public:
    explicit SpectrumAnalyzer(AudioRingBuffer &ringBuffer)
        : fft(fftLength),
          ringBuffer(ringBuffer),
          bufferA(fftLength / 2), bufferB(fftLength / 2),
          currentBuffer(&bufferA), writeBuffer(&bufferB) {
    }

    const std::vector<float> &getMagnitudes() const {
        return *currentBuffer.load(std::memory_order_acquire);
    }

    void start() {
        running.store(true, std::memory_order_relaxed);
        bufferThread = std::thread(&SpectrumAnalyzer::waitForSamples, this);
    }

    void stop() {
        running.store(false, std::memory_order_relaxed);
    }


    ~SpectrumAnalyzer() {
        stop();
        if (bufferThread.joinable()) {
            bufferThread.join();
        }
    }
};
#endif //SMFL_RTOS_AUDIO_VIS_SPECTRUMANALYZER_H
