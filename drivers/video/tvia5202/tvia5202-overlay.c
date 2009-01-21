/*
 *  TVIA CyberPro 5202 overlay interface
 *
 */

#include "tvia5202-def.h"
#include "tvia5202-overlay.h"
#include "tvia5202-misc.h"
#include "tvia5202-dvideo.h"

#define  NTSCTVOUT 1 /*by default, we assume you TV output mode is NTSC,
                       this selection only affects Overlay playback on TV set,
                       it doesn't matter if you are using monitor only*/

#undef RGBDAC_DISABLE
#undef RGB_FROM_CVBS_PATH

/* External variables for window adjustment when Direct Video is on */
extern u16 g_wDvAdjOvlY[2][4];
extern int g_iGenDvOn;
extern int g_iDvOn[MAX_OVL_ENG_NUM];
extern int g_iTvMode;   /* PAL/NTSC */
extern int g_iModeNo;   /* Mode # */

/* Overlay */
/* Which overlay engine you are using */
static u16 g_wSelOvlEngIndex = OVERLAY1; 
/* Pixle byte width of video data */
static u16 g_wBytePerPixel[MAX_OVL_ENG_NUM]; 

/* If old FIFO values have been recorded, then set it to 1 */
static u8 g_bInitialized = 0; 
/* FIFO control registers for 2D Graphics */
static u8 g_b3CE_74, g_b3CE_75; 
/* FIFO control registers for Overlay */
static u8 g_bOvlRegD9[2], g_bOvlRegDA[2], g_bOvlRegDD[2]; 

/*Following is our FIFO policy number, should be programmed to
0x3CE/0x74, 0x3CE/0x75, 0x3CE( 0x3C4 )/0xD9, 0x3CE( 0x3C4 )/0xDA,
0x3CE( 0x3c4 )/0xDD respectively in order to get a best memory bandwidth.
Current value is a group of experence value based on 70MHZ EDO/SG RAM.*/

/*For BOARD5300*/
static const u8 bFIFOPolicyNum[2][5] =
{
    {
        0x1F, 0x10, 0x1C, 0x1C, 0x06
    },
    {
        0x1F, 0x10, 0x2F, 0x2F, 0x06
    }
};

static u8 In_Overlay_Reg(u8 bIndex)
{
    u8 val;

    if(g_wSelOvlEngIndex == OVERLAY1) {
        val = In_Video_Reg(bIndex);
    }
    else if(g_wSelOvlEngIndex == OVERLAY2) {
        val = In_SEQ_Reg(bIndex);
    }

    return val;
}

/****************************************************************************
 Out_Overlay_Reg() sets Overlay related register to a special value.
 In  : bIndex - Overlay register index
       bValue - value to be set
 Out : None
 Note: wOverlayIndex must be set before calling this routine.
 ****************************************************************************/
static void Out_Overlay_Reg(u8 bIndex, u8 bValue)
{
    if(g_wSelOvlEngIndex == OVERLAY1) {
        Out_Video_Reg(bIndex, bValue);
    }
    else if(g_wSelOvlEngIndex == OVERLAY2) {
        Out_SEQ_Reg(bIndex, bValue);
    }
}

/****************************************************************************
 Out_Overlay_Reg_M() sets Overlay related register with bit mask.
 In  : bIndex - Overlay register index
       bValue - bit/bits to be set/clear, untouched bit/bits must be zero
       bMask  - bit/bits to be masked, 1:mask, 0:to be set/clear
       e.g.  set <4,3> = <1,0>, bValue = 0x10 and bMask = E7
 Out : None
 Note: wOverlayIndex must be set before calling this routine.
 ****************************************************************************/
static void Out_Overlay_Reg_M(u8 bIndex, u8 bValue, u8 bMask)
{
    if(g_wSelOvlEngIndex == OVERLAY1) {
        Out_Video_Reg_M(bIndex, bValue, bMask);
    }
    else if(g_wSelOvlEngIndex == OVERLAY2) {
        Out_SEQ_Reg_M(bIndex, bValue, bMask);
    }
}

/****************************************************************************
 Tvia_SelectOverlayIndex() specifies which Overlay engine will be used.
 In  : wIndex = 0, Overlay engine 1
              = 1, Overlay engine 2
 Out : None
 Note: This routine should be called before calling any other Overlay 
       functions, and should be called only once except switching to 
       another Overlay engine.
 ****************************************************************************/
void Tvia_SelectOverlayIndex(u16 wIndex)
{
    g_wSelOvlEngIndex = wIndex % MAX_OVL_ENG_NUM;
    g_wBytePerPixel[wIndex] = 2; /*default is 2 bytes per pixel*/
}

/****************************************************************************
 Tvia_GetOverlayIndex() is used to query Overlay engine index.
 In  : None
 Out : Return the Overlay engine index which is currently used.
 ****************************************************************************/
u16 Tvia_GetOverlayIndex()
{
    return g_wSelOvlEngIndex;
}

/****************************************************************************
 Tvia_InitOverlay() initializes Overlay engine.
 In  : None
 Out : None
 Note: wOverlayIndex must be set before calling this routine.
 ****************************************************************************/
void Tvia_InitOverlay(void)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    /*Clear Overlay path first*/
    Out_Overlay_Reg(DISP_CTL_I, 0x00);

    /*Video Display Vertical Starting Line (may not need initiate here)*/
    Out_Overlay_Reg(DEST_RECT_TOP_L, 0x00);
    Out_Overlay_Reg(DEST_RECT_TOP_H, 0x00);

    /* By default, turn off double-buffer */
