/*
 *  TVIA CyberPro 5202 IOCTL functions include file
 *
 */

#ifndef _TVIA5202_IOCTL_H_
#define _TVIA5202_IOCTL_H_

#include "tvia5202-def.h"
#include "tvia5202-misc.h"
#include "saa7114.h"
#include "tvia5202-alpha.h"
#include "tvia5202-capture.h"
#include "tvia5202-dvideo.h"
#include "tvia5202-overlay.h"
#include "tvia5202-synclock.h"
#include "tvia5202-flicker.h"


/*
 * ARub: set one I2C register, simply by passing it as argument
 * arg is  (reg << 8) | val
 */
#define FBIO_TVIA5202_I2C  _IO('t', 0)


// sa7114.c, sa7114.h
typedef struct _decoder_info {
	int nWhichDecoder;
	int nVideoSys;
	int nTuner;
	int nVBI;
} TVIA5202_DECODER_INFO;

#define FBIO_TVIA5202_InitDecoder		_IOR('t', 0x01, TVIA5202_DECODER_INFO)
#define FBIO_TVIA5202_ReleaseDecoder	_IOR('t', 0x02, unsigned long) //int nWhichDecoder

// tvia5202-capture.c, tvia5202-capture.h
typedef struct _rect {
	u16 wLeft;
	u16 wRight;
	u16 wTop;
	u16 wBottom;
} TVIA5202_RECT;

typedef struct _capdstaddr {
	u32 dwMemAddr;
	u16 wX;
	u16 wY;
	u16 wPitchWidth;
} TVIA5202_CAPDSTADDR;

typedef struct _capturescale {
	u16 wSrcXExt;
	u16 wDstXExt;
	u16 wSrcYExt;
	u16 wDstYExt;
	u8 bInterlaced;
} TVIA5202_CAPTURESCALE;
#if 1
typedef struct _backbufferaddr {
	u32 dwOffAddr;
	u16 wX;
	u16 wY;
	u16 wPitchWidth;
} TVIA5202_BACKBUFFERADDR;
#endif
typedef struct _capturepath {
	u8 bWhichPort;
	u8 bWhichCapEngine;
} TVIA5202_CAPTUREPATH;
#if 1
typedef struct _doublebufferselect {
	u16 wWhichCaptureEngine;
	u16 wWhichOverlay;
} TVIA5202_DOUBLEBUFFERSELECT;

typedef struct _capdoublebuffer {
	u16 wWhichCaptureEngine;
	u8 bEnable;
	u8 bInterleaved;
} TVIA5202_CAPDOUBLEBUFFER;

typedef struct _capdoublebuffer2 {
	u16 wWhichCaptureEngine;
	u8 bEnable;
} TVIA5202_CAPDOUBLEBUFFER2;

typedef struct _capbackbufferaddr {
	u16 wWhichCaptureEngine;
	u32 dwOffAddr;
	u16 wX;
	u16 wY;
	u16 wPitchWidth;
} TVIA5202_CAPBACKBUFFERADDR;
#endif
#define FBIO_TVIA5202_SelectCaptureIndex	_IOR('t', 0x03, unsigned long) //u16 wIndex
#define FBIO_TVIA5202_GetCaptureIndex		_IOW('t', 0x04, unsigned long) //u16 wIndex
#define FBIO_TVIA5202_InitCapture			_IOR('t', 0x05, unsigned long) //u16 wCCIR656
#define FBIO_TVIA5202_SetCapSrcWindow		_IOR('t', 0x06, TVIA5202_RECT)
#define FBIO_TVIA5202_SetCapDstAddr			_IOR('t', 0x07, TVIA5202_CAPDSTADDR)
#define FBIO_TVIA5202_SetCaptureScale		_IOR('t', 0x08, TVIA5202_CAPTURESCALE)
#define FBIO_TVIA5202_CaptureOn				_IO('t', 0x09)
#define FBIO_TVIA5202_CaptureOff			_IO('t', 0x0A)
#define FBIO_TVIA5202_SetCapSafeGuardAddr	_IOR('t', 0x0B, unsigned long) //u32 dwOffAddr
#define FBIO_TVIA5202_CaptureCleanUp		_IO('t', 0x0C)
#if 1
#define FBIO_TVIA5202_EnableDoubleBuffer	_IOR('t', 0x0D, unsigned long) //u8 bEnable
#define FBIO_TVIA5202_SetBackBufferAddr		_IOR('t', 0x0E, TVIA5202_BACKBUFFERADDR)
#endif
#define FBIO_TVIA5202_InvertFieldPolarity	_IOR('t', 0x0F, unsigned long) //u8 bInvert

