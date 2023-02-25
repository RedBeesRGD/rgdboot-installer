/* RGD SDBoot Installer */

#ifndef ERRORCODES_H_
#define ERRORCODES_H_

#define ALL_OK 0

typedef enum {
	ErrStr_NeedPerms = 0,
	ErrStr_InDolphin,
	ErrStr_InCafe,
	ErrStr_BadFile,
	ErrStr_SettingTooBig,
	ErrStr_SettingInvalid,
	ErrStr_SDCard,
	ErrStr_BadBoot2Ver,
	ErrStr_WrongVersion,
	ErrStr_MissingFiles,
	ErrStr_Generic,
	ErrStr_Count	// Number of values supported by this enum.
} ErrStr;

static const char *errorStrings[ErrStr_Count] = {
	"AHBPROT is enabled, so the RGD SDBoot Installer can't run properly. To fix this, make sure that you are running the RGD SDBoot Installer\nfrom the Homebrew Channel with the correct meta.xml file in the same folder on your SD card, or from Wiiload.",
	"The RGD SDBoot Installer cannot run in Dolphin, as it relies on hardware\nfeatures which Dolphin does not emulate.\n\nThis can be bypassed with the compiler flag -NO_DOLPHIN_CHECK\nfor testing purposes.",
	"The RGD SDBoot Installer cannot run on a Wii U, as\ninstalling a custom boot2 has no effect on the Wii U.",
	"The boot2 file (or the payload) on the SD card is invalid.\nNo data has been written yet, so your system will still boot.",
	"The setting.txt file is too big.",
	"The setting.txt file is invalid.",
	"The RGD SDBoot Installer cannot mount your SD card. Make sure it's inserted and try again.",
	"The boot2 version couldn't be obtained.",
	"The version of the RGD SDBoot Installer with Dolphin checking disabled will not run on a regular Wii with a boot2 version higher than v0, as it would cause a brick.",
	"One or more required files are missing.",
	"Error code:"
};

#endif
