#define NAND_PAGE_SIZE     0x840
#define NAND_BLOCK_SIZE    NAND_PAGE_SIZE*64

#include "flash.h"
#include "tools.h"

s32 eraseBlocks(int firstBlock, int lastBlock){
	s32 fd = -1;
	int rv;

	fd = IOS_Open("/dev/flash", IPC_OPEN_WRITE);
	if (fd < 0)
		return fd;

	for(int block = firstBlock; block < lastBlock + 1; block++){
		rv = IOS_Seek(fd, block * 64, 0);
		if (rv < 0)
			return SEEK_ERROR;

		rv = IOS_Ioctl(fd, 3, NULL, 0, NULL, 0); // Erase block
		if(rv < 0)
			return ERASE_ERROR;
	}

	IOS_Close(fd);

	return 0;
}

s32 eraseBlock(int block){
	return eraseBlocks(block, block);
}

s32 dumpBlocks(const char *filename, int firstBlock, int lastBlock){
	static unsigned char buffer[NAND_BLOCK_SIZE] __attribute__ ((aligned(32)));
	s32 fd = -1;
	int rv;

	FILE *fout = fopen(filename, "wb");
	if(fout == NULL)
		return MISSING_FILE;

	fd = IOS_Open("/dev/flash", IPC_OPEN_READ);
	if (fd < 0)
		return fd;

	rv = IOS_Seek(fd, firstBlock * 64, 0);
	if (rv < 0)
		return SEEK_ERROR;

	for (int page = firstBlock * 64; page < (lastBlock + 1) * 64; page++) {
		rv = IOS_Seek(fd, page, 0);
		if (rv < 0)
			return SEEK_ERROR;

		rv = IOS_Read(fd, buffer, (u32) NAND_PAGE_SIZE);

		fwrite(buffer, NAND_PAGE_SIZE, 1, fout);
	}

	IOS_Close(fd);
	fclose(fout);

	return 0;
}

s32 flashBlocks(const char *filename, int firstBlock, int lastBlock){
	static unsigned char buffer[NAND_BLOCK_SIZE] __attribute__ ((aligned(32)));
	s32 fd = -1;
	int rv;

	FILE *fin = fopen(filename, "rb");
	if(fin == NULL)
		return MISSING_FILE;

	fd = IOS_Open("/dev/flash", IPC_OPEN_WRITE);
	if (fd < 0)
		return fd;

	for(int block = firstBlock; block < lastBlock + 1; block++){
		rv = IOS_Seek(fd, block * 64, 0);
		if (rv < 0)
			return SEEK_ERROR;

		rv = IOS_Ioctl(fd, 3, NULL, 0, NULL, 0); // Erase block
		if(rv < 0)
			return ERASE_ERROR;
	}

	for (int page = firstBlock * 64; page < (lastBlock + 1) * 64; page++) {
		rv = IOS_Seek(fd, page, 0);
		if (rv < 0)
			return SEEK_ERROR;

		fread(buffer, NAND_PAGE_SIZE, 1, fin);

		rv = IOS_Write(fd, buffer, (u32) NAND_PAGE_SIZE);
	}

	IOS_Close(fd);
	fclose(fin);

	return 0;
}

s32 dumpBlock(const char *filename, int block){
	return dumpBlocks(filename, block, block);
}

s32 flashBlock(const char *filename, int block){
	return flashBlocks(filename, block, block);
}
