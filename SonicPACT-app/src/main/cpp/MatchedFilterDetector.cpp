//
// Created by Michael Specter on 5/24/20.
//

#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "MatchedFilterDetector.h"
#include "Logging.h"
#include "Timing.h"
#include <thread>
#include <fstream>

using namespace cv;

void MatchedFilterDetector::update(float *samples, int32_t numFrames, uint64_t timestamp) {

    timestamps[buffer_index] = timestamp;
    cv::Mat* wave_buffer = &wave_buffers[current_buffer];
    float* wave_buffer_ptr = reinterpret_cast<float *>(wave_buffer->ptr(0));

    if (buffer_index + numFrames < BUFF_LEN) {
        // The broadcast cannot possibly be clipped
        // Optimization: memcpy tends to be far faster than a loop, so, if we're guaranteed
        // to be within the bounds, just memcpy

        size_t size = numFrames * sizeof(float);
        memcpy(&(wave_buffer_ptr[buffer_index]), samples, size);
        buffer_index += numFrames;
        return;
    }

    // memcpy last few into buffer:
    size_t copy_len = (BUFF_LEN - buffer_index) * sizeof(float);
    memcpy(&(wave_buffer_ptr[buffer_index]), samples, copy_len);

    // Copy overlapping samples into new buffer:
    float* src_buffer  = &wave_buffer_ptr[BEGIN_BUFF_OVERLAP];
    auto* dest_buffer = reinterpret_cast<float *>(wave_buffers[next_buffer].ptr(0));
    memcpy(dest_buffer, src_buffer, SAMPLES_PER_BROADCAST_FLT);

    // Ensure that the previous detection is over:
    while(buffer_needs_analysis){
        std::chrono::nanoseconds wait(1);
        std::this_thread::sleep_for(wait);
    }

    // Tells the nanny thread what buffer to analyze
    target_buffer = wave_buffer;

    // Alert the Nanny thread that it needs to begin analyzing the new buffer:
    old_timestamps = timestamps;

    // update the buffers:
    // *should* already have overlapping samples
    buffer_index   = SAMPLES_PER_BROADCAST;
    current_buffer = (current_buffer + 1) % NUM_BUFFERS;
    next_buffer    = (next_buffer    + 1) % NUM_BUFFERS;
    buffer_needs_analysis = true;

    // copy over timestamps as required
    timestamps.clear();
    for (auto const& x : old_timestamps){
        // index into the array that the timestamp occurs
        int index = x.first;
        if (index > BEGIN_BUFF_OVERLAP)
            timestamps[index - BEGIN_BUFF_OVERLAP] = x.second;
    }

    wave_buffer      = &wave_buffers[current_buffer];
    wave_buffer_ptr  = reinterpret_cast<float *>(wave_buffer->ptr(0));

    // memcpy the rest of the samples into the next buffer
    memcpy(&wave_buffer_ptr[buffer_index], &samples[copy_len], copy_len - numFrames);
}

