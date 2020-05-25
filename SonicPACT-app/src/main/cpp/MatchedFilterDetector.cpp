//
// Created by Michael Specter on 5/24/20.
//

#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "MatchedFilterDetector.h"
#include "Logging.h"
#include "Timing.h"

void MatchedFilterDetector::update(float *samples, int32_t numFrames, uint64_t timestamp) {
    // Copy into buffer
    for (int i = 0; i < numFrames; i++) {
        buffer_index++;

        if (buffer_index >= BUFF_LEN){
            // run detection algorithm
            runDetect(&wave_buffer, timestamp);

            // clear the buffer
            wave_buffer.setTo(0);
            buffer_index = 0;
            buff_began = getTimeNsec();
        }

        wave_buffer.at<float>(static_cast<int>(buffer_index), 0) = samples[i];
    }

}

void MatchedFilterDetector::runDetect(cv::Mat* data, uint64_t timestmp){
    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    cv::Point matchLoc;

    // todo: run the detection in a new thread

    cv::matchTemplate(*data, wave_to_match, result_buffer, cv::TM_CCORR);

    /// Localizing the best match with minMaxLoc

    cv::minMaxLoc(result_buffer, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
    cv::Scalar mean, stddev;
    meanStdDev(result_buffer, mean, stddev);

    // calculate when the buffer actually began by the timer offset in the buffer
    // WE ASSUME that buffer contents are recvd in near-real time


    // show detection
    if (maxVal > .4)
        LOGE("MATCH DETECTED! Max:%f, location %d, Mean: %f, std: %f, timestamp %llu\n",
             maxVal, maxLoc.y, mean[0], stddev[0], timestmp);
}
