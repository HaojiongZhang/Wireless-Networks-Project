#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>

#define KB 1024

const int CHUNK_SIZE = 1024;

struct Packet {
    int chunkNumber;
    char data[CHUNK_SIZE];
};

int createSocketAndConnect(int port, const char* serverIP) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        exit(1);
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, serverIP, &serv_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        exit(1);
    }

    return sockfd;
}

void sendChunk(const char* filename, int startChunk, int socket, int totalChunks) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        exit(1);
    }

    int chunkNumber = startChunk;
    while (chunkNumber < totalChunks) {
        Packet packet;
        packet.chunkNumber = chunkNumber;

        file.seekg(chunkNumber * CHUNK_SIZE, std::ios::beg);
        file.read(packet.data, CHUNK_SIZE);
        std::streamsize bytes = file.gcount();

        if (bytes > 0) {
            char tmpBuffer[sizeof(packet)];
            
            memcpy(tmpBuffer, &packet, sizeof(packet));
            
            send(socket, tmpBuffer, sizeof(tmpBuffer), 0);
        }

        chunkNumber += 2; // Increment by 2 to cover alternating chunks
    }

    file.close();
}
int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <filename> <server IP> <port>" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    const char* serverIP = argv[2];
    int port = std::stoi(argv[3]);

    // Calculate the total number of chunks
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error opening file for size calculation" << std::endl;
        return 1;
    }
    std::streamoff fileSize = file.tellg();
    int totalChunks = static_cast<int>((fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE);
    file.close();

    // Create two sockets for two threads
    int socket1 = createSocketAndConnect(port, serverIP);
    int socket2 = createSocketAndConnect(port + 1, serverIP);

    // Start two threads for sending chunks
    std::thread thread1(sendChunk, filename, 0, socket1, totalChunks); // Thread 1 starts with chunk 0
    std::thread thread2(sendChunk, filename, 1, socket2, totalChunks); // Thread 2 starts with chunk 1

    // Wait for both threads to finish
    thread1.join();
    thread2.join();

    // Close the sockets
    close(socket1);
    close(socket2);

    return 0;
}
