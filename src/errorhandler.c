/* RGDSDB */

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "errorhandler.h"

void ThrowError(char* errorString) {
	u8 i = 0;
	u32 wpadButtons = 0;
	u32 padButtons = 0;
	printf("\x1b[2;0H\033[2J");

	printf("An error has occurred and RGDSDB can't continue. The details of the error are:\n\n");
	printf("%s", errorString);
	printf("\n\nPress any controller button to exit.");

	while(1) {
		WPAD_ScanPads();
		PAD_ScanPads();
		for(i = 0; i < 4; i++) {
			wpadButtons += WPAD_ButtonsDown(i);
			padButtons += PAD_ButtonsDown(i);
		}
		if (wpadButtons || padButtons) exit(0);

		VIDEO_WaitVSync();
	}
	
}

