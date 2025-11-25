#include <gtest/gtest.h>
#include "task.h"
#include <vector>
#include <string>

class LinkedListTest : public ::testing::Test {
protected:
    void SetUp() override {   
    }

    void TearDown() override {   
    }
};

// Тест пустого списка
TEST_F(LinkedListTest, EmptyList) {
    LinkedList<int> list;
    list.reverseRecursive();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);
    EXPECT_TRUE(list.toVector().empty());
}

// Тест одного элемента
TEST_F(LinkedListTest, SingleElement) {
    LinkedList<int> list;
    list.pushBack(42);

    list.reverseRecursive();

    auto result = list.toVector();
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 42);
}

// Тест нескольких элементов
TEST_F(LinkedListTest, MultipleElements) {
    LinkedList<int> list;
    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);
    list.pushBack(4);

    list.reverseRecursive();

    auto result = list.toVector();
    std::vector<int> expected = {4, 3, 2, 1};
    EXPECT_EQ(result, expected);
}

// Тест сохранения данных при развороте
TEST_F(LinkedListTest, ReversePreservesData) {
    LinkedList<std::string> list;
    list.pushBack("hello");
    list.pushBack("world");
    list.pushBack("test");

    list.reverseRecursive();

    auto result = list.toVector();
    std::vector<std::string> expected = {"test", "world", "hello"};
    EXPECT_EQ(result, expected);
}

// Тест большого списка
TEST_F(LinkedListTest, LargeList) {
    LinkedList<int> list;
    const int SIZE = 100;

    for (int i = 0; i < SIZE; ++i) {
        list.pushBack(i);
    }

    EXPECT_EQ(list.size(), SIZE);

    list.reverseRecursive();

    auto result = list.toVector();
    EXPECT_EQ(result.size(), SIZE);

    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(result[i], SIZE - 1 - i);
    }
}

// Тест очистки
TEST_F(LinkedListTest, Clear) {
    LinkedList<int> list;
    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    EXPECT_EQ(list.size(), 3);
    list.clear();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);

    list.reverseRecursive();
    EXPECT_TRUE(list.empty());
}

// Тест рекурсивного обхода
TEST_F(LinkedListTest, Traverse) {
    LinkedList<int> list;
    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    std::vector<int> visited;
    list.traverseRecursive([&visited](const int& data) {
        visited.push_back(data);
    });

    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(visited, expected);
}

// Тест добавления в начало
TEST_F(LinkedListTest, PushFront) {
    LinkedList<int> list;
    list.pushFront(3);
    list.pushFront(2);
    list.pushFront(1);

    auto result = list.toVector();
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

// Тест перемещения
TEST_F(LinkedListTest, MoveOperations) {
    LinkedList<int> list1;
    list1.pushBack(1);
    list1.pushBack(2);
    list1.pushBack(3);

    // Тест перемещающего конструктора
    LinkedList<int> list2 = std::move(list1);
    EXPECT_EQ(list2.toVector(), std::vector<int>({1, 2, 3}));
    EXPECT_TRUE(list1.empty());

    // Тест перемещающего присваивания
    LinkedList<int> list3;
    list3 = std::move(list2);
    EXPECT_EQ(list3.toVector(), std::vector<int>({1, 2, 3}));
    EXPECT_TRUE(list2.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
