//
// Created by Michael Specter on 5/8/20.
//
#include <cmath>
#include <random>
#include "AmpDetector.h"
#include "IncrementalLPF.h"
#include "buffer.h"
#include "Timing.h"

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



void AmpDetector::dump(){
    /*
    results.push_back(sample);
    results.push_back(I_T);
    results.push_back(Q_T);
      results.push_back(i_T);
    results.push_back(q_T);
    results.push_back(p_T);
     */
    return;
}


void AmpDetector::display_flip(double min_time, float t, float i, float q){
    if (t < min_time){
        theta = atanf(q/i);
    }
    else {
        float new_theta = atanf(q/i);
        if ( (PI - PI / 4) < principal_value_zero_to_2pi(new_theta - theta) < (PI + PI / 4) ) {
            theta = new_theta;
            if (state == 0)
                state = 1;
            else
                state = 0;
        }
    }
}

float AmpDetector::principal_value_zero_to_2pi(float input){
    if ( (input < 0.0) || (input >= 2*PI)) {
        return input - ( 2 * PI * floorf( input / (2*PI) ) );
    }
    return input;
}

uint64_t hammingDistance(uint64_t n1 , uint64_t n2)
{
    uint64_t x = n1 ^ n2;
    uint64_t setBits = 0;

    while (x > 0) {
        setBits += x & 1;
        x >>= 1;
    }

    return setBits;
}

void AmpDetector::update(float sample, uint64_t timestamp) {

    float f_c = mFrequency;
    float omega_c = 2*PI*f_c;

    t += 1 / mSampleRate;

    // Pull out sample's I & Q values
    float si = sample * cosf(omega_c * t);
    float sq = sample * sinf(omega_c * t);
    float i = i_lpf->process(si);
    float q = q_lpf->process(sq);

    // Magnitude of the current sample
    float mag = sqrtf(powf(i, 2) + powf(q, 2));

    //float mag_LPF_faster = mag_LPF_fast->process(mag);
    //float mag_LPF_slower = mag_LPF_slow->process(mag);

    if (skip > 0) {
        skip -= 1;
        return;
    }

    // Magnitude threshold cutoff for noise:
    if (mag < .00005) {
        /// Assume we're restarting the preamble from scratch
        // Reset which symbol we're on in the preamble, our bit string, and the bias of the current
        // symbol
        current_symbol = 0;
        current_bias = 0;
        //current_run = -1 * (int)(SAMPLES_PER_TRANSITION);
        current_run = 0;
        bitstring &= 0;

        // Every preamble starts with SAMPLES_PER_TRANSITION nothingness
        //current_run = -1 * (int)(SAMPLES_PER_TRANSITION);
       return;
    }

    current_run += 1;

    if (i > 0)
        current_bias += 1;
    else
        current_bias -= 1;

    // Ensures that we start on transition boundaries
    //if (abs(current_bias) < 1)
    //      current_run = 0;A


    //if (fabs(current_bias) >= SAMPLES_PER_SYMBOL ){
    if (current_run >= SAMPLES_PER_SYMBOL + SAMPLES_PER_TRANSITION){
        // Start a new symbol!

        int current_bit = 0;

        if (current_bias >= 0)
            current_bit = 1;
        else
            current_bit = 0;


        if (current_bit != last_bit)
            // There was a transition between this and the last bit
            current_run = 0;
            //current_run = -1 * (int)(SAMPLES_PER_TRANSITION)/2;
        else
            // There was no transition between this and the last bit
            current_run = 0;

        last_bit = current_bit;

        // Recording the new bit into our current bitstring
        bitstring <<= 1;
        bitstring |= current_bit;

        current_symbol ++;

        if (current_symbol >= 0){
            uint64_t x = 0b1111100110101;
            std::bitset<13> diff_bits(x);
            diff_bits ^= bitstring;
            uint64_t distance = diff_bits.count();

            if (distance <= 2){
                __android_log_print(ANDROID_LOG_ERROR,
                                    "NATIVE_PACT",
                                    "%llu, %s, %llu, %f, %d FOUND ",
                                    timestamp, bitstring.to_string().c_str(), distance, mag, current_bias);

                // Avoid multipath by skipping the next few samples
                //skip = static_cast<int>(SAMPLES_PER_SYMBOL);
            }
            else if (distance >= 11){

                __android_log_print(ANDROID_LOG_ERROR,
                                    "NATIVE_PACT",
                                    "%llu, %s, %llu, %f, %d FOUND INVERTED",
                                    timestamp, bitstring.to_string().c_str(), distance, mag, current_bias);
                // record success
                //skip = static_cast<int>(SAMPLES_PER_SYMBOL);
            }
            else {
                __android_log_print(ANDROID_LOG_ERROR,
                                    "NATIVE_PACT",
                                    "%llu, %s, %llu, %f, %d ",
                                    timestamp, bitstring.to_string().c_str(), distance, mag, current_bias);
            }

        }

        current_bias = 0;
    }
}

