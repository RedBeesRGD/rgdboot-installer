/* RGD SDBoot Installer

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2.0.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License 2.0 for more details.

   Copyright (C) 2012	tueidj
   Modified from the original version by RedBees

   Copyright (C) 2024	nitr8 - HEAVILY extended the SEEPROM code right here */

#include <stdarg.h>

/* [nitr8]: Added */
#include <stdio.h>

#include <stdlib.h>

/* [nitr8]: Added */
#include <string.h>

#include <unistd.h>
#include <ogc/machine/processor.h>

#include "seeprom.h"

/* [nitr8]: Added */
#include "tools.h"

/* [nitr8]: Added */
#include "hollywood.h"

/* [nitr8]: Added */
#include "gpio.h"

/* [nitr8]: Added */
#ifndef NO_VERSION_CLEAR

/* [nitr8]: Added */
#define BOOT2_INFO_COPY_1_OFFSET	0x48
#define BOOT2_INFO_COPY_2_OFFSET	0x52

#define SEEPROMDelay()			usleep(5)

/* [nitr8]: Added */
static u8 data_before[20];

/* [nitr8]: Reworked */
static u8 clear[10];

/* [nitr8]: Added */
static u8 data_after[20];

/* [nitr8]: Added */
static char *romver = "sd:/spromb2v.bin";

/* [nitr8]: Don't use that here... */
/*
enum {
	GP_EEP_CS = 0x000400,
	GP_EEP_CLK = 0x000800,
	GP_EEP_MOSI = 0x001000,
	GP_EEP_MISO = 0x002000
};
*/

/* [nitr8]: Fixed whitespace */
static void SEEPROMSendBits(int b, int bits)
{
	while (bits--)
	{
		if (b & (1 << bits))
		{
			mask32(HW_GPIO1OUT, 0, GP_EEP_MOSI);
		}
		else
		{
			mask32(HW_GPIO1OUT, GP_EEP_MOSI, 0);
		}

		SEEPROMDelay();
		mask32(HW_GPIO1OUT, 0, GP_EEP_CLK);
		SEEPROMDelay();
		mask32(HW_GPIO1OUT, GP_EEP_CLK, 0);
		SEEPROMDelay();
	}
}

/* [nitr8]: Add SEEPROM read support */
static int SEEPROMRecvBits(int bits)
{
	int res = 0;

	while (bits--)
	{
		res <<= 1;
		mask32(HW_GPIO1OUT, 0, GP_EEP_CLK);
		SEEPROMDelay();
		mask32(HW_GPIO1OUT, GP_EEP_CLK, 0);
		SEEPROMDelay();
		res |= !!(read32(HW_GPIO1IN) & GP_EEP_MISO);
	}

	return res;
}

/* [nitr8]: Add SEEPROM read support */
static int SEEPROMRead(void *dst, int offset, int size)
{
	int i;
	u16 *ptr = (u16 *)dst;
	u16 recv;
	u32 level;

	if (size & 1)
		return -1;

	offset >>= 1;

	_CPU_ISR_Disable(level);

	mask32(HW_GPIO1OUT, GP_EEP_CLK, 0);
	mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
	SEEPROMDelay();

	for (i = 0; i < size; ++i)
	{
		mask32(HW_GPIO1OUT, 0, GP_EEP_CS);
		SEEPROMSendBits((0x600 | (offset + i)), 11);
		recv = SEEPROMRecvBits(16);
		*ptr++ = recv;
		mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
		SEEPROMDelay();
	}

	_CPU_ISR_Restore(level);

	return size;
}

