/* [nitr8]: Moved up here */
#include "flash.h"
#include "tools.h"
#include "sha256.h"

/* [nitr8]: Added */
#include <ogc/machine/processor.h>

/* [nitr8]: Added */
#include "hollywood.h"

/* [nitr8]: Added */
#include "runtimeiospatch.h"

/* [nitr8]: Added */
#include "boot2.h"

/* [nitr8]: Added */
#include "errorcodes.h"

/* [nitr8]: Added */
#include "ecc.h"

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
typedef struct
{
	u64 signature;
	u32 generation;
	u8 blocks[BLOCK_SIZE];
} PACKED boot2blockmap;

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
static u8 sector_buf[PAGE_SIZE_NO_ECC] MEM2_BSS ALIGNED(64);
static u8 ecc_buf[ECC_BUFFER_ALLOC] MEM2_BSS ALIGNED(128);
static boot2blockmap good_blockmap MEM2_BSS;
static u8 boot2_blocks[BOOT2_END - BOOT2_START + 1];
static u32 valid_blocks;

/* [nitr8]: Make static */
/* u32 nand_min_block = 0; */
static u32 nand_min_block = 0;

/* [nitr8]: Rename from "fd" to "flash_fd" */
s32 flash_fd = -1;

/* [nitr8]: Make static */
/* s32 __isPageEmpty(u8* data) */
static s32 __isPageEmpty(u8* data)
{
	u8 emptyPage[NAND_PAGE_SIZE];

	memset(emptyPage, 0xFF, NAND_PAGE_SIZE);
	
	return (!memcmp(data, emptyPage, NAND_PAGE_SIZE));
}

/* [nitr8]: Make static */
/* s32 __isBlockBad(void) */
static s32 __isBlockBad(void)
{
	s32 rv;

	rv = IOS_Ioctl(flash_fd, 4, NULL, 0, NULL, 0); /* Is this block bad? */

	return (rv == BAD_BLOCK);
}

/* [nitr8]: Make static and rework (add ECC buffer) */
/* s32 __readPage(u8* data, int pageno) */
static s32 __readPage(u8* data, int pageno, void *ecc)
{
	static unsigned char buffer[NAND_PAGE_SIZE] ALIGNED(32);
	int rv;
	
	rv = IOS_Seek(flash_fd, pageno, 0);

	if (rv < 0)
		return SEEK_ERROR;

	rv = IOS_Read(flash_fd, buffer, (u32) NAND_PAGE_SIZE);

	/* [nitr8]: return value of IOS_Read is never checked... */
	gecko_printf("IOS_Read returned %d\n", rv);

	/* [nitr8]: Rework: add separated ECC buffer */
	if (ecc)
	{
		memcpy(data, buffer, PAGE_SIZE_NO_ECC);
		memcpy(ecc, buffer + PAGE_SIZE_NO_ECC, NAND_ECC_DATA_SIZE);
	}
	else
		memcpy(data, buffer, NAND_PAGE_SIZE);


	return 0;
}

/* [nitr8]: Make static */
/* s32 __flashPage(u8* data, int pageno) */
static s32 __flashPage(u8* data, int pageno)
{
	static unsigned char buffer[NAND_PAGE_SIZE] ALIGNED(32);

	/* [nitr8]: get rid of warning about signed and unsigned comparison
	if(pageno/64 < nand_min_block) */
	if((u32)pageno/BLOCK_SIZE < nand_min_block)
		return 0;
	
	/* [nitr8]: warning: unused variable
	int rv; */
	
	if(__isPageEmpty(data))
		return 0;

	memcpy(buffer, data, NAND_PAGE_SIZE);
	
	IOS_Seek(flash_fd, pageno, 0);
	IOS_Write(flash_fd, buffer, (u32) NAND_PAGE_SIZE);
	
	return 0;
}

/* [nitr8]: Make static */
/* s32 __eraseBlock(int blockno) */
static s32 __eraseBlock(int blockno)
{
	int rv;

	/* [nitr8]: get rid of warning about signed and unsigned comparison
	if(blockno < nand_min_block) */
	if((u32)blockno < nand_min_block)
		return 0;
		
	rv = IOS_Seek(flash_fd, blockno * BLOCK_SIZE, 0);

	if (rv < 0)
		return SEEK_ERROR;

	if(__isBlockBad())
		return BAD_BLOCK;
	
	rv = IOS_Ioctl(flash_fd, 3, NULL, 0, NULL, 0); /* Erase block */

	if(rv < 0)
		return ERASE_ERROR;
	
	return 0;
}

