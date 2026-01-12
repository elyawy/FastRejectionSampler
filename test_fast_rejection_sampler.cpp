#include <iostream>
#include <vector>
#include <map>
#include <random>
#include <cmath>
#include <iomanip>

// Define MDOUBLE if not already defined
#ifndef MDOUBLE
#define MDOUBLE double
#endif

// Include the modified FastRejectionSampler
#include "FastRejectionSampler.h"

/**
 * Chi-squared test for goodness of fit
 * Returns the chi-squared statistic
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
 * Test 1: Uniform weights
 */
void testUniformWeights() {
    std::cout << "=== Test 1: Uniform Weights ===" << std::endl;
    
    std::vector<double> weights = {1.0, 1.0, 1.0, 1.0, 1.0};
    double minWeight = 0.5;
    double maxWeight = 2.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(42);
    
    std::map<size_t, size_t> counts;
    const size_t numSamples = 100000;
    
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    std::cout << "Expected: Each index should be sampled ~" << (numSamples / 5) << " times" << std::endl;
    std::cout << "Observed frequencies:" << std::endl;
    for (size_t i = 0; i < weights.size(); ++i) {
        double observed = static_cast<double>(counts[i]) / numSamples;
        double expected = 0.2;
        std::cout << "  Index " << i << ": " << counts[i] 
                  << " (" << std::fixed << std::setprecision(4) << observed << ")"
                  << " [expected: " << expected << "]";
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "Chi-squared statistic: " << chiSq 
              << " (df=4, critical value at α=0.05 is ~9.49)" << std::endl;
    std::cout << std::endl;
}

/**
 * Test 2: Non-uniform weights
 */
void testNonUniformWeights() {
    std::cout << "=== Test 2: Non-uniform Weights ===" << std::endl;
    
    std::vector<double> weights = {0.5, 1.0, 2.0, 4.0, 8.0};  // Powers of 2
    double minWeight = 0.25;
    double maxWeight = 16.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(123);
    
    std::map<size_t, size_t> counts;
    const size_t numSamples = 100000;
    
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    double totalWeight = 0.0;
    for (const auto& w : weights) totalWeight += w;
    
    std::cout << "Weights: ";
    for (size_t i = 0; i < weights.size(); ++i) {
        std::cout << weights[i];
        if (i < weights.size() - 1) std::cout << ", ";
    }
    std::cout << " (total: " << totalWeight << ")" << std::endl;
    
    std::cout << "Expected vs Observed frequencies:" << std::endl;
    for (size_t i = 0; i < weights.size(); ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << i << ": weight=" << std::setw(4) << weights[i]
                  << ", expected=" << std::fixed << std::setprecision(4) << expected
                  << ", observed=" << observed;
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "Chi-squared statistic: " << chiSq 
              << " (df=4, critical value at α=0.05 is ~9.49)" << std::endl;
    std::cout << std::endl;
}

/**
 * Test 3: Skewed distribution
 */
void testSkewedDistribution() {
    std::cout << "=== Test 3: Heavily Skewed Distribution ===" << std::endl;
    
    std::vector<double> weights = {0.01, 0.02, 0.07, 0.9, 10.0};
    double minWeight = 0.005;
    double maxWeight = 20.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(456);
    
    std::map<size_t, size_t> counts;
    const size_t numSamples = 100000;
    
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    double totalWeight = 0.0;
    for (const auto& w : weights) totalWeight += w;
    
    std::cout << "Expected vs Observed frequencies:" << std::endl;
    for (size_t i = 0; i < weights.size(); ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << i << ": weight=" << std::setw(6) << weights[i]
                  << ", expected=" << std::fixed << std::setprecision(4) << expected
                  << ", observed=" << observed;
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "Chi-squared statistic: " << chiSq 
              << " (df=4, critical value at α=0.05 is ~9.49)" << std::endl;
    std::cout << std::endl;
}

/**
 * Test 4: Dynamic weight updates
 */
void testDynamicWeightUpdates() {
    std::cout << "=== Test 4: Dynamic Weight Updates ===" << std::endl;
    
    std::vector<double> weights = {1.0, 1.0, 1.0, 1.0};
    double minWeight = 0.5;
    double maxWeight = 10.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(789);
    
    const size_t numSamples = 50000;
    
    // Phase 1: Initial uniform sampling
    std::cout << "Phase 1: Initial uniform weights [1.0, 1.0, 1.0, 1.0]" << std::endl;
    std::map<size_t, size_t> counts1;
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts1[sample]++;
    }
    
    for (size_t i = 0; i < weights.size(); ++i) {
        double observed = static_cast<double>(counts1[i]) / numSamples;
        std::cout << "  Index " << i << ": " << std::fixed << std::setprecision(4) << observed;
        bool match = std::abs(observed - 0.25) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    // Phase 2: Update weights to heavily favor index 2
    std::cout << "\nPhase 2: After updating weight[2] to 8.0" << std::endl;
    sampler.updateWeight(2, 8.0);
    weights[2] = 8.0;
    
    std::map<size_t, size_t> counts2;
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts2[sample]++;
    }
    
    double totalWeight = 0.0;
    for (const auto& w : weights) totalWeight += w;
    
    for (size_t i = 0; i < weights.size(); ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts2[i]) / numSamples;
        std::cout << "  Index " << i << ": expected=" << expected
                  << ", observed=" << observed;
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    // Verify internal state
    std::cout << "\nInternal consistency check: ";
    bool valid = sampler.checkValidity();
    std::cout << (valid ? "PASS ✓" : "FAIL ✗") << std::endl;
    std::cout << std::endl;
}

/**
 * Test 5: Edge cases with weights near boundaries
 */
void testEdgeCaseWeights() {
    std::cout << "=== Test 5: Edge Case Weights ===" << std::endl;
    
    // Test with weights at various powers of 2
    std::vector<double> weights = {0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0};
    double minWeight = 0.0625;
    double maxWeight = 16.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(101112);
    
    std::map<size_t, size_t> counts;
    const size_t numSamples = 100000;
    
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    double totalWeight = 0.0;
    for (const auto& w : weights) totalWeight += w;
    
    std::cout << "Testing powers of 2 (good for log2/ilogb):" << std::endl;
    for (size_t i = 0; i < weights.size(); ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << i << ": weight=" << std::setw(5) << weights[i]
                  << ", expected=" << std::fixed << std::setprecision(4) << expected
                  << ", observed=" << observed;
        bool match = std::abs(observed - expected) < 0.01;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "Chi-squared statistic: " << chiSq 
              << " (df=6, critical value at α=0.05 is ~12.59)" << std::endl;
    std::cout << std::endl;
}

/**
 * Test 6: Large scale test
 */
void testLargeScale() {
    std::cout << "=== Test 6: Large Scale (Many Weights) ===" << std::endl;
    
    const size_t numWeights = 100;
    std::vector<double> weights;
    std::mt19937_64 weightGen(999);
    std::uniform_real_distribution<double> weightDist(0.1, 5.0);
    
    for (size_t i = 0; i < numWeights; ++i) {
        weights.push_back(weightDist(weightGen));
    }
    
    double minWeight = 0.05;
    double maxWeight = 10.0;
    
    FastRejectionSampler sampler(weights, minWeight, maxWeight);
    std::mt19937_64 rng(131415);
    
    std::map<size_t, size_t> counts;
    const size_t numSamples = 1000000;
    
    for (size_t i = 0; i < numSamples; ++i) {
        size_t sample = sampler.sample(rng);
        counts[sample]++;
    }
    
    double totalWeight = 0.0;
    for (const auto& w : weights) totalWeight += w;
    
    // Check first 10 and last 10 weights
    std::cout << "Sample of results (first 5 and last 5 indices):" << std::endl;
    for (size_t i = 0; i < 5; ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << std::setw(3) << i 
                  << ": expected=" << std::fixed << std::setprecision(5) << expected
                  << ", observed=" << observed;
        bool match = std::abs(observed - expected) < 0.001;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    for (size_t i = numWeights - 5; i < numWeights; ++i) {
        double expected = weights[i] / totalWeight;
        double observed = static_cast<double>(counts[i]) / numSamples;
        std::cout << "  Index " << std::setw(3) << i 
                  << ": expected=" << std::fixed << std::setprecision(5) << expected
                  << ", observed=" << observed;
        bool match = std::abs(observed - expected) < 0.001;
        std::cout << (match ? " ✓" : " ✗") << std::endl;
    }
    
    double chiSq = chiSquaredTest(counts, weights, numSamples);
    std::cout << "Chi-squared statistic: " << chiSq 
              << " (df=99, critical value at α=0.05 is ~123.23)" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "FastRejectionSampler Statistical Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    testUniformWeights();
    testNonUniformWeights();
    testSkewedDistribution();
    testDynamicWeightUpdates();
    testEdgeCaseWeights();
    testLargeScale();
    
    std::cout << "========================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}