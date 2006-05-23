/*
 *  TVIA CyberPro 5202 overlay include file
 *
 */

#ifndef _TVIA5202_OVERLAY_H_
#define _TVIA5202_OVERLAY_H_

#define MAX_OVL_ENG_NUM 2

extern void Tvia_SelectOverlayIndex(u16 wIndex);
extern u16 Tvia_GetOverlayIndex(void);
extern void Tvia_InitOverlay(void);
extern void Tvia_SetOverlayFormat(u8 bFormat);
extern void Tvia_SetOverlayMode(u8 bMode);
extern void Tvia_SetRGBKeyColor(u32 cref);
extern void Tvia_SetOverlaySrcAddr(u32 dwMemAddr, u16 wX, u16 wY, u16 wFetchWidth, u16 wPitchWidth);
extern void Tvia_SetOverlayWindow(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom);
extern void Tvia_SetOverlayScale(u8 bEnableBob, u16 wSrcXExt, u16 wDstXExt, u16 wSrcYExt, u16 wDstYExt);
extern void Tvia_ChangeOverlayFIFO(void);
extern void Tvia_WhichOverlayOnTop(u8 bWhich);
extern void Tvia_OverlayOn(void);
extern void Tvia_OverlayOff(void);    
extern void Tvia_OverlayCleanUp(void);
extern void Tvia_EnableChromaKey(u8 bOnOff);
extern void Tvia_SetChromaKey(u32 creflow, u32 crefhigh);
#if 0
extern void Tvia_EnableOverlayDoubleBuffer(u16 wWhichOverlay, u8 bEnable, u8 bInterleaved);
extern void Tvia_SyncOverlayDoubleBuffer(u16 wWhichOverlay, u8 bEnable);
extern void Tvia_SetOverlayBackBufferAddr(u16 wWhichOverlay, u32 dwOffAddr, u16 wX, u16 wY, u16 wPitchWidth);
#endif
#endif
