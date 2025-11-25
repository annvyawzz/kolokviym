#ifndef FACTORIAL_H
#define FACTORIAL_H

#include <vector>
#include <stdexcept>
#include <limits>

class Factorial
{
public:
    static std::vector<unsigned long long> calculateFirstNFactorials(int n);
};

#endif // FACTORIAL_H
