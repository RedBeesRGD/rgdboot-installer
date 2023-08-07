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
#include "flash.h"

#define SDBOOT_PATH           "/boot2/sdboot.bin"
#define NANDBOOT_PATH         "/boot2/nandboot.bin"
#define NANDBOOT_PAYLOAD_PATH "/boot2/payload.bin"
#define BOOT2WAD_PATH         "/boot2/boot2.wad"
#define BOOT2_BACKUP_PATH     "/boot2/backup.bin"

#define INSTALL_SD_BOOT   0
#define INSTALL_NAND_BOOT 1
#define INSTALL_WAD       2
#define INSTALL_BACKUP    3
#define MAKE_BOOT2_BACKUP 4
#define RESTORE_NAND_BACKUP 5

void HandleInstall(s32 ret, u8 installType) {
	switch(ret){
		case 0:
		case -2009:
			switch(installType){
				case INSTALL_SD_BOOT:
				case INSTALL_NAND_BOOT:
					printf("%s was installed successfully!\n", (installType == INSTALL_SD_BOOT) ? "SDBoot" : "NANDBoot");
					break;
				case INSTALL_WAD:
					printf("boot2 WAD was installed successfully!\n");
					break;
				case INSTALL_BACKUP:
					printf("boot2 backup was installed successfully!\n");
					break;
				case MAKE_BOOT2_BACKUP:
					printf("boot2 backup was performed successfully!\n");
					break;
				case RESTORE_NAND_BACKUP:
					printf("NAND backup was restored successfully!\n");
					break;
			} break;
		case MISSING_FILE:
			ThrowError(errorStrings[ErrStr_MissingFiles]); break;
		case BOOT2_DOLPHIN:
			ThrowError(errorStrings[ErrStr_InDolphin]); break;
		case HASH_MISMATCH:
			ThrowError(errorStrings[ErrStr_BadFile]); break;
		case CANNOT_DOWNGRADE:
			printf("Error: cannot downgrade boot2\n"); break;
		default:
			ThrowErrorEx(errorStrings[ErrStr_Generic], ret); break;
	}
}

void SEEPROMClearStep( void ) {
	printf("\n\nPress any controller button to clear the boot2 version.");
	WaitForPad();
	#ifdef DOLPHIN_CHECK // ClearVersion() crashes dolphin.
			     // This is included so that when building with NO_DOLPHIN_CHECK you can get past this point in Dolphin
	if(GetBoot2Version() > 0) {
	ClearVersion();
	}
	#endif
	
	printf("\nThe boot2 version was cleared successfully!\n");
}

void SDBootInstaller( void ) {
	Enable_DevBoot2();
	SEEPROMClearStep();
	if(IsMini()) {
		printf("Installing SDBoot on a Wii Mini could cause your system to be unusable due to the lack of an SD card slot.\n");
		printf("Press any controller button to continue anyways or press the power button on the console to exit.\n"); // TODO: Add SD file check
		WaitForPad();
	}
	
	HandleInstall(InstallSDBoot(SDBOOT_PATH), INSTALL_SD_BOOT);
	printf("\nPress any button to continue.");
	WaitForPad();
}

void NANDBootInstaller( void ) {
	//Enable_DevBoot2();
	SEEPROMClearStep();

	HandleInstall(InstallNANDBoot(NANDBOOT_PATH, NANDBOOT_PAYLOAD_PATH), INSTALL_NAND_BOOT);
	printf("\nPress any button to continue.");
	WaitForPad();
}

void Boot2WADInstaller( void ) {
	SEEPROMClearStep();
	Enable_DevBoot2();
	printf("\n\n");

	HandleInstall(InstallWADBoot2(BOOT2WAD_PATH), INSTALL_WAD);
	printf("\nPress any button to continue.");
	WaitForPad();
}

void Boot2BackupInstaller( void ) {
	SEEPROMClearStep();
	Enable_DevBoot2();
	printf("\n\n");

	HandleInstall(RestoreBoot2Blocks(BOOT2_BACKUP_PATH), INSTALL_BACKUP);
	printf("\nPress any button to continue.");
	WaitForPad();
}

void Boot2BackupMake( void ) {
	Enable_DevFlash();
	printf("\n\n");

	HandleInstall(BackupBoot2Blocks(BOOT2_BACKUP_PATH), MAKE_BOOT2_BACKUP);
	printf("\nPress any button to continue.");
	WaitForPad();
}

void RestoreNAND( void ){
	Enable_DevFlash();
	setMinBlock(8);
	
	printf("\nThis will first run in simulation mode. Press any key to continue.");
	WaitForPad();
	struct Simulation sim = flashFileSim("/nand.bin", 0, 4095);
	if(sim.blocksStatus == NULL)
		ThrowError(errorStrings[ErrStr_MissingFiles]);
				
	printf("\nPress A to continue, or any other button to go back to the menu\n");
	
	if(WaitForPad() != WPAD_BUTTON_A)
		return;
	
	HandleInstall(flashFile("/nand.bin", 0, 4095, &sim), RESTORE_NAND_BACKUP);
	setMinBlock(0);
	printf("\nPress any key to continue.");
	WaitForPad();
}