//    Tvia_SyncOverlayDoubleBuffer(g_wSelOvlEngIndex, OFF);
//    Tvia_EnableOverlayDoubleBuffer(g_wSelOvlEngIndex, OFF, INTERLACED);
//    if(g_wSelOvlEngIndex == OVERLAY1) {
//        Out_CRT_Reg_M(0xAB, 0x00, 0xFC);
//    }
//    else if(g_wSelOvlEngIndex == OVERLAY2) {
//        Out_CRT_Reg_M(0xAB, 0x00, 0xF3);
//    }

    /*Overlay Vertical DDA Increment Value*/
    Out_Overlay_Reg(DDA_Y_INC_L, 0x00);
    Out_Overlay_Reg(DDA_Y_INC_H, 0x10);

    /*Video Memory Starting Address*/
    Out_Overlay_Reg(MEMORY_START_L, 0x00);
    Out_Overlay_Reg(MEMORY_START_M, 0x0f);
    Out_Overlay_Reg(MEMORY_START_H, 0x03);  /*Temporary fixed to 
                                            0x30f00 = 0xc3c00 >> 2*/
    /*0x3c00 = 0x300*0x14 = 768*20*/

    /*Video Display Horizontal Starting Pixel -- may not need init here*/
    Out_Overlay_Reg(DEST_RECT_LEFT_L, 0x20);
    Out_Overlay_Reg(DEST_RECT_LEFT_H, 0x00);

    /*Video Display Horizontal Ending Pixel -- may not need init here*/
    Out_Overlay_Reg(DEST_RECT_RIGHT_L, 0x60);
    Out_Overlay_Reg(DEST_RECT_RIGHT_H, 0x01);

    /*Video Display Vertical Ending Line -- may not need init here*/
    Out_Overlay_Reg(DEST_RECT_BOTTOM_L, 0xe0);
    Out_Overlay_Reg(DEST_RECT_BOTTOM_H, 0x00);

    /*Video Color Compare Register*/
    Out_Overlay_Reg(COLOR_CMP_RED,  0x00);
    Out_Overlay_Reg(COLOR_CMP_GREEN,0x00);
    Out_Overlay_Reg(COLOR_CMP_BLUE, 0x00);

    /*Video Horizontal DDA Increment Value*/
    Out_Overlay_Reg(DDA_X_INC_L, 0x00);
    Out_Overlay_Reg(DDA_X_INC_H, 0x10);

    /*Video Format Control*/
    Out_Overlay_Reg(VIDEO_FORMAT, 0x00);

    /*Video Misc Control*/
    Out_Overlay_Reg(MISC_CTL_I, 0x00);

    Out_Overlay_Reg(MISC_CTL_I, 0x01); /*Video Misc Control*/
    /*Default to colorkey*/
    Out_Overlay_Reg(DISP_CTL_I, 0x04);

    /* By default, turn of chroma key */
    Tvia_EnableChromaKey(OFF);

    /* Select top/bottom (odd/even) source */
    Out_SEQ_Reg_M(0xA6, 0x20, 0xDF);

    /*Initialize Overlay Engine -- End*/
    if(g_bInitialized) /*only need to record old FIFO values once*/
        return;
    g_bInitialized = 1;

    g_b3CE_74 = In_Video_Reg(0x74);
    g_b3CE_75 = In_Video_Reg(0x75);

    g_bOvlRegD9[0] = In_Video_Reg(0xD9);
    g_bOvlRegDA[0] = In_Video_Reg(0xDA);
    g_bOvlRegDD[0] = In_Video_Reg(0xDD);

    g_bOvlRegD9[1] = In_SEQ_Reg(0xD9);
    g_bOvlRegDA[1] = In_SEQ_Reg(0xDA);
    g_bOvlRegDD[1] = In_SEQ_Reg(0xDD);

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_OverlayOn() turns on Overlay engine to display video image.
 In  : None
 Out : None
 ****************************************************************************/
void Tvia_OverlayOn(void)
{
    u8 b3CE_F7, b3CE_FA;
    u8 iTmp;
    u8 iTmpFA,val;


    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    Out_Overlay_Reg_M(DISP_CTL_I, 0x84, 0x7B);
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);

#ifdef RGBDAC_DISABLE
   printk("Tvia_OverlayOn RGBDAC disabling");
//  printk("3cf/bf02.3cf/b1\n");
   
//   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
//   iTmpFA = tvia_inb(0x3cf);
   iTmpFA = In_Video_Reg(0xBF);
	 
//   tvia_outb(0x02, 0x3cf);
   Out_Video_Reg(0xBF,0x02);
   
//   tvia_outb(0xB1, 0x3ce);
//   iTmp = tvia_inb(0x3cf);
   iTmp = In_Video_Reg(0xB1);

//   printk("3cf/bf02.3cf/b1=%x\n",iTmp);
   iTmp = (iTmp|0x0f);
	 
//   printk("3cf/fa05.3cf/b1=%x\n",iTmp);
//   tvia_outb(iTmp, 0x3cf);
   Out_Video_Reg(0xB1,iTmp);


//   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
//   tvia_outb(iTmpFA, 0x3cf);
   Out_Video_Reg(0xBF,iTmpFA);

#endif

#if RGB_FROM_CVBS_PATH
// Queste istruzioni abilitano SCART on connector
// che significa che gli RGBDAC usano
// il segnale che arriva dalla parte CVBS
// ciò fa sì che non ci siano le interference
// pur rimanendo accesi gli RGB analog outputs
// !!!raf
    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xFA, 0x05);
    iTmp = In_Video_Reg(0x4e);
	printk("Tvia_OverlayOn: ..3cf/4e=%x\n",iTmp);
//	iTmp=(iTmp&0x2f)|0x15;
	iTmp=iTmp|0x10;
	printk("Tvia_OverlayOn: mofified 3cf/4e =%x\n",iTmp);
    Out_Video_Reg(0x4e, iTmp);
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
//!!!raf
#endif

#if 0  
// !!!raf
    Out_CRT_Reg(0x1f, 0x13); /* to unlock CRT protection */	
	printk("\nTvia_OverlayOn: CRT protection unlocked\n");

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xbf);
    Out_Video_Reg(0xbf, 0x03);

	iTmp = In_Video_Reg(0xbc);
	printk("Tvia_OverlayOn: ..3cf/bc=%x\n",iTmp);
	iTmp=iTmp&0x0e;
	printk("Tvia_OverlayOn: mofified 3cf/bc =%x\n",iTmp);
    Out_Video_Reg(0xbc, iTmp);
	
    /* Restore banking */
    Out_Video_Reg(0xbf, b3CE_F7);
	
    Out_CRT_Reg(0x1f, 0x00); /* to unlock CRT protection */	
	
//!!!raf
#endif

#if 0 
// !!!raf
    Out_CRT_Reg(0x1f, 0x13); /* to unlock CRT protection */	
	printk("\nTvia_OverlayOn: CRT protection unlocked\n");
    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    iTmp = In_SEQ_Reg(0xdc);
	printk("Tvia_OverlayOn: ..3c5/dc=%x\n",iTmp);
	iTmp=(iTmp&0x7f);
	printk("Tvia_OverlayOn: mofified 3c5/dc =%x\n",iTmp);
    Out_SEQ_Reg(0xdc, iTmp);
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);

    Out_CRT_Reg(0x1f, 0x00); /* to unlock CRT protection */	
