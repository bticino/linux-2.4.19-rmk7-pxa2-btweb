/*
 *  TVIA CyberPro 5202 direct video include file
 *
 */

#ifndef _TVIA5202_DVIDEO_H_
#define _TVIA5202_DVIDEO_H_

extern void Tvia_EnableDirectVideo(u8 bWhichOverlay, u16 wTVMode, u16 wModenum, u8 bOnOff);
extern void Tvia_EnableGenericDirectVideo(u16 wTVMode, u16 wModenum, u8 bOnOff);
extern void Tvia_EnableWhichDirectVideo(u8 bWhichOverlay, u8 bOnOff);

#endif