/*The following are new functions for Tvia CyberPro53xx/52xx*/
#define FBIO_TVIA5202_SelectCaptureEngineIndex	_IOR('t', 0x10, unsigned long) //u16 wIndex
#define FBIO_TVIA5202_SetCapturePath		_IOR('t', 0x11, TVIA5202_CAPTUREPATH)
#if 1
#define FBIO_TVIA5202_EnableGeneralDoubleBuffer	_IOR('t', 0x12, unsigned long) //u8 bEnable
#define FBIO_TVIA5202_DoubleBufferSelectRivals	_IOR('t', 0x13, TVIA5202_DOUBLEBUFFERSELECT)
#define FBIO_TVIA5202_EnableCapDoubleBuffer	_IOR('t', 0x14, TVIA5202_CAPDOUBLEBUFFER)
#define FBIO_TVIA5202_SyncCapDoubleBuffer	_IOR('t', 0x15, TVIA5202_CAPDOUBLEBUFFER2)
#define FBIO_TVIA5202_SetCapBackBufferAddr	_IOR('t', 0x16, TVIA5202_CAPBACKBUFFERADDR)
#endif

typedef struct structtestdecoder {
  u8 type;
  u8 index;
  u8 val;
} TESTDECODER;

typedef struct structtesttvia {
  u8 type;
  u8 index;
  u8 val;
  u16 indexword;
  u16 valword;
} TESTTVIA;


// !!!raf
#define FBIO_TVIA5202_TestVideoMemoryRed	_IOR('t', 0xf7, unsigned long) 
#define	FBIO_TVIA5202_TestByteDecoder           _IOR('t', 0xf8, TESTDECODER) 
#define	FBIO_TVIA5202_DumpTotalTest             _IOR('t', 0xf9, unsigned long) 
#define FBIO_TVIA5202_3cf_ba_7_down             _IOR('t', 0xfa, unsigned long) 
#define FBIO_TVIA5202_testvmem                  _IOR('t', 0xfb, unsigned long)

// tvia5202-alpha.c, tvia5202-alpha.h
typedef struct _whichalphablend {
	u8 OnOff;
	u8 bWhichWindow;
} TVIA5202_WHICHALPHABLEND;

typedef struct _blendwindow {
	u16 wLeft;
	u16 wTop;
	u16 wRight;
	u16 wBottom;
	u8 bWhichWindow;
} TVIA5202_BLENDWINDOW;

typedef struct _invertfunction {
	u8 bInvtSrcA;
	u8 bInvtSrcB;
	u8 bInvtAlpha;
	u8 bInvtResult;
} TVIA5202_INVERTFUNCTION;

typedef struct _v2alphamap {
	u8 OnOff;
	u8 bAlphaBpp;
} TVIA5202_V2ALPHAMAP;

typedef struct _alpharam {
	u8 bRAMType;
	u8 bR;
	u8 bG;
	u8 bB;
} TVIA5202_ALPHARAM;

typedef struct _alpharamreg {
	u8 bIndex;
	u8 bR;
	u8 bG;
	u8 bB;
} TVIA5202_ALPHARAMREG;

typedef struct _alpharamreg2 {
	u8 bIndex;
	u32 dwReg; // output
} TVIA5202_ALPHARAMREG2;

