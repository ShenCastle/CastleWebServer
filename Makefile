CXX := g++
CXXFLAGS := -std=c++14 -O2 -Wall -g
LIBS := -pthread -lmysqlclient
TARGET := server
OBJS := $(wildcard code/*.cpp code/*/*.cpp)
.PHONY: all clean
all: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET) $(LIBS)
clean:
	rm -rf $(TARGET)