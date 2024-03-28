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
    // 测试在特定条件下触发与DRAM的节点交换
    // 需要设置特定的场景，例如NVM节点处于特定状态，然后调用triggerSwapWithDRAM()，并验证交换是否成功执行
}

TEST_F(ClockCacheTest, SwapNodes) {
    // 测试交换DRAM节点和NVM节点的逻辑
    // 可能需要先创建一些DRAM和NVM节点，然后调用swapNodes()进行交换，并验证交换后的结果是否符合预期
}
