#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "HandleBasedRejectionSampler.h"
#include <random>
#include <unordered_map>
#include <vector>
#include <cmath>

// Test fixture for HandleBasedRejectionSampler
class SamplerFixture {
protected:
    HandleBasedRejectionSampler sampler;
    std::mt19937 gen;
    
public:
    SamplerFixture() : sampler(0.1, 10.0), gen(42) {
        // Fixed seed for reproducibility
    }
};

TEST_CASE_FIXTURE(SamplerFixture, "Basic initialization and properties") {
    CHECK(sampler.size() == 0);
    CHECK(sampler.getTotalWeight() == 0.0);
    CHECK(sampler.checkValidity());
}

TEST_CASE_FIXTURE(SamplerFixture, "Adding weights") {
    // Add some weights
    auto h1 = sampler.addWeight(1.0, 100);
    auto h2 = sampler.addWeight(2.0, 200);
    auto h3 = sampler.addWeight(3.0, 300);
    
    CHECK(sampler.size() == 3);
    CHECK(sampler.getTotalWeight() == doctest::Approx(6.0));
    CHECK(sampler.getWeight(h1) == doctest::Approx(1.0));
    CHECK(sampler.getWeight(h2) == doctest::Approx(2.0));
    CHECK(sampler.getWeight(h3) == doctest::Approx(3.0));
    CHECK(sampler.getNumber(h1) == 100);
    CHECK(sampler.getNumber(h2) == 200);
    CHECK(sampler.getNumber(h3) == 300);
    CHECK(sampler.checkValidity());
}

TEST_CASE_FIXTURE(SamplerFixture, "Removing weights") {
    // Add some weights
    auto h1 = sampler.addWeight(1.0, 100);
    auto h2 = sampler.addWeight(2.0, 200);
    auto h3 = sampler.addWeight(3.0, 300);
    
    // Remove middle weight
    CHECK(sampler.removeWeight(h2));
    
    CHECK(sampler.size() == 2);
    CHECK(sampler.getTotalWeight() == doctest::Approx(4.0));
    CHECK(sampler.getWeight(h1) == doctest::Approx(1.0));
    CHECK(sampler.getWeight(h2) == doctest::Approx(-1.0)); // Not found
    CHECK(sampler.getWeight(h3) == doctest::Approx(3.0));
    CHECK(sampler.checkValidity());
    
    // Remove non-existent weight
    CHECK_FALSE(sampler.removeWeight(999));
    
    // Remove all remaining weights
    CHECK(sampler.removeWeight(h1));
    CHECK(sampler.removeWeight(h3));
    CHECK(sampler.size() == 0);
    CHECK(sampler.getTotalWeight() == doctest::Approx(0.0));
    CHECK(sampler.checkValidity());
}

TEST_CASE_FIXTURE(SamplerFixture, "Updating weights") {
    // Add some weights
    auto h1 = sampler.addWeight(1.0, 100);
    auto h2 = sampler.addWeight(2.0, 200);
    auto h3 = sampler.addWeight(4.0, 300);
    
    // Update weight within same level
    CHECK(sampler.updateWeight(h1, 1.5));
    
    // Update weight causing level change
    CHECK(sampler.updateWeight(h2, 5.0));
    
    CHECK(sampler.getWeight(h1) == doctest::Approx(1.5));
    CHECK(sampler.getWeight(h2) == doctest::Approx(5.0));
    CHECK(sampler.getTotalWeight() == doctest::Approx(10.5));
    CHECK(sampler.checkValidity());
    
    // Update non-existent weight
    CHECK_FALSE(sampler.updateWeight(999, 1.0));
    
    // Try to update weight outside bounds
    CHECK_FALSE(sampler.updateWeight(h3, 11.0)); // Above max
    CHECK_FALSE(sampler.updateWeight(h3, 0.05)); // Below min
}

