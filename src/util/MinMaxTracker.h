//
// Created by delphi on 6/22/26.
//

#ifndef SMFL_RTOS_AUDIO_VIS_MINMAXTRACKER_H
#define SMFL_RTOS_AUDIO_VIS_MINMAXTRACKER_H

#include <vector>
#include <algorithm>

class MinMaxTracker {
private:
    std::vector<float> topValues;
    std::vector<float> bottomValues;
    static constexpr size_t TRACK_COUNT = 10;

public:
    void record(float value) {
        // maintain top 10 (largest values), sorted ascending so smallest-of-the-top is at front
        if (topValues.size() < TRACK_COUNT) {
            topValues.push_back(value);
            std::sort(topValues.begin(), topValues.end());
        } else if (value > topValues.front()) {
            topValues.front() = value;
            std::sort(topValues.begin(), topValues.end());
        }

        // maintain bottom 10 (smallest values), sorted descending so largest-of-the-bottom is at front
        if (bottomValues.size() < TRACK_COUNT) {
            bottomValues.push_back(value);
            std::sort(bottomValues.begin(), bottomValues.end(), std::greater<float>());
        } else if (value < bottomValues.front()) {
            bottomValues.front() = value;
            std::sort(bottomValues.begin(), bottomValues.end(), std::greater<float>());
        }
    }

    const std::vector<float>& getTop() const { return topValues; }
    const std::vector<float>& getBottom() const { return bottomValues; }
};

#endif //SMFL_RTOS_AUDIO_VIS_MINMAXTRACKER_H
