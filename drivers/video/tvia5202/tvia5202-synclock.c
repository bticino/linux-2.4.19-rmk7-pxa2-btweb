/*
 *  TVIA CyberPro 5202 synclock interface
 *
 */

#include <linux/delay.h>
#include "tvia5202-def.h"
#include "tvia5202-synclock.h"
#include "tvia5202-misc.h"
#include "tvia5202-synctbl.h"
#include "tvia5202-ks127tbl.h"

static u8 b3CE_B0, b3CE_B1, b3CE_BA;
static u16 wSyncLock = 0; /*whether or not in synclock mode */
static u16 wVStandard = -1; /*internal flag to track TV/Video mode, NTSC or PAL*/
static u16 wVCRMode = 0; /*whether or not in VCR mode*/
static u16 wSyncLockGM= 0; /*whether or not in synclock graphics mode*/

static u16 wE696, wE61C, wE61E;

/*following are some internal variables used by Tvia_ChangeToVideoClk*/
static u8 bEA, bEB, bEC, bED, bEE, bEF, bA6;
static u16 wE5F0, wE5F2, wE5F4, wE5F6, wE4EC;
static u16 wExtClock = 0; /*1: CRT/TV is using external video clock*/

/*****************************************************************************
 DACPower(): Turn on/off DAC 
 in:   iOnOff 1 - turn on DAC
              0 - turn off DAC
 Note: internal use
 *****************************************************************************/
static void DACPower(u16 iOnOff)
{
    OutByte(TVIA3CEINDEX, 0x56);
    OutByte(TVIA3CFDATA, InByte(TVIA3CFDATA) | 0x04);

    if(iOnOff != 0) {
        OutByte(RAMDACMASK, InByte(0x3c6) & 0xBF); /*DAC power on*/
    }
    else {
        OutByte(RAMDACMASK, InByte(0x3c6) | 0x40); /*DAC power off*/
    }

    OutByte(TVIA3CEINDEX, 0x56);
    OutByte(TVIA3CFDATA, InByte(TVIA3CFDATA) & 0xBF);
}

/*****************************************************************************
 Wait for CRT VSync signal off then wait for VSync signal on
 in:   none
 Note: internal use
 *****************************************************************************/
static void WaitCRTVSync(void)
{
    while((InByte(ATTRRESET) & 0x08) == 0x00) /*wait unitl 1*/
    {
		udelay(1000);
	}
    while((InByte(ATTRRESET) & 0x08) != 0x00) /*wait until 0*/
    {
    	udelay(1000);
	}
    while((InByte(ATTRRESET) & 0x08) == 0x00) /*wait until 1*/
    {
    	udelay(1000);
	}
}
 
/*****************************************************************************
 Tvia_SetTVColor: It corrects TV color each time when a new mode is set.
 in:   wScnHeight was set
 out:  None
 Note: This routine is never needed any more for Tvia CyberPro53xx/52xx. Here
       we keep a null function in order to be compatible with SDK50xx.
 *****************************************************************************/
void Tvia_SetTVColor(void)
{
}

/*****************************************************************************
 ResetDisplayFetch() resets display fetch engine
 in:   bReset, 1 - Reset
               0 - UnReset
 out:  none
 Note: internal use
 *****************************************************************************/
static void ResetDisplayFetch(u16 bReset)
{
    if(bReset != 0) {
        Out_Video_Reg(0xDE, In_Video_Reg(0xDE) | 0x40); /*Reset Overlay 1*/
        WriteReg(SEQINDEX, 0xDE, ReadReg(SEQINDEX, 0xDE) | 0x40); /*Reset Overlay 2*/
        Out_Video_Reg(0x70, In_Video_Reg(0x70) | 0x40); /*Reset CRT*/
    }
    else {
        Out_Video_Reg(0xDE, In_Video_Reg(0xDE) & 0xBF); /*Disable Reset Overlay 1 */
        WriteReg(SEQINDEX, 0xDE, ReadReg(SEQINDEX, 0xDE) & 0xBF); /*Disable Reset Overlay 2 */
        Out_Video_Reg(0x70, In_Video_Reg(0x70) & 0xBF); /*Disable Reset CRT*/
    }
}

/*****************************************************************************
 WriteCrt() programs CRT related registers
 in:   wTVStandard - NTSC / PAL
       wResolution - resolution index
 out:  none
 Note: internal use
 *****************************************************************************/
static void WriteCrt(u16 wTVStandard, u16 wResolution)
{
    u8 b3D41F, b3D411;
    u16 wIndex, wTableInx = wResolution;

    ResetDisplayFetch(1);

    b3D41F = In_CRT_Reg(0x1f);
    Out_CRT_Reg(0x1f, 0x3); /*Unlock 1*/
    b3D411 = In_CRT_Reg(0x11);
    Out_CRT_Reg(0x11, b3D411 & 0x7F); /*Unlock 2*/
    
    if(wTVStandard == NTSC) {
        for(wIndex=0; wIndex<14; wIndex++) {
            bOldRegCrt[wIndex] = In_CRT_Reg(bRegIndex[wIndex]);
            
            if(bRegIndex[wIndex] == 0x00) {
                if(wResolution == NTSC720x480) {
                    Out_CRT_Reg(0x00, bNTSCRegCrt[wTableInx][wIndex]);
                }
                else {
                    Out_CRT_Reg( 0x00, bOldRegCrt[wIndex] > 0x66 ?
                        bOldRegCrt[wIndex] : bNTSCRegCrt[wTableInx][wIndex]);
                }
            }
            else {
                Out_CRT_Reg(bRegIndex[wIndex], bNTSCRegCrt[wTableInx][wIndex]);
            }
        }
    }
    else if(wTVStandard == PAL) {
        for(wIndex=0; wIndex<14; wIndex++) {
            bOldRegCrt[wIndex] = In_CRT_Reg(bRegIndex[wIndex]);
            Out_CRT_Reg(bRegIndex[wIndex], bPALRegCrt[wTableInx][wIndex]);
        }
    }
    
    Out_CRT_Reg(0x1f, b3D41F); /*lock 1*/
    Out_CRT_Reg(0x11, ReadReg(0x3D4, 0x11) | 0x80); /*lock 2*/
    
    /*change to 27MHz internal dot clock*/
    b3CE_B0 = In_Video_Reg(0xB0);
    b3CE_B1 = In_Video_Reg(0xB1);
    b3CE_BA = In_Video_Reg(0xBA);
    Out_Video_Reg(0xB0, 0x42);
    Out_Video_Reg(0xB1, 0xD0);
    
    /*For BOARD5300*/
    Out_Video_Reg(0xBA, 0x09);
    
    Out_Video_Reg(0xB9, 0x80);
    Out_Video_Reg(0xB9, 0x00);
    
    ResetDisplayFetch(0);
}

/*****************************************************************************
 CleanupCrt() restores CRT related registers to old values
 in:   none
 out:  none
 Note: internal use
 *****************************************************************************/
