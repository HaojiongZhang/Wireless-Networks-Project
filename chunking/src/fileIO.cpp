#include <fileIO.h>
#include <stdio.h>
#include <sys/stat.h>

bool isReadingBothEnds;
int fd;
int chunkSize;
int totalChunks;
int chunkIdx_t0, chunkIdx_t1;


int readSequentialChunk(int8_t* buf);

// Init fileIO and return total number of chunks in a file
int initFileRead(char* path, size_t bytesPerChunk, bool twoEnds){
    fd = open(path, O_RDONLY);
    if (fd == -1){
        printf("File does not exist");
        return -1;
    }

    struct stat st;
    stat(path, &st);
    int fileSize = st.st_size;
    chunkSize = bytesPerChunk;
    totalChunks = fileSize / bytesPerChunk;
    totalChunks = (totalChunks*bytesPerChunk == fileSize) ? totalChunks : totalChunks+1;
    isReadingBothEnds = twoEnds;
    chunkIdx_t0 = 0;
    chunkIdx_t1 = isReadingBothEnds ? totalChunks - 1 : 1;

    return totalChunks;
}

// Read chunk into buf and return the index of the chunk
// Return 0 when error occurs
// Return negative index when this is the last chunk to read
int readChunk_t0(int8_t* buf){
    if (isReadingBothEnds){
        return readSequentialChunk();
    }
    else {
        
    }
}

int readChunk_t1(int8_t* buf){
    if (isReadingBothEnds){
        return readSequentialChunk();
    }
    else {

    }
}

int readSequentialChunk(int8_t* buf){
    static int currentChunk = 0;
    memset(buf, 0, chunkSize);

    int bytesRead = read(fd, buf, chunkSize);
    if (bytesRead < 0){
        printf("Chunk read error");
        return 0;
    }
    else if (bytesRead < chunkSize){
        return 0 - currentChunk;
    }
    currentChunk++;
    return currentChunk;
}