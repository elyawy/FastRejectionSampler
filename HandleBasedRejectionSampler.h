#include <vector>
#include <unordered_map>
#include <queue>
#include <limits>
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>

class HandleBasedRejectionSampler
{
public:
    // Define handle type for weight references
    using WeightHandle = size_t;
    
private:
    // Structure to store information about each weight
    struct WeightEntry {
        double value;          // The actual weight value
        int levelIndex;        // Index of the level this weight belongs to
        size_t binPosition;    // Position within its level's bin
    };

    double _minWeight;         // Minimum allowed weight
    double _maxWeight;         // Maximum allowed weight
    double _totalWeightsSum;   // Sum of all weights
    
    int _minWeightLevel;       // Minimum level index
    int _maxWeightLevel;       // Maximum level index
    
    // Random distribution for rejection sampling
    std::uniform_real_distribution<double> _biasedCoin;
    
    // Next available handle
    WeightHandle _nextHandle;
    
    // Queue of reusable handles (from deleted weights)
    std::queue<WeightHandle> _reusableHandles;
    
    // Map from handle to weight entry
    std::unordered_map<WeightHandle, WeightEntry> _weightEntries;
    
    // Map from level to the handles in that level
    std::vector<std::vector<WeightHandle>> _levelToHandles;
    
    // Sum of weights for each level
    std::vector<double> _levelsWeights;
    
    // Optional: Map from handle to the value/number it represents
    std::unordered_map<WeightHandle, size_t> _handleToNumber;

public:
    HandleBasedRejectionSampler(double minWeight, double maxWeight)
        : _minWeight(minWeight), _maxWeight(maxWeight), _totalWeightsSum(0.0), 
          _biasedCoin(0.0, 1.0), _nextHandle(0) {
        
        // Calculate level ranges based on min and max weights
        _minWeightLevel = static_cast<int>(std::log2(minWeight));
        if (_minWeightLevel >= 0) _minWeightLevel += 1;
        _maxWeightLevel = static_cast<int>(std::log2(maxWeight)) + 1;
        
        // Initialize level structures
        size_t numLevels = _maxWeightLevel - _minWeightLevel + 1;
        _levelToHandles.resize(numLevels);
        _levelsWeights.resize(numLevels, 0.0);
    }
    
    /**
     * Adds a new weight to the sampler.
     * 
     * @param weight The weight value to add
     * @param numberToRepresent Optional value this weight represents
     * @return Handle that can be used to reference this weight
     */
    WeightHandle addWeight(double weight, size_t numberToRepresent = 0) {
        // Check if weight is within bounds
        if (weight < _minWeight || weight > _maxWeight) {
            std::cout << "Weight out of bounds: " << weight << std::endl;
            abort();
        }
        
        // Get a handle (reuse deleted ones if available)
        WeightHandle handle;
        if (!_reusableHandles.empty()) {
            handle = _reusableHandles.front();
            _reusableHandles.pop();
        } else {
            handle = _nextHandle++;
        }
        
        // Calculate level for this weight
        int level = static_cast<int>(std::log2(weight));
        if (level >= 0) level += 1;
        int levelIndex = level - _minWeightLevel;
        
        // Create weight entry
        WeightEntry entry;
        entry.value = weight;
        entry.levelIndex = levelIndex;
        entry.binPosition = _levelToHandles[levelIndex].size();
        
        // Store the entry and update mappings
        _weightEntries[handle] = entry;
        _levelToHandles[levelIndex].push_back(handle);
        
        // Update total weight sums
        _totalWeightsSum += weight;
        _levelsWeights[levelIndex] += weight;
        
        // Store what number this handle represents (if provided)
        if (numberToRepresent > 0) {
            _handleToNumber[handle] = numberToRepresent;
        }
        
        return handle;
    }
    
    /**
     * Removes a weight from the sampler.
     * 
     * @param handle The handle of the weight to remove
     * @return true if weight was found and removed, false otherwise
     */
    bool removeWeight(WeightHandle handle) {
        // Check if handle exists
        auto entryIt = _weightEntries.find(handle);
        if (entryIt == _weightEntries.end()) {
            return false;  // Handle not found
        }
        
        // Get the entry information
        WeightEntry& entry = entryIt->second;
        double weight = entry.value;
        int levelIndex = entry.levelIndex;
        size_t binPosition = entry.binPosition;
        
        // Update total weights
        _totalWeightsSum -= weight;
        _levelsWeights[levelIndex] -= weight;
        
        // Remove from level's vector (swap with last element and pop)
        auto& levelHandles = _levelToHandles[levelIndex];
        if (binPosition < levelHandles.size() - 1) {
            // Move the last handle to this position
            WeightHandle lastHandle = levelHandles.back();
            levelHandles[binPosition] = lastHandle;
            
            // Update bin position for the moved handle
            _weightEntries[lastHandle].binPosition = binPosition;
        }
        levelHandles.pop_back();
        
        // Remove the entry from maps
        _weightEntries.erase(handle);
        _handleToNumber.erase(handle);
        
        // Make handle available for reuse
        _reusableHandles.push(handle);
        
        return true;
    }
    