static void CleanupCrt(void)
{
    u8 b3D41F, b3D411;
    u16 wIndex;
    
    ResetDisplayFetch(1);
    
    b3D41F = In_CRT_Reg(0x1f);
    Out_CRT_Reg(0x1f, 0x3); /*Unlock 1*/
    b3D411 = In_CRT_Reg(0x11);
    Out_CRT_Reg(0x11, b3D411 & 0x7F); /*Unlock 2*/
    
    for(wIndex=0; wIndex<14; wIndex++) {
        Out_CRT_Reg(bRegIndex[wIndex], bOldRegCrt[wIndex]);
    }
    
    Out_CRT_Reg(0x1f, b3D41F); /*lock 1*/
    Out_CRT_Reg(0x11, ReadReg(0x3D4,0x11) | 0x80); /*lock 2*/
    
    /*change 27MHz internal dot clock*/
    Out_Video_Reg(0xB0, b3CE_B0);
    Out_Video_Reg(0xB1, b3CE_B1);
    Out_Video_Reg(0xBA, b3CE_BA);
    Out_Video_Reg(0xB9, 0x80);
    Out_Video_Reg(0xB9, 0x00);
    ResetDisplayFetch(0);
}

/*****************************************************************************
 Tvia_DoSyncLock() will enable Synclock mode
 in:   wCaptureIndex, PORTA - video comes from video port A
                      PORTB - video comes from video port B
       wTVStandard, NTSC - for NTSC mode TV output
                    PAL  - for PAL mode TV output
       wResolution, in NTSC: 0:640x480, 1:720x480, 2:640x440
                    in PAL : 0:640x480, 1:720x540, 2:768x576, 3:720x576
       bVideoFormat, incoming external video format CCIR656 or CCIR601
 out:  none
 Note: external video mode must be identical with TV output mode(NTSC/PAL)
 *****************************************************************************/
