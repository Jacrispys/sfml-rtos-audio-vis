//
// Created by delphi on 6/12/26.
//

#ifndef CMAKESFMLPROJECT_AUDIOPLAYER_H
#define CMAKESFMLPROJECT_AUDIOPLAYER_H
#include <atomic>
#include <stdexcept>

#include "AudioRingBuffer.h"
#include "miniaudio.h"


class AudioPlayer {
private:
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    ma_device device;
    ma_decoder decoder;
    std::atomic<bool> hasFrames{true};
    AudioRingBuffer ringBuffer;

public:
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    explicit AudioPlayer(const char* file_path);

    void play();
    void stop();
    void waitUntilFinished();

    ~AudioPlayer();
};


#endif //CMAKESFMLPROJECT_AUDIOPLAYER_H