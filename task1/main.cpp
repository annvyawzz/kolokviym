#include <iostream>
#include "Factorial.h"
#include <vector>
#include <stdexcept>
#include <limits>
using namespace std;

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
