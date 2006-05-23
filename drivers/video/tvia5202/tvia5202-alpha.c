/*
 *  TVIA CyberPro 5202 alpha blend interface
 *
 */

#include "tvia5202-def.h"
#include "tvia5202-alpha.h"
#include "tvia5202-overlay.h"
#include "tvia5202-misc.h"

extern unsigned long fb_mem_addr;
extern unsigned short x_res;

/* External variables for window adjustment when Direct Video is on */
extern u16 g_wDvAdjOvlY[2][4];
extern int g_iGenDvOn;
extern int g_iTvMode;   /* PAL/NTSC */
extern int g_iModeNo;   /* Mode # */

static u16 bEnabled8Bit = 0; /*internal flag - 1: in 8-bit alpha-blending mode*/

static int MagRepON = 0;

/*Synclock Overlay feature*/
static u8 bSLockOverlayInited = 0;
static u8 b3C4_4A;
static u8 b3C4_4F;

/*The following varible must be set prior to calling alphamap function AMap.c.
  Call function SetAlphaMapPtr() to intialize this variable.*/
u32 dwAlphaMapPtr; /*A pointer to alpha map*/
u32 dwTop8BitLines; /*A number of 8bit(BPP)-line before alpha map*/

/****************************************************************************
 SetBlendWindow() sets coordination of a bending window
 in:    Alpah-blending window position within the screen:
        wLeft   (x1)
        wTop    (y1)
        wRight  (x2)
        wBottom (y2)
        bWhichWindow = Win0 - Alpha blending window 0
                       Win1 - Alpha blending window 1
                       Win2 - Alpha blending window 2
                       Win3 - Alpha blending window 3
  out:  none
  Note: Total 4 alpha blending windows are supported.
        1. All 4 windows are enabled and overlaped each other, the overlaped 
           area will show image in the priority as follows:
           Alpha Windows0 < Alpha Window1 < Alpha Window2 < Alpha Window3
        2. All 4 alpha blending windows have same alpha source A and source B,
           e.g. source A = Overlay 1 and source B = graphics, then all 
           enabled alpha blending windows will all have Overlay 1 as source A
           and graphics as source B.
 ****************************************************************************/
void SetBlendWindow(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom, u8 bWhichWindow)
{
    u16 wData;
    u8 b3CE_F7, b3CE_FA;

    /* Saving banking */
    b3CE_F7 = In_Video_Reg(0xF7);
    b3CE_FA = In_Video_Reg(0xFA);
    
    /* If DirectVideo is on, we should adjust coordinates here */
    if(g_iGenDvOn)
    {
        if((g_iTvMode==NTSC) && (g_iModeNo==NTSC640x480) && !(wTop==0 && wBottom==486))
        {
            /* To display VBI correctly, we have to shift the alpha
             * window 3 pixels */
            wLeft  += 3;
            wRight += 3;
        }
        ++ wLeft;
        
        wTop    += g_wDvAdjOvlY[g_iTvMode][g_iModeNo];
        wBottom += g_wDvAdjOvlY[g_iTvMode][g_iModeNo];
        
        /* For full screen NTSC DirectVideo, vertical coordinates needs to - 4 
         * to display VBI. */
        if((g_iTvMode==NTSC) && (0 == wTop) && (485 == wBottom) )
        {
            wTop    -= 4;
            wBottom -= 4;
        }
    }
    
    /* Right and bottom should plus 1, since the values to be written to 
     * registers are 1 based. */
    ++ wRight;
    ++ wBottom;
    
    Out_Video_Reg( 0xF7, 0 );
    if (bWhichWindow == Win0)
    {
        Out_Video_Reg( 0xFA, 0 );
    }
    else if ( (bWhichWindow == Win1) || (bWhichWindow == Win2) )
    {
        Out_Video_Reg( 0xFA, 0x10 );
    }
    else if (bWhichWindow == Win3)
    {
        Out_Video_Reg( 0xFA, 0x18 );
    }
    
    if (bWhichWindow == Win2)
    {
        /*windows 2 has different indexes*/
        Out_SEQ_Reg( 0x46, (u8)(wLeft & 0x00FF));
        
        Out_SEQ_Reg( 0x48, (u8)(wRight & 0x00FF));
        
        wData = ((wLeft >> 8) & 0x07) + ((wRight >> 4) & 0x70);
        Out_SEQ_Reg( 0x47, (u8)wData );
        
        Out_SEQ_Reg( 0x49, (u8)(wTop & 0x00FF));
        
        Out_SEQ_Reg( 0x4B, (u8)(wBottom & 0x00FF));
        
        wData = ((wTop >> 8) & 0x07) + ((wBottom >> 4) & 0x70);
        Out_SEQ_Reg( 0x4A, (u8)wData);
    }
    else
    {
        /*windows 0,1 and 3 have different bank only*/
        Out_SEQ_Reg( 0x40, (u8)(wLeft & 0x00FF));
        
        Out_SEQ_Reg( 0x42, (u8)(wRight & 0x00FF));
        
        wData = ((wLeft >> 8) & 0x07) + ((wRight >> 4) & 0x70);
        Out_SEQ_Reg( 0x41, (u8)wData);
        
        Out_SEQ_Reg( 0x43, (u8)(wTop & 0x00FF));
        
        Out_SEQ_Reg( 0x45, (u8)(wBottom  & 0x00FF));
        
        wData = ((wTop >> 8) & 0x07) + ((wBottom >> 4) & 0x70);
        Out_SEQ_Reg( 0x44, (u8)wData);
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableAlphaBlend() enableor disable alpha-blending generally
 in:   ON  - enable alpha blending
       OFF - disable alpha blending
 out:  none
 Note: To enable/disable individual alpha-blending window, call
 ****************************************************************************/
void EnableAlphaBlend(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x4B, 0x80, 0x7F );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4B, 0x00, 0x7F );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );    
}

/****************************************************************************
 Enable or Disable Alpha Blending for entire screen
 in:   ON  - enable alpha blending
       OFF - disable alpha blending
 Note: 1. When full screen alpha blending is enabled, there is no multiple 
          alpha blending windows and no need to program alpha window or 
          windows.
       2. Caller routine still needs to call EnableAlphaBlend().
 ****************************************************************************/
void EnableFullScreenAlphaBlend(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0 );
    Out_Video_Reg( 0xF7, 0 );
    
    if (OnOff == ON)
    {
        Out_SEQ_Reg_M( 0x4B, 0x40, 0xBF );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4B, 0x00, 0xBF );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Enable or Disable individule Alpha Blending window
 in:   ON  - enable alpha blending
       OFF - disable alpha blending
       bWhichWindow = Win0 - Alpha blending window 0
                      Win1 - Alpha blending window 1
                      Win2 - Alpha blending window 2
                      Win3 - Alpha blending window 3
 Note: 1. All 4 alpha blending windows are enabled when power on as long as 
          main enable control (Reg_4B_0<7>) is on.
       2. This routine is provided so that after setting each or all 4 alpha
          windows, we can disable/enable them by just calling this routine 
          and leave alpha windows setting unchanged.
 ****************************************************************************/
void EnableWhichAlphaBlend(u8 OnOff, u8 bWhichWindow)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    bData = 0x01;
    if ( bWhichWindow == Win1 )
    {
        bData <<= 1;
    }
    else if ( bWhichWindow == Win2 )
    {
        bData <<= 2;
    }
    else if ( bWhichWindow == Win3 )
    {
        bData <<= 3;
    }
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x08 );
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x4E, 0x00, ~bData );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4E, bData, ~bData );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 CleanUpAlphaBlend() cleans up some Alpha blending related registers since 
 these registers will not be touched by setmode software
 in:   none
 out:  none
 Note: call this routine when you want to quit all alpha-blending operations
 ****************************************************************************/
void CleanUpAlphaBlend(void)
{
    u16 i, j;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0 );
    Out_Video_Reg( 0xF7, 0 );
    
    for ( i = 0; i < 16; i++ )
    {
        if ( i == 0x0A )
        {
            /* Don't clean up SyncLock video path if there is one */
            Out_SEQ_Reg_M( 0x4A, 0x00, 0x0B );
        }
        else
        {
            Out_SEQ_Reg( 0x40 + i, 0x00 );
        }
    }
    
    Out_Video_Reg( 0xFA, 0x08 );
    for ( i = 0; i < 16; i++ )
    {
        if (  i == 0x0F )
        {
            /* Just in case there is a SyncLock video */
            Out_SEQ_Reg_M( 0x4F, 0x00, 0xF9 );
        }
        else if( i == 0x0E )
        {
            Out_SEQ_Reg_M( 0x4e, 0x00, 0x80 );
        }
        else
        {
            Out_SEQ_Reg( 0x40 + i, 0x00 );
        }
    }
    
    for ( j = 0x10; j <= 0x58; j += 8 )
    {
        if( j == 0x38 )
        {
            continue; /* This should be handled in CleanUpSLockOverlay() */
        }
        
        Out_Video_Reg( 0xFA, j );
        for ( i = 0; i < 16; i++ )
        {
            if ( j == 0x40 && i <= 0x9 )
            {
                continue; /* This should be handled in CleanUpSLockOverlay() */
            }
            
            
            /* To avoid overide digital out settings, do not clear 
             * the following register if digital out is on. */
#if 0 //#if !( DIGITAL_OUT )
            if ( ( 0x10 == j && 0x0F == i ) ||
                ( 0x28 == j && 0x0C == i ) ||
                ( 0x20 == j && 0x07 == i ) ||
                ( 0x20 == j && 0x06 == i )    )
            {
                continue;
            }
#endif
            
            Out_SEQ_Reg( 0x40 + i, 0x00 );
        }
    }
    
    /* For video capture */
    Out_SEQ_Reg_M( 0xA6, 0x00, 0xF0 );
    
    /* Disable all alpha blending windows */
    Out_Video_Reg( 0xFA, 0x08 );
    Out_SEQ_Reg_M( 0x4E, 0x0F, 0xF0 );
    
    /* For 8-bit Index mode. 
     * If we are in 8-bit alpha-blending mode, remember to disable it */
    if ( bEnabled8Bit ) 
    {
        EnablePaletteMode(0);
    }
    
    Out_Video_Reg( 0xFA, 0x80 );
    Out_Video_Reg_M( 0xE0, 0x04, 0xFB );
    
    /* Clear CP52xx new alphamap engine settings */
    Out_Video_Reg( 0xF7, 0 );
    Out_Video_Reg( 0xFA, 0x00 );
    for ( i = 0; i < 0x10; i++ )
    {
        Out_CRT_Reg( 0xD0 + i, 0 );
    }
    
    /* Clear magic number settings */
    Out_Video_Reg( 0xFA, 0x48 );
    Out_SEQ_Reg_M( 0x4D, 0x0F, 0xF0 );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 RAM alpha continuously loops or loop only one time
 in:   ON  - enable h/w automatic loop of the index to alpha look up table 
             forever
       OFF - disable h/w forever loop, loop one time only
 Note: When RAM is chosen as the alpha source and auto function is enabled, 
       h/w will increase index to the alpha RAM automatically.
 ****************************************************************************/
