#ifndef __file_IO_h__
#define __file_IO_h__

#include <stdint.h>

enum partition_t{
    ALTERNATE,
    CONSECUTIVE,
    TWOENDS
};

extern volatile bool end;
// Return total number of chunks that the file has
int initFileRead(const char* path, int bytesPerChunk, partition_t partition);
// get next chunk in the buf, get chunk index in *chunkNumPtr
int readChunk(char* buf, int* chunkNumPtr, int thread);
bool hasMoreChunks(int thread);

void initFileWrite(char* writeFile, int bytesPerChunk, partition_t partition);
bool storeData(char* content, int chunkNumber, int bytesReceived);
void writeToFile();

void closeFile();




#endif

