#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <map>
#include <net/if.h>
#include <cstring>
#include <condition_variable>



#define KB 1024

const int CHUNK_SIZE = 1024; // Must be the same as in the sender program
bool finishedReceiving;
std::condition_variable cv;

struct Packet {
    int chunkNumber;
    char data[CHUNK_SIZE];
};

std::mutex chunkMapMutex;
std::map<int, std::vector<char>> chunkMap;

int createSocketAndBind(int port, const char* interfaceName) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interfaceName, strlen(interfaceName)) < 0) {
        std::cerr << "Error binding to interface" << std::endl;
        close(sockfd);
        return -1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    

    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error on binding" << std::endl;
        exit(1);
    }

    return sockfd;
}



int acceptConnection(int sockfd) {
    listen(sockfd, 5);
    sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    int newsockfd = accept(sockfd, (sockaddr*)&cli_addr, &clilen);
    if (newsockfd < 0) {
        std::cerr << "Error on accept" << std::endl;
        exit(1);
    }

    return newsockfd;
}


void receiveData(int socket) {
    while (true) {
        Packet packet;
        ssize_t n = recv(socket, &packet, sizeof(packet), 0);
        if (n <= 0) {
            std::lock_guard<std::mutex> lock(chunkMapMutex);
            finishedReceiving = true;
            cv.notify_one();
            break;
        }

        std::cout << "Received chunk number: " << packet.chunkNumber << std::endl;
        {
            std::lock_guard<std::mutex> lock(chunkMapMutex);
            chunkMap[packet.chunkNumber] = std::vector<char>(packet.data, packet.data + n - sizeof(int));
        }
        cv.notify_one();
    }
}

void writeData(std::ofstream& file, int& nextChunk) {
    std::unique_lock<std::mutex> lock(chunkMapMutex);
    while (!finishedReceiving || !chunkMap.empty()) {
        cv.wait(lock, [&]() { return finishedReceiving || !chunkMap.empty(); });

        auto it = chunkMap.find(nextChunk);
        while (it != chunkMap.end()) {
            std::cout << "Writing chunk number: " << it->first << std::endl;
            file.write(it->second.data(), it->second.size());
            chunkMap.erase(it);
            nextChunk++;
            it = chunkMap.find(nextChunk);
        }
    }
}
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <destination file>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    const char* destinationFile = argv[2];
    std::ofstream file(destinationFile, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return 1;
    }

    const char* interfaceName1 = "wlan0";
    const char* interfaceName2 = "espst0";

    int socket1 = createSocketAndBind(port, interfaceName1);
    int socket2 = createSocketAndBind(port + 1, interfaceName2);

    int newSocket1 = acceptConnection(socket1);
    int newSocket2 = acceptConnection(socket2);

    int nextChunk = 0;
    finishedReceiving = false;
    std::thread thread1(receiveData, newSocket1);
    std::thread thread2(receiveData, newSocket2);
    std::thread writerThread(writeData, std::ref(file), std::ref(nextChunk));

    thread1.join();
    thread2.join();
    writerThread.join();

    close(newSocket1);
    close(newSocket2);
    file.close();

    return 0;
}
