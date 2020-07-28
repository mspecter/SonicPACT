//
// Created by Michael Specter on 5/24/20.
//

#ifndef SONICPACT_MATCHEDFILTERDETECTOR_H
#define SONICPACT_MATCHEDFILTERDETECTOR_H
#include <oboe/Oboe.h>
#include <math.h>
#include <android/log.h>
#include <vector>
#include <map>

#include <opencv2/imgproc/imgproc.hpp>
#include <thread>
#include "IncrementalLPF.h"
#include "BPSKSignalGenerator.h"
#include "Logging.h"
#include "Timing.h"
#include "RandomNoise.h"


class MatchedFilterDetector{

public:
    ~MatchedFilterDetector(){
        should_continue_detecting = false;
        myThread->join();
    }

    MatchedFilterDetector(float broadcast_frequency, float recv_frequency, float sampleRate){
        broadcast_freq = broadcast_frequency;
        recv_freq      = recv_frequency;

        std::vector<float> broadcast_wave =  GenerateBPSKPreamble(broadcast_freq);
        std::vector<float> recv_wave      =  GenerateBPSKPreamble(recv_freq);

        LOGE("Broadcast Freq %f, recv freq %f", broadcast_frequency, recv_frequency);
        init(broadcast_wave, recv_wave);
    }

    MatchedFilterDetector(float sampleRate, bool isLeader){
        LOGE("MatchFilterDetector is Leader %d", isLeader);
        setIsLeader(isLeader);
    }

    void update(float* samples, int32_t numFrames, uint64_t timestamp);

    uint64_t last_broadcast_seen = 0;
    uint64_t last_recv_seen = 0;

    void setIsLeader(bool isLeader);

private:

    void init(std::vector<float>& broadcast_wave, std::vector<float>& recv_wave){
        std::lock_guard<std::mutex> lock(mtx);
        LOGE("INITIALIZING MATCHED FILTER DETECTOR");
        LOGE("Broadcast wave sz %d", broadcast_wave.size());
        LOGE("recv wave sz %d", recv_wave.size());

        // Stop Nanny thread:
        if (myThread != nullptr){
            should_continue_detecting = false;
            myThread->join();
        }

        LOGE("GOT HERE 1");
        // update sample numbers:
        SAMPLES_PER_BROADCAST = broadcast_wave.size();
        BEGIN_BUFF_OVERLAP = BUFF_LEN - SAMPLES_PER_BROADCAST;

        LOGE("GOT HERE 2");
        result_buffer.create(1, BUFF_LEN, CV_32F);

        // Allocate storage for the buffer:
        broadcast_wave.size();
        broadcast_wave_to_match.create(1, broadcast_wave.size(), CV_32F);
        recv_wave_to_match.create(1, recv_wave.size(), CV_32F);

        LOGE("GOT HERE 3");
        // store in our buffer
        for (int i = 0; i < broadcast_wave.size(); i++) {
            broadcast_wave_to_match.at<float>(0, i) = broadcast_wave[i];
        }
        for (int i = 0; i < recv_wave.size(); i++) {
            recv_wave_to_match.at<float>(0, i) = recv_wave[i];
        }

        LOGE("GOT HERE 4");
        LOGE("END INIT MATCHED FILTER");
        // kick off nanny thread:
        myThread = new std::thread(&MatchedFilterDetector::runNannyThread, this);
    }

    struct match_result {
        bool succeeded;
        uint64_t broadcast_started;  // Time the broadcast started
        uint64_t broadcast_finished; // Time the broadcast finished
        uint64_t match_max; // max matched value
        float mean;         // average value
        float stddev;       // STDDeviation
        int begin_location;
    };

    static const int BUFF_LEN = 35000;
    int SAMPLES_PER_BROADCAST = (SAMPLES_PER_TRANSITION + SAMPLES_PER_SYMBOL) * PREAMBLE_LEN;
    int BEGIN_BUFF_OVERLAP = BUFF_LEN - SAMPLES_PER_BROADCAST;
    int NUM_BUFFERS = 2;

    cv::Mat wave_buffers[2] = {cv::Mat::zeros(1, BUFF_LEN, CV_32F),
                               cv::Mat::zeros(1, BUFF_LEN, CV_32F)};
    int current_buffer = 0;
    int next_buffer = 1;

    float broadcast_freq;
    float recv_freq;
    float t = 0;

    std::mutex mtx;

    cv::Mat broadcast_wave_to_match;
    cv::Mat recv_wave_to_match;
    cv::Mat result_buffer;
    cv::Mat *target_buffer;

    std::map<int, uint64_t> timestamps;

    float theta = 0.0f;
    float time = 0.0f;
    int state;
    int buffer_index;

    std::thread *myThread = nullptr;

    std::map<int, uint64_t> old_timestamps;

    void runNannyThread();

    bool should_continue_detecting = true;
    bool buffer_needs_analysis = false;

    void print_result(match_result* result);
    void runDetect(cv::Mat *data, cv::Mat *wave_to_match, uint64_t last_seen, uint64_t thresh,
                   match_result *result);

    float recv_noise_avg;
    float broadcast_noise_avg;
};
#endif //SONICPACT_MATCHEDFILTERDETECTOR_H
