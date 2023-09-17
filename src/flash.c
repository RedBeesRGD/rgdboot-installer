#define NAND_PAGE_SIZE     0x840
#define NAND_BLOCK_SIZE    NAND_PAGE_SIZE*64

#include "flash.h"
#include "tools.h"
#include "sha256.h"

u32 nand_min_block = 0;

s32 fd = -1;

s32 NANDFlashInit(){
	fd = IOS_Open("/dev/flash", IPC_OPEN_RW);
	if (fd < 0)
		return fd;
	return 0;
}

void NANDFlashClose(){
	IOS_Close(fd);
}


s32 __isPageEmpty(u8* data){
	u8 emptyPage[NAND_PAGE_SIZE];
	memset(emptyPage, 0xFF, NAND_PAGE_SIZE);
	
	return (!memcmp(data, emptyPage, NAND_PAGE_SIZE));
}

s32 __isBlockBad(){
	s32 rv;
	rv = IOS_Ioctl(fd, 4, NULL, 0, NULL, 0); // Is this block bad?
	return (rv == BAD_BLOCK);
}

s32 __readPage(u8* data, int pageno){
	static unsigned char buffer[NAND_PAGE_SIZE] __attribute__ ((aligned(32)));
	int rv;
	
	rv = IOS_Seek(fd, pageno, 0);
	if (rv < 0)
		return SEEK_ERROR;
	rv = IOS_Read(fd, buffer, (u32) NAND_PAGE_SIZE);
	
	memcpy(data, buffer, NAND_PAGE_SIZE);
	
	return 0;
}

s32 __flashPage(u8* data, int pageno){
	if(pageno/64 < nand_min_block)
		return 0;
	
	static unsigned char buffer[NAND_PAGE_SIZE] __attribute__ ((aligned(32)));
	int rv;
	
	if(__isPageEmpty(data))
		return 0;
	
	memcpy(buffer, data, NAND_PAGE_SIZE);
	
	IOS_Seek(fd, pageno, 0);
	IOS_Write(fd, buffer, (u32) NAND_PAGE_SIZE);
	
	return 0;
}

s32 __eraseBlock(int blockno){
	if(blockno < nand_min_block)
		return 0;
	
	int rv;
	
	rv = IOS_Seek(fd, blockno * 64, 0);
	if (rv < 0)
		return SEEK_ERROR;

	if(__isBlockBad())
		return BAD_BLOCK;
	
	rv = IOS_Ioctl(fd, 3, NULL, 0, NULL, 0); // Erase block
	if(rv < 0)
		return ERASE_ERROR;
	
	return 0;
}

s32 __flashBlock(u8* data, int blockno){
	if(blockno < nand_min_block)
		return 0;
	
	int rv;
	u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE);
	
	rv = __eraseBlock(blockno);
	if(rv < 0)
		return rv;
	
	for(int pg = 0; pg < 64; pg++){
		memcpy(nandPage, data+pg*NAND_PAGE_SIZE, NAND_PAGE_SIZE);
		__flashPage(nandPage, pg+blockno*64);
	}
	
	free(nandPage);
	
	return 0;
}

