/* RGD SDBoot Installer */

/* [nitr8]: Added - Why is this missing??? */
/* 	    Basically - trying to write a file into a directory which is non-existent will always FAIL
	    So for a BARE setup once this app was started, ALWAYS create the required directories */
#include <sys/stat.h>

/* [nitr8]: Added */
#include <ogc/machine/processor.h>

#include "tools.h"
#include "errorhandler.h"
#include "errorcodes.h"

/* [nitr8]: Added*/
#include "errorstrings.h"

#include "sha256.h"

/* [nitr8]: Added */
#include "gecko.h"

/* [nitr8]: Added */
#include "boot2.h"

/* [nitr8]: Reworked - Due to fixing the menu bug with controller buttons "WPAD_BUTTON_B" and "PAD_BUTTON_DOWN" which
	    are assigned to the same value, add this here as I have this feeling there's more we need to check on... */
/* #define DEBUG_CONTROLLER	1 */

/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
int console_reload = 0;


/* [nitr8]: Added (used by hexdump support below) */
static char ascii(char s)
{
	if (s < 0x20)
		return '.';

	if (s > 0x7E)
		return '.';

	return s;
}

/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
/* u32 WaitForPad(void) { */
pad_t WaitForPad(void)
{
	/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
	/* u32 ret; */
	pad_t pad_type;

	/* [nitr8]: Even if the button is still held, it will immediately show a release of it */
	WPADData *wpad_data;

	u32 wpadButtons = 0;
	u32 padButtons = 0;

	while (1)
	{
		/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
		int i = 0;

		WPAD_ScanPads();
		PAD_ScanPads();

		/* [nitr8]: Anyway...: Why do you allow this for ALL of the 4 controllers...? */
		/*	    Tools like these should ONLY allow 1 controller after all without 
			    other people interfering this app's behavior - this is dangerous */

		/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
		/* for(int i = 0; i < 4; i++) { */
		/* for (i = 0; i < 4; i++) */
		/* { */
			/* [nitr8]: Even if the button is still held, it will immediately show a release of it */
			wpad_data = WPAD_Data(i);

			wpadButtons |= WPAD_ButtonsDown(i);
			padButtons |= PAD_ButtonsDown(i); 
		/* } */

		if (wpadButtons || padButtons)
		{
#ifdef DEBUG_CONTROLLER
			gecko_printf("wpadButtons (pressed) = %08x / padButtons = %08x / wpadButtons (released) = %08x\n", wpadButtons, padButtons, wpad_data->btns_u);
#endif
			break;
		}

		VIDEO_WaitVSync();
	}

	if (padButtons & PAD_BUTTON_UP)
	{
		padButtons &= ~PAD_BUTTON_UP;
		padButtons |= RGDSDB_PAD_BUTTON_UP;	/* To prevent conflicts */
	}

	/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
	/* ret = 0; */

	if (!wpadButtons)
	{
		/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
		pad_type.button = padButtons;
		pad_type.is_gc_controller = true;

		/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
		/* ret = padButtons; */

		pad_type.is_pressed = padButtons;
	}

	if (!padButtons)
	{
		/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
		pad_type.button = wpadButtons;
		pad_type.is_gc_controller = false;

		/* [nitr8]: Even if the button is still held, it will immediately show a release of it */
		pad_type.is_pressed = wpad_data->btns_d;
		pad_type.is_released = wpad_data->btns_u;

		/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
		/* ret = wpadButtons; */
	}

	/* [nitr8]: Reworked - Due to fixing the menu bugs with button matches between a Wiimote and a GC Controller which
		    are assigned to the same value, add this here as I have this feeling there's more we need to check on... */
#ifdef DEBUG_CONTROLLER
	gecko_printf("\n");
	gecko_printf("pad_type.is_gc_controller = %08x\n", pad_type.is_gc_controller);
	gecko_printf("padButtons                = %08x\n", padButtons);
	gecko_printf("wpadButtons               = %08x\n", wpadButtons);
	gecko_printf("\n");
#endif

	VIDEO_WaitVSync();

	/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
	/* return ret; */
	return pad_type;
}

u8 IsWiiU(void)
{
	if (*(vu16*)0xcd8005a0 == 0xCAFE)
		return 1;

	return 0;
}

u8 IsDolphin(void)
{
	u8 dolphinFlag = 0;
	int fd = IOS_Open("/dev/dolphin", 1);

	if (fd >= 0)
	{
		IOS_Close(fd);
		dolphinFlag = 1;
	}

	IOS_Close(fd);

#ifndef DOLPHIN_CHECK
	if (!dolphinFlag && GetBoot2Version())
		ThrowError(errorStrings[ErrStr_WrongVersion]);

	return 0;
#else
	return dolphinFlag;
#endif
}

void WaitExit(void)
{
	printf("\n\nPress any controller button to exit.");

	WaitForPad();

	exit(0);
}

