// haxx.h - PowerPC privilege elevation
//   Written by Palapeli
//
// Copyright (C) 2022 Palapeli
// SPDX-License-Identifier: MIT

#pragma once

#include <gctypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Performs an IOS exploit and branches to the entrypoint in system mode.
s32 Haxx_IOSExploit(u32 entrypoint);

// Flushes data on PowerPC and invalidates on ARM.
void Haxx_FlushRange(const void* data, u32 length);

// Checks if the PPC has full bus access.
bool Haxx_CheckBusAccess();

// Attempts to get full PPC bus access. Will perform an IOS exploit if needed.
bool Haxx_GetBusAccess();

#ifdef __cplusplus
}
#endif