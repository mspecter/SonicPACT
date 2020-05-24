//
// Created by Michael Specter on 5/23/20.
//

#ifndef SONICPACT_BPSKSIGNALGENERATOR_H
#define SONICPACT_BPSKSIGNALGENERATOR_H

#include <vector>
#include <string>

#include <Constants.h>

std::vector<float> GenerateBPSKPreamble(float carrier_frequency);

#endif //SONICPACT_BPSKSIGNALGENERATOR_H
