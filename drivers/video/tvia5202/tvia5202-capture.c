/*
 *  TVIA CyberPro 5202 capture interface
 *
 */

#include <linux/delay.h>
#include "tvia5202-def.h"
#include "tvia5202-capture.h"
#include "tvia5202-overlay.h"
#include "tvia5202-misc.h"

#define TIMEOUT 100
#define MAX_CAP_ENG_NUM 2

static u16 wMaxCaptureEngineNum = 2;
static u16 wCaptureEngineIndex = 0; /* Internal variable to track which video 
                                       capture engine we are using 0: Capture 1, 
                                       1: Capture 2.*/
static u32 dwCapDstAddr; /*for compatible with SDK50xx*/

extern volatile unsigned char *CyberRegs;


static u16 wCapturePortIndex[MAX_CAP_ENG_NUM] = 
{
    0, 0
};

#if 0
static u8 bV_Coeff[8][5] = /*scalar vertical coefficient*/
{
    {0x00, 0x00, 0x42, 0x8f, 0x00}, 
    {0x00, 0x00, 0x42, 0x8f, 0x00}, 
    {0x00, 0x00, 0x42, 0x8f, 0x00},
    {0x00, 0x00, 0x4a, 0x99, 0x00}, 
    {0x40, 0x80, 0x00, 0x00, 0x00},
    {0x40, 0x80, 0x00, 0x00, 0x00}, 
    {0x20, 0xc0, 0x00, 0x00, 0x00},
    {0x80, 0x40, 0x00, 0x00, 0x00},
};

static u8 bH_Coeff[8][7] = /*scalar horizontal coefficient*/
{
    {0x07, 0x24, 0x1a, 0x24, 0x28, 0x29, 0x00}, 
    {0x07, 0x24, 0x1a, 0x24, 0x28, 0x29, 0x00}, 
    {0x02, 0x13, 0x22, 0x35, 0x44, 0x4a, 0x00},
    {0x03, 0x04, 0x19, 0x3c, 0x5e, 0x6c, 0x01}, 
    {0x1f, 0x04, 0x27, 0x0f, 0x48, 0x84, 0x04}, 
    {0x02, 0x2d, 0x04, 0x24, 0x47, 0xa9, 0x09}, 
    {0x03, 0x1b, 0x20, 0x2c, 0x34, 0xc8, 0x0b}, 
    {0x04, 0x40, 0x15, 0x15, 0x16, 0xe9, 0x0b},
};

static u8 bScalarBaseW = 255; /*for ScalarRatioAdjustment()*/
static u8 bScalarBaseH = 255; /*for ScalarRatioAdjustment()*/
#endif

/****************************************************************************
 WaitVSync() waits for incoming video VSync signal.
 In  : None                                            
 Out : None
 Note: Be careful before calling this function, make sure you have programed
       you decoder properly and there is a video output to video port, and 
       make sure you've called Tvia_InitCapture() before calling this 
       function, otherwise, there will be a dead loop.
 ****************************************************************************/
static void WaitVSync(void)
{
    u16 wCounter = 0;
    u8 b3CE_F7, b3CE_FA;

    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xFA, 0x03);
    Out_Video_Reg(0xF7, 0x00);

    if(wCaptureEngineIndex == CAPENGINE1) {/*if it is Capture Engine 1*/

        while((In_Video_Reg(CAPTURE_TEST_CTL) & 0x80) == 0x00) {
            udelay(1000);
            wCounter++;
            if(wCounter >= TIMEOUT) break;
        }
        wCounter = 0;
        while((In_Video_Reg(CAPTURE_TEST_CTL) & 0x80) != 0x00) {
            udelay(1000);
            wCounter++;
            if(wCounter >= TIMEOUT) break;
        }
    }
    else if(wCaptureEngineIndex == CAPENGINE2) {/*if it is Capture Engine 2*/

       wCounter = 0;
        while((In_Video_Reg(CAPTURE_TEST_CTL) & 0x20) == 0x00) {
            udelay(1000);
            wCounter++;
            if(wCounter >= TIMEOUT) break;
        }
        wCounter = 0;
        while((In_Video_Reg(CAPTURE_TEST_CTL) & 0x20) != 0x00) {
            udelay(1000);
            wCounter++;
            if(wCounter >= TIMEOUT) break;
        }
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}    

/****************************************************************************
 sleep_VSync() is another time dealy routine that waits for some incoming 
 video VSync signals.
 In  : None                                            
 Out : Nnone
 Note: Be careful before calling this function, make sure you have programed
       you decoder properly and there is a video output to video port, and 
       make sure you've called Tvia_InitCapture() before calling this 
       function, otherwise, there will be a dead loop.
 ****************************************************************************/
static void sleep_VSync(u16 wait)
{
    u16 i;
    for(i=0; i<wait; i++)
        WaitVSync();
}

/****************************************************************************
 In_Capture_Reg() gets Capture related register value.
 In  : bIndex - Capture register index
 Out : byte value of that register
 Note: wCaptureIndex must be set before calling this routine.
 ****************************************************************************/
static u8 In_Capture_Reg(u8 bIndex)
{
    u8 bValue;

    if(wCaptureEngineIndex == CAPENGINE1) {
		bValue = In_Video_Reg(bIndex);
	}
    else {
        bValue = In_CRT_Reg(bIndex);
    }

    return bValue;
}

/****************************************************************************
 Out_Capture_Reg() sets Capture related register to a special value.
 In  :  bIndex - Capture register index
        bValue - value to be set
 Out :  None                      
 Note:  wCaptureIndex must be set before calling this routine.
 ****************************************************************************/
static void Out_Capture_Reg(u8 bIndex, u8 bValue)
{                      
    if(wCaptureEngineIndex == CAPENGINE1) {
        Out_Video_Reg(bIndex, bValue);
    }
    else {
        Out_CRT_Reg(bIndex, bValue);
    }
}

/****************************************************************************
 Out_Capture_Reg_M() sets Capture related register with bit mask.
 In  : bIndex - Capture register index
       bValue - bit/bits to be set/clear, untouched bit/bits must be zero
       bMask  - bit/bits to be masked, 1:mask, 0:to be set/clear
       e.g. set <4,3> = <1,0>, bValue=0x10 and bMask = E7
 Out : None
 Note: wCaptureIndex must be set before calling this routine.
 ****************************************************************************/
static void Out_Capture_Reg_M(u8 bIndex, u8 bValue, u8 bMask)
{      
    if(wCaptureEngineIndex == CAPENGINE1) {
        Out_Video_Reg_M(bIndex, bValue, bMask);
    }
    else {
        Out_CRT_Reg_M(bIndex, bValue, bMask);
    }
}

/****************************************************************************
 Tvia_SelectCaptureIndex() specifies which Video Port will be used.
 In  : wIndex = 0, video port A
              = 1, video port B
 Out : None                      
 Note: 1. This is an SDK50xx-style function that should be replaced with both
          Tvia_SelectCaptureEngineIndex() and Tvia_SetCapturePath() in 
          SDK53xx/SDK52xx.
       2. This routine should be called before calling any other Capture 
          functions, and should be called only once except switching to 
          another video port.
 ****************************************************************************/
void Tvia_SelectCaptureIndex(u16 wIndex)
{
	Tvia_SetCapturePath(wIndex, wCaptureEngineIndex);
}

/****************************************************************************
 Tvia_GetCaptureIndex() is used to query video port index.
 In  : None
 Out : Return the video port index which is currently used.                      
       0 - video port A
       1 - video port B
 ****************************************************************************/
