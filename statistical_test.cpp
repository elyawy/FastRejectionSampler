

#include "FastRejectionSampler.h"
// #include "avl_array.h"
#include <iostream>
#include <random>
#include <map>

#include <iomanip>
// #include <unordered_set>
// #include <cmath>
int main() {

    auto gen = std::mt19937_64(34);
    auto dist = std::uniform_real_distribution(0.001, 50.0);

    size_t numberOfWeights = 10;
    size_t numberOfSamples = 10000;

    auto indexSampler = std::uniform_int_distribution<int>(0, numberOfWeights - 1);
    std::vector<double> weights;

    for (size_t i = 0; i < numberOfWeights; i++) {
        double weight = dist(gen);
        weights.push_back(weight);
    }
    
    FastRejectionSampler fars(weights, 0.001, 50.0);
    auto dicreteDist = std::discrete_distribution<int>(weights.begin(), weights.end());
    

    const int nstars = 1000;   // maximum number of stars to distribute
    int std_sampler[10]={};
    int rejection_sampler[10]={};

    for (int i=0; i<numberOfSamples; ++i) {
        int number = dicreteDist(gen);
        ++std_sampler[number];
        number = fars.sample(gen);
        ++rejection_sampler[number];
    }

    std::cout << "absolute difference between std and rejection sampler:" << std::endl;
    for (int i=0; i<10; ++i) {
        int abs_difference = abs(nstars*(std_sampler[i] - rejection_sampler[i]))/numberOfSamples;
        std::cout << i << ": " << std::string(abs_difference,'*')  << std::endl;

        // std::cout << i << ": " << std::string(std_sampler[i]*nstars/numberOfSamples,'*') << "std" << std::endl;
        // std::cout << i << ": " << std::string(rejection_sampler[i]*nstars/numberOfSamples,'*') << "rejection" << std::endl;
    }



    return 0;
}