/* RGD SDBoot Installer */

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <gctypes.h>

#include "errorhandler.h"
#include "errorcodes.h"
#include "seeprom.h"
#include "boot2.h"
#include "tools.h"
#include "prodinfo.h"
#include "runtimeiospatch.h"
#include "menu.h"

#define RGDSDB_VER_MAJOR	0
#define RGDSDB_VER_MINOR	7

#define SDBOOT_PATH           "/boot2/sdboot.bin"
#define NANDBOOT_PATH         "/boot2/nandboot.bin"
#define NANDBOOT_PAYLOAD_PATH "/boot2/payload.bin"
#define BOOT2WAD_PATH         "/boot2/boot2.wad"

#define AHBPROT_DISABLED (*(vu32*)0xcd800064 == 0xFFFFFFFF)

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int main(int argc, char **argv) {
	// This will reload to IOS36 while applying the required runtime patches
	// It will enable access to /dev/flash and fix a bug with ES_ImportBoot
	// TODO: Maybe try reloading to other IOSes as well (in case IOS36 is missing...)
	s32 res = IosPatch_FULL(0, 36);

	VIDEO_Init();
	WPAD_Init();
	PAD_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	
	printf("\x1b[2;0H");
	
	if(IsDolphin()) {
		ThrowError(errorStrings[ErrStr_InDolphin]);
	} else if(IsWiiU()) {
		ThrowError(errorStrings[ErrStr_InCafe]);
	}
	if(!fatInitDefault()){
		ThrowError(errorStrings[ErrStr_SDCard]);
	}
	if(!AHBPROT_DISABLED) { 
		ThrowError(errorStrings[ErrStr_NeedPerms]);
	}

	printf("RGD SDBoot Installer v%u.%u - by \x1b[32mroot1024\x1b[37m, \x1b[31mRedBees\x1b[37m, \x1b[31mDeadlyFoez\x1b[37m\nraregamingdump.ca", RGDSDB_VER_MAJOR, RGDSDB_VER_MINOR);
	printf("\nCurrent boot2 version: %i [DEBUG] res = %d\n\n", GetBoot2Version(), res);
	
	EnterMenu();

	return 0; // NOT REACHED HERE
}
