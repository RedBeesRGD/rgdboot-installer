// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// Copyright (C) 2010		Joseph Jordan <joe.ftpii@psychlaw.com.au>
// Copyright (C) 2012-2013	damysteryman
// Copyright (C) 2012-2015	Christopher Bratusek <nano@jpberlin.de>
// Copyright (C) 2013		DarkMatterCore
// Copyright (C) 2014		megazig
// Copyright (C) 2015		FIX94

#include <gccore.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <string.h>

#include "runtimeiospatch.h"

#define MEM_REG_BASE 0xd8b4000
#define MEM_PROT (MEM_REG_BASE + 0x20a)

void TextColor(u8 color, u8 bold)
{
	/* Set foreground color */
	printf("\x1b[%u;%um", color + 30, bold);
	fflush(stdout);
}

static inline void disable_memory_protection(void) {
	write32(MEM_PROT, read32(MEM_PROT) & 0x0000FFFF);
}

static const u8 es_set_ahbprot_old[] = { 0x68, 0x5B, 0x22, 0xEC, 0x00, 0x52, 0x18, 0x9B, 0x68, 0x1B, 0x46, 0x98, 0x07, 0xDB };
static const u8 es_set_ahbprot_patch[]   = { 0x01 };

static const u8 es_importboot_old1[] = {0x4D, 0x18, 0xE0, 0x02};
static const u8 es_importboot_new1[] = {0xBF, 0x00, 0xBF, 0x00};

static const u8 es_importboot_old2[] = {0xD9, 0x00, 0x4E, 0x18};
static const u8 es_importboot_new2[] = {0xE0, 0x00, 0xBF, 0x00};

// Special thanks to nitr8 for the /dev/flash access patch!
static const u8 dev_flash_old[] = { 0xa7, 0x28, 0x00, 0xd1, 0x02, 0x23, 0x00 };
static const u8 dev_flash_new[] = { 0xa7, 0x28, 0x00, 0xd1, 0x02, 0x23, 0x01 };


static u8 apply_patch(const char *name, const u8 *old, u32 old_size, const u8 *patch, size_t patch_size, u32 patch_offset, bool verbose) {
	u8 *ptr_start = (u8*)*((u32*)0x80003134), *ptr_end = (u8*)0x94000000;
	u8 found = 0;
	if(verbose)
	{
		TextColor(7,1);
		printf("\t\t    Patching %-30s", name);
	}
	u8 *location = NULL;
	while (ptr_start < (ptr_end - patch_size)) {
		if (!memcmp(ptr_start, old, old_size)) {
			found++;
			location = ptr_start + patch_offset;
			u8 *start = location;
			u32 i;
			for (i = 0; i < patch_size; i++) {
				*location++ = patch[i];
			}
			DCFlushRange((u8 *)(((u32)start) >> 5 << 5), (patch_size >> 5 << 5) + 64);
			ICInvalidateRange((u8 *)(((u32)start) >> 5 << 5), (patch_size >> 5 << 5) + 64);
		}
		ptr_start++;
	}
	if(verbose){
		if (found)
		{
			TextColor(2, 1);
			printf("\t\t patched\n");
		}
		else
		{
			TextColor(1, 1);
			printf("\t\t not patched\n");
		}
	}
	return found;
}

s32 IosPatch_AHBPROT(bool verbose) {
	if (AHBPROT_DISABLED) {
		disable_memory_protection();
		s32 ret = apply_patch("es_set_ahbprot", es_set_ahbprot_old, sizeof(es_set_ahbprot_old), es_set_ahbprot_patch, sizeof(es_set_ahbprot_patch), 25, verbose);
		if (ret)
			return ret;
		else
			return ERROR_PATCH;
	}
	return ERROR_AHBPROT;
}

s32 IosPatch_RUNTIME(bool verbose) {
	s32 count = 0;

	if (AHBPROT_DISABLED) {
		disable_memory_protection();
		if(verbose) printf("\t\t\n\n\n\n\n");

		if(verbose)
		{
			TextColor(6, 1);
			printf("\t>> Applying standard Wii patches:\n");
		}
		count += apply_patch("ES_ImportBoot(1)", es_importboot_old1, sizeof(es_importboot_old1), es_importboot_new1, sizeof(es_importboot_new1), 0, verbose);
		count += apply_patch("ES_ImportBoot(2)", es_importboot_old2, sizeof(es_importboot_old2), es_importboot_new2, sizeof(es_importboot_new2), 0, verbose);
		count += apply_patch("/dev/flash", dev_flash_old, sizeof(dev_flash_old), dev_flash_new, sizeof(dev_flash_new), 0, verbose);
		
		return count;
	}
	return ERROR_AHBPROT;
}

s32 IosPatch_FULL(bool verbose, int IOS) {
	s32 ret = 0;
	s32 xret = 0;

	if (AHBPROT_DISABLED)
		ret = IosPatch_AHBPROT(verbose);
	else
		return ERROR_AHBPROT;

	if (ret) {
		IOS_ReloadIOS(IOS);
		xret = IosPatch_RUNTIME(verbose);
	} else {
		xret = ERROR_PATCH;
	}

	return xret;

}