void RAMLoop(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x4B, 0x08, 0xF7 );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4B, 0x00, 0xF7 );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Choose which one will be used as source A to be alpha blended
 in:   bSrcA - Source A used for alpha blending
               SrcA_Gr  = 0 - Source A is from orginal graphics
               SrcA_V1  = 1 - Source A is from Overlay one
               SrcA_V2  = 2 - Source A is from Overlay two
               SrcA_Ram = 3 - Source A is from look up table (RAM)
 Note: 1. SrcA data can be inverted before being blended. See 
          InvertFunction().
       2. Alpha value is meanful to source A, e.g. if you set Alpha value to
          0x30(that is 0x30/0xFF = 18.8%), that menas the output will be 
          18.8% source A plus 81.2% source B
 ****************************************************************************/
void SelectBlendSrcA(u8 bSrcA)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0 );
    Out_Video_Reg( 0xF7, 0 );
    
    Out_SEQ_Reg_M( 0x49, bSrcA, 0xFC );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Choose which one will be used to address the look up table(RAM)
 in:  bRAMAddrSel - Source to address the look up table (RAM)
                    Ram_Int   = 0 - RAM indexes are from interleave alpha 
                                    embeded in graphics data. This only 
                                    applies to 16 bpp: 4444 or 1555
                    Ram_V2    = 1 - RAM indexes are from Overlay two (this 
                                    time it is used as alpha-map)
                    Ram_VSync = 2 - V_Sync (auto loop)
                    Ram_CPU   = 3 - When CPU accesses look up table (RAM)
 ****************************************************************************/
void SelectRAMAddr(u8 bRAMAddrSel)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );    
    
    Out_SEQ_Reg_M( 0x49, ( bRAMAddrSel << 3 ), 0xE7 );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Choose alpha source where alpha data are defined
 in:   bAlphaSrc - Alpha data source
                   Alpha_Int = 0 - Alpha is from interleave alpha embeded
                                    in graphics data. This only applies to 
                                    32 bpp: 8888
                   Alpha_V2  = 1 - Alpha is from Overlay 2 map
                   Alpha_Ram = 2 - Alpha is from look up table (RAM)
                   Alpha_Reg = 3 - Alpha is from register
                   Alpha_Map = 4 - Alpha is from alphamap engine
 Note: 1. Alpha data can be inverted before being blended. See 
          InvertFunction().
       2. !!! if Overlay 2 selected as alpha map:
          1) Overlay 2 must be enabled as normal (3c4_dc<7>=1)
          2) Overlay 2 window size = alpha window size.
          3) call EnableV2AlphaMap() to enable Overlay 2 as alpha map
 ****************************************************************************/
void SelectAlphaSrc(u8 bAlphaSrc)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0 );
    Out_Video_Reg( 0xFA, 0 );
    
    if(bAlphaSrc == Alpha_Map) /*for Tvia CyberPro52xx alphamap engine*/
    {
        Out_SEQ_Reg_M( 0x49, 0x20, 0x9F );
    }
    else
    {
        Out_SEQ_Reg_M( 0x49, ( bAlphaSrc << 5 ), 0x9F );
    }
    
    /*if alpha source comes form Overlay2, we need to disable Overlay2 color
    key function*/
    
    if ( bAlphaSrc == Alpha_V2 )
    {
        /*Disable Overlay 2 in Source B path*/
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg_M( 0x4F, 0x08, 0xF7 );
        
        /*Disable V2 generally*/
        Out_Video_Reg( 0xFA, 0x20 );
        Out_SEQ_Reg_M( 0x47, 0x02, 0xFD );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Enable/disable Overlay 2 as alpha map
 in:  OnOff: ON  - Enable Overlay 2 as alpha map
             OFF - disable Overlay 2 as alpha map
      bAlphaBpp: Alpha depth of the alpha map
                 ALPHABPP1 = 0 - 1 bit alpha  <-+   These apply to RAM only,
                 ALPHABPP2 = 1 - 2 bit alpha  <-+   i.e. alpha is used as index 
                                 to RAM
                 ALPHABPP4 = 2 - 4 bit alpha  <-+   (look up table).
                 ALPHABPP8 = 3 - 8 bit alpha, as direct alpha value applied to 
                                 alpah blending formula.
                 ALPHABPP  = 4 - alpha type defined in 3c4_db<2,1,0>, this has 
                                 much less format meaning as an alpha.
 ****************************************************************************/
void EnableV2AlphaMap(u8 OnOff, u8 bAlphaBpp)
{
    u8 bDataA6, bData4A, bCurOverlay;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );    
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    if ( OnOff == OFF )
    {
        Out_SEQ_Reg_M( 0xA6, 0x00, 0xF8 );
        Out_SEQ_Reg( 0xDB, 0x00 );
    }
    else
    {
        bDataA6 = In_SEQ_Reg( 0xA6 );
        Out_SEQ_Reg( 0xA6, bDataA6 | 0x03 ); /* Set both bit 1 & 0 */
        
        bData4A = In_SEQ_Reg( 0x4A ) & 0x3F;
        
        switch (bAlphaBpp)
        {
        case ALPHABPP1:
            break;
            
        case ALPHABPP2:
            bData4A |= 0x40;
            break;
            
        case ALPHABPP4:
            bData4A |= 0x80;
            break;
            
        case ALPHABPP8:
            bData4A |= 0xC0;
            break;
            
        case ALPHABPP:   /* Set bit 1 only */
            Out_SEQ_Reg( 0xA6, ( bDataA6 & 0xFC ) | 0x02 );
            break;
            
        case ALPHABPP16:
            Out_SEQ_Reg( 0xA6, ( bDataA6 & 0xFC ) ); 
            break;
            
        case ALPHABPP24:
            Out_SEQ_Reg( 0xA6, ( bDataA6 & 0xFC ) ); 
            break;
        }
        
        Out_SEQ_Reg( 0x4A, bData4A );
        
        if ( bAlphaBpp < 3 )
        {
            /* If it is Index mode (1,2,4 bit) */
            /* Look up table index comes from alpha map */
            Out_SEQ_Reg_M( 0x49, 0x08, 0xE7 );                       
        }
        
        /* Disable Overlay 2 DDA scaling */
        /* Disable Overlay 2 Bob/Weave mode */
        Out_SEQ_Reg_M( 0xA7, 0x00, 0xF5 );
        
        DisableV2DDA(); /* Now it is not a true video overlay, so we don't
                           need DDA*/
        
        bCurOverlay = (u8)Tvia_GetOverlayIndex(); /* For Tvia CyberPro53xx/52xx*/
        Tvia_SelectOverlayIndex( OVERLAY2 );
        Tvia_SetOverlayMode( WINDOWKEY );
        Tvia_SelectOverlayIndex( (u16)bCurOverlay );
    }
    
    if ( OnOff == ON )
    {
        Out_Video_Reg( 0xFA, 0x80 );
        Out_Video_Reg_M( 0xE0, 0x00, 0xFB );
    }
    else
    {
        Out_Video_Reg( 0xFA, 0x80 );
        Out_Video_Reg_M( 0xE0, 0x04, 0xFB );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
  Disable Overlay 2 scaling 
  When Overlay 2 as alpha map, no scaling is allowed.
  Just set the DDA to be 1:1 ratio.
 ****************************************************************************/
void DisableV2DDA(void)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    Out_SEQ_Reg( 0xD1, 0x00 );  /* x */
    Out_SEQ_Reg( 0xD2, 0x80 );
    
    Out_SEQ_Reg( 0xD3, 0x00 );
    Out_SEQ_Reg( 0xD4, 0x10 );
    
    Out_SEQ_Reg( 0xD5, 0x00 );  /* y */
    Out_SEQ_Reg( 0xD6, 0x80 );
    
    Out_SEQ_Reg( 0xD7, 0x00 );
    Out_SEQ_Reg( 0xD8, 0x10 );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Choose which one will be used as source B to be alpha blended
 in:   bSrcB: Source A used for alpha blending
              SrcB_V1  = 0 - Source B is Overlay one
              SrcB_V2  = 1 - Source B is Overlay two
              SrcB_Gr  = 2 - Source B is orginal graphics
              SrcB_Ram = 3 - Source B is look up table (RAM)
              SrcB_V12 = 4 - Source B is Overlay one and two
 Note: SrcB data can be inverted before being blended. See InvertFunction().
       Currently, SrcB_Gr and SrcB_Ram are not working for 3000.
 ****************************************************************************/
void SelectBlendSrcB(u8 bSrcB)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    if ( bSrcB == SrcB_V12 )
    {
        Out_SEQ_Reg_M( 0x4D, 0x00, 0xCF );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4D, ( bSrcB << 4 ), 0xCF );
    }
    
    if ( SrcB_V1 == bSrcB )
    {
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg_M( 0x4F, 0x08, 0xF7 );
    }
    else if ( SrcB_V12 == bSrcB )
    {
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg_M( 0x4F, 0x00, 0xF7 );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Set waiting counter value when RAM is in auto loop mode
 in:   bWaitCount:  H/W will wait the number of v_sync times before auto 
                    increase the index to the alpha look up table.
                    bWaitCount must be in 0 - 127 (0-7F).
 Note: There is another register 3ce_4d_0 which can also be used to define 
       the counter in 2,4,8..256.
 ****************************************************************************/
void SelectWaitCount(u8 bWaitCount)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x10 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    Out_SEQ_Reg_M( 0x4E, ( bWaitCount | 0x80 ), ~( bWaitCount | 0x80 ) );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 set Alpha values into Alpha R,G,B registers
 in:    bR           - Red 8 bits
        bG           - Green 8 bits
        bB           - Blue  8 bits
        bWhichWindow = Win0 - Alpha blending window 0
                       Win1 - Alpha blending window 1
                       Win2 - Alpha blending window 2
                       Win3 - Alpha blending window 3
 Note: This function is used only when alpha source is from alpha register.
 ****************************************************************************/
void SetAlphaReg(u8 bR, u8 bG, u8 bB, u8 bWhichWindow)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0 );
    
    if ( bWhichWindow == 0 )
    {
        Out_Video_Reg( 0xFA, 0 );
    }
    else
    {
        /* For windows 1,2,3 */
        Out_Video_Reg( 0xFA, 0x18 );
    }
    
    switch ( bWhichWindow )
    {
    case 0:
    case 1:
        Out_SEQ_Reg( 0x46, bR );
        Out_SEQ_Reg( 0x47, bG );
        Out_SEQ_Reg( 0x48, bB );
        break;
        
    case 2:
        Out_SEQ_Reg( 0x49, bR );
        Out_SEQ_Reg( 0x4a, bG );
        Out_SEQ_Reg( 0x4b, bB );
        break;
        
    case 3:
        Out_SEQ_Reg( 0x4c, bR );
        Out_SEQ_Reg( 0x4d, bG );
        Out_SEQ_Reg( 0x4e, bB );
        break;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 set Alpha values into Alpha look up table (RGB RAM)
 in:   bRAMType = 0 - RAM Data from 0 -> 7 then 7 -> 0
                  1 - RAM Data from 0 -> 15
                  2 - All RAM in same R,G,B values
       bR, bG, bB are seeds of the look up table.
 Note: These are predefined schemes to set up alpha look up table.
       This routine is just a sample to show some ways to set look up table.
       Developers can set look up table in any way they like to.
       e.g. if bRAMType = 0 and R=G=B=0x23, the final look up table will 
       have the R,G,B values:
       00 23 46 69 8C AF FF  FF AF 8C 69 46 23 00
 ****************************************************************************/
void SetAlphaRAM(u8 bRAMType, u8 bR, u8 bG, u8 bB)
{
    u16 i,j;
    u16 wData;
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    bData = In_SEQ_Reg( 0x49 );
    Out_SEQ_Reg( 0x49, 0x18 );  /* Select CPU to write RAM */
    
    switch (bRAMType)
    {
    case 0:
        for (j=0; j<16; j++)
        {
            i = (j<=7) ? j : (15-j);
            Out_SEQ_Reg( 0x4E, 0x20+j ); /* Enable index of alpha RAM R */
            
            wData = bR*i;
            wData = (wData > 0xf0) ? 0xFF : wData;
            Out_SEQ_Reg( 0x4F, wData ); /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x40+j ); /* Enable index of alpha RAM G */
            
            wData = bG*i;
            wData = (wData > 0xf0) ? 0xFF : wData;
            Out_SEQ_Reg( 0x4F, wData ); /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x80+j ); /* Enable index of alpha RAM B */
            
            wData = bB*i;
            wData = (wData > 0xf0) ? 0xFF : wData;
            Out_SEQ_Reg( 0x4F, wData ); /* RAM data port */
        }
        break;
        
    case 1:
        for (i=0; i<16; i++)
        {
            Out_SEQ_Reg( 0x4E, 0x20+i ); /* Enable index of alpha RAM R */
            
            wData = bR*i;
            Out_SEQ_Reg( 0x4F, wData ); /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x40+i ); /* Enable index of alpha RAM G */
            
            wData = bG*i;
            Out_SEQ_Reg( 0x4F, wData ); /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x80+i ); /* Enable index of alpha RAM B */
            
            wData = bB*i;
            Out_SEQ_Reg( 0x4F, wData ); /* RAM data port */
        }
        break;
        
    case 2:
        for (i=0; i<16; i++)
        {
            Out_SEQ_Reg( 0x4E, 0x20+i ); /* Enable index of alpha RAM R */
            
            Out_SEQ_Reg( 0x4F, bR ); /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x40+i ); /* Enable index of alpha RAM G */
            
            Out_SEQ_Reg( 0x4F, bG ); /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x80+i ); /* Enable index of alpha RAM B */
            
            Out_SEQ_Reg( 0x4F, bB ); /* RAM data port */
        }
        break;
    }
    
    Out_SEQ_Reg( 0x49, bData );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Rotate RAM, 0x0F -> 0x0E -> 0x0D ... -> 0x01 -> 0x00 -> 0x0F
 in:  None
 ****************************************************************************/
