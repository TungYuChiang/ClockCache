#ifndef CIRCULAR_LIST_DRAM_H
#define CIRCULAR_LIST_DRAM_H

#include <iostream>
#include <cstdint>
#include <string>
#include <cstring> // 用于strcpy

using std::string;
class DramNode {
public:
    char* key;
    char* data;
    size_t size;
    DramNode* prev;
    DramNode* next;
    struct {
        unsigned int reference : 1; 
        unsigned int status : 2;     
    } attributes;

    enum DramNodeStatus {
        Inital = 0,
        Once_read = 1,
        Twice_read = 2,
        Be_Migration = 3
    };

    void setStatus(DramNodeStatus status) {
        attributes.status = static_cast<unsigned int>(status);
    }

    // 获取状态枚举值
    DramNodeStatus getStatus() const {
        return static_cast<DramNodeStatus>(attributes.status);
    }


    DramNode(const std::string& key, const std::string& data, size_t size): prev(nullptr), next(nullptr), size(size){
        size_t keySize = key.size() + 1;
        size_t dataSize = data.size() + 1;
        this->key = new char[keySize];
        this->data = new char[dataSize];
        std::strcpy(this->key, key.c_str());
        std::strcpy(this->data, data.c_str());
        attributes.reference = 0; 
        attributes.status = 0; 
    }
};

class DramCircularLinkedList {
public:
    DramNode* head;

    DramCircularLinkedList(): head(nullptr) {}

    void insertNode(const std::string& key, const std::string& data) {
        size_t size = key.size() + data.size() + sizeof(DramNode);
        DramNode* newNode = new DramNode(key, data, size);
        if (head == nullptr) {
            head = newNode;
            newNode->next = newNode;
            newNode->prev = newNode;
        } else {
            newNode->next = head;
            newNode->prev = head->prev;
            head->prev->next = newNode;
            head->prev = newNode;
        }
    }

    void deleteNode(DramNode* node) {
        if (node == node->next) {
            head = nullptr;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            if (head == node) head = node->next;
        }
        delete node;
    }

    ~DramCircularLinkedList() {
        while (head != nullptr && head != head->next) {
            deleteNode(head);
        }
        if (head) {
            delete head;
            head = nullptr;
        }
    }
};

#endif // CIRCULAR_LIST_DRAM_H
