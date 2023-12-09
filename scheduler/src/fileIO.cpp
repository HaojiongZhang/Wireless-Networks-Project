#include "fileIO.h"
#include <stdio.h>
#include <sys/stat.h>
#include <cstring>

#define NUMTHREADS 2


typedef struct{
    int chunkIdx[NUMTHREADS];
    int partitionStartChunk[NUMTHREADS];
}threadSpecficCtrl_t;

FILE* fp;
int chunkSize;
int totalChunks;
partition_t partitionType;

static int currentChunk = 0;
threadSpecficCtrl_t threadCtrl;



int readAltChunk(char* buf, int* chunkNumPtr, int partition);
int readSequentialChunk(char* buf, int* chunkNumPtr);


// Init fileIO and return total number of chunks of the file
int initFileRead(const char* path, int bytesPerChunk, partition_t partitiontype){
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

    partitionType = partitiontype;
    switch (partitiontype){
    case ALTERNATE:
    case CONSECUTIVE:
        chunkIdx[0] = 0;
        chunkIdx[1] = 1;
        break;
    case TWOENDS:
        chunkIdx[0] = 0;
        chunkIdx[1] = totalChunks-1;
    default:
        break;
    }

    return totalChunks;
}

// get next chunk in the buf, get chunk index in *chunkNumPtr
// returns the number of bytes read
int readChunk(char* buf, int* chunkNumPtr, int partition){
    switch (partitionType){
    case ALTERNATE:
        return readAltChunk(buf, chunkNumPtr, partition);
    case CONSECUTIVE:
        (void) partition;
        return readSequentialChunk(buf, chunkNumPtr);
    case TWOENDS:
        return readEndsChunk(buf, chunkNumPtr);
    }

    (void)partition;
    return 0;
}

int readAltChunk(char* buf, int* chunkNumBuf, int partition){
    int *chunk, finalChunk;
    chunk = threadCtrl.chunkIdx + partition;
    finalChunk = totalChunks - NUMTHREADS + partition;

    /* Check if already completed reading */
    if (*chunk == finalChunk){
        *chunkNumBuf = *chunk;
        return 0;
    }

    memset(buf, 0, chunkSize);
    fseek(fp, (*chunk)*chunkSize, SEEK_SET);
    int bytesRead = fread(buf, 1, chunkSize, fp);

    if (bytesRead <= 0){
        printf("Chunk read error \n");
        return -1;
    }
    *chunkNumBuf = *chunk;
    *chunk += 2;
    return bytesRead;
}

// Read the next sequential chunk from the file
int readSequentialChunk(char* buf, int* chunkNumPtr){
    /* Check if already completed reading */
    if (currentChunk == totalChunks){
        *chunkNumPtr = currentChunk;
        return 0;
    }

    memset(buf, 0, chunkSize);
    int bytesRead = fread(buf, 1, chunkSize, fp);

    if (bytesRead <= 0){
        printf("Chunk read error \n");
        return -1;
    }
    *chunkNumPtr = currentChunk;
    currentChunk++;
    return bytesRead;
}

int readEndsChunk(char* buf, int* chunkNumPtr){
    /* Check if already completed reading */
    if (threadCtrl.chunkIdx[0] == threadCtrl.chunkIdx[1]){
        *chunkNumPtr = threadCtrl.chunkIdx[0];
        return 0;
    }

    int *chunk, finalChunk;
    chunk = threadCtrl.chunkIdx + partition;

    memset(buf, 0, chunkSize);
    fseek(fp, (*chunk) * chunkSize, SEEK_SET);
    int bytesRead = fread(buf, 1, chunkSize, fp);

    if (bytesRead <= 0){
        printf("Chunk read error \n");
        return -1;
    }
    *chunkNumPtr = currentChunk;
    (*chunk)--;
    return bytesRead;
}

bool hasMoreChunks(int thread){
    switch (partitionType){
    case CONSECUTIVE:
        return currentChunk != totalChunks;
    case ALTERNATE:
        if (thread == 0){
            return chunkIdx_t0 != partitionStartChunk;
        }
        else {
            return chunkIdx_t1 != totalChunks;
        }
    case TWOENDS:
        return false;
    }
    
    return false;
}


void closeFile(){
    fclose(fp);
}