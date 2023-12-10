#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include "fileIO.h"

#define KB 1024

const int CHUNK_SIZE = 1024;

struct Packet {
    int chunkNumber;
    char data[CHUNK_SIZE];
};

int createSocketAndConnect(int port, const char* serverIP, const char* interfaceName) {
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
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, serverIP, &serv_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        exit(1);
    }

    return sockfd;
}

void sendChunk(int socket, int threadNum) {

    while (hasMoreChunks(threadNum)) {

        Packet packet;

        int bytes = readChunk(packet.data, &packet.chunkNumber, threadNum);
      

        if (bytes > 0) {
            char tmpBuffer[sizeof(packet)];
            
            memcpy(tmpBuffer, &packet, sizeof(packet));
            
            send(socket, tmpBuffer, sizeof(tmpBuffer), 0);
        }
    }
}
int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <filename> <server IP> <esp32 IP> <port>" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    const char* serverIP = argv[2];
    const char* espIP = argv[3];
    int port = std::stoi(argv[4]);


    initFileRead(filename, 1024, CONSECUTIVE);

    // Set partition type to initialize file chunking
    //int totalChunks = initFileRead(filename, CHUNK_SIZE, CONSECUTIVE);

    const char* interfaceName1 = "wlan0";
    const char* interfaceName2 = "wlan0";

    // Create two sockets for two threads
    int socket1 = createSocketAndConnect(port, serverIP, interfaceName1);
    int socket2 = createSocketAndConnect(port + 1, espIP, interfaceName2);

    // Start two threads for sending chunks
    std::thread thread1(sendChunk, socket1, 0); // Thread 1 starts with chunk 0
    std::thread thread2(sendChunk, socket2, 1); // Thread 2 starts with chunk 1

    // Wait for both threads to finish
    thread1.join();
    thread2.join();

    // Close the sockets
    close(socket1);
    close(socket2);

    closeFile();
    return 0;
}
