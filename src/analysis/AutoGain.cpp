#include "AutoGain.h"
#include <algorithm>
// AutoGain.cpp
AutoGain::AutoGain(size_t bandCount, float initialCeiling, float floorDb,
                   float attack, float release, float blend)
    : perBandCeilings(bandCount, initialCeiling)
    , sharedCeiling(initialCeiling)
    , floorDb(floorDb)
    , attack(attack)
    , release(release)
    , blend(std::clamp(blend, 0.0f, 1.0f))
{}

float AutoGain::advanceCeiling(float& ceiling, float targetDb) const {
    if (targetDb > ceiling) {
        ceiling += attack * (targetDb - ceiling);
    } else {
        ceiling += (1.0f - release) * (targetDb - ceiling);
    }
    return ceiling;
}

void AutoGain::normalize(const float* bandDb, float* outNormalized, size_t bandCount) {
    // advance the shared ceiling using this frame's loudest band
    float frameMax = *std::max_element(bandDb, bandDb + bandCount);
    advanceCeiling(sharedCeiling, frameMax);

    for (size_t i = 0; i < bandCount; i++) {
        // advance this band's own ceiling
        advanceCeiling(perBandCeilings[i], bandDb[i]);

        // blend the two ceilings per band
        float effectiveCeiling = (1.0f - blend) * perBandCeilings[i] + blend * sharedCeiling;

        if (effectiveCeiling <= floorDb) {
            outNormalized[i] = 0.0f;
            continue;
        }
        float normalized = (bandDb[i] - floorDb) / ((effectiveCeiling + headroomDb) - floorDb);
        outNormalized[i] = std::clamp(normalized, 0.0f, 1.0f);
    }
}