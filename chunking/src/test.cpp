#include "fileIO.h"

#include <stdio.h>
#include <iostream>
#include <thread>

#define CHUNKSIZE 32

void testRead(int totalChunks, int thread){
    char read[CHUNKSIZE];
    int chunk = 0;
    while (chunk < totalChunks - 1){
        readNextChunk(read, &chunk, thread);
        printf("char: %s, chunk:%d, thread:%d \n", read, chunk, thread);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <filename> \n", argv[0]);
        return 1;
    }

    int totalChunks = initFileRead(argv[1], CHUNKSIZE, CONSECUTIVE, 2);

    std::thread t0(testRead, totalChunks, 0);
    std::thread t1(testRead, totalChunks, 1);

    t0.join();
    t1.join();

    closeFile();

    return 1;
}