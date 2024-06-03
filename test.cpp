

#include "FastRejectionSampler.h"
// #include "avl_array.h"
#include <iostream>
#include <random>
#include <map>

#include <iomanip>
// #include <unordered_set>
// #include <cmath>
int main() {

    FastRejectionSampler fars({6.8, 0.03,20.1,0.5,1.7,4.2,10.3});

    auto gen = std::mt19937_64(0);


    size_t numSamples = 10000;
    std::map<int, int> hist;
    for (int n = 0; n != numSamples; ++n)
        ++hist[fars.sample(gen)];
 
    for (auto const& [x, y] : hist)
            std::cout  << x << " -> "  <<  y   << '\n';
    
    std::cout << "\n";
    fars.updateWeight(1, 15.0);

    hist.clear();

    for (int n = 0; n != numSamples; ++n)
        ++hist[fars.sample(gen)];
 
    for (auto const& [x, y] : hist)
            std::cout  << x << " -> "  <<  y   << '\n';

    std::cout << "\n";
    fars.updateWeight(5, 42.0);

    hist.clear();

    for (int n = 0; n != numSamples; ++n)
        ++hist[fars.sample(gen)];
 
    for (auto const& [x, y] : hist)
            std::cout  << x << " -> "  <<  y   << '\n';




    return 0;
}