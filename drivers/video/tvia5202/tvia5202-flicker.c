/*
 *  TVIA CyberPro 5202 flicker interface
 *
 */

#include "tvia5202-def.h"
#include "tvia5202-flicker.h"
#include "tvia5202-misc.h"
#include "tvia5202-dvideo.h"

#define tvia_outb(dat,reg)	((*(unsigned char  *)(CyberRegs + (reg))) = dat)

#define tvia_inb(reg) (*(volatile unsigned char  *)(CyberRegs + (reg | ( (reg & 0x03) << 12) )  ))   /* NO BYTE ENABLE MODE */
#define tvia_inb_novlio(reg)	(*(unsigned char  *)(CyberRegs + (reg)))  /* ORIG */

extern volatile unsigned char *CyberRegs;       /* mapped I/O base */

/*-------------------------------------------------------------------------------------
   FlickFree(int iOnOff);
   Turn on/off Flicker-Free control:
   in: iOnOff
       ON - Flick free control is on
       OFF- Flick free control is off

   Note:  ON - Good for graphics text display, say browser application.
         OFF - It gives better TV quality to full screen video playback (DVD playback)
  -------------------------------------------------------------------------------------*/
void FlickFree(u16 iOnOff)
{
    u16 wTmp;
    wTmp  = ReadTVReg(0xE438) & ~0x2000;
    (iOnOff == OFF) ? WriteTVReg(0xe438,wTmp) : WriteTVReg(0xe438,wTmp | 0x2000);
}

/*----------------------------------------------------------------------------------
  FlickerWinInside: set the Flicker free control inside or outside a rectangle (window)
             In   : iOnOff
                     ON : flicker free inside the window is on and outside is off
                    OFF : flicker free outside the window is on and inside is off
             Out  :  ON(INSIDE):   Flicker Free is enabled inside.
                     OFF(OUTSIDE): Flicker Free is enabled outside.
  -----------------------------------------------------------------------------------*/
void FlickerWinInside(u16 iOnOff)
{
    u16 wTmp;
    wTmp  = ReadTVReg(0xE5F6) & ~0x0001;
    (iOnOff == OFF) ? WriteTVReg(0xe5F6,wTmp ) : WriteTVReg(0xe5F6,wTmp |0x01);
}

/*---------------------------------------------------------------------------
  SetTVFlickFreeWin : Set a flicker free control Window (a rectangle)
  In:  Left,Top,Right,Bottom (x0,y0,x1,y1): the coordinate of the window
  Out: The flicker free control window is set

  Note: follow the calling procedures below:
  example 1:
    1. SetTVFlickFreeWin(x0,y0,x1,y1);
    2. FlickerWinInside(ON);
    3. FlickFree(ON)
           \ /
     ----------------
    |x0,y0           |
    |   --------     |
    |  |Flicker |    |
    |  |Free is |    |
    |  | on     |    |
    |   --------x1,y1|
    | Outside is off |
     ----------------
 This case is good for application to have full screen video (DVD) playback
 and a drop down or pop-up window with graphics/text


  example 2:
    1. SetTVFlickFreeWin(x0,y0,x1,y1);
    2. FlickerWinInside(OFF);
    3. FlickFree(ON)
           \  /
     ----------------
    |x0,y0           |
    |   --------     |
    |  |Flicker |    |
    |  |Free is |    |
    |  | off    |    |
    |   --------x1,y1|
    |Outside is On   |
     ----------------
 This case is good for application to have full screen graphics/text and
 a windowed video (DVD) playback

 See real code in SDK3

  -------------------------------------------------------------------------*/
