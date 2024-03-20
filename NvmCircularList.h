#ifndef CIRCULAR_LIST_H
#define CIRCULAR_LIST_H
#include <iostream>
#include <cstdint>
#include <string>
#include <cstring> // 用于strcpy
#include "pm_manager.h"

class NvmNode {
public:
    char* key;
    char* data;
    size_t size;
    NvmNode* prev;
    NvmNode* next;
    struct {
        unsigned int reference : 1; 
        unsigned int status : 2;    
        unsigned int twiceRead : 1; 
    } attributes;

    enum NvmNodeStatus {
        Inital = 0,
        Be_Written = 1,
        Pre_Migration = 2,
        Migration = 3
    };


    NvmNode(char* key, char* data, size_t size): prev(nullptr), next(nullptr), size(size) {
        // 假设key和data已经指向了合适的内存位置
        this->key = key;
        this->data = data;
        attributes.reference = 0; 
        attributes.status = 0;    
        attributes.twiceRead = 0;
    }
};

// NVM Circular Linked List
class NvmCircularLinkedList {
public:
    NvmNode* head;
    PMmanager* pm_;

    NvmCircularLinkedList(PMmanager* pm): head(nullptr), pm_(pm) {}

    NvmNode* createNode(std::string key, std::string data) {
        size_t keySize = key.size() + 1; // 加1为了null终结符
        size_t dataSize = data.size() + 1; // 同上

        // 计算总大小，包括Node本身的大小和两个字符串的大小
        size_t totalSize = sizeof(NvmNode) + keySize + dataSize;

        // 分配足够的内存
        void* ptr = pm_->Allocate(totalSize);

        // 计算key和data字符串应该存储的位置
        char* keyPtr = reinterpret_cast<char*>(ptr) + sizeof(NvmNode);
        char* dataPtr = keyPtr + keySize;

        // 复制字符串到分配的内存
        strcpy(keyPtr, key.c_str());
        strcpy(dataPtr, data.c_str());

        // 在分配的内存上构造Node对象
        NvmNode* newNode = new (ptr) NvmNode(keyPtr, dataPtr, totalSize);

        return newNode;
    }

    void insertNode(std::string key, std::string data) {
        NvmNode* newNode = createNode(key, data);
        if (head == nullptr) {
            head = newNode;
            newNode->next = newNode; // 指向自己，形成循环
            newNode->prev = newNode; // 同上
        } else {
            newNode->next = head;
            newNode->prev = head->prev;
            head->prev->next = newNode;
            head->prev = newNode;
        }
    }

    void deleteNode(NvmNode* node) {
        if (node == node->next) {
            head = nullptr;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            if (head == node) head = node->next;
        }
        pm_->Free(node);
    }

    ~NvmCircularLinkedList() {
        while (head != nullptr && head != head->next) {
            deleteNode(head);
        }
        if (head) {
            pm_->Free(head);
            head = nullptr;
        }
    }
};



#endif // CIRCULAR_LIST_H