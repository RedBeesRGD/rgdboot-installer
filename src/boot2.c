/* RGD SDBoot Installer */

/* [nitr8]: Added */
#include <stdarg.h>

/* [nitr8]: Added */
#include <ogc/machine/processor.h>

/* Uses the same method as the DOP-Mii boot2 installation routine */
#include "boot2.h"
#include "flash.h"
#include "runtimeiospatch.h"

/* [nitr8]: Added */
#include "flash.h"

/* [nitr8]: Added */
#include "errorcodes.h"

/* [nitr8]: Added */
#include "hollywood.h"

#define ALIGN(a, b)  ((((a) + (b) - 1) / (b)) * (b))

/* [nitr8]: Disabled - use global "PAGE_SIZE_NO_ECC" from the flash header file instead */
/* #define PAGESIZE     0x800 */

#define CACERTSIZE   0x400
#define CPCERTSIZE   0x300
#define XSCERTSIZE   0x300

/* [nitr8]: Remove it from the inside of the function to the outside of the
	    function and make it static so we can free the memory after usage */
static boot2 *b2 = NULL;

/* [nitr8]: Make static */
/* boot2* ReadBoot2NonECC(const char* filename) */
static boot2* ReadBoot2NonECC(const char* filename)
{
	u32 bytes_read;
	int ret = 0;
	char err_reason[30];
	u32 amount = 0;

	/* [nitr8]: Remove it from the inside of the function to the outside of the
		    function and make it static so we can free the memory after usage */
	/* boot2 *b2; */

	FILE *fp = fopen(filename, "rb");

	if (!CheckFile(fp, filename))
		return NULL;

	/* [nitr8]: MEMORY LEAK NOW FIXED */
	b2 = (boot2 *)malloc(sizeof(boot2));

	/* [nitr8]: Check memory allocation (don't use "ELSE" here...) */
	if (!b2)
	{
		strcpy(err_reason, " ");
		amount = b2->contentSize;
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2, 0, sizeof(boot2));

	/* Get headerLen, dataOffset, certsLen etc */
	bytes_read = fread(b2, 1, 0x14, fp);

	/* [nitr8]: Compare the amount of bytes read against the expected size */
	if (bytes_read != 0x14)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't read 0x%x bytes from file %s\n", __FUNCTION__, 0x14, filename);
		goto abort;
	}

	/* [nitr8]: This is where things start to change */
	/* Skip to 0x20 */
	ret = fseek(fp, b2->headerLen, SEEK_SET);

	/* [nitr8]: Check the return code of the call to FSEEK() */
	if (ret != 0)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't seek to offset 0x%x in file %s\n", __FUNCTION__, b2->headerLen, filename);
		goto abort;
	}

	b2->certs   = (certificates *)malloc(sizeof(certificates));

	if (!b2->certs)
	{
		strcpy(err_reason, " for the certificates");
		amount = sizeof(certificates);
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->certs, 0, sizeof(certificates));

	b2->tik     = (signed_blob *)alloc_aligned(b2->tikLen);

	if (!b2->tik)
	{
		strcpy(err_reason, " for the ticket");
		amount = b2->tikLen;
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->tik, 0, b2->tikLen);

	b2->TMD     = (signed_blob *)alloc_aligned(b2->TMDLen);

	if (!b2->TMD)
	{
		strcpy(err_reason, " for the TMD");
		amount = b2->TMDLen;
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->TMD, 0, b2->TMDLen);

	b2->certs->ca_cert = (signed_blob *)alloc_aligned(CACERTSIZE);

	if (!b2->certs->ca_cert)
	{
		strcpy(err_reason, " for the CA certificate");
		amount = CACERTSIZE;
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->certs->ca_cert, 0, CACERTSIZE);

	b2->certs->cp_cert = (signed_blob *)alloc_aligned(CPCERTSIZE);

	if (!b2->certs->cp_cert)
	{
		strcpy(err_reason, " for the CP certificate");
		amount = CPCERTSIZE;
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->certs->cp_cert, 0, CPCERTSIZE);

	b2->certs->xs_cert = (signed_blob *)alloc_aligned(XSCERTSIZE);

	if (!b2->certs->xs_cert)
	{
		strcpy(err_reason, " for the XS certificate");
		amount = XSCERTSIZE;
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->certs->xs_cert, 0, XSCERTSIZE);

	bytes_read = fread(b2->certs->ca_cert, 1, CACERTSIZE, fp); /* Using hardcoded sizes for CA, CP and XS */

	/* [nitr8]: Compare the amount of bytes read against the expected size */
	if (bytes_read != CACERTSIZE)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't read 0x%x bytes from file %s\n", __FUNCTION__, CACERTSIZE, filename);
		goto abort;
	}

	bytes_read = fread(b2->certs->cp_cert, 1, CPCERTSIZE, fp);

	/* [nitr8]: Compare the amount of bytes read against the expected size */
	if (bytes_read != CPCERTSIZE)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't read 0x%x bytes from file %s\n", __FUNCTION__, CPCERTSIZE, filename);
		goto abort;
	}

	bytes_read = fread(b2->certs->xs_cert, 1, XSCERTSIZE, fp);

	/* [nitr8]: Compare the amount of bytes read against the expected size */
	if (bytes_read != XSCERTSIZE)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't read 0x%x bytes from file %s\n", __FUNCTION__, XSCERTSIZE, filename);
		goto abort;
	}

	bytes_read = fread(b2->tik, 1, b2->tikLen,   fp);

	/* [nitr8]: Compare the amount of bytes read against the expected size */
	if (bytes_read != b2->tikLen)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't read 0x%x bytes from file %s\n", __FUNCTION__, b2->tikLen, filename);
		goto abort;
	}

	bytes_read = fread(b2->TMD, 1, b2->TMDLen,   fp);

	/* [nitr8]: Compare the amount of bytes read against the expected size */
	if (bytes_read != b2->TMDLen)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't read 0x%x bytes from file %s\n", __FUNCTION__, b2->TMDLen, filename);
		goto abort;
	}

	b2->contentSize = ALIGN(b2->TMD[0x7C], 16); /* Align to 16 */
	/* Address 0x7C (0x1F0, unaligned) of TMD contains content size */

	b2->certs->tik_cert = (signed_blob *)alloc_aligned(CACERTSIZE + XSCERTSIZE);

	if (!b2->certs->tik_cert)
	{
		strcpy(err_reason, " for the TIK certificate");
		amount = (CACERTSIZE + XSCERTSIZE);
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->certs->tik_cert, 0, CACERTSIZE + XSCERTSIZE);

	memcpy(b2->certs->tik_cert, b2->certs->xs_cert, XSCERTSIZE);
	memcpy(((u8*)(b2->certs->tik_cert)) + XSCERTSIZE, b2->certs->ca_cert, CACERTSIZE);
	/* Ticket Cert = XS + CA (concatenated) */
									
	b2->certs->TMD_cert = (signed_blob *)alloc_aligned(CACERTSIZE + CPCERTSIZE);

	if (!b2->certs->TMD_cert)
	{
		strcpy(err_reason, " for the TMD certificate");
		amount = (CACERTSIZE + CPCERTSIZE);
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->certs->TMD_cert, 0, CACERTSIZE + CPCERTSIZE);

	memcpy(b2->certs->TMD_cert, b2->certs->cp_cert, CPCERTSIZE);
	memcpy(((u8*)(b2->certs->TMD_cert)) + CPCERTSIZE, b2->certs->ca_cert, CACERTSIZE);
	/* TMD Cert = CP + CA (concatenated) */

	b2->content = (u8 *)alloc_aligned(b2->contentSize);

	if (!b2->content)
	{
		strcpy(err_reason, " for content");
		amount = b2->contentSize;
		goto err;
	}

	/* [nitr8]: Clear the memory first before using it when allocating memory by calling MALLOC() !!! */
	memset(b2->content, 0, b2->contentSize);

	ret = fseek(fp, b2->dataOffset, SEEK_SET);

	/* [nitr8]: Check the return code of the call to FSEEK() */
	if (ret != 0)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't seek to offset 0x%x in file %s\n", __FUNCTION__, b2->dataOffset, filename);
		goto abort;
	}

	bytes_read = fread(b2->content, 1, b2->contentSize, fp);

	/* [nitr8]: Compare the amount of bytes read against the expected size */
	if (bytes_read != b2->contentSize)
	{
		gecko_printf("[%s]: - [ERROR]: Couldn't read 0x%x bytes from file %s\n", __FUNCTION__, b2->contentSize, filename);
		goto abort;
	}

	fclose(fp);

	return b2;

