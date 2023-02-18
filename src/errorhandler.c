/* RGD SDBoot Installer */

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "errorhandler.h"
#include "tools.h"

void ThrowError(const char* errorString) {
	printf("\x1b[2;0H\033[2J");

	printf("An error has occurred and the RGD SDBoot Installer can't continue.\nThe details of the error are:\n\n");
	printf("%s", errorString);

	WaitExit();
}

void ThrowErrorEx(const char* errorString, s32 errorCode) {
	printf("\x1b[2;0H\033[2J");

	printf("An error has occurred and the RGD SDBoot Installer can't continue.\nThe details of the error are:\n\n");
	printf("%s %i", errorString, errorCode);
	
	WaitExit();
}