u16 Tvia_GetCaptureIndex(void)
{
    return wCapturePortIndex[wCaptureEngineIndex];
}

/****************************************************************************
 Tvia_InitCapture() initializes video Capture engine.
 In  : CCIR601 - initialize capture engine to fetch video data in CCIR601 
                 format
       CCIR656 - in CCIR656 format
 out : None
 Note: 1. wCaptureIndex must be set before calling this routine.
       2. You should program video decoder to output CCIR601 or CCIR656 
          video data to video port accordingly
 ****************************************************************************/
void Tvia_InitCapture(u16 wCCIR656)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    /* First enable data and clock path
     * Note: those codes vary from different hardware schematic, for your
     *       special board or settop box, check with TVIA 53xx data book. */
    Out_Video_Reg(0xBF, 0x01);
    Out_Video_Reg_M(0xBA, 0x80, 0x7F);
    Out_Video_Reg_M(0xBA, 0x00, 0xFD);
    Out_Video_Reg_M(0xBB, 0x02, 0xFD);
    Out_Video_Reg_M(0xBB, 0x04, 0xFB);
    Out_Video_Reg(0xBF, 0x00);

    /*First Disable SafeGuard*/
    if(wCaptureEngineIndex == CAPENGINE1)
        Out_SEQ_Reg_M(0xA2, 0x00, 0xFB);
    else 
        Out_CRT_Reg_M(0x92, 0x00, 0xFB);

    /* Disable double buffer */
//    Tvia_SyncCapDoubleBuffer(wCaptureEngineIndex, OFF);
//    Tvia_EnableCapDoubleBuffer(wCaptureEngineIndex, OFF, INTERLACED);

	Out_Video_Reg_M(0xB5, 0x83, 0x7C); /* Disable Vafc  first!!!*/

    /* Depend on your h/w connection */
    Out_Capture_Reg(VFAC_CTL_MODE_II, 0x10);

    if(wCCIR656 == CCIR601) {
//        printk("InitCapture: CCIR601\n");
        Out_Capture_Reg(CAPTURE_MODE_I, 0x10);
        Out_Capture_Reg(CAPTURE_MODE_II, 0x10);
        /*Turn on odd/even signal self-generator*/
        Out_SEQ_Reg_M(0xA4, 0x11, 0xEE);
    }
    else {
//        printk("InitCapture: CCIR565\n");

        Out_Capture_Reg(CAPTURE_MODE_I, 0x13);
        Out_Capture_Reg(CAPTURE_MODE_II, 0x12);
        /*Turn off odd/even signal self-generator*/
        Out_SEQ_Reg(0xA4, ReadReg(SEQINDEX, 0xA4) & 0xEE);
    }
 
    Out_Capture_Reg_M( VFAC_CTL_MODE_I, 0x6D, 0x92); /*Video capture enable, but freezed*/

    /* Init one of the two video ports.*/
    if(wCapturePortIndex[wCaptureEngineIndex] == PORTA) {
      
        printk("InitCapture: PORTA\n");

        /*Out_SEQ_Reg_M(0xEF, 0x00, ~0x02);*/
        Out_SEQ_Reg_M(0xA0, 0x00, 0x7F);
        Out_SEQ_Reg_M(0xED, 0x00, 0xEF);
    }
    else if(wCapturePortIndex[wCaptureEngineIndex] == PORTB) {                   
        /*Out_SEQ_Reg_M(0xEF, 0x02, ~0x02);*/

        printk("InitCapture: PORTB\n");
 
        Out_SEQ_Reg_M(0xA0, 0x80, 0x7F);
        Out_SEQ_Reg_M(0xED, 0x10, 0xEF);
        Out_Video_Reg_M(VFAC_CTL_MODE_III, 0x00, 0x22);
    } 
    Out_Capture_Reg(CAPTURE_DDA_X_INIT_L, 0x00);
    Out_Capture_Reg(CAPTURE_DDA_X_INIT_H, 0x08);
    Out_Capture_Reg(CAPTURE_DDA_Y_INIT_L, 0x00);
    Out_Capture_Reg(CAPTURE_DDA_Y_INIT_H, 0x08);
    Out_Video_Reg(CAPTURE_BANK_CTL, 0x00);
    Out_Capture_Reg(CAPTURE_NEW_CTL_I, 0x3F);
	Out_Capture_Reg(CAPTURE_NEW_CTL_II, 0xC4/*0xC0*/); /*New FIFO policy: enable bit 2*/
    Out_Video_Reg(0xFA, 0x80);
    Out_SEQ_Reg_M(0xE7, 0x30, 0xCF); /*New FIFO policy*/
    Out_Video_Reg(0xFA, 0x00);
    Out_Video_Reg_M(0xF6, 0x08, 0xF7);/*New FIFO policy*/

    Out_Capture_Reg_M(CAPTURE_MODE_I, 0x01, 0xFE); /* Enable 8 bit video*/
    Out_SEQ_Reg_M(0xA3, 0xC0, 0x3F); /*always enable Y scale down for Capture 1*/
    Out_SEQ_Reg_M(0xA5, 0xC0, 0x3F); /*Enable Y & UV value clamping for Capture 1*/
//    Out_CRT_Reg_M(0x93, 0xC0, 0x3F); /*always enable Y scale down for Capture 2*/
//    Out_CRT_Reg_M(0x95, 0xC0, 0x3F); /*Enable Y & UV value clamping for Capture 2*/

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetCapSrcWindow() specifies which part of incoming external video
 will be captured to frame buffer, normally we do some clipping here in order 
 to get rid of garbage video.
 In  : left   - x1
       top    - y1
       right  - x2
       bottom - y2
 Out : None
 Note: Coordiation unit is pixel.
 ****************************************************************************/
void Tvia_SetCapSrcWindow(u16 left, u16 right, u16 top, u16 bottom)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    left  = left < 1 ?  0 : left * 2 - 1;
    right = right * 2 + 1;
    ++ bottom;

    Out_Capture_Reg(CAPTURE_H_START_L, (u8)(left & 0x00FF));
    Out_Capture_Reg(CAPTURE_H_START_H, (u8)(left >> 8));
    Out_Capture_Reg(CAPTURE_H_END_L, (u8)(right & 0x00FF));
    Out_Capture_Reg(CAPTURE_H_END_H, (u8)(right >> 8));
    Out_Capture_Reg(CAPTURE_V_START_L, (u8)(top & 0x00FF));
    Out_Capture_Reg(CAPTURE_V_START_H, (u8)(top >> 8));
    Out_Capture_Reg(CAPTURE_V_END_L, (u8)(bottom & 0x00FF));
    Out_Capture_Reg(CAPTURE_V_END_H, (u8)(bottom >> 8));

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetCapDstAddr() specifies Capture destination (frame buffer) 
 coordination.
 In  : dwMemAddr   - memory address where you are concerned about video
                     Capture operation.
       wX          - horizontal starting position, based on dwMemAddr.
       wY          - vertical starting position, based on dwMemAddr.
       wPitchWidth - pixel width between two continual lines. This width 
                     will help video Capture engine decide where is the 
                     beginning address of next line.
 Out : None
 Note: dwMemAddr, wX, wY combination will uniquely decide where is the 
       first position inside frame buffer video data will be capture to, 
       this combination is very flexible, the real starting point is:
       dwMemAddr + (wX + wY * wPitchWidth) * BytesPerPixel.
 ****************************************************************************/
