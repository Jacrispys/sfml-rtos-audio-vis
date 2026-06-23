#ifndef SMFL_RTOS_AUDIO_VIS_AUTOGAIN_H
#define SMFL_RTOS_AUDIO_VIS_AUTOGAIN_H
#include <vector>
#include <cstddef>

// AutoGain.h
class AutoGain {
private:
    std::vector<float> perBandCeilings;
    float sharedCeiling;
    float floorDb;
    float attack;
    float release;
    float headroomDb = 5.0f;
    float blend;   // 0.0 = pure per-band, 1.0 = pure shared

    float advanceCeiling(float& ceiling, float targetDb) const;

public:
    AutoGain(size_t bandCount,
             float initialCeiling = 0.0f,
             float floorDb = -60.0f,
             float attack = 0.5f,
             float release = 0.9995f,
             float blend = 0.5f);

    void normalize(const float* bandDb, float* outNormalized, size_t bandCount);
};

#endif //SMFL_RTOS_AUDIO_VIS_AUTOGAIN_H