/* [nitr8]: Make it static and fix whitespace */
static int SEEPROMWrite(const void *src, unsigned int offset, unsigned int size)
{
	unsigned int i;
	const u8* ptr = (const u8*)src;
	u16 send;
	u32 level;

	if (offset&1 || size&1)
		return -1;

	offset>>=1;
	size>>=1;

	/* don't interrupt us, this is srs bsns */
	_CPU_ISR_Disable(level);

	mask32(HW_GPIO1OUT, GP_EEP_CLK, 0);
	mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
	SEEPROMDelay();

	/* EWEN - Enable programming commands */
	mask32(HW_GPIO1OUT, 0, GP_EEP_CS);
	SEEPROMSendBits(0x4FF, 11);
	mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
	SEEPROMDelay();

	for (i = 0; i < size; i++)
	{
		send = (ptr[0]<<8) | ptr[1];
		ptr += 2;

		/* start command cycle */
		mask32(HW_GPIO1OUT, 0, GP_EEP_CS);

		/* send command + address */
		SEEPROMSendBits((0x500 | (offset + i)), 11);

		/* send data */
		SEEPROMSendBits(send, 16);

		/* end of command cycle */
		mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
		SEEPROMDelay();

		/* wait for ready (write cycle is self-timed so no clocking needed) */
		mask32(HW_GPIO1OUT, 0, GP_EEP_CS);

		do
		{
			SEEPROMDelay();
		} while (!(read32(HW_GPIO1IN) & GP_EEP_MISO));

		mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
		SEEPROMDelay();
	}

	/* EWDS - Disable programming commands */
	mask32(HW_GPIO1OUT, 0, GP_EEP_CS);
	SEEPROMSendBits(0x400, 11);
	mask32(HW_GPIO1OUT, GP_EEP_CS, 0);
	SEEPROMDelay();

	_CPU_ISR_Restore(level);

	return size;
}
#endif

#ifndef NO_VERSION_CLEAR
/* [nitr8]: Reworked SEEPROM write support
            (now clears BOTH regions COMPLETELY) */
static int clear_boot2_info_region(int offset)
{
	u32 i;
	int ret = 0;

	/* Clear all array data */
	memset(clear, 0, sizeof(clear));
	memset(data_after, 0, sizeof(data_after));

	/* Clear the desired SEEPROM boot2 info region */
	SEEPROMWrite(clear, offset, sizeof(clear));

	/* Read back the data of the boot2 info regions */
	SEEPROMRead(data_after, offset, sizeof(clear));

	/* Check the contents... */
	for (i = 0; i < sizeof(clear); i++)
	{
		if (data_after[i] != 0)
		{
			gecko_printf("[ERROR]: boot2 info array element %d is not properly cleared (is set to %02x)\n", i, data_after[i]);
			ret = -1;
		}
	}

	return ret;
}
#endif

