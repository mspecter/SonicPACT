//
// Created by Michael Specter on 6/15/20.
//

#ifndef SONICPACT_RANDOMNOISE_H
#define SONICPACT_RANDOMNOISE_H

#include <vector>
#include <string>
#include <iostream>
#include <functional> //for std::hash
#include <random>

#include <Constants.h>


std::vector<float> GenerateRandom(std::string name);

#endif //SONICPACT_RANDOMNOISE_H