#endif	
//!!!raf


   // !!!raf
	 //Bank is 0 
	 //
	 
	 // 3cf/db[4:3] = 01 :  scale up , Duplicate previouse pixel  
   /* Save banking */
   b3CE_F7 = In_Video_Reg(0xF7);
   b3CE_FA = In_Video_Reg(0xFA);
   Out_Video_Reg(0xF7, 0x00);
   Out_Video_Reg(0xFA, 0x00);

   Out_Video_Reg_M(0xDB,0x00,~0x18); // 3cf/db[4:3] = 00 :  scale up , 0.5 Linearity approach  
	 
   /* Restore banking */
   Out_Video_Reg(0xF7, b3CE_F7);
   Out_Video_Reg(0xFA, b3CE_FA);


}

/****************************************************************************
 Tvia_OverlayOff() turns off Overlay engine.
 In  : None
 Out : None
 ****************************************************************************/
void Tvia_OverlayOff(void)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    /* Disable Vafc !!! */
    Out_Overlay_Reg_M(DISP_CTL_I, 0x00, 0x7F);
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetOverlayFormat() specifies video format, this video format is 
 necessary for Overlay engine to interpret and display video properly.
 In  : nFormat - one of popular video formats we support, see overlay.h for
                 the list.(YUV422, RGB555, RGB565, etc.)
 Out : None
 Note: You must set this format before you call Tvia_SetOverlaySrcAddr(), 
       because we should know how many bytes occupied by one pixel in frame 
       buffer.
 ****************************************************************************/