struct Simulation __simulateWrite(const char* fileName, int firstBlock, int lastBlock){
	int rv;
	struct Simulation sim;
	u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE);
	u8* filePage = (u8*)malloc(NAND_PAGE_SIZE);
	
	sim.blocksStatus = (u8*)malloc(lastBlock - firstBlock + 1);
	sim.toBeWritten  = 0;
	
	// 0: No need to write
	// 1: Needs to be erased and written
	// 2: Bad block
	
	FILE *fin = fopen(fileName, "rb");
	if(fin == NULL){
		printf("Cannot open file!\n");
		free(sim.blocksStatus);
		sim.blocksStatus = NULL;
		return sim;
	}
	
	for(int block = firstBlock; block <= lastBlock; block++){
		sim.blocksStatus[block - firstBlock] = 0;
		
		if(block < nand_min_block)
			continue;
		
		printf(" [+] Analyzing block %d... ", block);
		
		IOS_Seek(fd, block * 64, 0);
		
		if(__isBlockBad()){ // Is this block bad?
			printf("bad\n");
			sim.blocksStatus[block - firstBlock] = 2;
			continue;
		}
		
		for(int pg = block*64; pg < (block+1)*64; pg++){
			rv = __readPage(nandPage, pg);
			fseek(fin, pg*NAND_PAGE_SIZE, SEEK_SET);
			fread(filePage, NAND_PAGE_SIZE, 1, fin);
			
			if(memcmp(nandPage, filePage, NAND_PAGE_SIZE)){
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
	fclose(fin);
	
	return sim;
}

s32 flashFile(const char* fileName, int firstBlock, int lastBlock, struct Simulation* sim){
	int rv;
	u8* data = (u8*)malloc(NAND_BLOCK_SIZE);
	
	FILE *fin = fopen(fileName, "rb");
	if(fin == NULL)
		return MISSING_FILE;
	
	for(int block = firstBlock; block <= lastBlock; block++){
		if(block < nand_min_block)
			continue;
		
		//fseek(fin, block*NAND_BLOCK_SIZE, SEEK_SET);
		fseek(fin, (block - firstBlock)*NAND_BLOCK_SIZE, SEEK_SET);
		
		if(sim != NULL){
			if(sim->blocksStatus[block - firstBlock] == 1){
				printf(" [+] Flashing block %d...\n", block);
				fread(data, NAND_BLOCK_SIZE, 1, fin);
				__flashBlock(data, block);
			}
		}
		else{
			printf(" [+] Flashing block %d...\n", block);
			fread(data, NAND_BLOCK_SIZE, 1, fin);
			__flashBlock(data, block);
		}
	}
	
	free(data);
	fclose(fin);
	
	return 0;
}

struct Simulation flashFileSim(const char* fileName, int firstBlock, int lastBlock){
	int blocksToModify = 0;
	struct Simulation sim = __simulateWrite(fileName, firstBlock, lastBlock);
	
	printf("Bad blocks:\n");
	for(int block = firstBlock; block <= lastBlock; block++){
		if(sim.blocksStatus[block-firstBlock] == 2)
			printf(" [-] block %d\n", block);
		else if(sim.blocksStatus[block-firstBlock] == 1)
			blocksToModify++;
	}
	
	printf("Will erase %d pages (%d blocks)...\n", blocksToModify*64, blocksToModify);
	printf("Will write %d pages...\n", sim.toBeWritten);
	
	return sim;
}

s32 dumpPages(const char* fileName, int firstPage, int lastPage){
	u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE);
	
	FILE *fout = fopen(fileName, "wb");
	if(fout == NULL){
		printf("Cannot open file!\n");
		return MISSING_FILE;
	}
	
	for(int page = firstPage; page <= lastPage; page++){
		__readPage(nandPage, page);
		fwrite(nandPage, NAND_PAGE_SIZE, 1, fout);
	}
	
	fclose(fout);
	free(nandPage);
	
	return 0;
}

s32 dumpBlocks(const char* fileName, int firstBlock, int lastBlock){
	return dumpPages(fileName, firstBlock*64, (lastBlock+1)*64-1);
}

s32 eraseBlocks(int firstBlock, int lastBlock){
	for(int block = firstBlock; block <= lastBlock; block++){
		printf(" [+] Erasing block %d...\n", block);
		__eraseBlock(block);
	}
	return 0;
}

u32 checkBlocks(int firstBlock, int lastBlock){
	u32 badBlocks = 0;

	for(int block = firstBlock; block <= lastBlock; block++){
		printf(" [+] Checking block %d... ", block);
		
		IOS_Seek(fd, block * 64, 0);
		
		if(__isBlockBad()){
			badBlocks++;
			printf("BAD!!! :(\n");
		}
		else
			printf("good\n");
	}
	
	return badBlocks;
}

void setMinBlock(u32 blockno){
	nand_min_block = blockno;
}

char identifyBoot1(){
	u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE);
	u8 tmpByte;
	
	__readPage(nandPage, 0);
	tmpByte = nandPage[0x40];
	free(nandPage);
	
	switch(tmpByte){
		case 0x99: return 'a';
		case 0xC1: return 'b';
		case 0xFC: return 'c';
		case 0x48: return 'd';
		
		default:   return '?';
	}
	
	return '?';
}

