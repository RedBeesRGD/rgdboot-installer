/* RGD SDBoot Installer */

#include "menu.h"
#include "errorhandler.h"

/* [nitr8]: get rid of warnings when arrays are not used at all (MOVED) */
/* #include "errorcodes.h" */

#include "tools.h"
#include "installer.h"

/* [nitr8]: Added */
#include "seeprom.h"

/* [nitr8]: Added */
#include "flash.h"

/* [nitr8]: MOVED to "menu.h" */
/*
#define CURSOR "-> "

#define DOWN 0
#define UP 1
*/

/* [nitr8]: get rid of warnings when arrays are not used at all (MOVED)
            plus: add DEBUG stuff */
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
	"Exit to HBC",
	"DEBUG"
};

/* [nitr8]: Added */
static const char *debugStrings[DebugMenuStr_Count] = {
	"Backup SEEPROM boot2 info",
	"Restore SEEPROM boot2 info from backup",
	"Compare SEEPROM boot2 info with backup",
	"Clear SEEPROM boot2 info",
	"Test NAND Blockmaps",
	"Erase NAND blocks 8-4095"
};

/* [nitr8]: get rid of warnings when variables are not used at all (MOVED) */
static bool enableDebugMenu = false;

/* [nitr8]: Make static */
static u8 menuPosition = 0;

/* [nitr8]: Add sub-menu support (this variable is mostly used as a "flipper") */
static int pad_button_was_pressed = 1;

/* [nitr8]: Add sub-menu support */
static int entered_sub_menu = 0;

/* [nitr8]: Add sub-menu support */
static pad_t pad_button;

/* [nitr8]: Add sub-menu support */
static int initial_sub_menu_cleared = 0;

/* [nitr8]: Add sub-menu support */
static int debugMenuPosition = 0;

/* [nitr8]: Make static and moved up here */
static void PrintMenu(void)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int i;

	printf("\x1b[8;0H");

	/* [nitr8]: Add sub-menu support */
	if (entered_sub_menu)
	{
		for (i = 0; i < DebugMenuStr_Count; i++)
		{
			printf("   %s\n", debugStrings[i]);
		}
	}
	else
	{
		/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
		/* for(int i = 0; i < MenuStr_Count; i++) { */
		for (i = 0; i < (MenuStr_Count - (!enableDebugMenu)); i++)
		{
			printf("   %s\n", menuStrings[i]);
		}
#if 0
		if (enableDebugMenu)
		{
			/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
			/* for(int i = 0; i < DebugMenuStr_Count - MenuStr_Count; i++) { */
			for (i = 0; i < (DebugMenuStr_Count - MenuStr_Count); i++)
			{
				printf("   DEBUG - %s\n", debugMenuStrings[i]);
			}
		}
#endif
	}

	printf("\x1b[4;0H");
}

/* [nitr8]: Make static and moved up here */
static void PrintCursor(void)
{
	u8 location = menuPosition + 8;

	printf("\x1b[%i;0H", location);
	printf("%s", CURSOR);
	printf("\x1b[2;0H");
}

/* [nitr8]: Added (this is a SUB-menu) */
static void DoDebugMenu(void)
{
	int clear_screen_forced = 1;

	if (pad_button.is_pressed)
		pad_button_was_pressed ^= 1;

	ClearScreen();

	if (pad_button_was_pressed)
	{
		int i;

		switch (menuPosition)
		{
			case DebugMenuStr_BackupSEEPROMBoot2Info:
				SEEPROMBackupVersion();
				break;

			case DebugMenuStr_RestoreSEEPROMBoot2Info:
				SEEPROMRestoreVersion();
				break;

			case DebugMenuStr_CompareSEEPROMBoot2Info:
				SEEPROMCompareVersion(0);
				SEEPROMCompareVersion(1);
				break;

			case DebugMenuStr_ClearSEEPROMBoot2Info:
				SEEPROMClearVersion();
				break;

			case DebugMenuStr_TestNANDBlockmaps:
				for (i = 0; i < (DebugMenuStr_Count + 3); i++)
					printf("\n");
				TestNANDBlockmaps();
				clear_screen_forced = 0;
				break;

			case DebugMenuStr_EraseNANDFS:
				EraseNANDFS();
				break;

			default:
				gecko_printf("menuPosition out of range!\n");
				break;
		}
	}

	pad_button_was_pressed = 0;

	if (clear_screen_forced)
		ClearScreen();

	PrintMenu();
	PrintCursor();
}

