/*
 *  TVIA CyberPro 5202 alpha blend include file
 *
 */

#ifndef _TVIA5202_ALPHA_H_
#define _TVIA5202_ALPHA_H_

#define MAX_BLEND_WIN_NUM       4

/*for SetBlendWindow(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom, 
u8 bWhichWindow);*/
#define Win0 0
#define Win1 1
#define Win2 2
#define Win3 3

/*for SelectBlendSrcA(u8 bSrcA);*/
#define SrcA_Gr  0
#define SrcA_V1  1
#define SrcA_V2  2
#define SrcA_Ram 3

/*for SelectBlendSrcB(u8 bSrcB);*/
#define SrcB_V1  0
#define SrcB_V2  1
#define SrcB_Gr  2
#define SrcB_Ram 3
#define SrcB_V12 4

/*for SelectRAMAddr(u8 bRAMAddrSel);*/
#define Ram_Int   0 
#define Ram_V2    1
#define Ram_VSync 2
#define Ram_CPU   3

/*for SelectAlphaSrc(u8 bAlphaSrc);
  for SelectMagicAlphaSrc(u8 bAlphaSrc);*/
#define Alpha_Int 0
#define Alpha_V2  1
#define Alpha_Ram 2
#define Alpha_Reg 3
#define Alpha_Map 4

/*for EnableV2AlphaMap(u8 OnOff, u8 bAlphaBpp) and 
  Tvia_EnableAlphamap(u8 OnOff, u8 bAlphaBpp) */
#define ALPHABPP1  0
#define ALPHABPP2  1
#define ALPHABPP4  2
#define ALPHABPP8  3
#define ALPHABPP   4
#define ALPHABPP16 5
#define ALPHABPP24 6

/*for special alpha-blending graphics mode alpha value comes with graphics 
  data*/
#define GM_444 0
#define GM_555 1
#define GM_888 2

/*for SetMagicPolicy(u16 wMatch);*/
#define MATCH   0
#define NOMATCH 1

/*four group magic numbers for extended magic number feature*/
#define MAGIC0  0
#define MAGIC1  1
#define MAGIC2  2
#define MAGIC3  3

#define ALPHA_GAMAP      0
#define ALPHA_AMAP       1
#define ALPHA_LOOKUP     2
#define ALPHA_REGISTER   3

/*1) Miscellaneous alpha blending routines*/

extern void EnableAlphaBlend(u8 OnOff);
extern void EnableFullScreenAlphaBlend(u8 OnOff);
extern void EnableWhichAlphaBlend(u8 OnOff, u8 bWhichWindow);
extern void CleanUpAlphaBlend(void);
extern void SetBlendWindow(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom, u8 bWhichWindow);
extern void InvertFunction(u8 bInvtSrcA, u8 bInvtSrcB, u8 bInvtAlpha, u8 bInvtResult);
extern void InvertVideo(u8 bInvtVideo);
extern void EnableV2AlphaMap(u8 OnOff, u8 bAlphaBpp);
extern void DisableV2DDA(void);

/*2) Alpha RAM - Look up table*/

extern void SelectRAMAddr(u8 bRAMAddrSel);
extern void SetAlphaRAM(u8 bRAMType, u8 bR, u8 bG, u8 bB);
extern void SetAlphaRAMReg(u8 bIndex, u8 bR, u8 bG, u8 bB);
extern u32 ReadAlphaRAMReg(u8 bIndex);
extern void EnableSWIndexToRAM(u8 OnOff);
extern void RAMLoop(u8 OnOff);
extern void SelectWaitCount(u8 bWaitCount);
extern void RotateAlphaRAMReg(void);

/*3) Alpha sources*/

extern void SelectAlphaSrc(u8 bAlphaSrc);
extern void SetAlphaReg(u8 bR, u8 bG, u8 bB, u8 bWhichWindow);

/*4) Sources to be Alpha blended*/

extern void SelectBlendSrcA(u8 bSrcA);
extern void SelectBlendSrcB(u8 bSrcB);

/*5) Magic Number Alpha Blending or two alpha sources*/

extern void EnableMagicAlphaBlend(u8 OnOff, u8 bGeneral);
extern void EnableMagicReplace(u8 OnOff);
extern void SelectMagicAlphaSrc(u8 bAlphaSrc);
extern void SetMagicMatchReg(u8 bR, u8 bG, u8 bB);
extern void SetMagicMatchRegRange(u8 bRLow, u8 bGLow, u8 bBLow, u8 bRHigh, u8 bGHigh, u8 bBHigh);
extern void SetMagicPolicy(u16 wMatch);
extern void SetMagicReplaceReg(u8 bR, u8 bG, u8 bB);

