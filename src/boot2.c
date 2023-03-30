/* RGD SDBoot Installer */

// Uses the same method as the DOP-Mii boot2 installation routine
#include "boot2.h"
#include "flash.h"

#define ALIGN(a,b) ((((a)+(b)-1)/(b))*(b))

#define RAWBOOT2SIZE 0x21000
#define PAGESIZE     0x800

#define CACERTSIZE   0x400
#define CPCERTSIZE   0x300
#define XSCERTSIZE   0x300

boot2* ReadBoot2NonECC(const char* filename){
	FILE *fp = fopen(filename, "rb");
	if(!CheckFile(fp, filename)) return NULL;

	boot2 *b2 = (boot2 *)malloc(sizeof(boot2));
		
	fread(b2, 1, 0x14, fp);             // Get headerLen, dataOffset, certsLen etc
	fseek(fp, b2->headerLen, SEEK_SET); // Skip to 0x20
	
	b2->certs   = (certificates *)malloc(sizeof(certificates));
	b2->tik     = (signed_blob *)alloc(b2->tikLen);
	b2->TMD     = (signed_blob *)alloc(b2->TMDLen);
	
	b2->certs->ca_cert = (signed_blob *)alloc(CACERTSIZE);
	b2->certs->cp_cert = (signed_blob *)alloc(CPCERTSIZE);
	b2->certs->xs_cert = (signed_blob *)alloc(XSCERTSIZE);
	
	fread(b2->certs->ca_cert, 1, CACERTSIZE, fp); // Using hardcoded sizes for CA, CP and XS
	fread(b2->certs->cp_cert, 1, CPCERTSIZE, fp);
	fread(b2->certs->xs_cert, 1, XSCERTSIZE, fp);
	fread(b2->tik, 1, b2->tikLen,   fp);
	fread(b2->TMD, 1, b2->TMDLen,   fp);
	
	b2->contentSize = ALIGN(b2->TMD[0x7C], 16); // Align to 16
	// Address 0x7C (0x1F0, unaligned) of TMD contains content size

	b2->certs->tik_cert = (signed_blob *)alloc(CACERTSIZE + XSCERTSIZE);
	memcpy(b2->certs->tik_cert, b2->certs->xs_cert, XSCERTSIZE);
	memcpy(((u8*)(b2->certs->tik_cert)) + XSCERTSIZE, b2->certs->ca_cert, CACERTSIZE);
	// Ticket Cert = XS + CA (concatenated)
	
	b2->certs->TMD_cert = (signed_blob *)alloc(CACERTSIZE + CPCERTSIZE);
	memcpy(b2->certs->TMD_cert, b2->certs->cp_cert, CPCERTSIZE);
	memcpy(((u8*)(b2->certs->TMD_cert)) + CPCERTSIZE, b2->certs->ca_cert, CACERTSIZE);
	// TMD Cert = CP + CA (concatenated)

	b2->content = (u8 *)alloc(b2->contentSize);
	fseek(fp, b2->dataOffset, SEEK_SET);
	fread(b2->content, 1, b2->contentSize, fp);
	
	fclose(fp);
	
	return b2;
}

WAD* ReadWAD(const char* filename){
	FILE *fp = fopen(filename, "rb");
	if(!CheckFile(fp, filename)) return NULL;
	
	WAD *wad = (WAD *)malloc(sizeof(WAD));
	
	fread(wad, 1, 0x40, fp);
	
	wad->certs   = (certificates *)malloc(sizeof(certificates));
	wad->tik     = (signed_blob *)alloc(wad->tikLen);
	wad->TMD     = (signed_blob *)alloc(wad->TMDLen);
	wad->content = (u8 *)alloc(wad->contentSize);
	
	wad->certs->ca_cert = (signed_blob *)alloc(CACERTSIZE);
	wad->certs->cp_cert = (signed_blob *)alloc(CPCERTSIZE);
	wad->certs->xs_cert = (signed_blob *)alloc(XSCERTSIZE);
	
	fread(wad->certs->ca_cert, 1, CACERTSIZE, fp); // Using hardcoded sizes for CA, CP and XS
	fread(wad->certs->cp_cert, 1, CPCERTSIZE, fp); // Assuming certsLen == 0x0A00
	fread(wad->certs->xs_cert, 1, XSCERTSIZE, fp);
	fread(wad->tik, 1, wad->tikLen, fp);
	fseek(fp, ALIGN(wad->tikLen, 0x40) - wad->tikLen, SEEK_CUR);
	fread(wad->TMD, 1, wad->TMDLen, fp);
	fseek(fp, ALIGN(wad->TMDLen, 0x40) - wad->TMDLen, SEEK_CUR);
	fread(wad->content, 1, wad->contentSize, fp);
	
	wad->certs->tik_cert = (signed_blob *)alloc(CACERTSIZE + XSCERTSIZE);
	memcpy(wad->certs->tik_cert, wad->certs->xs_cert, XSCERTSIZE);
	memcpy(((u8*)(wad->certs->tik_cert)) + XSCERTSIZE, wad->certs->ca_cert, CACERTSIZE);
	// Ticket Cert = XS + CA (concatenated)
	
	wad->certs->TMD_cert = (signed_blob *)alloc(CACERTSIZE + CPCERTSIZE);
	memcpy(wad->certs->TMD_cert, wad->certs->cp_cert, CPCERTSIZE);
	memcpy(((u8*)(wad->certs->TMD_cert)) + CPCERTSIZE, wad->certs->ca_cert, CACERTSIZE);
	// TMD Cert = CP + CA (concatenated)
	
	fclose(fp);
	
	return wad;
}

