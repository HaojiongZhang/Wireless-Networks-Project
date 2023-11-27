#ifndef __file_IO_h__
#define __file_IO_h__

#include <stdint.h>

enum chunkType{
    HALF_HALF,
    CONSECUTIVE,
    TWOENDS
};

// Return total number of chunks that the file has
int initFileRead(const char* path, int bytesPerChunk, chunkType chunk, int totalThreads);
// get next chunk in the buf, get chunk index in *chunkNumPtr
int readNextChunk(char* buf, int* chunkNumPtr, int thread);

void closeFile();




#endif

