CXX = g++
COMPILERFLAGS = -g -Wall -Wextra -Wno-sign-compare -pthread -std=c++11 -Isrc


SERVEROBJECTS = obj/server.o obj/fileIO.o
CLIENTOBJECTS = obj/client.o obj/fileIO.o
TESTOBJS = obj/test.o obj/fileIO.o

.PHONY: all clean

all : obj server client test

server: $(SERVEROBJECTS)
	$(CXX) $(COMPILERFLAGS) $^ -o $@ 

client: $(CLIENTOBJECTS)
	$(CXX) $(COMPILERFLAGS) $^ -o $@

test: $(TESTOBJS)
	$(CXX) $(COMPILERFLAGS) $^ -o $@

clean :
	$(RM) obj/*.o server client test

obj/%.o: src/%.cpp
	$(CXX) $(COMPILERFLAGS) -c -o $@ $<
obj:
	mkdir -p obj
