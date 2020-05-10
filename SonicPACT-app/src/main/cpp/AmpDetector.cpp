//
// Created by Michael Specter on 5/8/20.
//

#include "AmpDetector.h"


float ExpAvg(float sample, float avg, float w) {
    return w*sample + (1-w) * avg;
}


void AmpDetector::updateComparators(float sample, uint64_t index){
    for (int i = 0; i < COMPARATORS; i++){
        float theta = 2.0f * PI * (mFrequency-90*i);
        float L1_T = cosf(index * theta);
        float L2_T = sinf(index * theta);
        float L_T = L1_T * sample;
        float Q_T = L2_T * sample;
        if (last_LTAVG_comparators[i] == 0.0f) {
            last_LTAVG_comparators[i] = L_T;
            last_QTAVG_comparators[i] = Q_T;
        }
        float i_T = ExpAvg(L_T, last_ITAVG, BACKOFF_WEIGHT);
        float q_T = ExpAvg(Q_T, last_QTAVG, BACKOFF_WEIGHT);

        last_ITAVG = i_T;
        last_QTAVG = q_T;
        float p_T = i_T * i_T + q_T * q_T;

        if (last_pTAVG == 0.0f)
            last_pTAVG = p_T;

        p_T_comparators[i] = ExpAvg(p_T, last_pTAVG, 0.0015);
    }
}

float AmpDetector::avgComparators(){
    float avg = 0;

    for (int i = 0; i < COMPARATORS; i++){
        avg += p_T_comparators[i];
    }

    avg /= COMPARATORS;

    return avg;
}

void AmpDetector::dump(){
    /*
    results.push_back(sample);
    results.push_back(I_T);
    results.push_back(Q_T);
      results.push_back(i_T);
    results.push_back(q_T);
    results.push_back(p_T);
     */

    for (auto i : timefloats)
        __android_log_print(ANDROID_LOG_ERROR, "GOAL:",
                "SAMPLE %f, I_T %f, Q_T %f, i_T %f, q_T %f, p_T %f",
               i[0],i[1],i[2],i[3],i[4],i[5]);
    return;
}

void AmpDetector::update(float sample, uint64_t index, uint64_t timestamp) {
    updateComparators(sample, index);
    std::vector<float> results;


    float omega = 2.0f * PI * mFrequency;

    //theta += angle_increment;

    time += 1/48000.0f;

    float I_T = cosf(time * omega) * sample;
    float Q_T = sinf(time * omega) * sample;

    results.push_back(sample);
    results.push_back(I_T);
    results.push_back(Q_T);

    if (last_ITAVG == -100000.0f) {
        last_ITAVG = I_T;
        last_QTAVG = Q_T;
    }

    float i_T = ExpAvg(I_T, last_ITAVG, BACKOFF_WEIGHT);
    float q_T = ExpAvg(Q_T, last_QTAVG, BACKOFF_WEIGHT);
    results.push_back(i_T);
    results.push_back(q_T);

    last_ITAVG = i_T;
    last_QTAVG = q_T;

    float p_T = i_T * i_T + q_T * q_T;

    results.push_back(p_T);
    timefloats.push_back(results);


    if (last_pTAVG == -100000.0f)
        last_pTAVG = p_T;

    //p_T = ExpAvg(p_T, last_pTAVG, BACKOFF_WEIGHT);
    last_pTAVG = p_T;

    if (p_T > .0001) {
        threshold_count++;
        //if(threshold_count == 200)

        //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT",
        //        "Broadcast threshold exceeded at time %llu, value: %f, avg nearby: %f", timestamp, p_T, avgComparators());
    } else {
        if (threshold_count >= 200)
            __android_log_print(ANDROID_LOG_ERROR, "THRESHOLD LOWERED:", "%llu, %llu, %f\n",
                                threshold_count, timestamp, p_T);

        threshold_count = 0;
    }
}

