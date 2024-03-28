#include "NvmCircularList.h"
#include "DramCircularList.h"
#include "pm_manager.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <list>
#include <gtest/gtest.h>

using std::string;
using std::unordered_map;

class ClockCacheTest; // 前置声明测试类
// 採用clock-RWRF cache algorithm
class ClockCache
{
private:
    friend class ClockCacheTest;
    PMmanager *pm;
    NvmCircularLinkedList nvm_list;
    DramCircularLinkedList dram_list;
    unordered_map<string, NvmNode*> nvm_cacheMap;
    unordered_map<string, DramNode*> dram_cacheMap;
    size_t dramSize;
    size_t nvmSize;
public:
    ClockCache(PMmanager *pm, size_t dramSize, size_t nvmSize);
    ~ClockCache();
    void put(const string& key, const string& value);
    bool get(const string& key, string* value);
    void triggerSwapWithDRAM(NvmNode* node);
    //This function is used to evict node from dram cache
    void evictDramNode();

    //This function is used to swap a DRAM node with an NVM node. 
    //When using it, ensure that both the DRAM cache and the NVM cache have enough space available for the swap.
    void swapNodes(NvmNode* nvmNode, DramNode* dramNode);

    //This fuction is used for testing
    FRIEND_TEST(ClockCacheTest, EvictDramNode);
};