TEST_CASE_FIXTURE(SamplerFixture, "Updating numbers") {
    // Add weights
    auto h1 = sampler.addWeight(1.0, 100);
    auto h2 = sampler.addWeight(2.0, 200);
    
    // Update numbers
    CHECK(sampler.updateNumber(h1, 500));
    CHECK(sampler.updateNumber(h2, 600));
    
    CHECK(sampler.getNumber(h1) == 500);
    CHECK(sampler.getNumber(h2) == 600);
    
    // Update non-existent handle
    CHECK_FALSE(sampler.updateNumber(999, 700));
}

TEST_CASE_FIXTURE(SamplerFixture, "Handle reuse") {
    // Add some weights
    auto h1 = sampler.addWeight(1.0);
    auto h2 = sampler.addWeight(2.0);
    
    // Remove a weight
    CHECK(sampler.removeWeight(h1));
    
    // Add a new weight - should reuse the handle
    auto h3 = sampler.addWeight(3.0);
    
    // The new handle should be the same as the removed one
    CHECK(h3 == h1);
    CHECK(sampler.getWeight(h3) == doctest::Approx(3.0));
    CHECK(sampler.checkValidity());
}

TEST_CASE_FIXTURE(SamplerFixture, "Basic sampling") {
    // Can't sample from empty sampler
    CHECK_THROWS_AS(sampler.sample(gen), std::runtime_error);
    
    // Add weights with equal values
    for (int i = 1; i <= 5; i++) {
        sampler.addWeight(1.0, i);
    }
    
    // Sample multiple times - with equal weights, we should see all numbers
    std::unordered_map<size_t, int> counts;
    const int NUM_SAMPLES = 1000;
    
    for (int i = 0; i < NUM_SAMPLES; i++) {
        size_t result = sampler.sampleNumber(gen);
        counts[result]++;
    }
    
    // With equal weights, each number should be sampled approximately 1/5 of the time
    for (int i = 1; i <= 5; i++) {
        CHECK(counts[i] > 0);
        double expected_ratio = 1.0 / 5.0;
        double actual_ratio = static_cast<double>(counts[i]) / NUM_SAMPLES;
        CHECK(actual_ratio == doctest::Approx(expected_ratio).epsilon(0.1));
    }
}

TEST_CASE_FIXTURE(SamplerFixture, "Weighted sampling") {
    // Add weights with different values
    sampler.addWeight(1.0, 1);  // 1/6 probability
    sampler.addWeight(2.0, 2);  // 2/6 probability
    sampler.addWeight(3.0, 3);  // 3/6 probability
    
    // Sample multiple times
    std::unordered_map<size_t, int> counts;
    const int NUM_SAMPLES = 5000;
    
    for (int i = 0; i < NUM_SAMPLES; i++) {
        size_t result = sampler.sampleNumber(gen);
        counts[result]++;
    }
    
    // Check proportions
    double total_weight = 6.0;
    for (int i = 1; i <= 3; i++) {
        double expected_ratio = static_cast<double>(i) / total_weight;
        double actual_ratio = static_cast<double>(counts[i]) / NUM_SAMPLES;
        CHECK(actual_ratio == doctest::Approx(expected_ratio).epsilon(0.05));
    }
}