void MatchedFilterDetector::runNannyThread() {
    LOGE("BEGINNING NANNY THREAD");
    std::chrono::nanoseconds wait(1);
    this->noise = 1000000.0f;

    // zero init
    match_result bcast_result = match_result();
    match_result recv_result = match_result();

    size_t length = broadcast_wave_to_match.cols * sizeof(float) * 2;

    while (should_continue_detecting){

        if (buffer_needs_analysis) {
            // Strategy:
            // listen for a broadcast first, if that trips, then remove those samples
            // if the broadcast wave has been hit, remove the bcast samples

            runDetect(target_buffer, &broadcast_wave_to_match, last_broadcast_seen_finished, 10, &bcast_result);

            if (bcast_result.succeeded) {
                // Broadcast detected, clear it from the target buffer
                auto* res = reinterpret_cast<float *>(target_buffer->ptr(0));
                /// Issue: begin loc is going to be after the beginning of the broadcast by a little
                /// remove a bit before the beginning as well, will get rid of earlier
                auto len = length+bcast_result.begin_location*sizeof(float);

                if (len > BUFF_LEN)
                    len = BUFF_LEN;

                memset(&res[0], 1, len);

                // clear from secondary buffer as well if necessary
                auto end_location = bcast_result.begin_location + SAMPLES_PER_BROADCAST;
                if (end_location > BEGIN_BUFF_OVERLAP){
                    Mat* wave_buffer = &wave_buffers[current_buffer];
                    auto* buf = reinterpret_cast<float *>(wave_buffer->ptr(0));
                    int begin = bcast_result.begin_location - BEGIN_BUFF_OVERLAP;
                    auto len = length;
                    if (begin < 0){
                        len += begin * sizeof(float);
                        begin = 0;
                    }

                    memset(&buf[begin], 0, len);

                    LOGE("Cleared part of 2nd buffer %d:%d", begin/sizeof(float), (begin+len)/sizeof(float));
                }

                last_broadcast_seen_finished = bcast_result.broadcast_finished;

                LOGE("bcast cols %d", SAMPLES_PER_BROADCAST);

                //dumpBuffer(target_buffer, "/sdcard/bcast_" + std::to_string(last_broadcast_seen_finished) + ".txt");
                print_result(&bcast_result);

                LOGE("^^ broadcast");
            }

            runDetect(target_buffer, &recv_wave_to_match, last_recv_seen, 2, &recv_result);
            if (recv_result.succeeded && ! bcast_result.succeeded){
                if (recv_result.broadcast_finished - last_recv_seen > 2e6){
                    last_recv_seen = recv_result.broadcast_finished;
                    print_result(&recv_result);
                    LOGE("%llu", last_recv_seen - last_broadcast_seen_finished);
                    LOGE("noise %f", noise);
                    LOGE("match max %f", recv_result.match_max);
                    LOGE("SNR %f", 10*log(sqrt(recv_result.match_max)/noise));
                    LOGE("^^ recv");
                    //dumpBuffer(target_buffer, "/sdcard/recv_" + std::to_string(last_recv_seen) + ".txt");
                }

            }
            if (!recv_result.succeeded && bcast_result.succeeded){
                // estimate Noise
                broadcast_noise_avg = bcast_result.mean;
                recv_noise_avg = recv_result.mean;
            }

            if (!recv_result.succeeded && !bcast_result.succeeded){

                if(noise > 10000)
                    noise = bcast_result.mean;
                else
                    noise = (noise + recv_result.mean + bcast_result.mean) / 3.0f;

                if (noise < 0.00001f)
                    noise = 0.00001f;
            }

            buffer_needs_analysis = false;
            target_buffer->setTo(0);
        }

        std::this_thread::sleep_for(wait);
    }

    LOGE("NANNY THREAD EXIT");
}

