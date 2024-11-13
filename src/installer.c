/* RGD SDBoot Installer */

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <gctypes.h>

#include "errorhandler.h"
#include "errorcodes.h"

/* [nitr8]: Added*/
#include "errorstrings.h"

#include "seeprom.h"
#include "boot2.h"
#include "tools.h"
#include "prodinfo.h"
#include "runtimeiospatch.h"
#include "menu.h"
#include "installer.h"
#include "flash.h"
#include "hbc.h"

#include "hbc/hbc_content0.h"
#include "hbc/hbc_content1.h"
#include "hbc/hbc_certs.h"
#include "hbc/hbc_tik.h"
#include "hbc/hbc_tmd.h"

/* [nitr8]: get rid of warning for implicit declaration of function */
#include "filemanager.h"


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
#define INSTALL_HBC 6

/* [nitr8]: Added controller button pattern so some of the menu entries don't get
	    executed out of a sudden, like "ERASE NAND" which is just dangerous */

/* The button pattern to match for a Wiimote */
static u32 wpad_pattern[] = {
	WPAD_BUTTON_UP,
	WPAD_BUTTON_UP,
	WPAD_BUTTON_DOWN,
	WPAD_BUTTON_DOWN,
	WPAD_BUTTON_LEFT,
	WPAD_BUTTON_RIGHT,
	WPAD_BUTTON_LEFT,
	WPAD_BUTTON_RIGHT,
	WPAD_BUTTON_B,
	WPAD_BUTTON_A
};

/* The button pattern to match for a Wiimote Classic Controller */
static u32 wpad_classic_pattern[] = {
	WPAD_CLASSIC_BUTTON_UP,
	WPAD_CLASSIC_BUTTON_UP,
	WPAD_CLASSIC_BUTTON_DOWN,
	WPAD_CLASSIC_BUTTON_DOWN,
	WPAD_CLASSIC_BUTTON_LEFT,
	WPAD_CLASSIC_BUTTON_RIGHT,
	WPAD_CLASSIC_BUTTON_LEFT,
	WPAD_CLASSIC_BUTTON_RIGHT,
	WPAD_CLASSIC_BUTTON_B,
	WPAD_CLASSIC_BUTTON_A
};

/* The button pattern to match for a GC Controller */
static u32 pad_pattern[] = {
	PAD_BUTTON_UP,
	PAD_BUTTON_UP,
	PAD_BUTTON_DOWN,
	PAD_BUTTON_DOWN,
	PAD_BUTTON_LEFT,
	PAD_BUTTON_RIGHT,
	PAD_BUTTON_LEFT,
	PAD_BUTTON_RIGHT,
	PAD_BUTTON_B,
	PAD_BUTTON_A
};

static int pattern_length = sizeof(wpad_pattern) / sizeof(wpad_pattern[0]);

/* [nitr8]: Make static and reorder error switch cases */
/* void HandleInstall(s32 ret, u8 installType) */
static void HandleInstall(s32 ret, u8 installType)
{
	switch (ret)
	{
		case 0:
		case -2009:
			switch (installType)
			{
				case INSTALL_SD_BOOT:
				case INSTALL_NAND_BOOT:
					printf("%s was installed successfully!\n", (installType == INSTALL_SD_BOOT) ? "SDBoot" : "NANDBoot");
					break;

				case INSTALL_WAD:
					printf("boot2 WAD was installed successfully!\n");
					break;

				case INSTALL_BACKUP:
					printf("boot2 backup was restored successfully!\n");
					break;

				case MAKE_BOOT2_BACKUP:
					printf("boot2 backup was performed successfully!\n");
					break;

				case RESTORE_NAND_BACKUP:
					printf("NAND backup was restored successfully!\n");
					break;

				case INSTALL_HBC:
					printf("HBC installation was performed successfully!\n");
			}

			break;

		case MISSING_FILE:
			ThrowError(errorStrings[ErrStr_MissingFiles]);
			break;

		/* [nitr8]: Added */
		case ERASE_ERROR:
			ThrowError(errorStrings[ErrStr_EraseError]);
			break;

		/* [nitr8]: Added */
		case SEEK_ERROR:
			ThrowError(errorStrings[ErrStr_SeekError]);
			break;

		case BOOT2_DOLPHIN:
			ThrowError(errorStrings[ErrStr_InDolphin]);
			break;

		case BAD_BOOT_BLOCKS:
			ThrowError(errorStrings[ErrStr_BadBlocks]);
			break;

		/* [nitr8]: Added */
		case ALLOC_ERROR:
			ThrowError(errorStrings[ErrStr_AllocError]);
			break;

		/* [nitr8]: Added */
		case READ_LENGTH_ERROR:
			ThrowError(errorStrings[ErrStr_ReadLengthError]);
			break;

		/* [nitr8]: Added */
		case BAD_BLOCK:
			ThrowError(errorStrings[ErrStr_BadBlockError]);
			break;

		case HASH_MISMATCH:
			ThrowError(errorStrings[ErrStr_BadFile]);
			break;

		/* [nitr8]: Modified (moved into errorStrings array)*/
		case CANNOT_DOWNGRADE:
			ThrowError(errorStrings[ErrStr_DowngradeError]);
			break;

		default:
			ThrowErrorEx(errorStrings[ErrStr_Generic], ret);
			break;
	}
}

