#ifndef __file_IO-h__
#define __file_IO_h__

#include <stdint.h>

// Return total number of chunks that the file has
int initFileRead(char* path, size_t bytesPerChunk, bool twoEnds);
int readChunk_t0(int8_t* buf);
int readChunk_t1(int8_t* buf);


#endif

