/* RGD SDBoot Installer */

#ifndef __BOOT2_H__
#define __BOOT2_H__

#include "tools.h"

/* [nitr8]: Moved to "errorcodes.h" */
/* #define MISSING_FILE     -1
#define BOOT2_DOLPHIN    -4
#define HASH_MISMATCH    -1022
#define CANNOT_DOWNGRADE -1031 */

#define RAWBOOT2SIZE     0x21000

/* [nitr8]: Added WAD header size */
#define WAD_HEADER_SIZE	0x40

typedef struct {
	signed_blob *ca_cert;	/* offset 0x20 (memset OK) */
	signed_blob *cp_cert;	/* offset 0x420 (memset OK) */
	signed_blob *xs_cert;	/* offset 0x720 (memset OK) */
	
//	u8 padding[0x40];	/* offset 0x14 */

	signed_blob *tik_cert;	/* offset 0xA20 (memset OK) */
	signed_blob *TMD_cert	/* (memset OK) */;
	
} certificates;

typedef struct {
	u32 headerLen;		/* offset 0x0 */
	u32 dataOffset;		/* offset 0x4 */
	u32 certsLen;		/* offset 0x8 */
	u32 tikLen;		/* offset 0xC */
	u32 TMDLen;		/* offset 0x10 */

	/* [nitr8]: Was this forgotten?
		    Let's just assume we're using this struct to assign
		    it to the contents of a WAD file with no ECC data:
		    You would point the "certs" struct to offset 0x14
		    in the WAD which is simply NOT where the certificates
		    start. Instead, they start at offset 0x20 which is
		    offset 0x14 + 0xC (12) bytes. Now an "integer" makes
		    use of 4 bytes, so 12 / 4 = 3. Let's put some padding
		    here... */
//	u8 padding_1[3];	/* offset 0x14 */
	
	certificates *certs;	/* offset 0x20 */
	signed_blob *tik;	/* offset 0x0 */
	signed_blob *TMD;

//	u8 padding_2[0x34];	/* offset 0x14 */

	u8 *content;
	u32 contentSize;
} boot2;

typedef struct {
	u32 headerLen;
	u16 wadType;
	u16 wadVersion;
	u32 certsLen;
	u8  reserved[4];
	u32 tikLen;
	u32 TMDLen;
	u32 contentSize;
	u32 footerSize;
	u8  padding[0x20];
	
	certificates *certs;
	signed_blob *tik;
	signed_blob *TMD;
	u8 *content;
} WAD;

#ifdef __cplusplus
extern "C" {
#endif

/* [nitr8]: Make static */
/* WAD *ReadWAD(const char *filename); */

/* [nitr8]: Make static */
/* boot2 *ReadBoot2(const char *filename); */

/* [nitr8]: Make static */
/* s32 InstallRawBoot2(const char* filename); */

s32 InstallWADBoot2(const char* filename);
s32 InstallSDBoot(const char* filename);
s32 InstallNANDBoot(const char* filename, const char* payload);
s32 BackupBoot2Blocks(const char* filename);
s32 RestoreBoot2Blocks(const char* filename);

/* [nitr8]: Add function to read ALL the boot2 blocks in a row - with the ECC data and blockmap data included */
s32 ReadBoot2Blocks(u8 *buffer, int start_block, int end_block);

/* [nitr8]: Added */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOOT2_H__ */

