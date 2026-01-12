#include <iostream>
#include <vector>
#include <map>
#include <random>
#include <cmath>
#include <iomanip>
#include <algorithm>

#ifndef MDOUBLE
#define MDOUBLE double
#endif

#include "FastRejectionSampler.h"

/**
 * Chi-squared test for goodness of fit
 */
double chiSquaredTest(const std::map<size_t, size_t>& observed, 
                      const std::vector<double>& weights,
                      size_t totalSamples) {
    double totalWeight = 0.0;
    for (const auto& w : weights) {
        totalWeight += w;
    }
    
    double chiSquared = 0.0;
    for (size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] == 0.0) continue;
        
        double expected = (weights[i] / totalWeight) * totalSamples;
        double obs = observed.count(i) ? observed.at(i) : 0.0;
        double diff = obs - expected;
        chiSquared += (diff * diff) / expected;
    }
    
    return chiSquared;
}

/**
 * Test 1: Sequential updates - update each weight once
 */
void testSequentialUpdates() {
    std::cout << "=== Test 1: Sequential Weight Updates ===" << std::endl;
    
    const size_t numWeights = 20;
    std::vector<double> weights(numWeights, 1.0);  // Start uniform
    double minWeight = 0.5;
    double maxWeight = 10.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> weightDist(0.5, 5.0);
    
    std::cout << "Initial state: all weights = 1.0" << std::endl;
    
    // Update each weight to a random value
    for (size_t i = 0; i < numWeights; ++i) {
        double newWeight = weightDist(rng);
        sampler.updateWeight(i, newWeight);
        weights[i] = newWeight;
    }
    
    std::cout << "After updating all " << numWeights << " weights to random values" << std::endl;
    
    // Verify internal consistency
    if (!sampler.checkValidity()) {
        std::cout << "ERROR: Internal consistency check failed!" << std::endl;
        return;
    }
    std::cout << "Internal consistency: PASS ✓" << std::endl;
    
    // Sample and check distribution
    std::map<size_t, size_t> counts;
    const size_t numSamples = 200000;
    
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    double totalWeight = sampler.getSumOfWeights();
    
    std::cout << "Sampling results (first 5 and last 5):" << std::endl;
    for (size_t i = 0; i < 5; ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << std::setw(2) << i 
                  << ": expected=" << std::fixed << std::setprecision(4) << expected
                  << ", observed=" << observed;
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    for (size_t i = numWeights - 5; i < numWeights; ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << std::setw(2) << i 
                  << ": expected=" << std::fixed << std::setprecision(4) << expected
                  << ", observed=" << observed;
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "Chi-squared: " << chiSq 
              << " (df=" << (numWeights-1) << ", critical ~" 
              << (numWeights-1 + 2*std::sqrt(2*(numWeights-1))) << ")" << std::endl;
    
    bool pass = chiSq < (numWeights-1 + 2*std::sqrt(2*(numWeights-1)));
    std::cout << "Result: " << (pass ? "PASS ✓" : "FAIL ✗") << std::endl;
    std::cout << std::endl;
}

/**
 * Test 2: Repeated random updates - many updates to random indices
 */
void testRepeatedRandomUpdates() {
    std::cout << "=== Test 2: Repeated Random Updates ===" << std::endl;
    
    const size_t numWeights = 20;
    std::vector<double> weights(numWeights);
    std::mt19937_64 setupRng(123);
    std::uniform_real_distribution<double> initialDist(0.5, 5.0);
    
    // Initialize with random weights
    for (size_t i = 0; i < numWeights; ++i) {
        weights[i] = initialDist(setupRng);
    }
    
    double minWeight = 0.1;
    double maxWeight = 10.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(456);
    std::uniform_int_distribution<size_t> indexDist(0, numWeights - 1);
    std::uniform_real_distribution<double> weightDist(0.5, 8.0);
    
    const size_t numUpdates = 100;
    std::cout << "Performing " << numUpdates << " random weight updates..." << std::endl;
    
    // Perform many random updates
    for (size_t update = 0; update < numUpdates; ++update) {
        size_t idx = indexDist(rng);
        double newWeight = weightDist(rng);
        sampler.updateWeight(idx, newWeight);
        weights[idx] = newWeight;
        
        // Check validity every 10 updates
        if ((update + 1) % 10 == 0) {
            if (!sampler.checkValidity()) {
                std::cout << "ERROR: Consistency check failed after update " << (update + 1) << std::endl;
                return;
            }
        }
    }
    
    std::cout << "All updates completed successfully" << std::endl;
    std::cout << "Final consistency check: ";
    if (!sampler.checkValidity()) {
        std::cout << "FAIL ✗" << std::endl;
        return;
    }
    std::cout << "PASS ✓" << std::endl;
    
    // Sample and check distribution
    std::map<size_t, size_t> counts;
    const size_t numSamples = 200000;
    
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    double totalWeight = sampler.getSumOfWeights();
    
    // Find worst errors
    std::vector<std::pair<double, size_t>> errors;
    for (size_t i = 0; i < weights.size(); ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        double error = std::abs(observed - expected);
        errors.push_back({error, i});
    }
    std::sort(errors.rbegin(), errors.rend());
    
    std::cout << "\nTop 5 sampling errors:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), errors.size()); ++i) {
        size_t idx = errors[i].second;
        double expected = weights[idx] / totalWeight;
        double observed = static_cast<double>(counts[idx]) / numSamples;
        
        std::cout << "  Index " << std::setw(2) << idx 
                  << ": weight=" << std::fixed << std::setprecision(3) << weights[idx]
                  << ", exp=" << std::setprecision(5) << expected
                  << ", obs=" << observed
                  << ", err=" << std::setprecision(6) << (observed - expected);
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "\nChi-squared: " << chiSq 
              << " (df=" << (numWeights-1) << ", critical ~" 
              << (numWeights-1 + 2*std::sqrt(2*(numWeights-1))) << ")" << std::endl;
    
    bool pass = chiSq < (numWeights-1 + 2*std::sqrt(2*(numWeights-1)));
    std::cout << "Result: " << (pass ? "PASS ✓" : "FAIL ✗") << std::endl;
    std::cout << std::endl;
}

/**
 * Test 3: Level transitions - update weights to force movement between levels
 */
void testLevelTransitions() {
    std::cout << "=== Test 3: Level Transitions ===" << std::endl;
    
    const size_t numWeights = 10;
    std::vector<double> weights(numWeights, 1.0);  // All start at level 1 (2^0 < 1.0 < 2^1)
    double minWeight = 0.25;
    double maxWeight = 16.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(789);
    
    std::cout << "Starting with all weights = 1.0" << std::endl;
    
    // Force each weight through different levels
    std::vector<double> targetWeights = {0.3, 0.6, 1.2, 2.4, 4.8, 0.5, 1.5, 3.0, 6.0, 12.0};
    
    std::cout << "Updating weights to force level transitions:" << std::endl;
    for (size_t i = 0; i < numWeights; ++i) {
        int oldLevel = std::ilogb(weights[i]) + 1;
        int newLevel = std::ilogb(targetWeights[i]) + 1;
        
        sampler.updateWeight(i, targetWeights[i]);
        weights[i] = targetWeights[i];
        
        std::cout << "  Weight[" << i << "]: " << std::fixed << std::setprecision(1)
                  << "1.0 (level " << oldLevel << ") → " 
                  << targetWeights[i] << " (level " << newLevel << ")" << std::endl;
    }
    
    // Verify consistency
    std::cout << "\nInternal consistency: ";
    if (!sampler.checkValidity()) {
        std::cout << "FAIL ✗" << std::endl;
        return;
    }
    std::cout << "PASS ✓" << std::endl;
    
    // Sample and check distribution
    std::map<size_t, size_t> counts;
    const size_t numSamples = 200000;
    
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    double totalWeight = sampler.getSumOfWeights();
    
    std::cout << "\nSampling results:" << std::endl;
    for (size_t i = 0; i < numWeights; ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        int level = std::ilogb(weights[i]) + 1;
        
        std::cout << "  Index " << i 
                  << " (weight=" << std::fixed << std::setprecision(1) << weights[i]
                  << ", level=" << level << "):" << std::setprecision(5)
                  << " exp=" << expected << ", obs=" << observed;
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "\nChi-squared: " << chiSq 
              << " (df=" << (numWeights-1) << ", critical ~" 
              << (numWeights-1 + 2*std::sqrt(2*(numWeights-1))) << ")" << std::endl;
    
    bool pass = chiSq < (numWeights-1 + 2*std::sqrt(2*(numWeights-1)));
    std::cout << "Result: " << (pass ? "PASS ✓" : "FAIL ✗") << std::endl;
    std::cout << std::endl;
}

/**
 * Test 4: Stress test - many weights, many updates
 */
void testStressTest() {
    std::cout << "=== Test 4: Stress Test (50 weights, 500 updates) ===" << std::endl;
    
    const size_t numWeights = 50;
    std::vector<double> weights(numWeights);
    std::mt19937_64 setupRng(999);
    std::uniform_real_distribution<double> initialDist(0.5, 5.0);
    
    for (size_t i = 0; i < numWeights; ++i) {
        weights[i] = initialDist(setupRng);
    }
    
    double minWeight = 0.1;
    double maxWeight = 10.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(111);
    std::uniform_int_distribution<size_t> indexDist(0, numWeights - 1);
    std::uniform_real_distribution<double> weightDist(0.2, 8.0);
    
    const size_t numUpdates = 500;
    std::cout << "Performing " << numUpdates << " random updates on " 
              << numWeights << " weights..." << std::endl;
    
    for (size_t update = 0; update < numUpdates; ++update) {
        size_t idx = indexDist(rng);
        double newWeight = weightDist(rng);
        sampler.updateWeight(idx, newWeight);
        weights[idx] = newWeight;
        
        if ((update + 1) % 100 == 0) {
            if (!sampler.checkValidity()) {
                std::cout << "ERROR: Consistency failed at update " << (update + 1) << std::endl;
                return;
            }
            std::cout << "  After " << (update + 1) << " updates: consistency OK" << std::endl;
        }
    }
    
    std::cout << "All updates completed" << std::endl;
    std::cout << "Final consistency check: ";
    if (!sampler.checkValidity()) {
        std::cout << "FAIL ✗" << std::endl;
        return;
    }
    std::cout << "PASS ✓" << std::endl;
    
    // Sample
    std::map<size_t, size_t> counts;
    const size_t numSamples = 500000;
    
    std::cout << "Sampling " << numSamples << " times..." << std::endl;
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    // Check a sample of results
    double totalWeight = sampler.getSumOfWeights();
    
    std::cout << "\nSample of results (indices 0-4 and 45-49):" << std::endl;
    for (size_t i = 0; i < 5; ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << std::setw(2) << i 
                  << ": exp=" << std::fixed << std::setprecision(5) << expected
                  << ", obs=" << observed;
        bool match = std::abs(observed - expected) < 0.005;  // Tighter tolerance
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    for (size_t i = numWeights - 5; i < numWeights; ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << std::setw(2) << i 
                  << ": exp=" << std::fixed << std::setprecision(5) << expected
                  << ", obs=" << observed;
        bool match = std::abs(observed - expected) < 0.005;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "\nChi-squared: " << chiSq 
              << " (df=" << (numWeights-1) << ", critical ~" 
              << (numWeights-1 + 2*std::sqrt(2*(numWeights-1))) << ")" << std::endl;
    
    bool pass = chiSq < (numWeights-1 + 2*std::sqrt(2*(numWeights-1)));
    std::cout << "Result: " << (pass ? "PASS ✓" : "FAIL ✗") << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "FastRejectionSampler Update Stress Tests" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;
    
    testSequentialUpdates();
    testRepeatedRandomUpdates();
    testLevelTransitions();
    testStressTest();
    
    std::cout << "============================================" << std::endl;
    std::cout << "All update tests completed!" << std::endl;
    std::cout << "============================================" << std::endl;
    
    return 0;
}
