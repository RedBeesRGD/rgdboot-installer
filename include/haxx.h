/* RGD SDBoot Installer */

/* haxx.h - PowerPC privilege elevation
     Written by Palapeli

   Copyright (C) 2022 Palapeli
   SPDX-License-Identifier: MIT */

/* [nitr8]: Added */
#ifndef __HAXX_H__
#define __HAXX_H__

/* [nitr8]: just "NO!"
#pragma once */

#include <gctypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Performs an IOS exploit and branches to the entrypoint in system mode. */

/* [nitr8]: Make static */
/* s32 Haxx_IOSExploit(u32 entrypoint); */

/* Flushes data on PowerPC and invalidates on ARM. */

/* [nitr8]: Make static */
/* void Haxx_FlushRange(const void* data, u32 length); */

/* Checks if the PPC has full bus access. */

/* [nitr8]: Make static */
/* bool Haxx_CheckBusAccess(void); */

/* Attempts to get full PPC bus access. Will perform an IOS exploit if needed. */
bool Haxx_GetBusAccess(void);

#ifdef __cplusplus
}
#endif

/* [nitr8]: Added */
#endif /* __HAXX_H__ */

