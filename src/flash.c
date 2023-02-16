#define NAND_PAGE_SIZE     0x840
#define NAND_BLOCK_SIZE    NAND_PAGE_SIZE*64

#include "flash.h"
#include "tools.h"

void eraseBlocks(int firstBlock, int lastBlock){
	s32 fd = -1;
	int rv;

	fd = IOS_Open("/dev/flash", IPC_OPEN_WRITE);
	if (fd < 0) {
		printf("Failed to open /dev/flash (fd = %d)\n", fd);
		WaitExit();
	}

	for(int block = firstBlock; block < lastBlock + 1; block++){
		rv = IOS_Seek(fd, block * 64, 0);
		if (rv < 0){
			printf("Failed to seek to block %d (rv = %d)\n", block, rv);
			WaitExit();
		}
		printf("Erasing block %d...\n", block);
		rv = IOS_Ioctl(fd, 3, NULL, 0, NULL, 0); // Erase block
		if(rv < 0){
			printf("Failed to erase block %d (rv = %d)\n", block, rv);
			WaitExit();
		}
	}

	IOS_Close(fd);
}

void eraseBlock(int block){
	eraseBlocks(block, block);
}

void dumpBlocks(const char *filename, int firstBlock, int lastBlock){
	static unsigned char buffer[NAND_BLOCK_SIZE] __attribute__ ((aligned(32)));
	s32 fd = -1;
	int rv;

	printf("Dumping blocks...\n");

	FILE *fout = fopen(filename, "wb");
	if(fout == NULL){
		printf("Error: cannot open file %s\n", filename);
		WaitExit();
	}

	fd = IOS_Open("/dev/flash", IPC_OPEN_READ);
	if (fd < 0) {
		printf("Failed to open /dev/flash (fd = %d)\n", fd);
		WaitExit();
	}

	rv = IOS_Seek(fd, firstBlock * 64, 0);
	if (rv < 0){
		printf("Failed to seek to block %d (rv = %d)\n", firstBlock, rv);
		WaitExit();
	}

	for (int page = firstBlock * 64; page < (lastBlock + 1) * 64; page++) {
		if(page % 64 == 0)
			printf("Flashing block %d...\n", page / 64);

		rv = IOS_Seek(fd, page, 0);
		if (rv < 0){
			printf("Failed to seek to page %d (rv = %d)\n", page, rv);
			WaitExit();
		}

		rv = IOS_Read(fd, buffer, (u32) NAND_PAGE_SIZE);

		fwrite(buffer, NAND_PAGE_SIZE, 1, fout);
	}

	IOS_Close(fd);
	fclose(fout);
}

void flashBlocks(const char *filename, int firstBlock, int lastBlock){
	static unsigned char buffer[NAND_BLOCK_SIZE] __attribute__ ((aligned(32)));
	s32 fd = -1;
	int rv;

	printf("Flashing blocks...\n");

	FILE *fin = fopen(filename, "rb");
	if(fin == NULL){
		printf("Error: cannot open file %s\n", filename);
		WaitExit();
	}

	fd = IOS_Open("/dev/flash", IPC_OPEN_WRITE);
	if (fd < 0) {
		printf("Failed to open /dev/flash (fd = %d)\n", fd);
		WaitExit();
	}

	for(int block = firstBlock; block < lastBlock + 1; block++){
		rv = IOS_Seek(fd, block * 64, 0);
		if (rv < 0){
			printf("Failed to seek to block %d (rv = %d)\n", block, rv);
			WaitExit();
		}
		printf("Erasing block %d...\n", block);
		rv = IOS_Ioctl(fd, 3, NULL, 0, NULL, 0); // Erase block
		if(rv < 0){
			printf("Failed to erase block %d (rv = %d)\n", block, rv);
			WaitExit();
		}
	}

	for (int page = firstBlock * 64; page < (lastBlock + 1) * 64; page++) {
		if(page % 64 == 0)
			printf("Flashing block %d...\n", page / 64);

		rv = IOS_Seek(fd, page, 0);
		if (rv < 0){
			printf("Failed to seek to page %d (rv = %d)\n", page, rv);
			WaitExit();
		}

		fread(buffer, NAND_PAGE_SIZE, 1, fin);

		rv = IOS_Write(fd, buffer, (u32) NAND_PAGE_SIZE);

		if(rv != 2112)
			printf("Write error: page %d block %d (rv = %d)\n", page, page / 64, rv);
	}

	IOS_Close(fd);
	fclose(fin);
}

void dumpBlock(const char *filename, int block){
	dumpBlocks(filename, block, block);
}

void flashBlock(const char *filename, int block){
	flashBlocks(filename, block, block);
}
