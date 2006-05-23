/*
 *  TVIA CyberPro 5202 utility functions
 *
 */

#include "tvia5202-def.h"
#include "tvia5202-misc.h"

extern volatile unsigned char *CyberRegs;

// !!!raf
#define tvia_inb(reg) (*(volatile unsigned char  *)(CyberRegs + (reg | ( (reg & 0x03) << 12) )  ))   /* NO BYTE ENABLE MODE */
#define tvia_inw(reg)	(*(volatile unsigned short *)(CyberRegs + (reg | ( (reg & 0x03) << 12) ))) // ORIG
//#define tvia_inl(reg)	(*(volatile unsigned long  *)(CyberRegs + (reg | ( (reg & 0x03) << 12) ))) // ORIG


void OutTVReg(u32 dwReg, u16 wIndex)
{
	*((volatile unsigned short *)(CyberRegs + dwReg + 0x000B0000)) = wIndex;
}

void OutTVRegM(u32 dwReg, u16 wIndex, u16 wMask)
{
	u16 wTmp;

//	wTmp = (*((volatile unsigned short *)(CyberRegs + dwReg + 0x000B0000))) & wMask; ORIG
   wTmp = ((tvia_inw( (dwReg + 0x000B0000) )) & wMask); // !!!raf-12-1-2006

   wTmp |= (wIndex & ~wMask);
	*((volatile unsigned short *)(CyberRegs + dwReg + 0x000B0000)) = wTmp;
}

u16 InTVReg(u32 dwReg)
{
//	return *((volatile unsigned short *)(CyberRegs + dwReg + 0x000B0000)); ORIG
	return tvia_inw( (dwReg + 0x000B0000) );
}

void OutByte(u16 wReg, u8 bIndex)
{
	*((volatile unsigned char *)(CyberRegs + wReg)) = bIndex;
}

/* ORIG
u8 InByte(u16 wReg)
{
//	return *((volatile unsigned char *)(CyberRegs + wReg)); //ORIG
}
*/

u8 InByte(u16 wReg)
{
	//return (*(volatile unsigned char  *)(CyberRegs + (wReg | ( (wReg & 0x03) << 12) )  ));
	return tvia_inb(wReg);
}



void OutWord(u16 wReg, u16 wIndex)
{
	*((volatile unsigned short *)(CyberRegs + wReg)) = wIndex;
}
void WriteReg(u16 wReg, u8 bIndex, u8 bData)
{
    *((volatile unsigned char *)(CyberRegs + wReg)) = bIndex;
    *((volatile unsigned char *)(CyberRegs + wReg + 1)) = bData;
}

/* ORIG 
u8 ReadReg(u16 wReg, u8 bIndex)
{
    *((volatile unsigned char *)(CyberRegs + wReg)) = bIndex; 
//    return *((volatile unsigned char *)(CyberRegs + wReg + 1)); ORIG
}
*/

u8 ReadReg(u16 wReg, u8 bIndex)
{
    *((volatile unsigned char *)(CyberRegs + wReg)) = bIndex;
//    return *((volatile unsigned char *)(CyberRegs + wReg + 1));
  return tvia_inb( (wReg+1) );

}

/****************************************************************************
 This routine is used internally
 WriteRegM() sets a register with bit mask
 in:   wReg   - register base
       bIndex - register index
       bData  - bit/bits to be set/clear, untouched bit/bits must be zero
       bMask  - bit/bits to be masked, 1:mask, 0:to be set/clear
       e.g. set <4,3>=<1,0>, bValue=0x10 and bMask=E7
 Note: none
*****************************************************************************/
void WriteRegM(u16 wReg, u8 bIndex, u8 bData, u8 bMask)
{
    u8 bTemp;

    *((volatile unsigned char *)(CyberRegs + wReg)) = bIndex;

//    bTemp = (*((volatile unsigned char *)(CyberRegs + wReg + 1))) & bMask; // ORIG
    bTemp = tvia_inb( (wReg+1) ) & bMask; //!!!raf

    bTemp |= (bData & ~bMask);
    *((volatile unsigned char *)(CyberRegs + wReg + 1)) = bTemp;
}

u8 In_Video_Reg(u8 bIndex)
{
	*((volatile unsigned char *)(CyberRegs + 0x3ce)) = bIndex;
// return *((volatile unsigned char *)(CyberRegs + 0x3cf)); //ORIG
	return tvia_inb(0x3cf);
}

/****************************************************************************
 This routine is used internally
 Out_Video_Reg() sets general video related register to a special value
 in:   bIndex - video register index
       bValue - value to be set
 out:  none
 Note: Register base is 0x3CE
 ****************************************************************************/
void Out_Video_Reg(u8 bIndex, u8 bValue)
{
    *((volatile unsigned char *)(CyberRegs + 0x3ce)) = bIndex;
    *((volatile unsigned char *)(CyberRegs + 0x3cf)) = bValue;
}

/****************************************************************************
 This routine is used internally
 Out_Video_Reg_M() sets general video related register with bit mask
 in:   bIndex - video register index
       bValue - bit/bits to be set/clear, untouched bit/bits must be zero
       bMask  - bit/bits to be masked, 1:mask, 0:to be set/clear
       e.g. set <4,3>=<1,0>, bValue=0x10 and bMask=E7
 Note: Register base is 0x3CE
 ****************************************************************************/
