/* WiiFetch */

/* Numeric */

#define ALL_OK 0
// TODO: Include these error codes (preferably with a user-friendly name/description) in the error output.
#define ERR_BADFILE -1
#define ERR_TOOBIG -2
#define ERR_NOENT -3

/* Strings */

typedef enum {
	ErrStr_CantOpenProdInfo = 0,
	ErrStr_CantGetSerno,
	ErrStr_CantGetDvd,
	ErrStr_CantGetIos,
	ErrStr_NeedPerms,
	ErrStr_Count	// Number of values supported by this enum.
} ErrStr;

static const char *errorStrings[ErrStr_Count] = {
	"The product info file (setting.txt) couldn't be opened. This should never happen on standard systems.\nTry using wiifetch_noprodinfo.dol/elf.",
	"The product serial number couldn't be obtained from the product info file (setting.txt). This should never happen on standard systems.",
	"The DVD-Video support flag couldn't be obtained from the product info file (setting.txt).",
	"The IOS version information couldn't be obtained. This should never happen on standard systems.",
	"AHBPROT is enabled, so WiiFetch can't run properly. To fix this, make sure that you are running WiiFetch\nfrom the Homebrew Channel with the correct meta.xml file in the same folder on your SD card, or from Wiiload."
};