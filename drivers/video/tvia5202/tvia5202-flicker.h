/*
 *  TVIA CyberPro 5202 flicker include file
 *
 */

#ifndef _TVIA5202_OVERLAY_H_
#define _TVIA5202_OVERLAY_H_

#include "tvia5202-def.h"

#define tvia_outw(dat,reg)	((*(unsigned short *)(CyberRegs + (reg))) = dat)

#define WriteTVReg(reg,dat)	tvia_outw(dat,reg+0x000B0000)

//#define tvia_inw(reg)	(*(unsigned short *)(CyberRegs + (reg))) // ORIG
#define tvia_inw(reg)	(*(volatile unsigned short *)(CyberRegs + (reg | ( (reg & 0x03) << 12) ))) // ORIG

#define ReadTVReg(reg)	tvia_inw( (reg+0x000B0000) )

extern void FlickFree(u16 iOnOff);
extern void FlickerWinInside(u16 iOnOff);
extern void SetTVFlickFreeWin(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom);
extern void FlickerCtrlCoefficient(u8 alpha,u8 beta, u8 gama);

#endif
