/* This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2.0.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License 2.0 for more details.

   Copyright (C) 2010		Joseph Jordan <joe.ftpii@psychlaw.com.au>
   Copyright (C) 2012-2013	damysteryman
   Copyright (C) 2012-2015	Christopher Bratusek <nano@jpberlin.de>
   Copyright (C) 2013		DarkMatterCore
   Copyright (C) 2014		megazig
   Copyright (C) 2015		FIX94 */

#include <gccore.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <string.h>

#include "runtimeiospatch.h"

/* [nitr8]: Added */
#include "hollywood.h"

/* [nitr8]: Added */
#include "gecko.h"

/* [nitr8]: Added */
#include "tools.h"

/* [nitr8]: Added */
#define DEBUG_RUNTIME_IOS_PATCH	1

static const u8 es_set_ahbprot_old[] = { 0x68, 0x5B, 0x22, 0xEC, 0x00, 0x52, 0x18, 0x9B, 0x68, 0x1B, 0x46, 0x98, 0x07, 0xDB };
static const u8 es_set_ahbprot_patch[]   = { 0x01 };

/* The good old Trucha Bug!!! */
static const u8 hash_old[] = { 0x20, 0x07, 0x23, 0xA2 };
static const u8 hash_patch[] = { 0x00 };
static const u8 new_hash_old[] = { 0x20, 0x07, 0x4B, 0x0B };

/* These patches are required to fix a bug where ES_ImportBoot would
   complain about being unable to downgrade boot2, even after
   setting the boot2 version to zero. UNTESTED!!! */
static const u8 es_importboot_old[] = {0x68, 0x5a, 0x9b, 0x1e, 0x42, 0x9a, 0xd2, 0x01};
static const u8 es_importboot_new[] = {0x68, 0x5a, 0x9b, 0x1e, 0x42, 0x9a, 0xe0, 0x01};


/* Special thanks to nitr8 for the /dev/flash access patch!
 * Slightly modified to make it compatible for a few more IOS versions.
 * It should work on the following IOS versions:
 * IOS9-64-v1034
 * IOS12-64-v526
 * IOS13-64-v1032
 * IOS14-64-v1032
 * IOS15-64-v1032
 * IOS17-64-v1032
 * IOS21-64-v1039
 * IOS22-64-v1294
 * IOS28-64-v1807
 * IOS31-64-v3608
 * IOS33-64-v3608
 * IOS34-64-v3608
 * IOS35-64-v3608
 * IOS36-64-v3351
 * IOS36-64-v3608
 * IOS38-64-v3610
 * IOS38-64-v3867
 * IOS38-64-v4123
 * IOS38-64-v4124
   This patch only works on IOS versions that formerly had /dev/flash access enabled. */
//static const u8 dev_flash_old[] = { 0xa7, 0x28, 0x00, 0xd1, 0x02, 0x23, 0x00 };
//static const u8 dev_flash_new[] = { 0xa7, 0x28, 0x00, 0xd1, 0x02, 0x23, 0x01 };
static const u8 dev_flash_old[] = { 0x28, 0x00, 0xd1, 0x02, 0x23, 0x00, 0x46, 0x98 };
static const u8 dev_flash_new[] = { 0x28, 0x00, 0xd1, 0x02, 0x23, 0x01, 0x46, 0x98 };

/* New /dev/flash patch that should work on every other IOS, by root1024.
 * WARNING: this has the side effect of making /dev/boot2 inaccessible!
 * It should work on the following IOS versions:
 * IOS56-64-v5661
 * IOS56-64-v5662
 * IOS57-64-v5661
 * IOS57-64-v5918
 * IOS57-64-v5919
 * IOS58-64-v6175
 * IOS58-64-v6176
 * IOS60-64-v6174
 * IOS61-64-v5662
 * IOS62-64-v6430
 * IOS70-64-v6687
 * IOS80-64-v6943
 * IOS80-64-v6944
 * If you want to access /dev/boot2, you NEED to UNDO this patch! */
static const u8 dev_flash_old1[] = { 0x66, 0x73, 0x00, 0x00, 0x62, 0x6f, 0x6f, 0x74, 0x32 };
static const u8 dev_flash_new1[] = { 0x66, 0x73, 0x00, 0x00, 0x66, 0x6C, 0x61, 0x73, 0x68 };

static const u8 dev_flash_old2[] = { 0xd0, 0x02, 0x20, 0x01, 0x42, 0x40, 0xe0, 0x08, 0xf7, 0xfa, 0xfc, 0xb7 };
static const u8 dev_flash_new2[] = { 0xe0, 0x02, 0x20, 0x01, 0x42, 0x40, 0xe0, 0x08, 0xf7, 0xff, 0xfd, 0xeb };

