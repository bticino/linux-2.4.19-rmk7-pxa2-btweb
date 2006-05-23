/*
 *  TVIA CyberPro 5202 direct video interface
 *
 */

#include "tvia5202-def.h"
#include "tvia5202-dvideo.h"
#include "tvia5202-misc.h"
#include "tvia5202-overlay.h"
#include "tvia5202-alpha.h"

/* DirectVideo on/off */
int g_iGenDvOn = 0;
int g_iDvOn[MAX_OVL_ENG_NUM] = 
{
    0, 0 
};

/* Display mode for Direct Video */
int g_iTvMode;   /* PAL/NTSC */
int g_iModeNo;   /* Mode # */

/* Direct Video overlay window adjustment value */
int g_wDvAdjOvlY[2][4] =
{
    {
        /* PAL */
        /* 640x480 PAL */
        0xB6,
        
        /* 720x540 PAL */
        0x00,
        
        /* 768x576 PAL */
        0x00,
        
        /* 720x576 PAL */
        0x54
    },
    {
        /* NTSC */
        /* 640x480 NTSC */
        0x4E,
        /* 720x480 NTSC */
        0x4E,
        /* 640x440 NTSC */
        0x00,
    }
};

/* PAL/NTSC mode alpha base registers value tables in the order of wModenum */
static u8 bAlphaBase[2][4][6] =
{
    {
        /* PAL */
        {
            /* 640x480 PAL */
            0x02, 0x10, 0x4F, 0x0A, 0x61, 0x1C
        },
        {
            /* 720x540 PAL */
            0x02, 0x10, 0x4F, 0xc6, 0x50, 0xe7
        },
        {
            /* 768x576 PAL */
            0x02, 0x10, 0x4B, 0x4e, 0x50, 0xe7
        },
        {
            /* 720x576 PAL */
            0x02, 0x10, 0x4B, 0x9C, 0x60, 0x4E
        }
    },    
    {
        /* NTSC */
        {
            /* 640x480 NTSC */
            0x02, 0x10, 0x19, 0x94, 0x50, 0xEA
        },
        {
            /* 720x480 NTSC */
            0x02, 0x10, 0x19, 0xC2, 0x60, 0x74
        },
        {
            /* 640x440 NTSC */
            0x02, 0x10, 0x18, 0x78, 0x80, 0x30
        }
    }
};

static u8 b3CE_A5, b3C4_A5;
static u8 b3CE_AD, b3C4_A6;
static u16 wE5F6, wE60E, wE696;

static int Tvia_AdjDvOvlWinInt(int iDvOn, int iTvMode, int iModeNo);
static int Tvia_AdjDvBlendWinInt(int iDvOn, int iTvMode, int iModeNo);

/**************************************************************************
 Tvia_EnableDirectVideo() is used to enable/disable DirectVideo mode
 in:  bWhichOverlay: 0 - Overlay1
                     1 - Overlay2
      wTVMode      : current TV output mode, should be NTSC or PAL
      wModenum     : current resolution mode
      bOnOff       : Non-zero - Enable
                     0        - Disable
 out: none
 Note: If Overlay double-buffer has to be programmed, make sure
       Tvia_EnableOverlayDoubleBuffer() is called before
       Tvia_EnableDirectVideo().
       Tvia_EnableDirectVideo() is obsolete and only for downward
       compatibility purpose. User should call Tvia_EnableGenericDirectVideo()
       and Tvia_EnableWhichDirectVideo() instead.
 **************************************************************************/
void Tvia_EnableDirectVideo(u8 bWhichOverlay, u16 wTVMode, u16 wModenum, u8 bOnOff)
{
    if(bOnOff) {  /* Enable DirectVideo */
        Tvia_EnableGenericDirectVideo(wTVMode, wModenum, bOnOff);
        Tvia_EnableWhichDirectVideo(bWhichOverlay, bOnOff);
    }
    else {           /* Disable DirectVideo */
        Tvia_EnableWhichDirectVideo(bWhichOverlay, bOnOff);
        if(!(g_iDvOn[0] || g_iDvOn[1])) {
            Tvia_EnableGenericDirectVideo(wTVMode, wModenum, bOnOff);
        }
    }
}

