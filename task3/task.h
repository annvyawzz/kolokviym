#ifndef TASK_H
#define TASK_H

#include <memory>
#include <vector>
#include <stdexcept>
#include <functional>
#include <string>

template<typename T>
class ListNode 
{
public:
    T data;
    std::unique_ptr<ListNode<T>> next;

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
    std::unique_ptr<ListNode<T>> head;
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

    void pushFront(const T& value)
    {
        auto newNode = std::make_unique<ListNode<T>>(value);
        newNode->next = std::move(head);
        head = std::move(newNode);
        size_++;
    }

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
        current->next = std::make_unique<ListNode<T>>(value);
        size_++;
    }

    void reverseRecursive() 
    {
        if (!head || !head->next) 
        {
            return;
        }

        try {
            head = reverseRecursiveImpl(std::move(head));
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("fail: " + std::string(e.what()));
        }
    }

    size_t size() const { return size_; }

    bool empty() const { return size_ == 0; }

    void clear()
    {
        while (head) 
        {
            head = std::move(head->next);
        }
        size_ = 0;
    }

    std::vector<T> toVector() const
    {
        std::vector<T> result;
        ListNode<T>* current = head.get();
        while (current) {
            result.push_back(current->data);
            current = current->next.get();
        }
        return result;
    }

    void print(const std::string& name = "List") const
    {
        std::cout << name << ": ";
        ListNode<T>* current = head.get();
        while (current) 
        {
            std::cout << current->data;
            if (current->next) {
                std::cout << " -> ";
            }
            current = current->next.get();
        }
        std::cout << " -> NULL" << std::endl;
    }

    void traverseRecursive(std::function<void(const T&)> func) const 
    {
        traverseRecursiveImpl(head.get(), func);
    }

private:
    std::unique_ptr<ListNode<T>> reverseRecursiveImpl(std::unique_ptr<ListNode<T>> node) 
    {
        if (!node) 
        {
            return nullptr;
        }

        if (!node->next)
        {
            return node;
        }

        std::unique_ptr<ListNode<T>> newHead = reverseRecursiveImpl(std::move(node->next));

        ListNode<T>* tail = newHead.get();
        while (tail->next) {
            tail = tail->next.get();
        }
        tail->next = std::move(node);

        return newHead;
    }

    void traverseRecursiveImpl(ListNode<T>* node, std::function<void(const T&)> func) const
    {
        if (!node) return;
        func(node->data);
        traverseRecursiveImpl(node->next.get(), func);
    }
};

#endif // TASK_H
