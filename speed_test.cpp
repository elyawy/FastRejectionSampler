#include <iostream>
#include <random>
#include <chrono>

#include "FastRejectionSampler.h"

int main() {

    auto gen = std::mt19937_64(42);
    auto dist = std::uniform_real_distribution(0.001, 10.0);

    size_t numberOfWeights = 1000;
    size_t numberOfSamples = 10000000;

    auto indexSampler = std::uniform_int_distribution<int>(0, numberOfWeights - 1);
    std::vector<double> weights;

    for (size_t i = 0; i < numberOfWeights; i++) {
        double weight = dist(gen);
        weights.push_back(weight);
    }
    
    FastRejectionSampler fars(weights, 0.001, 10.0);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (size_t i = 0; i < numberOfSamples; i++)
    {
        fars.sample(gen);
        fars.updateWeight(42, 5.0);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;



    // begin = std::chrono::steady_clock::now();
    // for (size_t i = 0; i < numberOfSamples; i++)
    // {
    //     weights[42] = 0.5;
    //     auto dicreteDist = std::discrete_distribution<int>(weights.begin(), weights.end());
    //     dicreteDist(gen);
    // }
    // end = std::chrono::steady_clock::now();
    // std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;


    return 0;
}