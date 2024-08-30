CXX = g++
CXXFLAGS = -O3 -Wall -std=c++17
CXXSRC = ./tests/test_fsrs.cpp
CXXINCLUDE = ./include

TARGET=space_repitition_algo

all: ${TARGET}

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(CXXSRC) -I$(CXXINCLUDE)

clean:
	rm -f $(TARGET)