    /**
     * Updates the weight for an existing handle.
     * 
     * @param handle The handle of the weight to update
     * @param newWeight The new weight value
     * @return true if weight was updated successfully, false otherwise
     */
    bool updateWeight(WeightHandle handle, double newWeight) {
        // Check if weight is within bounds
        if (newWeight < _minWeight || newWeight > _maxWeight) {
            std::cout << "New weight out of bounds: " << newWeight << std::endl;
            return false;
        }
        
        // Check if handle exists
        auto entryIt = _weightEntries.find(handle);
        if (entryIt == _weightEntries.end()) {
            return false;  // Handle not found
        }
        
        // Get the entry information
        WeightEntry& entry = entryIt->second;
        double oldWeight = entry.value;
        int oldLevelIndex = entry.levelIndex;
        size_t oldBinPosition = entry.binPosition;
        
        // Calculate new level
        int newLevel = static_cast<int>(std::log2(newWeight));
        if (newLevel >= 0) newLevel += 1;
        int newLevelIndex = newLevel - _minWeightLevel;
        
        // Update total weights
        _totalWeightsSum -= oldWeight;
        _totalWeightsSum += newWeight;
        
        // If level hasn't changed, simple update
        if (oldLevelIndex == newLevelIndex) {
            _levelsWeights[newLevelIndex] -= oldWeight;
            _levelsWeights[newLevelIndex] += newWeight;
            entry.value = newWeight;
            return true;
        }
        
        // Remove from old level
        _levelsWeights[oldLevelIndex] -= oldWeight;
        auto& oldLevelHandles = _levelToHandles[oldLevelIndex];
        if (oldBinPosition < oldLevelHandles.size() - 1) {
            // Move the last handle to this position
            WeightHandle lastHandle = oldLevelHandles.back();
            oldLevelHandles[oldBinPosition] = lastHandle;
            
            // Update bin position for the moved handle
            _weightEntries[lastHandle].binPosition = oldBinPosition;
        }
        oldLevelHandles.pop_back();
        
        // Add to new level
        _levelsWeights[newLevelIndex] += newWeight;
        entry.levelIndex = newLevelIndex;
        entry.binPosition = _levelToHandles[newLevelIndex].size();
        _levelToHandles[newLevelIndex].push_back(handle);
        
        // Update entry value
        entry.value = newWeight;
        
        return true;
    }
    
    /**
     * Samples a weight handle according to the weight distribution.
     * 
     * @param gen Random number generator
     * @return Handle of the selected weight
     */
    template <typename Generator>
    WeightHandle sample(Generator&& gen) {
        if (_totalWeightsSum <= 0 || _weightEntries.empty()) {
            throw std::runtime_error("Cannot sample from empty distribution");
        }
        
        // Select level proportionally to total weight
        double levelSampler = std::uniform_real_distribution<double>(0.0, _totalWeightsSum)(gen);
        int selectedLevelIndex = 0;
        double cumulativeWeight = 0.0;
        
        for (size_t i = 0; i < _levelsWeights.size(); i++) {
            cumulativeWeight += _levelsWeights[i];
            selectedLevelIndex = i;
            if (levelSampler < cumulativeWeight) break;
        }
        
        // Calculate the level conversion factor for rejection sampling
        int correctedLevel = selectedLevelIndex + _minWeightLevel;
        double levelConversion = 1.0 / std::pow(2, correctedLevel);
        
        auto& handlesInLevel = _levelToHandles[selectedLevelIndex];
        if (handlesInLevel.empty()) {
            // This shouldn't happen if level weights are maintained correctly
            throw std::runtime_error("Selected level has no weights");
        }
        
        // Perform rejection sampling within the selected level
        auto binSampler = std::uniform_int_distribution<size_t>(
            0, handlesInLevel.size() - 1);
            
        while (true) {
            size_t selectedBin = binSampler(gen);
            WeightHandle selectedHandle = handlesInLevel[selectedBin];
            double weight = _weightEntries[selectedHandle].value;
            
            // Rejection test
            double rejection = weight * levelConversion;
            if (_biasedCoin(gen) < rejection) {
                return selectedHandle;
            }
        }
    }
    
