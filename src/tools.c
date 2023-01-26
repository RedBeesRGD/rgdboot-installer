#include "tools.h"

u32 GetInput(){
	WPAD_ScanPads();
	u32 pressed = WPAD_ButtonsDown(0);
	
	if(pressed) return pressed;
	
	return 0;
}

void Terminate(){
	printf("Press HOME to exit\n");
	u32 pressed;
	while(1){
		pressed = GetInput();
		if(pressed & WPAD_BUTTON_HOME)
			exit(0);
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

int CheckFile(FILE* fp, const char *filename){
	if(fp == NULL){
		printf("Error: cannot read file %s\n", filename);
		return 0;
	}
	return 1;
}