/* [nitr8]: Added controller button pattern so some of the menu entries don't get
	    executed out of a sudden, like "ERASE NAND" which is just dangerous */
static int button_match_pattern(void)
{	
	static int err_state = 0;
	int wpad_index = 0;
	int wpad_classic_index = 0;
	int pad_index = 0;

	printf("\n");
	printf("If you are SURE you want to do this, KONAMI.\n");
	printf("Otherwise, press [Z] or [HOME] to give up.\n");

	while (1)
	{
		int match_wpad;
		int match_wpad_classic;
		int match_pad;

		pad_t pad_button = WaitForPad();

		match_wpad = (pad_button.button == wpad_pattern[wpad_index]);
		match_wpad_classic = (pad_button.button == wpad_classic_pattern[wpad_classic_index]);
		match_pad = (pad_button.button == pad_pattern[pad_index]);

		if (err_state)
			ClearScreen();

		if (match_wpad || match_wpad_classic || match_pad)
		{
			if (match_wpad)
			{
				wpad_index++;

				if (wpad_index == pattern_length)
					/* Pattern matched, continue... */
					break;

				/* Reset other indices */
				wpad_classic_index = 0;
				pad_index = 0;
			}
			else if (match_wpad_classic)
			{
				wpad_classic_index++;

				if (wpad_classic_index == pattern_length)
					/* Pattern matched, continue... */
					break;

				/* Reset other indices */
				wpad_index = 0;
				pad_index = 0;
			}
			else if (match_pad)
			{
				pad_index++;

				if (pad_index == pattern_length)
					/* Pattern matched, continue... */
					break;

				/* Reset other indices */
				wpad_index = 0;
				wpad_classic_index = 0;
			}

			if (err_state)
				printf("\n");

			printf("POW!\n");

			err_state = 0;
		}
		else
		{
			/* Reset the pattern index if the button does not match */
			if (pad_button.button != 0) /* Assuming 0 indicates no button pressed */
			{
				printf("\n");
				printf("Try harder Hashimoto-san!\n");

				err_state = 1;
			}

			wpad_index = 0;
			wpad_classic_index = 0;
			pad_index = 0;
		}

		if ((pad_button.button == WPAD_BUTTON_HOME) || (pad_button.button == WPAD_CLASSIC_BUTTON_HOME) || (pad_button.button == PAD_TRIGGER_Z))
			return -1;
	}

	return 0;
}

void SDBootInstaller(void)
{
	char* sdboot_path;
	int ret;

	Enable_DevBoot2();
	ret = SEEPROMClearStep();

	/* [nitr8]: Add a check if clearing the SEEPROM boot2 info regions succeeded */
	if (ret != 0)
	{
		gecko_printf("Aborting installation... (Press any button to continue)\n");

		WaitForPad();
		return;
	}

	if (IsMini())
	{
		printf("Installing SDBoot on a Wii Mini could cause your system to be unusable due to the lack of an SD card slot.\n");
		printf("Press any controller button to continue anyways or press the power button on the console to exit.\n");

		WaitForPad();
	}
	
	printf("Please select the file you want to install... ");
	WaitForPad();
	sdboot_path = FileSelect("/");

	/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
	if (strcmp(sdboot_path, "exit") == 0)
		return;
	else if (strcmp(sdboot_path, "b2m") == 0)
		return;

	HandleInstall(InstallSDBoot(sdboot_path), INSTALL_SD_BOOT);

	printf("\nPress any button to continue.");
	WaitForPad();
}

