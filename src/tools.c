/* RGD SDBoot Installer */

#include "tools.h"
#include "errorhandler.h"
#include "errorcodes.h"
#include "sha256.h"

u32 WaitForPad() {
	u32 wpadButtons = 0;
	u32 padButtons = 0;

	while(1) {
		WPAD_ScanPads();
		PAD_ScanPads();
		for(int i = 0; i < 4; i++) {
				wpadButtons |= WPAD_ButtonsDown(i);
				padButtons |= PAD_ButtonsDown(i); 
		}
		if (wpadButtons || padButtons) break;
		VIDEO_WaitVSync();
	}
	
	if(padButtons & PAD_BUTTON_UP) {
		padButtons &= ~PAD_BUTTON_UP;
		padButtons |= RGDSDB_PAD_BUTTON_UP;	// To prevent conflicts
	}
		
//	u32 ret = 0;
//	if(!wpadButtons) ret = padButtons;
//	if(!padButtons) ret = wpadButtons;

	return wpadButtons;
}

u8 IsWiiU( void ) {
	if(*(vu16*)0xcd8005a0 == 0xCAFE) return 1;
	return 0;
}

u8 IsDolphin( void ) {
	u8 dolphinFlag = 0;
	int fd = IOS_Open("/dev/dolphin", 1);
	if(fd >= 0) {
		IOS_Close(fd);
		dolphinFlag = 1;
	}
	IOS_Close(fd);
	#ifndef DOLPHIN_CHECK
	if(!dolphinFlag && GetBoot2Version()) ThrowError(errorStrings[ErrStr_WrongVersion]);
	return 0;
	#else
	return dolphinFlag;
	#endif
}

void WaitExit( void ) {
	printf("\n\nPress any controller button to exit.");
	WaitForPad();
	exit(0);
}

void *alloc(u32 size){
	return memalign(32, (size+31)&(~31));
}

u32 filesize(FILE* fp){
	fseek(fp, 0L, SEEK_END);
	u32 size = ftell(fp);
	rewind(fp);
	return size;
}

int CheckFile(FILE* fp, const char *filename){
	if(fp == NULL){
		printf("Error: cannot read file %s\n", filename);
		return 0;
	}
	return 1;
}

int CheckFileHash(const char *filename, u8 expectedHash[], int expectedSize){
	FILE *fp = fopen(filename, "rb");
	int bytesread;

	if(fp == NULL)
		return 2;

	BYTE payload[expectedSize];
	bytesread = fread(payload, 1, expectedSize, fp);
	fclose(fp);

	if(bytesread != expectedSize)
		return 0;

	BYTE actualHash[SHA256_BLOCK_SIZE];
	
	SHA256_CTX ctx;
	
	sha256_init(&ctx);
	sha256_update(&ctx, payload, expectedSize);
	sha256_final(&ctx, actualHash);

	return memcmp(expectedHash, actualHash, SHA256_BLOCK_SIZE);
}

u32 GetBoot2Version( void ) {
         u32 boot2Version = 0;
         if(ES_GetBoot2Version(&boot2Version) < 0) ThrowError(errorStrings[ErrStr_BadBoot2Ver]);
         return boot2Version;
}
