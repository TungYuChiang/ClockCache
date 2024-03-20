CC = g++
CFLAGS = -g
LIBS = -lgtest -lpthread -lpmemobj -lpmem

# 源文件
SOURCES = circular_list_test.cc pm_manager.cc

# 目標
all: circular_list_test

circular_list_test: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LIBS)

clean:
	rm -f circular_list_test