void Tvia_SetCapDstAddr(u32 dwMemAddr, u16 wX, u16 wY, u16 wPitchWidth)
{
    u32 dwAddr;          
    u8 bTemp;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xF7, 0x00);
    dwAddr = dwMemAddr + ((u32)wX + (u32)wPitchWidth * (u32)wY) * 2L;
    dwCapDstAddr = dwMemAddr;
    dwAddr = dwAddr >> 2;

    /*Turn off capture engine first. */
    Out_Video_Reg(CAPTURE_BANK_CTL, 0x00);
    bTemp = In_Capture_Reg(VFAC_CTL_MODE_I);
    Out_Capture_Reg_M(VFAC_CTL_MODE_I, 0x04, 0xFB);
 
    Out_Capture_Reg(CAPTURE_ADDR_L, (u8)(dwAddr & 0x0000ffL));
    Out_Capture_Reg(CAPTURE_ADDR_M, (u8)((dwAddr & 0x00ff00L) >> 8));
    Out_Capture_Reg_M(CAPTURE_ADDR_H, (u8)(dwAddr >> 16), 0xE0);
    Out_Capture_Reg(CAPTURE_PITCH_L, (u8)(wPitchWidth >> 2));
    Out_Capture_Reg_M(CAPTURE_ADDR_H,  (u8)(wPitchWidth >> 4), 0x3F);
    Out_Capture_Reg_M(VFAC_CTL_MODE_I, bTemp, 0xFB);

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetCaptureScale() specifies the ratio of our video Capture Scaler.
 In  : wSrcXExt     - incoming video window width
       wDstXExt     - frame buffer data window width
       wSrcYExt     - incoming video window height
       wDstYExt     - frame buffer data window height
       bInterlaced  - INTERLACED: Capture engine will captured interlaced 
                                  video in
                      NONINTERLACED: Capture engine will captured single 
                                     filed video in
 Out : None 
 Note: Source here refers to incoming video, Destination refers to frame 
       buffer. Coordation unit is pixel.
 ****************************************************************************/
void Tvia_SetCaptureScale(u16 wSrcXExt, u16 wDstXExt, u16 wSrcYExt, u16 wDstYExt, u8 bInterlaced)
{
	u32 dwIncX, dwIniX, dwIncY, dwIniY, dwTmp;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    if(bInterlaced) {
        Out_Capture_Reg_M(VFAC_CTL_MODE_III, 0x02, 0xFD); /*Interlace bit=1*/
        Out_Capture_Reg_M(CAPTURE_CTL_MISC, 0x00, 0xFB); /*Data Good bit=0*/
        wDstYExt = wDstYExt / 2;
    }
    else {
        Out_Capture_Reg_M(VFAC_CTL_MODE_III, 0x00, 0xFD); /*Interlace bit=0*/
        Out_Capture_Reg_M(CAPTURE_CTL_MISC, 0x04, 0xFB); /*Data Good bit=1*/
    }

    /* X */
    dwIncX = (((u32)wDstXExt) * 0x1000) / wSrcXExt;
    Out_Capture_Reg(CAPTURE_DDA_X_INC_L, (u8)(dwIncX & 0x00FF));
    Out_Capture_Reg(CAPTURE_DDA_X_INC_H, (u8)(dwIncX >> 8));

    /* Y */
    dwIncY = (((u32)wDstYExt) * 0x1000) / wSrcYExt;
    Out_Capture_Reg(CAPTURE_DDA_Y_INC_L, (u8)(dwIncY & 0x00FF));
    Out_Capture_Reg(CAPTURE_DDA_Y_INC_H, (u8)(dwIncY >> 8));

    /* Reset DDA Initialization */
    dwIniX = (((u32)wDstXExt) * 0x1000) % wSrcXExt;
    dwIniX = TVIA_MAX(dwIniX, 0x1000 - dwIncX);
    
    dwIniY = (((u32)wDstYExt) * 0x1000) % wSrcYExt;
    dwIniY = TVIA_MAX(dwIniY, 0x1000 - dwIncY);

    /* For capture 2, we have to swap init_x<11> and init_y<11> */
    if(wCaptureEngineIndex == CAPENGINE2) {
        dwTmp = dwIniX;
        dwIniX = (dwIniX & ~0x800) | (dwIniY & 0x800);
        dwIniY = (dwIniY & ~0x800) | (dwTmp & 0x800);
    } 
    Out_Capture_Reg(CAPTURE_DDA_X_INIT_L, (u8)(dwIniX & 0x00FF));
    Out_Capture_Reg(CAPTURE_DDA_X_INIT_H, (u8)(dwIniX >> 8));
    Out_Capture_Reg(CAPTURE_DDA_Y_INIT_L, (u8)(dwIniY & 0x00FF));
    Out_Capture_Reg(CAPTURE_DDA_Y_INIT_H, (u8)(dwIniY >> 8));

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_CaptureOn() turns on Capture engine to capture video data to frame 
 buffer.
 In  : None                                                                                     
 Out : None
 ****************************************************************************/
void Tvia_CaptureOn(void)
{
	u8 b3CE_F7, b3CE_FA;

	  printk("Tvia_CaptureOn\n");
    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);

    Out_Video_Reg(MISC_CTL_I, 0x01); /*toggle bit 7, 6 of 3cf(de)*/
    Out_Video_Reg(CAPTURE_BANK_CTL, 0x00); 
    Out_Capture_Reg_M(VFAC_CTL_MODE_I, 0x61, 0x9E); /*Video capture enable*/
    sleep_VSync(2);
    Out_Video_Reg(CAPTURE_BANK_CTL, 0x00); 
    Out_Capture_Reg_M(VFAC_CTL_MODE_I, 0x00, 0xF3);

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);

	}

/****************************************************************************
 Tvia_SetCapSafeGuardAddr() is to set capture ending address in case some 
 video decoder output is unstable (for instance, some TV channels) and data 
 would be written beyond expected boundary, after setting the ending addr, 
 video data will be written between starting addr (set by Tvia_SetCapDstAddr()
 and this ending addr. 
 In  : dwOffAddr - ending address
 Out : None
 ****************************************************************************/