void RotateAlphaRAMReg(void)
{
    u8 bR, bG, bB;
    u8 bR0, bG0, bB0;
    u8 bData;
    u16 i;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    /* Read RGB from index 0 */
    bData = In_SEQ_Reg( 0x49 );
    Out_SEQ_Reg( 0x49, 0x18 ); /* Select CPU to write */
    
    Out_SEQ_Reg( 0x4E, 0x20 ); /* Enable index of alpha RAM R */
    bR0= In_SEQ_Reg( 0x4F );   /* RAM data port */
    
    Out_SEQ_Reg( 0x4E, 0x40 ); /* Enable index of alpha RAM G */
    bG0 = In_SEQ_Reg( 0x4F );  /* RAM data port */
    
    Out_SEQ_Reg( 0x4E, 0x80 ); /* Enable index of alpha RAM B */
    bB0= In_SEQ_Reg( 0x4F );   /* RAM data port */
    
    for ( i = 0; i < 16; i++ )
    {
        Out_SEQ_Reg( 0x4E, 0x20 + i + 1 ); /* Enable index of alpha RAM R */
        bR = In_SEQ_Reg( 0x4F ); /* RAM data port */
        
        Out_SEQ_Reg( 0x4E, 0x40 + i + 1 ); /* Enable index of alpha RAM G */
        bG = In_SEQ_Reg( 0x4F ); /* RAM data port */
        
        Out_SEQ_Reg( 0x4E, 0x80 + i + 1 ); /* Enable index of alpha RAM B */
        bB = In_SEQ_Reg( 0x4F ); /* RAM data port */
        
        if ( i != 15 )
        {
            Out_SEQ_Reg( 0x4E, 0x20 + i ); /* Enable index of alpha RAM R */
            Out_SEQ_Reg( 0x4F, bR );       /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x40 + i ); /* Enable index of alpha RAM G */
            Out_SEQ_Reg( 0x4F, bG );       /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x80 + i ); /* Enable index of alpha RAM B */
            Out_SEQ_Reg( 0x4F, bB );       /* RAM data port */
        }
        else
        {
            Out_SEQ_Reg( 0x4E, 0x20 + i ); /* Enable index of alpha RAM R */
            Out_SEQ_Reg( 0x4F, bR0 );       /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x40 + i ); /* Enable index of alpha RAM G */
            Out_SEQ_Reg( 0x4F, bG0 );      /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x80 + i ); /* Enable index of alpha RAM B */
            Out_SEQ_Reg( 0x4F, bB0 );      /* RAM data port */
            
            /* Read RGB from index 0 */
            Out_SEQ_Reg( 0x4E, 0x20 ); /* Enable index of alpha RAM R */
            bR0 = In_SEQ_Reg( 0x4F );   /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x40 ); /* Enable index of alpha RAM G */
            bG0 = In_SEQ_Reg( 0x4F );  /* RAM data port */
            
            Out_SEQ_Reg( 0x4E, 0x80 ); /* Enable index of alpha RAM B */
            bB0 = In_SEQ_Reg( 0x4F );  /* RAM data port */
        }
    }
    
    Out_SEQ_Reg( 0x49, bData );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 set RGB value into one of the Alpha look up table (RAM)
 in:   bIndex = 0 - F: RAM index
       bR, bG, bB are RGB value to that RGB registers.
 Note: after calling this function, the look up table index is also set to 
       bIndex.
 ****************************************************************************/
void SetAlphaRAMReg(u8 bIndex, u8 bR, u8 bG, u8 bB)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    bData = In_SEQ_Reg( 0x49 );
    Out_SEQ_Reg( 0x49, 0x18 ); /* Select CPU to write */
    
    Out_SEQ_Reg( 0x4E, 0x20 + bIndex ); /* Enable index of alpha RAM R */
    
    Out_SEQ_Reg( 0x4F, bR ); /* RAM data port */
    
    Out_SEQ_Reg( 0x4E, 0x40 + bIndex ); /* Enable index of alpha RAM G */
    
    Out_SEQ_Reg( 0x4F, bG ); /* RAM data port */
    
    Out_SEQ_Reg( 0x4E, 0x80 + bIndex ); /* Enable index of alpha RAM B */
    
    Out_SEQ_Reg( 0x4F, bB ); /* RAM data port */
    
    Out_SEQ_Reg( 0x49, bData );
    
    Out_SEQ_Reg( 0x4E, bIndex ); /* Set index of alpha RAM */
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Read RGB value from one of the Alpha look up table (RAM)
 in:  bIndex = 0 - F: RAM index
 out: 32 bit value xRGB with B as low 8 bits, x don't care
 ****************************************************************************/
u32 ReadAlphaRAMReg(u8 bIndex)
{
    u32 dwData;
    u8 bData, bR, bG, bB;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    bData = In_SEQ_Reg( 0x49 );
    Out_SEQ_Reg( 0x49, 0x18 ); /* Select CPU to write */
    
    Out_SEQ_Reg( 0x4E, 0x20 + bIndex ); /* Enable index of alpha RAM R */
    
    bR = In_SEQ_Reg( 0x4F ); /* RAM data port */
    
    Out_SEQ_Reg( 0x4E, 0x40 + bIndex ); /* Enable index of alpha RAM G */
    
    bG = In_SEQ_Reg( 0x4F ); /* RAM data port */
    
    Out_SEQ_Reg( 0x4E, 0x80 + bIndex ); /* Enable index of alpha RAM B */
    
    bB = In_SEQ_Reg( 0x4F ); /* RAM data port */
    
    Out_SEQ_Reg( 0x49, bData );
    
    dwData = ((u32) bR << 16) + ((u32) bG << 8) + (u32) bB;
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
    
    return (dwData);
}

/****************************************************************************
 Enable the functionality that any one of Alpha look up table (RAM) can be 
 used as alpha register.
 in:   OnOff: ON - enable this function.
             OFF - disable it.
 out:  none
 Note: 1. It is similar to the alpha register used in SetAlphaReg().
       2. When enabled, we have two alpha registers sources for one alpha 
          blending window. It is useful when used with magic number alpha 
          blending.
       3. SetAlphaRAMReg() should be used to set the indexed RAM register.
 ****************************************************************************/
void EnableSWIndexToRAM(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if (OnOff == ON)
    {
        Out_SEQ_Reg_M( 0x4A, 0x20, 0xDF ); /* S/W select RAM index */
    }
    else
    {
        Out_SEQ_Reg_M( 0x4A, 0x00, 0xDF );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Invert source A or B to be blended, alpha value, blended results
 in:   bInvtSrcA   - 1: invert Source A, 0: not invert
       bInvtSrcB   - 1: invert Source B, 0: not invert
       bInvtAlpha  - 1: invert Alpha value, 0: not invert
       bInvtResult - 1: invert final results, 0: not invert
 Note: First three parameters are used to apply to the data before alpha 
       blending process, while the last parameter alpplies to the data after
       alpha blending process.
 ****************************************************************************/
void InvertFunction(u8 bInvtSrcA, u8 bInvtSrcB, u8 bInvtAlpha, u8 bInvtResult)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( bInvtSrcA )
    {
        Out_SEQ_Reg_M( 0x49, 0x04, 0xFB );
    }
    else
    {
        Out_SEQ_Reg_M( 0x49, 0x00, 0xFB );
    }
    
    if ( bInvtSrcB )
    {
        Out_SEQ_Reg_M( 0x4D, 0x40, 0xBF );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4D, 0x00, 0xBF );
    }
    
    if ( bInvtAlpha )
    {
        Out_SEQ_Reg_M( 0x49, 0x80, 0x7F );
    }
    else
    {
        Out_SEQ_Reg_M( 0x49, 0x00, 0x7F );
    }
    
    if ( bInvtResult )
    {
        Out_SEQ_Reg_M( 0x4A, 0x04, 0xFB );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4A, 0x00, 0xFB );
    }
    
    Out_Video_Reg( 0xFA, 0x10 );
    
    if ( bInvtResult )
    {
        Out_SEQ_Reg_M( 0x4D, 0x80, 0x7F );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4D, 0x00, 0x7F );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Invert whole video windows
 in:   bInvtVideo - 1: invert Source A, 0: not invert
 Note: Enabled video window/windows will be entirely inverted.
       i.e. if only one video window is enabled, that video window will be 
       entirely inverted.
 ****************************************************************************/