/* [nitr8]: Added */
int SEEPROMCompareVersion(int state)
{
	int ret = 0;

#ifndef NO_VERSION_CLEAR

	int bytes_read;
	FILE *fd;
	u8 current_data[20];

	/* Open the file in "read-only" mode */
	fd = fopen(romver, "r");

	if (fd)
	{
		/* Clear the array */
		memset(data_before, 0, sizeof(data_before));

		/* Read the data from the file on the SD-Card */
		bytes_read = fread(data_before, 1, sizeof(data_before), fd);

		if (bytes_read != sizeof(data_before))
		{
			gecko_printf("[ERROR]: Couldn't read %d bytes from file '%s' (only got %d bytes instead)\n", sizeof(data_before), romver, bytes_read);
			ret = -2;
		}
		else
		{
			/* Clear the array */
			memset(data_after, 0, sizeof(data_after));

			/* Read the data of the boot2 info regions */
			bytes_read = SEEPROMRead(data_after, BOOT2_INFO_COPY_1_OFFSET, sizeof(clear));

			if (bytes_read != sizeof(clear))
			{
				gecko_printf("[ERROR]: Couldn't read %d bytes from SEEPROM (only got %d bytes instead)\n", sizeof(current_data), bytes_read * 2);
				ret = -3;
			}
			else
			{
				u32 i;
				int is_empty = 0;
				int is_invalid = 0;

				gecko_printf("\n");
				gecko_printf("Comparing %d bytes from SEEPROM with %d bytes of backup file '%s'\n", bytes_read * 2, sizeof(current_data), romver);

				/* Clear the array */
				memset(current_data, 0, sizeof(current_data));

				gecko_printf("\n");
				gecko_printf("(STAGE1)\n");
				gecko_printf("\n");
				gecko_printf("Checking data from %s\n", state == 0 ? "BACKUP" : "SEEPROM");
				gecko_printf("\n");

				if (state == 0)
				{
					memcpy(current_data, data_before, sizeof(data_before));
				}
				else
				{
					memcpy(current_data, data_after, sizeof(data_after));
				}

				for (i = 0; i < sizeof(current_data); i++)
				{
					/* For both of the boot2 info regions: */

					/* Check if the main fields of use are 0 */
					/* Check if the other fields are NOT 0 */
					/* Check if the main fields are equal to each other */
					/* Check if the other fields are equal to each other */
					switch (i)
					{
						/* OS Version of boot2 info copy #1 */
						case 0:
							if (current_data[i] == 0)
								is_empty = 1;
							else
								is_empty = 0;

							if ((current_data[i] != current_data[7]) || (current_data[i] != current_data[8]) || (current_data[i] != current_data[9]))
								is_invalid = 1;
							else
								is_invalid = 0;

							break;

						/* CA-CRL Version of boot2 info copy #1 */
						case 1:
							if (current_data[i] == 0)
								is_empty = 1;
							else
								is_empty = 0;

							if ((current_data[i] != current_data[2]) || (current_data[i] != current_data[3]) ||
							    (current_data[i] != current_data[4]) || (current_data[i] != current_data[5]) || (current_data[i] != current_data[6]))
							{
								is_invalid = 1;
							}
							else
								is_invalid = 0;

							break;

						/* Sequence (byte 4) of boot2 info copy #1 */
						case 7:

						/* SUM (byte 1) of boot2 info copy #1 */
						case 8:

						/* SUM (byte 2) of boot2 info copy #1 */
						case 9:

						/* Sequence (byte 4) of boot2 info copy #2 */
						case 17:

						/* SUM (byte 1) of boot2 info copy #2 */
						case 18:

						/* SUM (byte 2) of boot2 info copy #2 */
						case 19:
							if (current_data[i] == 0)
								is_empty = 1;
							else
								is_empty = 0;

							if (i == (sizeof(current_data) - 1))
							{
								if (is_empty)
								{
									gecko_printf("\n");
									gecko_printf("Won't recreate existing backup file '%s' as data in SEEPROM is empty!\n", romver);
									gecko_printf("\n");
									gecko_printf("(Data in %s):\n", state == 1 ? "BACKUP" : "SEEPROM");
									gecko_printf("\n");

									hexdump(0, data_before, sizeof(data_before));

									gecko_printf("\n");
									gecko_printf("(Data in %s):\n", state == 0 ? "BACKUP" : "SEEPROM");
									gecko_printf("\n");

									hexdump(0, data_after, sizeof(data_after));

									gecko_printf("\n");

									goto abort;
								}
								else
								{
									gecko_printf("\n");
									gecko_printf("(Data in %s):\n", state == 0 ? "BACKUP" : "SEEPROM");
									gecko_printf("\n");

									hexdump(0, current_data, sizeof(current_data));

									goto check_stage_two;
								}
							}

							break;

						/* OS Version of boot2 info copy #2 */
						case 10:
							if (current_data[i] == 0)
								is_empty = 1;
							else
								is_empty = 0;

							if ((current_data[i] != current_data[17]) || (current_data[i] != current_data[18]) || (current_data[i] != current_data[19]))
								is_invalid = 1;
							else
								is_invalid = 0;

							break;

						/* CA-CRL Version of boot2 info copy #2 */
						case 11:
							if (current_data[i] == 0)
								is_empty = 1;
							else
								is_empty = 0;

							if ((current_data[i] != current_data[12]) || (current_data[i] != current_data[13]) || (current_data[i] != current_data[14]) ||
							    (current_data[i] != current_data[15]) || (current_data[i] != current_data[16]))
							{
								is_invalid = 1;
							}
							else
								is_invalid = 0;

							break;

						/* Signer-CRL Version of boot2 info copy #1 */
						case 2:

						/* Padding of boot2 info copy #1 */
						case 3:

						/* Sequence (byte 1) of boot2 info copy #1 */
						case 4:

						/* Sequence (byte 2) of boot2 info copy #1 */
						case 5:

						/* Sequence (byte 3) of boot2 info copy #1 */
						case 6:

						/* Signer-CRL Version of boot2 info copy #2 */
						case 12:

						/* Padding of boot2 info copy #2 */
						case 13:

						/* Sequence (byte 1) of boot2 info copy #2 */
						case 14:

						/* Sequence (byte 2) of boot2 info copy #2 */
						case 15:

						/* Sequence (byte 3) of boot2 info copy #2 */
						case 16:
							if (current_data[i] == 0)
								is_empty = 1;
							else
								is_empty = 0;

							break;
					}

					if (!is_invalid)
					{
						gecko_printf("%s element %d is ", state == 0 ? "BACKUP" : "SEEPROM", i);

						if (is_empty)
							gecko_printf("cleared out!");
						else
							gecko_printf("NOT cleared!");

						gecko_printf("\n");
					}
					else
						gecko_printf("%s element %d is INVALID!\n", state == 0 ? "BACKUP" : "SEEPROM", i);

					if (state == 1)
					{
						gecko_printf("SEEPROM boot2 info array element %d ", i);

						if (data_before[i] == data_after[i])
						{
							gecko_printf("HAS MATCH with ");
						}
						else
						{
							gecko_printf("does NOT MATCH ");
						}

						gecko_printf("array element %d from BACKUP\n", i);
					}
				}
check_stage_two:
				gecko_printf("\n");
				gecko_printf("(STAGE2)\n");
				gecko_printf("\n");

				if (ret == 0)
				{
					gecko_printf("Testing fields...\n");
					gecko_printf("\n");

					/* Scan all the array data +2 for valid / invalid elements ('0') */
					for (i = 0; i < (sizeof(current_data) + 2); i++)
					{
						switch (i)
						{
							/*
							 * These usually ARE set
							 */

							/* OS Version of boot2 info copy #1 */
							case 0:

							/* Sequence (byte 4) of boot2 info copy #1 */
							case 7:

							/* SUM (byte 1) of boot2 info copy #1 */
							case 8:

							/* SUM (byte 2) of boot2 info copy #1 */
							case 9:

							/* OS Version of boot2 info copy #2 */
							case 10:

							/* Sequence (byte 4) of boot2 info copy #2 */
							case 17:

							/* SUM (byte 1) of boot2 info copy #2 */
							case 18:

							/* SUM (byte 2) of boot2 info copy #2 */
							case 19:
								if (current_data[i] != 0)
								{
									gecko_printf("SEEPROM boot2 info BACKUP array element %d is set to %02x\n", i, current_data[i]);
									continue;
								}
								else
								{
									gecko_printf("[ERROR]: SEEPROM boot2 info BACKUP array element %d is empty!\n", i);
									goto abort;
								}

							/*
							 * These should NEVER be set to anything
							 */

							/* CA-CRL Version of boot2 info copy #1 */
							case 1:

							/* Signer-CRL Version of boot2 info copy #1 */
							case 2:

							/* Padding of boot2 info copy #1 */
							case 3:

							/* Sequence (byte 1) of boot2 info copy #1 */
							case 4:

							/* Sequence (byte 2) of boot2 info copy #1 */
							case 5:

							/* Sequence (byte 3) of boot2 info copy #1 */
							case 6:

							/* CA-CRL Version of boot2 info copy #2 */
							case 11:

							/* Signer-CRL Version of boot2 info copy #2 */
							case 12:

							/* Padding of boot2 info copy #2 */
							case 13:

							/* Sequence (byte 1) of boot2 info copy #2 */
							case 14:

							/* Sequence (byte 2) of boot2 info copy #2 */
							case 15:

							/* Sequence (byte 3) of boot2 info copy #2 */
							case 16:
								if (current_data[i] == 0)
								{
									gecko_printf("SEEPROM boot2 info BACKUP array element %d is empty!\n", i);
									continue;
								}
								else
								{
									gecko_printf("[ERROR]: SEEPROM boot2 info BACKUP array element %d is NOT empty (set to %02x)!\n", i, current_data[i]);
									goto abort;
								}

							/* Addtitional case 20 will test fields 0, 7, 8 and 9 against each other for each of the boot2 info regions */
							/* The same will be done for the empty fields */
							case 20:
								gecko_printf("\n");
								gecko_printf("Testing equality...\n");

								if ((current_data[0] == current_data[7]) && (current_data[0] == current_data[8]) && (current_data[0] == current_data[9]) &&
								    (current_data[10] == current_data[17]) && (current_data[10] == current_data[18]) && (current_data[10] == current_data[19]) &&
								    (current_data[1] == current_data[2]) && (current_data[1] == current_data[3]) && (current_data[1] == current_data[4]) &&
								    (current_data[1] == current_data[5]) && (current_data[1] == current_data[6]) &&
								    (current_data[1] == current_data[11]) && (current_data[1] == current_data[12]) && (current_data[1] == current_data[13]) &&
								    (current_data[1] == current_data[14]) && (current_data[1] == current_data[15]) && (current_data[1] == current_data[16]))
								{
									gecko_printf("All fields match up\n");
									continue;
								}
								else
								{
									gecko_printf("[ERROR]: SEEPROM boot2 info data fields in BACKUP are messed up!\n");
									goto abort;
								}

							/* Addtitional case 21 ("default") will just break out as all the data was scanned and valid */
							default:
								break;
						}
					}

					gecko_printf("\n");
					gecko_printf("(STAGE3)\n");

					gecko_printf("\n");
					gecko_printf("Testing versions (old vs. new)...\n");

					/* Scan all the array data of interest for versions according to their state (old vs. new) */
					if (((current_data[0]) < (current_data[10])) && ((current_data[7]) < (current_data[17])) &&
					    ((current_data[8]) < (current_data[18])) && ((current_data[9]) < (current_data[19])))
					{
						gecko_printf("boot2 version info in region 1 is LOWER than that in region 2 (OK)\n");
					}
					else
					{
						gecko_printf("[ERROR]: SEEPROM boot2 info data fields in BACKUP are messed up (version MISMATCH)!\n");
						goto abort;
					}
				}
				else
				{
abort:
					gecko_printf("Aborting...\n");
					ret = -5;
				}
			}
		}

		/* Close the file */
		fclose(fd);
	}
	else
	{
		gecko_printf("[ERROR]: Couldn't open SEEPROM backup file '%s' to compare it against data stored in SEEPROM!\n", romver);
		ret = -1;
	}
#endif
	return ret;
}

