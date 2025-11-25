#include <iostream>
#include "task.h"

void demonstrateExamples()
{
    std::cout << "Демонстрация работы" << std::endl;

    // Разворот числового списка
    {
        LinkedList<int> numbers;
        numbers.pushBack(1);
        numbers.pushBack(2);
        numbers.pushBack(3);
        numbers.pushBack(4);
        numbers.pushBack(5);

        std::cout << "Числа:" << std::endl;
        numbers.print("До разворота");
        numbers.reverseRecursive();
        numbers.print("После разворота");
    }

    // Разворот строкового списка
    {
        LinkedList<std::string> words;
        words.pushBack("Hello");
        words.pushBack("World");
        words.pushBack("!");

        std::cout << "Строки:" << std::endl;
        words.print("До разворота");
        words.reverseRecursive();
        words.print("После разворота");
    }

    // Рекурсивный обход
    {
        LinkedList<int> list;
        for (int i = 1; i <= 5; ++i) {
            list.pushBack(i * 10);
        }

        std::cout << "Рекурсивный обход:" << std::endl;
        list.print("Список");
        std::cout << "Элементы: ";
        list.traverseRecursive([](const int& data) {
            std::cout << data << " ";
        });
        std::cout << std::endl;
    }
}

void interactiveDemo() {
    std::cout << "Интерактивная демонстрация" << std::endl;
    std::cout << "Вводите числа по одному (или 'q' для завершения):" << std::endl;

    LinkedList<int> list;
    std::string input;

    while (true) {
        std::cout << "Введите число: ";
        std::cin >> input;

        if (input == "q" || input == "exit") {
            break;
        }

        try {
            int number = std::stoi(input);
            list.pushBack(number);
            list.print("Текущий список");
        }
        catch (const std::exception& e) {
            std::cout << "Ошибка ввода! Попробуйте снова." << std::endl;
            std::cin.clear();
        }
    }

    if (!list.empty()) {
        std::cout << "Разворачиваем список" << std::endl;
        list.reverseRecursive();
        list.print("Развернутый список");

        std::cout << "Элементы в обратном порядке: ";
        list.traverseRecursive([](const int& data) {
            std::cout << data << " ";
        });
        std::cout << std::endl;
    }
}

int main() {
    try {
        std::cout << "Запуск демонстрационных примеров" << std::endl;
        demonstrateExamples();
        
        std::cout << "\nХотите попробовать интерактивную демонстрацию? (y/n): ";
        char choice;
        std::cin >> choice;
        
        if (choice == 'y' || choice == 'Y') {
            interactiveDemo();
        }

        std::cout << "Программа завершена успешно!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