void InvertVideo(u8 bInvtVideo)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( bInvtVideo )
    {
        Out_SEQ_Reg_M( 0x4A, 0x04, 0xFB );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4A, 0x00, 0xFB );
    }
    
    Out_Video_Reg( 0xFA, 0x10 );
    
    if ( bInvtVideo )
    {
        Out_SEQ_Reg_M( 0x4D, 0x00, 0x7F );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Enable or Disable Magic number functionality
 in:   OnOff: ON  - enable magic number function
              OFF - disable magic number function
       bGeneral: 0        - Magic number only works for Win0
                 non-zero - Magic number works for all four alpha windows
 Note: 1. Magic number function will be very useful in the case of video + 
          graphics alpha blending with an important requirement that TEXT of
          graphics source will be solid while the other graphics will do 
          alpha blending with video, i.e, TEXT has no alpha blending at all 
          so that the TEXT is clearly visible.
       2. Basically, magic number function is just a h/w implementation of 
          two alpha sources.
 ****************************************************************************/
void EnableMagicAlphaBlend(u8 OnOff, u8 bGeneral)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x08 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x46, 0x01, 0xFE );
    }
    else
    {
        Out_SEQ_Reg_M( 0x46, 0x00, 0xFE );
    }
    
    if ( bGeneral == 0 )
    {
        /* Magic color feature will be only suitable for
         * alpha Win0.
         */
        Out_Video_Reg( 0xFA, 0x20 );
        Out_SEQ_Reg_M( 0x47, 0x80, 0x7F );
    }
    else
    {
        /* Magic color feature will be suitable for all
         * four alpha windows.
         */
        Out_Video_Reg( 0xFA, 0x20 );
        Out_SEQ_Reg_M( 0x47, 0x00, 0x7F );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Enable or Disable graphics replacement for magic number. Basically, when 
 magic number matches, the original graphics data can be replaced by RGB 
 values to do alpha blending, i.e not the original graphics data, instead, 
 the replacement RGB will get into alpha blending process.
 in:   ON - enable the replacement function
       OFF - disable the replacement function
 Note: if enabled, application should also call SetMagicReplaceReg() to set
       replacement RGB values.
 ****************************************************************************/
void EnableMagicReplace(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x08 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if (OnOff == ON)
    {
        Out_SEQ_Reg_M( 0x46, 0x10, 0xEF );
    }
    else
    {
        Out_SEQ_Reg_M( 0x46, 0x00, 0xEF );
    }
    
    MagRepON = 0;
    Out_Video_Reg( 0xFA, 0x58 );
    if (OnOff == ON)
    {
        Out_SEQ_Reg_M(0x4E, 0x00, 0x3F);
        MagRepON = 1;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Choose the alpha source for the magic number funtionality
 in:   bAlphaSrc: Alpha data source if Magic number matches
                  Alpha_Int = 0 - Alpha is from interleave alpha embeded in
                                   graphics data. It only applies to 32 bpp:
                                   8888
                  Alpha_V2  = 1 - Alpha is from video two map
                  Alpha_Ram = 2 - Alpha is from look up table (RAM)
                  Alpha_Reg = 3 - Alpha is from register
 Note: 1. This function is very similar to SelectAlphaSrc()
       2. !!! if video 2 selected as alpha map:
          1) video 2 must be enabled as normal (3c4_dc<7>=1)
          2) video 2 window size = alpha window size.
 ****************************************************************************/
void SelectMagicAlphaSrc(u8 bAlphaSrc)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x08 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    Out_SEQ_Reg_M( 0x46, ( bAlphaSrc << 2 ), 0xF3 );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Set Magic number registers
 If Magic number matches the graphics source contents,
     1) one extra alpha source can be selected see SelectMagicAlphaSrc().
     2) a RGB replacement for original graphics contents can be activated
        see SetMagicReplaceReg().
 in:   bR - Red 8 bits
       bG - Green 8 bits
       bB - Blue  8 bits
 Note: . At 256 color mode, input data should be bR=bG=bB.
       . At 16 Bpp graphics mode, input data should be
         bR = R & 0xF8
         bG = G & 0xFC
         bB = B & 0xF8
         which is RGB565 format.
 ****************************************************************************/
void SetMagicMatchReg(u8 bR, u8 bG, u8 bB)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x08 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    /* Disable range feature first */
    Out_SEQ_Reg_M( 0x46, 0x00, 0x7F );
    
    Out_SEQ_Reg( 0x40, bR );
    Out_SEQ_Reg( 0x41, bG );
    Out_SEQ_Reg( 0x42, bB );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Set Magic number register range
 If Magic number range matches the graphics source contents,
     1) one extra alpha source can be selected see SelectMagicAlphaSrc().
     2) a RGB replacement for original graphics contents can be activated
        see SetMagicReplaceReg().
 in:   bRLow, bGLow, bBLow  - Red/Green/Blue low value
       bRHigh,bGHigh,bBHigh - Red/Green/Blue high value
 Note: . At 256 color mode, input data should be bR=bG=bB.
       . At 16 Bpp graphics mode, input data should be
         bR = R & 0xF8
         bG = G & 0xFC
         bB = B & 0xF8
         which is RGB565 format.
       . This function in fact is a super set of SetMagicMatchReg(), if you
         set all values Low value = High value, they are same.
       . When you are doing alpha-blending under SyncLock mode, you should
         provide a YUV range instead of provide a unique RGB value for magic 
         color. So, first you need convert your maglic color from RGB to YUV,
         then expand this value to a range. The parameter sequence here 
         changes to VYU, that is (bVLow, bYLow, bULow, bVHigh, vYHigh, 
         bUHight).
 ****************************************************************************/
void SetMagicMatchRegRange(u8 bRLow, u8 bGLow, u8 bBLow, u8 bRHigh, u8 bGHigh, u8 bBHigh)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x08 ); 
    Out_Video_Reg( 0xF7, 0x00 );   
    
    /* Enable range feature first */
    Out_SEQ_Reg_M( 0x46, 0x80, 0x7F );
    
    Out_SEQ_Reg( 0x40, bRLow );
    Out_SEQ_Reg( 0x41, bGLow );
    Out_SEQ_Reg( 0x42, bBLow );
    
    Out_SEQ_Reg( 0x4B, bRHigh );
    Out_SEQ_Reg( 0x4C, bGHigh );
    Out_SEQ_Reg( 0x4D, bBHigh );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Set policy of how to do magic number alpha blending.
 in: wMatch - MATCH   = 0: when magic number matches graphics contents, the 
                           matched contents do magic number alpha blending 
                           and unmatched contents will do normal alpha 
                           blending.
            - NOMATCH = 1: when magic number does not match graphics 
                           contents the unmatched contents do magic number 
                           alpha blending and matched contents will do 
                           normal alpha blending.
 ****************************************************************************/
void SetMagicPolicy(u16 wMatch)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x08 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( wMatch == NOMATCH )
    {
        Out_SEQ_Reg_M( 0x46, 0x02, 0xFD );
    }
    else
    {
        Out_SEQ_Reg_M( 0x46, 0x00, 0xFD );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Set replacement RGB registers to replace original graphics contents to do 
 alpha blending.
 in: bR - Red 8 bits
     bG - Green 8 bits
     bB - Blue 8 bits
 ****************************************************************************/
void SetMagicReplaceReg(u8 bR, u8 bG, u8 bB)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x08 );
    
    Out_SEQ_Reg( 0x43, bR );
    Out_SEQ_Reg( 0x44, bG );
    Out_SEQ_Reg( 0x45, bB );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 SelectAlphaSrc2FromRam() will allow you to set one alpha-blending window 
 alpha source come from look up table while all three others come from alpha 
 register.
 in:   bWhichWindow - specify which alpha-blending window will use look up 
                      table as alpha source.
 Note: all other 3 alpha windows alpha source must come from register
 ****************************************************************************/
void SelectAlphaSrc2FromRam(u8 bWhichWindow)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    Out_SEQ_Reg_M( 0x49, 0x60, 0x9F );
    
    Out_Video_Reg( 0xFA, 0x18 );
    
    Out_SEQ_Reg_M( 0x4F, ( 0x04 | bWhichWindow ), 0xF8 );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}


/****************************************************************************
 EnablePaletteMode() can Enable/Disable graphic selecting after RAM PALETTE
 in:   bEnalbe = 1, enable this mode
                 0, disable it.
 Note: 8-bit graphics mode is different from 16-bit/24-bit mode, it will use 
       a color palette. So, for alpha-blending under 8-bit graphics mode, we
       need call EnablePaletteMode() to make alpha-blending work fine. 
       Remember to disable it when you quit from alpha-blending. The routine 
       EnablePaletteMode() will do it if we are under this mode
 ****************************************************************************/
