#include "NvmCircularList.h"
#include "pm_manager.h"
#include <gtest/gtest.h>
#include <string>

class CircularListNvmTest : public ::testing::Test {
protected:

    const size_t capacity = 4 * 100; //400 byte
    PMmanager* pm;

    void SetUp() override {
        // Assuming PMmanager can be instantiated without arguments or has a default constructor for simplicity
        pm = new PMmanager("circular_list_gtest");
    }

    void TearDown() override {
        delete pm;
    }
};


TEST_F(CircularListNvmTest, InsertAndCheckIntegrity) {
    NvmCircularLinkedList list(pm);

    list.insertNode("key1", "data1");
    NvmNode* head = list.head;
    ASSERT_NE(head, nullptr);
    EXPECT_EQ(head, head->next); // 循环链表中只有一个元素时，next指向自己
    EXPECT_EQ(head, head->prev); // 同上，prev也指向自己
    EXPECT_STREQ(head->key, "key1");
    EXPECT_STREQ(head->data, "data1");

    list.insertNode("key2", "data2");
    EXPECT_EQ(head, head->prev->next); // 确保循环性，头的前一个的下一个是头
    EXPECT_EQ(head, head->next->prev); // 确保循环性，头的下一个的前一个是头
    EXPECT_STREQ(head->next->key, "key2");
    EXPECT_STREQ(head->next->data, "data2");

    // 确保能够形成闭环
    EXPECT_EQ(head->next->next, head);
    EXPECT_EQ(head->prev, head->next);
}

TEST_F(CircularListNvmTest, DeleteNodeUpdatesLinksCorrectly) {
    NvmCircularLinkedList list(pm);

    list.insertNode("key1", "data1");
    list.insertNode("key2", "data2");
    list.insertNode("key3", "data3");

    NvmNode* nodeToDelete = list.head->next; // 删除中间节点，假设是key2
    list.deleteNode(nodeToDelete);

    EXPECT_STREQ(list.head->key, "key1");
    EXPECT_STREQ(list.head->next->key, "key3");
    EXPECT_EQ(list.head->next->next, list.head); // 确保删除后仍然是循环链表
    EXPECT_EQ(list.head->prev, list.head->next); // 确保删除后头节点的prev正确指向新的尾节点
}

TEST_F(CircularListNvmTest, DeleteLastNode) {
    NvmCircularLinkedList list(pm);

    list.insertNode("key1", "data1");
    NvmNode* nodeToDelete = list.head;
    list.deleteNode(nodeToDelete);

    EXPECT_EQ(list.head, nullptr); // 确保删除最后一个节点后head正确更新为nullptr
}

TEST_F(CircularListNvmTest, MultipleInsertionsAndDeletions) {
    NvmCircularLinkedList list(pm);

    // 插入多个节点
    for (int i = 1; i <= 5; ++i) {
        list.insertNode("key" + std::to_string(i), "data" + std::to_string(i));
    }

    // 删除一个中间节点
    list.deleteNode(list.head->next->next); // 假设删除key3

    // 验证节点是否正确删除并且链表结构正确
    NvmNode* current = list.head;
    int count = 0;
    do {
        EXPECT_NE(current, nullptr);
        current = current->next;
        ++count;
    } while (current != list.head);

    EXPECT_EQ(count, 4); // 确保链表中只剩下4个节点

    // 删除剩下的节点
    while (list.head != nullptr) {
        list.deleteNode(list.head);
    }

    EXPECT_EQ(list.head, nullptr); // 确保最终链表为空
}
TEST_F(CircularListNvmTest, DeleteFromSingleItemList) {
    NvmCircularLinkedList list(pm);

    list.insertNode("singleKey", "singleData");
    NvmNode* nodeToDelete = list.head;
    list.deleteNode(nodeToDelete);

    EXPECT_EQ(list.head, nullptr);
}

//測試enum
TEST_F(CircularListNvmTest, NodeStatusTransition) {
    NvmCircularLinkedList list(pm);
    list.insertNode("key", "data");
    NvmNode* node = list.head;

    // 假设 setStatus 是一个公共方法，用于改变节点的状态
    node->setStatus(NvmNode::Be_Written);
    EXPECT_EQ(node->getStatus(), NvmNode::Be_Written);

    node->setStatus(NvmNode::Pre_Migration);
    EXPECT_EQ(node->getStatus(), NvmNode::Pre_Migration);

    node->setStatus(NvmNode::Migration);
    EXPECT_EQ(node->getStatus(), NvmNode::Migration);
}
// 测试插入节点后currentSize的更新
TEST_F(CircularListNvmTest, CurrentSizeAfterInsertion) {
    NvmCircularLinkedList list(pm);

    list.insertNode("key1", "data1");
    size_t expectedSizeAfterFirstInsert = sizeof(NvmNode) + strlen("key1") + 1 + strlen("data1") + 1;
    EXPECT_EQ(list.currentSize, expectedSizeAfterFirstInsert);

    list.insertNode("key2", "data2");
    size_t expectedSizeAfterSecondInsert = expectedSizeAfterFirstInsert + sizeof(NvmNode) + strlen("key2") + 1 + strlen("data2") + 1;
    EXPECT_EQ(list.currentSize, expectedSizeAfterSecondInsert);
}

// 测试删除节点后currentSize的更新
TEST_F(CircularListNvmTest, CurrentSizeAfterDeletion) {
    NvmCircularLinkedList list(pm);

    list.insertNode("key1", "data1");
    list.insertNode("key2", "data2");
    size_t sizeBeforeDeletion = list.currentSize;

    NvmNode* nodeToDelete = list.head->next; // 删除第二个插入的节点
    list.deleteNode(nodeToDelete);
    size_t expectedSizeAfterDeletion = sizeBeforeDeletion - (sizeof(NvmNode) + strlen("key2") + 1 + strlen("data2") + 1);
    EXPECT_EQ(list.currentSize, expectedSizeAfterDeletion);
}

// 测试清空链表后currentSize是否为0
TEST_F(CircularListNvmTest, CurrentSizeAfterClearingList) {
    NvmCircularLinkedList list(pm);

    list.insertNode("key1", "data1");
    list.insertNode("key2", "data2");
    list.insertNode("key3", "data3");

    // 逐个删除所有节点
    while (list.head != nullptr && list.head->next != list.head) {
        list.deleteNode(list.head);
    }
    // 最后删除剩余的最后一个节点
    list.deleteNode(list.head);

    // 验证链表已清空且currentSize为0
    EXPECT_EQ(list.head, nullptr);
    EXPECT_EQ(list.currentSize, 0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