/* [nitr8]: Make static and rename*/
static void DoMainMenu(void)
{
	/* [nitr8]: Add sub-menu support */
	int clear_screen_forced = 1;

	if (pad_button.is_pressed)
		pad_button_was_pressed ^= 1;

	ClearScreen();

	if (pad_button_was_pressed)
	{
		/* [nitr8]: Add sub-menu support */
		int i;

		switch (menuPosition)
		{
			case MenuStr_InstallSDBoot:
				SDBootInstaller();
				break;

			case MenuStr_InstallNANDBoot:
				NANDBootInstaller();
				break;

			case MenuStr_InstallHBC:
				HBCInstaller();
				break;

			case MenuStr_InstallBoot2WAD:
				Boot2WADInstaller();
				break;

			case MenuStr_InstallBoot2Backup:
				Boot2BackupInstaller();
				break;

			case MenuStr_MakeBoot2Backup:
				Boot2BackupMake();
				break;

			case MenuStr_BootSysCheck:
				BootSysCheck();
				break;

			case MenuStr_RestoreNANDBackup:
				RestoreNAND();
				break;

			/* [nitr8]: Add sub-menu support */
			case MenuStr_DoDebugMenu:
				EnterMenu(false);
				break;

			case MenuStr_Credits:
/*				printf("\n\nApp Developers:\n       \x1b[32mroot1024\x1b[37m\n      \x1b31mRedBees\x1b[37m\n\nPayload Developers:\n         nitr8\n\nTesters:\n		\x1b[31mDeadlyFoez\x1b[37m\n\nUsing libruntimeiospatch by Nanolx\n\nAnd Wack0 for making it possible!"); */
				for (i = 0; i < (MenuStr_Count + 3); i++)
					printf("\n");
				printf("This doesn't work yet.\n");
				clear_screen_forced = 0;
				break;

			case MenuStr_Exit:
				exit(0);
				break;

			default:
				gecko_printf("menuPosition out of range!\n");
				break;
		}
	}

	pad_button_was_pressed = 1;

	/* [nitr8]: Add sub-menu support */
	if (clear_screen_forced)
		ClearScreen();

	PrintMenu();
	PrintCursor();
}

/* [nitr8]: Make static */
static void Move(u8 direction)
{
	if (direction == UP)
	{
		if (menuPosition > 0)
		{
			menuPosition--;
		}
		else
		{
			/* [nitr8]: Add sub-menu support */
			if (entered_sub_menu > 0)
			{
				menuPosition = (DebugMenuStr_Count - 1);
			}
			else
			{
				menuPosition = (MenuStr_Count - 1);

				if (!enableDebugMenu && menuPosition == (MenuStr_Count - 1))
					menuPosition--;
			}
		}
	}
	else if (direction == DOWN)
	{
		/* [nitr8]: Add sub-menu support */
		if (entered_sub_menu > 0)
		{
			if (menuPosition < (DebugMenuStr_Count - 1))
			{
				menuPosition++;
			}
			else
			{
				menuPosition = 0;
			}
		}
		else
		{
			if (menuPosition < (MenuStr_Count - 1))
			{
				menuPosition++;

				if (!enableDebugMenu && menuPosition == (MenuStr_Count - 1))
					menuPosition = 0;
			}
			else
			{
				menuPosition = 0;
			}
		}
	}

	PrintMenu();
	PrintCursor();
}

/* [nitr8]: Moved down here */
void ClearScreen(void)
{
	/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
	int i;

	printf("\x1b[6;0H");

	/* [nitr8]: Add sub-menu support */
	if (entered_sub_menu > 0)
	{
		/* [nitr8]: Special flag - only clear the sub-menu once when entering */
		/*	    This should prevent other options from not being triggered */
		if (!initial_sub_menu_cleared)
		{
			menuPosition = 0;
			initial_sub_menu_cleared = 1;
		}

		/* [nitr8]: Also - clear the WHOLE screen */
		/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
		/* for(int i = 0; i < MenuStr_Count + 15; i++) { */
		for (i = 0; i < (DebugMenuStr_Count + 18); i++)
		{
			printf("\33[2K\r\n");
		}
	}
	else
	{
		/* [nitr8]: Also - clear the WHOLE screen */
		/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode */
		/* for(int i = 0; i < MenuStr_Count + 15; i++) { */
		for (i = 0; i < (MenuStr_Count + 18); i++)
		{
			printf("\33[2K\r\n");
		}
	}

	printf("\x1b[6;0H");
}