err:

	gecko_printf("[%s]: - [ERROR]: Couldn't allocate 0x%x bytes of memory%s\n", __FUNCTION__, amount, err_reason);

abort:

	fclose(fp);

	return NULL;
}

/* [nitr8]: Make static and rework for PROPER error handling... */
/* WAD* ReadWAD(const char* filename) */
static WAD* ReadWAD(const char* filename)
{
	WAD *wad;

	FILE *fp = fopen(filename, "rb");

	if (!CheckFile(fp, filename))
		return NULL;
	
	/* [nitr8]: POTENTIAL MEMORY LEAK */
	wad = (WAD *)malloc(sizeof(WAD));

	fread(wad, 1, WAD_HEADER_SIZE, fp);	
	
	wad->certs   = (certificates *)malloc(sizeof(certificates));
	wad->tik     = (signed_blob *)alloc_aligned(wad->tikLen);
	wad->TMD     = (signed_blob *)alloc_aligned(wad->TMDLen);
	wad->content = (u8 *)alloc_aligned(wad->contentSize);
	
	wad->certs->ca_cert = (signed_blob *)alloc_aligned(CACERTSIZE);
	wad->certs->cp_cert = (signed_blob *)alloc_aligned(CPCERTSIZE);
	wad->certs->xs_cert = (signed_blob *)alloc_aligned(XSCERTSIZE);
	
	fread(wad->certs->ca_cert, 1, CACERTSIZE, fp); /* Using hardcoded sizes for CA, CP and XS */
	fread(wad->certs->cp_cert, 1, CPCERTSIZE, fp); /* Assuming certsLen == 0x0A00 */
	fread(wad->certs->xs_cert, 1, XSCERTSIZE, fp);
	fread(wad->tik, 1, wad->tikLen, fp);
	fseek(fp, ALIGN(wad->tikLen, WAD_HEADER_SIZE) - wad->tikLen, SEEK_CUR);
	fread(wad->TMD, 1, wad->TMDLen, fp);
	fseek(fp, ALIGN(wad->TMDLen, WAD_HEADER_SIZE) - wad->TMDLen, SEEK_CUR);
	fread(wad->content, 1, wad->contentSize, fp);
	
	wad->certs->tik_cert = (signed_blob *)alloc_aligned(CACERTSIZE + XSCERTSIZE);
	memcpy(wad->certs->tik_cert, wad->certs->xs_cert, XSCERTSIZE);
	memcpy(((u8*)(wad->certs->tik_cert)) + XSCERTSIZE, wad->certs->ca_cert, CACERTSIZE);
	/* Ticket Cert = XS + CA (concatenated) */
	
	wad->certs->TMD_cert = (signed_blob *)alloc_aligned(CACERTSIZE + CPCERTSIZE);
	memcpy(wad->certs->TMD_cert, wad->certs->cp_cert, CPCERTSIZE);
	memcpy(((u8*)(wad->certs->TMD_cert)) + CPCERTSIZE, wad->certs->ca_cert, CACERTSIZE);
	/* TMD Cert = CP + CA (concatenated) */
	
	fclose(fp);

	return wad;
}