void Tvia_DoSyncLock(u16 wCaptureIndex, u16 wTVStandard, u16 wResolution, u8 bVideoFormat)
{
    u16 nCounter;
    u32 dwData;
    u16 wRegVal, wRegE47C;
    int i;
    
    DACPower(0); /*Turn off DAC to avoid flash effect*/
    /*First enable data and clock path
    Note: those codes vary from different hardware schematic, for your
          special board or settop box, check with Tvia 50xx data book*/
    Out_Video_Reg(0xbf, 0x01);
    Out_Video_Reg_M(0xba, 0x80, 0x7F);
    Out_Video_Reg_M(0xba, 0x00, 0xFD);
    Out_Video_Reg_M(0xbb, 0x02, 0xFD);
    Out_Video_Reg_M(0xbb, 0x04, 0xFB);
    Out_Video_Reg(0xbf, 0x00);
    Out_Video_Reg(0xb5, 0x83);
    
    /*Init one of the two video ports.*/
    if (wTVStandard == NTSC)
    {
      printk("Tvia_DoSyncLock NTSC\n"); 

        /* NTSC */
        wSyncLock = 1;
        wVStandard = NTSC;
        
        /*Blank TV output first, to avoid the flashing effect*/
        wTVReg16[21] = InTVReg(0xE47C);
        OutTVReg(0xE47C, 0x0344);
        
        for (i=0; i<13; i++)
        {
            if(i == 12) /*for 3c4/ef, we should use read-modify-write*/
                Out_SEQ_Reg_M(bRegExtSyncIndex[i],
                    bNTSCRegExtSync[bVideoFormat][wResolution][i], 0xC6);
            else
                Out_SEQ_Reg(bRegExtSyncIndex[i],
                    bNTSCRegExtSync[bVideoFormat][wResolution][i]);
        }
        
        /*sync lock path in alpha  r/w*/
        Out_Video_Reg(0xFA, 0x00);
        Out_SEQ_Reg(0x4A, ReadReg(SEQINDEX, 0x4A) | 0x08);
        
        /*invert uv sync alpha out color r/w*/
        Out_Video_Reg(0xFA, 0x08);
        Out_SEQ_Reg(0x4F, ReadReg(SEQINDEX, 0x4f) | 0xC0);
        Out_SEQ_Reg(0x4F, ReadReg(SEQINDEX, 0x4f) | 0x11);
        Out_SEQ_Reg_M(0xED, 0x00, 0xBF);
        
        if (bVideoFormat == CCIR601)
        {
            Out_SEQ_Reg_M(0xee, 0x01, 0x7E);  /*self odd/even*/
            Out_SEQ_Reg_M(0xEF, 0x01, 0xF6);  /*synclock data comes from
                                               port A*/
            Out_SEQ_Reg_M(0xA6, 0x10, 0xEF);
        }
        else
        {
            Out_SEQ_Reg_M(0xee, 0x00, 0x7E);  /*odd/even field invert*/
            Out_SEQ_Reg_M(0xEF, 0x09, 0xF6);  /*synclock data comes from
                                               port A & format 656 */
            Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
        }
#if 0
        /*
         *  if video source is not exist, system will be hang up, we better
         *  detect here.
         */
        if (!Tvia_DetectSynclockVideoSignal() ) 
        {
            DACPower(1);
            Out_Video_Reg(0xFA, 0x00);
            Out_SEQ_Reg(0x4A, ReadReg(SEQINDEX, 0x4A) & 0xF7);
            Out_SEQ_Reg_M(0xEE, 0x00, 0x7E);  /*odd/even field invert*/
            Out_SEQ_Reg_M(0xEF, 0x00, 0xF6);  /*synclock data comes from
                                               port A & format 656 */
            Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
            return;
        }
#endif
        WriteCrt(wTVStandard, wResolution);
        ResetDisplayFetch(1);
        /*wait 2 vsync*/
        WaitCRTVSync();
        WaitCRTVSync();
        
        Out_SEQ_Reg(0xeb, ReadReg(SEQINDEX, 0xeb) | 0x80); /*Change clock*/
        ResetDisplayFetch(0);
        
        for (i=0; i<4; i++)
            Out_CRT_Reg(0x28+i, bNTSCRegSync[bVideoFormat][wResolution][i]);
        /*Program TV registers  */
        
        for (nCounter = 0; nCounter < 66; nCounter++)
        {
            if (nCounter == 21)
            {
                /*0xE47C, it will be taken care somewhere
                                      else*/
                wRegE47C = wNTSCTVReg16[bVideoFormat][nCounter];
                continue;
            }
            wTVReg16[nCounter] = InTVReg(0xE428 + nCounter * 4);
            OutTVReg(0xE428 + nCounter * 4, wNTSCTVReg16[bVideoFormat][nCounter]);
        }
        
        for (nCounter = 0; nCounter < 59; nCounter++)
        {
            dwData = InTVReg(0xE530 + nCounter * 4 + 2);
            dwData = dwData << 16;
            dwData += InTVReg(0xE530 + nCounter * 4);
            dwTVReg32[nCounter] = dwData;
            OutTVReg(0xE530 + nCounter * 4,
                (u16)((dwNTSCTVReg32[bVideoFormat][nCounter] & 0x0000FFFF)));
            OutTVReg(0xE530 + nCounter * 4 + 2,
                (u16)((dwNTSCTVReg32[bVideoFormat][nCounter] & 0xFFFF0000) >> 16));
        }
        
        /* modify TV registers for each mode from add-on table */
        for (nCounter = 0; nCounter < 34; nCounter++)
        {
            wRegVal = wAddonNTSCTVReg[bVideoFormat][wResolution][nCounter];
            if (wRegVal != (u16)0xffff) {
                if (nCounter == 13)
                    wRegE47C = wRegVal;
                else
                    OutTVReg(wAddonTVindex[nCounter], wRegVal);
            }
        }
        
        {
            /* wait for 15 VDE */
            u16 wTmp;
            for (wTmp = 0; wTmp < 15; wTmp++)
            {
                while((InByte(0x3da) & 0x08) == 0x00) {  /*search until 1*/
                	udelay(1000);
            	}
                while((InByte(0x3da) & 0x08) != 0x00) {  /*search until 0 active low*/
                	udelay(1000);
            	}
            }
            /*Output SyncLock video now*/
            OutTVReg(0xE47C, wRegE47C);
        }
    }
    else if (wTVStandard == PAL)
    {
      printk("Tvia_DoSyncLock PAL\n"); 
      /* PAL */
        wSyncLock = 1;
        wVStandard = PAL;
        
        /*Blank TV output first, to avoid the flashing effect*/
        wTVReg16[21] = InTVReg(0xE47C);
        OutTVReg(0xE47C, 0x0344);
        
        for(i=0; i<13; i++)
        {
            if(i == 12) /*for 3c4/ef, we should use read-modify-write*/
                Out_SEQ_Reg_M(bRegExtSyncIndex[i],
                    bPALRegExtSync[bVideoFormat][wResolution][i], 0xC6);
            else
                Out_SEQ_Reg(bRegExtSyncIndex[i],
                    bPALRegExtSync[bVideoFormat][wResolution][i]);
        }
        
        /*sync lock path in alpha  r/w*/
        Out_Video_Reg(0xfa, 0x00);
        Out_SEQ_Reg(0x4a, ReadReg(SEQINDEX, 0x4a) | 0x08);
        
        /*invert uv sync alpha out color r/w*/
        Out_Video_Reg(0xfa, 0x08);
        Out_SEQ_Reg(0x4f, ReadReg(SEQINDEX, 0x4f) | 0xC0);
        Out_SEQ_Reg(0x4f, ReadReg(SEQINDEX, 0x4f) | 0x11);
        Out_SEQ_Reg_M(0xed, 0x00, 0xBF);
        
        if (bVideoFormat == CCIR601)
        {
            Out_SEQ_Reg_M(0xee, 0x01, 0x7E); /*self odd/even*/
            Out_SEQ_Reg_M(0xEF, 0x01, 0xF6); /*synclock data comes from port A*/
            Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
        }
        else
        {
            Out_SEQ_Reg_M(0xee, 0x00, 0x7E); /*odd/even field invert*/
            Out_SEQ_Reg_M(0xEF, 0x09, 0xF6); /*synclock data comes from port A & format 656 */
            Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
        }
        
        WriteCrt(wTVStandard, wResolution);
        ResetDisplayFetch(1);
        /*wait 2 vsync*/
        WaitCRTVSync();
        WaitCRTVSync();
        Out_SEQ_Reg(0xeb, ReadReg(SEQINDEX, 0xeb) | 0x80); /*Change clock*/
        ResetDisplayFetch(0);
        
        for (i=0; i<4; i++)
            Out_CRT_Reg(0x28+i, bPALRegSync[bVideoFormat][wResolution][i]);
        
        /*Program TV registers  */
        if (wResolution == PAL768x576)
        {
            /*use TV tbl for KS127*/
            for (nCounter = 0; nCounter < 66; nCounter++)
            {
                if (nCounter == 21)
                {
                    wRegE47C = wKsPALTVReg16[bVideoFormat][nCounter];
                    continue;
                }
                wTVReg16[nCounter] = InTVReg(0xE428 + nCounter * 4);
                OutTVReg(0xE428 + nCounter * 4,
                    wKsPALTVReg16[bVideoFormat][nCounter]);
            }
            for (nCounter = 0; nCounter < 59; nCounter++)
            {
                dwData = InTVReg(0xE530 + nCounter * 4 + 2);
                dwData = dwData << 16;
                dwData += InTVReg(0xE530 + nCounter * 4);
                dwTVReg32[nCounter] = dwData;
                OutTVReg(0xE530 + nCounter * 4,
                    (u16)(dwKsPALTVReg32[bVideoFormat][nCounter] & 0x00FFFF));
                OutTVReg(0xE530 + nCounter * 4 + 2,
                    (u16)((dwKsPALTVReg32[bVideoFormat][nCounter] & 0xFFFF0000) >> 16));
            }
        }
        else
        {
            /*use PAL TV tbl for P7111*/
            for (nCounter = 0; nCounter < 66; nCounter++)
            {
                if (nCounter == 21)
                {
                    wRegE47C = wPALTVReg16[bVideoFormat][nCounter];
                    continue;
                }
                wTVReg16[nCounter] = InTVReg(0xE428 + nCounter * 4);
                OutTVReg(0xE428 + nCounter * 4,
                    wPALTVReg16[bVideoFormat][nCounter]);
            }
            for (nCounter = 0; nCounter < 59; nCounter++)
            {
                dwData = InTVReg(0xE530 + nCounter * 4 + 2);
                dwData = dwData << 16;
                dwData += InTVReg(0xE530 + nCounter * 4);
                dwTVReg32[nCounter] = dwData;
                OutTVReg(0xE530 + nCounter * 4,
                    (u16)(dwPALTVReg32[bVideoFormat][nCounter] & 0x00FFFF));
                OutTVReg(0xE530 + nCounter * 4 + 2,
                    (u16)((dwPALTVReg32[bVideoFormat][nCounter] & 0xFFFF0000) >> 16));
            }
        }
        
        /* modify TV registers for each mode from add-on table */
        for (nCounter = 0; nCounter < 34; nCounter++)
        {
            wRegVal = wAddonPALTVReg[bVideoFormat][wResolution][nCounter];
            if (wRegVal != (u16)0xffff) {
                if (nCounter == 13)
                    wRegE47C = wRegVal;
                else
                    OutTVReg(wAddonTVindex[nCounter], wRegVal);
            }
        }
        {
            /*wait for 15 VDE*/
            u16 wTmp;
            for (wTmp = 0; wTmp < 15; wTmp++)
            {
                while((InByte(0x3da) & 0x08 ) == 0x00) { /*search until 1*/
                	udelay(1000);
            	}
                while((InByte(0x3da) & 0x08 ) != 0x00) { /*search until 0 active low*/
                	udelay(1000);
            	}
            }
            /*Output SyncLock video now*/
            OutTVReg(0xE47C, wRegE47C);
        }
    } /* PAL */
    wE696 = InTVReg(0xE696);
    wE61C = InTVReg(0xE61C);
    wE61E = InTVReg(0xE61E);
    OutTVReg(0xE696, 0x400);
    
    if (wTVStandard == PAL)
    {
        if ((wResolution == PAL640x480) ||(wResolution == PAL720x540))
            OutTVReg(0xe61c, 0x30);
        else if (wResolution == PAL720x576)
            OutTVReg(0xe61c, 0x20);
        else
            OutTVReg(0xe61c, 0x24);
    }
    else
        OutTVReg(0xE61C, 0x0024);
    
    OutTVReg(0xE61E, 0x003F);
    OutTVReg(0xE5F6, (InTVReg(0xE5F6) & ~0x0004) | 0x0004);
    DACPower(1); /*Now it's time to turn on DAC*/
}

