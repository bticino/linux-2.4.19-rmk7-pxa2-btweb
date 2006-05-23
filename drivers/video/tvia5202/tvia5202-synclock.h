/*
 *  TVIA CyberPro 5202 synclock include file
 *
 */

#ifndef _TVIA5202_SYNCLOCK_H_
#define _TVIA5202_SYNCLOCK_H_

extern void Tvia_DoSyncLock(u16 wCaptureIndex, u16 wTVStandard, u16 wResolution, u8 bVideoFormat);
extern void Tvia_CleanupSyncLock(void);
extern void Tvia_SetTVColor(void);
extern void Tvia_ChangeToVideoClk(u16 wCaptureIndex, u16 wTVStandard, u8 bVideoFormat, u8 bSet, u16 wResolution);
extern u16 Tvia_GetTVMode(void);
extern void Tvia_SetVCRMode(u8 bVCRMode);
extern void Tvia_SetSyncLockGM(u16 wCaptureIndex, u16 wTVStandard, u16 wResolution, u8 bVideoFormat);
extern void Tvia_CleanupSyncLockGM(void);
extern void Tvia_OutputSyncLockVideo(u16 bOutput, u16 wTVStandard, u16 wResolution, u8 bVideoFormat);
extern void Tvia_SetSynclockPath(u8 bWhichPort);
extern int Tvia_DetectSynclockVideoSignal(u8 bVideoFormat);
#endif