/* [nitr8]: Make static and rework error handling / return codes COMPLETELY */
/* boot2 *ReadBoot2(const char *filename) */
static boot2 *ReadBoot2(const char *filename)
{
	/* [nitr8]: Number 1: error - 'for' loop initial declarations are only allowed in C99 mode
	   [nitr8]: Number 2: get rid of warning about signed and unsigned comparison */
	u32 i;
	u32 filelen;
	FILE *out;
	u8 *page;
	u32 pages;

	/* [nitr8]: Added these */
	int bytes_read;
	int bytes_written;
	int ret = 0;
	char *out_file = "/boot2/boot2_noecc.bin";

	FILE *fp = fopen(filename, "rb");

	if (!CheckFile(fp, filename))
	{
		ret = -1;
		goto err;
	}
	
	filelen = GetSizeOfFile(fp);

	if ((filelen != RAWBOOT2SIZE) && (filelen != (2 * RAWBOOT2SIZE)))
	{
		return ReadBoot2NonECC(filename);  
	}
	
	out = fopen(out_file, "wb"); /* Write output file without ECC */

	/* [nitr8]: Add a check if the output file was opened successfully */
	if (!CheckFile(out, out_file))
	{
		ret = -2;
		goto err;
	}

	page  = (u8 *)malloc(PAGE_SIZE_NO_ECC + NAND_ECC_DATA_SIZE);

	/* [nitr8]: Also, add a check if the buffer was actually allocated */
	if (page)
	{
		pages = filelen / RAWBOOT2SIZE * NAND_ECC_DATA_SIZE;
		
		/* [nitr8]: error: 'for' loop initial declarations are only allowed in C99 mode
		for(int i=0; i<pages; i++){ */
		for (i = 0; i < pages; i++)
		{
			bytes_read = fread(page, 1, PAGE_SIZE_NO_ECC + NAND_ECC_DATA_SIZE, fp);

			/* [nitr8]: Again, check if the bytes read match the actual size of the file */
			if (bytes_read != (PAGE_SIZE_NO_ECC + NAND_ECC_DATA_SIZE))
			{
				ret = -3;
				goto err;
			}

			bytes_written = fwrite(page, 1, PAGE_SIZE_NO_ECC, out);

			/* [nitr8]: And also check if the bytes written match the actual size of the file minus the ECC data for each page */
			if (bytes_written != PAGE_SIZE_NO_ECC)
			{
				ret = -4;
				goto err;
			}
		}
	}
	else
	{
		ret = -5;
	}

err:

	switch (ret)
	{
		case 0:
			/* [nitr8]: Clean everything up beforehand */
			break;

		case -1:
			gecko_printf("[ERROR]: Couldn't open input file %s\n", filename);
			goto out;

		case -2:
			gecko_printf("[ERROR]: Couldn't open output file %s\n", out_file);
			fclose(fp);
			goto close_in;

		case -3:
			gecko_printf("[ERROR]: Bytes read (%d) do not match expected size of the input file %s\n", bytes_read, filename);
			goto free;

		case -4:
			gecko_printf("[ERROR]: Bytes written (%d) do not match binary input file %s without the ECC data\n", bytes_written, filename);
			goto free;

		case -5:
			gecko_printf("[ERROR]: Couldn't allocate 0x%x bytes of memory for buffer\n", PAGE_SIZE_NO_ECC + NAND_ECC_DATA_SIZE);
			goto close_out;

		default:
			gecko_printf("Unknown error %d\n", ret);

			/* [nitr8]: ABSOLUTELY NO EXCEPTIONS! */
			goto free;
	}

free:

	/* [nitr8]: variable "page" was never freed previously... (MEMORY LEAK) */
	if (page)
		free(page);

close_out:

	if (out)
		fclose(out);

close_in:

	if (fp)
		fclose(fp);

out:

	/* [nitr8]: Clean everything up beforehand */
	if (ret == 0)
		goto success;

	return NULL;

success:

	return ReadBoot2NonECC("/boot2/boot2_noecc.bin");
}

