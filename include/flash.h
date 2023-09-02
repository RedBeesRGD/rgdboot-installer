#include "tools.h"

#ifndef FLASH_H_
#define FLASH_H_

#define MISSING_FILE -1
#define ERASE_ERROR  -2
#define SEEK_ERROR   -3
#define BAD_BLOCK    -13
#define BAD_BOOT_BLOCKS  -5

struct Simulation{
	u8* blocksStatus;
	int toBeWritten;
};

typedef struct{
	char version[10];
	bool isBootMii;
	char bootMiiVer[4];
	u8 blockSize;
} Boot2Block;

s32 NANDFlashInit();
void NANDFlashClose();

s32 flashFile(const char* fileName, int firstBlock, int lastBlock, struct Simulation* sim);
struct Simulation flashFileSim(const char* fileName, int firstBlock, int lastBlock);
s32 dumpPages(const char* fileName, int firstPage, int lastPage);
s32 dumpBlocks(const char* fileName, int firstBlock, int lastBlock);
s32 eraseBlocks(int firstBlock, int lastBlock);
u32 checkBlocks(int firstBlock, int lastBlock);

void setMinBlock(u32 blockno);

#endif