/* [nitr8]: Make static */
/* s32 __flashBlock(u8* data, int blockno) */
static s32 __flashBlock(u8* data, int blockno)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int pg;
	int rv;
	u8* nandPage;

	/* [nitr8]: get rid of warning about signed and unsigned comparison
	if(blockno < nand_min_block) */
	if((u32)blockno < nand_min_block)
		return 0;

	/* [nitr8]: with allocating memory right here and then returning
		    due to the call to "__eraseBlock()" failing,
		    we get a MEMORY LEAK because we never reach the call
		    to "free()" */
	/* nandPage = (u8*)malloc(NAND_PAGE_SIZE); */
	
	rv = __eraseBlock(blockno);

	if(rv < 0)
		return rv;

	/* [nitr8]: moved down here */
	nandPage = (u8*)malloc(NAND_PAGE_SIZE);

	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int pg = 0; pg < 64; pg++){ */
	for(pg = 0; pg < BLOCK_SIZE; pg++)
	{
		memcpy(nandPage, data+pg*NAND_PAGE_SIZE, NAND_PAGE_SIZE);
		__flashPage(nandPage, pg+blockno*BLOCK_SIZE);
	}
	
	free(nandPage);
	
	return 0;
}

/* [nitr8]: Make static */
/* struct Simulation __simulateWrite(const char* fileName, int firstBlock, int lastBlock) */
static struct Simulation __simulateWrite(const char* fileName, int firstBlock, int lastBlock)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int block;
	FILE *fin;

	/* [nitr8]: warning: variable set but not used
	int rv; */

	struct Simulation sim;

	/* [nitr8]: with allocating memory right here and then returning
		    due to the call to "fopen()" failing,
		    we get a MEMORY LEAK because we never reach the call
		    to "free()" */
	/* u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE);
	u8* filePage = (u8*)malloc(NAND_PAGE_SIZE); */
	u8* nandPage;
	u8* filePage;

	sim.blocksStatus = (u8*)malloc(lastBlock - firstBlock + 1);
	sim.toBeWritten  = 0;
	
	/* 0: No need to write
	 * 1: Needs to be erased and written
	 * 2: Bad block
	 */
	
	fin = fopen(fileName, "rb");

	if(fin == NULL)
	{
		printf("Cannot open file!\n");
		free(sim.blocksStatus);
		sim.blocksStatus = NULL;
		return sim;
	}
	
	/* [nitr8]: moved down here */
	nandPage = (u8*)malloc(NAND_PAGE_SIZE);
	filePage = (u8*)malloc(NAND_PAGE_SIZE);

	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int block = firstBlock; block <= lastBlock; block++){ */
	for(block = firstBlock; block <= lastBlock; block++)
	{
		/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
		int pg;

		sim.blocksStatus[block - firstBlock] = 0;
		
		/* [nitr8]: get rid of warning about signed and unsigned comparison
		if(block < nand_min_block) */
		if((u32)block < nand_min_block)
			continue;
		
		printf(" [+] Analyzing block %d... ", block);
		
		IOS_Seek(flash_fd, block * BLOCK_SIZE, 0);
		
		if(__isBlockBad())
		{	/* Is this block bad? */
			printf("bad\n");

			sim.blocksStatus[block - firstBlock] = 2;

			continue;
		}
		
		/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
		for(int pg = block*64; pg < (block+1)*64; pg++){ */
		for(pg = block*BLOCK_SIZE; pg < (block+1)*BLOCK_SIZE; pg++)
		{
			/* [nitr8]: warning: variable set but not used
			rv = __readPage(nandPage, pg); */
			__readPage(nandPage, pg, NULL);

			fseek(fin, pg*NAND_PAGE_SIZE, SEEK_SET);
			fread(filePage, NAND_PAGE_SIZE, 1, fin);
			
			if(memcmp(nandPage, filePage, NAND_PAGE_SIZE))
			{
				sim.blocksStatus[block - firstBlock] = 1;
				
				if(!__isPageEmpty(filePage))
					sim.toBeWritten++;
			}
		}
		
		if(sim.blocksStatus[block - firstBlock] == 1)
			printf("modified\n");
		else
			printf("intact\n");
	}
	
	free(nandPage);
	free(filePage);

	/* [nitr8]: sim.blocksStatus was never freed here previously... (MEMORY LEAK) */
	free(sim.blocksStatus);

	fclose(fin);
	
	return sim;
}

/* [nitr8]: Moved up here */
/* [nitr8]: Make static */
/* s32 dumpPages(const char* fileName, int firstPage, int lastPage) */
static s32 dumpPages(const char* fileName, int firstPage, int lastPage)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int page;

	/* [nitr8]: with allocating memory right here and then returning
		    due to the call to "fopen()" failing,
		    we get a MEMORY LEAK because we never reach the call
		    to "free()" */
	/* u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE); */
	u8* nandPage;

	FILE *fout = fopen(fileName, "wb");

	if(fout == NULL)
	{
		printf("Cannot open file!\n");
		return MISSING_FILE;
	}
	
	/* [nitr8]: moved down here */
	nandPage = (u8*)malloc(NAND_PAGE_SIZE);

	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int page = firstPage; page <= lastPage; page++){ */
	for(page = firstPage; page <= lastPage; page++)
	{
		__readPage(nandPage, page, NULL);
		fwrite(nandPage, NAND_PAGE_SIZE, 1, fout);
	}
	
	fclose(fout);
	free(nandPage);

	return 0;
}

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
/*	    Find two equal valid blockmaps from a set of three, return one of them */
static int find_valid_map(const boot2blockmap *maps)
{
	if (maps[0].signature == BLOCKMAP_SIGNATURE)
	{
		if (!memcmp(&maps[0], &maps[1], sizeof(boot2blockmap)))
			return 0;

		if (!memcmp(&maps[0], &maps[2], sizeof(boot2blockmap)))
			return 0;
	}

	if (maps[1].signature == BLOCKMAP_SIGNATURE)
	{
		if (!memcmp(&maps[1], &maps[2], sizeof(boot2blockmap)))
			return 1;
	}

	return -1;
}

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
static int nand_correct(u32 pageno, void *data, void *ecc)
{
	int i;
	u32 syndrome;
	u8 *dp = (u8 *)data;
	u32 *ecc_read = (u32 *)((u8 *)ecc + 0x30);
	u32 *ecc_calc = (u32 *)calc_page_ecc(dp);
	int uncorrectable = 0;
	int corrected = 0;

	for (i = 0; i < 4; i++)
	{
		syndrome = ecc_read[i] ^ ecc_calc[i];	/* Calculate ECC syndrome */

		/* Don't try to correct unformatted pages (all FF) */
		if ((ecc_read[i] != 0xFFFFFFFF) && syndrome)
		{
			if (!((syndrome - 1) & syndrome))
			{
				/* Single-bit error in ECC */
				corrected++;
			}
			else
			{
				/* Byte-swap and extract odd and even halves */
				u16 even = ((syndrome >> 24) | ((syndrome >> 8) & 0xf00));
				u16 odd = (((syndrome << 8) & 0xf00) | ((syndrome >> 8) & 0x0ff));

				if ((even ^ odd) != 0xfff)
				{
					/* Oops, can't fix this one */
					uncorrectable++;
				}
				else
				{
					/* Fix the bad bit */
					dp[odd >> 3] ^= (1 << (odd & 7));
					corrected++;
				}
			}
		}

		dp += DATA_CHUNK_LEN;
	}

	if (uncorrectable || corrected)
		gecko_printf("ECC stats for NAND page 0x%x: %d uncorrectable, %d corrected\n", pageno, uncorrectable, corrected);

	if (uncorrectable)
		return NAND_ECC_UNCORRECTABLE;

	if (corrected)
		return NAND_ECC_CORRECTED;

	return NAND_ECC_OK;
}