/* [nitr8]: Make static */
/* s32 InstallRawBoot2(const char* filename) */
static s32 UNUSED_InstallRawBoot2(const char* filename)
{
	s32 fd;
	u32 length;
	u32 bytes_read;
	u32 bytes_written;
	u8 *buffer;
	s32 ret = 0;

	FILE *fp = fopen(filename, "rb");

	if (fp)
	{
		//gecko_printf("%08x\n", nand_status());
		//gecko_printf("%08x\n", nand_getid());

		ret = ISFS_Initialize();

		if (ret != 0)
		{
			gecko_printf("[ERROR]: Unable to initialize NAND\n");
			goto out;
		}

		ret = fseek(fp, 0, SEEK_END);

		if (ret != 0)
		{
			gecko_printf("[ERROR]: Unable to seek to the end of file %s\n", filename);
			goto out;
		}

		length = ftell(fp);

		ret = fseek(fp, 0, SEEK_SET);

		if (ret != 0)
		{
			gecko_printf("[ERROR]: Unable to seek to the start of file %s\n", filename);
			goto out;
		}

		buffer = memalign(16, length);

		if (buffer)
		{
			bytes_read = fread(buffer, 1, length, fp);

			if (bytes_read != length)
			{
				gecko_printf("[ERROR]: Unable to read %d bytes from file %s (%d bytes expected)\n", bytes_read, filename, length);
				goto out;
			}

			ret = ISFS_CreateFile("/tmp/boot.sys", 0, 3, 3, 0);

			if (ret != 0)
			{
				gecko_printf("[ERROR]: Unable to create file %s (%d)\n", "/tmp/boot.sys", ret);
				goto out;
			}

			fd = ISFS_Open("/tmp/boot.sys", 2);

			if (fd < 0)
			{
				gecko_printf("[ERROR]: Unable to open %s for writing (%d)\n", "/tmp/boot.sys", fd);
				goto out;
			}

			ret = ISFS_Write(fd, (u8 *)length, sizeof(u32));

			if (ret != sizeof(u32))
			{
				gecko_printf("[ERROR]: Unable to write %d bytes to %s (%d)\n", sizeof(u32), "/tmp/boot.sys", ret);
				goto out;
			}

			ret = ISFS_Close(fd);

			if (ret != 0)
			{
				gecko_printf("[ERROR]: Unable to close file %s (%d)\n", "/tmp/boot.sys", ret);
				goto out;
			}

			//fd = flash_fd; //IOS_Open("/dev/flash", 0);

			if (flash_fd < 0)
			{
				gecko_printf("[ERROR]: Unable to open /dev/flash for writing (%d)\n", fd);
				ret = fd;
				goto out;
			}
			
			ret = 0;
			
			
			
#if 0

			ret = IOS_Seek(flash_fd, 64, 0);

			if (ret < 0)
			{
				gecko_printf("[ERROR]: Unable to seek within /dev/flash for writing (%d)\n", fd);
				goto out;
			}

			bytes_written = IOS_Write(flash_fd, buffer, length);

			if (bytes_written != length)
			{
				gecko_printf("[ERROR]: Unable to write %d bytes to NAND (%d)\n", length, bytes_written);
				goto out;
			}

			ret = IOS_Close(flash_fd);

			if (ret != 0)
			{
				gecko_printf("[ERROR]: Unable to close file %s (%d)\n", "/dev/boot2", ret);
				goto out;
			}
#endif

			free(buffer);
		}
		else
		{
			gecko_printf("[ERROR]: Unable to allocate a buffer of size %d bytes for file %s\n", length, filename);
			goto out;
		}
	}
	else
		gecko_printf("[ERROR]: Unable to open file %s\n", filename);

	goto out;
#if 0
	FILE *ticket = fopen("ticket", "wb");
	FILE *cert_tik = fopen("cert_tik", "wb");
	FILE *tmd = fopen("tmd", "wb");
	FILE *cert_tmd = fopen("cert_tmd", "wb");
	FILE *content = fopen("content", "wb");

	boot2 *b2 = ReadBoot2(filename);
#endif
	b2 = ReadBoot2(filename);
	
	if (b2 == NULL)
		return MISSING_FILE;
#if 0
	gecko_printf("\n");
	gecko_printf("tik length = %08x\n", b2->tikLen);
	gecko_printf("\n");
	hexdump(0, b2->certs->tik_cert, CACERTSIZE + XSCERTSIZE);
	gecko_printf("\n");
	gecko_printf("tik cert length = %08x\n", CACERTSIZE + XSCERTSIZE);
	gecko_printf("\n");
	hexdump(0, b2->TMD, b2->TMDLen);
	gecko_printf("\n");
	gecko_printf("tmd length = %08x\n", b2->TMDLen);
	gecko_printf("\n");
	hexdump(0, b2->certs->TMD_cert, CACERTSIZE + CPCERTSIZE);
	gecko_printf("\n");
	gecko_printf("tmd cert length = %08x\n", CACERTSIZE + CPCERTSIZE);
	gecko_printf("\n");
	hexdump(0, b2->content, b2->contentSize);
	gecko_printf("\n");
	gecko_printf("content_size = %08x\n", b2->contentSize);
	gecko_printf("\n");

	fwrite(b2->tik, 1, b2->tikLen, ticket);
	fwrite(b2->certs->tik_cert, 1, CACERTSIZE + XSCERTSIZE, cert_tik);
	fwrite(b2->TMD, 1, b2->TMDLen, tmd);
	fwrite(b2->certs->TMD_cert, 1, CACERTSIZE + CPCERTSIZE, cert_tmd);
	fwrite(b2->content, 1, b2->contentSize, content);

	fclose(ticket);
	fclose(cert_tik);
	fclose(tmd);
	fclose(cert_tmd);
	fclose(content);
#endif
//goto skip;

	ret = ES_ImportBoot(b2->tik,
							b2->tikLen,
							b2->certs->tik_cert,
							CACERTSIZE + XSCERTSIZE,
							b2->TMD,
							b2->TMDLen,
							b2->certs->TMD_cert,
							CACERTSIZE + CPCERTSIZE,
							b2->content,
							b2->contentSize
							);
//skip:
	/* [nitr8]: Now, from here on free all the memory occupied
		    in reverse direction within this struct... */

	if (b2->certs->ca_cert)
	{
		gecko_printf("freeing b2->certs->ca_cert\n");
		free(b2->certs->ca_cert);
	}

	if (b2->certs->cp_cert)
	{
		gecko_printf("freeing b2->certs->cp_cert\n");
		free(b2->certs->cp_cert);
	}

	if (b2->certs->xs_cert)
	{
		gecko_printf("freeing b2->certs->xs_cert\n");
		free(b2->certs->xs_cert);
	}

	if (b2->certs->tik_cert)
	{
		gecko_printf("freeing b2->tik_cert\n");
		free(b2->certs->tik_cert);
	}

	if (b2->certs->TMD_cert)
	{
		gecko_printf("freeing b2->TMD_cert\n");
		free(b2->certs->TMD_cert);
	}

	if (b2->certs)
	{
		gecko_printf("freeing b2->certs\n");
		free(b2->certs);
	}

	if (b2->tik)
	{
		gecko_printf("freeing b2->tik\n");
		free(b2->tik);
	}

	if (b2->TMD)
	{
		gecko_printf("freeing b2->TMD\n");
		free(b2->TMD);
	}

	if (b2->content)
	{
		gecko_printf("freeing b2->content\n");
		free(b2->content);
	}

	if (b2)
	{
		gecko_printf("freeing b2\n");
		free(b2);
	}
out:
	return ret;
}

