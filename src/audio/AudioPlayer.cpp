#include "AudioPlayer.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

AudioPlayer::AudioPlayer(const char* file_path, AudioRingBuffer& ringBuffer): ringBuffer(ringBuffer)  {
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 48000);
    if (ma_decoder_init_file(file_path, &decoderConfig, &decoder) != MA_SUCCESS) {
        throw std::runtime_error("Failed to load audio file");
    }

    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 48000;
    config.dataCallback = AudioPlayer::data_callback;
    config.pUserData = this;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize the audio device");
    }

};

void AudioPlayer::data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    auto* self = static_cast<AudioPlayer*>(pDevice->pUserData);

    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&self->decoder, pOutput, frameCount,  &framesRead);


    if (framesRead < frameCount) {
        // zero out remaining frames after file ends
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(self->decoder.outputFormat, self->decoder.outputChannels);
        memset((char*) pOutput + framesRead * bytesPerFrame, 0, (frameCount - framesRead) * bytesPerFrame);
        self->hasFrames.store(false, std::memory_order_release);
    }

    //mix stereo down to mono for average L/R in vis
    const float* stereo = static_cast<const float*>(pOutput);
    float mono[framesRead];
    for (ma_uint64 i = 0; i < framesRead; i++) {
        mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
    }
    self->ringBuffer.writeSamples(mono, framesRead);
    (void)pInput;
}

void AudioPlayer::play() {
    if (ma_device_start(&device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to start the audio device");
    }
}

void AudioPlayer::stop() {
    ma_device_stop(&device);
}

void AudioPlayer::waitUntilFinished() {
    while (hasFrames.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    stop();
}

AudioPlayer::~AudioPlayer() {
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
}
