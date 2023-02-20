/* RGD SDBoot Installer */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <wiiuse/wpad.h>

#define RGDSDB_PAD_BUTTON_UP 0x777

#ifndef TOOLS_H_
#define TOOLS_H_

u32 WaitForPad( void );
u8 IsDolphin( void );
u8 IsWiiU( void );
void WaitExit( void );
void *alloc(u32 size);
u32 filesize(FILE* fp);
int CheckFile(FILE* fp, const char *filename);
u32 GetBoot2Version( void );

#endif
