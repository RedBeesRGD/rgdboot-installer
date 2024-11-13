/* RGD SDBoot Installer */

/* [nitr8]: import 'errno' for local function of "scandir(...)" */
#include <errno.h>

/* [nitr8]: Added these */
#include <fat.h>
#include <sys/dir.h>
#include <sdcard/gcsd.h>
#include <sdcard/wiisd_io.h>

#include "filemanager.h"

/* [nitr8]: get rid of warning for implicit declaration of function */
#include "menu.h"

/* [nitr8]: MOVED to "menu.h" */
/*
#define CURSOR "-> "

#define DOWN 0
#define UP 1
*/

/* [nitr8]: Make static */ 
/* u8 entryPosition; */
static u8 entryPosition;

/* [nitr8]: Make static */
/* u8 currentPage, totalPages; */
static u8 currentPage, totalPages;

/* [nitr8]: Make static */
/* const u8 maxEntries = 12; */
static const u8 maxEntries = 12;

/* [nitr8]: Make static */ 
/* struct dirent **dirlist; */
static struct dirent **dirlist;

/* [nitr8]: Make static */ 
/* int nd; */
static int nd;

/* [nitr8]: Make static */
/* struct dirent **filelist; */
static struct dirent **filelist;

/* [nitr8]: Make static */ 
/* int nf; */
static int nf;

/* [nitr8]: Make static */ 
/* char currentDir[1024] = ""; */
static char currentDir[1024] = "";

/* [nitr8]: Add missing scandir(...) function (quick import from https://docs.ros.org/en/melodic/api/stage/html/scandir_8c_source.html) */
static int __scandir(const char *dir, struct dirent ***namelist, int (*select) (const struct dirent *), int (*cmp) (const void *, const void *))
{
	size_t i;
	struct dirent *d;
	int save;
	struct dirent *vnew = NULL;

	DIR *dp = opendir(dir);
	struct dirent **v = NULL;
	size_t vsize = 0;

	if (dp == NULL)
		return -1;

	save = errno;
	errno = 0;

	i = 0;

	while ((d = readdir(dp)) != NULL)
	{
		if (select == NULL || (*select)(d))
		{
			size_t dsize;

			/* Ignore errors from select or readdir */
			errno = 0;

			if (i == vsize)
			{
				 struct dirent **new;

				 if (vsize == 0)
					vsize = 10;
				 else
					vsize *= 2;

				 new = (struct dirent **)realloc(v, vsize * sizeof(*v));

				 if (new == NULL)
					break;

				 v = new;
			}

			dsize = &d->d_name[strlen(d->d_name) + 1] - (char *)d;

			/* [nitr8]: POTENTIAL MEMORY LEAK */
			vnew = (struct dirent *)malloc(dsize);

			if (vnew == NULL)
				break;

			v[i++] = (struct dirent *)memcpy(vnew, d, dsize);
		}
	}

	if (errno != 0)
	{
		save = errno;
		(void)closedir(dp);

		while (i > 0)
			free(v[--i]);

		free(v);
		errno = save;

		return -1;
	}

	(void)closedir(dp);
	errno = save;

	/* Sort the list if we have a comparison function to sort with. */
	if (cmp != NULL)
		qsort(v, i, sizeof(*v), cmp);

	*namelist = v;

	return i;
}

/* [nitr8]: Make static */ 
/* [nitr8]: Most likely, we could also "INLINE" this */ 
/* int dirsort (const struct dirent *a) */
static int dirsort (const struct dirent *a)
{
	return a->d_type == DT_DIR;
}

/* [nitr8]: Make static */ 
/* [nitr8]: Most likely, we could also "INLINE" this */ 
/* int regsort (const struct dirent *a) */
static int regsort (const struct dirent *a)
{
	return a->d_type == DT_REG;
}

/* [nitr8]: Make static */ 
/* char* GetFullFilePath(const char* filename) */
static char* GetFullFilePath(const char* filename)
{
	int len = strlen(currentDir) + strlen(filename) + 1;

	/* [nitr8]: POTENTIAL MEMORY LEAK */
	char* filepath = (char*)malloc(len+1);
	
	strcpy(filepath, currentDir);

	if(strcmp(currentDir, "/"))
		strcat(filepath, "/");

	strcat(filepath, filename);

	return filepath;
}

/* [nitr8]: get rid of warning for implicit declaration of function (moved HERE) */
/* [nitr8]: Make static */ 
/* void PrintFileManager(void) */
static void PrintFileManager(void)
{
	u16 start = currentPage * maxEntries;
	u16 end   = (currentPage + 1) * maxEntries - 1;

	printf("\x1b[6;0H");
	
	printf("Current Directory: %s\n\n", currentDir);
	
	for( ; start < nd && start <= end; start++)
		printf("   <DIR>  %s\n", dirlist[start]->d_name);
	
	for( ; start < nd + nf && start <= end; start++)
		printf("   <FILE> %s\n", filelist[start - nd]->d_name);

	printf("\x1b[4;0H");
}

/* [nitr8]: get rid of warning for implicit declaration of function (moved HERE) */
/* [nitr8]: Make static */ 
/* void PrintFMCursor(void) */
static void PrintFMCursor(void)
{
	u8 location = entryPosition + 8;
	printf("\x1b[%i;0H", location);
	printf("%s", CURSOR);
	printf("\x1b[2;0H");
}

