#include "tools.h"

#ifndef FLASH_H_
#define FLASH_H_

#define MISSING_FILE -1
#define ERASE_ERROR  -2
#define SEEK_ERROR   -3

s32 eraseBlocks(int firstBlock, int lastBlock);
s32 eraseBlock(int block);

s32 dumpBlocks(const char *filename, int firstBlock, int lastBlock);
s32 flashBlocks(const char *filename, int firstBlock, int lastBlock);

s32 dumpBlock(const char *filename, int block);
s32 flashBlock(const char *filename, int block);

#endif
