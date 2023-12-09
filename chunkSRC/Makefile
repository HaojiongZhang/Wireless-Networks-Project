CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread

all: sender receiver

sender: sender.cpp
	$(CXX) $(CXXFLAGS) sender.cpp -o sender

receiver: receiver.cpp
	$(CXX) $(CXXFLAGS) receiver.cpp -o receiver

clean:
	rm -f sender receiver