void NANDBootInstaller(void)
{
	char* nandboot_path;
	char* nandboot_payload_path;
	int ret;

	/* Enable_DevBoot2(); */
	ret = SEEPROMClearStep();
	
	/* [nitr8]: Add a check if clearing the SEEPROM boot2 info regions succeeded */
	if (ret != 0)
	{
		gecko_printf("Aborting installation... (Press any button to continue)\n");

		WaitForPad();
		return;
	}

	printf("Please select the file you want to install... ");
	WaitForPad();
	nandboot_path = FileSelect("/");

	/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
	if (strcmp(nandboot_path, "exit") == 0)
		return;
	else if (strcmp(nandboot_path, "b2m") == 0)
		return;

	printf("Please select the payload you want to install... ");
	WaitForPad();
	nandboot_payload_path = FileSelect("/");

	/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
	if (strcmp(nandboot_payload_path, "exit") == 0)
		return;

	HandleInstall(InstallNANDBoot(nandboot_path, nandboot_payload_path), INSTALL_NAND_BOOT);

	printf("\nPress any button to continue.");
	WaitForPad();
}

void HBCInstaller(void)
{
	HandleInstall(InstallHBC(), INSTALL_HBC);
	printf("\nPress any button to continue.");
	WaitForPad();
}

void Boot2WADInstaller(void)
{
	char* boot2wad_path;
	int ret;

	ret = SEEPROMClearStep();

	/* [nitr8]: Add a check if clearing the SEEPROM boot2 info regions succeeded */
	if (ret != 0)
	{
		gecko_printf("Aborting installation... (Press any button to continue)\n");

		WaitForPad();
		return;
	}

	Enable_DevBoot2();
	printf("\n\n");
	
	printf("Please select the WAD you want to install... ");
	WaitForPad();
	boot2wad_path = FileSelect("/");

	/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
	if (strcmp(boot2wad_path, "exit") == 0)
		return;
	else if (strcmp(boot2wad_path, "b2m") == 0)
		return;

	HandleInstall(InstallWADBoot2(boot2wad_path), INSTALL_WAD);

	ret = SEEPROMClearStep();

	/* [nitr8]: Add a check if clearing the SEEPROM boot2 info regions succeeded */
	if (ret != 0)
		return;

	printf("\nPress any button to continue.");
	WaitForPad();
}

void Boot2BackupInstaller(void)
{
	char* boot2_backup_path;
	int ret;

	ret = SEEPROMClearStep();

	/* [nitr8]: Add a check if clearing the SEEPROM boot2 info regions succeeded */
	if (ret != 0)
	{
		gecko_printf("Aborting installation... (Press any button to continue)\n");

		WaitForPad();
		return;
	}

	Enable_DevFlash();
	printf("\n\n");
	
	printf("Please select the boot2 backup you want to restore... ");
	WaitForPad();
	boot2_backup_path = FileSelect("/");

	/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
	if (strcmp(boot2_backup_path, "exit") == 0)
		return;
	else if (strcmp(boot2_backup_path, "b2m") == 0)
		return;

	HandleInstall(RestoreBoot2Blocks(boot2_backup_path), INSTALL_BACKUP);

	ret = SEEPROMClearStep();

	/* [nitr8]: Add a check if clearing the SEEPROM boot2 info regions succeeded */
	if (ret != 0)
		return;

	printf("\nPress any button to continue.");
	WaitForPad();
}

void Boot2BackupMake(void)
{
	Enable_DevFlash();
	printf("\n\n");

	HandleInstall(BackupBoot2Blocks(BOOT2_BACKUP_PATH), MAKE_BOOT2_BACKUP);
	printf("\nPress any button to continue.");
	WaitForPad();
}

