#include "tools.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

u32 getInput(){
	WPAD_ScanPads();
	u32 pressed = WPAD_ButtonsDown(0);
	
	if(pressed)
		return pressed;
	
	return 0;
}

void terminate(){
	printf("Press HOME to exit\n");
	u32 pressed;
	while(1){
		pressed = getInput();
		if(pressed & WPAD_BUTTON_HOME)
			exit(0);
	}
}

void setup(){
	VIDEO_Init();
	WPAD_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	if(!fatInitDefault()){
		printf("Error: cannot init FAT\n");
		terminate();
	}
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

int checkFile(FILE* fp, const char *filename){
	if(fp == NULL){
		printf("Error: cannot read file %s\n", filename);
		return 0;
	}
	return 1;
}
