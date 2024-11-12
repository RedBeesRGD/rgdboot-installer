/* RGD SDBoot Installer */

#ifndef __ERRORCODES_H__
#define __ERRORCODES_H__

#define ALL_OK               0

/* [nitr8]: Moved error codes here */
#define MISSING_FILE        -1
#define ERASE_ERROR         -2
#define SEEK_ERROR          -3
#define BOOT2_DOLPHIN       -4
#define BAD_BOOT_BLOCKS     -5

/* [nitr8]: Added */
#define ALLOC_ERROR         -6

/* [nitr8]: Added */
#define READ_LENGTH_ERROR   -7

#define BAD_BLOCK           -13
#define HASH_MISMATCH       -1022
#define CANNOT_DOWNGRADE    -1031

/* [nitr8]: Moved to new file "errorstrings.h" */
#if 0
typedef enum {
	ErrStr_NeedPerms = 0,
	ErrStr_InDolphin,
	ErrStr_InCafe,
	ErrStr_DevFlashErr,
	ErrStr_BadFile,
	ErrStr_SettingTooBig,
	ErrStr_SettingInvalid,
	ErrStr_SDCard,
	ErrStr_BadBoot2Ver,
	ErrStr_WrongVersion,
	ErrStr_MissingFiles,
	ErrStr_BadBlocks,
	ErrStr_Generic,
	ErrStr_Count	/* Number of values supported by this enum. */
} ErrStr;

const char *errorStrings[ErrStr_Count] = {
	"AHBPROT is enabled, so the RGD SDBoot Installer can't run properly. To fix this, make sure that you are running the RGD SDBoot Installer\nfrom the Homebrew Channel with the correct meta.xml file in the same folder on your SD card, or from Wiiload.",
	"The RGD SDBoot Installer cannot run in Dolphin, as it relies on hardware\nfeatures which Dolphin does not emulate.\n\nThis can be bypassed with the compiler flag -NO_DOLPHIN_CHECK\nfor testing purposes.",
	"The RGD SDBoot Installer cannot run on a Wii U, as\ninstalling a custom boot2 has no effect on the Wii U.",
	"The RGD SDBoot Installer cannot access /dev/flash.\nThis might be caused by using incompatible IOS.",
	"The boot2 file (or the payload) on the SD card is invalid.\nNo data has been written yet, so your system will still boot.",
	"The setting.txt file is too big.",
	"The setting.txt file is invalid.",
	"The RGD SDBoot Installer cannot mount your SD card. Make sure it's inserted and try again.",
	"The boot2 version couldn't be obtained.",
	"The version of the RGD SDBoot Installer with Dolphin checking disabled will not run on a regular Wii with a boot2 version higher than v0, as it would cause a brick.",
	"One or more required files are missing.",
	"Unfortunately, there is at least one bad block present in the boot2 area.\nFor safety reasons, RGDBoot Installer cannot install NANDBoot...",
	"Error code:"
};
#endif

#endif /* __ERRORCODES_H__ */