/* [nitr8]: Added */
/* WARNING: Making use of call to "Disable_FlashECCCheck" MUST A-L-W-A-Y-S ENSURE that it's */
/* 	    being turned back on afterwards BEFORE anything else continues to read data from NAND !!!*/
static const u8 flash_ecc_check_old[] = { 0xF0, 0x02, 0xFF, 0x20, 0x25, 0x0B, 0x42, 0x6D, 0xE0, 0x02, 0x25, 0x0C };

/* [nitr8]: Added */
/* WARNING: Making use of call to "Disable_FlashECCCheck" MUST A-L-W-A-Y-S ENSURE that it's */
/* 	    being turned back on afterwards BEFORE anything else continues to read data from NAND !!!*/
static const u8 flash_ecc_check_new[] = { 0xF0, 0x02, 0xFF, 0x20, 0x25, 0x0B, 0x42, 0x6D, 0xE0, 0x02, 0x25, 0x00 };

/* [nitr8]: Moved up here */
static inline void disable_memory_protection(void)
{
	write32(MEM_PROT, read32(MEM_PROT) & 0x0000FFFF);
}

/* [nitr8]: Make static */
/* void TextColor(u8 color, u8 bold) */
static void TextColor(u8 color, u8 bold)
{
	#if 0
	/* Set foreground color */

	/* [nitr8]: fix warning about signed int <-> unsigned int against string format */
	/* printf("\x1b[%u;%um", color + 30, bold); */
	printf("\x1b[%d;%um", color + 30, bold);

	fflush(stdout);
	#endif
}

static u8 apply_patch(const char *name, const u8 *old, u32 old_size, const u8 *patch, size_t patch_size, u32 patch_offset, bool verbose)
{
	u8 *location;
	u8 *ptr_start = (u8*)*((u32*)0x80003134), *ptr_end = (u8*)0x94000000;
	u8 found = 0;

	if (verbose)
	{
		TextColor(7,1);
		gecko_printf("\t\t    Patching %-30s", name);
	}

	location = NULL;

	while (ptr_start < (ptr_end - patch_size))
	{
		if (!memcmp(ptr_start, old, old_size))
		{
			u32 i;
			u8 *start;

			found++;
			location = ptr_start + patch_offset;
/* [nitr8]: Added */
#if DEBUG_RUNTIME_IOS_PATCH
			gecko_printf("found patch @ 0x%08x\n", location);
			hexdump(0, location, 0x20);
#endif
			start = location;

			for (i = 0; i < patch_size; i++)
			{
				*location++ = patch[i];
			}

			DCFlushRange((u8 *)(((u32)start) >> 5 << 5), (patch_size >> 5 << 5) + 64);
			ICInvalidateRange((u8 *)(((u32)start) >> 5 << 5), (patch_size >> 5 << 5) + 64);
		}

		ptr_start++;
	}

	if (verbose)
	{
		if (found)
		{
			TextColor(2, 1);
			gecko_printf("\t\t patched\n");
		}
		else
		{
			TextColor(1, 1);
			gecko_printf("\t\t not patched\n");
		}
	}

	return found;
}

/* [nitr8]: Unused */
/* [nitr8]: Moved up here */
#if 0
/* [nitr8]: Make static */
/* s32 IosPatch_AHBPROT(bool verbose) */
static s32 IosPatch_AHBPROT(bool verbose)
{
	if (AHBPROT_DISABLED)
	{
		s32 ret;

		disable_memory_protection();

		ret = apply_patch("es_set_ahbprot", es_set_ahbprot_old, sizeof(es_set_ahbprot_old), es_set_ahbprot_patch, sizeof(es_set_ahbprot_patch), 25, verbose);

		if (ret)
			return ret;
		else
			return ERROR_PATCH;
	}

	return ERROR_AHBPROT;
}

/* [nitr8]: Make static */
/* s32 IosPatch_RUNTIME(bool verbose) */
static s32 IosPatch_RUNTIME(bool verbose)
{
	if (AHBPROT_DISABLED)
	{
		s32 count = 0;

		disable_memory_protection();

		if(verbose)
			printf("\t\t\n\n\n\n\n");

		if(verbose)
		{
			TextColor(6, 1);
			printf("\t>> Applying standard Wii patches:\n");
		}

		count += apply_patch("ES_ImportBoot", es_importboot_old, sizeof(es_importboot_old), es_importboot_new, sizeof(es_importboot_new), 0, verbose);
		count += apply_patch("/dev/flash (old)", dev_flash_old, sizeof(dev_flash_old), dev_flash_new, sizeof(dev_flash_new), 0, verbose);
		//count += apply_patch("/dev/flash (new, part1)", dev_flash_old1, sizeof(dev_flash_old1), dev_flash_new1, sizeof(dev_flash_new1), 0, verbose);
		//count += apply_patch("/dev/flash (new, part2)", dev_flash_old2, sizeof(dev_flash_old2), dev_flash_new2, sizeof(dev_flash_new2), 0, verbose);
		
		return count;
	}

	return ERROR_AHBPROT;
}

