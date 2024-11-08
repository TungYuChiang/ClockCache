CC = g++
CFLAGS = -g
LIBS = -lgtest -lpthread -lpmemobj -lpmem

# NVM 测试源文件
NVM_TEST_SOURCE = NvmCircularListTest.cc pm_manager.cc
# DRAM 测试源文件
DRAM_TEST_SOURCE = DramCircularListTest.cc
# ClockRWRFCache 测试源文件
CLOCK_RWRFCACHE_TEST_SOURCE = ClockRWRFCacheTest.cc pm_manager.cc ClockRWRFCache.cc

# 目标测试执行文件
NVM_TEST_TARGET = NvmCircularListTest
DRAM_TEST_TARGET = DramCircularListTest
CLOCK_RWRFCACHE_TEST_TARGET = ClockRWRFCacheTest

# 目标
all: $(NVM_TEST_TARGET) $(DRAM_TEST_TARGET) $(CLOCK_RWRFCACHE_TEST_TARGET)

$(NVM_TEST_TARGET): $(NVM_TEST_SOURCE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(DRAM_TEST_TARGET): $(DRAM_TEST_SOURCE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS) -lgtest_main

$(CLOCK_RWRFCACHE_TEST_TARGET): $(CLOCK_RWRFCACHE_TEST_SOURCE)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS) -lgtest_main

clean:
	rm -f $(NVM_TEST_TARGET) $(DRAM_TEST_TARGET) $(CLOCK_RWRFCACHE_TEST_TARGET)
