/* RGD SDBoot Installer */

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <gctypes.h>

#include "prodinfo.h"
#include "errorhandler.h"
#include "errorcodes.h"

#define PROD_INFO_PATH "/title/00000001/00000002/data/setting.txt"
#define PROD_INFO_KEY_SEED 0x73B5DBFA

static char prodInfoBuffer[0x101] __attribute__((aligned(32)));

int GetProdInfoEntry(const char *name, char *buf, int length) {
	char *line = prodInfoBuffer;
	char *delim, *end;
	int slen;
	int nlen = strlen(name);
	
	while(line < (prodInfoBuffer + 0x100) ) {
		delim = strchr(line, '=');
		if(delim && ((delim - line) == nlen) && !memcmp(name, line, nlen)) {
			delim++;
			end = strchr(line, '\r');
			if (!end) end = strchr(line, '\n');
			if(end) {
				slen = end - delim;
				if(slen < length) {
					memcpy(buf, delim, slen);
					buf[slen] = 0;
					return slen;
				} else {
					ThrowError(errorStrings[ErrStr_SettingTooBig]);
				}
			}
		}
		while(line < (prodInfoBuffer + 0x100) && *line++ != '\n');
	}
	ThrowError(errorStrings[ErrStr_SettingInvalid]);
	// NOT REACHED HERE 
	return 0;
}

int OpenProdInfo(char* prodInfoPath) {
	int fd = IOS_Open(prodInfoPath, 1);
	int rv = 0;
	int i = 0;
	u32 prodInfoKey = PROD_INFO_KEY_SEED;
	
	if (fd < 0) return fd;
	memset(prodInfoBuffer, 0, 0x101);
	rv = IOS_Read(fd, prodInfoBuffer, 0x100);
	IOS_Close(fd);
	if(rv != 0x100) ThrowError(errorStrings[ErrStr_SettingInvalid]);
	
	for (i = 0; i < 0x100; i++) {
		prodInfoBuffer[i] ^= prodInfoKey & 0xff;
		prodInfoKey = (prodInfoKey << 1) | (prodInfoKey >> 31);
	}
	return ALL_OK;
}

u8 IsMini( void ) {
	int rv = OpenProdInfo(PROD_INFO_PATH);
	if(rv < 0) return 0;
	static char* buf[17]; // Official internal SC docs say that 16 characters should be allocated for the length.
	rv = GetProdInfoEntry("MODEL", buf, 17);

	if(strncmp("RVL-201", buf, 7)) return 1;
	return 0;
}
