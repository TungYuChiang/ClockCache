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
        while (cache->dram_list.currentSize + nodeSizeApprox <= cache->dramSize) {
            cache->dram_list.insertNode("fillKey", "fillData");
        }
    }

    // 将DRAM填充到一半容量
    void fillDramHalfway(ClockCache* cache) {
        size_t nodeSizeApprox = 100; // 假设每个节点大约占用100字节
        size_t halfCapacity = cache->dramSize / 2;
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

TEST_F(ClockCacheTest, EvictDramNode) {
    // 假设每个节点大约占用100字节的空间，为简单起见不考虑实际的sizeof(DramNode)
    size_t approximateNodeSize = 100;
    size_t dramCapacity = clockCache->dramSize;
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
