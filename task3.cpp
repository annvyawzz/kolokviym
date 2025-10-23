#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>
#include <functional>
#include <cassert>
#include <string>

using namespace std;

//класс связного списка
template<typename T>
class ListNode 
{
public:
    T data;
    unique_ptr<ListNode<T>> next;

    ListNode(const T& value) : data(value), next(nullptr) {}

    ListNode(const ListNode&) = delete;
    ListNode& operator=(const ListNode&) = delete;

    ListNode(ListNode&&) = default;
    ListNode& operator=(ListNode&&) = default;
};

template<typename T>
class LinkedList 
{
private:
    unique_ptr<ListNode<T>> head;
    size_t size_;

public:
    LinkedList() : head(nullptr), size_(0) {}

    ~LinkedList() 
    {
        clear(); 
    }

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    LinkedList(LinkedList&&) = default;
    LinkedList& operator=(LinkedList&&) = default;

    // в начало
    void pushFront(const T& value)
    {
        auto newNode = make_unique<ListNode<T>>(value);
        newNode->next = move(head);
        head = move(newNode);
        size_++;
    }

    //в конец
    void pushBack(const T& value)
    {
        if (!head) {
            pushFront(value);
            return;
        }

        ListNode<T>* current = head.get();
        while (current->next)
        {
            current = current->next.get();
        }
        current->next = make_unique<ListNode<T>>(value);
        size_++;
    }

    //использование рекурсии
    void reverseRecursive() 
    {
        if (!head || !head->next) 
        {
            return;
        }

        try {
            head = reverseRecursiveImpl(move(head));
        }
        catch (const exception& e)
        {
            throw runtime_error("fail: " + string(e.what()));
        }
    }

    size_t size() const { return size_; }

    bool empty() const { return size_ == 0; }

    void clear()
    {
       
        while (head) 
        {
            head = move(head->next);
        }
        size_ = 0;
    }

    vector<T> toVector() const
    {
        vector<T> result;
        ListNode<T>* current = head.get();
        while (current) {
            result.push_back(current->data);
            current = current->next.get();
        }
        return result;
    }

    void print(const string& name = "List") const
    {
        cout << name << ": ";
        ListNode<T>* current = head.get();
        while (current) 
        {
            cout << current->data;
            if (current->next) {
                cout << " -> ";
            }
            current = current->next.get();
        }
        cout << " -> NULL" << endl;
    }

    void traverseRecursive(function<void(const T&)> func) const 
    {
        traverseRecursiveImpl(head.get(), func);
    }

private:
    
    unique_ptr<ListNode<T>> reverseRecursiveImpl(unique_ptr<ListNode<T>> node) 
    {
        if (!node) 
        {
            return nullptr;
        }

        if (!node->next)
        {
            return node;
        }

        unique_ptr<ListNode<T>> newHead = reverseRecursiveImpl(move(node->next));

        ListNode<T>* tail = newHead.get();
        while (tail->next) {
            tail = tail->next.get();
        }
        tail->next = move(node);

        return newHead;
    }

    void traverseRecursiveImpl(ListNode<T>* node, function<void(const T&)> func) const
    {
        if (!node) return;
        func(node->data);
        traverseRecursiveImpl(node->next.get(), func);
    }
};

//иnit-тесты
class LinkedListTests 
{
public:
    static void runAllTests() 
    {
        testEmptyList();
        testSingleElement();
        testMultipleElements();
        testReversePreservesData();
        testLargeList();
        testClear();
        testTraverse();
        cout << "Все тесты пройдены успешно!" << endl;
    }

private:
    static void testEmptyList()
    {
        LinkedList<int> list;
        list.reverseRecursive();
        assert(list.empty());
        assert(list.size() == 0);
        assert(list.toVector().empty());
        cout << "testEmptyList: PASSED" << endl;
    }

    static void testSingleElement() 
    {
        LinkedList<int> list;
        list.pushBack(42);

        list.reverseRecursive();

        auto result = list.toVector();
        assert(result.size() == 1);
        assert(result[0] == 42);
        cout << "testSingleElement: PASSED" << endl;
    }

