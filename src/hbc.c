/* RGD SDBoot Installer */

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <gctypes.h>

#include "errorhandler.h"
#include "errorcodes.h"
#include "seeprom.h"
#include "boot2.h"
#include "tools.h"
#include "prodinfo.h"
#include "runtimeiospatch.h"
#include "menu.h"
#include "flash.h"
#include "version.h"
#include "haxx.h"
#include "hbc.h"

#include "hbc/hbc_content0.h"
#include "hbc/hbc_content1.h"
#include "hbc/hbc_certs.h"
#include "hbc/hbc_tik.h"
#include "hbc/hbc_tmd.h"

s32 InstallHBC( void ) {
	s32 BLOCK_SIZE = 1024, offset = 0, size;
	u8 *contentBuffer = (u8 *)memalign(32, BLOCK_SIZE);
	s32 ret, cfd;
	
	
	// Add Ticket
	ret = ES_AddTicket((signed_blob *)hbc_tik, hbc_tik_size, (signed_blob *)hbc_certs, hbc_certs_size, NULL, 0);
	printf("ES_AddTicket returned: %d\n", ret);
	
	// Start adding Title
	ret = ES_AddTitleStart((signed_blob *)hbc_tmd, hbc_tmd_size, (signed_blob *)hbc_certs, hbc_certs_size, NULL, 0);
	printf("ES_AddTitleStart returned: %d\n", ret);
	
	
	// Start adding content 0
	cfd = ES_AddContentStart(0x000100014F484243, 0);
	printf("ES_AddContentStart(0) returned: %d\n", ret);
	
	// Add content 0, with 1024-byte blocks
	while(offset < hbc_content0_size){
		size = (offset + BLOCK_SIZE <= hbc_content0_size) ? BLOCK_SIZE : hbc_content0_size - offset;
		
		memcpy(contentBuffer, (u8 *)(hbc_content0+offset), size);
		
		ret = ES_AddContentData(cfd, contentBuffer, size);
		
		offset += BLOCK_SIZE;
	}
	
	printf("ES_AddContentData(0) returned: %d\n", ret);
	
	// Finish adding content 0
	ret = ES_AddContentFinish(cfd);
	printf("ES_AddContentFinish(0) returned: %d\n", ret);
	
	
	
	/* ------------------------------------------ */
	
	
	
	cfd = ES_AddContentStart(0x000100014F484243, 1);
	printf("ES_AddContentStart(1) returned: %d\n", ret);
	
	offset = 0;
	
	// Add content 1, with 1024-byte blocks
	while(offset < hbc_content1_size){
		size = (offset + BLOCK_SIZE <= hbc_content1_size) ? BLOCK_SIZE : hbc_content1_size - offset;
		
		memcpy(contentBuffer, (u8 *)(hbc_content1+offset), size);
		
		ret = ES_AddContentData(cfd, contentBuffer, size);
		
		offset += BLOCK_SIZE;
	}
	
	printf("ES_AddContentData(1) returned: %d\n", ret);
	
	ret = ES_AddContentFinish(cfd);
	printf("ES_AddContentFinish(1) returned: %d\n", ret);
	
	ret = ES_AddTitleFinish();
	printf("ES_AddTitleFinish returned: %d\n", ret);

	if(contentBuffer != NULL)
		free(contentBuffer);
	
	WaitForPad();
	return ret;
}
