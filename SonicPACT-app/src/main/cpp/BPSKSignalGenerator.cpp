//
// Created by Michael Specter on 5/23/20.
//

#include "BPSKSignalGenerator.h"
#include "Timing.h"
#include <opencv2/core/core.hpp>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <chrono>
#include <android/log.h>



float clamp(float x, float lowerlimit, float upperlimit) {
    if (x < lowerlimit)
        x = lowerlimit;
    if (x > upperlimit)
        x = upperlimit;
    return x;
}

float smootherstep(float edge0, float edge1, float x) {
    // Scale, and clamp x to 0..2 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.0, 2.0);
    // Evaluate polynomial
    return x * x * x * (x * (x * 6 - 15) + 10);
}

std::vector<float> GenerateBPSKPreamble(float carrier_frequency) {
    std::vector<float> wave;

    uint64_t current_index_into_symbol = 0;
    uint64_t current_bit_index         = 0;
    uint64_t current_bit               = PREAMBLE[0];

    float message_i = 0;
    float message_q = 0;
    float new_i     = 1;

    if (current_bit)
        new_i = 1;
    else
        new_i = -1;

    float transition_time = 0;
    bool new_symbol  = true;
    bool should_ramp = true;

    float phase = 0;
    float phaseIncrement = ((float)(M_PI) * 2 * carrier_frequency) / SAMPLE_RATE;

    bool keep_going = true;

    // set up wave to match against
    while(keep_going){

        // Updates i & q values
        if (!new_symbol)
            current_index_into_symbol++;

        // Change to the next symbol!
        if (current_index_into_symbol > SAMPLES_PER_SYMBOL) {
            current_index_into_symbol = 0;
            current_bit_index++;
            new_symbol = true;
            transition_time  = 0;

            if (current_bit_index >= PREAMBLE_LEN){
                new_symbol = true;
                should_ramp = true;
                //current_bit = PREAMBLE[current_bit_index];
                new_i = 0;
            }
            else if (PREAMBLE[current_bit_index] != current_bit){
                current_bit = PREAMBLE[current_bit_index];
                if (current_bit)
                    new_i = 1;
                else
                    new_i = -1;
            }
        }

        float current_i = message_i;

        if (new_symbol) {
            // If we should change phase, we need to be able to follow a transition that won't click
            // This requires using a simple s-curve (which is provided by smootherstep)

            if (should_ramp){
                // We're ramping either up from 0 to +-1 or from 1 to 0

                if (new_i != 0){
                    // we're ramping to +/-1 from 0
                    current_i += smootherstep(0, 1, 1 / SAMPLES_PER_TRANSITION * transition_time) * new_i;
                }
                else {
                    // we're ramping down to 0
                    float direction = message_i * -1;
                    current_i += smootherstep(0, 1, 1 / SAMPLES_PER_TRANSITION * transition_time) * direction;
                    if (fabs(current_i) < .0001)
                        keep_going = false;

                }

            }
            else
                // Ramps from -1 to 1 or 1 to -1
                current_i += smootherstep(0, 1, 1 / SAMPLES_PER_TRANSITION * transition_time) * 2 * new_i;

            if ( current_i>= 1)
                current_i= 1;
            if ( current_i<= -1)
                current_i= -1;

            transition_time++;

            if (transition_time >= SAMPLES_PER_TRANSITION){
                message_i = new_i;
                current_i = new_i;
                new_symbol = false;
                should_ramp = false;
                transition_time = 0;
            }
        }

        // actual modulation!
        wave.push_back(current_i * cosf(phase) + message_q * sinf(phase));
        //__android_log_print(ANDROID_LOG_ERROR, "NATIVE_PACT", "RAW BUFFER DATA %f\n",
        //                    current_i * cosf(phase));

        // Increment current phase
        phase += phaseIncrement;
        if (phase >= 2 * (float)M_PI)
            phase -= 2 * (float)M_PI;
    }

    return wave;
}