#include <gtest/gtest.h>
#include "Factorial.h"
#include <vector>
#include <stdexcept>

class FactorialTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// Тест 1: Отрицательное значение (специальная ситуация)
TEST_F(FactorialTest, NegativeInputThrowsException) {
    EXPECT_THROW({
        Factorial::calculateFirstNFactorials(-1);
    }, std::invalid_argument);
    
    EXPECT_THROW({
        Factorial::calculateFirstNFactorials(-5);
    }, std::invalid_argument);
}

// Тест 2: Нулевое количество факториалов (граничное условие)
TEST_F(FactorialTest, ZeroInputReturnsEmptyVector) {
    auto result = Factorial::calculateFirstNFactorials(0);
    EXPECT_TRUE(result.empty());
}

// Тест 3: Один факториал (граничное условие)
TEST_F(FactorialTest, OneFactorial) {
    auto result = Factorial::calculateFirstNFactorials(1);
    std::vector<unsigned long long> expected = {1}; // 0! = 1
    EXPECT_EQ(result, expected);
}

// Тест 4: Несколько факториалов (нормальный случай)
TEST_F(FactorialTest, MultipleFactorials) {
    auto result = Factorial::calculateFirstNFactorials(5);
    std::vector<unsigned long long> expected = {1, 1, 2, 6, 24}; // 0!, 1!, 2!, 3!, 4!
    EXPECT_EQ(result, expected);
}

// Тест 5: Проверка правильности вычислений
TEST_F(FactorialTest, FactorialCalculationsAreCorrect) {
    auto result = Factorial::calculateFirstNFactorials(6);
    EXPECT_EQ(result[0], 1);  // 0!
    EXPECT_EQ(result[1], 1);  // 1!
    EXPECT_EQ(result[2], 2);  // 2!
    EXPECT_EQ(result[3], 6);  // 3!
    EXPECT_EQ(result[4], 24); // 4!
    EXPECT_EQ(result[5], 120); // 5!
}

// Проверка на переполнение (исключительная ситуация)
TEST_F(FactorialTest, OverflowThrowsException) {
    // unsigned long long переполняется примерно на 21!
    EXPECT_THROW({
        Factorial::calculateFirstNFactorials(100); // Заведомо большое значение
    }, std::overflow_error);
}


// Проверка конкретных значений факториалов
TEST_F(FactorialTest, SpecificFactorialValues) {
    auto result = Factorial::calculateFirstNFactorials(10);
    
    // Проверяем известные значения факториалов
    EXPECT_EQ(result[0], 1);    // 0!
    EXPECT_EQ(result[1], 1);    // 1!
    EXPECT_EQ(result[2], 2);    // 2!
    EXPECT_EQ(result[3], 6);    // 3!
    EXPECT_EQ(result[4], 24);   // 4!
    EXPECT_EQ(result[5], 120);  // 5!
    EXPECT_EQ(result[6], 720);  // 6!
    EXPECT_EQ(result[7], 5040); // 7!
    EXPECT_EQ(result[8], 40320); // 8!
    EXPECT_EQ(result[9], 362880); // 9!
}

// Тест Проверка сообщений об ошибках
TEST_F(FactorialTest, ErrorMessages) {
    try {
        Factorial::calculateFirstNFactorials(-1);
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& e) {
        EXPECT_STREQ(e.what(), "Vhodnoye znacheniye dolzhno byt neotritsatelnym");
    }
    
    try {
        Factorial::calculateFirstNFactorials(100);
        FAIL() << "Expected std::overflow_error";
    } catch (const std::overflow_error& e) {
        EXPECT_STREQ(e.what(), "perepolneniyе");
    }
}

// Параметризованный тест для различных входных значений
class FactorialParamTest : public ::testing::TestWithParam<std::tuple<int, std::vector<unsigned long long>>> {
};

TEST_P(FactorialParamTest, WithVariousInputs) {
    auto [input, expected] = GetParam();
    auto result = Factorial::calculateFirstNFactorials(input);
    EXPECT_EQ(result, expected);
}

INSTANTIATE_TEST_SUITE_P(
    FactorialValues,
    FactorialParamTest,
    ::testing::Values(
        std::make_tuple(0, std::vector<unsigned long long>{}),
        std::make_tuple(1, std::vector<unsigned long long>{1}),
        std::make_tuple(2, std::vector<unsigned long long>{1, 1}),
        std::make_tuple(3, std::vector<unsigned long long>{1, 1, 2}),
        std::make_tuple(4, std::vector<unsigned long long>{1, 1, 2, 6})
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
