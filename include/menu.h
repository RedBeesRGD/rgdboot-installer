/* RGD SDBoot Installer */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <wiiuse/wpad.h>

#ifndef MENU_H_
#define MENU_H_

typedef enum {
	MenuStr_InstallSDBoot = 0,
	MenuStr_InstallNANDBoot,
	MenuStr_InstallBoot2WAD,
	MenuStr_InstallBoot2Backup,
	MenuStr_MakeBoot2Backup,
	MenuStr_Credits,
	MenuStr_Exit,
	MenuStr_Count	// Number of values supported by this enum.
} MenuStr;

static const char *menuStrings[MenuStr_Count] = {
	"Install SDBoot",
	"Install NANDBoot",
	"Install a boot2 WAD",
	"Install a boot2 backup",
	"Perform a boot2 backup",
	"View Credits",
	"Exit to HBC"
};

void ClearScreen( void );
void EnterOption( void );
void PrintCursor( void );
void Move(u8 direction);
void PrintMenu( void );
u8 EnterMenu( void );

#endif
