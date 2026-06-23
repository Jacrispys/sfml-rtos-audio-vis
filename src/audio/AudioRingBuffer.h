#ifndef SMFL_RTOS_AUDIO_VIS_AUDIORINGBUFFER_H
#define SMFL_RTOS_AUDIO_VIS_AUDIORINGBUFFER_H
#include <atomic>
#include <cstddef>
#include <cstdint>


class AudioRingBuffer {
private:
    static constexpr size_t BUFFER_CAPACITY = 8192;
    static constexpr size_t WRAP_MASK = BUFFER_CAPACITY - 1;

    std::atomic<size_t> writeIndex = 0;
    std::atomic<size_t> readIndex = 0;
    float ringBuffer[BUFFER_CAPACITY] = {};

public:
    void writeSamples(const float samples[], size_t sampleCount) {
        size_t writeIdx = writeIndex.load(std::memory_order_relaxed);
        for (size_t idx = 0; idx < sampleCount; idx++) {
            size_t nextWrite = (writeIdx + 1) & WRAP_MASK;

            size_t readIdx = readIndex.load(std::memory_order_acquire);
            if (nextWrite == readIdx) {
                readIndex.store((readIdx + 1) & WRAP_MASK, std::memory_order_release);
            }
            ringBuffer[writeIdx] = samples[idx];
            writeIdx = nextWrite;
        }
        writeIndex.store(writeIdx, std::memory_order_release);
    }

    void readSamples(float* outBuffer, size_t sampleCount) {
        size_t tmpIndex = readIndex.load(std::memory_order_relaxed);
        for (int i = 0; i < sampleCount; i++) {
            outBuffer[i] = ringBuffer[tmpIndex];
            tmpIndex = (tmpIndex + 1) & WRAP_MASK;
        }
        readIndex.store(tmpIndex, std::memory_order_release);
    }

    bool canRead(size_t sampleCount) {
        size_t w = writeIndex.load(std::memory_order_acquire);
        size_t r = readIndex.load(std::memory_order_relaxed);

        size_t available = (w - r) & WRAP_MASK;
        return available >= sampleCount;
    }
};


#endif //SMFL_RTOS_AUDIO_VIS_AUDIORINGBUFFER_H
