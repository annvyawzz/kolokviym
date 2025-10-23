#include <iostream>
#include <vector>
#include <stdexcept>
#include <limits>
using namespace std;

class Factorial
{
public:
    static vector<unsigned long long> calculateFirstNFactorials(int n)
    {
        if (n < 0)
        {
            throw invalid_argument("Vhodnoye znacheniye dolzhno byt neotritsatelnym"); 
        }

        if (n == 0)
        {
            return {};
        }

        vector<unsigned long long> result;
        result.reserve(n);

        unsigned long long factorial = 1;
        result.push_back(factorial); // 0! = 1

        for (int i = 1; i < n; ++i)
        {

            if (factorial > numeric_limits<unsigned long long>::max() / i)
            {
                throw overflow_error("perepolneniyе"); 
            }
            factorial *= i;
            result.push_back(factorial);
        }

        return result;
    }
};

int main()
{
    try {
        int n;
        cout << "Vvedite kolichestvo factorialov n: ";
        cin >> n;

        if (cin.fail()) 
        {
            cout << "Oshibka: nevemny vvod!" << endl;
            return 1;
        }

        auto result = Factorial::calculateFirstNFactorials(n);

        cout << "Pervye " << n << " factorialov: ";
        for (size_t i = 0; i < result.size(); ++i)
        {
            cout << i << "! = " << result[i];
            if (i < result.size() - 1)
            {
                cout << ", ";
            }
        }
        cout << endl;

    }
    catch (const exception& e)
    {
        cout << "Oshibka: " << e.what() << endl;
    }

    return 0;
}
