//
// Created by Michael Specter on 5/10/20.
//

#ifndef SONICPACT_INCREMENTALLPF_H
#define SONICPACT_INCREMENTALLPF_H


class IncrementalLPF {

public:
    IncrementalLPF(float f_c, float f_s, int cycles_to_average){
        float samples_per_cycle = f_s / f_c;
        float samples_to_average = cycles_to_average * samples_per_cycle;
        float weight_of_last_sample = 0.5f;
        alpha = 1.0f - powf(weight_of_last_sample, 1.0f / samples_to_average);
    }

    float process(float sample){
        smoothed = alpha * sample + (1.0f - alpha) * smoothed;
        return smoothed;
    }

private:
    float smoothed = 0.0;
    float alpha;
};


#endif //SONICPACT_INCREMENTALLPF_H
