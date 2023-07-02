#define NAND_PAGE_SIZE     0x840
#define NAND_BLOCK_SIZE    NAND_PAGE_SIZE*64

#include "flash.h"
#include "tools.h"

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
		printf(" [+] Analyzing block %d... ", block);
		
		sim.blocksStatus[block - firstBlock] = 0;
		
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