s32 IosPatch_FULL(bool verbose, int IOS)
{
	s32 ret = 0;
	s32 xret = 0;

	if (AHBPROT_DISABLED)
		ret = IosPatch_AHBPROT(verbose);
	else
		return ERROR_AHBPROT;

	if (ret)
	{
		IOS_ReloadIOS(IOS);
		xret = IosPatch_RUNTIME(verbose);
	}
	else
	{
		xret = ERROR_PATCH;
	}

	return xret;

}
#endif

s32 Fix_ES_ImportBoot(void)
{
	if (AHBPROT_DISABLED)
	{
		s32 count = 0;

		disable_memory_protection();
		count += apply_patch("ES_ImportBoot", es_importboot_old, sizeof(es_importboot_old), es_importboot_new, sizeof(es_importboot_new), 0, true);
		
		return count;
	}

	return ERROR_AHBPROT;
}

/* [nitr8]: Added */
/* WARNING: Making use of call to "Disable_FlashECCCheck" MUST A-L-W-A-Y-S ENSURE that it's */
/* 	    being turned back on afterwards BEFORE anything else continues to read data from NAND !!!*/
s32 Disable_FlashECCCheck(void)
{
	if (AHBPROT_DISABLED)
	{
		s32 count = 0;

		disable_memory_protection();
		count += apply_patch("Flash ECC Check (disable)", flash_ecc_check_old, sizeof(flash_ecc_check_old), flash_ecc_check_new, sizeof(flash_ecc_check_new), 0, true);
		
		return count;
	}

	return ERROR_AHBPROT;
}

/* [nitr8]: Added */
/* WARNING: Making use of call to "Disable_FlashECCCheck" MUST A-L-W-A-Y-S ENSURE that it's */
/* 	    being turned back on afterwards BEFORE anything else continues to read data from NAND !!!*/
s32 Enable_FlashECCCheck(void)
{
	if (AHBPROT_DISABLED)
	{
		s32 count = 0;

		disable_memory_protection();
		count += apply_patch("Flash ECC Check (enable)", flash_ecc_check_new, sizeof(flash_ecc_check_new), flash_ecc_check_old, sizeof(flash_ecc_check_old), 0, true);
		
		return count;
	}

	return ERROR_AHBPROT;
}

s32 Enable_DevFlash(void)
{
	if (AHBPROT_DISABLED)
	{
		s32 count = 0;

		disable_memory_protection();
		count += apply_patch("/dev/flash", dev_flash_old, sizeof(dev_flash_old), dev_flash_new, sizeof(dev_flash_new), 0, true);
		count += apply_patch("/dev/flash (new, part1)", dev_flash_old1, sizeof(dev_flash_old1), dev_flash_new1, sizeof(dev_flash_new1), 0, true);
		count += apply_patch("/dev/flash (new, part2)", dev_flash_old2, sizeof(dev_flash_old2), dev_flash_new2, sizeof(dev_flash_new2), 0, true);
		
		return count;
	}

	return ERROR_AHBPROT;
}

s32 Enable_DevBoot2(void)
{
	if (AHBPROT_DISABLED)
	{
		s32 count = 0;

		disable_memory_protection();
		count += apply_patch("undo /dev/flash (new, part1)", dev_flash_new1, sizeof(dev_flash_new1), dev_flash_old1, sizeof(dev_flash_old1), 0, true);
		count += apply_patch("undo /dev/flash (new, part2)", dev_flash_new2, sizeof(dev_flash_new2), dev_flash_old2, sizeof(dev_flash_old2), 0, true);
		
		return count;
	}

	return ERROR_AHBPROT;
}

s32 Restore_Trucha(void)
{
	if (AHBPROT_DISABLED)
	{
		s32 count = 0;

		disable_memory_protection();
		count += apply_patch("hash_check", hash_old, sizeof(hash_old), hash_patch, sizeof(hash_patch), 0, true);
		count += apply_patch("new_hash_check", new_hash_old, sizeof(new_hash_old), hash_patch, sizeof(hash_patch), 0, true);
		
		return count;
	}

	return ERROR_AHBPROT;
}