TEST_CASE_FIXTURE(SamplerFixture, "Insertion between indices example") {
    // Add weights for numbers 1-10
    std::vector<double> weights = {5.0, 2.0, 8.0, 1.0, 3.0, 4.0, 6.0, 2.5, 7.0, 3.5};
    std::vector<HandleBasedRejectionSampler::WeightHandle> handles;
    
    for (size_t i = 0; i < weights.size(); i++) {
        handles.push_back(sampler.addWeight(weights[i], i + 1));
    }
    
    // Add two new weights for numbers 6 and 7
    auto h_new6 = sampler.addWeight(9.0, 6);
    auto h_new7 = sampler.addWeight(1.5, 7);
    
    // Shift old numbers 6-10 to become 8-12
    for (size_t i = 5; i < 10; i++) {
        sampler.updateNumber(handles[i], i + 3);  // Shift by +3
    }
    
    // Check that numbers were updated correctly
    CHECK(sampler.getNumber(handles[0]) == 1);
    CHECK(sampler.getNumber(handles[1]) == 2);
    CHECK(sampler.getNumber(handles[2]) == 3);
    CHECK(sampler.getNumber(handles[3]) == 4);
    CHECK(sampler.getNumber(handles[4]) == 5);
    CHECK(sampler.getNumber(h_new6) == 6);
    CHECK(sampler.getNumber(h_new7) == 7);
    CHECK(sampler.getNumber(handles[5]) == 8);  // Was 6
    CHECK(sampler.getNumber(handles[6]) == 9);  // Was 7
    CHECK(sampler.getNumber(handles[7]) == 10); // Was 8
    CHECK(sampler.getNumber(handles[8]) == 11); // Was 9
    CHECK(sampler.getNumber(handles[9]) == 12); // Was 10
    
    // Verify that weights are preserved
    CHECK(sampler.getWeight(handles[0]) == doctest::Approx(5.0));
    CHECK(sampler.getWeight(handles[5]) == doctest::Approx(4.0)); // Still has original weight
    CHECK(sampler.getWeight(h_new6) == doctest::Approx(9.0));
    
    // Verify total weight and validity
    CHECK(sampler.getTotalWeight() == doctest::Approx(52.5)); // Sum of all weights
    CHECK(sampler.checkValidity());
    
    // Simple sampling test
    std::unordered_map<size_t, int> counts;
    const int NUM_SAMPLES = 10000;
    
    for (int i = 0; i < NUM_SAMPLES; i++) {
        size_t result = sampler.sampleNumber(gen);
        counts[result]++;
    }
    
    // Numbers with higher weights should be sampled more frequently
    // Just check a few examples
    CHECK(counts[6] > counts[7]); // Weight 9.0 vs 1.5
    CHECK(counts[3] > counts[4]); // Weight 8.0 vs 1.0
}

TEST_CASE_FIXTURE(SamplerFixture, "Edge cases") {
    // Minimum and maximum weight handling
    auto h1 = sampler.addWeight(0.1); // Min weight
    auto h2 = sampler.addWeight(10.0); // Max weight
    
    CHECK(sampler.getWeight(h1) == doctest::Approx(0.1));
    CHECK(sampler.getWeight(h2) == doctest::Approx(10.0));
    
    // Sample distribution should favor higher weights
    std::unordered_map<HandleBasedRejectionSampler::WeightHandle, int> counts;
    const int NUM_SAMPLES = 1000;
    
    for (int i = 0; i < NUM_SAMPLES; i++) {
        auto handle = sampler.sample(gen);
        counts[handle]++;
    }
    
    // h2 (weight 10.0) should be sampled approximately 100x more than h1 (weight 0.1)
    double ratio = static_cast<double>(counts[h2]) / counts[h1];
    CHECK(ratio == doctest::Approx(100.0).epsilon(0.3));
}

TEST_CASE("Multiple level sampling") {
    // Create a sampler with a wider weight range
    HandleBasedRejectionSampler sampler(0.01, 100.0);
    std::mt19937 gen(42);
    
    // Add weights across multiple levels (powers of 2)
    auto h1 = sampler.addWeight(0.25, 1);
    auto h2 = sampler.addWeight(0.5, 2);
    auto h3 = sampler.addWeight(1.0, 3);
    auto h4 = sampler.addWeight(2.0, 4);
    auto h5 = sampler.addWeight(4.0, 5);
    auto h6 = sampler.addWeight(8.0, 6);
    auto h7 = sampler.addWeight(16.0, 7);
    auto h8 = sampler.addWeight(32.0, 8);
    
    CHECK(sampler.checkValidity());
    
    // Sample and count
    std::unordered_map<size_t, int> counts;
    const int NUM_SAMPLES = 10000;
    
    for (int i = 0; i < NUM_SAMPLES; i++) {
        size_t result = sampler.sampleNumber(gen);
        counts[result]++;
    }
    
    // Check that counts increase with weight
    for (size_t i = 1; i < 8; i++) {
        CHECK(counts[i] < counts[i+1]);
        
        // Ratio between adjacent weights should be approximately 2
        double expected_ratio = 2.0;
        double actual_ratio = static_cast<double>(counts[i+1]) / counts[i];
        CHECK(actual_ratio == doctest::Approx(expected_ratio).epsilon(0.3));
    }
}