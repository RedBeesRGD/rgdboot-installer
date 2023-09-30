/* RGD SDBoot Installer */

// Uses the same method as the DOP-Mii boot2 installation routine
#include "boot2.h"
#include "flash.h"
#include "runtimeiospatch.h"

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
	
	FILE *out = fopen("/boot2/boot2_noecc.bin", "wb"); // Write output file without ECC
	u8 *page  = (u8 *)malloc(PAGESIZE+0x40);
	u32 pages = filelen / RAWBOOT2SIZE * 0x40;
	
	for(int i=0; i<pages; i++){
		fread(page, 1, PAGESIZE+0x40, fp);
		fwrite(page, 1, PAGESIZE, out);
	}
	
	fclose(fp);
	fclose(out);
		
	return ReadBoot2NonECC("/boot2/boot2_noecc.bin");
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

	// 59d220c80c4aa7159e71a699ec3785d5669a479be2699264c1d482d944651525
	
	u8 expectedHash[] = {0x59,0xD2,0x20,0xC8,0x0C,0x4A,0xA7,0x15,0x9E,0x71,0xA6,0x99,0xEC,0x37,0x85,0xD5,0x66,0x9A,0x47,0x9B,0xE2,0x69,0x92,0x64,0xC1,0xD4,0x82,0xD9,0x44,0x65,0x15,0x25};
	int k = CheckFileHash(payload, expectedHash, 3*RAWBOOT2SIZE);

	if(k){
		if(k == 2)
			return MISSING_FILE;
		else
			return HASH_MISMATCH;
	}
	
	//ret = checkBlocks(1, 7);
	s32 ret = checkBlocks(2, 4);
	if(ret > 0)
		return BAD_BOOT_BLOCKS;
	
	
	Enable_DevBoot2();
	ret = InstallRawBoot2(filename);
	if(ret < 0)
		return ret;
	
	
	Enable_DevFlash();
	//ret = flashFile(payload, 2, 2, NULL);
	ret = flashFile(payload, 2, 4, NULL);
	if(ret < 0)
		return ret;

	// Erase blocks 3-6. No need to erase the boot2 backup copy

	/*ret = eraseBlocks(5, 7);
	if(ret < 0)
		return ret;*/

	return 0;
}

s32 BackupBoot2Blocks(const char* filename){
	return dumpBlocks(filename, 1, 7);
}

s32 RestoreBoot2Blocks(const char* filename){
	return flashFile(filename, 1, 7, NULL);
}
