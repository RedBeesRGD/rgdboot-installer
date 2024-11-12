/* RGD SDBoot Installer */

#ifndef __FLASH_H__
#define __FLASH_H__

#include "tools.h"

/* [nitr8]: Moved to "errorcodes.h" */
/* #define MISSING_FILE -1
#define ERASE_ERROR  -2
#define SEEK_ERROR   -3
#define BAD_BLOCK    -13
#define BAD_BOOT_BLOCKS  -5 */

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
#define NAND_ECC_DATA_SIZE	0x40

/* [nitr8]: Moved here */
#define PAGE_SIZE_NO_ECC	0x800
#define NAND_PAGE_SIZE		(PAGE_SIZE_NO_ECC + NAND_ECC_DATA_SIZE)
#define NAND_BLOCK_SIZE		(NAND_PAGE_SIZE * 64)

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
#define ECC_BUFFER_ALLOC	(NAND_ECC_DATA_SIZE + 32)

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
#define NAND_ECC_OK		0
#define NAND_ECC_CORRECTED	1
#define NAND_ECC_UNCORRECTABLE	-1

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
#define BLOCKMAP_SIGNATURE	0x26f29a401ee684cfULL

/* [nitr8]: With SDBoot being "installed" in NAND block 1, mark blocks 1 and 2 as "INVALID"
	    by skipping them as that would end up in running into an endless loop due to MINI
	    (as of BootMii) recognizing those as VALID blocks and MINI then trying to use them
	    as a boot2 copy for further booting. Block 2 is deactivated here as well due to boot2
	    (because of it's size) ALWAYS using "paired" blocks (like 1 & 2, 3 & 4, 7 & 6...). */
/* #define BOOT2_START		1 */
#define BOOT2_START		3

#define BOOT2_END		7

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
#define BLOCK_SIZE		64

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

/* [nitr8]: Added */
#ifdef __cplusplus
extern "C" {
#endif

s32 NANDFlashInit(void);

/* [nitr8]: Unused */
/* void NANDFlashClose(void); */

s32 flashFile(const char* fileName, int firstBlock, int lastBlock, struct Simulation* sim);
struct Simulation flashFileSim(const char* fileName, int firstBlock, int lastBlock);

/* [nitr8]: Make static */
/* s32 dumpPages(const char* fileName, int firstPage, int lastPage); */

s32 dumpBlocks(const char* fileName, int firstBlock, int lastBlock);
s32 eraseBlocks(int firstBlock, int lastBlock);
u32 checkBlocks(int firstBlock, int lastBlock);

void setMinBlock(u32 blockno);

char identifyBoot1(void);
Boot2Block identifyBoot2(u8 copy);

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
u32 TestNANDBlockmaps(void);

/* [nitr8]: Added */
extern s32 flash_fd;

/* [nitr8]: Added */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FLASH_H__ */