/*****************************************************************************
 Tvia_EnableGenericDirectVideo() is used to enable/disable DirectVideo generically
 in:
      wTVMode      : current TV output mode, should be NTSC or PAL
      wModenum     : current resolution mode
      bOnOff       : Non-zero - Enable
                     0        - Disable
 out: none
 Note: If Overlay double-buffer has to be programmed, make sure
       Tvia_EnableOverlayDoubleBuffer() is called before
       Tvia_EnableGenericDirectVideo().
 *****************************************************************************/
void Tvia_EnableGenericDirectVideo(u16 wTVMode, u16 wModenum, u8 bOnOff)
{
    unsigned int i, t1, t2;
    u8 b3CE_F7, b3CE_FA;
    
    /* If it's redundent calling, just return */
    t1 = bOnOff ? 1 : 0;
    t2 = g_iGenDvOn? 1 : 0;
    if(!(t1 ^ t2)) {
        return;
    }

    g_iTvMode = wTVMode;
    g_iModeNo = wModenum;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    /* Adjust window */
    Tvia_AdjDvOvlWinInt((int)bOnOff, g_iTvMode, g_iModeNo);
    Tvia_AdjDvBlendWinInt((int)bOnOff, g_iTvMode, g_iModeNo);
    
    if(bOnOff) { /* Enable DirectVideo mode */
        if(!g_iGenDvOn) { /* It's the first time to turn on DirectVideo */
            wE5F6 = InTVReg(0xE5F6);
            wE60E = InTVReg(0xE60E);
            /* First , save some reg value */
            Out_Video_Reg(0xFA , 0x00);
            b3C4_A5 = In_SEQ_Reg(0xA5);
            b3CE_A5 = In_Video_Reg(0xA5);
            
            wE696 = InTVReg(0xE696);
            OutTVRegM(0xE5F6, 0x106, ~0x106);
            OutTVRegM(0xE696, 0x400, ~0x400);
            OutTVRegM(0xE60E, 0x0000, ~0x0002);
        }
        
        g_iGenDvOn = 1;
        
        /*Enable interlace mode*/
        Out_CRT_Reg_M(0xF2, 0xC0, 0x3F);
        
        /* Set alpha-blending window base*/
        /* Switch register bank */
        Out_Video_Reg(0xFA, 0x80);
        /* Set alpha base vertical position */
        for(i = 0; i < 3; i++) {
            Out_Video_Reg((u8)(0xE2 + i), bAlphaBase[wTVMode][wModenum][i]);
        }
        /* Switch register bank */
        Out_Video_Reg(0xFA, 0x00);
        /*set alpha base horizontal position*/
        for(i=0; i < 3; i++) {
            Out_Video_Reg((u8)(0x4A + i), bAlphaBase[wTVMode][wModenum][i+3]);
        }
        
        /* Set H/V Sync come from internal clock */
        Out_SEQ_Reg_M(0xEF, 0x30, 0xCF); 
        
        /* H sync shifter delay */
        Out_SEQ_Reg(0xEA, 0x01);
        
        /*Programe alpha-blending path here*/
        /*You should call SelectBlendSrcA() and SelectBlendSrcB() to choose Graphics
        and Video1 or Video2 as alpha-blending source before you call this API*/
        Out_Video_Reg(0xFA, 0x00);
        Out_SEQ_Reg_M(0x4C, 0x40, 0xBF); /*Set Graphics to be TV feedback graphics*/
        /*You should call SelectAlphaSrc() and set alpha value accordingly before
        you call this API*/
        Out_Video_Reg(0xFA, 0x08);
        Out_SEQ_Reg_M(0x4F, 0x10, 0xEF); /*Only graphics send to TV encoder*/
        Out_Video_Reg(0xFA, 0x00);
        Out_SEQ_Reg_M(0x4A, 0x00, 0xFC); /*Invert graphics data which comes from TV encoder U/V bit 7*/
        Out_Video_Reg(0xFA, 0x08);
        Out_SEQ_Reg_M(0x47, 0x28, 0xD7); /*Invert U/V bit 7 back when finished alpha-blending*/
        Out_Video_Reg(0xFA, 0x00);

        /*Partically enable Synclock*/
        Out_SEQ_Reg_M(0xEF, 0x01, 0xFE);
        Out_SEQ_Reg_M(0xED, 0x20, 0xDF);
    }
    else { /* Disable DirectVideo mode */
        g_iGenDvOn = 0;

        /* Restore value back */
        OutTVReg(0xE5F6, wE5F6);
        OutTVReg(0xE60E, wE60E);

        OutTVReg(0xE696, wE696);

        /*Disable interlace mode*/
        Out_CRT_Reg_M(0xF2, 0x00, 0x3F);

        Out_Video_Reg(0xFA, 0x08);
        Out_SEQ_Reg_M(0x4F, 0x00, 0xEF);
        Out_Video_Reg(0xFA, 0x08);
        Out_SEQ_Reg_M(0x47, 0x00, 0xD7);
        
        Out_Video_Reg(0xFA , 0x00);
        Out_SEQ_Reg(0xA5 , b3C4_A5);
        Out_Video_Reg(0xA5 , b3CE_A5);

        Out_SEQ_Reg(0xEA, 0x00);

        /*Disable Synclock*/
        Out_SEQ_Reg_M(0xEF, 0x00, 0xFE);
        Out_SEQ_Reg_M(0xED, 0x00, 0xDF);
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/*****************************************************************************
 Tvia_EnableWhichDirectVideo() is used to enable/disable DirectVideo mode for
 individual Overlay engine.
 in:  bWhichOverlay: 0 - Overlay1
                     1 - Overlay2
      bOnOff       : Non-zero - Enable
                     0        - Disable
 out: none
 Note: When this API is used to enable individual Overlay engine,
       Tvia_EnableGenericDirectVideo() has to be called before call this API.
       After all Overlay engines have been disabled for DirectVideo mode,
       Tvia_EnableGenericDirectVideo() should be called afterwards to disable DirectVideo
       mode generically.
 *****************************************************************************/
void Tvia_EnableWhichDirectVideo(u8 bWhichOverlay, u8 bOnOff)
{
    unsigned int t1, t2;
    u8 b3CE_F7, b3CE_FA;

    /* If it's redundent calling, just return */
    t1 = bOnOff ? 1 : 0;
    t2 = g_iDvOn[bWhichOverlay]? 1 : 0;
    if(!(t1 ^ t2)) {
        return;
    }
    
    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    Out_Video_Reg(0xF7, 0x00);
    
    if(bOnOff) { /* Enable DirectVideo mode */
        /*Enable interlace mode*/
        if(bWhichOverlay == OVERLAY1) {
            g_iDvOn[OVERLAY1] = 1;
            
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xAB, 0x03, 0xFC);
            
            /* Correct double bufffer (if applicable) in interlace mode */
            Out_Video_Reg(0xFA, 0x80);
            Out_Video_Reg_M(0xDF, 0x80, 0x7F);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xF0, 0x40, 0xBF);
            Out_SEQ_Reg_M(0xA7, 0x00, 0xFE); /*Unlock vertical DDA from 2:1*/
            
            /* For DirectVideo + double buffer mode */
            Out_Video_Reg(0xFA, 0x80);
            Out_Video_Reg_M(0xE0, 0x40, 0xBF);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xF3, 0x20, 0xDF);
            
            /* disable claimping */
            Out_SEQ_Reg(0xA5 , 0x00);
            
            /* Disable Y data only */
            b3C4_A6 = TVIA_GET_MASKED(In_SEQ_Reg(0xA6), b3C4_A6, 0xBF);
            Out_SEQ_Reg_M(0xA6, 0x00, 0xBF);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_Video_Reg(0xF7, 0x00);
            b3CE_AD = In_Video_Reg(0xAD);
            if(!g_iDvOn[OVERLAY2]) {
                Out_Video_Reg(0xAD, b3CE_AD | 0x10);
            }
            else {
                Out_Video_Reg(0xAD, b3CE_AD & 0xCF);
            }
            Out_Video_Reg(0xFA, 0x08);
            Out_SEQ_Reg_M(0x4E, 0x00, 0x7F);
        }
        else if(bWhichOverlay == OVERLAY2) {
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xAB, 0x0C, 0xF3);
            
            /* Correct double bufffer (if applicable) in interlace mode */
            Out_Video_Reg(0xFA, 0x80);
            Out_SEQ_Reg_M(0xDF, 0x80, 0x7F);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xF0, 0x80, 0x7F);
            Out_SEQ_Reg_M(0xA7, 0x00, 0xFD); /* Unlock vertical DDA from 2:1 */
            
            /* For DirectVideo + double buffer mode */
            Out_Video_Reg(0xFA, 0x80);
            Out_Video_Reg_M(0xE0, 0x80, 0x7F);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xF3, 0x40, 0xBF);
            
            /* disable claimping */
            Out_CRT_Reg_M(0x95, 0x00, 0x3F);
            
            /* Disable Y data only */
            b3C4_A6 = TVIA_GET_MASKED(In_SEQ_Reg(0xA6), b3C4_A6, 0x7F);
            Out_SEQ_Reg_M(0xA6, 0x00, 0x7F);
            
            g_iDvOn[OVERLAY2] = 1;
            
            Out_Video_Reg(0xFA, 0x00);
            Out_Video_Reg(0xF7, 0x00);
            b3CE_AD = In_Video_Reg(0xAD);
            if(!g_iDvOn[OVERLAY1]) {
                Out_Video_Reg(0xAD, b3CE_AD | 0x20);
                Out_Video_Reg(0xFA, 0x08);
                Out_SEQ_Reg_M(0x4E, 0x80, 0x7F);
            }
            else {
                Out_Video_Reg(0xAD, b3CE_AD & 0xCF);
                Out_Video_Reg(0xFA, 0x08);
                Out_SEQ_Reg_M(0x4E, 0x00, 0x7F);
            }
        }
    }
    else { /* Disable DirectVideo mode */
        /* Disable interlace mode */
        if(bWhichOverlay == OVERLAY1) {
            /* Cleanup double buffer in interlaced mode */
            Out_Video_Reg(0xFA, 0x80);
            Out_Video_Reg_M(0xDF, 0x00, 0x7F);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xF0, 0x00, 0xBF);
            Out_CRT_Reg_M(0xF3, 0x00, 0xDF);
            g_iDvOn[OVERLAY1] = 0;
            
            Out_SEQ_Reg_M(0xA6, b3C4_A6, 0xBF);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xAD, b3CE_AD & 0xCF);
            
            Out_Video_Reg(0xFA, 0x80);
            Out_Video_Reg_M(0xE0, 0x0, 0xBF);
        }
        else if(bWhichOverlay == OVERLAY2) {
            /* Cleanup double buffer in interlaced mode */
            Out_Video_Reg(0xFA, 0x80);
            Out_SEQ_Reg_M(0xDF, 0x00, 0x7F);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_CRT_Reg_M(0xF0, 0x00, 0x7F);
            Out_CRT_Reg_M(0xF3, 0x00, 0xBF);
            g_iDvOn[OVERLAY2] = 0;
            
            Out_SEQ_Reg_M(0xA6, b3C4_A6, 0x7F);
            
            Out_Video_Reg(0xFA, 0x00);
            Out_Video_Reg(0xF7, 0x00);
            Out_Video_Reg(0xAD, b3CE_AD & 0xCF);
            
            Out_Video_Reg(0xFA, 0x08);
            Out_SEQ_Reg_M(0x4E, 0x00, 0x7F);
            
            Out_Video_Reg(0xFA, 0x80);
            Out_Video_Reg_M(0xE0, 0x0, 0x7F);
        }
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
}