/* [nitr8]: Renamed */
/* void *alloc(u32 size) */
void *alloc_aligned(u32 size)
{
//	return memalign(32, (size+31)&(~31));
	return memalign(64, size);
}


/* [nitr8]: Renamed and return type changed */
/* u32 filesize(FILE* fp) */
int GetSizeOfFile(FILE* fp)
{
	/* [nitr8]: NO! - Rework this by adding checks */
	/*
	u32 size;

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	rewind(fp);
	*/

	int size = 0;
	int ret = 0;

	/* Jump to the end of the file */
	if (fseek(fp, 0, SEEK_END) != 0)
	{
		/* If it FAILED... */
		gecko_printf("[ERROR]: Couldn't seek to the end of the input file\n");

		/* Set the return code */
		ret = SEEK_ERROR;

		/* Jump to the end of the function */
		goto err;
	}

	/* If it SUCCEEDED... */

	/* Get it's length */
	size = ftell(fp);

	/* Jump back to the start of the file so we can actually read it's data */
	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		/* If it FAILED... */
		gecko_printf("[ERROR]: Couldn't seek to the start of the input file\n");

		/* Set the return code */
		ret = SEEK_ERROR;

		/* Jump to the end of the function */
		goto err;
	}

	ret = size;

err:

	return ret;
}

int CheckFile(FILE* fp, const char *filename)
{
	if (fp == NULL)
	{
		printf("Error: cannot read file %s\n", filename);
		return 0;
	}

	return 1;
}

/* [nitr8]: Remove the size right here */
/* int CheckFileHash(const char *filename, u8 expectedHash[], int expectedSize){ */
int CheckFileHash(const char *filename, u8 expectedHash[])
{
	/* [nitr8]: What the hell is that?! */
	/* BYTE payload[expectedSize]; */

	/*
	 * It definitely ain't "CONSTANT" that way...
	 * You're trying to assign a random length to a "CONSTANT" buffer
	 * This is not supposed to work like that (see below)
	 */

	/* [nitr8]: Now, this would be the right way to make it "CONSTANT":
		#define EXPECTED_SIZE	(3 * RAWBOOT2SIZE)

		u8 payload[EXPECTED_SIZE]; */

	/* [nitr8]: But instead we allocate a buffer by the size of the actual file (see below) */
	u8 *payload_buffer;

	/* [nitr8]: Added */
	int payload_length;

	int bytes_read;
	u8 actualHash[SHA256_BLOCK_SIZE];
	SHA256_CTX ctx;

	/* [nitr8]: Added (return code)
		    Assuming "SUCCESS" */
	int ret = ALL_OK;

	/* Open the file (the MODE should be known to the actual needs) */
	FILE *fp = fopen(filename, "rb");

	/* Check if the file was opened successfully... */
	if (fp == NULL)
	{
		/* If it FAILED... */
		gecko_printf("[ERROR]: Couldn't open file %s for reading\n", filename);

		/* Set the return code */
		ret = MISSING_FILE;

		/* Jump to the end of the function */
		goto err;
	}

	/* If it SUCCEEDED... */

	/* Jump to the end of the file */
	if (fseek(fp, 0, SEEK_END) != 0)
	{
		/* If it FAILED... */
		gecko_printf("[ERROR]: Couldn't seek to the end of file %s\n", filename);

		/* Set the return code */
		ret = SEEK_ERROR;

		/* Jump to the end of the function */
		goto err;
	}

	/* If it SUCCEEDED... */

	/* Get it's length */
	payload_length = ftell(fp);

	/* Jump back to the start of the file so we can actually read it's data */
	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		/* If it FAILED... */
		gecko_printf("[ERROR]: Couldn't seek to the start of file %s\n", filename);

		/* Set the return code */
		ret = SEEK_ERROR;

		/* Jump to the end of the function */
		goto err;
	}

	/* If it SUCCEEDED... */

	/* Allocate a buffer by the length of the file */
	payload_buffer = malloc(payload_length);

	/* If it SUCCEEDED... */
	if (payload_buffer)
	{
		/* Read the data from the file into the buffer by the size of "X" elements */
		/* Usually, 1 element would be good enough by the whole size of the file */
		bytes_read = fread(payload_buffer, 1, payload_length, fp);

		/* Close the file if it's not used any further in the code */
		fclose(fp);

		/* Compare the expected payload length vs. the amount of bytes that was read from the file */
		if (bytes_read != payload_length)
		{
			/* If they do NOT match... */

			/* Print an error message */
			gecko_printf("[ERROR]: Couldn't read %d bytes from file %s\n", payload_length, filename);

			/* Set the return code */
			ret = READ_LENGTH_ERROR;

			/* Jump to the end of the function */
			goto err;
		}

		/* If they DO match... */

		/* Generate HASH */
		sha256_init(&ctx);
		sha256_update(&ctx, payload_buffer, payload_length);
		sha256_final(&ctx, actualHash);

		/* Don't forget to free the allocated buffer once it's not used any further in the code... */
		free(payload_buffer);

		/* Compare the expected hash vs. the calculated hash */
		ret = memcmp(expectedHash, actualHash, SHA256_BLOCK_SIZE);

		/* If the hashes do NOT match... */
		if (ret != ALL_OK)
		{
			u32 i;

			/* Print an error message */
			gecko_printf("[ERROR]: HASH MISMATCH - actual hash: ");

			for (i = 0; i < SHA256_BLOCK_SIZE; i++)
				/* Print an error message */
				gecko_printf("%02x", actualHash[i]);

			/* Print an error message */
			gecko_printf(" / expected hash: ");

			for (i = 0; i < SHA256_BLOCK_SIZE; i++)
				/* Print an error message */
				gecko_printf("%02x", expectedHash[i]);

			/* Print an error message */
			gecko_printf("\n");

			/* Set the return code */
			ret = HASH_MISMATCH;

			/* Jump to the end of the function */
			goto err;
		}

		/* If they DO match - just exit */
	}
	else
	{
		/* If it FAILED... */

		/* Print an error message */
		gecko_printf("[ERROR]: Couldn't allocate %d bytes of buffer for file %s\n", payload_length, filename);

		/* Set the return code */
		ret = ALLOC_ERROR;
	}

