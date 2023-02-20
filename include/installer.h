/* RGD SDBoot Installer */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <wiiuse/wpad.h>

#ifndef INSTALLER_H_
#define INSTALLER_H_

void SDBootInstaller( void );
void NANDBootInstaller( void );
void Boot2WADInstaller( void );

#endif
