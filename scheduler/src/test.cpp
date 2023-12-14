#include "fileIO.h"

#include <stdio.h>
#include <iostream>
#include <thread>
#include <unistd.h>

#define CHUNKSIZE 1400*100

struct chunk_t{
    char chunk_buf[CHUNKSIZE];
	unsigned int chunkBytes;
	int chunkIdx;
};

void testRead(int totalChunks, int thread){
    chunk_t *chunk_buffer = (chunk_t*)malloc(sizeof(chunk_t));
    while (hasMoreChunks(thread)){
        chunk_buffer->chunkBytes = readChunk(chunk_buffer->chunk_buf, &chunk_buffer->chunkIdx, thread);
        printf("chunk:%d, thread:%d, size: %d \n", chunk_buffer->chunkIdx, thread, chunk_buffer->chunkBytes);
        if (chunk_buffer->chunkBytes > 0){
            storeData(chunk_buffer->chunk_buf, chunk_buffer->chunkIdx, chunk_buffer->chunkBytes, thread);
        }
    }
    (void) totalChunks;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <filename> \n", argv[0]);
        return 1;
    }

    int totalChunks = initFileRead(argv[1], 1400*100, TWOENDS);
    initFileWrite("write", 1400*100, TWOENDS);



    std::thread t0(testRead, totalChunks, 0);
    std::thread t1(testRead, totalChunks, 1);
    std::thread t2(writeToFile);

    t0.join();
    t1.join();
    endTx = true;

    
    t2.join();


    closeFile();
    closeWriteFile();

    return 1;
}