/*****************************************************************************
 Tvia_CleanupSyncLock() cleanup call Synclock related operation and restore
 to normal graphics mode
 in:  none
 out: none
 *****************************************************************************/
void Tvia_CleanupSyncLock()
{
    u16 wi;
    
    if (wSyncLock == 0)
        return;
    DACPower(0); /*Turn off DAC to avoid flash effect*/
    wSyncLock = 0;
    
    /*Blank TV output first, to avoid the flashing effect*/
    OutTVReg(0xE47C, 0x0344);
    
    /*--> 3ce.70 = cb -- reset */
    CleanupCrt();
    ResetDisplayFetch(1);
    Out_SEQ_Reg(0xeb, ReadReg(SEQINDEX, 0xeb) & 0x7F); /*Change clock*/
    
    /*wait 2 vsync*/
    /*
    WaitCRTVSync();
    WaitCRTVSync();
    */
    
    /*--> 3ce.70 0b */
    ResetDisplayFetch(0);
    Out_SEQ_Reg_M(0xef, 0x00, 0x02);
    
    for (wi = 0; wi < 0xf; wi++)
    {
        if (wi == 3)
            continue;
        else if(wi == 0xD)
            Out_SEQ_Reg_M(0xED, 0x00, 0x9F);
        else
            Out_SEQ_Reg(0xe0 + wi, 0);
    }
    
    Out_CRT_Reg( 0x28, 0x00);
    Out_CRT_Reg( 0x29, 0x00);
    Out_CRT_Reg( 0x2a, 0x00);
    Out_CRT_Reg( 0x2b, 0x00);
    
    /*turn off sync lock path in alpha  r/w*/
    Out_Video_Reg(0xfa, 0x00);
    Out_SEQ_Reg(0x4a, ReadReg(SEQINDEX, 0x4a) & 0xF7);
    Out_Video_Reg(0xfa, 0x08);
    Out_SEQ_Reg_M(0x4f, 0x00, 0xEE);
    Out_Video_Reg(0xfa, 0x00);
    
    /*Restore TV Registers*/
    {
        u16 nCounter;
        for (nCounter = 0; nCounter < 66; nCounter++)
        {
            if (nCounter == 21)
                continue;
            OutTVReg(0xE428 + nCounter * 4, wTVReg16[nCounter]);
        }
        for (nCounter = 0; nCounter < 59; nCounter++)
        {
            OutTVReg(0xE530 + nCounter * 4, (u16)(dwTVReg32[nCounter] & 0x00FFFF));
            OutTVReg(0xE530 + nCounter * 4 + 2,
                (u16)(dwTVReg32[nCounter] >> 16));
        }
    }
    {
        /* wait for 15 VDE */
        u16 wTmp;
        
        for (wTmp = 0; wTmp < 15; wTmp++)
        {
            while((InByte(0x3da) & 0x08) == 0x00) { /*search until 1*/
            	udelay(1000);
        	}
            while((InByte(0x3da) & 0x08) != 0x00) { /*search until 0 active low*/
            	udelay(1000);
        	}
        }
        
        /*Output SyncLock video now*/
        OutTVReg(0xE47C, wTVReg16[21]);
    }
    
    DACPower(1); /*Now it's time to turn on DAC*/
    
    OutTVReg(0xE696, wE696);
    OutTVReg(0xE61C, wE61C);
    OutTVReg(0xE61E, wE61E);
}

/*****************************************************************************
 Tvia_SetVCRMode() will use self-generated internal clock instead of external
 video one, for instance, some VCR video source is quite unstable, video on
 TV will be jerking, after calling this function, video quality will be
 better.
 in:   bVCRMode, 1 - Enable VCR mode
                 0 - Disable VCR mode
 out:  none
 Note: DoSyncLock() must be called before calling this function
 *****************************************************************************/
void Tvia_SetVCRMode(u8 bVCRMode)
{
    if (wSyncLock == 0)
        return;
    ResetDisplayFetch(1);
    if (bVCRMode != 0)
    {
        WriteReg(SEQINDEX, 0xEB, ReadReg(SEQINDEX, 0xEB) & 0x7F);
        OutTVReg(0xE478, 0x330);
    }
    else
    {
        WriteReg(SEQINDEX, 0xEB, ReadReg(SEQINDEX, 0xEB) | 0x80);
        OutTVReg(0xE478, 0x338);
    }
    ResetDisplayFetch(0);
    wVCRMode = bVCRMode;
}

/*****************************************************************************
 Tvia_GetTVMode() detect TV output mode
 in:   none
 out:  PAL  - TV output mode is PAL mode
       NTSC - TV output mode is NTSC mode
 Note: This function only works fine when TV output is enabled
 *****************************************************************************/
u16 Tvia_GetTVMode(void)
{
    u16 wTmp;
    wTmp = InTVReg(0xE438);
    if ((wTmp & 0x1000) != 0)
        return NTSC;
    else
        return PAL;
}

/*****************************************************************************
 Tvia_ChangeToVideoClk() changes CRT/TV clock to external video clock or
 restores it to internal clock.
 in:   wCaptureIndex, PORTA - video comes from video port A
                      PORTB - video comes from video port B
       wTVStandard, NTSC - for NTSC mode TV output
                    PAL  - for PAL mode TV output
       bVideoFormat, incoming external video format CCIR656 or CCIR601
       bSet, 1 - change to external video clock
             0 - restore it.
       wResolution, the screen resolution used
 Note: This function in fact is part of Tvia_DoSyncLock() except this does not
       bypass external video to TV output.
 *****************************************************************************/