err:

	/* Return error code 0 ("SUCCESS" - see above at function entry) */
	return ret;
}

u32 GetBoot2Version(void)
{
         u32 boot2Version = 0;

         if (ES_GetBoot2Version(&boot2Version) < 0)
		ThrowError(errorStrings[ErrStr_BadBoot2Ver]);

         return boot2Version;
}

/* [nitr8]: Added - as this definitely might be very useful */
/* 	    This way, we can spy on buffers and memory */
void hexdump(u32 addr, const void *d, int len)
{
	u8 *data;
	int i, off;

	data = (u8 *)d;

	for (off = 0; off < len; off += 16)
	{
		gecko_printf("%08x  ", addr + off);

		for (i = 0; i < 16; i++)
		{
			if ((i + off) >= len)
				gecko_printf("   ");
			else
				gecko_printf("%02x ", data[off + i]);
		}

		gecko_printf(" ");

		for (i = 0; i < 16; i++)
		{
			if ((i + off) >= len)
				gecko_printf(" ");
			else
				gecko_printf("%c", ascii(data[off + i]));
		}

		gecko_printf("\n");
	}
}

/* [root1024]: Added - simply replaced "gecko_printf" with "printf" */
void hexdump_graphical(u32 addr, const void *d, int len)
{
	u8 *data;
	int i, off;

	data = (u8 *)d;

	for (off = 0; off < len; off += 16)
	{
		printf("%08x  ", addr + off);

		for (i = 0; i < 16; i++)
		{
			if ((i + off) >= len)
				printf("   ");
			else
				printf("%02x ", data[off + i]);
		}

		printf(" ");

		for (i = 0; i < 16; i++)
		{
			if ((i + off) >= len)
				printf(" ");
			else
				printf("%c", ascii(data[off + i]));
		}

		printf("\n");
	}
}


/* [nitr8]: Added - as this definitely might be very useful */
/* 	    "Borrowed" from the HBC's "channelapp" source */
void memstats(int reset)
{
	static u32 min_free = UINT_MAX;
	static u32 temp_free;
	static u32 level;

	if (reset)
		min_free = UINT_MAX;

	_CPU_ISR_Disable(level);

	temp_free = (u32)SYS_GetArena2Hi() - (u32)SYS_GetArena2Lo();

	_CPU_ISR_Restore(level);

	if (temp_free < min_free)
	{
		min_free = temp_free;
		gecko_printf("MEM2 free: %8u\n", min_free);
	}
}

/* [nitr8]: Added to be able and break out of endless loops on app startup */
int exit_on_error(int check_value, int check_amount, char *string, int error_code)
{
	/* [nitr8]: check if we reached about "check_amount" iterations on "check_value" */
	if ((check_value % check_amount) == 0)
	{
		/* [nitr8]: if "check_value" reached amount of "check_amount": */

		/* [nitr8]: print error */
		printf("\n");
		printf("%s took too long - please retry (restart this app)\n", string);
		printf("\n");
		printf("Exiting...\n");

		/* [nitr8]: wait a while for the user to read the message */
		sleep(3);

		/* [nitr8]: return "error_code"  */
		return error_code;
	}

	return 0;
}

/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
void console_reset(void)
{
	console_reload = 1;
}

/* [nitr8]: Added - used for calculation of the ECC data in a NAND block */
u32 swap32(u32 val)
{
	return ((val >> 24) & 0xFF) | ((val << 8) & 0xFF0000) | ((val >> 8) & 0xFF00) | ((val << 24) & 0xFF000000);
}

/* [nitr8]: Added - Why is this missing??? */
/* 	    Basically - trying to write a file into a directory which is non-existent will always FAIL
	    So for a BARE setup once this app was started, ALWAYS create the required directories */
void create_directory(char *dir)
{
	mkdir(dir, 0755);
}