void Tvia_SetCapSafeGuardAddr(u32 dwOffAddr)
{
    u32 dwL;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    dwL = dwOffAddr >> 2;

    if(wCaptureEngineIndex == CAPENGINE1) {
        /* First enable SafeGuard 1 */
        Out_Video_Reg(0xFA, 0x0);
        Out_Video_Reg(0xF7, 0x0);
        Out_SEQ_Reg_M(0xA2, 0x04, 0xFB);

        Out_Video_Reg(0x45, (u8)(dwL & 0x0000ff));
        Out_Video_Reg(0x46, (u8)((dwL & 0x00ff00) >> 8));
        Out_Video_Reg(0x49, (ReadReg(GRAINDEX, 0x49) & 0xF0) | ((u8)(((dwL & 0x00ff0000) >> 16 ) & 0x0F)));
        Out_Video_Reg(0x4D, (ReadReg(GRAINDEX, 0x4D) & 0xFB) | ((u8)(((dwL & 0x00ff0000) >> 18 ) & 0x04)));
    }
    else {
        /* First enable SafeGuard 2 */
        Out_Video_Reg(0xFA, 0x0);
        Out_Video_Reg(0xF7, 0x0);
        Out_CRT_Reg_M(0x92, 0x04, 0xFB);
        
        Out_CRT_Reg(0x75, (u8)(dwL & 0x0000ff));
        Out_CRT_Reg(0x76, (u8)((dwL & 0x00ff00) >> 8));
        Out_CRT_Reg(0x79, (ReadReg(CRTINDEX, 0x79) & 0xF0) | ((u8)(((dwL & 0x00ff0000) >> 16) & 0x0F)));
        Out_CRT_Reg(0x7F, (ReadReg(CRTINDEX, 0x7F) & 0xFB) | ((u8)(((dwL & 0x00ff0000) >> 18) & 0x04)));
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_CaptureCleanUp() does some cleanup job just before you quit Capture 
 operation.
 In  : None
 Out : None
 Note: This function should be called just before the application exits.
 ****************************************************************************/
void Tvia_CaptureCleanUp(void)
{
    u16 i;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    /*Disable SafeGuard*/
    Out_SEQ_Reg_M(0xA2, 0x00, 0xFB); /*for Capture 1*/
//    Out_CRT_Reg_M(0x92, 0x00, 0xFB); /*for Capture 2*/

    /* Disable double buffer */
//    for(i = 0; i < MAX_CAP_ENG_NUM; i++) {
//        Tvia_SyncCapDoubleBuffer(i, OFF);
//        Tvia_EnableCapDoubleBuffer(i, OFF, INTERLACED);
//    }

    Out_Video_Reg_M(0xb5, 0x00, 0x7C);    /* Disable Vafc  !!!*/

    Out_Video_Reg_M(VFAC_CTL_MODE_I, 0x00, 0x92); /*Video capture 1 disabled*/
//    Out_CRT_Reg_M(VFAC_CTL_MODE_I, 0x00, 0x92); /*Video capture 2 disabled*/

    Out_CRT_Reg(0xf6, 0x00); /* Clear video port selector */

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_CaptureOff() turns off Capture engine.
 In  : None                                                                                     
 Out : None
 ****************************************************************************/
void Tvia_CaptureOff(void)
{
    u16 wCounter = 0;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    Out_Video_Reg(0xF7, 0x00);

    Out_Video_Reg(CAPTURE_BANK_CTL, 0x00); 
    Out_Capture_Reg_M(VFAC_CTL_MODE_I, 0x08, 0xF7);

    while(!(In_Capture_Reg(VIDEO_ROM_UCB4G_HI) & 0x02)) {
        udelay(1000);
        wCounter++;
        if(wCounter >= TIMEOUT) break;
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}
//#if 0
/****************************************************************************
 Tvia_EnableDoubleBuffer() enables/disables Tvia CyberPro53xx/52xx 
 double-buffer feature.
 In  : bEnable - 1: enable, 0: disable
 Out : None
 Note: This is an SDK50xx-style function that should be replaced with 
       Tvia_EnableGeneralDoubleBuffer(), Tvia_EnableCapDoubleBuffer(), and
       Tvia_EnableOverlayDoubleBuffer() in SDK53xx/SDK52xx.
 ****************************************************************************/
void Tvia_EnableDoubleBuffer(u8 bEnable)
{
    if(bEnable) {
        Tvia_EnableGeneralDoubleBuffer(ON);
        Tvia_EnableCapDoubleBuffer(CAPENGINE1, ON, NONINTERLACED);
        Tvia_EnableOverlayDoubleBuffer(OVERLAY1, ON, NONINTERLACED);
    }
    else{
        Tvia_EnableGeneralDoubleBuffer(OFF);
        Tvia_EnableCapDoubleBuffer(CAPENGINE1, OFF, NONINTERLACED);
        Tvia_EnableOverlayDoubleBuffer(OVERLAY1, OFF, NONINTERLACED);
    }
}

/****************************************************************************
 Tvia_SetBackBufferAddr() sets double-buffer second/back buffer address.
 In  : dwOffAddr   - back buffer address.
       wX          - horizontal starting position, based on dwOffAddr.
       wY          - vertical starting position, based on dwOffAddr.
       wPitchWidth - pixel width between two continual lines. This width 
                     will help video Capture engine decide where is the 
                     beginning address of next line.
 Out : None
 Note: This is an SDK50xx-style function that should be replaced with
       Tvia_SetCapBackBufferAddr(), Tvia_SyncCapDoubleBuffer(),
       Tvia_SetOverlayBackBuffer(), and Tvia_SyncOverlayDoubleBuffer() in
       SDK53xx/SDK52xx.
 ****************************************************************************/
void Tvia_SetBackBufferAddr(u32 dwOffAddr, u16 wX, u16 wY, u16 wPitchWidth)
{
    Tvia_SetCapDstAddr(dwCapDstAddr, wX, wY, wPitchWidth);

    Tvia_SetCapBackBufferAddr(CAPENGINE1, dwOffAddr, wX, wY, wPitchWidth);
    Tvia_SyncCapDoubleBuffer(CAPENGINE1,ON);
           
    Tvia_SetOverlayBackBufferAddr(OVERLAY1, dwOffAddr, wX, wY, wPitchWidth);
    Tvia_SyncOverlayDoubleBuffer(OVERLAY1,ON);
}
//#endif
/****************************************************************************
 Tvia_SelectCaptureEngineIndex() specifies which Capture engine will be used.
 In  : wIndex = 0, Capture engine 1
              = 1, Capture engine 2
 Out : None                      
 Note: This routine should be called before calling any other Capture 
       functions, and should be called only once except switching to another 
       Capture engine.
 ****************************************************************************/
void Tvia_SelectCaptureEngineIndex(u16 wIndex)
{               
    if(wIndex > MAX_CAP_ENG_NUM)
        wCaptureEngineIndex = CAPENGINE1;
    else 
        wCaptureEngineIndex = wIndex;
}

/****************************************************************************
 Tvia_SelectCapturePath() specifies a path that which Capture engine will 
 capture data from which video port.
 In  : wWhichPort = 0, Video Port A 
                  = 1, Video Port B 
                  = 2, Video Port C
                  = 3, TV digital output (TVVIDEO)
       wIndex = 0, Capture engine 1 
              = 1, Capture engine 2 
 Out : None                      
 Note: This routine should be called after calling 
       Tvia_SelectCaptureEngineIndex(), and should be called only once except 
       switching to another Capture engine or vodeo port.
 ****************************************************************************/
void Tvia_SetCapturePath(u8 bWhichPort, u8 bWhichCapEngine)
{
#if 0
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    /*
    {
      u32 tmp;
      for(tmp=0;tmp<100;tmp++);
    }
    */
    
    switch (bWhichPort) {
        case PORTA:
	    printk("Tvia_SetCapturePath:PORTA\n");
            if(bWhichCapEngine == CAPENGINE1) {
	      printk("Tvia_SetCapturePath:CAPENGINE1 in\n");
	      /*
	      {
		u8 tmp;
		u32 tmp1;
		tmp=In_Video_Reg(0xF6);
		printk("Tvia_SetCapturePath:3cf/f6=%x\n",tmp); // !!!raf  debug

	tmp=In_CRT_Reg(0xF6) ;
		printk("Tvia_SetCapturePath:3d5/f6=%x\n",tmp); // !!!raf 
		for(tmp1=0;tmp1<100;tmp1++);
	      }
	      */

	      /*	      Out_CRT_Reg_M(0xF6, */ /* 0x00 !!!raf */  /*0x01, 0xAA); */ /* Says to use following register to select */

	      {
		u8 tmp;
		/* tmp = ReadReg(0x3d4, 0xF6); */
		printk("Tvia_SetCapturePath_read:3d4=0xf6\n");
		*((volatile unsigned char *)(CyberRegs + 0x3d4)) = 0xF6;
		printk("Tvia_SetCapturePath_read:3d4=0xf6\n");
		tmp=*((volatile unsigned char *)(CyberRegs + 0x3d4 + 1));
		printk("Tvia_SetCapturePath_read:3d5/f6=%x\n",tmp);
		tmp |= (0x01 & ~0xAA);
		printk("Tvia_SetCapturePath:3d5/f6 new val=%x\n",tmp);
		WriteReg(0x3d4,0xF6,tmp);
		printk("Tvia_SetCapturePath:3d5/f6 new val=%x written\n",tmp);
	      }

	      /*
    printk("Out_CRT_Reg_M 0\n");
    *((volatile unsigned char *)(CyberRegs + 0x3d4)) = bIndex;
    printk("Out_CRT_Reg_M 1\n");
    bTemp = (*((volatile unsigned char *)(CyberRegs + 0x3d5))) & bMask;
     printk("Out_CRT_Reg_M 4\n");
   bTemp |= (bValue & ~bMask);
    printk("Out_CRT_Reg_M 4\n");
    *((volatile unsigned char *)(CyberRegs + 0x3d5)) = bTemp;
    printk("Out_CRT_Reg_M 4\n");

}
void WriteReg(u16 wReg, u8 bIndex, u8 bData)
{
    *((volatile unsigned char *)(CyberRegs + wReg)) = bIndex;
    *((volatile unsigned char *)(CyberRegs + wReg + 1)) = bData;
}

u8 ReadReg(u16 wReg, u8 bIndex)
{
    *((volatile unsigned char *)(CyberRegs + wReg)) = bIndex;
    return *((volatile unsigned char *)(CyberRegs + wReg + 1));
}

              */

 	      printk("Tvia_SetCapturePath:CAPENGINE1 1\n");

//	      Out_CRT_Reg_M(0xE1, 0x01, 0xFC); /* orig 0x00 */ /*Port A uses Pin 43*/
	      Out_CRT_Reg_M(0xE1, 0x01, 0x7C); /* orig 0x00 */ /*Port A uses Pin 43 and capture port A */



 	      printk("Tvia_SetCapturePath:CAPENGINE1 2\n");

               Out_SEQ_Reg_M(0xEF, 0x00, 0xFD);

	      printk("Tvia_SetCapturePath:CAPENGINE1 3\n");
            }
            else { /*Capture engine 2*/
 	      printk("Tvia_SetCapturePath:CAPENGINE2\n");
               Out_CRT_Reg_M(0xF6, 0x00/*0x02*/, 0x55);
                Out_CRT_Reg_M(0xE1, 0x00, 0xFC); /*Port A uses Pin 43*/
                Out_CRT_Reg(0x7A, ReadReg(CRTINDEX, 0x7A) & 0xDF);
            }
            break;
        case PORTB:
  	  printk("Tvia_SetCapturePath:PORTB\n");
          if(bWhichCapEngine == CAPENGINE1) {
   	      printk("Tvia_SetCapturePath:CAPENGINE1\n");
             Out_CRT_Reg_M(0xF6, 0x04/*0x05*/, 0xAA);
                Out_CRT_Reg_M(0xE1, 0x00, 0xF3); /*Port B uses Pin 44*/
                Out_SEQ_Reg_M(0xEF, 0x02, 0xFD);
            }
            else { /*Capture engine 2*/
    	      printk("Tvia_SetCapturePath:CAPENGINE2\n");
               Out_CRT_Reg_M(0xF6, 0x08/*0x0A*/, 0x55);
                Out_CRT_Reg_M(0xE1, 0x00, 0xF3); /*Port B uses Pin 44*/
                Out_CRT_Reg(0x7A, (ReadReg(CRTINDEX, 0x7A) & 0xDF) | 0x20);
            }
            break;
        case PORTC:
            if(bWhichCapEngine == CAPENGINE1) {
                Out_CRT_Reg_M(0xF6, 0x11, 0xAA);
            }
            else { /*Capture engine 2*/
                Out_CRT_Reg_M(0xF6, 0x22, 0x55);
            }
            break;
        case TVVIDEO:
            if(bWhichCapEngine == CAPENGINE1) {
                Out_CRT_Reg_M(0xF6, 0x41, 0xAA);
            }
            else { /*Capture engine 2*/
                Out_CRT_Reg_M(0xF6, 0x82, 0x55);
            }
            break;
        default:
            return;
    }

	wCapturePortIndex[bWhichCapEngine] = bWhichPort;

    /* Restore banking */
    printk("Tvia_SetCapturePath:CAPENGINE out\n");
    Out_Video_Reg(0xF7, b3CE_F7);
    printk("Tvia_SetCapturePath:CAPENGINE out1\n");
    Out_Video_Reg(0xFA, b3CE_FA);
    printk("Tvia_SetCapturePath:CAPENGINE out2\n");

#else
	wCapturePortIndex[bWhichCapEngine] = bWhichPort;
#endif
}
//#if 0
/****************************************************************************
 Tvia_EnableGeneralDoubleBuffer() enables/disables Tvia CyberPro53xx/52xx 
 double-buffer feature.
 In  : bEnable - 1: enable, 0: disable
 Out : None
 ****************************************************************************/
void Tvia_EnableGeneralDoubleBuffer(u8 bEnable)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    Out_Video_Reg(0xF7, 0x00);

    if(bEnable) {
        Out_Video_Reg(0xFA, 0x00);
        Out_Video_Reg_M(0xAC, 0x02, 0xFD);
        Out_Video_Reg_M(0xA9, 0x10, 0xEF);
    }
    else{
        Out_Video_Reg(0xFA, 0x00);
        Out_Video_Reg_M(0xAC, 0x00, 0xFD);
        Out_Video_Reg_M(0xA9, 0x00, 0xEF);
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_DoubleBufferSelectRivals() selects rivals between Capture engine and 
 Overlay engine.
 In  : wWhichCaptureEngine = 0, Capture engine 1 
                           = 1, Capture engine 2 
       wWhichOverlay       = 0, Overlay engine 1 
                           = 1, Overlay engine 2 
 Out : None
 ****************************************************************************/
void Tvia_DoubleBufferSelectRivals(u16 wWhichCaptureEngine, u16 wWhichOverlay)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    if(wWhichCaptureEngine == CAPENGINE1) {
        if(wWhichOverlay == OVERLAY1) { /*Cap1 vs. Overlay1*/
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x80);

            Out_Video_Reg_M(0xE7, 0x00, 0xEF);
            Out_Video_Reg_M(0xE9, 0x00, 0xFE);

            Out_Video_Reg(0xFA, 0x00);

            Out_CRT_Reg_M(0xAC, 0x00, 0xFD);
        }
        else if(wWhichOverlay == OVERLAY2) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x80);

            Out_Video_Reg_M(0xE7, 0x10, 0xEF);
            Out_Video_Reg_M(0xE9, 0x00, 0xEF);
        }
    }
    else if(wWhichCaptureEngine == CAPENGINE2) {
        if(wWhichOverlay == OVERLAY1) {/*Cap2 vs. Overlay1*/
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);

            Out_CRT_Reg_M(0xF4, 0x00, 0xFE);

            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x80);

            Out_Video_Reg_M(0xE9, 0x00, 0xFE);

            Out_Video_Reg(0xFA, 0x00);

            Out_CRT_Reg_M(0xAC, 0x02, 0xFD);
        }
        else if(wWhichOverlay == OVERLAY2) {/*Cap2 vs. Overlay2*/
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);

            Out_CRT_Reg_M(0xF4, 0x01, 0xFE);

            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x80);

            Out_Video_Reg_M(0xE9, 0x10, 0xEF);      
        }
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_EnableCapDoubleBuffer() enables/disables Tvia CyberPro53xx/52xx Capture
 double-buffer feature.
 In  : bEnable      - 1: enable, 0: disable
       bInterleaved - 1: interleaved, 0: non-interleaved
 Out : None
 ****************************************************************************/
