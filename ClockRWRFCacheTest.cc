#include "ClockRWRFCache.h"
#include <gtest/gtest.h>

class ClockCacheTest : public ::testing::Test {
protected:
    ClockCache* clockCache;
    PMmanager* pm;

    void SetUp() override {
        // 假设PMmanager可以无参数构造或提供一个合适的构造函数
        pm = new PMmanager("ClockRWRFCacheTest");
        size_t dramSize = 1024; // 示例DRAM大小
        size_t nvmSize = 2048;  // 示例NVM大小
        clockCache = new ClockCache(pm, dramSize, nvmSize);
    }

    void TearDown() override {
        delete clockCache;
        delete pm;
    }
    // 填满DRAM，假设每个节点大约占用100字节空间
    void fillDramToCapacity(ClockCache* cache) {
        size_t nodeSizeApprox = 100; // 假设每个节点大约占用100字节
        while (cache->dram_list.currentSize + nodeSizeApprox <= cache->dramCapacity) {
            cache->dram_list.insertNode("fillKey", "fillData");
        }
    }

    // 将DRAM填充到一半容量
    void fillDramHalfway(ClockCache* cache) {
        size_t nodeSizeApprox = 100; // 假设每个节点大约占用100字节
        size_t halfCapacity = cache->dramCapacity / 2;
        while (cache->dram_list.currentSize + nodeSizeApprox <= halfCapacity) {
            cache->dram_list.insertNode("halfFillKey", "halfFillData");
        }
}
};
size_t countDramNodes(DramCircularLinkedList& list) {
    size_t count = 0;
    if (list.head == nullptr) {
        return count; // 如果头节点为空，直接返回计数为0
    }

    DramNode* node = list.head;
    do {
        count++;
        node = node->next;
    } while (node != list.head); // 遍历直到回到头节点

    return count;
}
size_t countNvmNodes(const NvmCircularLinkedList& nvmList) {
    size_t count = 0;
    if (nvmList.head == nullptr) {
        return count; // 如果链表为空，直接返回计数为0
    }

    // 从头节点开始遍历链表
    NvmNode* current = nvmList.head;
    do {
        count++; // 为当前节点计数
        current = current->next; // 移动到下一个节点
    } while (current != nvmList.head); // 直到回到头节点停止

    return count;
}


TEST_F(ClockCacheTest, EvictDramNode) {
    // 假设每个节点大约占用100字节的空间，为简单起见不考虑实际的sizeof(DramNode)
    size_t approximateNodeSize = 100;
    size_t dramCapacity = clockCache->dramCapacity;
    size_t numNodesToFit = dramCapacity / approximateNodeSize;

    // 直接向DRAM填充节点，留下最后一个节点的空间不填，以便测试逐出
    for (size_t i = 0; i < numNodesToFit - 1; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string data = "data" + std::to_string(i);
        // 直接调用dram_list的insertNode方法
        clockCache->dram_list.insertNode(key, data);
    }

    // 记录逐出前的节点数量
    size_t initialNodeCount = countDramNodes(clockCache->dram_list); // 假设你有这样一个方法来计数
    // 调用evictDramNode()逐出一个节点
    clockCache->evictDramNode();

    // 记录逐出后的节点数量
    size_t finalNodeCount = countDramNodes(clockCache->dram_list); // 再次计数
    // 验证DRAM缓存中的节点数量减少
    EXPECT_EQ(finalNodeCount, initialNodeCount - 1);

    // 验证DRAM的currentSize是否减少了大约一个节点的大小
    // 注意：这里的检查可能需要根据实际节点大小进行微调
    EXPECT_LE(clockCache->dram_list.currentSize, dramCapacity - approximateNodeSize);
}
TEST_F(ClockCacheTest, EvictNvmNode) {
    // 假设每个节点大约占用100字节的空间，为简单起见不考虑实际的sizeof(NvmNode)
    size_t approximateNodeSize = 100;
    size_t nvmCapacity = clockCache->nvmCapacity;
    size_t numNodesToFit = nvmCapacity / approximateNodeSize;

    // 直接向NVM填充节点，留下最后一个节点的空间不填，以便测试逐出
    for (size_t i = 0; i < numNodesToFit - 1; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string data = "data" + std::to_string(i);
        // 直接调用nvm_list的insertNode方法
        clockCache->nvm_list.insertNode(key, data);
    }

    // 记录逐出前的节点数量
    size_t initialNodeCount = countNvmNodes(clockCache->nvm_list); // 假设你有这样一个方法来计数

    // 调用evictNvmNode()逐出一个节点
    clockCache->evictNvmNode();

    // 记录逐出后的节点数量
    size_t finalNodeCount = countNvmNodes(clockCache->nvm_list); // 再次计数

    // 验证NVM缓存中的节点数量减少
    EXPECT_EQ(finalNodeCount, initialNodeCount - 1);

    EXPECT_LE(clockCache->nvm_list.currentSize, nvmCapacity - approximateNodeSize);
}

