/* RGD SDBoot Installer */

#ifndef ERRORCODES_H_
#define ERRORCODES_H_

/* Numeric */

#define ALL_OK 0

/* Strings */

typedef enum {
	ErrStr_NeedPerms = 0,
	ErrStr_Count	// Number of values supported by this enum.
} ErrStr;

static const char *errorStrings[ErrStr_Count] = {
	"AHBPROT is enabled, so the RGD SDBoot Installer can't run properly. To fix this, make sure that you are running the RGD SDBoot Installer\nfrom the Homebrew Channel with the correct meta.xml file in the same folder on your SD card, or from Wiiload."
};

#endif