void Tvia_ChangeToVideoClk(u16 wCaptureIndex, u16 wTVStandard, u8 bVideoFormat, 
    u8 bSet, u16 wResolution)
{
    int i;
    
    if (bSet)
    {
        /*force CRT/TV clock follow external video clock*/
        if (wExtClock) /*Already in*/
            return;
        
        wExtClock = 1;
        DACPower(0); /*Turn off DAC to avoid flash effect*/
        bEA = ReadReg(SEQINDEX, 0xea);
        bEB = ReadReg(SEQINDEX, 0xeb);
        bEC = ReadReg(SEQINDEX, 0xec);
        bED = ReadReg(SEQINDEX, 0xed);
        bEE = ReadReg(SEQINDEX, 0xee);
        bEF = ReadReg(SEQINDEX, 0xef);
        bA6 = ReadReg(SEQINDEX, 0xA6);
        
        wE5F0 = InTVReg(0xE5F0);
        wE5F2 = InTVReg(0xE5F2);
        wE5F4 = InTVReg(0xE5F4);
        wE5F6 = InTVReg(0xE5F6);
        wE4EC = InTVReg(0xE4EC);
        
        Out_SEQ_Reg_M(0xec, 0x00, 0xEB);
        
        if (wTVStandard == NTSC)
        {
            Out_SEQ_Reg(0xe4, 0x50);
            Out_SEQ_Reg(0xe5, 0x03);
            Out_SEQ_Reg_M(0xeb, 0x80, 0x7F);
            Out_SEQ_Reg_M(0xed, 0x00, 0xBF);
            
            if (bVideoFormat == CCIR601)
            {
                Out_SEQ_Reg_M(0xee, 0x01, 0x7E);
                Out_SEQ_Reg_M(0xEF, 0x31, 0x06);
                Out_SEQ_Reg_M(0xA6, 0x10, 0xEF);
            }
            else
            {
                Out_SEQ_Reg_M(0xee, 0x80, 0x7E);
                Out_SEQ_Reg_M(0xEF, 0x39, 0x06);
                Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
            }
            
            for (i=0; i<4; i++)
                Out_CRT_Reg( 0x28+i,
                    bNTSCRegSync[bVideoFormat][wResolution][i]);
            
            Out_SEQ_Reg_M(0xec, 0x00, 0xEB);
            OutTVReg(0xE5f0, 0x010f);
            OutTVReg(0xE5f2, 0x0dd0);
            OutTVReg(0xE5f4, 0xf300);
            OutTVReg(0xE4EC, wE4EC | 0x0008);
            OutTVReg(0xE5F6, wE5F6 |
                ((u8)((dwNTSCTVReg32[bVideoFormat][49] & 0x00FF0000) >> 16) & 0x0030));
        }
        else
        {
            /*PAL TV mode*/
            Out_SEQ_Reg(0xe4, 0x56);
            Out_SEQ_Reg(0xe5, 0x03);
            Out_SEQ_Reg_M(0xeb, 0x80, 0x7F);
            Out_SEQ_Reg_M(0xed, 0x00, 0xBF);
            
            if (bVideoFormat == CCIR601)
            {
                Out_SEQ_Reg_M(0xee, 0x81, 0x7E); /*self odd/even*/
                Out_SEQ_Reg_M(0xEF, 0x31, 0x06);
                Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
            }
            else
            {
                Out_SEQ_Reg_M(0xee, 0x00, 0x7E); /*odd/even field invert*/
                Out_SEQ_Reg_M(0xEF, 0x39, 0x06);
                Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
            }
            
            for (i=0; i<4; i++)
                Out_CRT_Reg( 0x28+i,
                    bPALRegSync[bVideoFormat][wResolution][i]);
            
            Out_SEQ_Reg_M(0xec, 0x00, 0xEB);
            OutTVReg(0xE5f0, 0xE150);
            OutTVReg(0xE5f2, 0x080d);
            OutTVReg(0xE5f4, 0xED0D);
            OutTVReg(0xE5F6, wE5F6 |
                ((u8)((dwPALTVReg32[bVideoFormat][49] & 0x00FF0000) >> 16) & 0x0030));
        } /*PAL TV mode*/
    }
    else
    {
        /*Restore CRT/TV clock to original one(internal clock)*/
        
        if (wExtClock == 0) /*Already out*/
            return;
        DACPower(0); /*Turn off DAC to avoid flash effect*/
        wExtClock = 0;
        
        Out_SEQ_Reg(0xe4, 0x00);
        Out_SEQ_Reg(0xe5, 0x00);
        Out_SEQ_Reg_M(0xeb, bEB, 0x7F);
        Out_SEQ_Reg_M(0xee, bEE, 0x7E);
        Out_SEQ_Reg_M(0xef, bEF, 0x02);
        Out_SEQ_Reg_M(0xed, bED, 0xBF);
        Out_SEQ_Reg_M(0xA6, bA6, 0xEF);
        Out_CRT_Reg( 0x28, 0x00);
        Out_CRT_Reg( 0x29, 0x00);
        Out_CRT_Reg( 0x2a, 0x00);
        Out_CRT_Reg( 0x2b, 0x00);
        OutTVReg(0xE5f0, wE5F0);
        OutTVReg(0xE5f2, wE5F2);
        OutTVReg(0xE5f4, wE5F4);
        OutTVReg(0xE4EC, wE4EC);
        OutTVReg(0xE5F6, wE5F6);
        
        Out_SEQ_Reg_M(0xec, bEC, 0xEB);
    }
    
    DACPower(1); /*Now it's time to turn on DAC*/
}

/*****************************************************************************
 Tvia_SetSyncLockGM() will set a graphics mode almost same Synclock mode
 except that there is no video output to TV.
 in:   wCaptureIndex, PORTA - video comes from video port A
                      PORTB - video comes from video port B
       wTVStandard, NTSC - for NTSC mode TV output
                    PAL  - for PAL mode TV output
       wResolution, in NTSC: 0:640x480, 1:720x480, 2:640x440
                    in PAL : 0:640x480, 1:720x540, 2:768x576, 3:720x576
       bVideoFormat, incoming external video format CCIR656 or CCIR601
 out:  none
 Note: external video mode must be identical with TV output mode(NTSC/PAL)
 *****************************************************************************/