s32 InstallRawBoot2(const char* filename){
	gecko_printf("[%s]: about to read provided boot2: %s\n", __FUNCTION__, filename);
	boot2 *b2 = ReadBoot2(filename);
	
	if(b2 == NULL)
		return MISSING_FILE;
	
	gecko_printf("[%s]: about to install provided boot2: %s\n", __FUNCTION__, filename);
	s32 ret = ES_ImportBoot(b2->tik,
							b2->tikLen,
							b2->certs->tik_cert,
							CACERTSIZE + XSCERTSIZE,
							b2->TMD,
							b2->TMDLen,
							b2->certs->TMD_cert,
							CACERTSIZE + CPCERTSIZE,
							b2->content,
							b2->contentSize
							);
	
	/* [root1024]: TODO: FREE the struct! */
	
	return ret;
}

s32 InstallWADBoot2(const char* filename)
{
	s32 ret;
	WAD *wad = ReadWAD(filename);
	
	if (wad == NULL)
		return MISSING_FILE;
	
	ret = ES_ImportBoot(wad->tik,
							wad->tikLen,
							wad->certs->tik_cert,
							CACERTSIZE + XSCERTSIZE,
							wad->TMD,
							wad->TMDLen,
							wad->certs->TMD_cert,
							CACERTSIZE + CPCERTSIZE,
							wad->content,
							wad->contentSize
							);
	
	return ret;
}