typedef struct _alphareg {
	u8 bR;
	u8 bG;
	u8 bB;
	u8 bWhichWindow;
} TVIA5202_ALPHAREG;

typedef struct _magicalphablend {
	u8 OnOff;
	u8 bGeneral;
} TVIA5202_MAGICALPHABLEND;

typedef struct _magicmatchreg {
	u8 bR;
	u8 bG;
	u8 bB;
} TVIA5202_MAGICMATCHREG;

typedef struct _magicmatchregrange {
	u8 bRLow;
	u8 bGLow;
	u8 bBLow;
	u8 bRHigh;
	u8 bGHigh;
	u8 bBHigh;
} TVIA5202_MAGICMATCHREGRANGE;

typedef struct _extendedmagicalpha {
	u8 OnOff;
	u8 b1On1;
	u8 bByColor;
} TVIA5202_EXTENDEDMAGICALPHA;

typedef struct _whichmagicnumber {
	u8 OnOff;
	u8 bWhich;
} TVIA5202_WHICHMAGICNUMBER;

typedef struct _magicnumberwindow {
	u8 OnOff;
	u8 bWhichWindow;
} TVIA5202_MAGICNUMBERWINDOW;

typedef struct _magicmatchregext {
	u8 bR;
	u8 bG;
	u8 bB;
	u8 bWhichWindow;
} TVIA5202_MAGICMATCHREGEXT;

typedef struct _magicmatchregrangeext {
	u8 bRLow;
	u8 bGLow;
	u8 bBLow;
	u8 bRHigh;
	u8 bGHigh;
	u8 bBHigh;
	u8 bWhichWindow;
} TVIA5202_MAGICMATCHREGRANGEEXT;

typedef struct _magicmatchalphareg {
	u8 bR;
	u8 bG;
	u8 bB;
	u8 bWhichWindow;
} TVIA5202_MAGICMATCHALPHAREG;

typedef struct _magicpolicyext {
	u16 wMatch;
	u8 bWhichWindow;
} TVIA5202_MAGICPOLICYEXT;

typedef struct _slockalphablend {
	u8 bEnable;
	u16 wTVMode;
	u16 wModenum;
} TVIA5202_SLOCKALPHABLEND;

typedef struct _alphabase {
	u8 TVmode;
	u8 Modenum;
} TVIA5202_ALPHABASE;

typedef struct _slockoverlaywindow {
	u16 wLeft;
	u16 wTop;
	u16 wRight;
	u16 wBottom;
	u8 bWhichWindow;
} TVIA5202_SLOCKOVERLAYWINDOW;

typedef struct _whichslockoverlay {
	u8 OnOff;
	u8 bWhichWindow;
} TVIA5202_WHICHSLOCKOVERLAY;

typedef struct _windowblendsrca {
	u8 bSrcA;
	u8 bWin;
} TVIA5202_WINDOWBLENDSRCA;

typedef struct _windowblendsrcb {
	u8 bSrcB;
	u8 bWin;
} TVIA5202_WINDOWBLENDSRCB;

typedef struct _windowalphasrc {
	u8 bAlphaSrc;
	u8 bWin;
} TVIA5202_WINDOWALPHASRC;

#define FBIO_TVIA5202_EnableAlphaBlend		_IOR('t', 0x17, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_EnableFullScreenAlphaBlend	_IOR('t', 0x18, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_EnableWhichAlphaBlend	_IOR('t', 0x19, TVIA5202_WHICHALPHABLEND)
#define FBIO_TVIA5202_CleanUpAlphaBlend		_IO('t', 0x1A)
#define FBIO_TVIA5202_SetBlendWindow		_IOR('t', 0x1B, TVIA5202_BLENDWINDOW)
#define FBIO_TVIA5202_InvertFunction		_IOR('t', 0x1C, TVIA5202_INVERTFUNCTION)
#define FBIO_TVIA5202_InvertVideo			_IOR('t', 0x1D, unsigned long) //u8 bInvtVideo
#define FBIO_TVIA5202_EnableV2AlphaMap		_IOR('t', 0x1E, TVIA5202_V2ALPHAMAP)
#define FBIO_TVIA5202_DisableV2DDA			_IO('t', 0x1F)