/* Tvia_AdjDvOvlWinInt() -- Internal routine for adjusting overlay windows when 
 *                          turning on/off Direct Video.
 *
 * Parameters:
 *      iDvOn         -- [IN] DirectVideo on/off
 *      iTvMode       -- [IN] PAL/NTSC
 *      iModeNo       -- [IN] Mode #
 *
 * Returns:
 *      non-zero, if adjusted.
 *      zero, if no need to adjust as it has been adjust before.
 */
static int Tvia_AdjDvOvlWinInt(int iDvOn, int iTvMode, int iModeNo)
{
    unsigned int i;
    u16 wLeft, wRight, wTop, wBottom;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    Out_Video_Reg(0xF7, 0x00);
    Out_Video_Reg(0xFA, 0x00);
    
    for (i=OVERLAY1; i < MAX_OVL_ENG_NUM; i++) {
        /* Load vertical coordinates first */
        if(i == OVERLAY1) {
            wLeft = In_Video_Reg(DEST_RECT_LEFT_L);
            wLeft |= ((u16)In_Video_Reg(DEST_RECT_LEFT_H)) << 8;
            wRight = In_Video_Reg(DEST_RECT_RIGHT_L);
            wRight |= ((u16)In_Video_Reg(DEST_RECT_RIGHT_H)) << 8;
            wTop = In_Video_Reg(DEST_RECT_TOP_L);
            wTop |= ((u16)In_Video_Reg(DEST_RECT_TOP_H)) << 8;
            wBottom = In_Video_Reg(DEST_RECT_BOTTOM_L);
            wBottom |= ((u16)In_Video_Reg(DEST_RECT_BOTTOM_H)) << 8;
        }
        else if(i == OVERLAY2) {
            wLeft = In_SEQ_Reg(DEST_RECT_LEFT_L);
            wLeft |= ((u16)In_SEQ_Reg(DEST_RECT_LEFT_H)) << 8;
            wRight = In_SEQ_Reg(DEST_RECT_RIGHT_L);
            wRight |= ((u16)In_SEQ_Reg(DEST_RECT_RIGHT_H)) << 8;
            wTop = In_SEQ_Reg(DEST_RECT_TOP_L);
            wTop |= ((u16)In_SEQ_Reg(DEST_RECT_TOP_H)) << 8;
            wBottom = In_SEQ_Reg(DEST_RECT_BOTTOM_L);
            wBottom |= ((u16)In_SEQ_Reg(DEST_RECT_BOTTOM_H)) << 8;
        }
        
        /* Adjust */
        if(iDvOn) {
            /* Align non-full-screen video with graphics in NTSC640x480 */
            if((iTvMode==NTSC) && (iModeNo==NTSC640x480) && !(wTop==0 && wBottom==486)) {
                wLeft  += 3;
                wRight += 3;
            }
            
            wTop += g_wDvAdjOvlY[iTvMode][iModeNo];
            wBottom += g_wDvAdjOvlY[iTvMode][iModeNo];
            
            /* For full screen NTSC DirectVideo, vertical coordinates needs to - 4 
             * to display VBI. */
            if((iTvMode==NTSC) && (g_wDvAdjOvlY[iTvMode][iModeNo]==wTop) && 
                ((g_wDvAdjOvlY[iTvMode][iModeNo]+486)==wBottom)) {
                wTop    -= 4;
                wBottom -= 4;
            }
        }
        else {
            /* When turning off full screen NTSC DirectVideo, vertical coordinates 
             * needs to + 4 */
            if((iTvMode==NTSC) && ((g_wDvAdjOvlY[iTvMode][iModeNo]-4)==wTop) && 
                ((g_wDvAdjOvlY[iTvMode][iModeNo]-4+486)==wBottom)) {
                wTop    += 4;
                wBottom += 4;
            }
            
            wTop    -= g_wDvAdjOvlY[iTvMode][iModeNo];
            wBottom -= g_wDvAdjOvlY[iTvMode][iModeNo];
            
            /* Align non-full-screen video with graphics in NTSC640x480 */
            if((iTvMode==NTSC) && (iModeNo==NTSC640x480) &&
                !(wTop==0 && wBottom==486)) {
                wLeft  -= 3;
                wRight -= 3;
            }
        }

        /* Set adjusted vertical coordinates back */
        if(i == OVERLAY1) {
            Out_Video_Reg(DEST_RECT_LEFT_L, (u8)(wLeft & 0x00FF));
            Out_Video_Reg(DEST_RECT_LEFT_H, (u8)(wLeft >> 8));
            Out_Video_Reg(DEST_RECT_RIGHT_L, (u8)(wRight & 0x00FF));
            Out_Video_Reg(DEST_RECT_RIGHT_H, (u8)(wRight >> 8));
            Out_Video_Reg(DEST_RECT_TOP_L, (u8)(wTop & 0x00FF));
            Out_Video_Reg(DEST_RECT_TOP_H, (u8)(wTop >> 8));
            Out_Video_Reg(DEST_RECT_BOTTOM_L, (u8)(wBottom & 0x00FF));
            Out_Video_Reg(DEST_RECT_BOTTOM_H, (u8)(wBottom >> 8));
        }
        else if(i == OVERLAY2) {
            Out_SEQ_Reg(DEST_RECT_LEFT_L, (u8)(wLeft & 0x00FF));
            Out_SEQ_Reg(DEST_RECT_LEFT_H, (u8)(wLeft >> 8));
            Out_SEQ_Reg(DEST_RECT_RIGHT_L, (u8)(wRight & 0x00FF));
            Out_SEQ_Reg(DEST_RECT_RIGHT_H, (u8)(wRight >> 8));
            Out_SEQ_Reg(DEST_RECT_TOP_L, (u8)(wTop & 0x00FF));
            Out_SEQ_Reg(DEST_RECT_TOP_H, (u8)(wTop >> 8));
            Out_SEQ_Reg(DEST_RECT_BOTTOM_L, (u8)(wBottom & 0x00FF));
            Out_SEQ_Reg(DEST_RECT_BOTTOM_H, (u8)(wBottom >> 8));
        }
    }
    
    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);

    return 1;
}


