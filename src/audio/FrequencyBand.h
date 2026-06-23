//
// Created by delphi on 6/22/26.
//

#ifndef SMFL_RTOS_AUDIO_VIS_FREQUENCYBANDS_H
#define SMFL_RTOS_AUDIO_VIS_FREQUENCYBANDS_H
#include <vector>
#include <algorithm>

struct FrequencyBand {
    float lowHz;
    float highHz;
    const char* name;
};


//TODO: Maybe mess with the bands a bit more to isolate vocal ranges to a band
static const FrequencyBand BANDS[6] = {
    { 20.0f,    80.0f,    "Sub-bass" },
    { 80.0f,    250.0f,   "Bass" },
    { 250.0f,   600.0f,   "Low mids" },
    { 600.0f,   1500.0f,  "Mids" },
    { 1500.0f,  4000.0f,  "Upper mids" },
    { 4000.0f,  20000.0f, "Presence/Brilliance" },
};

float averageBandMagnitude(const std::vector<float>& mags, const FrequencyBand& band, float sampleRate, size_t fftLength) {
    // convert Hz range to bin index range
    size_t lowBin  = static_cast<size_t>(band.lowHz  * fftLength / sampleRate);
    size_t highBin = static_cast<size_t>(band.highHz * fftLength / sampleRate);
    highBin = std::min(highBin, mags.size() - 1);

    if (lowBin > highBin) return 0.0f;

    float sum = 0.0f;
    for (size_t i = lowBin; i <= highBin; i++) {
        sum += mags[i];
    }
    return sum / static_cast<float>(highBin - lowBin + 1);
}

float magnitudeToDb(float magnitude) {
    const float epsilon = 1e-6f;  // avoid log10(0) = -infinity
    return 20.0f * std::log10(magnitude + epsilon);
}

float normalizeDb(float db, float floorDb, float ceilingDb) {
    float clamped = std::clamp(db, floorDb, ceilingDb);
    return (clamped - floorDb) / (ceilingDb - floorDb);  // 0.0 at floor, 1.0 at ceiling
}

float magToNormalDb(float magnitude, float floorDb, float ceilingDb) {
    return normalizeDb(magnitudeToDb(magnitude), floorDb, ceilingDb);
}

std::vector<FrequencyBand> generateBands(size_t numBands, float minHz, float maxHz) {
    std::vector<FrequencyBand> bands(numBands);

    if (numBands == 6) {
        for (size_t i = 0; i < 6; i++) {
            bands[i] = BANDS[i];
        }
        return  bands;
    }

    float minLog = std::log2(minHz);
    float maxLog = std::log2(maxHz);
    float stepLog = (maxLog - minLog) / static_cast<float>(numBands);

    for (size_t i = 0; i < numBands; i++) {
        float lowLog  = minLog + i * stepLog;
        float highLog = minLog + (i + 1) * stepLog;
        bands[i] = { std::pow(2.0f, lowLog), std::pow(2.0f, highLog), "" };
    }

    return bands;
}


#endif //SMFL_RTOS_AUDIO_VIS_FREQUENCYBANDS_H