    /**
     * Samples a number according to the weight distribution.
     * This is a convenience method that returns the number associated with
     * the sampled handle.
     * 
     * @param gen Random number generator
     * @return The number represented by the sampled handle
     */
    template <typename Generator>
    size_t sampleNumber(Generator&& gen) {
        WeightHandle handle = sample(gen);
        auto it = _handleToNumber.find(handle);
        if (it != _handleToNumber.end()) {
            return it->second;
        }
        // If no number mapping exists, return the handle itself
        return handle;
    }
    
    /**
     * Updates the number/value that a handle represents.
     * 
     * @param handle The handle to update
     * @param newNumber The new number this handle should represent
     * @return true if successful, false if handle not found
     */
    bool updateNumber(WeightHandle handle, size_t newNumber) {
        if (_weightEntries.find(handle) == _weightEntries.end()) {
            return false;
        }
        _handleToNumber[handle] = newNumber;
        return true;
    }
    
    /**
     * Gets the current weight for a handle.
     * 
     * @param handle The handle to query
     * @return The weight, or -1 if handle not found
     */
    double getWeight(WeightHandle handle) const {
        auto it = _weightEntries.find(handle);
        if (it != _weightEntries.end()) {
            return it->second.value;
        }
        return -1.0;
    }
    
    /**
     * Gets the number represented by a handle.
     * 
     * @param handle The handle to query
     * @return The number, or 0 if no mapping exists
     */
    size_t getNumber(WeightHandle handle) const {
        auto it = _handleToNumber.find(handle);
        if (it != _handleToNumber.end()) {
            return it->second;
        }
        return 0;
    }
    
    /**
     * Gets the total sum of all weights.
     * 
     * @return The sum of weights
     */
    double getTotalWeight() const {
        return _totalWeightsSum;
    }
    
    /**
     * Gets the number of weights in the sampler.
     * 
     * @return Number of weights
     */
    size_t size() const {
        return _weightEntries.size();
    }
    
    /**
     * Checks if the internal data structures are valid and consistent.
     * 
     * @return true if valid, false otherwise
     */
    bool checkValidity() const {
        double epsilon = 1e-10;
        
        // Check total weight consistency
        double calculatedTotal = 0.0;
        for (const auto& [handle, entry] : _weightEntries) {
            calculatedTotal += entry.value;
        }
        if (std::abs(calculatedTotal - _totalWeightsSum) > epsilon) {
            std::cout << "Total weight mismatch: " << calculatedTotal 
                     << " vs " << _totalWeightsSum << std::endl;
            return false;
        }
        
        // Check level weights consistency
        double levelWeightsSum = 0.0;
        for (const auto& levelWeight : _levelsWeights) {
            levelWeightsSum += levelWeight;
        }
        if (std::abs(levelWeightsSum - _totalWeightsSum) > epsilon) {
            std::cout << "Level weights sum mismatch: " << levelWeightsSum 
                     << " vs " << _totalWeightsSum << std::endl;
            return false;
        }
        
        // Check level-to-handles consistency
        for (size_t levelIdx = 0; levelIdx < _levelToHandles.size(); ++levelIdx) {
            double levelSum = 0.0;
            for (WeightHandle handle : _levelToHandles[levelIdx]) {
                auto it = _weightEntries.find(handle);
                if (it == _weightEntries.end()) {
                    std::cout << "Handle " << handle << " in level " << levelIdx 
                             << " doesn't exist in entries" << std::endl;
                    return false;
                }
                
                const WeightEntry& entry = it->second;
                if (entry.levelIndex != levelIdx) {
                    std::cout << "Handle " << handle << " has level index " 
                             << entry.levelIndex << " but is in level " 
                             << levelIdx << std::endl;
                    return false;
                }
                
                levelSum += entry.value;
            }
            
            if (std::abs(levelSum - _levelsWeights[levelIdx]) > epsilon) {
                std::cout << "Level " << levelIdx << " weight sum mismatch: " 
                         << levelSum << " vs " << _levelsWeights[levelIdx] << std::endl;
                return false;
            }
        }
        
        return true;
    }
};