void Out_Video_Reg_M(u8 bIndex, u8 bValue, u8 bMask)
{
    u8 bTemp;

    *((volatile unsigned char *)(CyberRegs + 0x3ce)) = bIndex;
// ORIG    bTemp = (*((volatile unsigned char *)(CyberRegs + 0x3cf))) & bMask;
    bTemp = tvia_inb(0x3cf) & bMask;

    bTemp |= (bValue & ~bMask);
    *((volatile unsigned char *)(CyberRegs + 0x3cf)) = bTemp;
}

u8 In_SEQ_Reg(u8 bIndex)
{
	*((volatile unsigned char *)(CyberRegs + 0x3c4)) = bIndex;
//	return *((volatile unsigned char *)(CyberRegs + 0x3c5)); ORIG
	return tvia_inb(0x3c5);
}

/****************************************************************************
 This routine is used internally
 Out_SEQ_Reg() sets 0x3C4 Based register to a special value
 in:   bIndex - video register index
       bValue - value to be set
 out:  none
 Note: Register base is 0x3C4
 ****************************************************************************/
void Out_SEQ_Reg(u8 bIndex, u8 bValue)
{
    *((volatile unsigned char *)(CyberRegs + 0x3c4)) = bIndex;
    *((volatile unsigned char *)(CyberRegs + 0x3c5)) = bValue;
}

/****************************************************************************
 This routine is used internally
 Out_SEQ_Reg_M() sets 0x3C4 based register with bit mask
 in:   bIndex - video register index
       bValue - bit/bits to be set/clear, untouched bit/bits must be zero
       bMask  - bit/bits to be masked, 1:mask, 0:to be set/clear
       e.g. set <4,3>=<1,0>, bValue=0x10 and bMask=E7
 Note: Register base is 0x3C4
 ****************************************************************************/
void Out_SEQ_Reg_M(u8 bIndex, u8 bValue, u8 bMask)
{
    u8 bTemp;

    *((volatile unsigned char *)(CyberRegs + 0x3c4)) = bIndex;
//    bTemp = (*((volatile unsigned char *)(CyberRegs + 0x3c5))) & bMask; ORIG
    bTemp = tvia_inb(0x3c5) & bMask;

    bTemp |= (bValue & ~bMask);
    *((volatile unsigned char *)(CyberRegs + 0x3c5)) = bTemp;
}

u8 In_CRT_Reg(u8 bIndex)
{
	*((volatile unsigned char *)(CyberRegs + 0x3d4)) = bIndex;
//	return *((volatile unsigned char *)(CyberRegs + 0x3d5)); ORIG
	return tvia_inb(0x3d5);
}

/****************************************************************************
This routine is used internally
Out_CRT_Reg() sets 0x3D4 Based register to a special value
in:   bIndex - video register index
      bValue - value to be set
out:  none                      
Note: Register base is 0x3D4
*****************************************************************************/
void Out_CRT_Reg(u8 bIndex, u8 bValue)
{            
    *((volatile unsigned char *)(CyberRegs + 0x3d4)) = bIndex;
    *((volatile unsigned char *)(CyberRegs + 0x3d5)) = bValue;
}

/****************************************************************************
This routine is used internally
Out_CRT_Reg_M() sets 0x3D4 based register with bit mask
in:   bIndex - video register index
      bValue - bit/bits to be set/clear, untouched bit/bits must be zero
      bMask  - bit/bits to be masked, 1:mask, 0:to be set/clear
e.g.  set <4,3>=<1,0>, bValue=0x10 and bMask=E7
Note: Register base is 0x3D4
*****************************************************************************/
void Out_CRT_Reg_M(u8 bIndex, u8 bValue, u8 bMask)
{            
    u8 bTemp;

    printk("Out_CRT_Reg_M 0\n");
    *((volatile unsigned char *)(CyberRegs + 0x3d4)) = bIndex;
    printk("Out_CRT_Reg_M 1\n");
//    bTemp = (*((volatile unsigned char *)(CyberRegs + 0x3d5))) & bMask; ORIG
    bTemp=tvia_inb(0x3d5) & bMask;

     printk("Out_CRT_Reg_M 4\n");
   bTemp |= (bValue & ~bMask);
    printk("Out_CRT_Reg_M 4\n");
    *((volatile unsigned char *)(CyberRegs + 0x3d5)) = bTemp;
    printk("Out_CRT_Reg_M 4\n");

}

/*----------------------------------------------------------------------
  GetBpp() returns current graphics mode color depth
  In:  None
  Out: return 8 - 8 Bpp (256 color mode)
             16 - 16 Bpp (High color mode)
             24 - 24 Bpp (True color mode)
             32 - 32 Bpp (True color mode)
 ----------------------------------------------------------*/
u16 GetBpp(void)
{
    u8 bModeBpp;
    
    OutByte(TVIA3CEINDEX, 0x77);
    bModeBpp = InByte(TVIA3CFDATA) & 0x07;
    switch (bModeBpp) {
    case 0xA:                      /*444*/
    case 2:                        /*565*/
    case 6: bModeBpp = 16; break;  /*555*/
    case 3: bModeBpp = 32; break;  /*x888*/
    case 4: bModeBpp = 24; break;  /*888*/
    case 1: bModeBpp =  8; break;  /*8bpp index*/
    default:bModeBpp =  4; break;  /*4bpp VGA modes*/
    }
    return ((u16)bModeBpp);
}
