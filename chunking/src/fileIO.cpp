#include "fileIO.h"
#include <stdio.h>
#include <sys/stat.h>
#include <cstring>

bool isReadingBothEnds;
FILE* fp;
uint32_t chunkSize;
uint32_t totalChunks;
uint32_t chunkIdx_t0, chunkIdx_t1;
chunkType chunktype;

static int currentChunk = 0;

volatile static bool fileMutex = false;

int readSequentialChunk(char* buf, int* chunkNumPtr);


// Init fileIO and return total number of chunks of the file
int initFileRead(const char* path, int bytesPerChunk, chunkType chunk, int totalThreads){
    fp = fopen(path, "r");
    if (fp == NULL){
        printf("File does not exist");
        return -1;
    }

    struct stat st;
    stat(path, &st);
    int fileSize = st.st_size;

    chunkSize = bytesPerChunk;
    totalChunks = (fileSize + chunkSize - 1)/ bytesPerChunk;
    printf("file size:%d, totalChunks:%d \n",fileSize, totalChunks);

    chunktype = chunk;
    (void)totalThreads;

    return totalChunks;
}

// get next chunk in the buf, get chunk index in *chunkNumPtr
// returns the number of bytes read
int readNextChunk(char* buf, int* chunkNumPtr, int thread){
    switch (chunktype){
    case HALF_HALF:
        break;
    case CONSECUTIVE:
        return readSequentialChunk(buf, chunkNumPtr);
    case TWOENDS:
        break;
    }

    (void)thread;
    return 0;
}

// Read the next sequential chunk from the file
int readSequentialChunk(char* buf, int* chunkNumPtr){
    memset(buf, 0, chunkSize);

    if (currentChunk == totalChunks){
        *chunkNumPtr = currentChunk;
        return 0;
    }

    int bytesRead = fread(buf, 1, chunkSize, fp);

    if (bytesRead <= 0){
        printf("Chunk read error \n");
        return -1;
    }
    *chunkNumPtr = currentChunk;
    currentChunk++;
    return bytesRead;
}

void closeFile(){
    fclose(fp);
}