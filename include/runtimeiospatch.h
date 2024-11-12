/* RGD SDBoot Installer */

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

/**
 * NOTE: this library has been modified to only include the patches necessary 
 * for the boot2 installer.
 */

#ifndef __RUNTIMEIOSPATCH_H__
#define __RUNTIMEIOSPATCH_H__

/**
 * Version information for Libruntimeiospatch.
 */
#define LIB_RUNTIMEIOSPATCH_VERSION "1.5.4"

/*==============================================================================
   HW_RVL header
  ============================================================================== */
#if defined(HW_RVL) /* defined(HW_RVL) */

/**
 *Returns true when HW_AHBPROT access can be applied
 */
#define AHBPROT_DISABLED (*(vu32*)0xcd800064 == 0xFFFFFFFF)

/*==============================================================================
   Error code definitions:
  ============================================================================== */
#define ERROR_AHBPROT       -5
#define ERROR_PATCH         -7

/*==============================================================================
   C++ header
  ============================================================================== */
#ifdef __cplusplus
extern "C" {
#endif
/* __cplusplus */

/*==============================================================================
   Patchsets:
  ============================================================================== */
/*
Wii:
    * /dev/flash access
    * ES_ImportBoot
*/
/*==============================================================================
   Functions:
  ============================================================================== */

/**
 * This function can be used to keep HW_AHBPROT access when going to reload IOS
 * @param verbose Flag determing whether or not to print messages on-screen
 * @example 
 *      if(AHBPROT_DISABLED) {
 *          s32 ret;
 *          ret = IosPatch_AHBPROT(false);
 *          if (ret) {
 *              IOS_ReloadIOS(36);
 *          } else {
 *              printf("IosPatch_AHBPROT failed.");
 *          }
 *      }
 * @return Signed 32bit integer representing code
 *      > 0             : Success   - return equals to number of applied patches
 *      ERROR_AHBPROT   : Error     - No HW_AHBPROT access
 */

/* [nitr8]: Unused */
/*s32 IosPatch_AHBPROT(bool verbose); */


s32 Fix_ES_ImportBoot(void);
s32 Enable_DevFlash(void);

/* [nitr8]: get rid of warning for implicit declaration of function */
s32 Restore_Trucha(void);

s32 Enable_DevBoot2(void);

/* [nitr8]: Added */
s32 Disable_FlashECCCheck(void);
s32 Enable_FlashECCCheck(void);

/* [nitr8]: Added */
s32 Disable_UIDCheck(void);
s32 Enable_UIDCheck(void);

/* [nitr8]: Added */
s32 Disable_IOSOpen_ACCESS_prevention(void);
s32 Enable_IOSOpen_ACCESS_prevention(void);

/* [nitr8]: Added */
s32 Disable_IOS_Debug_Output(void);
s32 Enable_IOS_Debug_Output(void);

/**
 * This function applies patches on current IOS
 * @see Patchsets
 * @param verbose Flag determing whether or not to print messages on-screen.
 * @example if(AHBPROT_DISABLED) IosPatch_FULL(true, false, false, false);
 * @return Signed 32bit integer representing code
 *      > 0             : Success   - return equals to number of applied patches
 *      ERROR_AHBPROT   : Error     - No HW_AHBPROT access
 *      ERROR_PATCH     : Error     - Patching HW_AHBPROT access failed
 */

/* [nitr8]: Unused */
/* s32 IosPatch_RUNTIME(bool verbose); */


/**
 * This function combines IosPatch_AHBPROT + IOS_ReloadIOS + IosPatch_RUNTIME
 * @see Patchsets
 * @param verbose Flag determing whether or not to print messages on-screen.
 * @param IOS Which IOS to reload into.
 * @example if(AHBPROT_DISABLED) IosPatch_FULL(true, false, false, false, 58);
 * @return Signed 32bit integer representing code
 *      > 0             : Success   - return equals to number of applied patches
 *      ERROR_AHBPROT   : Error     - No HW_AHBPROT access
 *      ERROR_PATCH     : Error     - Patching HW_AHBPROT access failed
 */

/* [nitr8]: Unused */
/* s32 IosPatch_FULL(bool verbose, int IOS); */

/*==============================================================================
   C++ footer
  ============================================================================== */
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*==============================================================================
   HW_RVL footer
  ============================================================================== */
#endif /* defined(HW_RVL) */

#endif /* __RUNTIMEIOSPATCH_H__ */

