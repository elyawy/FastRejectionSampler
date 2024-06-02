#include <vector>
#include <unordered_map>

#include <cmath>
#include <random>
#include <algorithm>

#include <iostream>

// #include "avl_array.h"

class FastRejectionSampler
{
private:
    // using avl_tree = avl_array<double, int, std::uint8_t, 128, true>;

    std::vector<double> _weights;
    std::unordered_map<int, std::pair<double, std::vector<double>>> _levelToWeights;
    std::uniform_real_distribution<double> distribution;

    double _totalWeightsSum;
    int _minWeightLevel;
    int _maxWeightLevel;

    std::vector<double> _levelsCDF;

public:
    FastRejectionSampler(const std::vector<double> &weights): _weights(weights) {

        auto it = std::minmax_element(std::begin(_weights), std::end(_weights)); // C++11
        _minWeightLevel = static_cast<int>(std::log2(*it.first)) + 1;
        _maxWeightLevel = static_cast<int>(std::log2(*it.second)) + 1;
        size_t numLevels = _maxWeightLevel - _minWeightLevel + 1;

        std::cout << "min level=" << _minWeightLevel << "\n";
        std::cout << "max level=" << _maxWeightLevel << "\n";
        std::cout << "num levels=" << numLevels << "\n";

        _levelToWeights.reserve(numLevels);
        _levelsCDF.resize(numLevels, 0.0);

        for (int i = _minWeightLevel; i <= _maxWeightLevel; ++i) {
            _levelToWeights[i] = std::make_pair<double, std::vector<double>>(0.0, {});
        }

    
        for(auto &weight: _weights) {
            _totalWeightsSum += weight;
            int level = static_cast<int>(std::log2(weight)) + 1;
            _levelToWeights.at(level).first += weight;
            _levelToWeights.at(level).second.push_back(weight);
        }

        double cumulativeSumOfWeights = 0.0;
        for (int level = _minWeightLevel; level <= _maxWeightLevel; ++level) {
            if (_levelToWeights.at(level).first == 0.0) continue;
            cumulativeSumOfWeights += _levelToWeights.at(level).first;
            _levelsCDF[level - _minWeightLevel] = (cumulativeSumOfWeights);
        }
        // std::sort(_levelsCDF.begin(), _levelsCDF.end());
    }
    


    const std::vector<double> & getLevelsCDF() {
        return _levelsCDF;
    }


    double getLevelWeight(int level) {
        return _levelToWeights.at(level).first;
    }

    int getLevelBin(int level) {
        return level + _minWeightLevel;
    }




    ~FastRejectionSampler(){};
};