void Tvia_EnableCapDoubleBuffer(u16 wWhichCaptureEngine, u8 bEnable, u8 bInterleaved)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    if(bEnable) {
        if(wWhichCaptureEngine == CAPENGINE1) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);

            Out_Video_Reg_M(0xA9, 0x10, 0xEF);
            
            Out_Video_Reg(0xFA, 0x80);
            
            Out_Video_Reg_M(0xE7, 0x20, 0xDF);

            if(bInterleaved) {
                Out_Video_Reg_M(0xE0, 0x00, 0xDF);
                
                Out_Video_Reg(0xFA, 0x00);
                
                Out_Video_Reg_M(0xEA, 0x02, 0xFD);
            }
            else {
                Out_Video_Reg_M(0xE0, 0x20, 0xDF);
                
                Out_Video_Reg(0xFA, 0x00);
                
                Out_Video_Reg_M(0xEA, 0x00, 0xFD);
            }

            /* Invert polarity of odd/Even signal used for VPE double buffer control */
            Out_Video_Reg(0xFA, 0x80);
            
            Out_Video_Reg_M(0xE0, 0x08, 0xF7);
        }
        else if(wWhichCaptureEngine == CAPENGINE2) {/*Capture engine 2*/
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);
            
            Out_CRT_Reg_M(0xF4, 0x02, 0xFD);
            Out_CRT_Reg_M(0xA9, 0x10, 0xEF);
            
            /* By default, capture 2 selects rival overlay 2 */
            Out_CRT_Reg_M(0xF4, 0x01, 0xFE);
            
            if(bInterleaved) {
                Out_CRT_Reg_M(0xF4, 0x00, 0xDF);
                Out_CRT_Reg_M(0xEA, 0x02, 0xFD);
            }
            else {
                Out_CRT_Reg_M(0xF4, 0x20, 0xDF);
                Out_CRT_Reg_M(0xEA, 0x00, 0xFD);
            }

            /* Invert polarity of odd/Even signal used for VPE double buffer control */
            Out_CRT_Reg_M(0xF4, 0x40, 0xBF);
        }
    }
    else{
        if(wWhichCaptureEngine == CAPENGINE1) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);
            
            Out_Video_Reg_M(0xA9, 0x00, 0xEF);
            
            Out_Video_Reg(0xFA, 0x80);
            
            Out_Video_Reg_M(0xE7, 0x00, 0xDF);
        }
        else if(wWhichCaptureEngine == CAPENGINE2) {/*Capture engine 2*/
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);

            Out_CRT_Reg_M(0xF4, 0x00, 0xFD);
            Out_CRT_Reg_M(0xA9, 0x00, 0xEF);
        }
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}                                                 

