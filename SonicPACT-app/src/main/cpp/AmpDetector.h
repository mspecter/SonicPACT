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
#include "IncrementalLPF.h"


class AmpDetector {

public:
    AmpDetector(float frequency, float sampleRate){
        mFrequency = frequency;
        mSampleRate = sampleRate;
        i_lpf = new IncrementalLPF(frequency, sampleRate, CYCLES_TO_AVERAGE);
        q_lpf = new IncrementalLPF(frequency, sampleRate, CYCLES_TO_AVERAGE);
        reset();
    }

    void reset(){
    }

    void update(float sample, uint64_t timestamp);

private:
    float mFrequency;
    float last_ITAVG = -100000.0f;
    float last_QTAVG = -100000.0f;
    float last_pTAVG = -100000.0f;

    int CYCLES_TO_AVERAGE = 5;
    float t = 0;

    uint64_t threshold_count = 0;

    float BACKOFF_WEIGHT = 0.002;
    constexpr static float PI = 3.14159265358979323846f;
    constexpr static int COMPARATORS = 10;

    float last_LTAVG_comparators[COMPARATORS] = {0.0};
    float last_QTAVG_comparators[COMPARATORS] = {0.0};
    float p_T_comparators[COMPARATORS] = {0.0};

    float theta = 0.0f;
    float time = 0.0f;

    void updateComparators(float sample, uint64_t index);

    float avgComparators();

    void dump();

    float mSampleRate;
    IncrementalLPF *i_lpf;
    IncrementalLPF *q_lpf;

    float principal_value_zero_to_2pi(float i);

    int state;

    void display_flip(double min_time, float t, float i, float q);

    bool state_updated = false;
    bool badstate;
    int run =0;
    int samps_since_flip = 0;


    uint64_t transition_window = 0;
    uint64_t current_symbol = 0;
    int current_bias = 0;
    int last_bit = 0;
    int current_run = 0;

    std::bitset<13> bitstring = {0};
    int skip =0 ;
    //uint64_t bitstring;
};

#endif //SONICPACT_AMPDETECTOR_H