/* [nitr8]: Added */
int SEEPROMRestoreVersion(void)
{
#ifndef NO_VERSION_CLEAR
	FILE *fd;
	int bytes_read;
	int bytes_written;
	int ret = 0;

	if (!sd_initialized)
		sd_initialized = fatInitDefault();

	if (!sd_initialized)
	{
		gecko_printf("[ERROR]: Couldn't initialize the filesystem (%d)\n", sd_initialized);
		ret = -4;
		goto err;
	}

	/* Before restoring anything to the SEEPROM, compare the data to be written */
	ret = SEEPROMCompareVersion(0);

	if (ret != 0)
		goto err;

	/* Open the file */
	fd = fopen(romver, "r");

	if (fd)
	{
		gecko_printf("Opened file '%s' for reading\n", romver);

		/* Clear the array */
		memset(data_before, 0, sizeof(data_before));

		/* Read the data from the file on the SD-Card */
		bytes_read = fread(data_before, 1, sizeof(data_before), fd);

		gecko_printf("SEEPROM boot2 info regions read (%d bytes)\n", bytes_read);

		if (bytes_read != sizeof(data_before))
		{
			gecko_printf("[ERROR]: Couldn't read %d bytes from BACKUP (only got %d bytes instead)\n", sizeof(data_before), bytes_read);
			ret = -2;
		}
		else
		{
			gecko_printf("Restoring SEEPROM boot2 info data...\n");

			/* Write the data to the boot2 info regions */
			bytes_written = SEEPROMWrite(data_before, BOOT2_INFO_COPY_1_OFFSET, sizeof(data_before));

			if (bytes_written != sizeof(clear))
			{
				gecko_printf("[ERROR]: Couldn't write %d bytes to SEEPROM (only wrote %d bytes instead)\n", sizeof(data_before), bytes_written * 2);
				ret = -3;
			}
			else
			{
				gecko_printf("Wrote %d bytes to SEEPROM\n", bytes_written * 2);

				/* Close the file */
				fclose(fd);

				ret = SEEPROMCompareVersion(1);

				goto err;
			}
		}
	}
	else
	{
		gecko_printf("[ERROR]: Couldn't open file '%s' for reading\n", romver);
		ret = -1;
		goto err;
	}

	/* Close the file */
	fclose(fd);

err:

#endif
	return ret;
}