boot2 *ReadBoot2(const char *filename){
	FILE *fp = fopen(filename, "rb");
	if(!CheckFile(fp, filename)) return NULL;
	
	u32 filelen = filesize(fp);
	if(filelen != RAWBOOT2SIZE && filelen != 2*RAWBOOT2SIZE){
		return ReadBoot2NonECC(filename);  
	}
	
	u8 pageCount = filelen / RAWBOOT2SIZE * 0x40;
	
	boot2 *b2 = (boot2 *)malloc(sizeof(boot2));
	u8 *pages = (u8 *)malloc(PAGESIZE * pageCount);
	u8 *singlePage = (u8 *)malloc(PAGESIZE);
	u8 pageIndex = 0;
	
	while(!feof(fp)){ // REMOVE ECC
		fread(singlePage, 1, PAGESIZE, fp);
		for(int i=0; i<PAGESIZE; i++)
			pages[PAGESIZE*pageIndex+i] = singlePage[i];
		fread(singlePage, 1, 0x40, fp); // Ignore ECC
		pageIndex++;
	}
	
	FILE *out = fopen("/boot2/boot2_noecc.bin", "wb"); // Write output file without ECC
	fwrite(pages, 1, PAGESIZE * pageCount, out);
	fclose(out);
	
	fp = fopen("/boot2/boot2_noecc.bin", "rb");
	if(!CheckFile(fp, "/boot2/boot2_noecc.bin")){ // Read file without ECC
		printf("Something went wrong... Aborting\n");
		WaitExit();
	}
		
	fread(b2, 1, 0x14, fp);             // Get headerLen, dataOffset, certsLen etc
	fseek(fp, b2->headerLen, SEEK_SET); // Skip to 0x20
	
	b2->certs   = (certificates *)malloc(sizeof(certificates));
	b2->tik     = (signed_blob *)alloc(b2->tikLen);
	b2->TMD     = (signed_blob *)alloc(b2->TMDLen);
	
	b2->certs->ca_cert = (signed_blob *)alloc(CACERTSIZE);
	b2->certs->cp_cert = (signed_blob *)alloc(CPCERTSIZE);
	b2->certs->xs_cert = (signed_blob *)alloc(XSCERTSIZE);
	
	fread(b2->certs->ca_cert, 1, CACERTSIZE, fp); // Using hardcoded sizes for CA, CP and XS
	fread(b2->certs->cp_cert, 1, CPCERTSIZE, fp);
	fread(b2->certs->xs_cert, 1, XSCERTSIZE, fp);
	fread(b2->tik, 1, b2->tikLen,   fp);
	fread(b2->TMD, 1, b2->TMDLen,   fp);
	
	b2->contentSize = ALIGN(b2->TMD[0x7C], 16); // Align to 16
	// Address 0x7C (0x1F0, unaligned) of TMD contains content size

	b2->certs->tik_cert = (signed_blob *)alloc(CACERTSIZE + XSCERTSIZE);
	memcpy(b2->certs->tik_cert, b2->certs->xs_cert, XSCERTSIZE);
	memcpy(((u8*)(b2->certs->tik_cert)) + XSCERTSIZE, b2->certs->ca_cert, CACERTSIZE);
	// Ticket Cert = XS + CA (concatenated)
	
	b2->certs->TMD_cert = (signed_blob *)alloc(CACERTSIZE + CPCERTSIZE);
	memcpy(b2->certs->TMD_cert, b2->certs->cp_cert, CPCERTSIZE);
	memcpy(((u8*)(b2->certs->TMD_cert)) + CPCERTSIZE, b2->certs->ca_cert, CACERTSIZE);
	// TMD Cert = CP + CA (concatenated)

	b2->content = (u8 *)alloc(b2->contentSize);
	fseek(fp, b2->dataOffset, SEEK_SET);
	fread(b2->content, 1, b2->contentSize, fp);
	
	fclose(fp);

	free(pages);
	free(singlePage);
	
	return b2;
}

s32 InstallRawBoot2(const char* filename){
	boot2 *b2 = ReadBoot2(filename);
	
	if(b2 == NULL)
		return MISSING_FILE;
	
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
	
	return ret;
}

s32 InstallWADBoot2(const char* filename){
	WAD *wad = ReadWAD(filename);
	
	if(wad == NULL)
		return MISSING_FILE;
	
	s32 ret = ES_ImportBoot(wad->tik,
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

s32 InstallSDBoot(const char* filename){
	return InstallRawBoot2(filename);
}

s32 InstallNANDBoot(const char* filename, const char* payload){
	// Let's check if the payload hash is correct ...
	// It's not a good idea to install nandboot without a non-functional payload :)

	u8 expectedHash[] = {0xa7,0xf6,0x41,0x30,0xc6,0xda,0xc4,0x99,0x95,0xe3,0xe6,0xee,0x10,0x26,0xc8,0xcb,0x29,0x4d,0x9d,0xb3,0xcb,0x18,0x09,0x79,0x52,0x50,0x6c,0x49,0x46,0x52,0xaa,0x01};
	int k = CheckFileHash(payload, expectedHash, RAWBOOT2SIZE);

	if(k){
		if(k == 2)
			return MISSING_FILE;
		else
			return HASH_MISMATCH;
	}

	s32 ret = InstallRawBoot2(filename);
	if(ret < 0)
		return ret;

	ret = flashFile(payload, 2, 2, NULL);
	if(ret < 0)
		return ret;

	// Erase blocks 3-6. No need to erase the boot2 backup copy
	ret = eraseBlocks(3, 6);
	if(ret < 0)
		return ret;

	return 0;
}

s32 BackupBoot2Blocks(const char* filename){
	return dumpBlocks(filename, 1, 7);
}

s32 RestoreBoot2Blocks(const char* filename){
	return flashFile(filename, 1, 7, NULL);
}
