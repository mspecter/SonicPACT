//
// Created by Michael Specter on 5/13/20.
//

#ifndef SONICPACT_BUFFER_H
#define SONICPACT_BUFFER_H
#include <cstdio>
#include <memory>
#include <mutex>

class TimestampSample {
public:
    float sample;
    uint64_t timestamp;
};

class RingBuffer {
public:
    RingBuffer(size_t size){
        max_size = size;
        buffer = (float *) malloc(sizeof(float)*size);
        timestamp_buffer = (uint64_t*) malloc(sizeof(uint64_t)*size);
    }

    void put(float sample, uint64_t timestamp) {
        std::lock_guard<std::mutex> lock(mutex);

        buffer[head] = sample;
        timestamp_buffer[head] = timestamp;

        if(full)
            tail = (tail + 1) % max_size;

        head = (head + 1) % max_size;

        full = head == tail;
        putCounter ++;
        __android_log_print(ANDROID_LOG_ERROR,
                            "NATIVE_PACT",
                            "putting! %f, %llu, %llu\n", sample, timestamp,putCounter);
    }

    int get(TimestampSample* result) {
        std::lock_guard<std::mutex> lock(mutex);

        if(isEmpty())
            return -1;

        getCounter ++;
        /*
        __android_log_print(ANDROID_LOG_ERROR,
                            "NATIVE_PACT",
                            "GETTING! %f, %llu, %llu \n", buffer[tail], timestamp_buffer[tail], getCounter);
        */

        //Read data and advance the tail (we now have a free space)
        //auto val = buffer[tail];
        result->sample = buffer[tail];
        result->timestamp = timestamp_buffer[tail];

        full = false;
        tail = (tail + 1) % max_size;

        return 1;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex);
        head = tail;
        full = false;
    }

    bool isEmpty() {
        return (!full && (head == tail));
    }

    bool isFull() {
        return full;
    }

    size_t capacity() {
        return max_size;
    }

    size_t size() {
        size_t size = max_size;

        if (!full) {
            if (head >= tail)
                size = head - tail;
            else
                size = max_size + head - tail;
        }

        return size;
    }

private:
    std::mutex mutex;
    float* buffer;
    uint64_t* timestamp_buffer;
    size_t head = 0;
    size_t tail = 0;
    size_t max_size = 0;
    uint64_t getCounter = 0;
    uint64_t putCounter = 0;

    bool full = false;
};


#endif //SONICPACT_BUFFER_H
