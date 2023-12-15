#include "fileIO.h"

#include <stdio.h>
#include <iostream>
#include <thread>
#include <unistd.h>

#define CHUNKSIZE 1024

void testRead(int totalChunks, int thread){
    char buf[CHUNKSIZE];
    int chunk = 0, bytesRead = 0;
    while (hasMoreChunks(thread)){
        bytesRead = readChunk(buf, &chunk, thread);
        printf("chunk:%d, thread:%d, size: %d \n", chunk, thread, bytesRead);
        if (bytesRead > 0){
            storeData(buf, chunk, bytesRead);
        }
    }
    (void) totalChunks;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <filename> \n", argv[0]);
        return 1;
    }

    int totalChunks = initFileRead(argv[1], CHUNKSIZE, CONSECUTIVE);
    initFileWrite("write", CHUNKSIZE, CONSECUTIVE);



    std::thread t0(testRead, totalChunks, 0);
    std::thread t1(testRead, totalChunks, 1);
    std::thread t2(writeToFile);

    t0.join();
    t1.join();
    end = true;

    
    t2.join();


    closeFile();

    return 1;
}