/* Tvia_AdjDvBlendWinInt() -- Internal routine for adjusting alpha windows when 
 *                            turning on/off Direct Video.
 *
 * Parameters:
 *      iDvOn         -- [IN] DirectVideo on/off
 *      iTvMode       -- [IN] PAL/NTSC
 *      iModeNo       -- [IN] Mode #
 *
 * Returns:
 *      non-zero, if adjusted.
 *      zero, if no need to adjust as it has been adjust before.
 */
static int Tvia_AdjDvBlendWinInt(int iDvOn, int iTvMode, int iModeNo)
{
    unsigned int i;
    u16 wLeft, wRight, wTop, wBottom, wTmp;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);

    Out_Video_Reg(0xF7, 0x00);
    
    for(i=0; i<MAX_BLEND_WIN_NUM; i++) {
        /* Set banking */
        if(i == Win0) {
            Out_Video_Reg(0xFA, 0x00);
        }
        else if((i == Win1) || (i == Win2)) {
            Out_Video_Reg(0xFA, 0x10);
        }
        else if(i == Win3) {
            Out_Video_Reg(0xFA, 0x18);
        }
        
        /* Load alpha-blending window coordinates */
        if(i == Win2) {
            /*windows 2 has different indexes*/
            wLeft = In_SEQ_Reg(0x46);
            wRight = In_SEQ_Reg(0x48);
            wTmp = In_SEQ_Reg(0x47);
            wLeft |= (wTmp & 0x0007) << 8;
            wRight |= (wTmp & 0x0070) << 4;
            
            wTop = In_SEQ_Reg(0x49);
            wBottom = In_SEQ_Reg(0x4B);
            wTmp = In_SEQ_Reg(0x4A);
            wTop |= (wTmp & 0x0007) << 8;
            wBottom |= (wTmp & 0x0070) << 4;
        }
        else {
            /* windows 0,1 and 3 have different bank only */
            wLeft = In_SEQ_Reg(0x40);
            wRight = In_SEQ_Reg(0x42);
            wTmp = In_SEQ_Reg(0x41);
            wLeft |= (wTmp & 0x0007) << 8;
            wRight |= (wTmp & 0x0070) << 4;
            
            wTop = In_SEQ_Reg(0x43);
            wBottom = In_SEQ_Reg(0x45);
            wTmp = In_SEQ_Reg(0x44);
            wTop |= (wTmp & 0x0007) << 8;
            wBottom |= (wTmp & 0x0070) << 4;
        }
        
        /* Adjust */
        if(iDvOn) {
            if((iTvMode==NTSC) && (iModeNo==NTSC640x480) && !(wTop==0 && wBottom==486)) {
                /* To display VBI correctly, we have to shift the alpha
                 * window 3 pixels */
                wLeft  += 3;
                wRight += 3;
            }
            ++ wLeft;
            
            wTop    += g_wDvAdjOvlY[iTvMode][iModeNo];
            wBottom += g_wDvAdjOvlY[iTvMode][iModeNo];
            
            /* For full screen NTSC DirectVideo, vertical coordinates needs to - 4 
             * to display VBI. */
            if((iTvMode==NTSC) && (g_wDvAdjOvlY[iTvMode][iModeNo]==wTop) &&
                ((g_wDvAdjOvlY[iTvMode][iModeNo]+486)==wBottom)) {
                wTop    -= 4;
                wBottom -= 4;
            }
        }
        else {
            /* When turning off full screen NTSC DirectVideo, vertical coordinates 
             * needs to + 4 */
            if((iTvMode==NTSC) && ((g_wDvAdjOvlY[iTvMode][iModeNo]-4)==wTop) && 
                ((g_wDvAdjOvlY[iTvMode][iModeNo]-4+486)== wBottom)) {
                wTop    += 4;
                wBottom += 4;
            }
            
            wTop    -= g_wDvAdjOvlY[iTvMode][iModeNo];
            wBottom -= g_wDvAdjOvlY[iTvMode][iModeNo];
            
            if((iTvMode==NTSC) && (iModeNo==NTSC640x480) && !(wTop==0 && wBottom==486)) {
                /* To display VBI correctly, we have to shift the alpha
                 * window 3 pixels */
                wLeft  -= 3;
                wRight -= 3;
            }
            -- wLeft;
        }
        
        /* Set adjusted coordinates back */
        if(i == Win2) {
            /*windows 2 has different indexes*/
            Out_SEQ_Reg(0x46, (u8)(wLeft & 0x00FF));
            
            Out_SEQ_Reg(0x48, (u8)(wRight & 0x00FF));
            
            wTmp = ((wLeft >> 8) & 0x07) + ((wRight >> 4) & 0x70);
            Out_SEQ_Reg(0x47, (u8)wTmp);
            
            Out_SEQ_Reg(0x49, (u8)(wTop & 0x00FF));
            
            Out_SEQ_Reg(0x4B, (u8)(wBottom & 0x00FF));
            
            wTmp = ((wTop >> 8) & 0x07) + ((wBottom >> 4) & 0x70);
            Out_SEQ_Reg(0x4A, (u8)wTmp);
        }
        else {
            /* windows 0,1 and 3 have different bank only */
            Out_SEQ_Reg(0x40, (u8)(wLeft & 0x00FF));
            
            Out_SEQ_Reg(0x42, (u8)(wRight & 0x00Ff));
            
            wTmp = ((wLeft >> 8) & 0x07) + ((wRight >> 4) & 0x70);
            Out_SEQ_Reg(0x41, (u8)wTmp);
            
            Out_SEQ_Reg(0x43, (u8)(wTop & 0x00FF));
            
            Out_SEQ_Reg(0x45, (u8)(wBottom & 0x00FF));
            
            wTmp = ((wTop >> 8) & 0x07) + ((wBottom >> 4) & 0x70);
            Out_SEQ_Reg(0x44, (u8)wTmp);
        }
    }

    /* Restore banking */
    Out_Video_Reg(0xF7, b3CE_F7);
    Out_Video_Reg(0xFA, b3CE_FA);
    
    return 1;
}