void MatchedFilterDetector::runDetect(Mat* data, Mat* wave_to_match, uint64_t last_seen, uint64_t thresh, match_result* result){
    // Returns a timestamp if detected, or a -1.
    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    cv::Scalar mean;
    cv::Scalar stddev;
    uint64_t closest_timestamp;
    int timestamp_index;
    // error handling
    if (!data || data->rows == 0)
        return;

    if (!wave_to_match|| wave_to_match->rows == 0)
        return;


    // initialize the match result as zero:
    memset(result, 0, sizeof(match_result));

    cv::matchTemplate(*data, *wave_to_match, result_buffer, cv::TM_CCORR);

    // square the result:
    result_buffer = result_buffer.mul(result_buffer);

    /// Localizing the best match with minMaxLoc
    cv::minMaxLoc(result_buffer, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
    meanStdDev(result_buffer, mean, stddev);

    // Threshold based on noise data:
    cv::Mat dst2 = result_buffer > noise*thresh;
    double minVal2 = 0;
    double maxVal2 = 0;
    cv::Point minLoc2;
    cv::Point maxLoc2;
    cv::minMaxLoc(dst2, &minVal2, &maxVal2, &minLoc2, &maxLoc2);
    double maxValResult = result_buffer.at<float>(0, maxLoc2.x);


    if (maxVal2 == 255) {
        //LOGE("maxval match! loc: (%d, %d), %f, %f\n",maxLoc2.x, maxLoc.x, maxValResult, noise );
    }


    // If the beginning of the broadcast starts at a point where we'd probably be clipped, skip
    if (maxLoc.x > BEGIN_BUFF_OVERLAP)
        return;


    // get the time index right below the maxloc:
    auto iterator = old_timestamps.lower_bound(maxLoc.x);
    iterator++;
    timestamp_index   = iterator->first;
    closest_timestamp = iterator->second;

    // calculate when the buffer actually began by the timer offset in the buffer
    // WE ASSUME that buffer contents are recvd in near-real time
    auto ns_into_buf = (timestamp_index - maxLoc.x) * 1e+9 / SAMPLE_RATE;
    //auto ns_into_buf = 0;

    // Time that the *head* of the transmission was seen
    auto broadcast_started = (uint64_t)(closest_timestamp - ns_into_buf);
    //uint64_t broadcast_started = closest_timestamp;

    auto wave_len = (float)wave_to_match->cols / SAMPLE_RATE * 1e+9; // waves / ns

    // Time that the *tail* of the transmission was seen
    // Time that the transmission finished
    auto broadcast_finished = (uint64_t)(broadcast_started + wave_len);

    if (broadcast_started - last_seen < 1000000 || broadcast_started < last_seen){
        // We just saw a signal less than 1 MS ago, skip!
        return;
    }

    result->match_max = maxVal;
    result->stddev = stddev[0];
    result->mean = mean[0];

    // show detection
    //if (maxVal > thresh) {
    if (maxVal2 == 255) {
        result->broadcast_started  = broadcast_started;
        result->broadcast_finished = broadcast_finished;
        result->succeeded = true;
        result->begin_location = maxLoc.x;

        auto iterator2 = iterator;
        iterator2--;
        for (auto foo : old_timestamps){
            if (std::abs(foo.first - iterator->first) < 500 )
                LOGE("time: %d, %llu", foo.first, foo.second);
        }

        LOGE("NS_INTO_BUF : %f ", ns_into_buf*1e-6);
        LOGE("WAVE_LEN : %f ", wave_len);
        LOGE("timestamp1 : %d, %llu", iterator->first, iterator->second);
        LOGE("timestamp2 : %d, %llu", iterator2->first, iterator2->second);
        LOGE("diff : %f",  (iterator2->second - iterator->second)*1e-6);
    }
    return;
}

void MatchedFilterDetector::print_result(match_result *result) {
    LOGE("MATCH! match began %llu, broadcast finished %llu, Max:%f, Mean: %f, std: %f, loc: %d\n",
         result->broadcast_started, result->broadcast_finished, result->match_max, result->mean,
         result->stddev, result->begin_location);
}

void MatchedFilterDetector::setIsLeader(bool isLeader) {
    LOGE("MatchFilterDetector setting Leader %d", isLeader);

    this->isLeader = isLeader;

    std::vector<float> LEADER   = GenerateRandom("leader");
    std::vector<float> FOLLOWER = GenerateRandom("follower");

    if(isLeader)
        init(LEADER, FOLLOWER);
    else
        init(FOLLOWER, LEADER);

}

void MatchedFilterDetector::dumpBuffer(cv::Mat* mat, std::string filename) {
    std::ofstream myfile (filename);

    if (!myfile.is_open()){
        LOGE("FAILED TO OPEN FILE");
        return;
    }

    auto* wave_buffer_ptr = reinterpret_cast<float *>(mat->ptr(0));

    for (int i = 0; i < mat->cols; i++){
        myfile << std::to_string(wave_buffer_ptr[i]);
        myfile << "\n";
    }

    myfile.close();
}