#define FBIO_TVIA5202_SelectRAMAddr			_IOR('t', 0x20, unsigned long) //u8 bRAMAddrSel
#define FBIO_TVIA5202_SetAlphaRAM			_IOR('t', 0x21, TVIA5202_ALPHARAM)
#define FBIO_TVIA5202_SetAlphaRAMReg		_IOR('t', 0x22, TVIA5202_ALPHARAMREG)
#define FBIO_TVIA5202_ReadAlphaRAMReg		_IOW('t', 0x23, TVIA5202_ALPHARAMREG2)
#define FBIO_TVIA5202_EnableSWIndexToRAM	_IOR('t', 0x24, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_RAMLoop				_IOR('t', 0x25, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_SelectWaitCount		_IOR('t', 0x26, unsigned long) //u8 bWaitCount
#define FBIO_TVIA5202_RotateAlphaRAMReg		_IO('t', 0x27)

#define FBIO_TVIA5202_SelectAlphaSrc		_IOR('t', 0x28, unsigned long) //u8 bAlphaSrc
#define FBIO_TVIA5202_SetAlphaReg			_IOR('t', 0x29, TVIA5202_ALPHAREG)

#define FBIO_TVIA5202_SelectBlendSrcA		_IOR('t', 0x2A, unsigned long) //u8 bSrcA
#define FBIO_TVIA5202_SelectBlendSrcB		_IOR('t', 0x2B, unsigned long) //u8 bSrcB

#define FBIO_TVIA5202_EnableMagicAlphaBlend	_IOR('t', 0x2C, TVIA5202_MAGICALPHABLEND)
#define FBIO_TVIA5202_EnableMagicReplace	_IOR('t', 0x2D, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_SelectMagicAlphaSrc	_IOR('t', 0x2E, unsigned long) //u8 bAlphaSrc
#define FBIO_TVIA5202_SetMagicMatchReg		_IOR('t', 0x2F, TVIA5202_MAGICMATCHREG)
#define FBIO_TVIA5202_SetMagicMatchRegRange	_IOR('t', 0x30, TVIA5202_MAGICMATCHREGRANGE)
#define FBIO_TVIA5202_SetMagicPolicy		_IOR('t', 0x31, unsigned long) //u16 wMatch
#define FBIO_TVIA5202_SetMagicReplaceReg	_IOR('t', 0x32, TVIA5202_MAGICMATCHREG)

#define FBIO_TVIA5202_EnableExtendedMagicAlpha	_IOR('t', 0x33, TVIA5202_EXTENDEDMAGICALPHA)
#define FBIO_TVIA5202_EnableWhichMagicNumber	_IOR('t', 0x34, TVIA5202_WHICHMAGICNUMBER)
#define FBIO_TVIA5202_EnableMagicNumberInWhichWindow	_IOR('t', 0x35, TVIA5202_MAGICNUMBERWINDOW)
#define FBIO_TVIA5202_SetMagicMatchRegExt	_IOR('t', 0x36, TVIA5202_MAGICMATCHREGEXT)
#define FBIO_TVIA5202_SetMagicMatchRegRangeExt	_IOR('t', 0x37, TVIA5202_MAGICMATCHREGRANGEEXT)
#define FBIO_TVIA5202_SetMagicMatchAlphaReg		_IOR('t', 0x38, TVIA5202_MAGICMATCHALPHAREG)
#define FBIO_TVIA5202_SetMagicPolicyExt		_IOR('t', 0x39, TVIA5202_MAGICPOLICYEXT)

#define FBIO_TVIA5202_SelectAlphaSrc2FromRam	_IOR('t', 0x3A, unsigned long) //u8 bWhichWindow
#define FBIO_TVIA5202_EnablePaletteMode		_IOR('t', 0x3B, unsigned long) //u8 bEnable
#define FBIO_TVIA5202_EnableSLockAlphaBlend	_IOR('t', 0x3C, TVIA5202_SLOCKALPHABLEND)
#define FBIO_TVIA5202_EnableDvideoAlphaBlend	_IOR('t', 0x3D, unsigned long) //u8 bEnable