s32 flashFile(const char* fileName, int firstBlock, int lastBlock, struct Simulation* sim)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int block;

	/* [nitr8]: warning: unused variable
	int rv; */

	/* [nitr8]: with allocating memory right here and then returning
		    due to the call to "fopen()" failing,
		    we get a MEMORY LEAK because we never reach the call
		    to "free()" */
	/* u8* data = (u8*)malloc(NAND_BLOCK_SIZE); */
	u8* data;

	FILE *fin = fopen(fileName, "rb");

	if(fin == NULL)
		return MISSING_FILE;
	
	/* [nitr8]: moved down here */
	data = (u8*)malloc(NAND_BLOCK_SIZE);

	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int block = firstBlock; block <= lastBlock; block++){ */
	for(block = firstBlock; block <= lastBlock; block++)
	{
		/* [nitr8]: get rid of warning about signed and unsigned comparison
		if(block < nand_min_block) */
		if((u32)block < nand_min_block)
			continue;
		
		/* fseek(fin, block*NAND_BLOCK_SIZE, SEEK_SET); */
		fseek(fin, (block - firstBlock)*NAND_BLOCK_SIZE, SEEK_SET);
		
		if(sim != NULL)
		{
			if(sim->blocksStatus[block - firstBlock] == 1)
			{
				printf(" [+] Flashing block %d...\n", block);
				fread(data, NAND_BLOCK_SIZE, 1, fin);
				__flashBlock(data, block);
			}
		}
		else
		{
			printf(" [+] Flashing block %d...\n", block);
			fread(data, NAND_BLOCK_SIZE, 1, fin);
			__flashBlock(data, block);
		}
	}
	
	free(data);
	fclose(fin);
	
	return 0;
}

