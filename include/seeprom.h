/* RGD SDBoot Installer */

#ifndef __SEEPROM_H__
#define __SEEPROM_H__

/* [nitr8]: Added */
#ifdef __cplusplus
extern "C" {
#endif

/* [nitr8]: Added */
int SEEPROMClearStep(void);

/* [nitr8]: Added */
int SEEPROMBackupVersion(void);

/* [nitr8]: Added */
int SEEPROMRestoreVersion(void);

/* [nitr8]: Added */
int SEEPROMCompareVersion(int state);

/* [nitr8]: Added */
int SEEPROMClearVersion(void);

/* [root1024]: Added */
void SEEPROMDisplayInfo(void);

/* [nitr8]: Added */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SEEPROM_H__ */

