#include <iostream>
#include <vector>
#include <stdexcept>
#include <limits>
#include <unordered_set>
#include <algorithm>
#include <string>
#include <cassert>
#include <cmath>
#include <sstream>

using namespace std;

//класс для удаления дубликатов
template<typename T>
class DuplicateRemover
{
public:
    static vector<T> removeDuplicates(const vector<T>& input)
    {
        if (input.empty())
        {
            return {};
        }

        validateInput(input);

        vector<T> result;
        unordered_set<T> seen;

        for (const auto& item : input) 
        {
            if (seen.find(item) == seen.end()) {
                seen.insert(item);
                result.push_back(item);
            }
        }

        return result;
    }

private:
    static void validateInput(const vector<T>& input)
    {
        if (input.size() > MAX_INPUT_SIZE)
        {
            throw invalid_argument("Input size exceeds maximum allowed limit");
        }
    }

    static constexpr size_t MAX_INPUT_SIZE = 1000000;
};

//unit-тесты
class DuplicateRemoverTests
{
public:
    static void runAllTests() 
    {
        testEmptyInput();
        testIntegerDuplicates();
        testStringDuplicates();
        testPreservesOrder();
        testAllDuplicates();
        testNoDuplicates();
        testWithNullptr();
        cout << "All tests passed! happy happy!!!!!!!!!" << endl;
    }

private:
    static void testEmptyInput()
    {
        vector<int> input = {};
        auto result = DuplicateRemover<int>::removeDuplicates(input);
        assert(result.empty());
        cout << "testEmptyInput: PASSED" << endl;
    }

    static void testIntegerDuplicates()
    {
        vector<int> input = { 1, 2, 2, 3, 4, 4, 4, 5, 1, 3 };
        auto result = DuplicateRemover<int>::removeDuplicates(input);
        vector<int> expected = { 1, 2, 3, 4, 5 };
        assert(result == expected);
        cout << "testIntegerDuplicates: PASSED" << endl;
    }

    static void testStringDuplicates() 
    {
        vector<string> input = { "apple", "banana", "apple", "cherry", "banana" };
        auto result = DuplicateRemover<string>::removeDuplicates(input);
        vector<string> expected = { "apple", "banana", "cherry" };
        assert(result == expected);
        cout << "testStringDuplicates: PASSED" << endl;
    }

    static void testPreservesOrder()
    {
        vector<int> input = { 5, 3, 1, 3, 2, 5, 1, 4 };
        auto result = DuplicateRemover<int>::removeDuplicates(input);
        vector<int> expected = { 5, 3, 1, 2, 4 };
        assert(result == expected);
        cout << "testPreservesOrder: PASSED" << endl;
    }

    static void testAllDuplicates() 
    {
        vector<int> input = { 7, 7, 7, 7, 7 };
        auto result = DuplicateRemover<int>::removeDuplicates(input);
        vector<int> expected = { 7 };
        assert(result == expected);
        cout << "testAllDuplicates: PASSED" << endl;
    }

    static void testNoDuplicates()
    {
        vector<int> input = { 1, 2, 3, 4, 5 };
        auto result = DuplicateRemover<int>::removeDuplicates(input);
        assert(result == input);
        cout << "testNoDuplicates: PASSED" << endl;
    }

    static void testWithNullptr()
    {
        int a = 1, b = 2, c = 3;
        vector<int*> input = { &a, &b, &a, &c, &b };
        auto result = DuplicateRemover<int*>::removeDuplicates(input);
        assert(result.size() == 3);
        cout << "testWithNullptr: PASSED" << endl;
    }
};

void demonstrateWithUserInput()
{
    cout << "работa DuplicateRemover" << endl;

    while (true)
    {
        cout << "\nВведите числа через пробел (или 'q' для выхода): ";
        string inputLine;
        getline(cin, inputLine);

        if (inputLine == "q" || inputLine == "exit")
        {
            break;
        }

        try 
        {
            vector<int> numbers;
            stringstream ss(inputLine);
            int number;

            while (ss >> number) 
            {
                numbers.push_back(number);
            }

            if (!numbers.empty())
            {
                auto result = DuplicateRemover<int>::removeDuplicates(numbers);

                cout << "Исходный массив: ";
                for (auto num : numbers) cout << num << " ";

                cout << "\nБез дубликатов: ";
                for (auto num : result) cout << num << " ";
                cout << endl;
            }

        }
        catch (const exception& e)
        {
            cout << "Ошибка: " << e.what() << endl;
        }
    }
}

void demonstrateExamples() 
{
    cout << "Примеры использования" << endl;

    //целые числа
    vector<int> numbers = { 1, 2, 2, 3, 4, 4, 5, 1 };
    auto result1 = DuplicateRemover<int>::removeDuplicates(numbers);
    cout << "Числа: ";
    for (auto num : result1) cout << num << " ";
    cout << endl;

    //строки
    vector<string> words = { "apple", "banana", "apple", "cherry" };
    auto result2 = DuplicateRemover<string>::removeDuplicates(words);
    cout << "Строки: ";
    for (const auto& word : result2) cout << word << " ";
    cout << endl;

    //дубликаты
    vector<int> largeInput;
    for (int i = 0; i < 10; ++i)
    {
        largeInput.push_back(i % 5);
    }
    auto result3 = DuplicateRemover<int>::removeDuplicates(largeInput);
    cout << "Большой массив: ";
    for (auto num : result3) cout << num << " ";
    cout << endl;
}

int main() 
{
    setlocale(LC_ALL, "RUS");
    try {
       
        cout << "Запуск unit-тестов" << endl;
        DuplicateRemoverTests::runAllTests();

        demonstrateExamples();

        demonstrateWithUserInput();

    }
    catch (const exception& e) 
    {
        cerr << "Критическая ошибка: " << e.what() << endl;
        return 1;
    }

    return 0;
}
