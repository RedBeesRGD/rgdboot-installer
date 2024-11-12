/* RGD SDBoot Installer */

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <gccore.h>
#include <gctypes.h>

#include "prodinfo.h"
#include "errorhandler.h"
#include "errorcodes.h"

/* [nitr8]: Added*/
#include "errorstrings.h"

/* [nitr8]: Added */
#include "gecko.h"

/* [nitr8]: Added */
#include "tools.h"

#define PROD_INFO_PATH "/title/00000001/00000002/data/setting.txt"
#define PROD_INFO_KEY_SEED 0x73B5DBFA

static char prodInfoBuffer[0x101] ALIGNED(32);

/* [nitr8]: Disable this... (moved into function "IsMini()" below) */
#if 0
/* [nitr8]: Make static */
/* int GetProdInfoEntry(const char *name, char *buf, int length) */
static int GetProdInfoEntry(const char *name, char *buf, int length)
{
	char *delim, *end;
	int slen;
	int nlen = strlen(name);
	char *line = prodInfoBuffer;
	
	while(line < (prodInfoBuffer + 0x100))
	{
		delim = strchr(line, '=');

		if(delim && ((delim - line) == nlen) && !memcmp(name, line, nlen))
		{
			delim++;
			end = strchr(line, '\r');

			if (!end)
				end = strchr(line, '\n');

			if(end)
			{
				slen = end - delim;

				if(slen < length)
				{
					memcpy(buf, delim, slen);
					buf[slen] = 0;
					return slen;
				}
				else
				{
					ThrowError(errorStrings[ErrStr_SettingTooBig]);
				}
			}
		}

		while(line < (prodInfoBuffer + 0x100) && *line++ != '\n');
	}

	ThrowError(errorStrings[ErrStr_SettingInvalid]);

	/* NOT REACHED HERE */
	return 0;
}
#endif

/* [nitr8]: Make static */
/* int OpenProdInfo(char* prodInfoPath) */
static int OpenProdInfo(char* prodInfoPath)
{
	int fd = IOS_Open(prodInfoPath, 1);
	int rv = 0;
	int i = 0;
	u32 prodInfoKey = PROD_INFO_KEY_SEED;
	
	if (fd < 0)
		return fd;

	memset(prodInfoBuffer, 0, 0x101);
	rv = IOS_Read(fd, prodInfoBuffer, 0x100);
	IOS_Close(fd);

	if (rv != 0x100)
		ThrowError(errorStrings[ErrStr_SettingInvalid]);
	
	for (i = 0; i < 0x100; i++)
	{
		prodInfoBuffer[i] ^= prodInfoKey & 0xff;
		prodInfoKey = (prodInfoKey << 1) | (prodInfoKey >> 31);
	}

	return ALL_OK;
}

/* [nitr8]: Disable this... */
#if 0
u8 IsMini(void)
{
	/* [nitr8]: What? - Either array or pointer (depends on you) - but not both... */
	static char* buf[17]; /* Official internal SC docs say that 16 characters should be allocated for the length. */

	int rv = OpenProdInfo(PROD_INFO_PATH);

	if(rv < 0)
		return 0;

	/* [nitr8]: return value of GetProdInfoEntry is never checked... */
	rv = GetProdInfoEntry("MODEL", buf, 17);

	if(!strncmp("RVL-201", buf, 7))
		return 1;

	return 0;
}
#endif

/* [nitr8]: Modified it by adding the code of function "GetProdInfoEntry" right here... */
u8 IsMini(void)
{
	char *delim, *end;
	int slen;
	char* buf;
	char *line = prodInfoBuffer;
	int rv = OpenProdInfo(PROD_INFO_PATH);
	int ret = 0;
	const char *name = "MODEL";
	int nlen = strlen(name);
	int length = 17;

	if (rv < 0)
		return ret;

	/* [nitr8]: like... "WHAT?!" - You can use a buffer pointer or some buffer array but what is this right here ??? */
	/* static char* buf[17]; */ /* Official internal SC docs say that 16 characters should be allocated for the length. */

	/* [nitr8]: allocate the buffer */
	buf = malloc(length);

	while (line < (prodInfoBuffer + 0x100))
	{
		delim = strchr(line, '=');

		if (delim && ((delim - line) == nlen) && !memcmp(name, line, nlen))
		{
			delim++;
			end = strchr(line, '\r');

			if (!end)
				end = strchr(line, '\n');

			if (end)
			{
				slen = end - delim;

				if (slen < length)
				{
					memcpy(buf, delim, slen);
					buf[slen] = 0;
					rv = slen;

					/* [nitr8]: break out */
					goto got_data;
				}
				else
				{
					ThrowError(errorStrings[ErrStr_SettingTooBig]);
				}
			}
		}

		while (line < (prodInfoBuffer + 0x100) && *line++ != '\n');
	}

	ThrowError(errorStrings[ErrStr_SettingInvalid]);

	/* NOT REACHED HERE */
	/* [nitr8]: You really think so...? */
	rv = 0;

/* [nitr8]: jump here */
got_data:

	/* [nitr8]: return value of GetProdInfoEntry is never checked... - why? */
	gecko_printf("GetProdInfoEntry returned %d bytes ('%s')\n", rv, buf);

	if (!strncmp("RVL-201", buf, 7))
		ret = 1;

	/* [nitr8]: free the buffer */
	free(buf);

	return ret;
}