    static void testMultipleElements()
    {
        LinkedList<int> list;
        list.pushBack(1);
        list.pushBack(2);
        list.pushBack(3);
        list.pushBack(4);

        list.reverseRecursive();

        auto result = list.toVector();
        vector<int> expected = { 4, 3, 2, 1 };
        assert(result == expected);
        cout << "testMultipleElements: PASSED" << endl;
    }

    static void testReversePreservesData()
    {
        LinkedList<string> list;
        list.pushBack("hello");
        list.pushBack("world");
        list.pushBack("test");

        list.reverseRecursive();

        auto result = list.toVector();
        vector<string> expected = { "test", "world", "hello" };
        assert(result == expected);
        cout << "testReversePreservesData: PASSED" << endl;
    }

    static void testLargeList() 
    {
        LinkedList<int> list;
        const int SIZE = 100;

        for (int i = 0; i < SIZE; ++i) 
        {
            list.pushBack(i);
        }

        assert(list.size() == SIZE);

        list.reverseRecursive();

        auto result = list.toVector();
        assert(result.size() == SIZE);

        for (int i = 0; i < SIZE; ++i) {
            assert(result[i] == SIZE - 1 - i);
        }
        cout << "testLargeList: PASSED" << endl;
    }

    static void testClear()
    {
        LinkedList<int> list;
        list.pushBack(1);
        list.pushBack(2);
        list.pushBack(3);

        assert(list.size() == 3);
        list.clear();
        assert(list.empty());
        assert(list.size() == 0);

        list.reverseRecursive();
        assert(list.empty());
        cout << "testClear: PASSED" << endl;
    }

    static void testTraverse()
    {
        LinkedList<int> list;
        list.pushBack(1);
        list.pushBack(2);
        list.pushBack(3);

        vector<int> visited;
        list.traverseRecursive([&visited](const int& data) 
            {
            visited.push_back(data);
            });

        vector<int> expected = { 1, 2, 3 };
        assert(visited == expected);
        cout << "testTraverse: PASSED" << endl;
    }
};
void demonstrateExamples() 
{
    cout << "Демонстрация работы" << endl;

    //разворот числового списка
    {
        LinkedList<int> numbers;
        numbers.pushBack(1);
        numbers.pushBack(2);
        numbers.pushBack(3);
        numbers.pushBack(4);
        numbers.pushBack(5);

        cout << "Числа:" << endl;
        numbers.print("До разворота");
        numbers.reverseRecursive();
        numbers.print("После разворота");
    }

    //разворот строкового списка
    {
        LinkedList<string> words;
        words.pushBack("Hello");
        words.pushBack("World");
        words.pushBack("!");

        cout << "Строки:" << endl;
        words.print("До разворота");
        words.reverseRecursive();
        words.print("После разворота");
    }

    //рекурсивный обход
    {
        LinkedList<int> list;
        for (int i = 1; i <= 5; ++i) 
        {
            list.pushBack(i * 10);
        }

        cout << "рекурсивный обход:" << endl;
        list.print("Список");
        cout << "Элементы: ";
        list.traverseRecursive([](const int& data)
            {
            cout << data << " ";
            });
        cout << endl;
    }
}

void interactiveDemo() {
    cout << "Интерактивная демонстрация" << endl;
    cout << "Вводите числа по одному (или 'q' для завершения):" << endl;

    LinkedList<int> list;
    string input;

    while (true) 
    {
        cout << "Введите число: ";
        cin >> input;

        if (input == "q" || input == "exit")
        {
            break;
        }

        try {
            int number = stoi(input);
            list.pushBack(number);
            list.print("Текущий список");
        }
        catch (const exception& e)
        {
            cout << "Ошибка ввода! Попробуйте снова." << endl;
            cin.clear();
        }
    }

    if (!list.empty())
    {
        cout << "Разворачиваем список" << endl;
        list.reverseRecursive();
        list.print("Развернутый список");

        cout << "Элементы в обратном порядке: ";
        list.traverseRecursive([](const int& data) 
            {
            cout << data << " ";
            });
        cout << endl;
    }
}

int main()
{
    try {
        cout << "Запуск unit-тестов" << endl;
        LinkedListTests::runAllTests();

        demonstrateExamples();
        interactiveDemo();

        cout << "Программа завершена успешно!" << endl;

    }
    catch (const exception& e)
    {
        cerr << "Критическая ошибка: " << e.what() << endl;
        return 1;
    }

    return 0;
}
