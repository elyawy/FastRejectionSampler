
#include "FastRejectionSampler.h"
// #include "avl_array.h"
#include <iostream>

// #include <unordered_set>
// #include <cmath>
int main() {

    FastRejectionSampler fars({6.8, 0.03,20.1,0.5,1.7,4.2,10.3});

    auto levels = fars.getLevelsCDF();

    for (size_t i = 0; i < levels.size(); i++)
    {
        std::cout << i << " " <<  levels.at(i) << "\n";//<< "->" << fars.getLevelWeight(i-4) << "\n";
    }


    return 0;
}