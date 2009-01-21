/*
 *   TVIA CyberPro 5202 I2C interface include file
 *
 */

#ifndef _TVIA5202_I2C_H_
#define _TVIA5202_I2C_H_

extern void InitI2C(void);
extern u8 DetectI2C(u8 bAddr);
extern u8 ReadOneByte(u8 bAdr, u8 bSubAdr, u8 *bData);
extern u8 ReadMoreByte(u8 bAdr, u8 bSubAdr, u8 *bDataPtr, u16 wCount);
extern u8 SendOneByte(u8 bAdr, u8 bSubAdr, u8 bData);
extern u8 SendMoreByte(u8 bAddr, u8 bSubaddr, u8 *bValPtr, u16 wCount);
#endif
