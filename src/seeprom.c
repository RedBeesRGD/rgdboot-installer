/* RGD SDBoot Installer */

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// Copyright (C) 2012	tueidj
// Modified from the original version by RedBees

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <ogc/machine/processor.h>

#include "seeprom.h"

#define HW_GPIO1OUT				0x0D8000E0
#define HW_GPIO1IN				0x0D8000E8

enum {
		GP_EEP_CS = 0x000400,
		GP_EEP_CLK = 0x000800,
		GP_EEP_MOSI = 0x001000,
		GP_EEP_MISO = 0x002000
};
#define SEEPROMDelay() usleep(5)

static void SEEPROMSendBits(int b, int bits) {
		while (bits--) {
				if (b & (1 << bits)) {
						mask32(HW_GPIO1OUT, 0, GP_EEP_MOSI);
				}
				else {
						mask32(HW_GPIO1OUT, GP_EEP_MOSI, 0);
				}
				SEEPROMDelay();
				mask32(HW_GPIO1OUT, 0, GP_EEP_CLK);
				SEEPROMDelay();
				mask32(HW_GPIO1OUT, GP_EEP_CLK, 0);
				SEEPROMDelay();
		}
}

int SEEPROMWrite(const void *src, unsigned int offset, unsigned int size) {
		unsigned int i;
		const u8* ptr = (const u8*)src;
		u16 send;
		u32 level;

		if (offset&1 || size&1) return -1;

		offset>>=1;
		size>>=1;

		// don't interrupt us, this is srs bsns
		_CPU_ISR_Disable(level);

		mask32(HW_GPIO1OUT, GP_EEP_CLK, 0);
		mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
		SEEPROMDelay();

		// EWEN - Enable programming commands
		mask32(HW_GPIO1OUT, 0, GP_EEP_CS);
		SEEPROMSendBits(0x4FF, 11);
		mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
		SEEPROMDelay();

		for (i = 0; i < size; i++) {
				send = (ptr[0]<<8) | ptr[1];
				ptr += 2;
				// start command cycle
				mask32(HW_GPIO1OUT, 0, GP_EEP_CS);
				// send command + address
				 SEEPROMSendBits((0x500 | (offset + i)), 11);
				 // send data
				SEEPROMSendBits(send, 16);
				// end of command cycle
				mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
				SEEPROMDelay();

				// wait for ready (write cycle is self-timed so no clocking needed)
				mask32(HW_GPIO1OUT, 0, GP_EEP_CS);
				do {
					SEEPROMDelay();
				} while (!(read32(HW_GPIO1IN) & GP_EEP_MISO));

				mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
				SEEPROMDelay();
		}

		// EWDS - Disable programming commands
		mask32(HW_GPIO1OUT, 0, GP_EEP_CS);
		SEEPROMSendBits(0x400, 11);
		mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
		SEEPROMDelay();

		_CPU_ISR_Restore(level);

		return size;
}

void ClearVersion()
{
		u8 clear[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		SEEPROMWrite(clear, 0x48, 12);
}