/* [nitr8]: get rid of warning for implicit declaration of function (moved HERE) */
/* [nitr8]: Make static */ 
/* void ChangeDirectory(const char* dirname) */
static void ChangeDirectory(const char* dirname)
{
	if(!strcmp(dirname, "..") && strcmp(currentDir, "/"))
		currentDir[strrchr(currentDir, '/') - currentDir] = '\0';
	else if(strcmp(dirname, "."))
	{
		if(strcmp(currentDir, "/"))
			strcat(currentDir, "/");

		strcat(currentDir, dirname);
	}
	
	if(!strcmp(currentDir, ""))
		strcpy(currentDir, "/"); /* TODO: SD or USB? */
	
	nd = __scandir(currentDir, &dirlist, dirsort, 0);

	if (nd < 0)
		/* [nitr8]: WHAT THE ...??? Returning a VOID POINTER to VOID ??? */
		/* return NULL; */
		return;
	
	nf = __scandir(currentDir, &filelist, regsort, 0);

	if (nf < 0)
		/* [nitr8]: WHAT THE ...??? Returning a VOID POINTER to VOID ??? */
		/* return NULL; */
		return;
	
	entryPosition = 0;
	currentPage   = 0;
	totalPages    = (nf + nd) / maxEntries;

	if(totalPages * maxEntries < nf + nd)
		totalPages++;
	
	ClearScreen();
	PrintFileManager();
	PrintFMCursor();
}

/* [nitr8]: get rid of warning for implicit declaration of function (moved HERE) */
/* [nitr8]: Make static */ 
/* void MoveFM(u8 direction) */
static void MoveFM(u8 direction)
{
	if(direction == UP)
	{
		if(entryPosition > 0)
			entryPosition--;
		else if(currentPage > 0)
		{
			ClearScreen();
			entryPosition = maxEntries - 1;
			currentPage--;
		}
	}
	else if (direction == DOWN)
	{
		if(entryPosition < maxEntries - 1 && entryPosition + currentPage*maxEntries < nf + nd - 1)
			entryPosition++;
		else if(currentPage < totalPages - 1 && entryPosition == maxEntries - 1)
		{
			ClearScreen();
			entryPosition = 0;
			currentPage++;
		}
	}

	//ClearScreen();
	PrintFileManager();
	PrintFMCursor();
}

char* FileSelect(const char* dir)
{
	int index;
	strcpy(currentDir, dir);
	ChangeDirectory(".");

	/* [nitr8]: This should help alot when it comes to restarting things... */
	/*	    Better than having to "hard-reset" the console by holding down POWER every time */
	/* while(1) */
	while(!console_reload)
	{
		/* [nitr8]: Reworked - Fix menu bugs with button matches between a Wiimote and a GC Controller */
		/* switch(WaitForPad()) { */
		pad_t pad_button = WaitForPad();

		switch(pad_button.button)
		{
			case RGDSDB_PAD_BUTTON_UP:

			/* [nitr8]: Add Classic Controller button mapping */
			case WPAD_CLASSIC_BUTTON_UP:

			case WPAD_BUTTON_UP:
				MoveFM(UP);
				break;

			case PAD_BUTTON_DOWN:
				/* [nitr8]: This is the actual bug-fix */
				switch (pad_button.is_gc_controller)
				{
					/* [nitr8]: It's a Wiimote */
					case 0:
						/* [nitr8]: Escape this menu */
						return "b2m";

					/* [nitr8]: It's a GC Controller */
					case 1:
						/* [nitr8]: Step down the menu */
						MoveFM(DOWN);
						break;
				}
				break;

			/* [nitr8]: Add Classic Controller button mapping */
			case WPAD_CLASSIC_BUTTON_DOWN:
				MoveFM(DOWN);
				break;

			case PAD_BUTTON_A:

			/* [nitr8]: Add Classic Controller button mapping */
			case WPAD_CLASSIC_BUTTON_A:

			case WPAD_BUTTON_A:
				index = entryPosition + currentPage * maxEntries;
			
				if(index < nd)
				{	/* Directory */
					ChangeDirectory(dirlist[index]->d_name);
				}
				else if(index < nf + nd)
				{	/* File */
					return GetFullFilePath(filelist[index - nd]->d_name);
				}
				break;

			/* [nitr8]: Done! */
			case PAD_BUTTON_X:
				/* [nitr8]: This is the actual bug-fix */
				switch (pad_button.is_gc_controller)
				{
					/* [nitr8]: It's a Wiimote */
					case 0:
						/* [nitr8]: Step down the menu */
						MoveFM(DOWN);
						break;

					/* [nitr8]: It's a GC Controller */
					case 1:
						/* [nitr8]: Step back the directory tree */
						ChangeDirectory("..");
						break;
				}
				break;

			/* [nitr8]: Add Classic Controller button mapping */
			case WPAD_CLASSIC_BUTTON_MINUS:
				ChangeDirectory("..");
				break;
			
			/* [nitr8]: Add special exit */
			case WPAD_CLASSIC_BUTTON_HOME:
			case WPAD_BUTTON_HOME:
			case PAD_TRIGGER_Z:
				/* [nitr8]: This is the actual bug-fix */
				switch (pad_button.is_gc_controller)
				{
					/* [nitr8]: It's a Wiimote */
					case 0:
						/* [nitr8]: Step down the menu */
						ChangeDirectory("..");
						break;

					/* [nitr8]: It's a GC Controller */
					case 1:
						/* [nitr8]: Return back to main menu */
						console_reload = 1;
						return "b2m";
				}
				break;

			/* [nitr8]: Add special case "b2m" - back to menu */
			case WPAD_CLASSIC_BUTTON_B:
			case PAD_BUTTON_B:
				return "b2m";

			default:
				break;
		}
	}

	/* [nitr8]: Added - used for easier RESET when it comes to restarting the app */
	if (console_reload)
		goto out;

out:

	return "exit";
}

