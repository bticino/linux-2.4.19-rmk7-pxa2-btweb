/*
 *  TVIA CyberPro 5202 capture include file
 *
 */

#ifndef _TVIA5202_CAPTURE_H_
#define _TVIA5202_CAPTURE_H_

extern void Tvia_SelectCaptureIndex(u16 wIndex);
extern u16 Tvia_GetCaptureIndex(void);
extern void Tvia_InitCapture(u16 wCCIR656);
extern void Tvia_SetCapSrcWindow(u16 left, u16 right, u16 top, u16 bottom);
extern void Tvia_SetCapDstAddr(u32 dwMemAddr, u16 wX, u16 wY, u16 wPitchWidth);
extern void Tvia_SetCaptureScale(u16 wSrcXExt, u16 wDstXExt, u16 wSrcYExt, u16 wDstYExt, u8 bInterlaced);
extern void Tvia_CaptureOn(void);
extern void Tvia_CaptureOff(void);
extern void Tvia_SetCapSafeGuardAddr(u32 dwOffAddr);
extern void Tvia_CaptureCleanUp(void);
#if 0
extern void Tvia_EnableDoubleBuffer(u8 bEnable);
extern void Tvia_SetBackBufferAddr(u32 dwOffAddr, u16 wX, u16 wY, u16 wPitchWidth);
#endif
extern void Tvia_InvertFieldPolarity(u8 bInvert);

/*The following are new functions for Tvia CyberPro53xx/52xx*/
extern void Tvia_SelectCaptureEngineIndex(u16 wIndex);
extern void Tvia_SetCapturePath(u8 bWhichPort, u8 bWhichCapEngine);
#if 0
extern void Tvia_EnableGeneralDoubleBuffer(u8 bEnable);
extern void Tvia_DoubleBufferSelectRivals(u16 wWhichCaptureEngine, u16 wWhichOverlay);
extern void Tvia_EnableCapDoubleBuffer(u16 wWhichCaptureEngine, u8 bEnable, u8 bInterleaved);
extern void Tvia_SyncCapDoubleBuffer(u16 wWhichCaptureEngine, u8 bEnable);
extern void Tvia_SetCapBackBufferAddr(u16 wWhichCaptureEngine, u32 dwOffAddr, u16 wX, u16 wY, u16 wPitchWidth);
#endif
/*
extern void Tvia_EnableScalar(u8 onoff);
extern void Tvia_SetCapScalarRatio(u16 wSrcWidth, u16 wDstWidth, u16 wSrcHeight, u16 wDstHeight, u8 bInterlaced);
*/
#endif
