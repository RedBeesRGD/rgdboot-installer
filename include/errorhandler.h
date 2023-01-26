/* RGD SDBoot Installer */

#ifndef ERRORHANDLER_H_
#define ERRORHANDLER_H_

extern void ThrowError(char* errorString);
extern void ThrowErrorEx(char* errorString, s32 errorCode);

#endif