struct Simulation flashFileSim(const char* fileName, int firstBlock, int lastBlock)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int block;

	int blocksToModify = 0;
	struct Simulation sim = __simulateWrite(fileName, firstBlock, lastBlock);
	
	printf("Bad blocks:\n");

	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int block = firstBlock; block <= lastBlock; block++){ */
	for(block = firstBlock; block <= lastBlock; block++)
	{
		if(sim.blocksStatus[block-firstBlock] == 2)
			printf(" [-] block %d\n", block);
		else if(sim.blocksStatus[block-firstBlock] == 1)
			blocksToModify++;
	}
	
	printf("Will erase %d pages (%d blocks)...\n", blocksToModify*BLOCK_SIZE, blocksToModify);
	printf("Will write %d pages...\n", sim.toBeWritten);
	
	return sim;
}

s32 dumpBlocks(const char* fileName, int firstBlock, int lastBlock)
{
	return dumpPages(fileName, firstBlock*BLOCK_SIZE, (lastBlock+1)*BLOCK_SIZE-1);
}

s32 eraseBlocks(int firstBlock, int lastBlock)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int block;

	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int block = firstBlock; block <= lastBlock; block++){ */
	for(block = firstBlock; block <= lastBlock; block++)
	{
		printf(" [+] Erasing block %d...\n", block);
		__eraseBlock(block);
	}

	return 0;
}

u32 checkBlocks(int firstBlock, int lastBlock)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int block;

	u32 badBlocks = 0;

	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int block = firstBlock; block <= lastBlock; block++){ */
	for(block = firstBlock; block <= lastBlock; block++)
	{
		printf(" [+] Checking block %d... ", block);
		
		IOS_Seek(flash_fd, block * BLOCK_SIZE, 0);
		
		if(__isBlockBad())
		{
			badBlocks++;
			printf("BAD!!! :(\n");
		}
		else
			printf("good\n");
	}
	
	return badBlocks;
}

void setMinBlock(u32 blockno)
{
	nand_min_block = blockno;
}

char identifyBoot1(void)
{
	u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE);
	u8 tmpByte;
	
	__readPage(nandPage, 0, NULL);
	tmpByte = nandPage[0x40];
	free(nandPage);
	
	switch(tmpByte)
	{
		case 0x99:
			return 'a';
		case 0xC1:
			return 'b';
		case 0xFC:
			return 'c';
		case 0x48:
			return 'd';		
		default:
			return '?';
	}
	
	return '?';
}