u8 EnterMenu(bool enableDebug)
{
	int i;

	enableDebugMenu = enableDebug;

	/* [nitr8]: The purpose for this is that it should ALWAYS return the sub-menu entry number */
	for (i = 0; i < MenuStr_Count; i++)
	{
		if (strcmp(menuStrings[i], "DEBUG") == 0)
		{
			debugMenuPosition = i;

			gecko_printf("Found debugMenuPosition entry @ %d\n", debugMenuPosition);

			break;
		}
	}

	PrintMenu();
	PrintCursor();

	/* [nitr8]: This should help alot when it comes to restarting things... */
	/*	    Better than having to "hard-reset" the console by holding down POWER every time */
	/* while(1) */
	while (!console_reload)
	{
		/* [nitr8]: Added */
		memstats(0);

		/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
		/* switch(WaitForPad()) { */
		pad_button = WaitForPad();

		switch (pad_button.button)
		{
			case RGDSDB_PAD_BUTTON_UP:

			/* [nitr8]: Add Classic Controller button mapping */
			case WPAD_CLASSIC_BUTTON_UP:

			case WPAD_BUTTON_UP:
				Move(UP);
				break;

			/* [nitr8]: Add sub-menu support */
			case WPAD_CLASSIC_BUTTON_B:

			/* [nitr8]: Reworked - Fix menu bug #1 where button press of "WPAD_BUTTON_B" matches GC "PAD_BUTTON_DOWN" */
			case PAD_BUTTON_DOWN:
				/* [nitr8]: This is the actual bug-fix */
				switch (pad_button.is_gc_controller)
				{
					/* [nitr8]: It's a Wiimote */
					case 0:
						/* [nitr8]: Add sub-menu support */
						pad_button_was_pressed = 1;

						if (entered_sub_menu)
						{
							entered_sub_menu = 0;
							initial_sub_menu_cleared = 0;
							DoMainMenu();
						}

						break;

					/* [nitr8]: It's a GC Controller */
					case 1:
						/* [nitr8]: Step down the menu */
						Move(DOWN);
						break;
				}

				/* [nitr8]: Added this to fix menu bug #1 where button press of "WPAD_BUTTON_B" matches GC "PAD_BUTTON_DOWN" */
				break;

			/* [nitr8]: Add Classic Controller button mapping */
			case WPAD_CLASSIC_BUTTON_DOWN:

			case WPAD_BUTTON_DOWN:
				Move(DOWN);
				break;

			/* [nitr8]: Add sub-menu support */
			case PAD_BUTTON_B:
				pad_button_was_pressed = 1;

				if (entered_sub_menu)
				{
					entered_sub_menu = 0;
					initial_sub_menu_cleared = 0;
					DoMainMenu();
				}

				break;

			/* [nitr8]: Reworked - Fix menu bug #2 where button press of "WPAD_BUTTON_LEFT" matches GC "PAD_BUTTON_A" */
			case PAD_BUTTON_A:
				/* [nitr8]: This is the actual bug-fix */
				switch (pad_button.is_gc_controller)
				{
					/* [nitr8]: It's a Wiimote */
					case 0:
						/* [nitr8]: Do nothing but head on */
						break;

					/* [nitr8]: It's a GC Controller */
					case 1:
						/* [nitr8]: Enter the selected option (menu) */
						/* [nitr8]: Add sub-menu support */
						if (menuPosition == debugMenuPosition)
						{
							entered_sub_menu = 1;

							if (!pad_button.is_pressed)
								pad_button_was_pressed ^= 1;

							DoDebugMenu();
						}
						else
						{
							if (entered_sub_menu)
							{
								if (!pad_button.is_pressed)
									pad_button_was_pressed ^= 1;

								DoDebugMenu();
							}
							else
							{
								pad_button_was_pressed ^= 1;
								initial_sub_menu_cleared = 0;
								DoMainMenu();
							}
						}

						break;
				}

				/* [nitr8]: Added this to fix menu bug #2 where button press of "WPAD_BUTTON_LEFT" matches GC "PAD_BUTTON_A" */
				break;

			/* [nitr8]: Add Classic Controller button mapping */
			case WPAD_CLASSIC_BUTTON_A:

			case WPAD_BUTTON_A:
				/* [nitr8]: Enter the selected option (menu) */
				/* [nitr8]: Add sub-menu support */
				if (menuPosition == debugMenuPosition)
				{
					entered_sub_menu = 1;

					if (pad_button.is_released)
						pad_button_was_pressed ^= 1;

					DoDebugMenu();
				}
				else
				{
					if (entered_sub_menu)
					{
						if (pad_button.is_released)
							pad_button_was_pressed ^= 1;

						DoDebugMenu();
					}
					else
					{
						pad_button_was_pressed ^= 1;
						initial_sub_menu_cleared = 0;
						DoMainMenu();
					}
				}

				break;

			/* [nitr8]: Add special exit */
			case WPAD_CLASSIC_BUTTON_HOME:
			case WPAD_BUTTON_HOME:
			case PAD_TRIGGER_Z:
				console_reload = 1;
				break;

			default:
				break;
		}
	}

	/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
	if (console_reload)
		goto out;

out:

	return 0;
}

