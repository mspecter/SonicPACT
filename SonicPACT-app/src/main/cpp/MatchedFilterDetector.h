//
// Created by Michael Specter on 5/24/20.
//

#ifndef SONICPACT_MATCHEDFILTERDETECTOR_H
#define SONICPACT_MATCHEDFILTERDETECTOR_H
#include <oboe/Oboe.h>
#include <math.h>
#include <android/log.h>
#include <vector>

#include <opencv2/imgproc/imgproc.hpp>
#include "IncrementalLPF.h"
#include "BPSKSignalGenerator.h"


class MatchedFilterDetector{

public:
    MatchedFilterDetector(float frequency, float sampleRate){
        mFrequency = frequency;
        mSampleRate = sampleRate;

        // TODO: figure out if we want to filter in this as well.
        //i_lpf = new IncrementalLPF(frequency, sampleRate, CYCLES_TO_AVERAGE);
        //q_lpf = new IncrementalLPF(frequency, sampleRate, CYCLES_TO_AVERAGE);

        result_buffer.create(BUFF_LEN, 1, CV_32F);

        std::vector<float> generated_wave =  GenerateBPSKPreamble(frequency);

        // Allocate storage for the buffer:
        wave_to_match.create(generated_wave.size(), 1, CV_32F);
        wave_to_match_size = generated_wave.size();

        // store in our buffer
        for (int i =0; i < generated_wave.size(); i++) {
            wave_to_match.at<float>(i,0) = generated_wave[i];
        }
    }

    void update(float* samples, int32_t numFrames, uint64_t timestamp);

private:
    int BUFF_LEN = 10000;
    cv::Mat wave_buffer = cv::Mat::zeros(BUFF_LEN, 1, CV_32F);
    float mFrequency = 18000;
    int CYCLES_TO_AVERAGE = 5;
    float t = 0;
    cv::Mat wave_to_match;
    cv::Mat result_buffer;

    float theta = 0.0f;
    float time = 0.0f;

    float mSampleRate;
    IncrementalLPF *i_lpf;
    IncrementalLPF *q_lpf;
    int state;
    int run =0;
    int skip =0 ;


    int64_t buffer_index;
    int64_t buff_began = -1;
    unsigned int wave_to_match_size;

    void runDetect(cv::Mat *data, uint64_t timestmp);
};
#endif //SONICPACT_MATCHEDFILTERDETECTOR_H