static u8 SDBOOT_hash_ECC[] = { 0xe5,0xde,0x10,0xbf,0xec,0xcc,0x07,0x21,0x80,0xc3,0xa4,0x0d,0xea,0xe8,0xa7,0x2f,0xff,0x16,0xe5,0x0f,0x66,0xe1,0x84,0x8f,0xb3,0xd5,0x5b,0xfc,0x12,0x89,0xa1,0x65 };
/* static u8 SDBOOT_hash_no_ECC[] = { 0x39,0x4E,0x73,0x9E,0x23,0x47,0xCF,0x98,0x90,0x5B,0x58,0x4D,0xC6,0xC6,0x56,0xD9,0xEC,0xD0,0x7D,0xCA,0x42,0xB4,0x6F,0xD7,0x91,0xAF,0x2A,0x6D,0x91,0x91,0x49,0xF4 }; */

s32 InstallSDBoot(const char* filename)
{
	int ret;

	ret = CheckFileHash(filename, SDBOOT_hash_ECC);

	if (ret != 0)
	{
#if 0
		/* [nitr8]: Optional - Check a file without the ECC data included */
		ret = CheckFileHash(filename, SDBOOT_hash_no_ECC);

		if (ret != 0)
		{
#endif
			gecko_printf("[ERROR]: ret = %d\n", ret);
			return ret;
#if 0
		}
#endif
	}

	/* TODO: Add SD file check */

	return InstallRawBoot2(filename);
}

