#include "fileIO.h"
#include <stdio.h>
#include <sys/stat.h>
#include <cstring>

FILE* fp;
int chunkSize;
int totalChunks;
int chunkIdx_t0, chunkIdx_t1;
int partitionStartChunk;

partition_t partitionType;

static int currentChunk = 0;

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
    case HALF_HALF:
        chunkIdx_t0 = 0; 
        chunkIdx_t1 = totalChunks/2;
        partitionStartChunk = chunkIdx_t1;
        break;
    
    default:
        break;
    }

    return totalChunks;
}

// get next chunk in the buf, get chunk index in *chunkNumPtr
// returns the number of bytes read
int readChunk(char* buf, int* chunkNumPtr, int partition){
    switch (partitionType){
    case HALF_HALF:
        return readAltChunk(buf, chunkNumPtr, partition);
    case CONSECUTIVE:
        (void) partition;
        return readSequentialChunk(buf, chunkNumPtr);
    case TWOENDS:
        break;
    }

    (void)partition;
    return 0;
}

bool hasMoreChunks(int thread){
    switch (partitionType){
    case CONSECUTIVE:
        return currentChunk != totalChunks;
    case HALF_HALF:
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


int readAltChunk(char* buf, int* chunkNumPtr, int partition){
    memset(buf, 0, chunkSize);
    int *chunk, finalChunk;
    if (partition == 0){
        chunk = &chunkIdx_t0;
        finalChunk = partitionStartChunk;
    }
    else {
        chunk = &chunkIdx_t1;
        finalChunk = totalChunks;
    }


    if (*chunk == finalChunk){
        *chunkNumPtr = *chunk;
        return 0;
    }

    int bytesRead = fread(buf, 1, chunkSize, fp);

    if (bytesRead <= 0){
        printf("Chunk read error \n");
        return -1;
    }
    *chunkNumPtr = *chunk;
    (*chunk)++;
    return bytesRead;
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