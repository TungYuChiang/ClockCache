#include "NvmCircularList.h"
#include "DramCircularList.h"
#include "pm_manager.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <list>

using std::string;
using std::unordered_map;

// 採用clock-RWRF cache algorithm
class ClockCache
{
private:
    PMmanager *pm;
    NvmCircularLinkedList nvm_list;
    DramCircularLinkedList dram_list;
    unordered_map<string, NvmNode*> nvm_cacheMap;
    unordered_map<string, DramNode*> dram_cacheMap;
public:
    ClockCache(PMmanager *pm);
    ~ClockCache();
    void put(const string& key, const string& value);
    bool get(const string& key, string* value);
};


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
        Twice_readf = 2,
        Be_Migration = 3
    };


    DramNode(char* key, char* data, size_t size): prev(nullptr), next(nullptr), size(size) {
        // 假设key和data已经指向了合适的内存位置
        this->key = key;
        this->data = data;
        attributes.reference = 0; 
        attributes.status = 0;    
    }
};