s32 InstallNANDBoot(const char* filename, const char* payload)
{
	s32 ret;

	/* Let's check if the payload hash is correct ...
	   It's not a good idea to install nandboot without a non-functional payload :)

	   59d220c80c4aa7159e71a699ec3785d5669a479be2699264c1d482d944651525 */
	
	/* u8 expectedHash[] = { 0x59,0xD2,0x20,0xC8,0x0C,0x4A,0xA7,0x15,0x9E,0x71,0xA6,0x99,0xEC,0x37,0x85,0xD5,0x66,0x9A,0x47,0x9B,0xE2,0x69,0x92,0x64,0xC1,0xD4,0x82,0xD9,0x44,0x65,0x15,0x25 }; */

	/* [nitr8]: remove the size right here - move it directly into CheckFileHash itself */
	/*int k = CheckFileHash(payload, expectedHash, 3*RAWBOOT2SIZE);
	int k = CheckFileHash(payload, expectedHash);

	if (k)
	{
		if (k == 2)
			return MISSING_FILE;
		else
			return HASH_MISMATCH;
	}*/
	
	/* Enable /dev/flash access (disabling /dev/boot2) */
//	Enable_DevFlash();
	
	/* Check if blocks are good before flashing */
	ret = checkBlocks(1, 4);

	if (ret > 0)
		return BAD_BOOT_BLOCKS;
		
	/* Flash nandboot */
	ret = flashFile(filename, 1, 1, NULL);

	if (ret < 0)
		return ret;
	
	/* Flash payload */
	ret = flashFile(payload, 2, 4, NULL);

	if (ret < 0)
		return ret;

	/* Erase boot2 backup copy */
	ret = eraseBlocks(5, 7);

	if (ret < 0)
		return ret;

	return 0;
}

s32 BackupBoot2Blocks(const char* filename)
{
	return dumpBlocks(filename, 1, 7);
}

