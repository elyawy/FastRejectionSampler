#include <vector>
#include <unordered_map>

#include <cmath>
#include <random>
#include <algorithm>

#include <iostream>

#include "avl_array.h"

class FastRejectionSampler
{
private:
    // using avl_tree = avl_array<double, int, std::uint8_t, 128, true>;

    std::vector<double> _weights;
    std::unordered_map<int, std::pair<double, std::vector<double>>> _levelToWeights;
    std::uniform_real_distribution<double> distribution;

    double _totalWeightsSum;
    std::vector<double> _levelsCDF;

public:
    FastRejectionSampler(const std::vector<double> &weights): _weights(weights) {

        auto it = std::minmax_element(std::begin(_weights), std::end(_weights)); // C++11
        int minWeightLevel = static_cast<int>(std::log2(*it.first)) + 1;
        int maxWeightLevel = static_cast<int>(std::log2(*it.second)) + 1;
        size_t numLevels = std::log2((*it.second) / (*it.first));

        std::cout << "num levels=" << numLevels << "\n";

        _levelToWeights.reserve(numLevels);
        _levelsCDF.resize(numLevels, 0.0);

        for (int i = minWeightLevel; i <= maxWeightLevel; ++i) {
            _levelToWeights[i] = std::make_pair<double, std::vector<double>>(0.0, {});
        }

    
        for(auto &weight: _weights) {
            _totalWeightsSum += weight;
            int level = static_cast<int>(std::log2(weight)) + 1;
            _levelToWeights.at(level).first += weight;
            _levelToWeights.at(level).second.push_back(weight);

            std::cout << level << " " << _levelToWeights.at(level).first << "\n";
        }

        double cumulativeSumOfWeights = 0.0;
        std::cout << cumulativeSumOfWeights << "\n";
        for (int level = minWeightLevel; level <= maxWeightLevel; ++level) {
             cumulativeSumOfWeights += _levelToWeights.at(level).first;
             std::cout << level - minWeightLevel << "->" << cumulativeSumOfWeights << "\n";

             _levelsCDF[level - minWeightLevel] = (cumulativeSumOfWeights);
        }
    }
    


    const std::vector<double> & getLevelsCDF() {
        return _levelsCDF;
    }


    double getLevelWeight(int level) {
        return _levelToWeights.at(level).first;
    }




    ~FastRejectionSampler(){};
};

