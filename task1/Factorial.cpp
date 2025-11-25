#include "Factorial.h"

std::vector<unsigned long long> Factorial::calculateFirstNFactorials(int n)
{
    if (n < 0)
    {
        throw std::invalid_argument("Vhodnoye znacheniye dolzhno byt neotritsatelnym"); 
    }

    if (n == 0)
    {
        return {};
    }

    std::vector<unsigned long long> result;
    result.reserve(n);

    unsigned long long factorial = 1;
    result.push_back(factorial); // 0! = 1

    for (int i = 1; i < n; ++i)
    {
        if (factorial > std::numeric_limits<unsigned long long>::max() / i)
        {
            throw std::overflow_error("perepolneniyе"); 
        }
        factorial *= i;
        result.push_back(factorial);
    }

    return result;
}
