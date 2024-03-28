#include "ClockRWRFCache.h"
#include <iostream> 
#include <algorithm>

ClockCache::ClockCache(PMmanager* pm, size_t dramSize, size_t nvmSize) 
    : pm(pm), dramSize(dramSize), nvmSize(nvmSize), nvm_list(pm) {}

//TODO
ClockCache::~ClockCache(){}

void ClockCache::put(const string& key, const string& value) {
    size_t newNodeSize = key.size() + value.size() + sizeof(DramNode); // 计算新节点的大小
    if (newNodeSize > dramSize) {
        // 如果新节点本身就大于DRAM的总容量，无法插入
        // TODO: 返回错误或记录日志
        return; // 直接返回，不执行插入
    }

    // 1. 檢查DRAM是否有該key
    auto dramIt = dram_cacheMap.find(key);
    if (dramIt != dram_cacheMap.end()) {
        // 獲取舊節點的狀態並將其刪除
        auto oldNode = dramIt->second;
        auto oldStatus = oldNode->attributes.status; // 假設使用 attributes.status 來存儲狀態
        size_t oldNodeSize = dramIt->second->size;
        dram_list.deleteNode(oldNode);
        dram_cacheMap.erase(dramIt);
        //檢查空間
        while (dram_list.currentSize + newNodeSize > dramSize) {
            evictDramNode();
        }
        // 插入新節點
        dram_list.insertNode(key, value);
        auto newNode = dram_list.head->prev; // 新節點是列表的最後一個節點
        dram_cacheMap[key] = newNode;
        newNode->attributes.reference = 1;

        // 更新狀態
        unsigned int newStatus = std::max(static_cast<unsigned int>(0), oldStatus - 1); // 確保狀態不會小於0
        newNode->attributes.status = newStatus; // 直接設置狀態

        return;
    }

    // 2. 檢查NVM是否有該key
    auto nvmIt = nvm_cacheMap.find(key);
    if (nvmIt != nvm_cacheMap.end()) {
        // 獲取舊節點的狀態並將其刪除
        auto oldNode = nvmIt->second;
        auto oldStatus = oldNode->attributes.status;
        nvm_list.deleteNode(oldNode);
        nvm_cacheMap.erase(nvmIt);
        
        //TODO: NVM 的空間檢查，若是value值比原本大要怎麼處理

        // Insert Node 
        nvm_list.insertNode(key, value);
        auto newNode = nvm_list.head->prev; 
        nvm_cacheMap[key] = newNode;
        newNode->attributes.reference = 1;
        

        // 更新狀態
        unsigned int newStatus = (oldStatus + 1);
        newNode->attributes.status = newStatus;
        if (newStatus == 2 || newStatus == 3) {
            triggerSwapWithDRAM(newNode);
        }
        return;
    }

    // 3. 如果在两个cache中都没有找到key
    // 首先检查DRAM缓存是否有足够的空间
    while (dram_list.currentSize + newNodeSize > dramSize) {
        evictDramNode();
    }
    // 挪出空間後，插入新的Node
    dram_list.insertNode(key, value);
    auto newNode = dram_list.head->prev; // 新節點是列表的最後一個節點
    newNode->attributes.reference = 1;
    dram_cacheMap[key] = newNode;

    return;
}

bool ClockCache::get(const string& key, string* value) {

}

void ClockCache::triggerSwapWithDRAM(NvmNode* nvmNode) {
    // 檢查NVM節點的狀態
    unsigned int nvmNodeStatus = nvmNode->attributes.status;

    if (nvmNodeStatus != 2 && nvmNodeStatus != 3) {
        // 如果NVM節點的狀態不是Pre-Migration或Migration，則不執行任何操作
        return;
    }

    // 嘗試在DRAM中找到合適的節點進行交換
    bool foundSuitableDramNode = false;
    DramNode* candidate = dram_list.head;
    //Pre-Migration and Migration 
    do {
        if ((candidate->attributes.status == 2 || candidate->attributes.status == 3) &&
            candidate->attributes.reference == 0) {
            // 找到了合適的DRAM節點進行交換
            foundSuitableDramNode = true;
            swapNodes(nvmNode, candidate);
            break;
        } else if (candidate->attributes.reference == 1) {
            // 將reference位設置為0並繼續找
            candidate->attributes.reference = 0;
        }
        candidate = candidate->next;
    } while (candidate != dram_list.head);
    //Migration
    if (!foundSuitableDramNode && nvmNodeStatus == 3) {
        // 如果NVM節點狀態為Migration但未找到合適的DRAM節點，且DRAM空間不足，則逐出DRAM節點
         size_t requiredSize = sizeof(DramNode) + strlen(nvmNode->key) + 1 + strlen(nvmNode->data) + 1;
        while (dram_list.currentSize + requiredSize > dramSize) {
            evictDramNode();
        }
        // 現在DRAM有足夠空間，執行NVM到DRAM的節點遷移
        string key(nvmNode->key);
        string data(nvmNode->data);
        dram_list.insertNode(key, data);
        nvm_list.deleteNode(nvmNode);
        nvm_cacheMap.erase(key); 
    } else if (!foundSuitableDramNode && nvmNodeStatus == 2) {
        // 如果NVM節點狀態為Pre-Migration但未找到合適的DRAM節點，且不強制交換，則不進行操作
        // 可以選擇記錄日誌或其他操作
    }
}

void ClockCache::evictDramNode() {
    if (dram_list.head == nullptr) return; // 确保DRAM列表非空

    DramNode* candidate = dram_list.head;
    do {
        if (candidate->attributes.reference == 0) {
            // 找到第一个reference为0的节点，将其逐出
            // 从DRAM链表和缓存映射中移除节点
            dram_cacheMap.erase(candidate->key); // 假设DramNode有存储key
            dram_list.deleteNode(candidate);

            return; // 完成逐出操作后返回
        } else {
            // 将reference位设置为0并继续遍历
            candidate->attributes.reference = 0;
            candidate = candidate->next;
        }
    } while (candidate != dram_list.head); // 循环直到回到起点

    // 如果所有节点的reference位都已经是0，但仍然需要逐出节点，则选择头节点逐出
    // 这是一个后备方案，实际中应该很少发生，因为上面的逻辑已经尝试将所有节点的reference置为0
    if (candidate == dram_list.head) {
        dram_cacheMap.erase(candidate->key);
        dram_list.deleteNode(candidate);
    }
}


void ClockCache::swapNodes(NvmNode* nvmNode, DramNode* dramNode) {
    string dramKey(dramNode->key);
    string dramData(dramNode->data);
    string nvmKey(nvmNode->key);
    string nvmData(nvmNode->data);

    // 从各自的链表和映射中移除节点
    dram_list.deleteNode(dramNode);
    dram_cacheMap.erase(dramKey);

    nvm_list.deleteNode(nvmNode);
    nvm_cacheMap.erase(nvmKey);

    // 使用保存的数据将DRAM节点迁移到NVM
    nvm_list.insertNode(dramKey, dramData);
    auto newNvmNode = nvm_list.head->prev; // 获取新插入的NVM节点
    nvm_cacheMap[dramKey] = newNvmNode;

    // 使用保存的数据将NVM节点迁移到DRAM
    dram_list.insertNode(nvmKey, nvmData);
    auto newDramNode = dram_list.head->prev; // 获取新插入的DRAM节点
    dram_cacheMap[nvmKey] = newDramNode;

    // TODO: 更新节点的状态或其他属性，标记为最近访问
    newNvmNode->attributes.reference = 1;
    newDramNode->attributes.reference = 1;
}

