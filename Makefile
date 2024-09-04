CXX = g++
CXXFLAGS = -O3 -Wall -Werror -Wpedantic -std=c++17
CXXSRC = ./src/models.cpp ./src/FSRS.cpp ./tests/test_fsrs.cpp
CXXINCLUDE = ./include

TESTTARGET=./tests/space_repitition_test
OBJS=$(CXXSRC:.cpp=.o)

all: clean ${TESTTARGET}

$(TESTTARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TESTTARGET) $(OBJS) -I$(CXXINCLUDE)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(CXXINCLUDE) -c $< -o $@

clean:
	rm -f $(TESTTARGET) $(OBJS)
