/* RGD SDBoot Installer */

#ifndef __ERRORHANDLER_H__
#define __ERRORHANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void ThrowError(const char* errorString);
extern void ThrowErrorEx(const char* errorString, s32 errorCode);

/* [nitr8]: Added */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ERRORHANDLER_H__ */

