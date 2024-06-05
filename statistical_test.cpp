

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
    auto dist = std::uniform_real_distribution(0.001, 10.0);

    size_t numberOfWeights = 10;
    size_t numberOfSamples = 1000000;

    auto indexSampler = std::uniform_int_distribution<int>(0, numberOfWeights - 1);
    std::vector<double> weights;

    for (size_t i = 0; i < numberOfWeights; i++) {
        double weight = dist(gen);
        weights.push_back(weight);
        if (i == numberOfWeights-1) {
            std::cout << weight << "\n";
            break;
        }
        std::cout << weight << " ";
    }
    
    FastRejectionSampler fars(weights, 0.001, 50.0);
    std::cout << "sum of weights=" << fars.getSumOfWeights() << "\n";
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
        double std_freq = std_sampler[i]/ (1.0 * numberOfSamples);
        double rejection_freq = rejection_sampler[i]/(1.0 * numberOfSamples);

        std::cout <<  std::fixed << i << ": " << std_freq << " ~ " << rejection_freq << std::endl;

        std_sampler[i] = 0;
        rejection_sampler[i] = 0;
    }

    weights[0] = 0.3;
    fars.updateWeight(0, 0.3);
    std::cout << "sum of weights=" << fars.getSumOfWeights() << "\n";
    dicreteDist = std::discrete_distribution<int>(weights.begin(), weights.end());


    for (int i=0; i<numberOfSamples; ++i) {
        int number = dicreteDist(gen);
        ++std_sampler[number];
        number = fars.sample(gen);
        ++rejection_sampler[number];
    }


    std::cout << "absolute difference between std and rejection sampler after update:" << std::endl;
    for (int i=0; i<10; ++i) {
        double std_freq = std_sampler[i]/ (1.0 * numberOfSamples);
        double rejection_freq = rejection_sampler[i]/(1.0 * numberOfSamples);

        std::cout << std::fixed << i << ": " << std_freq << " ~ " << rejection_freq << std::endl;

        std_sampler[i] = 0;
        rejection_sampler[i] = 0;
    }

    return 0;
}