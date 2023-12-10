#include "fileIO.h"
#include <stdio.h>
#include <sys/stat.h>
#include <cstring>

#define NUMTHREADS 2


typedef struct{
    int chunkIdx[NUMTHREADS];
    int partitionStartChunk[NUMTHREADS];
    FILE fp_rx_thread[NUMTHREADS];
}threadSpecficCtrl_t;

typedef struct{
    int startChunkNum;
    int nextEmpty;
    int consec;
    char* buf;
    bool* occupied;
    bool mutex;
}recv_buf_t;

FILE* fp;
int chunkSize;
int totalChunks;
partition_t partitionMethod;
const int recv_buf_size = 64;

static int currentChunk = 0;
threadSpecficCtrl_t threadCtrl; 
recv_buf_t recv_buf;


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

    partitionMethod = partitiontype;
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
    switch (partitionMethod){
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
    switch (partitionMethod){
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


void initFileWrite(char* writeFile, int bytesPerChunk, partition_t partition){
    int t;
    for (t = 0; t < NUMTHREADS; t++){
        char filename[5];
        sprintf(filename, "%d", t);
        threadCtrl.fp_rx_thread[t] = fopen(filename, "w");
    }
    partitionMethod = partition;
    chunkSize = bytesPerChunk;

    recv_buf.storeBuf = malloc(bytesPerChunk * recv_buf_size);
    recv_buf.occupied = malloc(sizeof(bool) * recv_buf_size);
    recv_buf.startChunkNum = 0;
    recv_buf.consec = 0;
    recv_buf.nextEmpty = 0;
}

void storeData(char* content, int chunkNumber){
    switch (partitionMethod){
        case ALTERNATE:
        case CONSECUTIVE:
            InOrderStore(content, chunkNumber);
            break;
        default:
            break;
    }
}

bool InOrderStore(char* content, int chunkNumber){
    int idx = chunkNumber - recv_buf.startChunkNum;
    if (chunkNumber - recv_buf.startChunkNum < recv_buf_size){
        while (recv_buf.mutex){
            recv_buf.mutex = true;
            memcpy(&recv_buf.buf[idx], content, chunkSize);
            recv_buf.occupied[idx] = true;
            recv_buf.mutex = false;
        }
        return true;
    }
    else{
        return false;
    }
}

void writeToFile(){
    while(recv_buf.mutex){
        recv_buf.mutex = true;
        int idx = 0;
        while (idx < recv_buf_size){
            if (recv_buf.occupied[idx]){
                idx++;
                recv_buf.consec = idx;
            }
        }
        memmove(recv_buf.buf, recv_buf.buf[recv_buf.consec], recv_buf_size - recv_buf.consec);
        recv_buf.consec = 0;
        recv_buf.mutex = false;
    }
}

void closeFile(){
    fclose(fp);
}