TEST_F(ClockCacheTest, TriggerSwapWithDRAM) {
    // 首先插入一个DRAM节点作为交换的候选对象
    clockCache->dram_list.insertNode("dramKey1", "dramData1");
    DramNode* dramNode = clockCache->dram_list.head;
    dramNode->attributes.status = 2; // 假设2代表Pre-Migration或Migration
    dramNode->attributes.reference = 0;

    // 插入一个NVM节点，并设定其状态为Migration
    // 使用NvmCircularLinkedList的insertNode方法插入节点
    clockCache->nvm_list.insertNode("nvmKey1", "nvmData1");
    NvmNode* nvmNode = clockCache->nvm_list.head; // 假设新插入的节点成为了头节点
    nvmNode->attributes.status = 3; // 设置状态为Migration

    // 执行交换
    clockCache->triggerSwapWithDRAM(nvmNode);

    // 验证：交换后，原NVM节点的键值应该在DRAM中找到
    bool isSwappedToDram = clockCache->dram_cacheMap.find("nvmKey1") != clockCache->dram_cacheMap.end();
    EXPECT_TRUE(isSwappedToDram);

    // 验证：原DRAM节点应该被逐出或交换到NVM中
    bool isOriginalDramNodeEvicted = clockCache->dram_cacheMap.find("dramKey1") == clockCache->dram_cacheMap.end();
    EXPECT_TRUE(isOriginalDramNodeEvicted);

    // 注意：确保在测试结束时适当地管理内存
}

TEST_F(ClockCacheTest, TriggerSwapWithMigrationStateAndDramFull) {
    // 先填满DRAM
    fillDramToCapacity(clockCache);

    // 插入一个NVM节点，并设置其状态为Migration
    clockCache->nvm_list.insertNode("nvmKeyMigration", "nvmDataMigration");
    NvmNode* nvmNode = clockCache->nvm_list.head;
    nvmNode->attributes.status = NvmNode::Migration; // 设置状态为Migration

    // 执行交换
    clockCache->triggerSwapWithDRAM(nvmNode);

    // 验证DRAM中现在有新插入的NVM节点数据
    bool isNvmDataInDram = clockCache->dram_cacheMap.find("nvmKeyMigration") != clockCache->dram_cacheMap.end();
    EXPECT_TRUE(isNvmDataInDram);

    // 验证原NVM节点是否已经被正确逐出或交换到DRAM中
    bool isNvmNodeEvicted = clockCache->nvm_cacheMap.find("nvmKeyMigration") == clockCache->nvm_cacheMap.end();
    EXPECT_TRUE(isNvmNodeEvicted);
}