Boot2Block identifyBoot2(u8 copy)
{
	const char boot2v1_sha1[0x14] = { 0xFD, 0x53, 0x7E, 0x4E, 0xD7, 0x9A, 0x3F, 0xB3, 0xE0, 0x35, 0xC5, 0x10, 0x1A, 0xCC, 0x48, 0xF1, 0x9E, 0x5D, 0xE1, 0x05 };
	const char boot2v2_sha1[0x14] = { 0xBD, 0x0F, 0x4F, 0xC7, 0xDF, 0xE0, 0xD8, 0xF1, 0x37, 0x54, 0x9E, 0xB3, 0x6F, 0xBF, 0xD5, 0x6B, 0x3D, 0xAE, 0x84, 0xEE };
	const char boot2v3_sha1[0x14] = { 0xEA, 0x5C, 0x22, 0x73, 0xC8, 0xAB, 0xAD, 0x2B, 0x13, 0x64, 0x7B, 0xB4, 0xCB, 0x2D, 0x20, 0xF0, 0xF9, 0x49, 0x29, 0x51 };
	const char boot2v4_sha1[0x14] = { 0xEB, 0xFC, 0x36, 0x20, 0x0D, 0xDB, 0x3E, 0x32, 0x8D, 0xA1, 0xC5, 0x2B, 0x23, 0x3D, 0xFC, 0x8C, 0xAF, 0x60, 0xB6, 0x71 };
	const char boot2v5_sha1[0x14] = { 0x4F, 0x99, 0xBA, 0xDF, 0x98, 0x8E, 0x3F, 0xD7, 0xA8, 0x21, 0x2C, 0xCA, 0x35, 0xD5, 0x3C, 0xBB, 0x9E, 0x7B, 0x7C, 0xBC };
	
	const char sdboot_sha1[0x14] = { 0xE5, 0x30, 0xDE, 0xAF, 0x73, 0x0D, 0x24, 0xE0, 0xA8, 0xE0, 0x8D, 0xCA, 0xC0, 0x94, 0x39, 0x01, 0x46, 0x36, 0x1A, 0x5E };
	const char nandboot_sha1[0x14] = { 0xD3, 0x84, 0x29, 0xDA, 0xBB, 0x18, 0xF2, 0x47, 0xCC, 0xF5, 0x24, 0x47, 0x78, 0xD4, 0x66, 0x6B, 0x2E, 0xFB, 0x3F, 0x29 };
	
	Boot2Block boot2Block;

	/* [nitr8]: with allocating memory right here and then returning
		    due to the call to "__isBlockBad()" failing,
		    we get a MEMORY LEAK because we never reach the call
		    to "free()" */
	/* u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE); */
	u8* nandPage;

	u8 blocksToTest[3] = {1,3,7};
	u8 block = blocksToTest[copy];
	
	IOS_Seek(flash_fd, block * BLOCK_SIZE, 0);
	
	if(__isBlockBad())
	{	/* Is this block bad? */
		boot2Block.blockSize = 0;
		
		return boot2Block;
	}
	
	/* [nitr8]: moved down here */
	nandPage = (u8*)malloc(NAND_PAGE_SIZE);

	__readPage(nandPage, block * BLOCK_SIZE + 1, NULL);
	
	if(!memcmp(boot2v1_sha1, nandPage + 1720, 0x14))
	{
		strcpy(boot2Block.version,    "boot2v1");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(boot2v2_sha1, nandPage + 1720, 0x14))
	{
		strcpy(boot2Block.version,    "boot2v2");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(boot2v3_sha1, nandPage + 1720, 0x14))
	{
		strcpy(boot2Block.version,    "boot2v3");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(boot2v4_sha1, nandPage + 1720, 0x14))
	{
		strcpy(boot2Block.version,    "boot2v4");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(boot2v5_sha1, nandPage + 1720, 0x14))
	{
		strcpy(boot2Block.version,    "boot2v5");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(sdboot_sha1, nandPage + 1720, 0x14))
	{
		strcpy(boot2Block.version,    "sdboot");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 1;
	}
	else if(!memcmp(nandboot_sha1, nandPage + 1720, 0x14))
	{
		strcpy(boot2Block.version,    "nandboot");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 1;
	}
	else if(nandPage[1654] == 0x42 && nandPage[1655] == 0x4D)
	{	/* "BM" */
		strcpy(boot2Block.version,    "BootMii");

		/* [nitr8]: warning: pointer targets in passing argument 2 of 'strncpy' differ in signedness
		strncpy(boot2Block.bootMiiVer, nandPage+1656, 3);   (e.g. "1.1") */
		strncpy(boot2Block.bootMiiVer, (const char *)nandPage+1656, 3);   /* e.g. "1.1" */

		boot2Block.bootMiiVer[3] = '\0';
		boot2Block.isBootMii = true;
		boot2Block.blockSize = (nandPage[1615] > 0) ? 2 : 1; /* boot2 version */
	}
	else
		boot2Block.blockSize = 0;
		
	/* [nitr8]: nandPage was never freed previously... (MEMORY LEAK) */
	free(nandPage);

	return boot2Block;
}

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
u32 TestNANDBlockmaps(void)
{
	/* Variable holds the size of ALL the boot blocks in NAND flash (incl. boot1) */
	static u8 boot_block_buffer[RAWBOOT2SIZE * (BOOT2_END + 1)] MEM2_BSS ALIGNED(64);

	u32 block;
	u32 page;
	int mapno;
	boot2blockmap *maps = (boot2blockmap*)sector_buf;
	u32 found = 0;

	memset(&good_blockmap, 0, sizeof(boot2blockmap));
	valid_blocks = 0;

	if (ReadBoot2Blocks(boot_block_buffer, BOOT2_START, BOOT2_END) == 0)
	{
		for (block = BOOT2_START; block <= BOOT2_END; block++)
		{
			// if block = 1 then page = 0x7F (first boot2 page with offset 0x417C0 in NAND flash)
			page = ((block + 1) * BLOCK_SIZE - 1);

			gecko_printf("read 0x%x bytes of data from 0x%x into 'sector_buf'\n", PAGE_SIZE_NO_ECC, (page * NAND_PAGE_SIZE) - (BOOT2_START * RAWBOOT2SIZE));

			memcpy(sector_buf, boot_block_buffer + (page * NAND_PAGE_SIZE) - (BOOT2_START * RAWBOOT2SIZE), PAGE_SIZE_NO_ECC);

			gecko_printf("read 0x%x bytes of ecc  from 0x%x into 'ecc_buf'\n", NAND_ECC_DATA_SIZE, (block * NAND_PAGE_SIZE) - ((BOOT2_END - block) * NAND_PAGE_SIZE) + PAGE_SIZE_NO_ECC + RAWBOOT2SIZE);

			memcpy(ecc_buf, boot_block_buffer + (page * NAND_PAGE_SIZE) + PAGE_SIZE_NO_ECC - (BOOT2_START * RAWBOOT2SIZE), NAND_ECC_DATA_SIZE);

			if (nand_correct(page, sector_buf, ecc_buf) < 0)
				printf("boot2 map candidate page 0x%x is uncorrectable, trying anyway\n", page);

			mapno = find_valid_map(maps);

			if (mapno >= 0)
			{
				printf("found valid boot2 blockmap at page 0x%x, submap %d, generation %u\n",
					page, mapno, maps[mapno].generation);

				if (maps[mapno].generation >= good_blockmap.generation)
				{
					memcpy(&good_blockmap, &maps[mapno], sizeof(boot2blockmap));
					found = 1;
				}
			}
		}
	}

	if (!found)
	{
		printf("no valid boot2 blockmap found!\n");
		return -1;
	}

	/* traverse the blockmap and make a list of the actual boot2 blocks, in order */
	for (block = BOOT2_END; block >= BOOT2_START; block--)
	{
		if (good_blockmap.blocks[block] == 0x00)
			boot2_blocks[valid_blocks++] = block;
	}

	printf("boot2 blocks:");

	for (block = 0; block < valid_blocks; block++)
		printf(" %02x", boot2_blocks[block]);

	printf("\n");

	return 0;
}

/* [nitr8]: Moved down here */
/* [nitr8]: Add Preprocessor flag to check upon */
#ifndef NO_DOLPHIN_CHECK
s32 NANDFlashInit(void)
{
	flash_fd = IOS_Open("/dev/flash", IPC_OPEN_RW);

	if (flash_fd < 0)
		return flash_fd;

	return 0;
}
#endif

/* [nitr8]: Moved down here */
/* [nitr8]: Unused */
#if 0
void NANDFlashClose(void)
{
	IOS_Close(flash_fd);
}
#endif

