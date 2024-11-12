/* RGD SDBoot Installer */

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <wiiuse/wpad.h>

/* [nitr8]: Added */
#include "gecko.h"

#define RGDSDB_PAD_BUTTON_UP 0x777

/* [nitr8]: Disables warning about unused function parameters where desired */
#define UNUSED(x)	((x) = (x))

/* [nitr8]: Added */
#ifndef UINT_MAX
#define UINT_MAX	((u32)((s32) - 1))
#endif

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
#define DATA_CHUNK_LEN	0x200

/* [nitr8]: Added - at least used for calculation of the ECC data in a NAND block but it's used everywhere in the code */
#define ALIGNED(x)	__attribute__((aligned(x)))

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
#define PACKED		__attribute__((packed))

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
#define MEM2_BSS	__attribute__((section(".bss.mem2")))

/* [nitr8]: Added - Fix menu bugs with button matches between a Wiimote and a GC Controller */
typedef struct
{
	u32 button;
	u32 is_pressed;
	u32 is_released;
	bool is_gc_controller;
} pad_t;

/* [nitr8]: Added */
#ifdef __cplusplus
extern "C" {
#endif

/* [nitr8]: Reworked: Fix menu bug where button press of "WPAD_BUTTON_B" matches GC "PAD_BUTTON_DOWN" */
/* u32 WaitForPad(void); */
pad_t WaitForPad(void);

u8 IsDolphin(void);
u8 IsWiiU(void);
void WaitExit(void);

/* [nitr8]: Renamed */
/* void *alloc(u32 size); */
void *alloc_aligned(u32 size);

/* [nitr8]: Renamed and return type changed */
/* u32 filesize(FILE* fp); */
int GetSizeOfFile(FILE* fp);

int CheckFile(FILE* fp, const char *filename);

/* [nitr8]: remove variable expectedSize size right here and move it's amount directly into CheckFileHash itself */
/* int CheckFileHash(const char *filename, u8 expectedHash[], int expectedSize); */
int CheckFileHash(const char *filename, u8 expectedHash[]);

u32 GetBoot2Version(void);

/* [nitr8]: Added */
void hexdump(u32 addr, const void *d, int len);

/* [nitr8]: Added */
void memstats(int reset);

/* [nitr8]: Added */
int exit_on_error(int check_value, int check_amount, char *string, int error_code);

/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
void console_reset(void);

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
u32 swap32(u32 val);

/* [nitr8]: Added - Why is this missing??? */
/* 	    Basically - trying to write a file into a directory which is non-existent will always FAIL
	    So for a BARE setup once this app was started, ALWAYS create the required directories */
void create_directory(char *dir);

/* [nitr8]: Added */
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* [nitr8]: Added */
extern int sd_initialized;

/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
extern int console_reload;

#endif /* __TOOLS_H__ */