void Tvia_SetOverlayFormat(u8 bFormat)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    switch(bFormat) {
    case YUV422:
        Out_Overlay_Reg_M(VIDEO_FORMAT, 0x00, 0xF8);
        g_wBytePerPixel[g_wSelOvlEngIndex] = 2;
        break;
    case RGB555:
        Out_Overlay_Reg_M(VIDEO_FORMAT, 0x01, 0xF8);
        g_wBytePerPixel[g_wSelOvlEngIndex] = 2;
        break;
    case RGB565:
        Out_Overlay_Reg_M(VIDEO_FORMAT, 0x02, 0xF8);
        g_wBytePerPixel[g_wSelOvlEngIndex] = 2;
        break;
    case RGB888:
        Out_Overlay_Reg_M(VIDEO_FORMAT, 0x03, 0xF8);
        g_wBytePerPixel[g_wSelOvlEngIndex] = 3;
        break;
    case RGB8888:
        Out_Overlay_Reg_M(VIDEO_FORMAT, 0x04, 0xF8);
        g_wBytePerPixel[g_wSelOvlEngIndex] = 4;
        break;
    case RGB8:
        Out_Overlay_Reg_M(VIDEO_FORMAT, 0x05, 0xF8);
        g_wBytePerPixel[g_wSelOvlEngIndex] = 1;
        break;
    case RGB4444:
        Out_Overlay_Reg_M(VIDEO_FORMAT, 0x06, 0xF8);
        g_wBytePerPixel[g_wSelOvlEngIndex] = 2;
        break;
    case RGB8T:
        Out_Overlay_Reg_M(VIDEO_FORMAT, 0x07, 0xF8);
        g_wBytePerPixel[g_wSelOvlEngIndex] = 1;
        break;
    default:
        break;
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetOverlayMode() specifies overlay display mode.
 In  : nFormat = COLORKEY, if the graphic data at destination position(pixel)
                 exactly matchs ColorKey RGB value, video image will be 
                 displayed, otherwise show up graphic instead.
                 See Tvia_SetRGBKeyColor() for how to set ColorKey RGB value
               = WINDOWKEY, video image is always displayed no matter what 
                 graphic data it is.
 Out : None
 Note: Use COLORKEY mode can display mixed graphic and video image in same 
       area, while using WINDOWKEY can get better video memory bandwidth.
 ****************************************************************************/
void Tvia_SetOverlayMode(u8 bMode)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    switch (bMode) {
    case COLORKEY:
        Out_Overlay_Reg_M(DISP_CTL_I, 0x00, 0xFD);
        break;
    case WINDOWKEY:
    default:
        Out_Overlay_Reg_M(DISP_CTL_I, 0x02, 0xFD);
        break;
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetRGBKeyColor () setups color key registers.
 In  : cref - 32 bit values for rgb color, x, B, G, R with R in lower byte
 Out : None
 Note: 1.This function is only useful when color key format is used.
       2.At 256 color mode, input data should be R=G=B.
       3.At 16 Bpp mode, input data should be RGB = RGB & 0xF8FCF8 which is 
         RGB565 format.
 ****************************************************************************/
void Tvia_SetRGBKeyColor(u32 cref)
{
    /*Video Color Compare Register*/
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    bData = (u8)(cref & 0x00FF);
    Out_Overlay_Reg(COLOR_CMP_RED, bData);
    bData = (u8)((cref & 0x00FF00) >> 8);
    Out_Overlay_Reg(COLOR_CMP_GREEN, bData);
    bData = (u8)((cref & 0x00FF0000) >> 16);
    Out_Overlay_Reg(COLOR_CMP_BLUE, bData);
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetOverlayWindow() setup video overlay window (destination window).
 In  : Rectangle in which video will be displayed on the screen,
       wLeft   - x1
       wTop    - y1
       wRight  - x2
       wBottom - y2
 Out:  None
 Note: Coordiation unit is pixel.
 ****************************************************************************/
void Tvia_SetOverlayWindow(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    /* If DirectVideo is on, we should adjust coordinates here */
    if(g_iGenDvOn) {
        /* Align non-full-screen video with graphics in NTSC640x480 */
        if((g_iTvMode==NTSC) && (g_iModeNo==NTSC640x480) && !(wTop==0 && wBottom==486)) {
            wLeft  += 3;
            wRight += 3;
        }
        
        wTop    += g_wDvAdjOvlY[g_iTvMode][g_iModeNo];
        wBottom += g_wDvAdjOvlY[g_iTvMode][g_iModeNo];
        
        /* For full screen NTSC DirectVideo, vertical coordinates needs to - 4 
         * to display VBI. */
        if((g_iTvMode==NTSC) && (wTop==0) && (wBottom==485)) {
            wTop    -= 4;
            wBottom -= 4;
        }
    }
    
    /* Right and bottom are 1 based, so add 1 here */
    ++ wRight;
    ++ wBottom;
    
    Out_Overlay_Reg(DEST_RECT_LEFT_L, (u8)(wLeft & 0x00FF));
    Out_Overlay_Reg(DEST_RECT_LEFT_H, (u8)(wLeft >> 8));
    Out_Overlay_Reg(DEST_RECT_RIGHT_L, (u8)(wRight & 0x00FF));
    Out_Overlay_Reg(DEST_RECT_RIGHT_H, (u8)(wRight >> 8));
    
    Out_Overlay_Reg(DEST_RECT_TOP_L, (u8)(wTop & 0x00FF));
    Out_Overlay_Reg(DEST_RECT_TOP_H, (u8)(wTop >> 8));
    Out_Overlay_Reg(DEST_RECT_BOTTOM_L, (u8)(wBottom & 0x00FF));
    Out_Overlay_Reg(DEST_RECT_BOTTOM_H, (u8)(wBottom >> 8));
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetOverlayScale() specifies the ratio of our Overlay Scaler
 In  : bEnableBob = BOBMODE, Overlay engine will use Bob mode render video,
                    this mode display odd/even field respectively according
                    to CRT sync and is only necessary when you render 
                    interweaved video image (with odd and even field) on 
                    Non-Interlaced Monitor and tried to avoid motion effect. 
                    Because TV is interlaced display device by nature, you 
                    will not see motion effect on TV anyway.
                  = WEAVEMODE, Overlay engine will use Weave mode render 
                    video, this mode display the whole frame every CRT sync.
       wSrcXExt   - source window width
       wDstXExt   - destination window width
       wSrcYExt   - source window height
       wDstYExt   - destination window height
 Out : None
 Note: Source here refers to frame buffer, Destination refers to screen.
       Coordation unit is pixel.
 ****************************************************************************/
typedef u32 DWORD_;
typedef u16 WORD_;
typedef u8 BYTE_;

void Tvia_SetOverlayScale(u8 bEnableBob, u16 wSrcXExt, u16 wDstXExt, u16 wSrcYExt, u16 wDstYExt)
{
#if 0 
	  // ADVANTECH - diverso da SDK3
    u32 dwScale;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);

    /* X */
    Out_Overlay_Reg(DDA_X_INIT_L, 0x0); /* set to 0x0000; */
    Out_Overlay_Reg(DDA_X_INIT_H, 0x0);
    if(wSrcXExt == wDstXExt) {
        dwScale = 0x1000;
    }
    else {
        dwScale = ((u32)(wSrcXExt - 1) * 0x1000) / (wDstXExt - 1);
    }

    Out_Overlay_Reg(DDA_X_INC_L, (u8)((u16)(dwScale & 0x00FF)));
    Out_Overlay_Reg(DDA_X_INC_H, (u8)((u16)(dwScale & 0x00FF00) >> 8));

    /* Y */
    Out_Overlay_Reg(DDA_Y_INIT_L, 0x0);     /* set to 0x0000; */
    Out_Overlay_Reg(DDA_Y_INIT_H, 0x0);

    if(wSrcYExt == wDstYExt) {
        dwScale = 0x1000;
    }
    else {
        dwScale = ((u32)(wSrcYExt) * 0x1000) / (wDstYExt + 1);
    }

    if(!bEnableBob) {
        /*Disable Bob mode*/
        if(g_wSelOvlEngIndex == OVERLAY1) {
//            Out_CRT_Reg_M(0xf3, 0x00, 0xFD);
            Out_SEQ_Reg_M(BOB_WEAVE_CTL, 0x0, 0xFA); /*Bob/Weave disable*/
        }
        else if(g_wSelOvlEngIndex == OVERLAY2)
        {
//            Out_CRT_Reg_M(0xf3, 0x00, 0xFB);
            Out_SEQ_Reg_M(BOB_WEAVE_CTL, 0x0, 0xF5); /*Bob/Weave disable*/
        }
    }
    else {
        /*Enable Bob mode*/
        wSrcYExt = wSrcYExt / 2;

        if(wSrcYExt == wDstYExt) {
            dwScale = 0x1000;
        }
        else {
            dwScale = ((u32)(wSrcYExt) * 0x1000 ) / (wDstYExt + 1);
        }
        
        if(dwScale <= 0x815 && dwScale >= 0x7eB) {
            /* Bob/Weave enable, vertial DDA half density enable */
            if(g_wSelOvlEngIndex == OVERLAY1) {
                Out_SEQ_Reg_M(BOB_WEAVE_CTL, 0x5, 0xFA);
            }
            else if(g_wSelOvlEngIndex == OVERLAY2) {
                Out_SEQ_Reg_M(BOB_WEAVE_CTL, 0xA, 0xF5);
            }
        }
        else {
            /* Bob/Weave enable, vertial DDA half density disable */
            if(g_wSelOvlEngIndex == OVERLAY1) {
                Out_SEQ_Reg_M(BOB_WEAVE_CTL, 0x4, 0xFA);
            }
            else if(g_wSelOvlEngIndex == OVERLAY2) {
                Out_SEQ_Reg_M(BOB_WEAVE_CTL, 0x8, 0xF5);
            }
        }

//        Out_CRT_Reg_M(0xE0, 0x08, 0xF7);
//        if(g_wSelOvlEngIndex == OVERLAY1) {
//            Out_CRT_Reg_M(0xF3, 0x02, 0xFD);
//        }
//        else if(g_wSelOvlEngIndex == OVERLAY2) {
//            Out_CRT_Reg_M(0xF3, 0x04, 0xFB);
//        }
    }
    
    /* When DDA is on and data is YUV format and DirectVideo is not on, we should
     * turn on Y only. */
    if((dwScale < 0x1000) && !(In_Overlay_Reg(VIDEO_FORMAT) & 0x07) && 
        !g_iDvOn[g_wSelOvlEngIndex]) {
        if(g_wSelOvlEngIndex == OVERLAY1) {
            Out_SEQ_Reg_M( 0xA6, 0x40, 0xBF);
        }
        else if(g_wSelOvlEngIndex == OVERLAY2) {
            Out_SEQ_Reg_M( 0xA6, 0x80, 0x7F);
        }
    }
    else {
        if(g_wSelOvlEngIndex == OVERLAY1) {
            Out_SEQ_Reg_M( 0xA6, 0x00, 0xBF);
        }
        else if(g_wSelOvlEngIndex == OVERLAY2) {
            Out_SEQ_Reg_M( 0xA6, 0x00, 0x7F);
        }
    }

    Out_Overlay_Reg(DDA_Y_INC_L, (u8)((u16)(dwScale & 0x00FF)));
    Out_Overlay_Reg(DDA_Y_INC_H, (u8)((u16)(dwScale & 0x00FF00) >> 8));

    /* Always turn on Y interpolation */
    Out_Overlay_Reg_M(DISP_CTL_I, 0x00, 0xDF);

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);

#else

    // From SDK3
    DWORD_      dwIncX, dwIniX, dwIncY, dwIniY;
    BYTE_       b3CE_F7, b3CE_FA;
    
    
    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    /* X */
    if ( wSrcXExt == wDstXExt )
    {
        dwIncX = 0x1000;
    }
    else
    {
        dwIncX = ( ( DWORD_ )( wSrcXExt - 1 ) * 0x1000 ) / ( wDstXExt - 1 );
    }
    Out_Overlay_Reg( DDA_X_INC_L, ( BYTE_ )dwIncX );
    Out_Overlay_Reg( DDA_X_INC_H, ( BYTE_ )( dwIncX >> 8 ) );
    
    dwIniX = dwIncX << 1;
    if ( dwIniX >= 0x1000 )
    {
        dwIniX = 0;
    }
    else
    {
        dwIniX = 0x1000 - dwIniX;
    } 
    Out_Overlay_Reg( DDA_X_INIT_L, ( BYTE_ )dwIniX );
    Out_Overlay_Reg( DDA_X_INIT_H, ( BYTE_ )( dwIniX >> 8 ) );
    
    
    /* Y */
    if ( wSrcYExt == wDstYExt )
    {
        dwIncY = 0x1000;
    }
    else
    {
        dwIncY = ( ( DWORD_ )( wSrcYExt ) * 0x1000 ) / ( wDstYExt + 1 );
    }
    
    if ( !bEnableBob )
    {
        /*Disable Bob mode*/
        if      ( OVERLAY1 == g_wSelOvlEngIndex )
        {
            Out_CRT_Reg_M( 0xf3, 0x00, ~0x02 );
            Out_SEQ_Reg_M( BOB_WEAVE_CTL, 0x0, ~0x5 ); /*Bob/Weave disable*/
        }
        else if ( OVERLAY2 == g_wSelOvlEngIndex )
        {
            Out_CRT_Reg_M( 0xf3, 0x00, ~0x04 );
            Out_SEQ_Reg_M( BOB_WEAVE_CTL, 0x0, ~0xA ); /*Bob/Weave disable*/
        }
    }
    else
    {
        /*Enable Bob mode*/
        wSrcYExt = wSrcYExt / 2;
        
        if ( wSrcYExt == wDstYExt )
        {
            dwIncY = 0x1000;
        }
        else
        {
            dwIncY = ( ( DWORD_ )( wSrcYExt ) * 0x1000 ) / ( wDstYExt + 1 );
        }
        
        if ( dwIncY <= 0x815 && dwIncY >= 0x7eB )
        {
            /* Bob/Weave enable, vertial DDA half density enable */
            if      ( OVERLAY1 == g_wSelOvlEngIndex )
            {
                Out_SEQ_Reg_M( BOB_WEAVE_CTL, 0x5, ~0x5 );
            }
            else if ( OVERLAY2 == g_wSelOvlEngIndex )
            {
                Out_SEQ_Reg_M( BOB_WEAVE_CTL, 0xA, ~0xA );
            }
        }
        else
        {
            /* Bob/Weave enable, vertial DDA half density disable */
            if      ( OVERLAY1 == g_wSelOvlEngIndex )
            {
                Out_SEQ_Reg_M( BOB_WEAVE_CTL, 0x4, ~0x5 );
            }
            else if ( OVERLAY2 == g_wSelOvlEngIndex )
            {
                Out_SEQ_Reg_M( BOB_WEAVE_CTL, 0x8, ~0xA );
            }
        }
        
        Out_CRT_Reg_M( 0xE0, 0x08, ~0x08 );
        if      ( OVERLAY1 == g_wSelOvlEngIndex  )
        {
            Out_CRT_Reg_M( 0xF3, 0x02, ~0x02 );
        }
        else if ( OVERLAY2 == g_wSelOvlEngIndex  )
        {
            Out_CRT_Reg_M( 0xF3, 0x04, ~0x04 );
        }
    }
    
    /* When DDA is on and data is YUV format and DirectVideo is not on, we should
     * turn on Y only. */
    if ( dwIncY < 0x1000                           && 
        !( In_Overlay_Reg( VIDEO_FORMAT ) & 0x07 ) && 
        !g_iDvOn[g_wSelOvlEngIndex]                   )
    {
        if      ( OVERLAY1 == g_wSelOvlEngIndex )
        {
            Out_SEQ_Reg_M( 0xA6, 0x40, ~0x40 );
        }
        else if ( OVERLAY2 == g_wSelOvlEngIndex )
        {
            Out_SEQ_Reg_M( 0xA6, 0x80, ~0x80 );
        }
    }
    else
    {
        if      ( OVERLAY1 == g_wSelOvlEngIndex )
        {
            Out_SEQ_Reg_M( 0xA6, 0x00, ~0x40 );
        }
        else if ( OVERLAY2 == g_wSelOvlEngIndex )
        {
            Out_SEQ_Reg_M( 0xA6, 0x00, ~0x80 );
        }
    }
    
    Out_Overlay_Reg( DDA_Y_INC_L, ( BYTE_ )( ( WORD_ )dwIncY ) );
    Out_Overlay_Reg( DDA_Y_INC_H, ( BYTE_ )( ( WORD_ )dwIncY >> 8 ) );
    
    dwIniY = dwIncY << 1;
    if ( dwIniY >= 0x1000 )
    {
        dwIniY = 0;
    }
    else
    {
        dwIniY = 0x1000 - dwIniY;
    } 
    Out_Overlay_Reg( DDA_Y_INIT_L, ( BYTE_ )dwIniY );
    Out_Overlay_Reg( DDA_Y_INIT_H, ( BYTE_ )( dwIniY >> 8 ) );
    
    /* Always turn on Y interpolation */
    Out_Overlay_Reg_M( DISP_CTL_I, 0x00, ~0x20 );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
#endif
}


/****************************************************************************
 Tvia_SetOverlaySrcAddr() specifies Overlay source(frame buffer) coordination.
 In:   dwMemAddr   - memory address where you are concerned about Overlay
                     operation.
       wX          - source horizontal starting position, based on dwMemAddr.
       wY          - source vertical starting position, based on dwMemAddr.
       wFetchWidth - pixel width for each horizontal line, this width will
                     decide for each line how many pixels video data will be
                     fetched from frame buffer and rendered by Overlay 
                     engine.
       wPitchWidth - pixel width between two continual lines. This width will
                     help Overlay engine decide where is the beginning 
                     address of next line.
 Out : None
 Note: 1. dwMemAddr, wX, wY combination will uniquely decide where is the
          first pixel which will be fetched and rendered, this combination is
          very flexible, the real starting point is:
          dwMemAddr + (wX + wY * wPitchWidth) * BytesPerPixel
       2. wFetchWidth may be identical to wPitchWidth if you want to render
          the whole line, while it may be less than wPitchWidth if you are
          interrested in part of the line. Anyhow, it should never be larger
          than wPitchWidth, in that case, you will waste memory bandwidth
 ****************************************************************************/
void Tvia_SetOverlaySrcAddr(u32 dwMemAddr, u16 wX, u16 wY, u16 wFetchWidth, u16 wPitchWidth)
{
    u8 bHigh;
    u16 wByteFetch = wFetchWidth * g_wBytePerPixel[g_wSelOvlEngIndex];
    u16 wBytePitch = wPitchWidth * g_wBytePerPixel[g_wSelOvlEngIndex];
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    dwMemAddr += (u32)wY * wBytePitch + wX * g_wBytePerPixel[g_wSelOvlEngIndex];
    dwMemAddr >>= 2;
    /*playback start addr*/
    Out_Overlay_Reg(MEMORY_START_L, (u8)(dwMemAddr & 0x0000ff));
    Out_Overlay_Reg(MEMORY_START_M, (u8)((dwMemAddr & 0x00ff00) >> 8));
    Out_Overlay_Reg(MEMORY_START_H, (u8)((dwMemAddr & 0x00ff0000) >> 16));
    
    /* pitch and fetch are multiple of 64 bits */
    wBytePitch >>= 3;
    wByteFetch = ( wByteFetch + 7 ) >> 3;
    
    bHigh  = (u8)(wBytePitch >> 8) & 0x03;
    bHigh |= (u8)(wByteFetch >> 6) & 0x0C;
    
    Out_Overlay_Reg(MEMORY_PITCH_L, (u8)(wBytePitch & 0x00FF));
    Out_Overlay_Reg_M(MEMORY_PITCH_H, bHigh, 0xF0);
    
    Out_Overlay_Reg(MEMORY_OFFSET_PHASE, (u8)(wByteFetch & 0x00FF));
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_ChangeOverlayFIFO() sets some FIFO related registers in order to get  
 best memory bandwidth.
 In  : None
 Out : None
 Note: bFIFOPolicyNum array contains the preset FIFO values, if you want to 
       adjust these values, change the value there.
*****************************************************************************/
void Tvia_ChangeOverlayFIFO()
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    /*For BOARD5300*/
    Out_Video_Reg(0x74, bFIFOPolicyNum[g_wSelOvlEngIndex][0]);
    Out_Video_Reg(0x75, bFIFOPolicyNum[g_wSelOvlEngIndex][1]);
    Out_Overlay_Reg(0xD9, bFIFOPolicyNum[g_wSelOvlEngIndex][2]);
    Out_Overlay_Reg(0xDA, bFIFOPolicyNum[g_wSelOvlEngIndex][3]);
    
    if(g_wSelOvlEngIndex == OVERLAY1) {
        Out_Video_Reg_M(0xA6, 0x08, 0xF7);
        Out_Video_Reg_M(0xF1, 0x40, 0x3F);
    }
    else if(g_wSelOvlEngIndex == OVERLAY2) {
        Out_SEQ_Reg_M(0xDD, 0x12, 0xED);
        /* Out_Overlay_Reg_M( 0xF1, 0x40, ~0xC0 ); */
        Out_Video_Reg_M(0xF1, 0x40, 0x3F);
    }
    
    /* For BOARD5300 */
    Out_Overlay_Reg_M(FIFO_CTL_I, bFIFOPolicyNum[g_wSelOvlEngIndex][4] & 0x05, 0xFA);
    
    Out_Overlay_Reg_M(FIFO_CTL_I, 0x2, 0xFD);
    
    if(GetBpp() == 8) {
        Out_SEQ_Reg_M(0xDD, 0x20, 0xDF);
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_WhichOverlayOnTop() specifies which overlay will be on top for 
 overlapped area while displaying two overlay simultaneously.
 In:  bWhich - OVERLAY1: Overlay 1 will be on top
             - OVERLAY2: Overlay 2 will be on top
 Out: None
 ****************************************************************************/
void Tvia_WhichOverlayOnTop(u8 bWhich)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    if(bWhich == OVERLAY1) {
        Out_SEQ_Reg_M(0xDE, 0x00, 0xFE);
    }
    else if(bWhich == OVERLAY2) {
        Out_SEQ_Reg_M(0xDE, 0x01, 0xFE);
        Out_Video_Reg(0xFA, 0x08);
        Out_SEQ_Reg_M(0x4F, 0x08, 0xF7);
        Out_Video_Reg(0xFA, 0x00);
        Out_SEQ_Reg_M(0x4C, 0x00, 0x7F);
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_OverlayCleanUp() does some cleanup job just before you quit Overlay 
 operation.
 In  : None
 Out : None
 Note: This function should be called just before the application exits.
 ****************************************************************************/
void Tvia_OverlayCleanUp(void)
{
    u16 i;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    /* Disable Vafc !!! */
    Out_Video_Reg_M(DISP_CTL_I, 0x00, 0x7F);  
    Out_SEQ_Reg_M(DISP_CTL_I, 0x00, 0x7F);  
    
    /* Disable double buffer */
//    for(i=0; i<MAX_OVL_ENG_NUM; i++) {
//        Tvia_SyncOverlayDoubleBuffer(i, OFF);
//        Tvia_EnableOverlayDoubleBuffer(i, OFF, INTERLACED);
//    }
//    Out_CRT_Reg_M(0xAB, 0x00, 0xF0);
    
    /*restore FIFO control regs*/
    Out_SEQ_Reg_M(0xA7, 0x0, 0xFA);
    
    if(!g_bInitialized) {
        /* Restore banking */
        Out_Video_Reg(0xF7, b3CE_F7);
        Out_Video_Reg(0xFA, b3CE_FA);
        return;
    }
    g_bInitialized = 0;
    Out_Video_Reg(0x74, g_b3CE_74);
    Out_Video_Reg(0x75, g_b3CE_75);
    
    Out_Video_Reg(0xD9, g_bOvlRegD9[0]);
    Out_Video_Reg(0xDA, g_bOvlRegDA[0]);
    Out_Video_Reg(0xDD, g_bOvlRegDD[0]);
    
    Out_SEQ_Reg(0xD9, g_bOvlRegD9[1]);
    Out_SEQ_Reg(0xDA, g_bOvlRegDA[1]);
    Out_SEQ_Reg(0xDD, g_bOvlRegDD[1]);
    
    Out_SEQ_Reg_M(0xA6, 0x0, 0x3F); /*clear 3c4/a6[6][7]*/
    
    Out_Video_Reg(0xFA, 0x08);
    Out_SEQ_Reg_M(0x4F, 0x00, 0xF7); /* Clear 3c4/4f[3] */
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}
//#if 0
/****************************************************************************
 Tvia_Tvia_EnableOverlayDoubleBuffer() enables/disables Tvia 
 CyberPro53xx/52xx Overlay double-buffer feature.
 In  : wWhichOverlay = 0, Overlay engine 1 
                     = 1, Overlay engine 2 
       bEnable       - 1: enable, 0: disable
       bInterleaved  - 1: interleaved, 0: non-interleaved
 Out : None
 ****************************************************************************/
void Tvia_EnableOverlayDoubleBuffer(u16 wWhichOverlay, u8 bEnable, u8 bInterleaved)
{
    u8 bh, bl;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    if(bEnable) {
        if(wWhichOverlay == OVERLAY1) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);
            
            Out_Video_Reg_M(0xAC, 0x02, 0xFD);
            
            Out_Video_Reg(0xFA, 0x80);
            Out_Video_Reg_M(0xE9, 0x02, 0xFD);
            
            if(bInterleaved) {
                Out_Video_Reg_M(0xE3, 0x00, 0xF7);
            }
            else {
                Out_Video_Reg_M(0xE3, 0x08, 0xF7);
            }
            
            /* Overlay 1 should not shift down one line except in DirectVideo + 
             * double buffer. */ 
            if(!g_iDvOn[OVERLAY1]) {
                Out_Video_Reg_M(0xE0, 0x00, 0xBF);
            }
            
            /* Default is overlay 1 vs. capture 1. 
             * You can call the routine Tvia_DoubleBufferSelectRivals() to let 
             * overlay 1 vs. capture 2. */
            /* Overlay 1 selects rival capture 1 */
            Out_Video_Reg_M(0xE9, 0x00, 0xFE);
            
            Out_Video_Reg(0xFA, 0x00);
            
            Out_CRT_Reg_M(0xAC, 0x00, 0xFD);
            
            /* For topmost overlay window, close new double-buffering logic and use old one. */
            bl = In_Video_Reg(DEST_RECT_TOP_L); 
            bh = In_Video_Reg(DEST_RECT_TOP_H);
            if(!(bl | bh)) {
                Out_CRT_Reg_M(0xAB, 0x00, 0xFC);
            }
            else {
                Out_CRT_Reg_M(0xAB, 0x03, 0xFC);
            }
        }
        else if(wWhichOverlay == OVERLAY2) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x80);
            
            /* Enable Overlay 2 Double-buffer */
            Out_Video_Reg_M(0xE9, 0x20, 0xDF);
            Out_Video_Reg_M(0xE1, 0x40, 0xBF);
            
            /* Default is capture 2 vs. overlay 2.
             * You can call the routine Tvia_DoubleBufferSelectRivals() to let 
             * overlay 2 vs. capture 1. */
            /* Overlay 2 selects rival capture 2 */
            Out_Video_Reg_M(0xE9, 0x10, 0xEF); 
            
            if(bInterleaved) {
                Out_Video_Reg_M(0xE3, 0x00, 0x7F);
            }
            else {
                Out_Video_Reg_M(0xE3, 0x80, 0x7F);
            }
            
            /* Overlay 2 should not shift down one line except in DirectVideo + 
             * double buffer. */ 
            if(!g_iDvOn[OVERLAY2]) {
                Out_Video_Reg_M(0xE0, 0x00, 0x7F);
            }
            
            /* For topmost overlay window, close new double-buffering logic and use old one. */
            Out_Video_Reg(0xFA, 0x00);
            
            bl = In_SEQ_Reg(DEST_RECT_TOP_L); 
            bh = In_SEQ_Reg(DEST_RECT_TOP_H);
            if(!(bl | bh)) {
                Out_CRT_Reg_M(0xAB, 0x00, 0xF3);
            }
            else {
                Out_CRT_Reg_M(0xAB, 0x0C, 0xF3);
            }
        }
    }
    else {
        if(wWhichOverlay == OVERLAY1) {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x00);
            
            Out_Video_Reg_M(0xAC, 0x00, 0xFD);
            
            Out_Video_Reg(0xFA, 0x80);
            
            Out_Video_Reg_M(0xE9, 0x00, 0xFD);
            
            Out_Video_Reg(0xFA, 0x00);
            
            Out_CRT_Reg_M(0xAB, 0x00, 0xFC);
        }
        else if(wWhichOverlay == OVERLAY2)
        {
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xFA, 0x80);
            
            Out_Video_Reg_M(0xE9, 0x00, 0xDF);
            Out_Video_Reg_M(0xE1, 0x00, 0xBF); 
            
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xAB, 0x00, 0xF3);
        }
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SyncOverlayDoubleBuffer() enables/disables Tvia CyberPro53xx/52xx 
 Overlay double-buffer synchnorous mode.
 In  : wWhichOverlay = 0, Overlay engine 1 
                     = 1, Overlay engine 2 
       bEnable       - 1: enable, 0: disable
 Out : None
 ****************************************************************************/