Boot2Block identifyBoot2(u8 copy){
	
	const char boot2v1_sha1[0x14] = {0xFD, 0x53, 0x7E, 0x4E, 0xD7, 0x9A, 0x3F, 0xB3, 0xE0, 0x35, 0xC5, 0x10, 0x1A, 0xCC, 0x48, 0xF1, 0x9E, 0x5D, 0xE1, 0x05};
	const char boot2v2_sha1[0x14] = {0xBD, 0x0F, 0x4F, 0xC7, 0xDF, 0xE0, 0xD8, 0xF1, 0x37, 0x54, 0x9E, 0xB3, 0x6F, 0xBF, 0xD5, 0x6B, 0x3D, 0xAE, 0x84, 0xEE};
	const char boot2v3_sha1[0x14] = {0xEA, 0x5C, 0x22, 0x73, 0xC8, 0xAB, 0xAD, 0x2B, 0x13, 0x64, 0x7B, 0xB4, 0xCB, 0x2D, 0x20, 0xF0, 0xF9, 0x49, 0x29, 0x51};
	const char boot2v4_sha1[0x14] = {0xEB, 0xFC, 0x36, 0x20, 0x0D, 0xDB, 0x3E, 0x32, 0x8D, 0xA1, 0xC5, 0x2B, 0x23, 0x3D, 0xFC, 0x8C, 0xAF, 0x60, 0xB6, 0x71};
	const char boot2v5_sha1[0x14] = {0x4F, 0x99, 0xBA, 0xDF, 0x98, 0x8E, 0x3F, 0xD7, 0xA8, 0x21, 0x2C, 0xCA, 0x35, 0xD5, 0x3C, 0xBB, 0x9E, 0x7B, 0x7C, 0xBC};
	
	const char sdboot_sha1[0x14]   = {0xE5, 0x30, 0xDE, 0xAF, 0x73, 0x0D, 0x24, 0xE0, 0xA8, 0xE0, 0x8D, 0xCA, 0xC0, 0x94, 0x39, 0x01, 0x46, 0x36, 0x1A, 0x5E};
	const char nandboot_sha1[0x14] = {0xD3, 0x84, 0x29, 0xDA, 0xBB, 0x18, 0xF2, 0x47, 0xCC, 0xF5, 0x24, 0x47, 0x78, 0xD4, 0x66, 0x6B, 0x2E, 0xFB, 0x3F, 0x29};
	
	Boot2Block boot2Block;
	u8* nandPage = (u8*)malloc(NAND_PAGE_SIZE);
	u8 blocksToTest[3] = {1,3,7};
	u8 block = blocksToTest[copy];
	
	IOS_Seek(fd, block * 64, 0);
	
	if(__isBlockBad()){ // Is this block bad?
		boot2Block.blockSize = 0;
		
		return boot2Block;
	}
	
	__readPage(nandPage, block * 64 + 1);
	
	if(!memcmp(boot2v1_sha1, nandPage + 1720, 0x14)){
		strcpy(boot2Block.version,    "boot2v1");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(boot2v2_sha1, nandPage + 1720, 0x14)){
		strcpy(boot2Block.version,    "boot2v2");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(boot2v3_sha1, nandPage + 1720, 0x14)){
		strcpy(boot2Block.version,    "boot2v3");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(boot2v4_sha1, nandPage + 1720, 0x14)){
		strcpy(boot2Block.version,    "boot2v4");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	else if(!memcmp(boot2v5_sha1, nandPage + 1720, 0x14)){
		strcpy(boot2Block.version,    "boot2v5");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 2;
	}
	
	else if(!memcmp(sdboot_sha1, nandPage + 1720, 0x14)){
		strcpy(boot2Block.version,    "sdboot");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 1;
	}
	else if(!memcmp(nandboot_sha1, nandPage + 1720, 0x14)){
		strcpy(boot2Block.version,    "nandboot");
		strcpy(boot2Block.bootMiiVer, "\0");
		boot2Block.isBootMii = false;
		boot2Block.blockSize = 1;
	}
	
	else if(nandPage[1654] == 0x42 && nandPage[1655] == 0x4D){    // "BM"
		strcpy(boot2Block.version,    "BootMii");
		strncpy(boot2Block.bootMiiVer, nandPage+1656, 3);   // e.g. "1.1"
		boot2Block.bootMiiVer[3] = '\0';
		boot2Block.isBootMii = true;
		boot2Block.blockSize = (nandPage[1615] > 0) ? 2 : 1; // boot2 version
	}
	else
		boot2Block.blockSize = 0;
	
	
	return boot2Block;
}




