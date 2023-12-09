#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>

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

void sendChunk(const char* filename, int startChunk, int socket) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        exit(1);
    }

    file.seekg(startChunk * CHUNK_SIZE, std::ios::beg);

    int chunkNumber = startChunk;
    while (!file.eof()) {
        Packet packet;
        packet.chunkNumber = chunkNumber;
        file.read(packet.data, CHUNK_SIZE);
        std::streamsize bytes = file.gcount();

        if (bytes > 0) {
            send(socket, &packet, sizeof(packet), 0);
            chunkNumber += 2; // Increment by 2 since we have 2 threads
        }
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

    int socket1 = createSocketAndConnect(port, serverIP);
    int socket2 = createSocketAndConnect(port + 1, serverIP);

    std::thread thread1(sendChunk, filename, 0, socket1); // Start with chunk 0
    std::thread thread2(sendChunk, filename, 1, socket2); // Start with chunk 1

    thread1.join();
    thread2.join();

    close(socket1);
    close(socket2);

    return 0;
}
