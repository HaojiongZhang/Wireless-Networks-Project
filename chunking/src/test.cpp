#include "fileIO.h"

#include <stdio.h>
#include <iostream>
#include <thread>

#define CHUNKSIZE 1024

void testRead(int thread){
    char read[CHUNKSIZE];
    int chunk = 0;
    while (hasMoreChunks(thread)){
        readChunk(read, &chunk, thread);
        printf("char: %s, chunk:%d, thread:%d \n", read, chunk, thread);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <filename> \n", argv[0]);
        return 1;
    }

    initFileRead(argv[1], CHUNKSIZE, HALF_HALF);

    std::thread t0(testRead, 0);
    std::thread t1(testRead, 1);

    t0.join();
    t1.join();

    closeFile();

    return 1;
}