void Tvia_SetSyncLockGM(u16 wCaptureIndex, u16 wTVStandard, u16 wResolution, u8 bVideoFormat)
{
    u16 nCounter;
    u32 dwData;
    u16 wRegVal, wRegE47C;
    int i;
    
    DACPower(0); /*Turn off DAC to avoid flash effect*/
    /*First enable data and clock path
    Note: those codes vary from different hardware schematic, for your
          special board or settop box, check with Tvia 53xx data book*/
    Out_Video_Reg(0xbf, 0x01);
    Out_Video_Reg_M(0xba, 0x80, 0x7F);
    Out_Video_Reg_M(0xba, 0x00, 0xFD);
    Out_Video_Reg_M(0xbb, 0x02, 0xFD);
    Out_Video_Reg_M(0xbb, 0x04, 0xFB);
    Out_Video_Reg(0xbf, 0x00);
    Out_Video_Reg(0xb5, 0x83);
    
    /* Init one of the two video ports.*/
    if (wTVStandard == NTSC)
    {
        /* NTSC */
        wSyncLockGM = 1;
        wVStandard = NTSC;
        
        /*Blank TV output first, to avoid the flashing effect*/
        wTVReg16[21] = InTVReg(0xE47C);
        OutTVReg(0xE47C, 0x0344);
        
        for (i=0; i<13; i++)
        {
            if(i == 12) /*for 3c4/ef, we should use read-modify-write*/
                Out_SEQ_Reg_M(bRegExtSyncIndex[i],
                    bNTSCRegExtSync[bVideoFormat][wResolution][i], 0xC6);
            else
                Out_SEQ_Reg(bRegExtSyncIndex[i],
                    bNTSCRegExtSync[bVideoFormat][wResolution][i]);
        }
        
        Out_SEQ_Reg_M(0xed, 0x00, 0xBF);
        
        if (bVideoFormat == CCIR601)
        {
            Out_SEQ_Reg_M(0xee, 0x01, 0x7E); /*self odd/even*/
            Out_SEQ_Reg_M(0xEF, 0x01, 0xF6); /*synclock data comes from
                                              port A*/
            Out_SEQ_Reg_M(0xA6, 0x10, 0xEF);
        }
        else
        {
            Out_SEQ_Reg_M(0xee, 0x00, 0x7E); /*odd/even field invert*/
            Out_SEQ_Reg_M(0xEF, 0x09, 0xF6); /*synclock data comes from
                                              port A & format 656 */
            Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
        }
        
        WriteCrt(wTVStandard, wResolution);
        ResetDisplayFetch(1);
        /*wait 2 vsync*/
        WaitCRTVSync();
        WaitCRTVSync();
        Out_SEQ_Reg(0xeb, ReadReg(SEQINDEX, 0xeb) | 0x80); /*Change clock*/
        ResetDisplayFetch(0);
        
        for (i=0; i<4; i++)
            Out_CRT_Reg( 0x28+i,
                bNTSCRegSync[bVideoFormat][wResolution][i]);
        /*Program TV registers*/
        for (nCounter = 0; nCounter < 66; nCounter++)
        {
            if (nCounter == 21)
            {
                /*0xE47C, it will be taken care somewhere
                                    else*/
                wRegE47C = wNTSCTVReg16[bVideoFormat][nCounter];
                continue;
            }
            wTVReg16[nCounter] = InTVReg(0xE428 + nCounter * 4);
            OutTVReg(0xE428 + nCounter * 4,
                wNTSCTVReg16[bVideoFormat][nCounter]);
        }
        
        for (nCounter = 0; nCounter < 59; nCounter++)
        {
            dwData = InTVReg(0xE530 + nCounter * 4 + 2);
            dwData = dwData << 16;
            dwData += InTVReg(0xE530 + nCounter * 4);
            dwTVReg32[nCounter] = dwData;
            OutTVReg(0xE530 + nCounter * 4,
                (u16)(dwNTSCTVReg32[bVideoFormat][nCounter] & 0x00FFFF));
            if(nCounter != 49) /*Do not modify E5F6 here*/
                OutTVReg(0xE530 + nCounter * 4 + 2,
                    (u16)(dwNTSCTVReg32[bVideoFormat][nCounter] >> 16));
            else
                OutTVReg(0xE530 + nCounter * 4 + 2,
                    (u16)(dwNTSCTVReg32[bVideoFormat][nCounter] >> 16) & 0x30);
        }
        
        /* modify TV registers for each mode from add-on table */
        for (nCounter = 0; nCounter < 34; nCounter++)
        {
            wRegVal = wAddonNTSCTVReg[bVideoFormat][wResolution][nCounter];
            if (wRegVal != (u16)0xffff) {
                if (nCounter == 13)
                    wRegE47C = wRegVal;
                else
                    OutTVReg(wAddonTVindex[nCounter], wRegVal);
            }
        }
        {
            /* wait for 15 VDE */
            u16 wTmp;
            for (wTmp = 0; wTmp < 15; wTmp++)
            {
                while((InByte(0x3da) & 0x08) == 0x00) { /*search until 1*/
                	udelay(1000);
            	}
                while((InByte(0x3da) & 0x08) != 0x00) { /*search until 0 active low*/
                	udelay(1000);
            	}
            }
            /*Output SyncLock video now*/
            OutTVReg(0xE47C, wRegE47C);
        }
    }
    else if (wTVStandard == PAL)
    {
        /* PAL */
        wSyncLockGM = 1;
        wVStandard = PAL;
        
        /*Blank TV output first, to avoid the flashing effect*/
        wTVReg16[21] = InTVReg(0xE47C);
        OutTVReg(0xE47C, 0x0344);
        
        for (i=0; i<13; i++)
        {
            if(i == 12) /*for 3c4/ef, we should use read-modify-write*/
                Out_SEQ_Reg_M(bRegExtSyncIndex[i],
                    bPALRegExtSync[bVideoFormat][wResolution][i], 0xC6);
            else
                Out_SEQ_Reg(bRegExtSyncIndex[i],
                    bPALRegExtSync[bVideoFormat][wResolution][i]);
        }
        Out_SEQ_Reg_M(0xed, 0x00, 0xBF);
        
        if (bVideoFormat == CCIR601)
        {
            Out_SEQ_Reg_M(0xee, 0x01, 0x7E); /*self odd/even*/
            Out_SEQ_Reg_M(0xEF, 0x31, 0xC6); /*synclock data comes from port A*/
            Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
        }
        else
        {
            Out_SEQ_Reg_M(0xee, 0x00, 0x7E); /*odd/even field invert*/
            Out_SEQ_Reg_M(0xEF, 0x39, 0xC6); /*synclock data comes from port A & format 656 */
            Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
        }
        
        WriteCrt(wTVStandard, wResolution);
        ResetDisplayFetch(1);
        /*wait 2 vsync*/
        WaitCRTVSync();
        WaitCRTVSync();
        Out_SEQ_Reg(0xeb, ReadReg(SEQINDEX, 0xeb) | 0x80); /*Change clock*/
        ResetDisplayFetch(0);
        
        for (i=0; i<4; i++)
            Out_CRT_Reg( 0x28+i,
                bPALRegSync[bVideoFormat][wResolution][i]);
        
        /*Program TV registers*/
        if (wResolution == PAL768x576)
        {
            /*use TV tbl for KS127*/
            for (nCounter = 0; nCounter < 66; nCounter++)
            {
                if (nCounter == 21)
                {
                    wRegE47C = wKsPALTVReg16[bVideoFormat][nCounter];
                    continue;
                }
                wTVReg16[nCounter] = InTVReg(0xE428 + nCounter * 4);
                OutTVReg(0xE428 + nCounter * 4,
                    wKsPALTVReg16[bVideoFormat][nCounter]);
            }
            for (nCounter = 0; nCounter < 59; nCounter++)
            {
                dwData = InTVReg(0xE530 + nCounter * 4 + 2);
                dwData = dwData << 16;
                dwData += InTVReg(0xE530 + nCounter * 4);
                dwTVReg32[nCounter] = dwData;
                OutTVReg(0xE530 + nCounter * 4,
                    (u16)(dwKsPALTVReg32[bVideoFormat][nCounter] & 0x00FFFF));
                if(nCounter != 49) /*Do not modify E5F6 here*/
                    OutTVReg(0xE530 + nCounter * 4 + 2,
                        (u16)(dwKsPALTVReg32[bVideoFormat][nCounter] >> 16));
                else
                    OutTVReg(0xE530 + nCounter * 4 + 2,
                        (u16)(dwKsPALTVReg32[bVideoFormat][nCounter] >> 16) & 0x30);
            }
        }
        else
        {
            /*use PAL TV tbl for P7111*/
            for (nCounter = 0; nCounter < 66; nCounter++)
            {
                if (nCounter == 21)
                {
                    wRegE47C = wPALTVReg16[bVideoFormat][nCounter];
                    continue;
                }
                wTVReg16[nCounter] = InTVReg(0xE428 + nCounter * 4);
                OutTVReg(0xE428 + nCounter * 4,
                    wPALTVReg16[bVideoFormat][nCounter]);
            }
            for (nCounter = 0; nCounter < 59; nCounter++)
            {
                dwData = InTVReg(0xE530 + nCounter * 4 + 2);
                dwData = dwData << 16;
                dwData += InTVReg(0xE530 + nCounter * 4);
                dwTVReg32[nCounter] = dwData;
                OutTVReg(0xE530 + nCounter * 4,
                    (u16)(dwPALTVReg32[bVideoFormat][nCounter] & 0x00FFFF));
                if(nCounter != 49) /*Do not modify E5F6 here*/
                    OutTVReg(0xE530 + nCounter * 4 + 2,
                        (u16)(dwPALTVReg32[bVideoFormat][nCounter] >> 16));
                else
                    OutTVReg(0xE530 + nCounter * 4 + 2,
                        (u16)(dwPALTVReg32[bVideoFormat][nCounter] >> 16) & 0x30);
            }
        }
        
        /*modify TV registers for each mode from add-on table*/
        for (nCounter = 0; nCounter < 34; nCounter++)
        {
            wRegVal = wAddonPALTVReg[bVideoFormat][wResolution][nCounter];
            if (wRegVal != (u16)0xffff) {
                if (nCounter == 13)
                    wRegE47C = wRegVal;
                else
                    OutTVReg(wAddonTVindex[nCounter], wRegVal);
            }
        }
        {
            /* wait for 15 VDE */
            u16 wTmp;
            for (wTmp = 0; wTmp < 15; wTmp++)
            {
                while((InByte(0x3da) & 0x08) == 0x00) { /*search until 1*/
                	udelay(1000);
            	}
                while((InByte(0x3da) & 0x08) != 0x00) { /*search until 0 active low*/
                	udelay(1000);
            	}
            }
            /*Output SyncLock video now*/
            OutTVReg(0xE47C, wRegE47C);
        }
    } /* PAL */
    
    DACPower(1); /*Now it's time to turn on DAC*/
}
    
