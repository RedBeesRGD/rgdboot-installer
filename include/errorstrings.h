/* RGD SDBoot Installer */

/* [nitr8]: New file */

#ifndef __ERRORSTRINGS_H__
#define __ERRORSTRINGS_H__

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

	/* [nitr8]: Added these */
	ErrStr_BadBlockError,
	ErrStr_EraseError,
	ErrStr_SeekError,
	ErrStr_AllocError,
	ErrStr_ReadLengthError,
	ErrStr_DowngradeError,

	ErrStr_Generic,
	ErrStr_Count	/* Number of values supported by this enum. */
} ErrStr;

static const char *errorStrings[ErrStr_Count] = {
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
	"Unfortunately, there are multiple bad blocks present in the boot2 area.\nFor safety reasons, RGDBoot Installer cannot install NANDBoot...",

	/* [nitr8]: Added these */
	"Unfortunately, there is a bad block present in the boot2 area.\nFor safety reasons, RGDBoot Installer cannot install NANDBoot...",
	"Couldn't erase block",
	"Couldn't seek in file",
	"Couldn't allocate enough memory",
	"Couldn't read from file (bad length)",
	"Error: Cannot downgrade boot2",

	"Error code:"
};

#endif /* __ERRORSTRINGS_H__ */

