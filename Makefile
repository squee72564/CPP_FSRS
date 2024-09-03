CXX = g++
CXXFLAGS = -O3 -Wall -std=c++17
CXXSRC = ./src/models.cpp ./src/FSRS.cpp ./tests/test_fsrs.cpp
CXXINCLUDE = ./include

TARGET=space_repitition_algo

all: ${TARGET}

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(CXXSRC) -I$(CXXINCLUDE)

clean:
	rm -f $(TARGET)
