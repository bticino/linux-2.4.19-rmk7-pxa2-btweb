/*
 *   TVIA CyberPro 5202 I2C interface
 *
 */

#include <linux/delay.h>
#include <linux/sched.h>
#include "tvia5202-def.h"
#include "tvia5202-i2c.h"
#include "tvia5202-misc.h"

#define I2C_MASK 0xA0  /*I2C control bit mask*/

#define I2C_SCL_RW 0x20 /*Serial Clock read/write*/
#define I2C_SDA_RW 0x80 /*Serial Data read/write*/

#define I2C_SCL_DISABLE 0x10 /* Disbale the Clock Output Buffer*/
#define I2C_SDA_DISABLE 0x40 /* Disbale the Data Output Buffer*/

#define MAX_WAIT_STATES 1000
#define ACK 0   /*I2C Acknowledge*/
#define NACK 1  /*I2C No Acknowledge*/

#define I2CDELAY 50

static void ReadSCL(u16 *data)
{
    OutByte(0x3ce, 0xb6);
    OutByte(0x3cf, (InByte(0x3cf) | I2C_SCL_DISABLE) & ~I2C_MASK);

    *data = (InByte(0x3cf) & I2C_SCL_RW) ? 1:0;
}

static void ReadSDA(u16 *data)
{
    OutByte(0x3ce, 0xb6);
	OutByte(0x3cf, (InByte(0x3cf) | I2C_SDA_DISABLE) & ~I2C_MASK);

    *data = (InByte(0x3cf) & I2C_SDA_RW) ? 1:0;
}

static void SetSDALine(void)
{
    OutByte(0x3ce, 0xb6);
    OutByte(0x3cf, (InByte(0x3cf) | I2C_SDA_DISABLE) & ~I2C_MASK);
    udelay(I2CDELAY);
}

static void ResetSDALine(void)
{
    OutByte(0x3ce, 0xb6);
    OutByte(0x3cf, (InByte(0x3cf) & ~I2C_SDA_DISABLE) & ~I2C_MASK);
    udelay(I2CDELAY);
}

static void SetSCLLine(void)
{
    OutByte(0x3ce, 0xb6);
    OutByte(0x3cf, (InByte(0x3cf) | I2C_SCL_DISABLE) & ~I2C_MASK);
    udelay(I2CDELAY);
}

static void ResetSCLLine(void)
{
    OutByte(0x3ce, 0xb6);
    OutByte(0x3cf, (InByte(0x3cf) & ~I2C_SCL_DISABLE) & ~I2C_MASK);
    udelay(I2CDELAY);
}

static void WaitHighSCLLine(void)
{
    u16 data_in;
    u16 retries = MAX_WAIT_STATES;

    do {
        ReadSCL(&data_in);
        if(data_in)
            break;
    } while(retries--);
}

static void Start_Cycle(void)
{
    SetSDALine();
    SetSCLLine();
    WaitHighSCLLine();
    ResetSDALine();
    ResetSCLLine();
}

static void Stop_Cycle(void)
{
    ResetSCLLine();
    ResetSDALine();
    SetSCLLine();
    WaitHighSCLLine();
    SetSDALine();
    /* schedule, we don't want to lock the computer */
    schedule();
}

static u16 Acknowledge(void)
{
    u16 ack;

    ResetSCLLine();
    SetSDALine();
    SetSCLLine();
    ReadSDA(&ack);
    ResetSCLLine();
    return ack;
}

static u8 ReadI2C(u8 ack)
{
    u16 data=0;
    u8 bRet=0;
    int i;

    ResetSCLLine();
    SetSDALine();

    for(i=0; i<8; i++) {
        ResetSCLLine();
        ResetSCLLine();
        SetSCLLine();
        WaitHighSCLLine();
        ReadSDA(&data);
        bRet <<= 1;
        bRet |= (data == 1);
    }

    ResetSCLLine();
    if (ack)
        SetSDALine();
    else
        ResetSDALine();

    SetSCLLine(); 
    WaitHighSCLLine();
    ResetSCLLine();
    return bRet;
}

static u16 WriteI2C(u8 bVal)
{
    int  i;

    for(i=0; i<8 ; i++) {
        ResetSCLLine();
        if (bVal & 0x80)
            SetSDALine();
        else
            ResetSDALine();
        SetSCLLine();
        WaitHighSCLLine();
        bVal <<= 1;
    }
    return (Acknowledge());
}

void InitI2C(void)
{
    /*Disable all I2C access first*/
    WriteReg(0x3ce, 0xBF, 0x01);
    OutByte(0x3ce, 0xB0);
    OutByte(0x3cf, InByte(0x3cf) & ~0x11);
    WriteReg(0x3ce, 0xBF, 0x00);
    OutByte(0x3ce, 0xB6);
    OutByte(0x3cf, (InByte(0x3cf) & ~0x50) | 0x50);
}

u8 DetectI2C(u8 bAddr)
{
    u8 bReturn = 0x01;

    Start_Cycle();
    if(WriteI2C(bAddr))
        bReturn = 0x00;
    Stop_Cycle();
    //udelay(50000);
    return bReturn;
}

u8 ReadOneByte(u8 bAdr, u8 bSubAdr, u8 *bData)
{
    Start_Cycle();

    if(WriteI2C(bAdr)) {
        Stop_Cycle();
        return 0x00;
    }

    if(WriteI2C(bSubAdr)) {
        Stop_Cycle();
        return 0x00;
    }

    Stop_Cycle();
    Start_Cycle();
    if(WriteI2C(bAdr+0x01)) {
        Stop_Cycle();
        return 0x00;
    }

    *bData = ReadI2C(NACK);
    Stop_Cycle();
    return 0x01;
}

u8 ReadMoreByte(u8 bAdr, u8 bSubAdr, u8 *bDataPtr, u16 wCount)
{
    u8 bData;
    u16 i;

    Start_Cycle();
    if(WriteI2C(bAdr)) {
        Stop_Cycle();
        return 0x00;
    }

    if(WriteI2C(bSubAdr)) {
        Stop_Cycle();
        return 0x00;
    }

    Stop_Cycle();
    Start_Cycle();
    if(WriteI2C(bAdr+0x01)) {
        Stop_Cycle();
        return 0x00;
    }

    for(i=wCount; i>0; i--) {
        bData = ReadI2C((i == 1) ? NACK : ACK);
        *bDataPtr++ = bData;
    }

    Stop_Cycle();
    return 0x01;
}

u8 SendOneByte(u8 bAdr, u8 bSubAdr, u8 bData)
{
    Start_Cycle();

    if(WriteI2C(bAdr)) {
        Stop_Cycle();
        return 0x00;
    }

    if(WriteI2C(bSubAdr)) {
        Stop_Cycle();
        return 0x00;
    }

    if(WriteI2C(bData)) {
        Stop_Cycle();
        return 0x00;
    }

    Stop_Cycle();
    return 0x01;
}

u8 SendMoreByte(u8 bAddr, u8 bSubaddr, u8 *bValPtr, u16 wCount)
{
    u8 bVal;
    u16 i;

    Start_Cycle();

    if(WriteI2C(bAddr)) {
        Stop_Cycle();
        return 0x00;
    }

    if(WriteI2C(bSubaddr)) {
        Stop_Cycle();
        return 0x00;
    }

    for(i=0; i<wCount; i++) {
        bVal = *bValPtr++;
        if(WriteI2C(bVal)) {
            Stop_Cycle();
            return 0x00;
        }
    }

    Stop_Cycle();
    return 0x01;
}
