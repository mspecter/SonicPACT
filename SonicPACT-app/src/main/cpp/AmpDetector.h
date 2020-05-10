//
// Created by Michael Specter on 5/8/20.
//

#ifndef SONICPACT_AMPDETECTOR_H
#define SONICPACT_AMPDETECTOR_H
#include <oboe/Oboe.h>
#include <math.h>
#include <android/log.h>
#include <vector>

#include "kissfft/tools/kiss_fftr.h"
#include "kissfft/kiss_fft.h"


class AmpDetector {

public:
    AmpDetector(float frequency){
        mFrequency = frequency;
        reset();
    }

    void reset(){
        /*
        last_ITAVG = 0.0f;
        last_QTAVG = 0.0f;
        last_pTAVG = 0.0f;
        for (int i = 0; i < COMPARATORS; i ++){
            last_LTAVG_comparators[i] = 0.0;
            last_QTAVG_comparators[i] = 0.0;
            p_T_comparators[i] = 0.0;
        }
        threshold_count = 0;
        */
    }

    void update(float sample, uint64_t index, uint64_t timestamp);

private:
    float mFrequency;
    float last_ITAVG = -100000.0f;
    float last_QTAVG = -100000.0f;
    float last_pTAVG = -100000.0f;
    uint64_t threshold_count = 0;

    float BACKOFF_WEIGHT = 0.002;
    constexpr static float PI = 3.14159265358979323846f;
    constexpr static int COMPARATORS = 10;

    float last_LTAVG_comparators[COMPARATORS] = {0.0};
    float last_QTAVG_comparators[COMPARATORS] = {0.0};
    float p_T_comparators[COMPARATORS] = {0.0};
    const float angle_increment = 2*PI/48000.0f; // TODO: Make global

    float theta = 0.0f;
    float time = 0.0f;
    std::vector<std::vector<float>> timefloats;

    void updateComparators(float sample, uint64_t index);

    float avgComparators();

    void dump();
};

#endif //SONICPACT_AMPDETECTOR_H
