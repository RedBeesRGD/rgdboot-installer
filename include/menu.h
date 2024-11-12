/* RGD SDBoot Installer */

#ifndef __MENU_H__
#define __MENU_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <wiiuse/wpad.h>

#define CURSOR "-> "

#define DOWN 0
#define UP 1

/* [nitr8]: Extended */
typedef enum {
	MenuStr_InstallSDBoot = 0,
	MenuStr_InstallNANDBoot,
	MenuStr_InstallHBC,
	MenuStr_InstallBoot2WAD,
	MenuStr_InstallBoot2Backup,
	MenuStr_MakeBoot2Backup,
	MenuStr_BootSysCheck,
	MenuStr_RestoreNANDBackup,
	MenuStr_Credits,
	MenuStr_Exit,

	/* [nitr8]: Added (this is a SUB-menu) */
	MenuStr_DoDebugMenu,

	MenuStr_Count	/* Number of values supported by this enum. */
} MenuStr;

#if 0
typedef enum {
	DebugMenuStr_EraseNANDFS = MenuStr_Count,
	DebugMenuStr_Count	/* Number of values supported by this enum. */
} DebugMenuStr;
#endif

/* [nitr8]: Added */
typedef enum {
	DebugMenuStr_BackupSEEPROMBoot2Info = 0,
	DebugMenuStr_RestoreSEEPROMBoot2Info,
	DebugMenuStr_CompareSEEPROMBoot2Info,
	DebugMenuStr_ClearSEEPROMBoot2Info,
	DebugMenuStr_TestNANDBlockmaps,
	DebugMenuStr_EraseNANDFS,
	DebugMenuStr_Count
} debugMenuStr;

/* [nitr8]: get rid of warnings when arrays are not used at all (MOVED)
static const char *menuStrings[MenuStr_Count] = {
	"Install SDBoot",
	"Install NANDBoot",
	"Install HBC",
	"Install a boot2 WAD",
	"Restore a boot2 backup",
	"Perform a boot2 backup",
	"Boot SysCheck",
	"Restore a NAND backup",
	"View Credits",
	"Exit to HBC"
};

   [nitr8]: get rid of warnings when arrays are not used at all
static const char *debugMenuStrings[DebugMenuStr_Count] = {
	"Erase NAND blocks 8-4095"
};

   [nitr8]: get rid of warnings when variables are not used at all (MOVED)
static bool enableDebugMenu = false;
*/

/* [nitr8]: Added */
#ifdef __cplusplus
extern "C" {
#endif

void ClearScreen(void);

/* [nitr8]: Make static */
/* void EnterOption(void); */

/* [nitr8]: Make static */
/* void PrintCursor(void); */

/* [nitr8]: Make static */
/* void Move(u8 direction); */

/* [nitr8]: Add sub-menu support */
/* void PrintMenu(void); */

/* [nitr8]: Make static */
/* void PrintMenu(int which); */

u8 EnterMenu(bool enableDebug);

/* [nitr8]: Added */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENU_H__ */

