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

void Terminate();
void *alloc(u32 size);
u32 filesize(FILE* fp);
int CheckFile(FILE* fp, const char *filename);

#endif