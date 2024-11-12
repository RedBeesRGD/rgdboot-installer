/* RGD SDBoot Installer */

#ifndef __INSTALLER_H__
#define __INSTALLER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <wiiuse/wpad.h>

/* [nitr8]: Added */
#ifdef __cplusplus
extern "C" {
#endif

void SDBootInstaller(void);
void NANDBootInstaller(void);
void Boot2WADInstaller(void);
void Boot2BackupInstaller(void);
void Boot2BackupMake(void);

/* [nitr8]: get rid of warning for implicit declaration of function */
void HBCInstaller(void);

/* [nitr8]: get rid of warning for implicit declaration of function */
void BootSysCheck(void);

/* [nitr8]: get rid of warning for implicit declaration of function */
void RestoreNAND(void);

/* [nitr8]: get rid of warning for implicit declaration of function */
void EraseNANDFS(void);

/* [nitr8]: Added */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INSTALLER_H__ */

