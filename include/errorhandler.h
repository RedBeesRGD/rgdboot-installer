/* RGD SDBoot Installer */

#ifndef ERRORHANDLER_H_
#define ERRORHANDLER_H_

extern void ThrowError(const char* errorString);
extern void ThrowErrorEx(const char* errorString, s32 errorCode);

#endif