extern void EnableExtendedMagicAlpha(u8 OnOff, u8 b1On1, u8 bByColor);
extern void EnableWhichMagicNumber(u8 OnOff, u8 bWhich);
extern void EnableMagicNumberInWhichWindow(u8 OnOff, u8 bWhichWindow);
extern void SetMagicMatchRegExt(u8 bR, u8 bG, u8 bB, u8 bWhichWindow);
extern void SetMagicMatchRegRangeExt(u8 bRLow, u8 bGLow, u8 bBLow, u8 bRHigh, u8 bGHigh, u8 bBHigh, u8 bWhichWindow);
extern void SetMagicMatchAlphaReg(u8 bR, u8 bG, u8 bB, u8 bWhichWindow);
extern void SetMagicPolicyExt(u16 wMatch, u8 bWhichWindow);

/*6) Others*/

extern void SelectAlphaSrc2FromRam(u8 bWhichWindow);
extern void EnablePaletteMode(u8 bEnable);
extern void EnableSLockAlphaBlend(u8 bEnable, u16 wTVMode, u16 wModenum);
extern void EnableDvideoAlphaBlend(u8 bEnable);

extern void SetAlphaMapPtr(u32 dwAlphamapPtr);
extern u32 GetAlphaMapPtr(void);
extern void SetAlphaBase(u8 TVmode, u8 Modenum);

#define INCLUDE_API_2NDALPHA        0
    
#if INCLUDE_API_2NDALPHA

/*Second layer alpha-blending*/
#define BIT0    0x1
#define BIT(n)  (0x1L<<n) 

#define SECOND_SRCA_GFX              000
#define SECOND_SRCA_LUT              001
#define SECOND_SRCA_NEWALPHAMAP      002
#define SECOND_SRCA_REGISTER         003
#define SECOND_SRCA_FEEDBACKTVGFX    004
#define SECOND_SRCA_V1               005
#define SECOND_SRCA_V2               006
#define SECOND_SRCA_SYNCVIDEO        007    

#define SECOND_SRCB_NORMALALPHA      1
#define SECOND_SRCB_SLOCKALPHA       2

extern void Init2ndAlphaBlend(void);
extern void Enable2ndAlphaBlend(u8 bOnOff);
extern void Enable2ndWhichAlphaBlend(u8 bOnOff, u8 bWhichWindow, u8 bFullScreen);
extern void Select2ndAlphaBlendSrcA(u8 source , u8 bWhichWindow);
extern void Select2ndAlphaBlendSrcB(u8 source);
extern void Select2ndAlphaSrc(u8 bAlphaSrc, u8 bWhichWindow);
extern void Enable2ndAlphaReg(u8 bROn, u8 bGOn, u8 bBOn, u8 bWhichWindow);
extern void Set2ndAlphaReg(u8 bR , u8 bG , u8 bB, u8 bWhichWindow);
extern void CleanUp2ndAlphaBlend(void);
#endif //#if INCLUDE_API_2NDALPHA

/*Synclock Overlay feature*/
extern void InitSLockOverlay(void);
extern void CleanupSLockOverlay(void);
extern void SetSLockOverlayWindow(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom, u8 bWhichWindow);
extern void EnableSLockOverlay(u8 OnOff);
extern void EnableWhichSLockOverlay(u8 OnOff, u8 bWhichWindow);
extern void EnableInterlaceAlpha(u8 bEnable);
extern void EnableInterlaceRGBMagicNumber(u8 bEnable);

    /* Separated Window Alpha Blending */
extern void SelectWindowBlendSrcA(u8 bSrcA, u8 bWin);
extern void SelectWindowBlendSrcB(u8 bSrcB, u8 bWin);
extern void SelectWindowAlphaSrc(u8 bAlphaSrc, u8 bWin);
extern void EnableWindowAlphaBlendSrcA(u8 OnOff);
extern void EnableWindowAlphaBlendSrcB(u8 OnOff);
extern void EnableWindowAlphaBlendAlpha(u8 OnOff);
extern void EnableWindowAlphaBlend(u8 OnOff);

#endif