TEST_F(ClockCacheTest, TriggerSwapWithPreMigrationStateAndDramFull) {
    // 先填满DRAM
    fillDramToCapacity(clockCache);

    // 插入一个NVM节点，并设置其状态为Pre-Migration
    clockCache->nvm_list.insertNode("nvmKeyPreMigration", "nvmDataPreMigration");
    NvmNode* nvmNode = clockCache->nvm_list.head;
    nvmNode->attributes.status = NvmNode::Pre_Migration; // 设置状态为Pre-Migration

    // 执行交换
    clockCache->triggerSwapWithDRAM(nvmNode);

    // 验证：DRAM不应该包含Pre-Migration状态的NVM节点数据，因为不强制交换
    bool isNvmDataInDram = clockCache->dram_cacheMap.find("nvmKeyPreMigration") != clockCache->dram_cacheMap.end();
    EXPECT_FALSE(isNvmDataInDram);
}

TEST_F(ClockCacheTest, TriggerSwapWithPreMigrationStateAndDramNotFull) {
    // 确保DRAM未满，只填充到一半
    fillDramHalfway(clockCache);

    // 插入一个NVM节点，并设置其状态为Pre-Migration
    clockCache->nvm_list.insertNode("nvmKeyPreMigrationNotFull", "nvmDataPreMigrationNotFull");
    NvmNode* nvmNode = clockCache->nvm_list.head;
    nvmNode->attributes.status = NvmNode::Pre_Migration; // 设置状态为Pre-Migration

    // 执行交换
    clockCache->triggerSwapWithDRAM(nvmNode);

    // 验证：如果DRAM中找到了对应的NVM节点数据，说明进行了交换
    bool isNvmDataInDram = clockCache->dram_cacheMap.find("nvmKeyPreMigrationNotFull") != clockCache->dram_cacheMap.end();
    EXPECT_TRUE(isNvmDataInDram);


    // 验证原NVM节点是否已经被正确逐出或交换到DRAM中
    bool isNvmNodeEvicted = clockCache->nvm_cacheMap.find("nvmKeyPreMigrationNotFull") == clockCache->nvm_cacheMap.end();
    EXPECT_TRUE(isNvmNodeEvicted);
}

TEST_F(ClockCacheTest, SwapNodes) {
    // 插入一个DRAM节点
    string dramKey = "dramKey";
    string dramData = "dramData";
    clockCache->dram_list.insertNode(dramKey, dramData);
    clockCache->dram_cacheMap[dramKey] = clockCache->dram_list.head; // 假设最后一个插入的节点成为头节点

    // 插入一个NVM节点
    string nvmKey = "nvmKey";
    string nvmData = "nvmData";
    clockCache->nvm_list.insertNode(nvmKey, nvmData);
    clockCache->nvm_cacheMap[nvmKey] = clockCache->nvm_list.head; // 同上

    // 获取插入的DRAM和NVM节点
    DramNode* dramNode = clockCache->dram_list.head;
    NvmNode* nvmNode = clockCache->nvm_list.head;

    // 执行交换
    clockCache->swapNodes(nvmNode, dramNode);

    // 验证交换后的结果
    // 验证原NVM节点的数据现在在DRAM中
    bool isNvmDataInDram = clockCache->dram_cacheMap.find(nvmKey) != clockCache->dram_cacheMap.end();
    EXPECT_TRUE(isNvmDataInDram);

    // 验证原DRAM节点的数据现在在NVM中
    bool isDramDataInNvm = clockCache->nvm_cacheMap.find(dramKey) != clockCache->nvm_cacheMap.end();
    EXPECT_TRUE(isDramDataInNvm);

    // 验证缓存映射是否正确更新
    EXPECT_EQ(clockCache->dram_cacheMap[nvmKey]->data, nvmData);
    EXPECT_EQ(clockCache->nvm_cacheMap[dramKey]->data, dramData);
}

TEST_F(ClockCacheTest, UpdateValueInDram) {
    string key = "testKey";
    string initialValue = "initialValue";
    string updatedValue = "updatedValue";
    
    // 直接插入初始值到DRAM链表并更新DRAM缓存映射
    clockCache->dram_list.insertNode(key, initialValue); 
    DramNode* insertedNode = clockCache->dram_list.head->prev; 
    clockCache->dram_cacheMap[key] = insertedNode; 

    clockCache->put(key, updatedValue);

    auto it = clockCache->dram_cacheMap.find(key);
    bool found = (it != clockCache->dram_cacheMap.end());
    string retrievedValue = found ? it->second->data : "";

    EXPECT_TRUE(found);
    EXPECT_EQ(retrievedValue, updatedValue);
}

