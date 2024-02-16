/* RGD SDBoot Installer */

#include "filemanager.h"

#define CURSOR "-> "

#define DOWN 0
#define UP 1

u8 entryPosition;
u8 currentPage, totalPages;
const u8 maxEntries = 12;

struct dirent **dirlist;
int nd;
struct dirent **filelist;
int nf;

char currentDir[1024] = "";


int dirsort (const struct dirent *a){
	return a->d_type == DT_DIR;
}
int regsort (const struct dirent *a){
	return a->d_type == DT_REG;
}

char* GetFullFilePath(const char* filename){
	int len = strlen(currentDir) + strlen(filename) + 1;
	char* filepath = (char*)malloc(len+1);
	
	strcpy(filepath, currentDir);
	if(strcmp(currentDir, "/"))
		strcat(filepath, "/");
	strcat(filepath, filename);
	
	return filepath;
}

char* FileSelect(const char* dir){
	int index;
	strcpy(currentDir, dir);
	ChangeDirectory(".");

	while(1) {
		switch(WaitForPad()) {
			case RGDSDB_PAD_BUTTON_UP:
			case WPAD_BUTTON_UP:
				MoveFM(UP);
				break;
			case PAD_BUTTON_DOWN:
			case WPAD_BUTTON_DOWN:
				MoveFM(DOWN);
				break;
			case PAD_BUTTON_A:
			case WPAD_BUTTON_A:
			
				index = entryPosition + currentPage * maxEntries;
			
				if(index < nd){ // Directory
					ChangeDirectory( dirlist[index]->d_name );
				}
				else if(index < nf + nd){ // File
					return GetFullFilePath( filelist[index - nd]->d_name );
				}
				break;
			
			//case PAD_BUTTON_B: // TODO: FIX THIS!
			case WPAD_BUTTON_1:
				ChangeDirectory("..");
				break;
			
			default:
				break;
		}
	}
}

void ChangeDirectory(const char* dirname){
	if(!strcmp(dirname, "..") && strcmp(currentDir, "/"))
		currentDir[strrchr(currentDir, '/') - currentDir] = '\0';
	else if(strcmp(dirname, ".")){
		if(strcmp(currentDir, "/"))
			strcat(currentDir, "/");
		strcat(currentDir, dirname);
	}
	
	if(!strcmp(currentDir, ""))
		strcpy(currentDir, "/"); // TODO: SD or USB?
	
	nd = scandir(currentDir, &dirlist, dirsort, 0);
	if (nd < 0)
		return NULL;
	
	nf = scandir(currentDir, &filelist, regsort, 0);
	if (nf < 0)
		return NULL;
	
	entryPosition = 0;
	currentPage   = 0;
	totalPages    = (nf + nd) / maxEntries;
	if(totalPages * maxEntries < nf + nd)
		totalPages++;
	
	ClearScreen();
	PrintFileManager();
	PrintFMCursor();
}

void PrintFileManager() {
	u16 start = currentPage * maxEntries;
	u16 end   = (currentPage + 1) * maxEntries - 1;

	printf("Current Directory: %s\n\n", currentDir);
	
	printf("\x1b[8;0H");
	
	for( ; start < nd && start <= end; start++)
		printf("   <DIR>  %s\n", dirlist[start]->d_name);
	
	for( ; start < nd + nf && start <= end; start++)
		printf("   <FILE> %s\n", filelist[start - nd]->d_name);

	printf("\x1b[4;0H");
}

void PrintFMCursor( void ) {
	u8 location = entryPosition + 8;
	printf("\x1b[%i;0H", location);
	printf("%s", CURSOR);
	printf("\x1b[2;0H");
}

void MoveFM(u8 direction) {
	if(direction == UP) {
		if(entryPosition > 0)
			entryPosition--;
		else if(currentPage > 0){
			ClearScreen();
			entryPosition = maxEntries - 1;
			currentPage--;
		}
		
	}  else if (direction == DOWN) {
		if(entryPosition < maxEntries - 1 && entryPosition + currentPage*maxEntries < nf + nd - 1)
			entryPosition++;
		else if(currentPage < totalPages - 1 && entryPosition == maxEntries - 1){
			ClearScreen();
			entryPosition = 0;
			currentPage++;
		}
	}

	PrintFileManager();
	PrintFMCursor();
}
