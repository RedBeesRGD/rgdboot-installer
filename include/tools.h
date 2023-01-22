#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <wiiuse/wpad.h>

#ifndef TOOLS_H_
#define TOOLS_H_

u32 getInput();
void terminate();
void setup();
void *alloc(u32 size);
u32 filesize(FILE* fp);
int checkFile(FILE* fp, const char *filename);

#endif