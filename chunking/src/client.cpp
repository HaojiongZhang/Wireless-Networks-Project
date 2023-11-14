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

#define KB 1024

const int CHUNK_SIZE = 1024; // Must be the same as in the sender program

struct Packet {
    int chunkNumber;
    char data[CHUNK_SIZE];
};

std::mutex fileMutex;
std::map<int, std::vector<char>> chunkMap;

int createSocketAndBind(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        exit(1);
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

void receiveData(int socket, std::ofstream& file, int& nextChunk) {
    while (true) {
        Packet packet;
        ssize_t n = recv(socket, &packet, sizeof(packet), 0);
        if (n <= 0) break;

        std::cout << "Received chunk number: " << packet.chunkNumber << std::endl;
        std::lock_guard<std::mutex> lock(fileMutex);
        chunkMap[packet.chunkNumber] = std::vector<char>(packet.data, packet.data + n - sizeof(int));

        // Write chunks in order if available
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

    int socket1 = createSocketAndBind(port);
    int socket2 = createSocketAndBind(port + 1);

    int newSocket1 = acceptConnection(socket1);
    int newSocket2 = acceptConnection(socket2);

    int nextChunk = 0;
    std::thread thread1(receiveData, newSocket1, std::ref(file), std::ref(nextChunk));
    std::thread thread2(receiveData, newSocket2, std::ref(file), std::ref(nextChunk));

    thread1.join();
    thread2.join();

    close(newSocket1);
    close(newSocket2);
    file.close();

    return 0;
}
