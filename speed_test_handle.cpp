#include <iostream>
#include <random>
#include <chrono>
#include <vector>

#include "HandleBasedRejectionSampler.h"

int main() {
    // Initialize random number generator
    auto gen = std::mt19937_64(42);
    auto dist = std::uniform_real_distribution<double>(0.001, 10.0);

    // Test parameters
    size_t numberOfWeights = 1000;
    size_t numberOfSamples = 10000000;
    
    // Create the HandleBasedRejectionSampler
    HandleBasedRejectionSampler sampler(0.001, 10.0);
    
    // Create weights and add them to the sampler
    std::vector<HandleBasedRejectionSampler::WeightHandle> handles;
    handles.reserve(numberOfWeights);
    
    for (size_t i = 0; i < numberOfWeights; i++) {
        double weight = dist(gen);
        // Store the handle for later use in updates
        handles.push_back(sampler.addWeight(weight, i));
    }
    
    // Verify the sampler is in a consistent state
    if (!sampler.checkValidity()) {
        std::cerr << "Sampler failed validity check after initialization!" << std::endl;
        return 1;
    }
    
    // Perform sampling and weight updates, measuring time
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < numberOfSamples; i++) {
        // Sample a handle
        auto handle = sampler.sample(gen);
        
        // Update weight for handle at index 42 (similar to the original test)
        sampler.updateWeight(handle, 5.0);
    }
    
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    
    // Output timing results
    std::cout << "Time difference = " << 
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() 
        << "[ms]" << std::endl;
    
    // Verify the sampler is still in a consistent state after all operations
    if (!sampler.checkValidity()) {
        std::cerr << "Sampler failed validity check after test!" << std::endl;
        return 1;
    }
    
    return 0;
}