void Tvia_SyncOverlayDoubleBuffer(u16 wWhichOverlay, u8 bEnable)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x80);
    
    if(bEnable) {
        if(wWhichOverlay == OVERLAY1) {
            /* Overlay 1 */
            Out_Video_Reg_M(0xE9, 0x04, 0xFB);
        }
        else if(wWhichOverlay == OVERLAY2) {
            /* Overlay 2 */
            Out_Video_Reg_M(0xE9, 0x40, 0xBF);
        }
    }
    else {
        if(wWhichOverlay == OVERLAY1) {
            /* Overlay 1 */
            Out_Video_Reg_M(0xE9, 0x00, 0xFB);
        }
        else if(wWhichOverlay == OVERLAY2) {
            /* Overlay 2 */
            Out_Video_Reg_M(0xE9, 0x00, 0xBF);
        }
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/****************************************************************************
 Tvia_SetOverlayBackBufferAddr() sets Overlay double-buffer second/back 
 buffer address
 In  : dwOffAddr   - back buffer address
       wX          - source horizontal starting position, based on dwOffAddr.
       wY          - source vertical starting position, based on dwOffAddr.
       wPitchWidth - pixel width between two continual lines. This width 
                     will help Overlay engine decide where is the beginning 
                     address of next line.
 Out : None                                            
 ****************************************************************************/
void Tvia_SetOverlayBackBufferAddr(u16 wWhichOverlay, u32 dwOffAddr, u16 wX, u16 wY, u16 wPitchWidth)
{                          
    u32 dwL;
    u16 wBytePitch = wPitchWidth* g_wBytePerPixel[g_wSelOvlEngIndex];
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    dwL = dwOffAddr + (u32)wY * wBytePitch + wX * g_wBytePerPixel[g_wSelOvlEngIndex];
    dwL = dwL >> 2;
    
    Out_Video_Reg(0xFA, 0x00);
    Out_Video_Reg(0xF7, 0x00);
    
    if(wWhichOverlay == OVERLAY1) {
        WriteReg(GRAINDEX, 0xFA, 0x01);
        WriteReg(GRAINDEX, 0x41, (u8)(dwL & 0x00FF) );
        WriteReg(GRAINDEX, 0x42, (u8)((dwL & 0x00FF00) >> 8));
        Out_Video_Reg(0xFA, 0x00);
        Out_Video_Reg_M(0xAD, (u8)(dwL >> 16), 0xF0);
        Out_Video_Reg_M(0x4D, (u8)(dwL >> 15), 0xDF);
    }
    else if(wWhichOverlay == OVERLAY2) {
        WriteReg(GRAINDEX, 0xFA, 0x80);
        WriteReg(GRAINDEX, 0xE5, (u8)(dwL & 0x00FF) );
        WriteReg(GRAINDEX, 0xE6, (u8)((dwL & 0x00FF00) >> 8));
        Out_Video_Reg_M(0xE7, (u8)(dwL >> 16), 0xF0);
        WriteReg(GRAINDEX, 0xFA, 0x00);
        Out_Video_Reg_M(0x4D, (u8)(dwL >> 14), 0xBF);
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}
//#endif
/****************************************************************************
 Tvia_EnableChromaKey() enables/disables video chroma key.
 in:   bOnOff - 1: enable
                0: disable
 out:  none
 ****************************************************************************/
void Tvia_EnableChromaKey(u8 bOnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    if(!bOnOff) { /* disable */
        if(g_wSelOvlEngIndex == OVERLAY1) {
            Out_Video_Reg_M(0xea, 0x00, 0xDF);
        }
        else if(g_wSelOvlEngIndex == OVERLAY2) {
            Out_SEQ_Reg_M(0xde, 0x00, 0xFB);
        }
    }
    else { /* Enable */
        if(g_wSelOvlEngIndex == OVERLAY1) {
            Out_Video_Reg_M(0xea, 0x20, 0xDF);
        }
        else if(g_wSelOvlEngIndex == OVERLAY2) {
            Out_SEQ_Reg_M(0xde, 0x04, 0xFB);
        }
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}        

/****************************************************************************
 Tvia_SetChromaKey() setups chroma key registers.  
 in:   creflow  - 32 bit RGB low value, x, B, G, R with R in lower byte
       crefhigh - 32 bit RGB high value, x, B, G, R with R in lower byte
 out:  none
 ****************************************************************************/
void Tvia_SetChromaKey(u32 creflow, u32 crefhigh)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    Out_Video_Reg(0xF7, 0x00);
    
    if(g_wSelOvlEngIndex == OVERLAY1) {
        Out_Video_Reg(0xfa, 0x00);
        bData = (u8)(creflow & 0x00FF);
        Out_Video_Reg(CHROMA_CMP_RED_LO, bData);
        bData = (u8)((creflow & 0x00FF00) >> 8);
        Out_Video_Reg(CHROMA_CMP_GREEN_LO, bData);
        bData = (u8)((creflow & 0x00FF0000) >> 16);
        Out_Video_Reg(CHROMA_CMP_BLUE_LO, bData);
        bData = (u8)(crefhigh & 0x00FF);
        Out_Video_Reg(CHROMA_CMP_RED_HI, bData);
        bData = (u8)((crefhigh & 0x00FF00) >> 8);
        Out_Video_Reg(CHROMA_CMP_GREEN_HI, bData);
        bData = (u8)((crefhigh & 0x00FF0000) >> 16);
        Out_Video_Reg(CHROMA_CMP_BLUE_HI, bData);
    }
    else if(g_wSelOvlEngIndex == OVERLAY2) {
        Out_Video_Reg(0xfa, 0x00);
        bData = (u8)(creflow & 0x00FF);
        Out_CRT_Reg(CHROMA_CMP_RED_LO, bData);
        bData = (u8)((creflow & 0x00FF00) >> 8);
        Out_CRT_Reg(CHROMA_CMP_GREEN_LO, bData);
        bData = (u8)((creflow & 0x00FF0000) >> 16);
        Out_CRT_Reg(CHROMA_CMP_BLUE_LO, bData);
        bData = (u8)(crefhigh & 0x00FF);
        Out_CRT_Reg(CHROMA_CMP_RED_HI, bData);
        bData = (u8)((crefhigh & 0x00FF00) >> 8);
        Out_CRT_Reg(CHROMA_CMP_GREEN_HI, bData);
        bData = (u8)((crefhigh & 0x00FF0000) >> 16);
        Out_CRT_Reg(CHROMA_CMP_BLUE_HI, bData);
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}
