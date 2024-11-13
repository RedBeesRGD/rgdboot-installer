/* RGD SDBoot Installer */

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <gctypes.h>
#include <debug.h>

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
#include "flash.h"
#include "version.h"
#include "haxx.h"

#include "jpgogc.h"

/* [nitr8]: For use with JPEG, change it from a void pointer to an unsigned int pointer */
/* static void *xfb = NULL; */
static u32 *xfb = NULL;

static GXRModeObj *rmode = NULL;

/* [nitr8]: Add a "/dev/flash" lock counter so we don't have to restart the app every time */
static int dev_flash_access_count = 0;

/* [nitr8]: Add a counter for this one as well as fatInitDefault() may take some time */
static int fat_init_count = 0;

/* [nitr8]: Added - used by SEEPROM code */
int sd_initialized = 0;

/* [nitr8]: Add nice and shiny RGD logo */
extern char bg_jpg[];
extern int bg_jpg_size;

static void display_jpeg(JPEGIMG jpeg, int x, int y)
{
    unsigned int *jpegout = (unsigned int *)jpeg.outbuffer;

    int i, j;
    int height = jpeg.height;
    int width = jpeg.width / 2;

    for (i = 0; i <= width; i++)
        for (j = 0; j <= height - 2; j++)
            xfb[(i + x) + 320 * (j + 16 + y)] = jpegout[i + width * j];

    free(jpeg.outbuffer);
}

