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
    size_t dramSize;
    size_t curDramSize;
    size_t nvmSize;
    size_t curNvmSize;
public:
    ClockCache(PMmanager *pm, size_t dramSize, size_t nvmSize);
    ~ClockCache();
    void put(const string& key, const string& value);
    bool get(const string& key, string* value);
    void triggerSwapWithDRAM(NvmNode* node);
    void ClockCache::evictDramNode();
};