TEST_F(ClockCacheTest, UpdateAndMigrateFromNvm) {
    string key = "nvmKey";
    string initialValue = "initialValue";
    string updatedValue = "updatedValue";
    
    // 直接插入初始值到NVM并更新NVM缓存映射
    clockCache->nvm_list.insertNode(key, initialValue);
    NvmNode* insertedNvmNode = clockCache->nvm_list.head; // 假设插入后成为头节点
    clockCache->nvm_cacheMap[key] = insertedNvmNode;
    insertedNvmNode->attributes.status = NvmNode::Initial; // 显式设置初始状态
    
    // 第一次更新
    clockCache->put(key, updatedValue);
    // 验证更新后的值及状态
    EXPECT_EQ(std::string(clockCache->nvm_cacheMap[key]->data), updatedValue);
    EXPECT_EQ(clockCache->nvm_cacheMap[key]->attributes.status, NvmNode::Be_Written);

    // 第二次更新，模拟状态更新到Pre_Migration
    string updatedValue2 = "updatedValue2";
    clockCache->put(key, updatedValue2);
    // 由于NVM节点状态应该更新到Pre_Migration，并触发迁移，所以我们需要验证迁移是否成功
    EXPECT_TRUE(clockCache->dram_cacheMap.find(key) != clockCache->dram_cacheMap.end()); // 确认迁移至DRAM
    EXPECT_EQ(std::string(clockCache->dram_cacheMap[key]->data), updatedValue2); // 确认DRAM中的数据是最新的
    EXPECT_TRUE(clockCache->nvm_cacheMap.find(key) == clockCache->nvm_cacheMap.end()); // 确认从NVM中移除
}

TEST_F(ClockCacheTest, InsertNewNodeIntoDramWhenNotPresentInBoth) {
    string key = "uniqueKey";
    string value = "testValue";
    
    // 确保key在DRAM和NVM中都不存在
    clockCache->put(key, value);
    
    // 验证key是否被插入到了DRAM中
    auto itDram = clockCache->dram_cacheMap.find(key);
    bool isInsertedInDram = (itDram != clockCache->dram_cacheMap.end());
    EXPECT_TRUE(isInsertedInDram);
    if (isInsertedInDram) {
        EXPECT_EQ(std::string(itDram->second->data), value);
    }
    
    // 验证key是否不存在于NVM中
    auto itNvm = clockCache->nvm_cacheMap.find(key);
    bool isNotPresentInNvm = (itNvm == clockCache->nvm_cacheMap.end());
    EXPECT_TRUE(isNotPresentInNvm);
}

TEST_F(ClockCacheTest, EvictNodeWhenDramIsFull) {
    // 填满DRAM空间，但留有足够的空间插入一个额外的节点
    fillDramToCapacity(clockCache);

    // 现在DRAM应接近满状态，尝试插入一个额外的节点
    string extraKey = "extraKey";
    string extraValue = "extraValue";
    clockCache->put(extraKey, extraValue);
    
    // 验证新节点是否成功插入到DRAM中
    auto itExtra = clockCache->dram_cacheMap.find(extraKey);
    bool isExtraInserted = (itExtra != clockCache->dram_cacheMap.end());
    EXPECT_TRUE(isExtraInserted);
    if (isExtraInserted) {
        // 验证插入的数据是否正确
        EXPECT_EQ(std::string(itExtra->second->data), extraValue);
    }

    // 验证DRAM空间是否通过逐出节点以适应新节点插入而得到维护
    // 这里检查DRAM的当前使用大小是否未超过其容量
    EXPECT_LE(clockCache->dram_list.currentSize, clockCache->dramCapacity);
}







