#include "fileIO.h"
#include <stdio.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdlib>
#include <ostream>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define NUMTHREADS 2


typedef struct{
    int chunkIdx[NUMTHREADS];
    int partitionStartChunk[NUMTHREADS];
    FILE* fp_rx_thread[NUMTHREADS];
}threadSpecficCtrl_t;

typedef struct{
    int startChunkNum;
    int consec;
    int count;
    char* buf;
    bool* occupied;
    volatile bool mutex;
}recv_buf_t;

FILE* fp;
int chunkSize;
int totalChunks;
partition_t partitionMethod;
const int recv_buf_size = 64;
volatile bool start = false;
volatile bool end = false;

static int currentChunk = 0;
threadSpecficCtrl_t threadCtrl; 
recv_buf_t recv_buf;



int readAltChunk(char* buf, int* chunkNumPtr, int partition);
int readSequentialChunk(char* buf, int* chunkNumPtr);
int readEndsChunk(char* buf, int* chunkNumPtr, int partition);
bool InOrderStore(char* content, int chunkNumber);


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
        threadCtrl.chunkIdx[0] = 0;
        threadCtrl.chunkIdx[1] = 1;
        break;
    case TWOENDS:
        threadCtrl.chunkIdx[0] = 0;
        threadCtrl.chunkIdx[1] = totalChunks-1;
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
        return readEndsChunk(buf, chunkNumPtr, partition);
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
    pthread_mutex_lock(&mutex);
    fseek(fp, (*chunk)*chunkSize, SEEK_SET);
    int bytesRead = fread(buf, 1, chunkSize, fp);
    pthread_mutex_unlock(&mutex);

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
    pthread_mutex_lock(&mutex);
    /* Check if already completed reading */
    if (currentChunk == totalChunks){
        *chunkNumPtr = currentChunk;
        return 0;
    }

    memset(buf, 0, chunkSize);
    int bytesRead = fread(buf, 1, chunkSize, fp);
    if (bytesRead < 0){
        printf("chunk read error");
        return -1;
    }
    *chunkNumPtr = currentChunk;
    currentChunk++;
    pthread_mutex_unlock(&mutex);
    
    return bytesRead;
}

int readEndsChunk(char* buf, int* chunkNumPtr, int partition){
    /* Check if already completed reading */
    if (threadCtrl.chunkIdx[0] == threadCtrl.chunkIdx[1]){
        *chunkNumPtr = threadCtrl.chunkIdx[0];
        return 0;
    }

    int *chunk;
    chunk = threadCtrl.chunkIdx + partition;

    memset(buf, 0, chunkSize);
    pthread_mutex_lock(&mutex);
    fseek(fp, (*chunk) * chunkSize, SEEK_SET);
    int bytesRead = fread(buf, 1, chunkSize, fp);
    pthread_mutex_unlock(&mutex);

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
        return currentChunk < totalChunks;
    case ALTERNATE:
        return threadCtrl.chunkIdx[thread] == (totalChunks - NUMTHREADS + thread);
    case TWOENDS:
        return threadCtrl.chunkIdx[1] <= threadCtrl.chunkIdx[0];
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

    recv_buf.buf = (char*)malloc(bytesPerChunk * recv_buf_size);
    recv_buf.occupied = (bool*)malloc(sizeof(bool) * recv_buf_size);
    recv_buf.startChunkNum = 0;
    recv_buf.consec = 0;
    recv_buf.count = 0;
    recv_buf.mutex = false;
    fp = fopen(writeFile, "w");
}

bool storeData(char* content, int chunkNumber){
    switch (partitionMethod){
        case ALTERNATE:
        case CONSECUTIVE:
            return InOrderStore(content, chunkNumber);
            break;
        default:
            return false;
            break;
    }
}

bool InOrderStore(char* content, int chunkNumber){
    int idx = chunkNumber - recv_buf.startChunkNum;
    if (idx < recv_buf_size){
        while (recv_buf.mutex){        }
        recv_buf.mutex = true;
        memcpy(&recv_buf.buf[idx], content, chunkSize);
        start = true;
        printf("buffer head: %d. ", recv_buf.startChunkNum);
        // printf("storing chunk: %d, in idx: %d \n", chunkNumber, idx);
        recv_buf.occupied[idx] = true;
        recv_buf.count++;
        recv_buf.mutex = false;
        return true;
    }
    else{
        return false;
    }
}

void writeToFile(){
    while (true){
        
        
        int count = 0;
        while (count < recv_buf_size){
            if (recv_buf.occupied[count]){
                count++;
                recv_buf.consec = count;
                printf("%d", count);
            }
            else{
                break;
            }
        }
        if (count != 0){
            while(recv_buf.mutex){        }
            recv_buf.mutex = true;
            printf("writing to %d. ", count+recv_buf.startChunkNum);
            fwrite(recv_buf.buf, 1024, count, fp);
            memmove(recv_buf.buf, &recv_buf.buf[count], (recv_buf_size - count) * 1024);
            memmove(recv_buf.occupied, &recv_buf.occupied[count], sizeof(bool)*(recv_buf_size - count));
            memset(&recv_buf.occupied[recv_buf_size - count], 0, sizeof(bool)*count);
            recv_buf.count -= count;
            recv_buf.startChunkNum += count;
            recv_buf.mutex = false;
            
        }

        if (end && recv_buf.count == 0){
            return;
        }
    }
}

void closeFile(){
    fclose(fp);
}