void EnablePaletteMode(u8 bEnable)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    if ( bEnable )
    {
        Out_Video_Reg_M( 0x5c, 0x20, 0xDF ); /* Turn on TV */
        
        bEnabled8Bit = 1;
        OutTVRegM( 0xE60C, 0x12, 0xED );
        
#if 0
        /* nav: This bit for SCART, why we turn it on? */
        Out_Video_Reg( 0xFA, 0x05 );
        Out_Video_Reg_M( 0x4E, 0x10, 0xEF );
#endif
        Out_Video_Reg( 0xFA, 0x10 );
        Out_SEQ_Reg_M( 0x4F, 0x81, 0x7E );
        
        Out_Video_Reg( 0xFA, 0x28 );
        Out_SEQ_Reg_M( 0x4C, 0x80, 0x7F );
        
        Out_Video_Reg( 0xFA, 0x20 );
        Out_SEQ_Reg_M( 0x47, 0x10, 0xEF );
        Out_SEQ_Reg_M( 0xDC, 0x01, 0xFE );
        
        Out_Video_Reg_M( 0xDC, 0x01, 0xFE );
        Out_Video_Reg( 0xFA, 0x20 );
        Out_SEQ_Reg( 0x40, 0x00 ); /* Cusor color 1 R,G,B */
        Out_SEQ_Reg( 0x41, 0x00 ); /* The color for cusor board */
        Out_SEQ_Reg( 0x42, 0x00 ); /* Default is black */
        Out_SEQ_Reg( 0x43, 0xFF ); /* Cusor color 2 R,G,B */
        Out_SEQ_Reg( 0x44, 0xFF ); /* The color inside the cusor */
        Out_SEQ_Reg( 0x45, 0xFF ); /* Default is white */
        Out_SEQ_Reg( 0x46, 0x01 ); /* Enable h/w cusor */
    }
    else
    {
        if ( (In_Video_Reg( 0x5C ) & 0x20) != 0x20 )
        {
            Out_Video_Reg_M( 0x5C, 0x20, 0xDF );
        }
        bEnabled8Bit = 0;
        OutTVRegM( 0xE60C, 0x0000, 0xFFED );
        
        Out_Video_Reg( 0xFA, 0x05 );
        Out_Video_Reg_M( 0x4E, 0x00, 0xEF );
        
        /* To avoid overide digital out settings, do not clear 
         * the following register bits if digital out is on. */
#if 0 //#if !( DIGITAL_OUT )
        Out_Video_Reg( 0xFA, 0x10 );
        Out_SEQ_Reg_M( 0x4F, 0x00, 0x7E );
        
        Out_Video_Reg( 0xFA, 0x28 );
        Out_SEQ_Reg_M( 0x4C, 0x00, 0x7F );
        Out_Video_Reg( 0xFA, 0x20 );
        Out_SEQ_Reg_M( 0x47, 0x00, 0xEF );
#endif
        
        Out_Video_Reg( 0xFA, 0x20 );
        Out_SEQ_Reg_M( 0xDC, 0x00, 0xFE );
        Out_Video_Reg_M( 0xDC, 0x00, 0xFE );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableSLockAlphaBlend() will enable alpha-blending under SyncLock mode
 in:   bEnable : 1 - Enable alpha-blending under SyncLock mode
                 0 - Disable it
       wTVMode : current TV output mode, should be NTSC or PAL
       wModenum: current resolution mode, should be,
                 0 - NTSC 640X480
                 1 - NTSC 720X480
                 2 - NTSC 640X440
                 0 - PAL 640X480
                 1 - PAL 720X540
                 2 - PAL 768X576
                 3 - PAL 720X576
 NOTE: 1.SyncLock must be turned on before call this function
       2.If you want to implement alpha-blending under SyncLock mode, you 
         should not call SelectBlendSrcA/SelectBlendSrcB to select sources,
         instead, this function call will specify sources for alpha-blending.
         In fact, there are only two sources avaliable, we set source A to 
         be General Graphics and source B to be SyncLock video.(General 
         Graphics here means everything as long as the data are come from 
         frame buffer, compared to SyncLock video which is bypassed directly
         from video port to TV set. So, General Graphics includes normal 
         graphics data, Overlay 1 & 2 
 ****************************************************************************/
void EnableSLockAlphaBlend(u8 bEnable, u16 wTVMode, u16 wModenum)
{
    u8 b3CE_F7, b3CE_FA;
    
    
    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( bEnable )
    {
        Out_Video_Reg( 0xFA, 0x00 );
        /* Source B comes from Sync Lock video */
        Out_SEQ_Reg_M( 0x4A, 0x08, 0xF7 );
        /* Source A from tranitional Graphics */
        Out_SEQ_Reg_M( 0x49, 0x00, 0xFC );
        /* Soruce A from TV graphics feed back */
        Out_SEQ_Reg_M( 0x4C, 0x40, 0xBF );
        
        Out_Video_Reg( 0xFA, 0x08 );
        /* Invert u v color */
        Out_SEQ_Reg( 0x47, 0x28 );
        /* Graphics data passthrough, allow Overlay(1,2) coming in */
        Out_SEQ_Reg( 0x4F, 0x11 );
        
        Out_Video_Reg( 0xFA, 0x10 );
        /* Graphic from tv */
        Out_SEQ_Reg_M( 0x4F, 0x00, 0x7E );
        
        /* Set alpha base */
        SetAlphaBase( wTVMode, wModenum );
    }
    else
    {
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg( 0x47, 0x00 );
        Out_SEQ_Reg( 0x4F, 0xD1 );
        Out_Video_Reg( 0xFA, 0x00 );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 SetAlphaMapPtr() sets a pointer to alpha map and a number of 8bit(BPP)-line
 before alpha map.
 In:  dwAlphamapPtr - a pointer to alpha map.
 Out: None
 ****************************************************************************/
void SetAlphaMapPtr(u32 dwAlphamapPtr)
{
    dwTop8BitLines = (dwAlphamapPtr - (u32)fb_mem_addr +
        (u32)x_res - 1)/(u32)x_res;
    dwAlphaMapPtr = (u32)fb_mem_addr + dwTop8BitLines * (u32)x_res;
}

/****************************************************************************
 GetAlphaMapPtr() gets a pointer to alpha map.
 In:  None
 Out: A pointer to alpha map
 ****************************************************************************/
u32 GetAlphaMapPtr(void)
{
    return (dwAlphaMapPtr);
}

/****************************************************************************
 SetAlphaBase() detects clock and sync settings automatically and sets alpha 
 base accordingly. You have to call EnableSLockAlphaBlend() first before 
 calling this function.
 in:   wTVMode : current TV output mode, should be NTSC or PAL
       wModenum: current resolution mode, should be,
                 0 - NTSC 640X480
                 1 - NTSC 720X480
                 2 - NTSC 640X440
                 0 - PAL 640X480
                 1 - PAL 720X540
                 2 - PAL 768X576
                 3 - PAL 720X576
 out:  none
 ****************************************************************************/
void SetAlphaBase(u8 TVmode, u8 Modenum)
{
    u16 wAlphaBaseXStart, wAlphaBaseXEnd;
    u16 wAlphaBaseYStart, wAlphaBaseYEnd;
    u16 wHTotal, wHrstart, wHsld, wE468, wE528, wMhsdly;
    u16 wVTotal, wVSld;
    u16 wResolutionX, wResolutionY;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    
    /* X */
    wHTotal  = In_CRT_Reg( 0x00 );
    wHTotal |= ( (u16)In_CRT_Reg( 0x53 ) & 0x01 ) << 8;
    
    wHrstart  = In_CRT_Reg( 0x04 );
    wHrstart |= ( (u16)In_CRT_Reg( 0x53 ) & 0x04 ) << 6;
    
    wE468 = InTVReg( 0xE468 ) & 0x3ff;
    wE528 = InTVReg( 0xE528 ) & 0x3ff;
    
    wHsld  = In_CRT_Reg( 0x28 );
    wHsld |= ( (u16)In_CRT_Reg( 0x29 ) & 0x01 ) << 8;
    
    wMhsdly  = In_SEQ_Reg( 0xEA );
    wMhsdly |= ( (u16)In_SEQ_Reg( 0xEB ) & 0x07 ) << 8;
    
    if ( TVmode == NTSC )
    {
        switch ( Modenum )
        {
        case 0:
        case 2:
        default:
            wResolutionX = 640;
            break;
        case 1:
            wResolutionX = 720;
            break;
        }
    }
    else /*PAL mode*/
    {
        switch (Modenum)
        {
        case 0:
        default:
            wResolutionX = 640;
            break;
        case 1:
        case 3:
            wResolutionX = 720;
            break;
        case 2:
            wResolutionX = 768;
            break;
        case 4:
            wResolutionX = 976;
        }
    }
    
    wAlphaBaseXStart = 2 * ( 8 * ( wHTotal - wHsld ) + 0x3C ) - 
    ( 8 * ( wHrstart - wHsld + 2 ) - 1 ) -
    4 * wE468 + 2 * wE528 + 0x1C - 0x14 - wMhsdly;
    
    wAlphaBaseXEnd = wAlphaBaseXStart + wResolutionX * 2;
    
    /* Y */
    wVTotal  = In_CRT_Reg( 0x06 );
    wVTotal |= ( (u16)In_CRT_Reg( 0x07 ) & 0x01 ) << 8;
    wVTotal |= ( (u16)In_CRT_Reg( 0x07 ) & 0x20 ) << 4;
    wVTotal |= ( (u16)In_Video_Reg( 0x11 ) & 0x01 ) << 10;
    
    wVSld  = In_CRT_Reg( 0x2A );
    wVSld |= ( (u16)In_CRT_Reg( 0x2B ) & 0x07 ) << 8;
    
    if ( TVmode == NTSC )
    {
        switch ( Modenum )
        {
        case 0:
        case 1:
        default:
            wResolutionY = 480;
            break;
        case 2:
            wResolutionY = 440;
            break;
        }
    }
    else /*PAL mode*/
    {
        switch (Modenum)
        {
        case 0:
        default:
            wResolutionY = 480;
            break;
        case 1:
            wResolutionY = 540;
            break;
        case 2:
        case 3:
            wResolutionY = 576;
            break;
        }
    }
    wAlphaBaseYStart = ( wVTotal - wVSld + 8 ) / 2;
    if ( NTSC == TVmode )
    {
        ++ wAlphaBaseYStart;
    }
    
    wAlphaBaseYEnd = wAlphaBaseYStart + wResolutionY / 2;
    if      ( PAL == TVmode && wAlphaBaseYEnd > 0x137 )
    {
        wAlphaBaseYEnd = 0x138;
    }
    else if ( NTSC == TVmode && wAlphaBaseYEnd > 0x106 )
    {
        wAlphaBaseYEnd = 0x106;
    }
    
    /* Set alpha base registers */
    /* Switch register bank */
    Out_Video_Reg( 0xFA, 0x80 );
    /* Set alpha base vertical position */
    Out_Video_Reg( 0xE2, (u8)(wAlphaBaseYStart & 0x00FF) );
    Out_Video_Reg( 0xE3, (u8)( ( wAlphaBaseYStart >> 8 ) & 0x07 ) |
        (u8)( ( wAlphaBaseYEnd >> 4 ) & 0x70 ) );
    Out_Video_Reg( 0xE4, (u8)(wAlphaBaseYEnd & 0x00FF) );
    
    /* Switch register bank */
    Out_Video_Reg( 0xFA, 0x00 );
    /* Set alpha base horizontal position */
    Out_Video_Reg( 0x4A, (u8)(wAlphaBaseXStart & 0x00FF) );
    Out_Video_Reg( 0x4B, (u8)( ( wAlphaBaseXStart >> 8 ) & 0x07 ) |
        (u8)( ( wAlphaBaseXEnd >> 4 ) & 0x70 ) );
    Out_Video_Reg( 0x4C, (u8)(wAlphaBaseXEnd & 0x00FF) );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableExtendedMagicAlpha() enables/disables CP52xx extended magic number
 features.
 in : OnOff    - ON  : enable
                 OFF : disable
      b1On1    - 1: each magic number only matches in corresponding alpha window
                 0: four magic numbers match in all four alpha window
      bByColor - 0: four match alpha values assigned by corresponding window
                 1: four match alpha values assigned by corresponding magic number
 out: none
 Note: 1. CP52xx supported extended magic number feature, which is four magic 
          numbers instead of the original one magic number.
       2. For interlace alpha-blending, One-on-one or By-Color mode doesn't work, 
          b1On1 and bByColor will be ignored. All used magic numbers work for all 
          used alpha windows and active match alpha value is associated with each
          alpha window.
 ****************************************************************************/
void EnableExtendedMagicAlpha(u8 OnOff, u8 b1On1, u8 bByColor)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x20 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x4F, 0x08, 0xF7 );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4F, 0x00, 0xF7 );
    }
    
    if ( b1On1 )
    {
        Out_SEQ_Reg_M( 0x4F, 0x04, 0xFB );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4F, 0x00, 0xFB );
    }
    
    if( bByColor )
    {
        Out_SEQ_Reg_M( 0x4F, 0x02, 0xFD );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4F, 0x00, 0xFD );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableWhichMagicNumber() enables/disables magic number functionality
 individually.
 in : OnOff        - ON  : enable 
                     OFF : disable
      bWhich       - (MAGIC0 - MAGIC3)
 ****************************************************************************/
void EnableWhichMagicNumber(u8 OnOff, u8 bWhich)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    bData = 0x01;
    if ( bWhich == MAGIC1 )
    {
        bData <<= 1;
    }
    else if ( bWhich == MAGIC2 )
    {
        bData <<= 2;
    }
    else if ( bWhich == MAGIC3 )
    {
        bData <<= 3;
    }
    
    Out_Video_Reg( 0xFA, 0x48 );
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x4D, 0x00, ~bData );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4D, bData, ~bData );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableMagicNumberInWhichWindow() enables/disables magic number functionality,
 which is associated with an alpha-blending window.
 in : OnOff        - ON  : enable 
                     OFF : disable
      bWhichWindow - (Win0 - Win3)
 ****************************************************************************/
