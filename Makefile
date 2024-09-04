CXX = g++
CXXFLAGS = -O3 -Wall -std=c++17
CXXSRC = ./src/models.cpp ./src/FSRS.cpp ./tests/test_fsrs.cpp
CXXINCLUDE = ./include

TARGET=space_repitition_algo
OBJS=$(CXXSRC:.cpp=.o)

all: clean ${TARGET}

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) -I$(CXXINCLUDE)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(CXXINCLUDE) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
