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

#define RGDSDB_VER_MAJOR	0
#define RGDSDB_VER_MINOR	3

#define SDBOOT_PATH "/boot2/sdboot.bin"
#define NANDBOOT_PATH "/boot2/nandboot.bin"
#define BOOT2WAD_PATH "/boot2/boot2.wad"

#define AHBPROT_DISABLED (*(vu32*)0xcd800064 == 0xFFFFFFFF)

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int main(int argc, char **argv) {

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
	
	printf("RGD SDBoot Installer v%u.%u - by \x1b[32mroot1024\x1b[37m, \x1b[31mRedBees\x1b[37m, \x1b[31mDeadlyFoez\x1b[37m\nraregamingdump.ca", RGDSDB_VER_MAJOR, RGDSDB_VER_MINOR);
	printf("\nCurrent boot2 version: %i", GetBoot2Version());
	if(!fatInitDefault()){
		ThrowError(errorStrings[ErrStr_SDCard]);
	}

	if(!AHBPROT_DISABLED) { 
		ThrowError(errorStrings[ErrStr_NeedPerms]);
	}
	printf("\n\nPress any controller button to clear the boot2 version.");
	WaitForPad();
	if(!IsDolphin()) ClearVersion(); // ClearVersion() crashes dolphin.
					 // This is included so that when building with NO_DOLPHIN_CHECK you can get past this point in Dolphin
	
	u32 choice = 0;
	s32 ret = 0;
	printf("\nThe boot2 version was cleared successfully!\nPress the A button to install SDboot from /boot2/sdboot.bin, or the B button to install nandboot from /boot2/nandboot.bin.\n");
	printf("Press the + button to install a boot2 WAD from /boot2/boot2.wad\n");

choice:
	choice = WaitForPad();
	if(choice & WPAD_BUTTON_A) {
		if(IsMini()) {
			printf("Installing SDBoot on a Wii Mini could cause your system to be unusable due to the lack of an SD card slot.\nPress any controller button to continue anyways or press the power button on the console to exit."); // TODO: Add SD file check
		WaitForPad();																			
		}
	ret = InstallRawBoot2(SDBOOT_PATH);
	goto out;
		} else if(choice & WPAD_BUTTON_B) {
			ret = InstallRawBoot2(NANDBOOT_PATH);
			goto out;
			} else if(choice & WPAD_BUTTON_PLUS){
				ret = InstallWADBoot2(BOOT2WAD_PATH);
			} else { goto choice; }
	
out:
	switch(ret){
		case 0:
			if(choice & WPAD_BUTTON_PLUS) {printf("boot2 WAD was installed successfully!\n"); break;}
			printf("%s was installed successfully!\n", (choice & WPAD_BUTTON_A) ? "SDBoot" : "NANDBoot"); break;
		case -4:
			ThrowError(errorStrings[ErrStr_InDolphin]); break;
		case -1022:
			ThrowError(errorStrings[ErrStr_BadFile]); break;
		case -1031: // TODO: Find out why this triggers sometimes even after writing SEEPROM
			printf("Error: cannot downgrade boot2\n"); break;
		default:
			ThrowErrorEx(errorStrings[ErrStr_Generic], ret); break;
	}

	WaitExit();
	return 0;	// NOT REACHED HERE
}
