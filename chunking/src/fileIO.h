#ifndef __file_IO_h__
#define __file_IO_h__

#include <stdint.h>

enum partition_t{
    HALF_HALF,
    CONSECUTIVE,
    TWOENDS
};


// Return total number of chunks that the file has
int initFileRead(const char* path, int bytesPerChunk, partition_t partition);
// get next chunk in the buf, get chunk index in *chunkNumPtr
int readChunk(char* buf, int* chunkNumPtr, int thread);
bool hasMoreChunks(int thread);

void closeFile();




#endif