/****************************************************************************
 Tvia_SyncCapDoubleBuffer() enables/disables Tvia CyberPro53xx/52xx Capture
 double-buffer synchnorous mode.
 In  : wWhichCaptureEngine = 0, Capture engine 1 
                           = 1, Capture engine 2 
       bEnable             - 1: enable, 0: disable
 Out : None
 ****************************************************************************/
void Tvia_SyncCapDoubleBuffer(u16 wWhichCaptureEngine, u8 bEnable)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    if(bEnable) {           
        if(wWhichCaptureEngine == CAPENGINE1) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x80);

            Out_Video_Reg_M(0xE7, 0x40, 0xBF);
        }
        else if(wWhichCaptureEngine == CAPENGINE2) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);

            Out_CRT_Reg_M(0xF4, 0x04, 0xFB);
        }
    }
    else {
        if(wWhichCaptureEngine == CAPENGINE1) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x80);

            Out_Video_Reg_M(0xE7, 0x00, 0xBF);
        }
        else if(wWhichCaptureEngine == CAPENGINE2) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);

            Out_CRT_Reg_M(0xF4, 0x00, 0xFB);
        }
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetCapBackBufferAddr() sets Capture double-buffer second/back buffer 
 address.
 In  : dwOffAddr   - back buffer address
       wX          - horizontal starting position, based on dwOffAddr.
       wY          - vertical starting position, based on dwOffAddr.
       wPitchWidth - pixel width between two continual lines. This width 
                     will help video Capture engine decide where is the 
                     beginning address of next line.
 Out : None                                            
 ****************************************************************************/