int main(int argc, char **argv)
{
	/* [nitr8]: Add check on /dev/flash lock release and filesystem initialization timeout */
	int ret = 0;
	bool enableDebug = false;

	/* [nitr8]: Add nice and shiny RGD logo */
	JPEGIMG about;

/* [nitr8]: Add support for realtime debugging using a USB-Gecko */
#ifdef _DEBUG
	DEBUG_Init(GDBSTUB_DEVICE_USB, GDBSTUB_DEF_CHANNEL);

	/* [nitr8]: Set breakpoint here */
	//_break();
#endif

	//gecko_printf("%08x\n", ipc_initialize());

	/* [nitr8]: Add nice and shiny RGD logo */
	memset(&about, 0, sizeof(JPEGIMG));
	about.inbuffer = bg_jpg;
	about.inbufferlength = bg_jpg_size;
	JPEG_Decompress(&about);

	VIDEO_Init();
	WPAD_Init();
	PAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	/* [nitr8]: Add nice and shiny RGD logo */
        display_jpeg(about, 0, 344);

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (rmode->viTVMode&VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	/* [nitr8]: DEBUG */
//	IOS_ReloadIOS(236);

	/* [nitr8]: This should help alot when it comes to restarting things... */
	/*	    Better than having to "hard-reset" the console by holding down POWER every time */
	SYS_SetResetCallback(console_reset);

	printf("\x1b[2;0H");
	
	/* Get Bus Access (disable AHBPROT) */
	Haxx_GetBusAccess();
	
	gecko_init(1);
	
	if (!AHBPROT_DISABLED)
	{ 
		ThrowError(errorStrings[ErrStr_NeedPerms]);
	}
	
//	Fix_ES_ImportBoot();
	Enable_DevFlash();
	Restore_Trucha();
	
	if (IsDolphin())
	{
		ThrowError(errorStrings[ErrStr_InDolphin]);
	}
	else if (IsWiiU())
	{
		ThrowError(errorStrings[ErrStr_InCafe]);
	}

	/* [nitr8]: Disabled... */

	if(!fatInitDefault()){
		ThrowError(errorStrings[ErrStr_SDCard]);
	}

	/* [nitr8]: Instead, loop until the filesystem was initialized */
	/*while (!sd_initialized)
	{
		sd_initialized = fatInitDefault();
		fat_init_count++;

		ret = exit_on_error(fat_init_count, 2000, "fatInitDefault()", -1);

		if (ret != 0)
			return ret;
	}*/

	/* [nitr8]: Added - Why is this missing??? */
	/* 	    Basically - trying to write a file into a directory which is non-existent will always turn into a DSI exception interrupt
		    So for a BARE setup once this app was started, ALWAYS create the required directories */
	create_directory("boot2");

#ifndef NO_DOLPHIN_CHECK
	/* [nitr8]: Disabled... */
/*
	if(NANDFlashInit() < 0){
		ThrowError(errorStrings[ErrStr_DevFlashErr]);
	}
*/
	/* [nitr8]: Instead, loop until we get a release from a locked access to /dev/flash */
	while (NANDFlashInit() < 0)
	{
		dev_flash_access_count++;

		ret = exit_on_error(dev_flash_access_count, 2000, "NANDFlashInit()", -2);

		if (ret != 0)
			return ret;
	}
#endif

	gecko_printf("Took %d iterations until filesystem was available\n", fat_init_count);
	gecko_printf("Took %d iterations until /dev/flash was available\n", dev_flash_access_count);

	if (argc == 2)
		enableDebug = (strcmp(argv[1], "debug") == 0) ? true : false;

	enableDebug = true;

	printf("RGD SDBoot Installer build %s - by \x1b[32mroot1024\x1b[37m, \x1b[31mRedBees\x1b[37m, \x1b[31mDeadlyFoez\x1b[37m\nraregamingdump.ca", buildNumber);
	printf("\nCurrent boot2 version: %i", GetBoot2Version());
	printf("\tCurrent IOS version: IOS%i v%i\n\n", IOS_GetVersion(), IOS_GetRevision());
	
	/* [nitr8]: At the same time - mark that as a high risk warning! */
	/* printf("\nWARNING: PLEASE READ THIS CAREFULLY!\n\n"); */
	printf("\n\x1b[31mWARNING: PLEASE READ THIS CAREFULLY!\x1b[37m\n\n");
	printf("THIS IS BETA SOFTWARE. AS SUCH, IT CARRIES A HIGH RISK OF BRICKING THE\nCONSOLE.\n\n");

	/* [nitr8]: Regarding warning: don't forget about the SEEPROM here... */
	/* [nitr8]: At the same time - mark that as a high risk warning! */
	/* [nitr8]: Make room for an additional intro logo by removing unnecessary line feeds */
	printf("\x1b[31mTHIS TOOL DIRECTLY WRITES TO BOOT1 AND BOOT2 AS WELL AS THE SEEPROM.\x1b[37m\n\n");

	printf("IT'S ADVISED YOU USE THIS TOOL ONLY IF YOU HAVE AN EXTERNAL NAND FLASHER.\n");
	printf("BY CONTINUING, YOU ACCEPT THAT YOU USE THIS PROGRAM AT YOUR OWN RISK.\n");
	printf("THE AUTHORS CANNOT BE HELD RESPONSIBLE TO ANY DAMAGE THIS TOOL MAY CAUSE.\n");
	printf("IF YOU DON'T AGREE TO THESE TERMS, PLEASE QUIT THIS PROGRAM IMMEDIATELY.\n\n");

	printf("Press (A) to continue, or (HOME) / (Z) to quit and return to the HBC.\n");

	
	/* [nitr8]: This should help alot when it comes to restarting things... */
	/*	    Better than having to "hard-reset" the console by holding down POWER every time */
	/* while(1) */
	while (!console_reload)
	{
		/* [nitr8]: Added */
		memstats(0);

		/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
		/* switch(WaitForPad()) { */
		pad_t pad_button = WaitForPad();

		/* [nitr8]: Much more simplified... */
		switch (pad_button.button)
		{
			/* [nitr8]: Add Classic Controller button mapping */
			case WPAD_CLASSIC_BUTTON_A:

			/* [nitr8]: Add forgotten Wiimote button */
			case WPAD_BUTTON_A:

			case PAD_BUTTON_A:
				ClearScreen();
				EnterMenu(enableDebug);

				/* [nitr8]: NOT REACHED HERE */
				break;

			/* [nitr8]: Add special exit */
			case WPAD_CLASSIC_BUTTON_HOME:
			case WPAD_BUTTON_HOME:
			case PAD_TRIGGER_Z:
				console_reload = 1;
				break;

			/* [nitr8]: Add default case */
			default:
				if (!console_reload)
					continue;
				else
					break;
		}
	}

	/* [nitr8]: You really think so...? */
	return 0; /* NOT REACHED HERE */
}