/*****************************************************************************
 Tvia_CleanupSyncLockGM() cleanup call Synclock graphics mode and restore to
 normal graphics mode
 in:  none
 out: none
 *****************************************************************************/
void Tvia_CleanupSyncLockGM(void)
{
    u16 wi;
    
    if (wSyncLockGM == 0)
        return;
    DACPower(0); /*Turn off DAC to avoid flash effect*/
    wSyncLockGM = 0;
    
    /*Blank TV output first, to avoid the flashing effect*/
    OutTVReg(0xE47C, 0x0344);
    
    /*--> 3ce.70 = cb -- reset */
    CleanupCrt();
    ResetDisplayFetch(1);
    Out_SEQ_Reg(0xeb, ReadReg(SEQINDEX, 0xeb) &0x7F); /*Change clock*/
    
    /*wait 2 vsync*/
    WaitCRTVSync();
    WaitCRTVSync();
    
    /*--> 3ce.70 0b */
    ResetDisplayFetch(0);
    Out_SEQ_Reg_M(0xef, 0x00, 0x02);
    
    for (wi = 0; wi < 0xf; wi++)
    {
        if (wi == 3)
            continue;
        else if(wi == 0xD)
            Out_SEQ_Reg_M(0xED, 0x00, 0x9F);
        else
            Out_SEQ_Reg(0xe0 + wi, 0);
    }
    
    Out_CRT_Reg( 0x28, 0x00);
    Out_CRT_Reg( 0x29, 0x00);
    Out_CRT_Reg( 0x2a, 0x00);
    Out_CRT_Reg( 0x2b, 0x00);
    /*Restore TV Registers*/
    {
        u16 nCounter;
        for (nCounter = 0; nCounter < 66; nCounter++)
        {
            if (nCounter == 21)
                continue;
            OutTVReg(0xE428 + nCounter * 4, wTVReg16[nCounter]);
        }
        for (nCounter = 0; nCounter < 59; nCounter++)
        {
            OutTVReg(0xE530 + nCounter * 4, (u16)(dwTVReg32[nCounter] & 0x00FFFF));
            OutTVReg(0xE530 + nCounter * 4 + 2,
                (u16)(dwTVReg32[nCounter] >> 16));
        }
    }
    {
        /*wait for 15 VDE*/
        u16 wTmp;
        for (wTmp = 0; wTmp < 15; wTmp++)
        {
            while((InByte(0x3da) & 0x08) == 0x00) { /*search until 1*/
            	udelay(1000);
        	}
            while((InByte(0x3da) & 0x08) != 0x00) { /*search until 0 active low*/
            	udelay(1000);
        	}
        }
        
        /*Output SyncLock video now*/
        OutTVReg(0xE47C, wTVReg16[21]);
    }
    
    DACPower(1); /*Now it's time to turn on DAC*/
}

/*****************************************************************************
 Tvia_OutputSyncLockVideo() turns on/off SyncLock video output under SyncLock
 graphics mode.
 in:   wTVStandard, NTSC - for NTSC mode TV output
                    PAL  - for PAL mode TV output
       wResolution, in NTSC: 0:640x480, 1:720x480, 2:640x440
                    in PAL : 0:640x480, 1:720x540, 2:768x576, 3:720x576
       bVideoFormat, incoming external video format CCIR656 or CCIR601
 out:  none
 Note: Tvia_SetSyncLockGM() must be called before call this function
 *****************************************************************************/
void Tvia_OutputSyncLockVideo(u16 bOutput, u16 wTVStandard, u16 wResolution, u8 bVideoFormat)
{     
    u16 wRegVal;
    if(wSyncLockGM == 0)
        return;
    if(bOutput != 0)
    {
        /*sync lock path in alpha  r/w*/
        Out_Video_Reg(0xfa, 0x00);
        Out_SEQ_Reg(0x4a, ReadReg(0x3c4, 0x4a) | 0x08);
        
        /*invert uv sync alpha out color r/w*/
        Out_Video_Reg(0xfa, 0x08);
        Out_SEQ_Reg(0x4f, ReadReg(0x3c4, 0x4f) | 0xC0);
        Out_SEQ_Reg(0x4f, ReadReg(0x3c4, 0x4f) | 0x11);
        Out_Video_Reg(0xfa, 0x00);
        if(wTVStandard == NTSC)
        {
            /* modify TV registers for each mode from add-on table */
            wRegVal = wAddonNTSCTVReg[bVideoFormat][wResolution][0];
            if (wRegVal != (u16)0xffff)
                OutTVReg(0xE5F6, wRegVal);
            else
                OutTVReg(0xE5F6,
                    (u16)(dwNTSCTVReg32[bVideoFormat][49] >> 16));
        }
        else
        {
            /* modify TV registers for each mode from add-on table */
            wRegVal = wAddonPALTVReg[bVideoFormat][wResolution][0];
            if (wRegVal != (u16)0xffff)
                OutTVReg(0xE5F6, wRegVal);
            else
                OutTVReg(0xE5F6,
                    (u16)(dwPALTVReg32[bVideoFormat][49] >> 16));
        }
    }
    else
    {
        Out_Video_Reg(0xfa, 0x00);
        Out_SEQ_Reg(0x4a, ReadReg(0x3c4, 0x4a) & 0xF7);
        
        Out_Video_Reg(0xfa, 0x08);
        Out_SEQ_Reg(0x4f, ReadReg(0x3c4, 0x4f) & 0xEE);
        Out_Video_Reg(0xfa, 0x00);
        if(wTVStandard == NTSC)
        {
            /* modify TV registers for each mode from add-on table */
            wRegVal = wAddonNTSCTVReg[bVideoFormat][wResolution][0];
            if (wRegVal != (u16)0xffff)
                OutTVReg(0xE5F6, wRegVal & 0x30);
            else
                OutTVReg(0xE5F6,
                    (u16)(dwNTSCTVReg32[bVideoFormat][49] >> 16) & 0x30);
        }
        else
        {
            /* modify TV registers for each mode from add-on table */
            wRegVal = wAddonPALTVReg[bVideoFormat][wResolution][0];
            if (wRegVal != (u16)0xffff)
                OutTVReg(0xE5F6, wRegVal & 0x30);
            else
                OutTVReg(0xE5F6,
                    (u16)(dwPALTVReg32[bVideoFormat][49] >> 16) & 0x30);
        }
    }
}