void SetTVFlickFreeWin(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom)
{
    u16 i,wHT = 0, wHRS = 0, wVT,wVRS;
    u8 bGRA_11;
    u8 bCRTReg[8][2]=
    {
        {
            0x00,0
        }, /* 0x3D4_0 */
        {
            0x04,0
        }, /* 0x3D4_4 */
        {
            0x06,0
        }, /* 0x3D4_6 */
        {
            0x07,0
        }, /* 0x3D4_7 */
        {
            0x10,0
        }, /* 0x3D4_10 */
        {
            0x44,0
        }, /* 0x3D4_44 */
        {
            0x45,0
        }, /* 0x3D4_45 */
        {
            0x53,0
        }  /* 0x3D4_53 */
    };
    
    for(i=0; i<sizeof(bCRTReg)/2;i++)
    {
        //tviaoutp(CRTINDEX, bCRTReg[i][0]);
		//OutWord(CRTINDEX, (u16) bCRTReg[i][0]);
		tvia_outb(bCRTReg[i][0],CRTINDEX);
        //bCRTReg[i][1] = tviainp(CRTDATA);
		//bCRTReg[i][1] = InByte(CRTDATA);
		bCRTReg[i][1] = tvia_inb(CRTDATA);
    }
    //tviaoutp(GRAINDEX, 0x11);
	//OutWord(GRAINDEX, 0x11);
	tvia_outb(0x11,GRAINDEX);
    //bGRA_11 = tviainp(GRADATA);
	//bGRA_11 = InByte(GRADATA);
	bGRA_11 = tvia_inb(GRADATA);
    
    /*SWDD_NEW: Calculate the H_Base & V_Base from CRT register values*/
    if ( (bCRTReg[7][1]&0x40) != 0 )
    {
        wHT = bCRTReg[7][1] & 0x01;
        wHT <<= 8;   /* Bit 8 */
        
        wHRS = bCRTReg[7][1] & 0x04;
        wHRS <<= 6;    /* Bit 8 */
    }
    wHT     = (wHT + (bCRTReg[0][1] + 5)) * 8;
    wHRS     = (wHRS + bCRTReg[1][1]) * 8;
    
    wVT     = (bGRA_11&0x01)<<10;             /* bit 10 */
    wVT    |= (bCRTReg[3][1] & 0x20)<<4;      /* bit 9 */
    wVT    |= (bCRTReg[3][1] & 0x01)<<8;      /* bit 8 */
    wVT    |= bCRTReg[2][1] + 2;              /* Bit 7,6,5,4,3,2,1,0  */
    wVRS     = (bGRA_11 & 0x04)<< 8;          /* bit 10 */
    wVRS    |= (bCRTReg[3][1]&0x80)<< 2;      /* bit 9 */
    wVRS    |= (bCRTReg[3][1]&0x04)<< 6;      /* bit 8 */
    wVRS    |= bCRTReg[4][1];                 /* Bit 7,6,5,4,3,2,1,0  */
    
    WriteTVReg(0xE514,(wHT - wHRS + wLeft) / 2 + 1);
    WriteTVReg(0xE518,(wHT - wHRS + wRight) / 2 + 1);
    
    if(bCRTReg[6][1] & 0x10)
    {
        /* TV V-Underscan */
        WriteTVReg(0xE51c,wVT - wVRS + wTop - wTop/(bCRTReg[5][1] & 0x0F) + 1);
        WriteTVReg(0xE520,wVT - wVRS + wBottom - wBottom/(bCRTReg[5][1] & 0x0F) + 1);
    } 
    else 
    {
        /* TV V-Overscan */
        WriteTVReg(0xE51c,wVT - wVRS + wTop - 1);
        WriteTVReg(0xE520,wVT - wVRS + wBottom - 1);
    }
}
/*---------------------------------------------------------------------------------
  FlickerCtrlCoefficient : Set the Flicker Control Coefficient
                     In  :   alpha ,beta ,gama
                     Out : None
                    Note :
                    1. Central Line = alpha * L0 + beta * L1 + gama * L2
                    2. Recommand to obey the regular below to get the better
                       video image quality.
                       255 >= alpha + beta + gama
---------------------------------------------------------------------------------*/
void FlickerCtrlCoefficient(u8 alpha,u8 beta, u8 gama)
{
    u16 wTmp;
    
    wTmp = ReadTVReg(0xE628);
    wTmp = (wTmp&~0xff) | alpha;                           /* 0xE628 */
    wTmp = (wTmp &~0xff00) | (beta<<8);                    /* 0xE629 */
    WriteTVReg(0xE628,wTmp);
    WriteTVReg(0xE62A,(ReadTVReg(0xE62A) &~0xff) | gama);  /* 0xE62A */
}