void RestoreNAND(void)
{
	char* nand_backup_path;
	struct Simulation sim;
	int i;

	/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
	pad_t pad_button;

	Enable_DevFlash();
	setMinBlock(8);
	
	printf("Please select the NAND you want to restore... ");
	WaitForPad();
	nand_backup_path = FileSelect("/");

	/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
	if (strcmp(nand_backup_path, "exit") == 0)
		return;
	else if (strcmp(nand_backup_path, "b2m") == 0)
		return;

	for (i = 0; i < (MenuStr_Count + 3); i++)
		printf("\n");

	/* [nitr8]: Allow to break out of this way before we waste worthy time */
	/* printf("This will first run in simulation mode. Press any key to continue."); */
	printf("This will first run in simulation mode.\n");
	printf("Press (B) to abort or any other key to continue.");

	/* [nitr8]: Allow to break out of this way before we waste worthy time */
	/* WaitForPad(); */
	pad_button = WaitForPad();

	/* [nitr8]: Allow to break out of this way before we waste worthy time */
	if ((pad_button.button == WPAD_BUTTON_B) || (pad_button.button == WPAD_CLASSIC_BUTTON_B) || (pad_button.button == PAD_BUTTON_B))
		return;

	sim = flashFileSim(nand_backup_path, 0, 4095);

	if (sim.blocksStatus == NULL)
		ThrowError(errorStrings[ErrStr_MissingFiles]);
				
	printf("\nPress (A) to continue, or any other button to go back to the menu\n");
	
	/* [nitr8]: Reworked - Fix menu bug #1 where button press of "WPAD_BUTTON_B" matches GC "PAD_BUTTON_DOWN" */
	pad_button = WaitForPad();

	/* [nitr8]: Add Classic Controller button mapping */
	/* [nitr8]: Reworked - Fix menu bug #1 where button press of "WPAD_BUTTON_B" matches GC "PAD_BUTTON_DOWN" */
	/* if(WaitForPad() != WPAD_BUTTON_A) */
	if ((pad_button.button != WPAD_BUTTON_A) && (pad_button.button != WPAD_CLASSIC_BUTTON_A) && (pad_button.button != PAD_BUTTON_A))
		return;

	HandleInstall(flashFile(nand_backup_path, 0, 4095, &sim), RESTORE_NAND_BACKUP);
	setMinBlock(0);
	printf("\nPress any key to continue.");
	WaitForPad();
}

void BootSysCheck(void)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int i;
	char boot1Version;
	Boot2Block boot2Block;
	u8 boot2BlockNumbers[6] = {1,2,3,4,7,6};

	Enable_DevFlash();
	
	boot1Version = identifyBoot1();
	
	printf(" [+] Boot1 version: boot1%c\n\n", boot1Version);
	printf(" [+] Boot2 versions: \n\n");
	
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int i=0; i<3; i++){ */
	for (i = 0; i < 3; i++)
	{
		boot2Block = identifyBoot2(i);
		
		if (boot2Block.blockSize == 0)
			continue;
		
		if (boot2Block.blockSize == 1)
			printf("   [-] block %d: ", boot2BlockNumbers[i*2]);
		else
			printf("   [-] blocks %d-%d: ", boot2BlockNumbers[i*2], boot2BlockNumbers[i*2+1]);
		
		printf("%s %s\n", boot2Block.version, boot2Block.bootMiiVer);
	}
	checkBlocks(1,7);
	
	SEEPROMDisplayInfo();
	
	printf("\nPress any key to continue.");
	WaitForPad();
}

void EraseNANDFS(void)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int i;
	
	int ret = 0;
	Boot2Block boot2Block;
	bool hasBoot2V0;

	/* [nitr8]: Added controller button pattern so some of the menu entries don't get
		    executed out of a sudden, like "ERASE NAND" which is just dangerous */
	ret = button_match_pattern();

	if (ret != 0)
		return;

	Enable_DevFlash();

	hasBoot2V0 = false;

	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
	for(int i=0; i<3; i++){ */
	for (i = 0; i < 3; i++)
	{
		boot2Block = identifyBoot2(i);
		
		if (!strcmp(boot2Block.version, "sdboot")){
			hasBoot2V0 = true;
			break;
		}
		
		if (!strcmp(boot2Block.version, "nandboot")){
			hasBoot2V0 = true;
			break;
		}
	}
	
	if (!hasBoot2V0)
		printf("ERROR: you must have either sdboot or nandboot to erase the NAND!\n");
	else
		eraseBlocks(8, 4095);
	
	printf("\nPress any key to continue.");
	WaitForPad();
}