/****************************************************************************
 Tvia_SelectSynclockPath() specifies a path that Synclock engine will 
 get data & clock from which video port.
 In  : wWhichPort = 0, Video Port A 
                  = 1, Video Port B 
                  = 2, Video Port C
 Out : None                      
 Note: This routine should be called before calling 
       Tvia_DoSyncLock(), Tvia_ChangeToVideoClk() or Tvia_SetSyncLockGM().
 ****************************************************************************/
void Tvia_SetSynclockPath(u8 bWhichPort)
{
    switch (bWhichPort)
    {
    case PORTA:
        WriteRegM(CRTINDEX, 0x96, 0x00, 0xFC);
        WriteRegM(CRTINDEX, 0xE1, 0x00, 0xFC); /*Port A uses Pin 43*/
        Out_SEQ_Reg_M(0xEF, 0x04, 0xFB);  /*synclock data comes from port A*/
        break;
    case PORTB:
        WriteRegM(CRTINDEX, 0x96, 0x00, 0xFC);
        WriteRegM(CRTINDEX, 0xE1, 0x00, 0xF3); /*Port B uses Pin 44*/
        Out_SEQ_Reg_M(0xEF, 0x00, 0xFB);  /*synclock data comes from port B*/
        break;
    case PORTC:
        WriteRegM(CRTINDEX, 0x96, 0x03, 0xFC);
    default:
        break;
    }
}

/****************************************************************************
 Tvia_DetectSynclockVideoSignal() will try to detect whether incoming
 video is  exisiting or not.
 In  : bVideoFormat: either CCIR601 or CCIR656
 Out : 1 if incoming video is existing
 Note: This routine must be called right after Tvia_SelectSynclockPath() to
       detect synclock video signal, and before doing any other Synclock APIs
 ****************************************************************************/
int Tvia_DetectSynclockVideoSignal(u8 bVideoFormat) 
{
    u32 i;
    u8 sig1, sig2;
    int ok = 0;
    
    /* Partialy enable synclock */
    Out_Video_Reg(0xBF, 0x01);          /* Enable pin in/out */
    Out_Video_Reg_M(0xBA, 0x80, 0x7F);
    Out_Video_Reg_M(0xBA, 0x00, 0xFD);
    Out_Video_Reg_M(0xBB, 0x02, 0xFD);
    Out_Video_Reg_M(0xBB, 0x04, 0xFB);
    Out_Video_Reg(0xBF, 0x00);
    Out_Video_Reg(0xB5, 0x83);          /* enable PAD */
    
    /* Init one of the two video ports.*/
    for (i=0; i<13; i++) 
    {
        /* For 3C4/EF, we should use read-modify-write */
        if (bRegExtSyncIndex[i] == 0xEF)
            Out_SEQ_Reg_M(bRegExtSyncIndex[i], bNTSCRegExtSync[bVideoFormat][0][i], 0xC6);
        else
            Out_SEQ_Reg(bRegExtSyncIndex[i], bNTSCRegExtSync[bVideoFormat][0][i]);
    }
    /* sync lock path in alpha  r/w*/
    Out_Video_Reg(0xFA, 0x00);
    Out_SEQ_Reg(0x4A, ReadReg(SEQINDEX, 0x4A) | 0x08);
    
    /*invert uv sync alpha out color r/w*/
    Out_Video_Reg(0xFA, 0x08);
    Out_SEQ_Reg(0x4F, ReadReg(SEQINDEX, 0x4f) | 0xC0);
    Out_SEQ_Reg(0x4F, ReadReg(SEQINDEX, 0x4f) | 0x11);
    Out_SEQ_Reg_M(0xED, 0x00, 0xBF);
    
    if (bVideoFormat == CCIR601) 
    {
        Out_SEQ_Reg_M(0xEE, 0x01, 0x7E);  /*self odd/even*/
        Out_SEQ_Reg_M(0xEF, 0x01, 0xF6);  /*synclock data comes from
                                           port A*/
        Out_SEQ_Reg_M(0xA6, 0x10, 0xEF);
    }
    else 
    {
        Out_SEQ_Reg_M(0xEE, 0x00, 0x7E);  /*odd/even field invert*/
        Out_SEQ_Reg_M(0xEF, 0x09, 0xF6);  /*synclock data comes from
                                           port A & format 656 */
        Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
    }
    /* detect to see whether V and H Sync exist */
    OutByte(0x3CE, 0xFA);
    OutByte(0x3CF, 0x00);
    OutByte(0x3D4, 0x7D);
    sig1 = InByte(0x3D5) & 0x0C;
    for (i=0; i<0x800000; i++) 
    {
        sig2 = InByte(0x3D5) & 0x0C;
        if (sig1 != sig2) 
        {
            ok = 1;
            break;
        }
    }
    /* restore */
    /* Turn off sync lock path in alpha r/w */
    Out_Video_Reg(0xFA, 0x00);
    Out_SEQ_Reg(0x4A, ReadReg(SEQINDEX, 0x4A) & 0xF7);
    Out_Video_Reg(0xFA, 0x08);
    Out_SEQ_Reg_M(0x4F, 0x00, 0xEE);
    Out_Video_Reg(0xFA, 0x00);
    Out_SEQ_Reg_M(0xA6, 0x00, 0xEF);
    
    /* Cleanup Synclock registers */
    for (i=0; i<13; i++) 
    {
        if (bRegExtSyncIndex[i] == 0xEF)
            Out_SEQ_Reg_M(bRegExtSyncIndex[i], 0x0, 0xC6);
        else
            Out_SEQ_Reg(bRegExtSyncIndex[i], 0);
    }
    Out_SEQ_Reg_M(0xEE, 0x00, 0x7E);
    Out_SEQ_Reg_M(0xEF, 0x00, 0xF6);
    return ok;
}