void Tvia_SetCapBackBufferAddr(u16 wWhichCaptureEngine, u32 dwOffAddr, u16 wX,
    u16 wY, u16 wPitchWidth)
{                          
    u32 dwL;
    u8  b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    dwL = dwOffAddr + ((u32)wX + (u32)wPitchWidth * (u32)wY) * 2L;
    dwL = dwL >> 2;                      

    Out_Video_Reg(0xFA, 0x00);
    Out_Video_Reg(0xF7, 0x00);

    if(wWhichCaptureEngine == CAPENGINE1)  {
        Out_Video_Reg(0xA7, (u8)(dwL & 0x0000ff));
        Out_Video_Reg(0xA8, (u8)((dwL & 0x00ff00) >> 8));
        Out_Video_Reg_M(0xA9, (u8)((dwL & 0x0f0000) >> 16), 0xF0);
        Out_Video_Reg_M(0x4d, (u8)((dwL & 0x100000) >> 16), 0x7F);
    }   
    else {
        Out_CRT_Reg(0xA7, (u8)(dwL & 0x0000ff));
        Out_CRT_Reg(0xA8, (u8)((dwL & 0x00ff00) >> 8));
        Out_CRT_Reg(0xA9,
            (ReadReg(CRTINDEX, 0xA9) & 0xF0) | (u8)((dwL & 0x0f0000) >> 16));
        Out_CRT_Reg(0x7f,
            (ReadReg(CRTINDEX, 0x7F) & 0x7F) | (u8)((dwL & 0x100000) >> 16));
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}
//#endif
/****************************************************************************
 Tvia_InvertFieldPolarity() inverts incoming video odd/even field polarity.
 In  : bInvert - 1: Invert field polarity, 0: Do not invert polarity
 Out : None
 Note: This function can take effect after calling Tvia_InitCapture()
 ****************************************************************************/
void Tvia_InvertFieldPolarity(u8 bInvert)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    if(bInvert)
        Out_Capture_Reg_M(VFAC_CTL_MODE_II, 0x20, 0xDF);
    else
        Out_Capture_Reg_M(VFAC_CTL_MODE_II, 0x00, 0xDF);

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

#if 0
/****************************************************************************
 This routine is used internally. 
 ScalarRatioAdjustment() adjusts the ratio of our video Capture Scalar
 in:    wSrcWidth  - incoming video window width
        wDstWidth  - frame buffer data window width
        wSrcHeight - incoming video window height
        wDstHeight - frame buffer data window height
 out:   none, but set bScalarBaseW and bScalarBaseH that match the best
        Capture Scalar Ratio.
 Note:  Source here refers to incoming video, Destination refers to frame 
        buffer. Coordination unit is pixel.
 ****************************************************************************/
static void ScalarRatioAdjustment(u16 wSrcWidth, u16 wDstWidth, u16 wSrcHeight,
    u16 wDstHeight)
{
    u16 wMinComDivisor = 2;
    u16 wMaxComDivisor = wDstWidth;
    
    if(wSrcWidth > 255) {
        do {
            if(((wSrcWidth % wMinComDivisor) == 0) && ((wSrcWidth % wMinComDivisor) == 0)) {
                wSrcWidth = wSrcWidth / wMinComDivisor;
                wDstWidth = wDstWidth / wMinComDivisor;
                if (wSrcWidth <= 255) break;
                else {
                    wMinComDivisor = 2;
                    wMaxComDivisor = wDstWidth;
                }
            }
            else wMinComDivisor++;
        } while(wMinComDivisor <= wMaxComDivisor);
    }

    if(wSrcWidth > 255) bScalarBaseW = 255;
    else bScalarBaseW = (u8)wSrcWidth;

    wMinComDivisor = 2;
    wMaxComDivisor = wDstHeight;

    if(wSrcHeight > 255) {
        do {
            if(((wSrcHeight % wMinComDivisor) == 0) && ((wSrcHeight % wMinComDivisor) == 0)) {
                wSrcHeight = wSrcHeight / wMinComDivisor;
                wDstHeight = wDstHeight / wMinComDivisor;
                if(wSrcHeight <= 255) break;
                else {
                    wMinComDivisor = 2;
                    wMaxComDivisor = wDstHeight;
                }
            }
            else wMinComDivisor++;
        } while(wMinComDivisor <= wMaxComDivisor);
    }

    if(wSrcHeight > 255) bScalarBaseH = 255;
    else bScalarBaseH = (u8)wSrcHeight;
}

/****************************************************************************
 Tvia_EnableScalar() turns on/off Capture Scalar engine.
 in:  onoff = 1, on
              0, off
 out: none
 ****************************************************************************/
void Tvia_EnableScalar(u8 onoff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    if(onoff) {
        if(wCapturePortIndex[wCaptureEngineIndex] == PORTA) /*Port A*/
            Out_CRT_Reg(0xFA, 0x41); /*Enable scalar*/
        else /*Port B*/
            Out_CRT_Reg(0xFA, 0x82); /*Enable scalar*/
    }
    else {
        if(wCapturePortIndex[wCaptureEngineIndex] == PORTA) /*Port A*/
            Out_CRT_Reg_M(0xFA, 0x00, 0xBE); /*Disable scalar*/
        else /*Port B*/
            Out_CRT_Reg_M(0xFA, 0x00, 0x7D); /*Disable scalar*/
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetCapScalarRatio() specifies the ratio of our video Capture Scalar
 in:    wSrcWidth  - incoming video window width
        wDstWidth  - frame buffer data window width
        wSrcHeight - incoming video window height
        wDstHeight - frame buffer data window height
 Note:  Source here refers to incoming video, Destination refers to frame 
        buffer. Coordination unit is pixel.
 ****************************************************************************/
void Tvia_SetCapScalarRatio(u16 wSrcWidth, u16 wDstWidth, u16 wSrcHeight, 
    u16 wDstHeight, u8 bInterlaced)
{
    u8 bNewDstWidth, bNewDstHeight;
    u8 bfa, bf7;
    u32 HRatio, VRatio;
    u8 new_left, new_right;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xFA, 0x00);
    Out_Video_Reg(0xF7, 0x00);

    if(bInterlaced != 0) {
        Out_Capture_Reg_M(VFAC_CTL_MODE_III, 0x02, 0xFD); /*Interlace bit=1*/
        Out_Capture_Reg_M(CAPTURE_CTL_MISC, 0x00, 0xFB); /*Data Good bit=0*/
        wDstHeight = wDstHeight / 2;
    }
    else {
        Out_Capture_Reg_M(VFAC_CTL_MODE_III, 0x00, 0xFD); /*Interlace bit=0*/
        Out_Capture_Reg_M(CAPTURE_CTL_MISC, 0x04, 0xFB); /*Data Good bit=1*/
    }                             

    if (wDstWidth % 4) wDstWidth = (wDstWidth / 4) * 4; /*64-bit alignment*/

    ScalarRatioAdjustment(wSrcWidth, wDstWidth, wSrcHeight, wDstHeight);

    bNewDstWidth = (u8)(((u32)wDstWidth*(u32)bScalarBaseW)/wSrcWidth);
    bNewDstHeight = (u8)(((u32)wDstHeight*(u32)bScalarBaseH)/wSrcHeight);

    /*Set Horizontal Coefficient*/
    HRatio = ((u32)bNewDstWidth*(u32)64)/bScalarBaseW; 
    if (HRatio < 8) //(float)1/(float)8)
    {
        Out_CRT_Reg(0xb0, bH_Coeff[0][0]);
        Out_CRT_Reg(0xb1, bH_Coeff[0][1]);
        Out_CRT_Reg(0xb2, bH_Coeff[0][2]);
        Out_CRT_Reg(0xb3, bH_Coeff[0][3]);
        Out_CRT_Reg(0xb4, bH_Coeff[0][4]);
        Out_CRT_Reg(0xb5, bH_Coeff[0][5]);
        Out_CRT_Reg(0xbe, bH_Coeff[0][6]);
    }
    else if (HRatio < 16) //(float)1/(float)4)
    {
        Out_CRT_Reg(0xb0, bH_Coeff[1][0]);
        Out_CRT_Reg(0xb1, bH_Coeff[1][1]);
        Out_CRT_Reg(0xb2, bH_Coeff[1][2]);
        Out_CRT_Reg(0xb3, bH_Coeff[1][3]);
        Out_CRT_Reg(0xb4, bH_Coeff[1][4]);
        Out_CRT_Reg(0xb5, bH_Coeff[1][5]);
        Out_CRT_Reg(0xbe, bH_Coeff[1][6]);
    }
    else if (HRatio < 24) //(float)3/(float)8)
    {
        Out_CRT_Reg(0xb0, bH_Coeff[2][0]);
        Out_CRT_Reg(0xb1, bH_Coeff[2][1]);
        Out_CRT_Reg(0xb2, bH_Coeff[2][2]);
        Out_CRT_Reg(0xb3, bH_Coeff[2][3]);
        Out_CRT_Reg(0xb4, bH_Coeff[2][4]);
        Out_CRT_Reg(0xb5, bH_Coeff[2][5]);
        Out_CRT_Reg(0xbe, bH_Coeff[2][6]);
    }
    else if (HRatio < 32) //(float)1/(float)2)
    {
        Out_CRT_Reg(0xb0, bH_Coeff[3][0]);
        Out_CRT_Reg(0xb1, bH_Coeff[3][1]);
        Out_CRT_Reg(0xb2, bH_Coeff[3][2]);
        Out_CRT_Reg(0xb3, bH_Coeff[3][3]);
        Out_CRT_Reg(0xb4, bH_Coeff[3][4]);
        Out_CRT_Reg(0xb5, bH_Coeff[3][5]);
        Out_CRT_Reg(0xbe, bH_Coeff[3][6]);
    }
    else if (HRatio < 40) //(float)5/(float)8)
    {
        Out_CRT_Reg(0xb0, bH_Coeff[4][0]);
        Out_CRT_Reg(0xb1, bH_Coeff[4][1]);
        Out_CRT_Reg(0xb2, bH_Coeff[4][2]);
        Out_CRT_Reg(0xb3, bH_Coeff[4][3]);
        Out_CRT_Reg(0xb4, bH_Coeff[4][4]);
        Out_CRT_Reg(0xb5, bH_Coeff[4][5]);
        Out_CRT_Reg(0xbe, bH_Coeff[4][6]);
    }                     
    else if (HRatio < 48) //(float)3/(float)4)
    {
        Out_CRT_Reg(0xb0, bH_Coeff[5][0]);
        Out_CRT_Reg(0xb1, bH_Coeff[5][1]);
        Out_CRT_Reg(0xb2, bH_Coeff[5][2]);
        Out_CRT_Reg(0xb3, bH_Coeff[5][3]);
        Out_CRT_Reg(0xb4, bH_Coeff[5][4]);
        Out_CRT_Reg(0xb5, bH_Coeff[5][5]);
        Out_CRT_Reg(0xbe, bH_Coeff[5][6]);
    }                    
    else if (HRatio < 56) //(float)7/(float)8)
    {
        Out_CRT_Reg(0xb0, bH_Coeff[6][0]);
        Out_CRT_Reg(0xb1, bH_Coeff[6][1]);
        Out_CRT_Reg(0xb2, bH_Coeff[6][2]);
        Out_CRT_Reg(0xb3, bH_Coeff[6][3]);
        Out_CRT_Reg(0xb4, bH_Coeff[6][4]);
        Out_CRT_Reg(0xb5, bH_Coeff[6][5]);
        Out_CRT_Reg(0xbe, bH_Coeff[6][6]);
    }
    else if (HRatio < 64) //(float)1)
    {
        Out_CRT_Reg(0xb0, bH_Coeff[7][0]);
        Out_CRT_Reg(0xb1, bH_Coeff[7][1]);
        Out_CRT_Reg(0xb2, bH_Coeff[7][2]);
        Out_CRT_Reg(0xb3, bH_Coeff[7][3]);
        Out_CRT_Reg(0xb4, bH_Coeff[7][4]);
        Out_CRT_Reg(0xb5, bH_Coeff[7][5]);
        Out_CRT_Reg(0xbe, bH_Coeff[7][6]);
    }

    /*Set Vertical Coefficient*/
    VRatio = ((u32)bNewDstHeight*(u32)64)/bScalarBaseH;
    if (VRatio < 8) //(float)1/(float)8)// || (VRatio == (float)1))
    {
        Out_CRT_Reg(0xc0, bV_Coeff[0][0]);
        Out_CRT_Reg(0xc1, bV_Coeff[0][1]);
        Out_CRT_Reg(0xc2, bV_Coeff[0][2]);
        Out_CRT_Reg(0xc3, bV_Coeff[0][3]);
        Out_CRT_Reg(0xce, bV_Coeff[0][4]);
    }
    else if (VRatio < 16) //(float)1/(float)4)
    {
        Out_CRT_Reg(0xc0, bV_Coeff[1][0]);
        Out_CRT_Reg(0xc1, bV_Coeff[1][1]);
        Out_CRT_Reg(0xc2, bV_Coeff[1][2]);
        Out_CRT_Reg(0xc3, bV_Coeff[1][3]);
        Out_CRT_Reg(0xce, bV_Coeff[1][4]);
    }	 
    else if (VRatio < 24) //(float)3/(float)8)
    {
        Out_CRT_Reg(0xc0, bV_Coeff[2][0]);
        Out_CRT_Reg(0xc1, bV_Coeff[2][1]);
        Out_CRT_Reg(0xc2, bV_Coeff[2][2]);
        Out_CRT_Reg(0xc3, bV_Coeff[2][3]);
        Out_CRT_Reg(0xce, bV_Coeff[2][4]);
    }
    else if (VRatio < 32) //(float)1/(float)2)
    {
        Out_CRT_Reg(0xc0, bV_Coeff[3][0]);
        Out_CRT_Reg(0xc1, bV_Coeff[3][1]);
        Out_CRT_Reg(0xc2, bV_Coeff[3][2]);
        Out_CRT_Reg(0xc3, bV_Coeff[3][3]);
        Out_CRT_Reg(0xce, bV_Coeff[3][4]);
    }
    else if (VRatio < 40) //(float)5/(float)8)
    {
        Out_CRT_Reg(0xc0, bV_Coeff[4][0]);
        Out_CRT_Reg(0xc1, bV_Coeff[4][1]);
        Out_CRT_Reg(0xc2, bV_Coeff[4][2]);
        Out_CRT_Reg(0xc3, bV_Coeff[4][3]);
        Out_CRT_Reg(0xce, bV_Coeff[4][4]);
    }
    else if (VRatio < 48) //(float)3/(float)4)
    {
        Out_CRT_Reg(0xc0, bV_Coeff[5][0]);
        Out_CRT_Reg(0xc1, bV_Coeff[5][1]);
        Out_CRT_Reg(0xc2, bV_Coeff[5][2]);
        Out_CRT_Reg(0xc3, bV_Coeff[5][3]);
        Out_CRT_Reg(0xce, bV_Coeff[5][4]);
    }
    else if (VRatio < 56) //(float)7/(float)8)
    {
        Out_CRT_Reg(0xc0, bV_Coeff[6][0]);
        Out_CRT_Reg(0xc1, bV_Coeff[6][1]);
        Out_CRT_Reg(0xc2, bV_Coeff[6][2]);
        Out_CRT_Reg(0xc3, bV_Coeff[6][3]);
        Out_CRT_Reg(0xce, bV_Coeff[6][4]);
    }
    else if (VRatio < 64) //(float)1)
    {
        Out_CRT_Reg(0xc0, bV_Coeff[7][0]);
        Out_CRT_Reg(0xc1, bV_Coeff[7][1]);
        Out_CRT_Reg(0xc2, bV_Coeff[7][2]);
        Out_CRT_Reg(0xc3, bV_Coeff[7][3]);
        Out_CRT_Reg(0xce, bV_Coeff[7][4]);
    }

    /*Set Horizontal Control*/
    Out_Video_Reg(0xf7, 0x48);
    if (HRatio == 64) //(float)1)
        Out_CRT_Reg(0xb3, 0xb4);
    else
    	Out_CRT_Reg(0xb3, 0xb8);
    Out_CRT_Reg(0xb4, bScalarBaseW);
    Out_CRT_Reg(0xb5, bNewDstWidth);
    Out_CRT_Reg(0xb6, 0x00);
    Out_CRT_Reg(0xb7, 0x0c);

    /*Set Vertical Control*/
    Out_Video_Reg(0xf7, 0x20);
    if (VRatio == 64) //(float)1)
        Out_CRT_Reg(0xca, 0x74);
    else
    	Out_CRT_Reg(0xca, 0x78);
    Out_CRT_Reg(0xcb, bScalarBaseH);
    Out_CRT_Reg(0xcc, bNewDstHeight);
    Out_CRT_Reg(0xcd, 0x0c);
    Out_CRT_Reg(0xce, 0x00);
    Out_CRT_Reg(0xcf, 0x2c);
    
    Out_Video_Reg(0xf7, 0x28);
    Out_CRT_Reg(0xc0, 0x00);
    Out_CRT_Reg(0xc1, 0x04);

#if 0
    /*correct video color*/
    new_left = In_Video_Reg(0x61);
    new_left = (new_left << 8) + In_Video_Reg(0x60);
    new_right = In_Video_Reg(0x63);
    new_right = (new_right << 8) + In_Video_Reg(0x62);
    
    new_left += 0x0a;
    new_right += 0x09;
    
    Out_Capture_Reg(CAPTURE_H_START_L, (u8)(new_left & 0x00FF) );
    Out_Capture_Reg(CAPTURE_H_START_H, (u8)(new_left >> 8));
    Out_Capture_Reg(CAPTURE_H_END_L, (u8)(new_right & 0x00FF));
    Out_Capture_Reg(CAPTURE_H_END_H, (u8)(new_right >> 8));
#endif
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}
#endif
