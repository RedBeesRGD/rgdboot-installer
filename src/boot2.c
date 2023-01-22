// Uses the same method as the DOP-Mii boot2 installation routine

#include "boot2.h"

#define ALIGN(a,b) ((((a)+(b)-1)/(b))*(b))

#define RAWBOOT2SIZE 0x21000
#define PAGESIZE     0x800

#define CACERTSIZE   0x400
#define CPCERTSIZE   0x300
#define XSCERTSIZE   0x300

WAD *readWAD(const char *filename){
	FILE *fp = fopen(filename, "rb");
	if(!checkFile(fp, filename))
		return NULL;
	
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

boot2 *readboot2(const char *filename){
	FILE *fp = fopen(filename, "rb");
	if(!checkFile(fp, filename))
		return NULL;
	u32 filelen = filesize(fp);
	if(filelen != RAWBOOT2SIZE && filelen != 2*RAWBOOT2SIZE){
		printf("Error: invalid file size\n");
		return NULL;
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
	
	FILE *out = fopen("noecc.bin", "wb"); // Write output file without ECC
	fwrite(pages, 1, PAGESIZE * pageCount, out);
	fclose(out);
	
	fp = fopen("noecc.bin", "rb");
	if(!checkFile(fp, "noecc.bin")){ // Read file without ECC
		printf("Something went wrong... Aborting\n");
		terminate();
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

s32 installRAWboot2(){
	boot2 *b2 = readboot2(RAWBOOT2FILENAME);
	
	if(b2 == NULL)
		terminate();
	
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

s32 installWADboot2(){
	WAD *wad = readWAD(WADBOOT2FILENAME);
	
	if(wad == NULL)
		terminate();
	
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