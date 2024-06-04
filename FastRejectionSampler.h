#include <vector>
#include <unordered_map>

#include <cmath>
#include <random>
#include <algorithm>

#include <iostream>
class FastRejectionSampler
{
private:

    std::vector<double> _weights;
    double _minWeight;
    double _maxWeight;
    std::uniform_real_distribution<double> _biasedCoin;
    std::vector<std::pair<double, std::vector<size_t>>> _levelToWeights;
    std::unordered_map<size_t, size_t> _weightIndexToBin;

    double _totalWeightsSum;
    int _minWeightLevel;
    int _maxWeightLevel;

    std::vector<double> _levelsWeights;

public:
    FastRejectionSampler(const std::vector<double> &weights, double minWeight, double maxWeight): 
        _weights(weights), _minWeight(minWeight), _maxWeight(maxWeight) , _biasedCoin(0.0,1.0) {

        _minWeightLevel = static_cast<int>(std::log2(minWeight));
        if (_minWeightLevel >= 0) _minWeightLevel += 1;
        _maxWeightLevel = static_cast<int>(std::log2(maxWeight)) + 1;
        size_t numLevels = _maxWeightLevel - _minWeightLevel + 1;
        std::cout << "_minWeightLevel=" << _minWeightLevel << "\n"; 

        std::cout << "numLevels=" << numLevels << "\n"; 

        _levelToWeights.resize(numLevels);
        _levelsWeights.resize(numLevels, 0.0);

        for (int i = 0; i < numLevels; ++i) {
            _levelToWeights[i] = std::make_pair<double, std::vector<size_t>>(0, {});
            std::cout << _levelToWeights.size() << "\n";
        }


        for(size_t i=0; i < _weights.size(); ++i) {
            _totalWeightsSum += _weights[i];
            int level = static_cast<int>(std::log2(_weights[i]));
            if (level >= 0) level += 1;
            std::cout << "_weights[i]=" << _weights[i] <<"\n";

            std::cout << "index=" << i << " is in level=" <<level<<"\n";
            level -= _minWeightLevel;
            std::cout << "index=" << i << " is in level=" <<level<<"\n";
            std::cout << "_levelToWeights.size=" << _levelToWeights.size() <<"\n";

            _levelToWeights.at(level).first += _weights[i];
            size_t innerIndex = _levelToWeights.at(level).second.size();
            _levelToWeights.at(level).second.push_back(i);
            _weightIndexToBin[i] = innerIndex;

            _levelsWeights[level] = _levelToWeights.at(level).first;
        }

    }
    
    template <typename Generator>
    size_t sample(Generator&& gen) {
        double levelSampler = std::uniform_real_distribution<double>(0.0, _totalWeightsSum)(gen);
        int selectedLevel = 0;

        double cumulativeWeight = 0.0;

        for (int i = 0; i < _levelsWeights.size(); i++) {
            cumulativeWeight += _levelsWeights[i];
            // std::cout << "level=" << i << " _levelsWeights=" << _levelsWeights[i] << "\n";  
            selectedLevel = i;
            if (levelSampler < cumulativeWeight) break;
        }

        int correctedLevel =  selectedLevel + _minWeightLevel;
        double levelConversion = 1.0 / std::pow(2, correctedLevel);
        auto binsInSelectedLevel = _levelToWeights.at(correctedLevel).second;

        auto binSampler = std::uniform_int_distribution<int>(0, binsInSelectedLevel.size() - 1);

        // rejection sampling:
        while (true) {
            int selectedBin = binSampler(gen);
            size_t selectedIndex = binsInSelectedLevel[selectedBin];
            double rejection = _weights[selectedIndex] * levelConversion;
            double coinToss = _biasedCoin(gen);
            if (coinToss < rejection) return selectedIndex;
        }
    
        // return UINT64_MAX;
    }


    void updateWeight(int weightIndex, double newWeight) {
        double oldWeight = _weights[weightIndex];
        int oldLevel = static_cast<int>(std::log2(_weights[weightIndex]));
        size_t oldBinIndex = _weightIndexToBin.at(weightIndex);
        if (oldLevel >= 0) oldLevel += 1;
        int oldLevelIndex = oldLevel - _minWeightLevel;

        int newLevel = static_cast<int>(std::log2(newWeight));
        if (newLevel >= 0) newLevel += 1;
        int newLevelIndex = newLevel - _minWeightLevel;

        _totalWeightsSum -= oldWeight;
        _totalWeightsSum += newWeight;

        if (oldLevel == newLevel) {
            _levelToWeights.at(newLevel).first -= oldWeight;
            _levelToWeights.at(newLevel).first += newWeight;
            _levelsWeights[newLevel] = _levelToWeights.at(newLevel).first;

            _weights[weightIndex] = newWeight;
            return;
        }

        // remove weight from old level
        _levelToWeights.at(oldLevel).first -= oldWeight;
        _levelsWeights[oldLevel] = _levelToWeights.at(oldLevel).first;
        size_t numberOfBins = _levelToWeights.at(oldLevel).second.size();
        if (numberOfBins > 1) {
            size_t weightIndexOfLastBin = _levelToWeights.at(oldLevel).second[numberOfBins - 1];
            _levelToWeights.at(oldLevel).second[oldBinIndex] = weightIndexOfLastBin;
            _weightIndexToBin[weightIndexOfLastBin] = oldBinIndex;
        }
        _levelToWeights.at(oldLevel).second.pop_back();

        // add to new level
        _levelToWeights.at(newLevel).first += newWeight;
        _weightIndexToBin[weightIndex] = _levelToWeights.at(newLevel).second.size();
        _levelToWeights.at(newLevel).second.push_back(weightIndex);
        _levelsWeights[newLevel] = _levelToWeights.at(newLevel).first;

        // update weight in the original weights vector
        _weights[weightIndex] = newWeight;

    }

    const std::vector<double> & getLevelsWeights() {
        return _levelsWeights;
    }


    double getLevelWeight(int level) {
        return std::get<0>(_levelToWeights.at(level));
    }

    int getLevelBin(int level) {
        return level + _minWeightLevel;
    }




    ~FastRejectionSampler(){};
};

