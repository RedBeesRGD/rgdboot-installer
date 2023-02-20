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
#include "installer.h"

#define SDBOOT_PATH           "/boot2/sdboot.bin"
#define NANDBOOT_PATH         "/boot2/nandboot.bin"
#define NANDBOOT_PAYLOAD_PATH "/boot2/payload.bin"
#define BOOT2WAD_PATH         "/boot2/boot2.wad"

void SEEPROMClearStep( void ) {
	printf("\n\nPress any controller button to clear the boot2 version.");
	WaitForPad();
	#ifdef DOLPHIN_CHECK
	if(GetBoot2Version() > 0) {
	ClearVersion(); // ClearVersion() crashes dolphin.
			// This is included so that when building with NO_DOLPHIN_CHECK you can get past this point in Dolphin
	}
	#endif
	
	printf("\nThe boot2 version was cleared successfully!\n");
}

void SDBootInstaller( void ) {
	SEEPROMClearStep();
	if(IsMini()) {
		printf("Installing SDBoot on a Wii Mini could cause your system to be unusable due to the lack of an SD card slot.\n");
		printf("Press any controller button to continue anyways or press the power button on the console to exit.\n"); // TODO: Add SD file check
		WaitForPad();																			
		}
	s32 ret = 0;
	ret = InstallSDBoot(SDBOOT_PATH);
	
	printf("\nPress any button to continue.");
	WaitForPad();
}

void NANDBootInstaller( void ) {
	SEEPROMClearStep();
	s32 ret = 0;
	ret = InstallNANDBoot(NANDBOOT_PATH, NANDBOOT_PAYLOAD_PATH);
	
	printf("\nPress any button to continue.");
	WaitForPad();
}

void Boot2WADInstaller( void ) {
	s32 ret = 0;
	
	printf("\n\n");
	InstallWADBoot2(BOOT2WAD_PATH);
	
	printf("\nPress any button to continue.");
	WaitForPad();	
}

/*	switch(ret){
		case 0:
			if(choice & WPAD_BUTTON_PLUS) {printf("boot2 WAD was installed successfully!\n"); break;}
			printf("%s was installed successfully!\n", (choice & WPAD_BUTTON_A) ? "SDBoot" : "NANDBoot"); break;
		case MISSING_FILE:
			ThrowError(errorStrings[ErrStr_MissingFiles]); break;
		case BOOT2_DOLPHIN:
			ThrowError(errorStrings[ErrStr_InDolphin]); break;
		case HASH_MISMATCH:
			ThrowError(errorStrings[ErrStr_BadFile]); break;
		case CANNOT_DOWNGRADE: // TODO: Find out why this triggers sometimes even after writing SEEPROM
			printf("Error: cannot downgrade boot2\n"); break;
		default:
			ThrowErrorEx(errorStrings[ErrStr_Generic], ret); break;
	}*/

/*	WaitExit();
	return 0;	// NOT REACHED HERE
}*/
