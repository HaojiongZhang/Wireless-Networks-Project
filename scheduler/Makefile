CXX = g++
COMPILERFLAGS = -g -Wall -Wextra -Wno-sign-compare -pthread -std=c++11

RECEIVEOBJECTS = obj/reliableReceive.o
TRANSFEROBJECTS = obj/reliableTransfer.o

.PHONY: all clean setup

all: setup reliableReceive reliableTransfer

reliableReceive: $(RECEIVEOBJECTS)
	$(CXX) $(COMPILERFLAGS) $^ -o $@

reliableTransfer: $(TRANSFEROBJECTS)
	$(CXX) $(COMPILERFLAGS) $^ -o $@

clean:
	$(RM) obj/*.o reliableReceive reliableTransfer

obj/%.o: src/%.cpp
	$(CXX) $(COMPILERFLAGS) -c -o $@ $<

setup:
	mkdir -p obj