#define FBIO_TVIA5202_SetAlphaMapPtr	_IOR('t', 0x3E, unsigned long) //u32 dwAlphamapPtr
#define FBIO_TVIA5202_GetAlphaMapPtr	_IOW('t', 0x3F, unsigned long) //u32 output
#define FBIO_TVIA5202_SetAlphaBase		_IOR('t', 0x40, TVIA5202_ALPHABASE)

#define FBIO_TVIA5202_InitSLockOverlay	_IO('t', 0x41)
#define FBIO_TVIA5202_CleanupSLockOverlay	_IO('t', 0x42)
#define FBIO_TVIA5202_SetSLockOverlayWindow	_IOR('t', 0x43, TVIA5202_SLOCKOVERLAYWINDOW)
#define FBIO_TVIA5202_EnableSLockOverlay	_IOR('t', 0x44, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_EnableWhichSLockOverlay	_IOR('t', 0x45, TVIA5202_WHICHSLOCKOVERLAY)
#define FBIO_TVIA5202_EnableInterlaceAlpha	_IOR('t', 0x46, unsigned long) //u8 bEnable
#define FBIO_TVIA5202_EnableInterlaceRGBMagicNumber	_IOR('t', 0x47, unsigned long) //u8 bEnable

#define FBIO_TVIA5202_SelectWindowBlendSrcA	_IOR('t', 0x48, TVIA5202_WINDOWBLENDSRCA)
#define FBIO_TVIA5202_SelectWindowBlendSrcB	_IOR('t', 0x49, TVIA5202_WINDOWBLENDSRCB)
#define FBIO_TVIA5202_SelectWindowAlphaSrc	_IOR('t', 0x4A, TVIA5202_WINDOWALPHASRC)
#define FBIO_TVIA5202_EnableWindowAlphaBlendSrcA	_IOR('t', 0x4B, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_EnableWindowAlphaBlendSrcB	_IOR('t', 0x4C, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_EnableWindowAlphaBlendAlpha	_IOR('t', 0x4D, unsigned long) //u8 OnOff
#define FBIO_TVIA5202_EnableWindowAlphaBlend		_IOR('t', 0x4E, unsigned long) //u8 OnOff

// tvia5202-dvideo.c, tvia5202-dvideo.h
typedef struct _directvideo {
	u8 bWhichOverlay;
	u16 wTVMode;
	u16 wModenum;
	u8 bOnOff;
} TVIA5202_DIRECTVIDEO;

typedef struct _genericdirectvideo {
	u16 wTVMode;
	u16 wModenum;
	u8 bOnOff;
} TVIA5202_GENERICDIRECTVIDEO;

typedef struct _whichdirectvideo {
	u8 bWhichOverlay;
	u8 bOnOff;
} TVIA5202_WHICHDIRECTVIDEO;

#define FBIO_TVIA5202_EnableDirectVideo		_IOR('t', 0x4F, TVIA5202_DIRECTVIDEO)
#define FBIO_TVIA5202_EnableGenericDirectVideo	_IOR('t', 0x50, TVIA5202_GENERICDIRECTVIDEO)
#define FBIO_TVIA5202_EnableWhichDirectVideo	_IOR('t', 0x51, TVIA5202_WHICHDIRECTVIDEO)

// tvia5202-overlay.c, tvia5202-overlay.h
typedef struct _overlaysrcaddr {
	u32 dwMemAddr;
	u16 wX;
	u16 wY;
	u16 wFetchWidth;
	u16 wPitchWidth;
} TVIA5202_OVERLAYSRCADDR;

typedef struct _overlaywindow {
	u16 wLeft;
	u16 wTop;
	u16 wRight;
	u16 wBottom;
} TVIA5202_OVERLAYWINDOW;

