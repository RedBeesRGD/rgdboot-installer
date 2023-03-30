#include "tools.h"

#ifndef FLASH_H_
#define FLASH_H_

#define MISSING_FILE -1
#define ERASE_ERROR  -2
#define SEEK_ERROR   -3
#define BAD_BLOCK    -13

struct Simulation{
	u8* blocksStatus;
	int toBeWritten;
};

s32 NANDFlashInit();
void NANDFlashClose();

s32 flashFile(const char* fileName, int firstBlock, int lastBlock, struct Simulation* sim);
struct Simulation flashFileSim(const char* fileName, int firstBlock, int lastBlock);
s32 dumpPages(const char* fileName, int firstPage, int lastPage);
s32 dumpBlocks(const char* fileName, int firstBlock, int lastBlock);
s32 eraseBlocks(int firstBlock, int lastBlock);

#endif
