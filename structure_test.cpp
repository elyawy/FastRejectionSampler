#include <iostream>
#include <iomanip>
#include <random>

#include "FastRejectionSampler.h"

int main() {

    auto gen = std::mt19937_64(56);
    auto dist = std::uniform_real_distribution(0.001, 10.0);

    size_t numberOfWeights = 1000;
    size_t numberOfUpdates = 100000;

    auto indexSampler = std::uniform_int_distribution<int>(0, numberOfWeights - 1);
    std::vector<double> weights;

    for (size_t i = 0; i < numberOfWeights; i++) {
        double weight = dist(gen);
        weights.push_back(weight);
    }
    
    FastRejectionSampler fars(weights, 0.001, 10.0);


    for (size_t i = 0; i < numberOfUpdates; i++) {
        size_t sample = fars.sample(gen);
        if (sample < 0) {
            std::cout << "INVALID INDEX=" << sample << "\n";
        }
        double newWeight = dist(gen);
        int indexToUpdate = indexSampler(gen);
        fars.updateWeight(indexToUpdate, newWeight);
        if (!fars.checkValidity()){
            std::cout << "update number=" << i << "\n";
            std::cout << "FAILED CHECK!\n";
        }
        
    }

    return 0;
}