typedef struct _overlayscale {
	u8 bEnableBob;
	u16 wSrcXExt;
	u16 wDstXExt;
	u16 wSrcYExt;
	u16 wDstYExt;
} TVIA5202_OVERLAYSCALE;

typedef struct _chromakey {
	u32 creflow;
	u32 crefhigh;
} TVIA5202_CHROMAKEY;
#if 1
typedef struct _overlaydoublebuffer {
	u16 wWhichOverlay;
	u8 bEnable;
	u8 bInterleaved;
} TVIA5202_OVERLAYDOUBLEBUFFER;

typedef struct _overlaydoublebuffer2 {
	u16 wWhichOverlay;
	u8 bEnable;
} TVIA5202_OVERLAYDOUBLEBUFFER2;

typedef struct _overlaybackbufferaddr {
	u16 wWhichOverlay;
	u32 dwOffAddr;
	u16 wX;
	u16 wY;
	u16 wPitchWidth;
} TVIA5202_OVERLAYBACKBUFFERADDR;
#endif
#define FBIO_TVIA5202_SelectOverlayIndex	_IOR('t', 0x52, unsigned long) //u16 wIndex
#define FBIO_TVIA5202_GetOverlayIndex	_IOW('t', 0x53, unsigned long) //u16 output
#define FBIO_TVIA5202_InitOverlay		_IO('t', 0x54)
#define FBIO_TVIA5202_SetOverlayFormat	_IOR('t', 0x55, unsigned long) //u8 bFormat
#define FBIO_TVIA5202_SetOverlayMode	_IOR('t', 0x56, unsigned long) //u8 bMode
#define FBIO_TVIA5202_SetRGBKeyColor	_IOR('t', 0x57, unsigned long) //u32 cref
#define FBIO_TVIA5202_SetOverlaySrcAddr	_IOR('t', 0x58, TVIA5202_OVERLAYSRCADDR)
#define FBIO_TVIA5202_SetOverlayWindow	_IOR('t', 0x59, TVIA5202_OVERLAYWINDOW)
#define FBIO_TVIA5202_SetOverlayScale	_IOR('t', 0x5A, TVIA5202_OVERLAYSCALE)
#define FBIO_TVIA5202_ChangeOverlayFIFO	_IO('t', 0x5B)
#define FBIO_TVIA5202_WhichOverlayOnTop	_IOR('t', 0x5C, unsigned long) //u8 bWhich
#define FBIO_TVIA5202_OverlayOn			_IO('t', 0x5D)
#define FBIO_TVIA5202_OverlayOff		_IO('t', 0x5E)
#define FBIO_TVIA5202_OverlayCleanUp	_IO('t', 0x5F)
#define FBIO_TVIA5202_EnableChromaKey	_IOR('t', 0x60, unsigned long) //u8 bOnOff
#define FBIO_TVIA5202_SetChromaKey		_IOR('t', 0x61, TVIA5202_CHROMAKEY)
#if 1
#define FBIO_TVIA5202_EnableOverlayDoubleBuffer	_IOR('t', 0x62, TVIA5202_OVERLAYDOUBLEBUFFER)
#define FBIO_TVIA5202_SyncOverlayDoubleBuffer	_IOR('t', 0x63, TVIA5202_OVERLAYDOUBLEBUFFER2)
#define FBIO_TVIA5202_SetOverlayBackBufferAddr	_IOR('t', 0x64, TVIA5202_OVERLAYBACKBUFFERADDR)
#endif
// tvia5202-synclock.c, tvia5202-synclock.h
typedef struct _synclock {
	u16 wCaptureIndex;
	u16 wTVStandard;
	u16 wResolution;
	u8 bVideoFormat;
} TVIA5202_SYNCLOCK;

typedef struct _videoclk {
	u16 wCaptureIndex;
	u16 wTVStandard;
	u8 bVideoFormat;
	u8 bSet;
	u16 wResolution;
} TVIA5202_VIDEOCLK;

