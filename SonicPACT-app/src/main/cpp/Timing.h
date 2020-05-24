//
// Created by Michael Specter on 5/2/20.
//

#ifndef SONICPACT_TIMING_H
#define SONICPACT_TIMING_H
#include "Constants.h"
static bool shouldTakeMeasure = false;
static long nanosecondsStart = 0;

// Sample rate
const static float SAMPLE_RATE = 48000.0;

// number of miliseconds to broadcast a symbol
const static  float SAMPLES_PER_SYMBOL = SAMPLE_RATE / 1000;

// number of samples over which to transition
const static float SAMPLES_PER_TRANSITION = SAMPLE_RATE / 2000;

static long CHIRP_LEN_NS = static_cast<long>(SAMPLES_PER_SYMBOL * 1000000 * PREAMBLE_LEN);

#endif //SONICPACT_TIMING_H