s32 RestoreBoot2Blocks(const char* filename)
{
	return flashFile(filename, 1, 7, NULL);
}

/* [nitr8]: Add function to read ALL the boot2 blocks in a row - with the ECC data and blockmap data included */
s32 ReadBoot2Blocks(u8 *buffer, int start_block, int end_block)
{
	int rv;
	int ret;
	u32 start_page;
	u32 end_page;
	u32 num_pages;
	u32 i = 0;
	u32 j = 0;
	u32 block_size = RAWBOOT2SIZE;

	if (start_block < 1)
	{
		gecko_printf("[ERROR]: Start block must be >= 1\n");
		return -1;
	}

	if (end_block > 7)
	{
		gecko_printf("[ERROR]: End block must be <= 7\n");
		return -2;
	}

	if (start_block > end_block)
	{
		gecko_printf("[ERROR]: Invalid range for start block and end block\n");
		return -3;
	}

	gecko_printf("[%s]: IOS_Open returned %x\n", __FUNCTION__, flash_fd);

	if (!buffer)
	{
		gecko_printf("[ERROR]: Couldn't allocate 0x%x bytes for buffer\n", block_size * end_block);
		return -4;
	}

	ret = 0;
	start_page = (start_block * BLOCK_SIZE);
	end_page = ((end_block * BLOCK_SIZE) + BLOCK_SIZE);
	num_pages = (end_page - start_page);

	if (num_pages == 0)
	{
		gecko_printf("[ERROR]: No pages to read\n");
		return -5;
	}

	/* Clear the buffer first */
	memset(buffer, 0, block_size * end_block);

	gecko_printf("Going to read 0x%x pages (%d block%s)\n", num_pages, num_pages / BLOCK_SIZE, (end_block - start_block) > 1 ? "s" : "");

	/* WARNING: Making use of call to "Disable_FlashECCCheck" MUST A-L-W-A-Y-S ENSURE that it's */
	/* 	    being turned back on afterwards BEFORE anything else continues to read data from NAND !!!*/
	/* Disable_FlashECCCheck() */;

	for (j = 0; j < num_pages; j++)
	{
		if (i >= (block_size * end_block))
		{
			gecko_printf("[ERROR]: BUFFER OVERFLOW (0x%x vs. 0x%x)\n", i, block_size * end_block);
			ret = -6;
			break;
		}

		/* Back to start in the source (might not be neccessary at all but I feel a bit more comfortable with it) */
		rv = IOS_Seek(flash_fd, 0, 0);

		gecko_printf("[%s]: IOS_Seek returned %x (REWIND)\n", __FUNCTION__, rv);

		/* Go to offset "start_page + j" in the source */
		rv = IOS_Seek(flash_fd, start_page + j, 0);

		gecko_printf("[%s]: IOS_Seek returned %x (SEEK_SET)\n", __FUNCTION__, rv);

		gecko_printf("Reading page 0x%x from offset 0x%x in NAND flash to offset 0x%x in buffer\n", start_page + j, (start_page + j) * NAND_PAGE_SIZE, i + (BOOT2_START * RAWBOOT2SIZE));

		/* Read the data from the NAND flash to the destination (buffer) */
		rv = IOS_Read(flash_fd, buffer + i, NAND_PAGE_SIZE);

		/* Get error output from IOS_Read(...): return code -12 means ECC data corruption */
		if (rv < 0)
		{
			gecko_printf("[%s]: IOS_Read returned error ", __FUNCTION__);

			if (rv == -12)
				gecko_printf("ECC (%d)", rv);

			gecko_printf(" for page 0x%x (offset 0x%x)\n", start_page + j, (start_page + j) * NAND_PAGE_SIZE);
		}

		/* increase the buffer offset */
		i += NAND_PAGE_SIZE;
	}

	/* WARNING: Making use of the call to "Disable_FlashECCCheck" MUST A-L-W-A-Y-S ENSURE that it's */
	/* 	    being turned back on afterwards BEFORE anything else continues to read data from NAND !!!*/
	/* Enable_FlashECCCheck() */;

	return ret;
}