/* [nitr8]: Added */
int SEEPROMBackupVersion(void)
{
#ifndef NO_VERSION_CLEAR
	FILE *fd;
	int bytes_read;
	int bytes_written;
	int ret = 0;

	if (!sd_initialized)
		sd_initialized = fatInitDefault();

	if (!sd_initialized)
	{
		gecko_printf("[ERROR]: Couldn't initialize the filesystem (%d)\n", sd_initialized);
		ret = -1;
		goto err;
	}

	/* Before backup to the SD-Card, compare the data to be written */
	/* Open the file in "read" mode - check if it even exists for comparing the contents */
	/* If it does NOT exist - allow to create it anyway */
	fd = fopen(romver, "r");

	if (fd)
	{
		/* Close the file */
		fclose(fd);

		ret = SEEPROMCompareVersion(1);
	}

	if (ret != 0)
		goto err;

	/* Open the file in "write" mode */
	fd = fopen(romver, "wb");

	if (fd)
	{
		gecko_printf("Opened file '%s' for writing\n", romver);

		/* Clear the array */
		memset(data_before, 0, sizeof(data_before));

		/* Read the data of the boot2 info regions */
		bytes_read = SEEPROMRead(data_before, BOOT2_INFO_COPY_1_OFFSET, sizeof(clear));

		gecko_printf("Read %d bytes from SEEPROM\n", bytes_read * 2);

		if (bytes_read != sizeof(clear))
		{
			gecko_printf("[ERROR]: Couldn't read %d bytes from SEEPROM (only got %d bytes instead)\n", sizeof(data_before), bytes_read * 2);
			ret = -2;
		}
		else
		{
			gecko_printf("Creating BACKUP of SEEPROM boot2 info data...\n");

			/* Write it to the file on the SD-Card */
			bytes_written = fwrite(data_before, 1, sizeof(data_before), fd);

			if (bytes_written != sizeof(data_before))
			{
				gecko_printf("[ERROR]: Couldn't write %d bytes to file '%s' (only wrote %d bytes instead)\n", sizeof(data_before), romver, bytes_written);
				ret = -3;
			}
			else
			{
				gecko_printf("SEEPROM boot2 info regions dumped to file '%s' on the SD-Card (%d bytes)\n", romver, bytes_written);

				/* Close the file */
				fclose(fd);

				ret = SEEPROMCompareVersion(0);

				goto err;
			}
		}
	}
	else
	{
		gecko_printf("[ERROR]: Couldn't open file '%s' for writing\n", romver);
		ret = -4;
		goto err;
	}

	/* Close the file */
	fclose(fd);

err:

#endif
	return ret;
}