typedef struct _synclockgm {
	u16 wCaptureIndex;
	u16 wTVStandard;
	u16 wResolution;
	u8 bVideoFormat;
} TVIA5202_SYNCLOCKGM;

typedef struct _synclockvideo {
	u16 bOutput;
	u16 wTVStandard;
	u16 wResolution;
	u8 bVideoFormat;
} TVIA5202_SYNCLOCKVIDEO;

typedef struct _synclockvideosignal {
	u8 bVideoFormat;
	int Ok;
} TVIA5202_SYNCLOCKVIDEOSIGNAL;

#define FBIO_TVIA5202_DoSyncLock	_IOR('t', 0x65, TVIA5202_SYNCLOCK)
#define FBIO_TVIA5202_CleanupSyncLock	_IO('t', 0x66)
#define FBIO_TVIA5202_SetTVColor	_IO('t', 0x67)
#define FBIO_TVIA5202_ChangeToVideoClk	_IOR('t', 0x68, TVIA5202_VIDEOCLK)
#define FBIO_TVIA5202_GetTVMode		_IOW('t', 0x69, unsigned long) //u16 output
#define FBIO_TVIA5202_SetVCRMode	_IOR('t', 0x6A, unsigned long) //u8 bVCRMode
#define FBIO_TVIA5202_SetSyncLockGM	_IOR('t', 0x6B, TVIA5202_SYNCLOCKGM)
#define FBIO_TVIA5202_CleanupSyncLockGM	_IO('t', 0x6C)
#define FBIO_TVIA5202_OutputSyncLockVideo	_IOR('t', 0x6D, TVIA5202_SYNCLOCKVIDEO)
#define FBIO_TVIA5202_SetSynclockPath	_IOR('t', 0x6E, unsigned long) //u8 bWhichPort
#define FBIO_TVIA5202_DetectSynclockVideoSignal	_IOW('t', 0x6F, TVIA5202_SYNCLOCKVIDEOSIGNAL)

//CARLOS DMA
typedef struct _dmareadconf {
	u8 *dsadr;
	u8 *dtadr;
	u32 width;
	u32 height;
	u32 pitch;
	int pid;
} TVIA5202_DMACONF;

#define FBIO_TVIA5202_DMAReadRequest _IO('t', 0x70)
#define FBIO_TVIA5202_DMAWriteRequest _IO('t', 0x71)
#define FBIO_TVIA5202_DMAReadFree _IO('t', 0x72)
#define FBIO_TVIA5202_DMAWriteFree _IO('t', 0x73)
#define FBIO_TVIA5202_DMAReadFrame _IOW('t', 0x74, TVIA5202_DMACONF)
#define FBIO_TVIA5202_DMAWriteFrame _IOR('t', 0x75, TVIA5202_DMACONF)
#define FBIO_TVIA5202_DMAReadPolling _IO('t', 0x76)
//END CARLOS DMA


//CARLOS FLICKER FILTER
/* Initialization Flicker coefficient:
   Central line = alpha * L0 + beta * L1 + gama * L2 (Hardware operation)
   Recommand to obey the regular below to get the better video image quality.
          255 >= alpha + beta + gama
*/
typedef struct _FlickerWin {
	u16 wLeft;
	u16 wTop;
	u16 wRight;
	u16 wBottom;
} TVIA5202_FLICKERWIN;

typedef struct _FlickerCtrlCoeffs {
	u8 alpha;
	u8 beta;
	u8 gama;
} TVIA5202_FLICKERCOEFFS;

#define FBIO_TVIA5202_FLICKERFree _IOR('t', 0x77, u16)
#define FBIO_TVIA5202_FLICKERWinInside _IOR('t', 0x78, u16)
#define FBIO_TVIA5202_FLICKERWin _IOR('t', 0x79, TVIA5202_FLICKERWIN)
#define FBIO_TVIA5202_FLICKERCtrlCoeffs _IOR('t', 0x80, TVIA5202_FLICKERCOEFFS)
//END CARLOS FLICKER FILTER

#endif