void EnableMagicNumberInWhichWindow(u8 OnOff, u8 bWhichWindow)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    bData = 0x10;
    if ( bWhichWindow == Win1 )
    {
        bData <<= 1;
    }
    else if ( bWhichWindow == Win2 )
    {
        bData <<= 2;
    }
    else if ( bWhichWindow == Win3 )
    {
        bData <<= 3;
    }
    
    Out_Video_Reg( 0xFA, 0x20 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x48, 0x00, ~bData );
    }
    else
    {
        Out_SEQ_Reg_M( 0x48, bData, ~bData );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 SetMagicMatchRegExt() sets magic number registers for individual magic number
 in :  bR, bG, bB   - magic number RGB value
       bWhich - (MAGIC0 - MAGIC3)
 out: none
 Note: . At 256 color mode, input data should be bR=bG=bB.
       . At 16 Bpp graphics mode, input data should be
         bR = R & 0xF8
         bG = G & 0xFC
         bB = B & 0xF8
         which is RGB565 format.
****************************************************************************/
void SetMagicMatchRegExt(u8 bR, u8 bG, u8 bB, u8 bWhich)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    bData = 0x01;
    if ( bWhich == MAGIC1 )
    {
        bData <<= 1;
    }
    else if ( bWhich == MAGIC2 )
    {
        bData <<= 2;
    }
    else if ( bWhich == MAGIC3 )
    {
        bData <<= 3;
    }
    
    Out_Video_Reg( 0xFA, 0x08 );
    
    /* Disable range feature first */
    Out_SEQ_Reg_M( 0x46, 0x00, 0x7F );
    
    Out_Video_Reg( 0xFA, 0x48 );
    Out_SEQ_Reg_M( 0x4C, 0x00, ~bData );
    
    switch( bWhich )
    {
    case MAGIC0:
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg( 0x40, bR );
        Out_SEQ_Reg( 0x41, bG );
        Out_SEQ_Reg( 0x42, bB );
        break;
        
    case MAGIC1:
        Out_Video_Reg( 0xFA, 0x48 );
        Out_SEQ_Reg( 0x40, bR );
        Out_SEQ_Reg( 0x41, bG );
        Out_SEQ_Reg( 0x42, bB );
        break;
        
    case MAGIC2:
        Out_Video_Reg( 0xFA, 0x28 );
        Out_SEQ_Reg( 0x46, bR );
        Out_SEQ_Reg( 0x47, bG );
        Out_SEQ_Reg( 0x48, bB );
        break;
        
    case MAGIC3:
        Out_Video_Reg( 0xFA, 0x28 );
        Out_SEQ_Reg( 0x49, bR );
        Out_SEQ_Reg( 0x4A, bG );
        Out_SEQ_Reg( 0x4B, bB );
        break;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 SetMagicMatchRegRangeExt() sets magic number register range for individual 
 magic number
 in : bRLow, bGLlw, bBLow    - magic number renge RGB low value 
      bRHigh, bGHigh, bBHigh - magic number renge RGB high value 
      bWhich           - (MAGIC0 - MAGIC3) 
 out: none
 Note: . At 256 color mode, input data should be bR=bG=bB.
       . At 16 Bpp graphics mode, input data should be
         bR = R & 0xF8
         bG = G & 0xFC
         bB = B & 0xF8
         which is RGB565 format.
       . This function in fact is a super set of SetMagicMatchRegExt(), if 
         you set all values Low value = High value, they are same.
       . When you are doing alpha-blending under SyncLock mode, you should
         provide a YUV range instead of provide a unique RGB value for magic 
         color. So, first you need convert your maglic color from RGB to YUV,
         then expand this value to a range. The parameter sequence here 
         changes to VYU, that is (bVLow, bYLow, bULow, bVHigh, vYHigh, 
         bUHight).
****************************************************************************/
void SetMagicMatchRegRangeExt(u8 bRLow, u8 bGLow, u8 bBLow, u8 bRHigh, 
    u8 bGHigh, u8 bBHigh, u8 bWhich)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    bData = 0x01;
    if ( bWhich == MAGIC1 )
    {
        bData <<= 1;
    }
    else if ( bWhich == MAGIC2 )
    {
        bData <<= 2;
    }
    else if ( bWhich == MAGIC3 )
    {
        bData <<= 3;
    }
    
    Out_Video_Reg( 0xFA, 0x08 );
    
    /*Enable range feature first*/
    Out_SEQ_Reg_M( 0x46, 0x80, 0x7F );
    
    Out_Video_Reg( 0xFA, 0x48 );
    
    Out_SEQ_Reg_M( 0x4C, bData, ~bData );
    
    switch(bWhich)
    {
    case MAGIC0:
        Out_Video_Reg( 0xFA, 0x08 );
        
        Out_SEQ_Reg( 0x40, bRLow );
        Out_SEQ_Reg( 0x41, bGLow );
        Out_SEQ_Reg( 0x42, bBLow );
        
        Out_SEQ_Reg( 0x4B, bRHigh );
        Out_SEQ_Reg( 0x4C, bGHigh );
        Out_SEQ_Reg( 0x4D, bBHigh );
        break;
        
    case MAGIC1:
        Out_Video_Reg( 0xFA, 0x48 );
        
        Out_SEQ_Reg( 0x40, bRLow );
        Out_SEQ_Reg( 0x41, bGLow );
        Out_SEQ_Reg( 0x42, bBLow );
        
        Out_SEQ_Reg( 0x43, bRHigh );
        Out_SEQ_Reg( 0x44, bGHigh );
        Out_SEQ_Reg( 0x45, bBHigh );
        break;
        
    case MAGIC2:
        Out_Video_Reg( 0xFA, 0x28 );
        
        Out_SEQ_Reg( 0x46, bRLow );
        Out_SEQ_Reg( 0x47, bGLow );
        Out_SEQ_Reg( 0x48, bBLow );
        
        Out_Video_Reg( 0xFA, 0x48 );
        
        Out_SEQ_Reg( 0x46, bRHigh );
        Out_SEQ_Reg( 0x47, bGHigh );
        Out_SEQ_Reg( 0x48, bBHigh );
        break;
        
    case MAGIC3:
        Out_Video_Reg( 0xFA, 0x28 );
        
        Out_SEQ_Reg( 0x49, bRLow );
        Out_SEQ_Reg( 0x4A, bGLow );
        Out_SEQ_Reg( 0x4B, bBLow );
        
        Out_Video_Reg( 0xFA, 0x48 );
        
        Out_SEQ_Reg( 0x49, bRHigh );
        Out_SEQ_Reg( 0x4A, bGHigh );
        Out_SEQ_Reg( 0x4B, bBHigh );
        break;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 SetMagicMatchAlphaReg() sets Alpha value for the magic number associated 
 with an alpha-blending window into Alpha R, G, B registers, which are also 
 associated with the alpha-blending window.
 in : bR           - Red   8 bits
      bG           - Green 8 bits
      bB           - Blue  8 bits
      bWhichWindow - (Win0 - Win3)
 out: none
 ****************************************************************************/
void SetMagicMatchAlphaReg(u8 bR, u8 bG, u8 bB, u8 bWhichWindow)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( bWhichWindow == Win0 || bWhichWindow == Win1 )
    {
        Out_Video_Reg( 0xFA, 0x20 );
    }
    else
    {
        /* For windows 2,3 */
        Out_Video_Reg( 0xFA, 0x28 );
    }
    
    switch ( bWhichWindow )
    {
    case Win0:
        Out_SEQ_Reg( 0x49, bR );
        Out_SEQ_Reg( 0x4A, bG );
        Out_SEQ_Reg( 0x4B, bB );
        break;
        
    case Win1:
        Out_SEQ_Reg( 0x4C, bR );
        Out_SEQ_Reg( 0x4D, bG );
        Out_SEQ_Reg( 0x4E, bB );
        break;
        
    case Win2:
        Out_SEQ_Reg( 0x40, bR );
        Out_SEQ_Reg( 0x41, bG );
        Out_SEQ_Reg( 0x42, bB );
        break;
        
    case Win3:
        Out_SEQ_Reg( 0x43, bR );
        Out_SEQ_Reg( 0x44, bG );
        Out_SEQ_Reg( 0x45, bB );
        break;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 SetMagicPolicyExt() sets policy of how to do magic number alpha blending for
 an alpha-blending window.
 in : wMatch       - MATCH   = 0: when magic number matches graphics contents, 
                                  the matched contents do magic number alpha 
                                  blending and unmatched contents will do 
                                  normal alpha blending.
                   - NOMATCH = 1: when magic number does not match graphics 
                                  contents the unmatched contents do magic 
                                  number alpha blending and matched contents 
                                  will do normal alpha blending.
      bWhichWindow - (Win0 - Win3)
 out: none
 ****************************************************************************/
void SetMagicPolicyExt(u16 wMatch, u8 bWhichWindow)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    bData = 0x10;
    if ( bWhichWindow == Win1 )
    {
        bData <<= 1;
    }
    else if ( bWhichWindow == Win2 )
    {
        bData <<= 2;
    }
    else if ( bWhichWindow == Win3 )
    {
        bData <<= 3;
    }
    
    Out_Video_Reg( 0xFA, 0x48 );
    if ( wMatch == NOMATCH )
    {
        Out_SEQ_Reg_M( 0x4C, bData, ~bData );
    }
    else
    {
        Out_SEQ_Reg_M( 0x4C, 0x00, ~bData );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

#if INCLUDE_API_2NDALPHA
/*Second layer alpha-blending*/
static u8 b3C4_47;
static u8 b2ndAlphaInited = 0;

void Init2ndAlphaBlend(void)
{
    u8 tmp;
    u8 b3CE_F7, b3CE_FA;

    if ( b2ndAlphaInited )
    {
        return;
    }    
    
    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x08 );
    
    b3C4_47 = In_SEQ_Reg( 0x47 );
    b2ndAlphaInited = 1;
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void Enable2ndAlphaBlend(u8 bOnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x50 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if( bOnOff == ON )
    {
        Out_SEQ_Reg_M(0x4F, 0x80, 0x7F);
    }
    else
    {
        Out_SEQ_Reg_M(0x4F, 0x0, 0x7F);
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void Enable2ndWhichAlphaBlend(u8 bOnOff, u8 bWhichWindow, u8 bFullScreen)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x50 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( bOnOff == ON )
    {
        Out_SEQ_Reg_M( 0x48 , 0x00, ~BIT(bWhichWindow) ) ;
        if ( bFullScreen )
        {
            Out_SEQ_Reg_M( 0x48 , BIT(bWhichWindow+4), ~BIT(bWhichWindow+4) ) ;
        }
        else
        {
            Out_SEQ_Reg_M( 0x48 , 0, ~BIT(bWhichWindow+4) ) ;
        }
    }
    else
    {
        Out_SEQ_Reg_M( 0x48 , BIT(bWhichWindow), ~BIT(bWhichWindow) );
        Out_SEQ_Reg_M( 0x48 , 0, ~BIT(bWhichWindow+4) ) ;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void Select2ndAlphaBlendSrcA(u8 source, u8 bWhichWindow)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    /* The other source (SrcB) is the result of first layer alpha-blending. */
    
    Out_Video_Reg( 0xFA, 0x50 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    switch( bWhichWindow )
    {
    case 0 :
        Out_SEQ_Reg_M( 0x4C, ( source & 0x7 ), ~0x7 );
        break ;
    case 1 :
        Out_SEQ_Reg_M( 0x4C, ( ( source & 0x7 ) << 4 ), ~( 0x7 << 4 ) );
        break ;
    case 2 :
        Out_SEQ_Reg_M( 0x4D, ( source & 0x7 ), ~0x7 );
        break ;
    case 3 :
        Out_SEQ_Reg_M( 0x4D, ( ( source & 0x7 ) << 4 ), ~( 0x7 << 4 ) );
        
        break ;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void Select2ndAlphaBlendSrcB(u8 source)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    switch ( source )
    {
    case SECOND_SRCB_SLOCKALPHA:
        Out_Video_Reg( 0xFA, 0x58 );
        Out_SEQ_Reg_M( 0x4C, 0x80, 0x3F );
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg_M( 0x47, 0x00, ~(BIT(3) | BIT(5)) );
        Out_Video_Reg( 0xFA, 0x50 );
        Out_SEQ_Reg_M( 0x4B, BIT0, ~BIT0 ); 
        Out_Video_Reg( 0xFA, 0x58 );
        Out_SEQ_Reg_M( 0x49, BIT(6), ~BIT(6) );
        break;
    default:
    case SECOND_SRCB_NORMALALPHA:
        Out_Video_Reg( 0xFA, 0x58 );
        Out_SEQ_Reg_M( 0x4C, 0x40, 0x3F );
        break;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void Select2ndAlphaSrc(u8 bAlphaSrc, u8 bWhichWindow)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x58 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    Out_SEQ_Reg_M( 0x45, ( ( bAlphaSrc & 0x3 ) << ( bWhichWindow * 2 ) ), ~( 0x3 << ( bWhichWindow * 2 ) ) );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void Enable2ndAlphaReg(u8 bROn, u8 bGOn, u8 bBOn, u8 bWhichWindow)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x60 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    switch( bWhichWindow )
    {
    case 0:
        Out_SEQ_Reg_M( 0x4C, ( bROn | ( bGOn << 1 ) | ( bBOn << 2 ) ), 0xF8 );
        break ;
    case 1:
        Out_SEQ_Reg_M( 0x4C, ( ( bROn | ( bGOn << 1 ) | ( bBOn << 2 ) ) << 4 ), 0x8F );
        break ;
    case 2:
        Out_SEQ_Reg_M( 0x4D, ( bROn | ( bGOn << 1 ) | ( bBOn << 2 ) ), 0xF8 );
        break ;
    case 3:
        Out_SEQ_Reg_M( 0x4D, ( ( bROn | ( bGOn << 1 ) | ( bBOn << 2 ) ) << 4 ), 0x8F );
        break ;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void Set2ndAlphaReg(u8 bR, u8 bG, u8 bB, u8 bWhichWindow)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x60 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    switch( bWhichWindow )
    {
    case 0 :
        Out_SEQ_Reg( 0x40, bR );
        Out_SEQ_Reg( 0x41, bG );
        Out_SEQ_Reg( 0x42, bB );
        break ;
    case 1:
        Out_SEQ_Reg( 0x43, bR );
        Out_SEQ_Reg( 0x44, bG );
        Out_SEQ_Reg( 0x45, bB );
        break ;
    case 2:
        Out_SEQ_Reg( 0x46, bR );
        Out_SEQ_Reg( 0x47, bG );
        Out_SEQ_Reg( 0x48, bB );
        break ;
    case 3:
        Out_SEQ_Reg( 0x49, bR );
        Out_SEQ_Reg( 0x4A, bG );
        Out_SEQ_Reg( 0x4B, bB );
        break ;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void CleanUp2ndAlphaBlend(void)
{
    u16 index;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x50 );        
    
    for ( index = 0x48; index < 0x50; index++ )
    {
        Out_SEQ_Reg( index, 0x00 );
    }
    
    Out_Video_Reg( 0xFA, 0x58 );
    
    for ( index = 0x40; index < 0x50; index++ )
    {
        Out_SEQ_Reg( index, 0x00 );
    }
    
    Out_Video_Reg( 0xFA, 0x60 );
    
    for ( index = 0x40; index < 0x4E; index++ )
    {
        Out_SEQ_Reg( index, 0x00 );
    }
    
    Out_Video_Reg( 0xFA, 0x68 );
    
    for ( index = 0x40; index < 0x4E; index++ )
    {
        Out_SEQ_Reg( index, 0x00 );
    }
    
    if ( !b2ndAlphaInited )
    {
        /* Restore banking */
        Out_Video_Reg( 0xF7, b3CE_F7 );
        Out_Video_Reg( 0xFA, b3CE_FA );
        
        return;
    }
    Out_Video_Reg( 0xFA, 0x08 );
    Out_SEQ_Reg( 0x47, b3C4_47 );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

#endif /* INCLUDE_API_2NDALPHA */

void InitSLockOverlay(void)
{
    u8 b3CE_F7, b3CE_FA;

    if ( bSLockOverlayInited )
    {
        return;
    }
    
    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    b3C4_4A = In_SEQ_Reg( 0x4A );
    
    Out_Video_Reg( 0xFA, 0x08 );
    b3C4_4F = In_SEQ_Reg( 0x4F );
    
    bSLockOverlayInited = 1;
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void CleanupSLockOverlay(void)
{
    u16 index;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x38 );
    
    for ( index = 0x40; index < 0x4F; index++ )
    {
        Out_SEQ_Reg( index, 0x00 );
    }
    
    Out_Video_Reg( 0xFA, 0x40 );
    for( index = 0x40; index < 0x4A; index++ )
    {
        Out_SEQ_Reg( index, 0x00 );
    }
    
    if( !bSLockOverlayInited )
    {
        /* Restore banking */
        Out_Video_Reg( 0xF7, b3CE_F7 );
        Out_Video_Reg( 0xFA, b3CE_FA );        
        
        return;
    }
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_SEQ_Reg_M( 0x4A, b3C4_4A, 0xF4 );
    Out_Video_Reg( 0xFA, 0x08 );
    Out_SEQ_Reg_M( 0x4F, b3C4_4F, 0x0E );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void SetSLockOverlayWindow(u16 wLeft, u16 wTop, u16 wRight, u16 wBottom, 
    u8 bWhichWindow)
{
    u16 wData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    switch( bWhichWindow )
    {
    case Win0:
        Out_Video_Reg( 0xFA, 0x38 );
        Out_SEQ_Reg(0x40, (u8)(wLeft & 0x00FF));
        Out_SEQ_Reg(0x42, (u8)(wRight & 0x00FF));
        wData = ((wLeft >> 8) & 0x07) + ((wRight >> 4) & 0x70);
        Out_SEQ_Reg_M(0x41, wData, 0x88);
        
        Out_SEQ_Reg(0x43, (u8)(wTop & 0x00FF));
        Out_SEQ_Reg(0x45, (u8)(wBottom & 0x00FF));
        wData = ((wTop >> 8) & 0x07) + ((wBottom >> 4) & 0x70);
        Out_SEQ_Reg_M(0x44, wData, 0x88);
        break;
        
    case Win1:
        Out_Video_Reg( 0xFA, 0x38 );
        Out_SEQ_Reg(0x46, (u8)(wLeft & 0x00FF));
        Out_SEQ_Reg(0x48, (u8)(wRight & 0x00FF));
        wData = ((wLeft >> 8) & 0x07) + ((wRight >> 4) & 0x70);
        Out_SEQ_Reg_M(0x47, wData, 0x88);
        
        Out_SEQ_Reg(0x49, (u8)(wTop & 0x00FF));
        Out_SEQ_Reg(0x4B, (u8)(wBottom & 0x00FF));
        wData = ((wTop >> 8) & 0x07) + ((wBottom >> 4) & 0x70);
        Out_SEQ_Reg_M(0x4A, wData, 0x88);
        break;
        
    case Win2:
        Out_Video_Reg( 0xFA, 0x38 );
        Out_SEQ_Reg(0x4C, (u8)(wLeft & 0x00FF));
        Out_SEQ_Reg(0x4E, (u8)(wRight & 0x00FF));
        wData = ((wLeft >> 8) & 0x07) + ((wRight >> 4) & 0x70);
        Out_SEQ_Reg_M(0x4D, wData, 0x88);
        
        Out_SEQ_Reg(0x4F, (u8)(wTop & 0x00FF));
        WriteReg( 0x3CE , 0xFA , 0x40 ) ;
        Out_SEQ_Reg(0x41, (u8)(wBottom & 0x00FF));
        wData = ((wTop >> 8) & 0x07) + ((wBottom >> 4) & 0x70);
        Out_SEQ_Reg_M(0x40, wData, 0x88);
        break;
        
    case Win3:
        Out_Video_Reg( 0xFA, 0x40 );
        Out_SEQ_Reg(0x42, (u8)(wLeft & 0x00FF));
        Out_SEQ_Reg(0x44, (u8)(wRight & 0x00FF));
        wData = ((wLeft >> 8) & 0x07) + ((wRight >> 4) & 0x70);
        Out_SEQ_Reg_M(0x43, wData, 0x88);
        
        Out_SEQ_Reg(0x45, (u8)(wTop & 0x00FF));
        Out_SEQ_Reg(0x47, (u8)(wBottom & 0x00FF));
        wData = ((wTop >> 8) & 0x07) + ((wBottom >> 4) & 0x70);
        Out_SEQ_Reg_M(0x46, wData, 0x88);
        break;
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void EnableSLockOverlay(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x40 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x48, 0x10, 0xEF );
    }
    else
    {
        Out_SEQ_Reg_M( 0x48, 0x00, 0xEF );
    }
    
    Out_Video_Reg( 0xFA, 0x00 );
    Out_SEQ_Reg_M( 0x4A, 0x03, 0xF4 );
    
    Out_Video_Reg( 0xFA, 0x08 );
    Out_SEQ_Reg_M( 0x4F, 0xC0, 0x0E );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

void EnableWhichSLockOverlay(u8 OnOff, u8 bWhichWindow)
{
    u8 bData;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xFA, 0x40 );
    Out_Video_Reg( 0xF7, 0x00 );
    
    bData = 0x01 << bWhichWindow;
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x48, bData, ~bData );
    }
    else
    {
        Out_SEQ_Reg_M( 0x48, 0, ~bData );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableInterlaceAlpha() is used to enable/disalbe interlace alpha-blending
 in:   bEnable : 1 - Enable interface alpha
                 0 - Disable interface alpha
 NOTE: 1.if one of the alpha-blending sources is interlace source, like Synclock
         video or direct video, the other source has to be interlace source also.
       2.For interlace alpha-blending, EnableInterlaceAlpha() has to be called
         to enable interlace alpha feature.
 ****************************************************************************/
void EnableInterlaceAlpha(u8 bEnable)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x00 );
    
    if ( bEnable )
    {
        Out_CRT_Reg_M( 0xDF, 0x18, 0xE7 ); /* Enable interlace alpha */
        
        /* Turn on G2 */ 
        /*
        Out_Video_Reg( 0xFA, 0x28 );
        Out_SEQ_Reg_M( 0x4C, 0x80, 0x7F ); 
        */
    }
    else
    {
        Out_CRT_Reg_M( 0xDF, 0x00, 0xE7 ); /* Disable interlace alpha */
        
        /* Turn off G2 */ 
        /*
        Out_Video_Reg( 0xFA, 0x28 );
        Out_SEQ_Reg_M( 0x4C, 0x00, 0x7F ); 
        */
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableInterlaceRGBMagicNumber() is used to enable/disalbe MagicColor comparison
 in RGB space for interlace alpha-blending.
 in:   bEnable : 1 - Enable 
                 0 - Disable
 Note: 
 ****************************************************************************/
void EnableInterlaceRGBMagicNumber(u8 bEnable)
{
    u8 bYmDelay = 0x1A;
    u8 b3CE_F7, b3CE_FA;
    
    
    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    Out_Video_Reg( 0xFA, 0x28 );
    if ( In_SEQ_Reg( 0x4C ) & 0x80 )
    {
        bYmDelay -= 0x0C;
    }
    
    Out_Video_Reg( 0xFA, 0x58 );
    
    if ( bEnable )
    {
        if ( !MagRepON )
        {
            Out_SEQ_Reg_M( 0x4E, 0xC0, 0x3F );
        }
        
        /* Some necessary timing adjustment here */
        Out_SEQ_Reg_M( 0x4F, bYmDelay, 0xE0 ); 
        
        Out_Video_Reg( 0xFA, 0x10 );
        Out_SEQ_Reg_M(0x4C, 0x00, 0xF0);
    }
    else
    {
        if ( !MagRepON )
        {
            Out_SEQ_Reg_M( 0x4E, 0x00, 0x3F );
        }
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/**************************************************************************
 EnableDvideoAlphaBlend( ) will enable alpha-blending under direct video mode.
 in:   bEnable : 1 - Enable alpha-blending under direct video mode
                 0 - Disable it
 out:  Null
**************************************************************************/
void EnableDvideoAlphaBlend( u8 bEnable )
{
    u16 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( bEnable )
    {
        Out_Video_Reg( 0xFA, 0x00 );
        Out_SEQ_Reg_M( 0x4C, 0x40, 0xBF ); /* Set Graphics to be TV feedback graphics */
        
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg_M( 0x4F, 0x10, 0xEF ); /* Only graphics send to TV encoder*/
        Out_Video_Reg( 0xFA, 0x00 );
        
        Out_SEQ_Reg_M( 0x4A, 0x00, 0xFC ); /* Invert graphics data which comes from TV encoder U/V bit 7*/
        Out_Video_Reg( 0xFA, 0x08 );
        
        Out_SEQ_Reg_M( 0x47, 0x28, 0xD7 ); /* Invert U/V bit 7 back when finished alpha-blending*/
    }
    else
    {
        Out_Video_Reg( 0xFA, 0x00 );
        Out_SEQ_Reg_M( 0x4C, 0x00, 0xBF );
        
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg_M( 0x4F, 0x00, 0xEF );
        Out_Video_Reg( 0xFA, 0x00 );
        
        Out_SEQ_Reg_M( 0x47, 0x00, 0xD7 );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Choose which one will be used as source A to be alpha blended
 In:
       bSrcA - Source A used for alpha blending
               SrcA_Gr  = 0 - Source A is from orginal graphics
               SrcA_V1  = 1 - Source A is from Overlay one
               SrcA_V2  = 2 - Source A is from Overlay two
               SrcA_Ram = 3 - Source A is from look up table (RAM)
       bWin  - SrcA will be set only for this window
               Win0, Win1, Win2 and Win3
 Note: 1. SrcA data can be inverted before being blended. See
          InvertFunction().
       2. Alpha value is meanful to source A, e.g. if you set Alpha value to
          0x30(that is 0x30/0xFF = 18.8%), that menas the output will be
          18.8% source A plus 81.2% source B
 ****************************************************************************/
void SelectWindowBlendSrcA(u8 bSrcA, u8 bWin)
{
    u8 winMask;
    u8 winVal;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    winVal  = bSrcA << ( 2 * bWin );
    winMask = 3 << ( 2 * bWin );
    
    Out_Video_Reg( 0xFA, 0x50 );
    Out_SEQ_Reg_M( 0x40, winVal, ~winMask );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Choose which one will be used as source B to be alpha blended
 In:
       bSrcB - Source B used for alpha blending
               SrcB_V1  = 0 - Source B is from orginal graphics
               SrcB_V2  = 1 - Source B is from Overlay one
               SrcB_Gr  = 2 - Source B is from Overlay two
               SrcB_Ram = 3 - Source B is from look up table (RAM)
               SrcB_V12 = 4 - Source B is Overlay one and two
       bWin  - SrcB will be set only for this window
               Win0, Win1, Win2 and Win3
 Note: 1. SrcB data can be inverted before being blended. See
          InvertFunction().
       2. Alpha value is meanful to source A, e.g. if you set Alpha value to
          0x30(that is 0x30/0xFF = 18.8%), that menas the output will be
          18.8% source A plus 81.2% source B
 ****************************************************************************/
void SelectWindowBlendSrcB(u8 bSrcB, u8 bWin)
{
    u8 winMask;
    u8 winVal;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    
    if ( bSrcB == SrcB_V12 )
    {
        bSrcB = SrcB_V1;
    }
    
    winVal  = bSrcB << ( 2 * bWin );
    winMask = 3 << ( 2 * bWin );
    
    Out_Video_Reg( 0xFA, 0x50 );
    Out_SEQ_Reg_M( 0x41, winVal, ~winMask );
    
    if      ( SrcB_V1 == bSrcB )
    {
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg_M( 0x4F, 0x08, 0xF7 );
    }
    else if ( SrcB_V12 == bSrcB )
    {
        Out_Video_Reg( 0xFA, 0x08 );
        Out_SEQ_Reg_M( 0x4F, 0x00, 0xF7 );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 Choose alpha source where alpha data are defined
 In:
      bAlphaSrc - Alpha data source
                   Alpha_Int     - Alpha is from interleave alpha embeded
                                    in graphics data. This only applies to
                                    32 bpp: 8888
                   Alpha_Map     - Alpha is from alphamap engine
                   Alpha_Ram     - Alpha is from look up table (RAM)
                   Alpha_Reg     - Alpha is from register
      bWin - Alpha value will be set to this alpha blending window only
 Note: 1. Alpha data can be inverted before being blended. See
          InvertFunction().
 ****************************************************************************/
void SelectWindowAlphaSrc(u8 bAlphaSrc, u8 bWin)
{
    u8 winMask;
    u8 winVal;
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    /*
       Because register value is defined different from original 5000 value:
       Alpha_Map = 2 (insteads of 4) so we need to adjust, there.
     */
    if ( bAlphaSrc == Alpha_Map )
    {
        bAlphaSrc = Alpha_V2;
    }
    winMask = 3 << ( 2 * bWin );
    winVal  = bAlphaSrc << ( 2 * bWin );
    
    Out_Video_Reg( 0xFA, 0x50 );
    Out_SEQ_Reg_M( 0x42, winVal, ~winMask );
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableWindowAlphaBlendSrcA() enable or disable alpha-blending separated
window.  in:   ON  - enable alpha blending srcA
       OFF - disable alpha blending srcA
 out:  none
 ****************************************************************************/
void EnableWindowAlphaBlendSrcA(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x50 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x43, 0x01, 0xFE );
    }
    else
    {
        Out_SEQ_Reg_M( 0x43, 0x00, 0xFE );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableWindowAlphaBlendSrcB() enable or disable alpha-blending separated
window.  in:   ON  - enable alpha blending srcB
       OFF - disable alpha blending srcB
 out:  none
 ****************************************************************************/
void EnableWindowAlphaBlendSrcB(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x50 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x43, 0x02, 0xFD );
    }
    else
    {
        Out_SEQ_Reg_M( 0x43, 0x00, 0xFD );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}

/****************************************************************************
 EnableWindowAlpha() enable or disable alpha-blending separated window.
 in:   ON  - enable alpha blending alpha
       OFF - disable alpha blending alpha
 out:  none
 ****************************************************************************/
void EnableWindowAlphaBlendAlpha(u8 OnOff)
{
    u8 b3CE_F7, b3CE_FA;

    /* Save banking */
    b3CE_F7 = In_Video_Reg( 0xF7 );
    b3CE_FA = In_Video_Reg( 0xFA );
    
    Out_Video_Reg( 0xF7, 0x00 );
    Out_Video_Reg( 0xFA, 0x50 );
    
    if ( OnOff == ON )
    {
        Out_SEQ_Reg_M( 0x43, 0x04, 0xFB );
    }
    else
    {
        Out_SEQ_Reg_M( 0x43, 0x00, 0xFB );
    }
    
    /* Restore banking */
    Out_Video_Reg( 0xF7, b3CE_F7 );
    Out_Video_Reg( 0xFA, b3CE_FA );
}


/* EnableWindowAlphaBlend() -- enable or disable by-window alpha blending. 
 *
 * Parameters:
 *      OnOff -- [IN] turn on/off
 *
 * Returns:
 *      None.
 *
 * Note:
 *      You can call this routine instead of calling EnableWindowAlphaBlendAlpha(),
 *      EnableWindowAlphaBlendSrcA() and EnableWindowAlphaBlendSrcB() simultaneously.
 */
void EnableWindowAlphaBlend( u8 OnOff )
{
    EnableWindowAlphaBlendAlpha( OnOff );
    EnableWindowAlphaBlendSrcA( OnOff );
    EnableWindowAlphaBlendSrcB( OnOff );
}