/* [nitr8]: Completely reworked */
int SEEPROMClearVersion(void)
{
	int ret = 0;

#ifndef NO_VERSION_CLEAR

	/* Backup the boot2 info regions to the SD-Card first!!! */
	ret = SEEPROMBackupVersion();

	/* Read or write failed for either the backup file or the SEEPROM */
	if (ret != 0)
		goto out;

	/* Clear boot2 info region 1 in SEEPROM */
	ret = clear_boot2_info_region(BOOT2_INFO_COPY_1_OFFSET);

	if (ret == 0)
		gecko_printf("SEEPROM boot2 info region 1 cleared - going for region 2\n");

	/* Clear boot2 info region 2 in SEEPROM */
	ret = clear_boot2_info_region(BOOT2_INFO_COPY_2_OFFSET);

	if (ret == 0)
		gecko_printf("SEEPROM boot2 info region 2 cleared - DONE\n");

out:

#endif

	return ret;
}

/* [nitr8]: MOVED HERE (FROM INSTALLER.C) - and edited */
int SEEPROMClearStep(void)
{
	int ret = 0;

	printf("\n\nPress any controller button to clear the boot2 version.");

	WaitForPad();

#ifdef DOLPHIN_CHECK /* ClearVersion() crashes dolphin.
			This is included so that when building with NO_DOLPHIN_CHECK you can get past this point in Dolphin */
	if (GetBoot2Version() > 0)
	{
		ret = SEEPROMClearVersion();
	}
#endif

	if (ret == 0)
		printf("\nThe boot2 version was cleared successfully!\n");
	else
		printf("\nError %d while trying to clear the boot2 version!\n", ret);

	return ret;
}

