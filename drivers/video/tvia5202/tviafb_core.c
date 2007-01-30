/*--------------------------------------------
  -----General Notice-----

  Copyright (C) 1997-2002, Tvia, Inc. (Formerly IGS Technologies, Inc.)

  PERMISSION IS HEREBY GRANTED TO USE, COPY AND MODIFY THIS SOFTWARE
  ONLY FOR THE PURPOSE OF DEVELOPING TVIA RELATED PRODUCTS.

  THIS SOFTWARE MAY NOT BE DISTRIBUTED TO ANY PARTY WHICH IS NOT COVERED
  BY TVIA NON-DISCLOSURE AGREEMENT (NDA). UNAUTHORIZED DISCLOSURE
  IS AGAINST LAW AND STRICTLY PROHIBITED.

  THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
  EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

  --------About this file---------
  tviafb.c
 
  Tvia,Inc. CyberPro5k frame buffer device
  
  Based on cyber2000fb.c
  Based on cyberfb.c

  --------Modification History-----------
  1. 10-15-2002  1)Support 5002
 		 2)Support work without BIOS
		 3)Only support the mode given in SDK 
 *---------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/pm.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#include <video/fbcon.h>
/* ADVANTECH  #include <video/fbcon-cfb8.h> */
#include <video/fbcon-cfb16.h>
/* ADVANTECH #include <video/fbcon-cfb24.h> */


/* DMA */
#include <asm/dma.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <asm/siginfo.h>

/* DEBUG */
#define DEBUG 1

#ifdef DEBUG
	#define dbg(format, arg...) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg)
#else
	#define dbg(format, arg...) do {} while (0)
#endif

#define trace(format, arg...) printk(KERN_INFO __FILE__ ": " format "\n" , ## arg)



/* Define some data structure and the mode table*/
#include "tviafb.h"
#include "tvia5202-ioctl.h"

#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif

#define VMEM_BASEADDR	0x14000000
#define VMEM_SIZE		0x800000 /* 0x400000 */
#define VMEM_HIGH_MEMORY_63MB 	0xA3F00000 /* 63-simo MB */
#define MEM_FOR_MOTION_JPEG 320*240*2
//#define MEM_FOR_MOTION_JPEG 320

/* Enabling RGBDAC and VGA HSINC/VSINC outputs */
#undef DISABLE_RGBDAC
/* #define DISABLE_RGBDAC 1 */


#ifdef CONFIG_PM
static struct pm_dev *fb_pm_dev=NULL;
static unsigned char *video_buf=NULL;
static unsigned char *cursor_buf=NULL;
#endif

/*-----------------------------------------------------
* 	Debug functions
*-----------------------------------------------------*/
#ifdef DEBUG
static void debug_printf(char *fmt, ...)
{
    char buffer[128];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    printascii(buffer);
}
#else
#define debug_printf(x...) do { } while (0)
#endif

#define deb  debug_printf

/*-------------------------------------------------------
 *    Defaults  mode
 *-------------------------------------------------------*/
//#define DEFAULT_XRES	640	//800
//#define DEFAULT_YRES	480	//600
#define DEFAULT_XRES	720	//800
#define DEFAULT_YRES	576     //600

#define DEFAULT_BPP	16


/*-------------------------------------------------------
 *    Tvia mode selection
 *-------------------------------------------------------*/
#define TVIA_MODE 1 /* PAL 710x576 */

static int reg_tv_size_5202=REG_TV_SIZE_5202;
static int init_mode=INIT_MODE;
static int tvia_mode=TVIA_MODE;
MODULE_PARM(video,"i");
MODULE_PARM_DESC(tvia_mode,"Set Tvia mode output: 1=PAL-720x576,2=NTSC-M-640x480,default=PAL-720x576");


int selected_xres = DEFAULT_XRES;
int selected_yres = DEFAULT_YRES;

#ifndef MODULE
/* kernel command-line management */
static int __init tvia_mode_set(char *str)
{
        int par;
        if (get_option(&str,&par))
          {
          tvia_mode = par;
          }

        return 1;
}
__setup("tvia_mode=", tvia_mode_set);

#endif


/*------------------------------------------------------
*	Global variable
*------------------------------------------------------*/
volatile unsigned char *CyberRegs;       /* mapped I/O base */
volatile unsigned char *Buffer_Motion_Jpeg;    /* mapped I/O base */
unsigned long fb_mem_addr;
unsigned short x_res;
static struct display global_disp;      /*default display */
static struct fb_info fbinfo;
static struct tviafb_par current_par;   /*record current configuration */
static struct display_switch *dispsw;
static struct fb_var_screeninfo init_var;      /*initilization data */
static struct fb_ops tviafb_ops;
	
//CARLOS DMA
static int w_dma_ch, r_dma_ch;
static int w_dma_ch_flag = 0;
static int r_dma_ch_flag = 0;
static TVIA5202_DMACONF WConf;
static TVIA5202_DMACONF RConf;
static DECLARE_WAIT_QUEUE_HEAD(r_queue);
static DECLARE_WAIT_QUEUE_HEAD(w_queue);
/* DEBUG */
//static int org[(320*240*2)+7];
extern unsigned char btweb_bigbuf[];
extern unsigned char btweb_bigbuf1[];
//static int dst[(320*240*2)+7];
static int orga, dsta;
//END CARLOS DMA

/*-------------------------------------------------------
*	Prototypes
*--------------------------------------------------------*/
static int Tvia_DMAReadFrame(void);
static int Tvia_DMAWriteFrame(void);


/*Hardware cursor founctions*/
static void tvia_init_hwcursor(void);
static int tviafb_get_fix_cursorinfo(struct fb_fix_cursorinfo *fix,
                                     int con);
static int tviafb_get_var_cursorinfo(struct fb_var_cursorinfo *var,
                                     u_char * data, int con);
static int tviafb_set_var_cursorinfo(struct fb_var_cursorinfo *var,
                                     u_char * data, int con);
static int tviafb_get_cursorstate(struct fb_cursorstate *state, int con);
static int tviafb_set_cursorstate(struct fb_cursorstate *state, int con);

/*-------------------------------------------------------
*	CRT, GRAPHIC,ATTRIBUTE,SEQUENCER registers' I/O functions
*-------------------------------------------------------*/
static inline void tvia_crtcw(int val, int reg)
{
    tvia_outb(reg, 0x3d4);
    tvia_outb(val, 0x3d5);
}

static inline void tvia_grphw(int val, int reg)
{
    tvia_outb(reg, 0x3ce);
    tvia_outb(val, 0x3cf);
}

static inline void tvia_attrw(int val, int reg)
{
    tvia_inb(0x3da);
    tvia_outb(reg, 0x3c0);
    tvia_inb(0x3c1);
    tvia_outb(val, 0x3c0);
}

static inline void tvia_seqw(int val, int reg)
{
    tvia_outb(reg, 0x3c4);
    tvia_outb(val, 0x3c5);
}

/* -------------------------------------------------------*
* 
*	Hardware specific routines
*	
*-------------------------------------------------------- */

/* -------------------------------------------- 
* The following functions is used in initchip and set modes.
* Please refer to SDK1 .
* ---------------------------------------------*/
static void timedelay(u32 iCounter)
{
	u32 i;
	u8 iTmp;

    for (i = 0; i < iCounter; i++)
        iTmp = tvia_inb(0x3ce);
}

static void ToggleClock(void)
{
    u8 bTmp, i;

    tvia_outb(0xBF, 0x3ce);
    tvia_outb(0x00, 0x3cf);     /*banking register */
    tvia_outb(0xBA, 0x3ce);
    tvia_outb(tvia_inb(0x3cf), 0x3cf);  /*use internal RC filter for PLL */
    tvia_outb(0xB9, 0x3ce);
    bTmp = tvia_inb(0x3cf);

    tvia_outb(bTmp & 0x7F, 0x3cf);      /*bit <7> is the control bit */
    tvia_outb(bTmp | 0x80, 0x3cf);
    tvia_outb(bTmp & 0x7F, 0x3cf);

    /* wait for clock toggle stable */
    for (i = 0; i < 5; i++) {   /* wait for 3 CRT vertical sync */
#ifdef __arm__
		// We should do some time delay
		do {
			timedelay(1000);
			bTmp = tvia_inb(0x3da);
		} while((bTmp & 0x08) != 0);
		
		do {
			timedelay(1000);
			bTmp = tvia_inb(0x3da);
		} while((bTmp & 0x08) == 0);
#else
        while ((tvia_inb(0x3DA) & 0x08) != 0x00);       /*search until 0 */
        while ((tvia_inb(0x3DA) & 0x08) == 0x00);       /*search until 1 */
#endif
    }
}

static void SetExtReg(u8 * bPtr, u16 iCounter)
{
    u16 i;
    for (i = 0; i < iCounter; i++) {
        tvia_outb(*bPtr++, 0x3ce);      /*index */
        tvia_outb(*bPtr++, 0x3cf);      /*data */
    }
}

static void SetStandardReg(u8 * bPtr)
{
    u16 i;

    /* 1. set sequencer reg */
    for (i = 0; i < SEQCOUNT; i++) {
        tvia_outb((u8) i, 0x3c4);
        tvia_outb(*bPtr++, 0x3c5);
    }

    /* 2. set miscellaneous reg */
    tvia_outb(*bPtr++, 0x3c2);

    tvia_outb(0x00, 0x3c4);
    tvia_outb(0x03, 0x3c5);     /*seq normal operation */

    /* 3. set CRT reg */
    tvia_outb(0x11, 0x3d4);
    tvia_outb(0x00, 0x3d5);     /*unlock CRT */

    for (i = 0; i < CRTCOUNT; i++) {
        tvia_outb((u8) i, 0x3d4);
        tvia_outb(*bPtr++, 0x3d5);
    }

    /* 4. set attribute reg */
    i = tvia_inb(0x3da);        /*reset Attribute */

    for (i = 0; i < ATTRCOUNT; i++) {
        tvia_outb((u8) i, 0x3c0);
        tvia_outb(*bPtr++, 0x3c0);
    }

    tvia_outb(0x20, 0x3c0);     /*turn on screen */
    i = tvia_inb(0x3da);        /*reset Attribute */

    /* 5. set graphics reg */
    for (i = 0; i < GRACOUNT; i++) {
        tvia_outb((u8) i, 0x3ce);
        tvia_outb(*bPtr++, 0x3cf);
    }
}

static void SetPartExtReg(u8 * bPtr)
{
    u16 i;

    tvia_outb(0x13, 0x3d4);     /*index */
    tvia_outb(*bPtr++, 0x3d5);  /*data */

    for (i = 0; i < EXTPARTIALCOUNT; i++) {
        tvia_outb(*bPtr++, 0x3ce);      /*index */
        tvia_outb(*bPtr++, 0x3cf);      /*data */
    }

    ToggleClock();              /*to latch the clock value into working set */
}

static void SetRamDac(u8 * bPtr, u16 iCounter)
{
    u16 i;

    tvia_outb(0, 0x3c8);        /*start from index 0 */
    for (i = 0; i < iCounter; i++) {
	    tvia_outb(*bPtr++, 0x3c9);    /*R*/
	    tvia_outb(*bPtr++, 0x3c9);    /*G*/
	    tvia_outb(*bPtr++, 0x3c9);    /*B*/
    }
    tvia_outb(0xff, 0x3c6);     /*Mask register */
}

static void SetExtSGReg(u8 * bPtr, u16 iCounter)
{
    u16 i;
    for (i = 0; i < iCounter; i++) {
        tvia_outb(*bPtr++, 0x3c4);      /*index */
        tvia_outb(*bPtr++, 0x3c5);      /*data */
    }
}

static void ResetSeq(void)
{
    u8 bTmp;
    tvia_outb(0x30, 0x3c4);
    bTmp = tvia_inb(0x3c5);
    tvia_outb(bTmp & 0x7F, 0x3c5);      /*bit 7=0 -> 1, sync reset seq */
    tvia_outb(bTmp | 0x80, 0x3c5);
    tvia_outb(0x36, 0x3c4);
    bTmp = tvia_inb(0x3c5);
    tvia_outb(bTmp & 0xFB, 0x3c5);
    tvia_outb(bTmp | 0x04, 0x3c5);
}

static void SetDACPower(u16 iOnOff)
{
    tvia_outb(0x56, 0x3ce);
    tvia_outb(tvia_inb(0x3cf) | 0x04, 0x3cf);   /*banking bit */

    if (iOnOff == ON)
        tvia_outb(tvia_inb(0x3c6) & ~0x40, 0x3c6);      /*DAC power on */
    else
        tvia_outb(tvia_inb(0x3c6) | 0x40, 0x3c6);       /*DAC power off */

    tvia_outb(0x56, 0x3ce);
    tvia_outb(tvia_inb(0x3cf) & ~0x04, 0x3cf);
}

static void EnableChip(void)
{
    SetDACPower(OFF);
}

#if 0 /* Replaced with similar instructions at the beginning of _init */
static int TestChip(void)
{
    int idx;
    u8 bTmp;
  	u8 tmp;


  	/* Wake up the chip */
    trace("TestChip: 46e8=0x18");
    tvia_outb(0x18, 0x46e8); // orig 0x18 //!!!rob 0x10
    udelay(1000);
    trace("TestChip: 102=0x01");
    tvia_outb(0x01, 0x102);
    udelay(1000);
 
    trace("TestChip: 102 read not VLIO mode because NO_BE_MODE not already configured !!");
	for(idx=0;idx<20;idx++){
      bTmp = tvia_inb_novlio(0x102);
      if (bTmp&0x01)
		break;
      trace("Retry N.%i",idx);
      udelay(1000);
    }
    if (idx==20){
	  trace("TestChip: 102 read NOT read");
	  return 1;
	}

	trace("TestChip: END SETUP 46e8=0x08");
    tvia_outb(0x08, 0x46e8);//!!!rob 0x10
    udelay(1000);     

	/* Enable linear address */
	trace("Enable linear address!");
	tvia_outb(0x33, 0x3ce);
	tvia_outb(0x01, 0x3cf);
    debug_printf("tviafb: enable linear address\n");

	
    /* Set NO_BE_MODE */	
//	trace("BANKING");
//	tvia_outb(0x33, 0x3ce);
//	tvia_outb(0x40, 0x3cf); /* set the banking */
	
//	trace("NO_BE_MODE setting");
//	tvia_outb(0x3c, 0x3ce);
//	tvia_outb(0x40, 0x3cf); /* set NO_BE_MODE */
	
//	trace("NO_BE_MODE - verify");
//	if(tvia_inb(0x3cf) != 0x40)
//	{
	    //tvia_outb(0x33, 0x3ce);
        //tvia_outb(0x00, 0x3cf);
	    //return 1;
	//}
	
	//trace("CLEAR BANKING");
	//tvia_outb(0x33, 0x3ce);
	//tvia_outb(0x00, 0x3cf);  /* clear the banking */


	
/*
	trace("BYTE enable mode ?");
	tvia_outb(0x33, 0x3ce);
	trace("1BYTE enable mode ?");
	tvia_outb(0x45, 0x3cf);
	trace("BYTE enable mode - selection bank");
	tvia_outb(0x3c, 0x3ce);
	tmp=tvia_inb(0x3cf);
	tmp|=0x40;
	tvia_outb(tmp, 0x3cf);
 	trace("BYTE enable mode - done");
	tvia_outb(0x33, 0x3ce);
	tvia_outb(0x05, 0x3cf);
	trace("BYTE enable mode - selection bank back");
*/	
	
    trace("TestChip: 3ce=0x95");
    tvia_outb(0x95, 0x3ce);
    udelay(1000);

    trace("TestChip: 3ce read");
    for(idx=0;idx<20;idx++){
      bTmp = tvia_inb(0x3ce);
      if (bTmp==0x95)
	break;
      udelay(1000);
    }
    if (idx==20)
      return 2;

    trace("TestChip: 3cf=0x95");
    tvia_outb(0x5a, 0x3cf);
    udelay(1000);

    trace("TestChip: 3cf read");
    for(idx=0;idx<20;idx++){
      bTmp = tvia_inb(0x3cf);
      if (bTmp==0x5a)
	break;
      udelay(1000);
    }
    if (idx==20)
      return 3;

	
    trace("TestChip: Lettura identificativo");
    tvia_outw(0x0033,0x3ce);
    trace("TestChip: Lettura identificativo1");
    tvia_outb(0x91, 0x3ce);
    trace("TestChip: Lettura identificativo2");
    bTmp = tvia_inb(0x3cf);
    trace("TestChip: Lettura identificativo FIRST BYTE=%x",bTmp);

    if (bTmp!=0xa3){
      trace("TestChip: Lettura identificativo FIRST BYTE ERRATO");
      return 4;
    }

    tvia_outb(0x92, 0x3ce);
    trace("TestChip: Lettura identificativo3");
    bTmp = tvia_inb(0x3cf);
    trace("TestChip: Lettura identificativo SECOND BYTE=%x",bTmp);

    if (bTmp!=0x0e){
      trace("TestChip: Lettura identificativo SECOND BYTE ERRATO");
      return 4;
    }

    tvia_outb(0x93, 0x3ce);
    trace("TestChip: Lettura identificativo4");
    bTmp = tvia_inb(0x3cf);
    trace("TestChip: Lettura identificativo THIRD BYTE=%x",bTmp);

    if (bTmp!=0x02){
      trace("TestChip: Lettura identificativo THIRD BYTE ERRATO");
      return 4;
    }
	
    return 0;
}
#endif

static void CleanUpReg(void)
{
    /*Unlock CRT
       It is useful only when TVIA VGA BIOS is present.TVIA hardware power 
       on default is CRT unlocked, i.e 3D4_1F default is 0x03.
     */

    tvia_outb(tvia_inb(0x3cc) | 0x01, 0x3c2);
    tvia_outb(0x1F, 0x3d4);
    tvia_outb(0x03, 0x3d5);     /* to unlock CRT protection */

    /*Underscan registers */
    tvia_outb(0x44, 0x3d4);
    tvia_outb(0x00, 0x3d5);
    tvia_outb(0x45, 0x3d4);
    tvia_outb(0x00, 0x3d5);

    /*Video Capture Registers */
    tvia_outb(0xE8, 0x3ce);
    tvia_outb(0x00, 0x3cf);

    /*Video Overlay Registers */
    tvia_outb(0xDC, 0x3ce);
    tvia_outb(0x00, 0x3cf);
    tvia_outb(0xDC, 0x3c4);
    tvia_outb(0x00, 0x3c5);

    /*Alpha Blending Registers */
    tvia_outb(0xFA, 0x3ce);
    tvia_outb(0x00, 0x3cf);
    tvia_outb(0x4B, 0x3c4);
    tvia_outb(0x00, 0x3c5);
}

static void InitChip(void)
{
    ModeInit *bpTVIAModeReg;
    u16 i;
    u8 tmp;

    bpTVIAModeReg = TVIAModeReg_5202[init_mode]; 

    deb("InitChip");
    /*CYBER5000SG */
    tvia_outb(0x70, 0x3ce);
    tvia_outb(0xCB, 0x3cf);     /*reset sequencer */

    SetExtReg(ExtRegData_5202, sizeof(ExtRegData_5202) / 2);

    SetStandardReg(bpTVIAModeReg->bpStdReg->VGARegs);

    SetPartExtReg(bpTVIAModeReg->bpExtReg->TVIARegs);

    SetRamDac(RamDacData, sizeof(RamDacData) / 3);

    ResetSeq();

    for (i = 0; i < 10; i++) {
#ifdef __arm__

		do {
			timedelay(1000);
			tmp = tvia_inb(0x3da);
		} while((tmp & 0x08) != 0);

		
		do {
			timedelay(1000);
			tmp = tvia_inb(0x3da);
		} while((tmp & 0x08) == 0);
#else
        while ((tvia_inb(0x3da) & 0x08) != 0x00);
        while ((tvia_inb(0x3da) & 0x08) == 0x00);
#endif
    }

    dbg("SetExtSGReg");
    SetExtSGReg(ExtSGRegData_5202, sizeof(ExtSGRegData_5202) / 2);

    tvia_outb(0x70, 0x3ce);
    tvia_outb(0x4B, 0x3cf);     /*Block CRT request */

    tvia_outb(0x36, 0x3c4);
    tvia_outb(0x3D, 0x3c5);     /*activate SG/SDRAM initial cycle */

    dbg("SGRAM init");
    /* wait for SGRAM initializaion stable */
    for (i = 0; i < 10; i++) {  /* wait for 15 CRT vertical sync */
#ifdef __arm__
		do {
			timedelay(1000);
			tmp = tvia_inb(0x3da);
		} while((tmp & 0x08) != 0);
		
		do {
			timedelay(1000);
			tmp = tvia_inb(0x3da);
		} while((tmp & 0x08) == 0);
#else
        while ((tvia_inb(0x3DA) & 0x08) != 0x00);       /*search until 0 */
        while ((tvia_inb(0x3DA) & 0x08) == 0x00);       /*search until 1 */
#endif
    }

    tvia_outb(0x70, 0x3ce);
    tvia_outb(0x0B, 0x3cf);     /*set sequencer */

    { 
      /* !!!raf */
      u8 bTmp;

      tvia_outb(0x70, 0x3ce);     
      bTmp = tvia_inb(0x3cf);
      deb("Tvia-5202: 3cf/70=%x",bTmp);
    }


    /*choose voltage reference */
    tvia_outb(0x56, 0x3ce);
    tvia_outb(0x04, 0x3cf);
    tvia_outb(0x44, 0x3c6);     /*power down DAC and use VRef */
    tvia_outb(0x56, 0x3ce);
    tvia_outb(0x00, 0x3cf);
}

/* PostRoutine */
static void PostRoutine(void)
{
    /*force to use externel RC filter for PLL */
    tvia_outb(0xBF, 0x3ce);
    tvia_outb(0x03, 0x3cf);
    tvia_outb(0xB2, 0x3ce);
    tvia_outb(0x00, 0x3cf);
    tvia_outb(0xB5, 0x3ce);
    tvia_outb(0x00, 0x3cf);
    tvia_outb(0xBF, 0x3ce);
    tvia_outb(0x00, 0x3cf);

    /*clean up some reigsters in case warm boot */
    CleanUpReg();

    dbg("CleanUpReg() pass");
    InitChip();                 /* initialize to 800x600x16 @60Hz mode */
}

static void tvia_init_hw(void)
{
    EnableChip();
    dbg("EnableChip() pass");
    PostRoutine();
}

/* ---------------------------------
*	Hardware CyberPro5k Acceleration 
*----------------------------------*/
static void tvia_accel_wait(void)
{
    int count = 10000;

    while (tvia_inb(0xbf011) & 0x80) {
        if (!count--) {
            debug_printf("accel_wait timed out\n");
            tvia_outb(0, 0xbf011);
            return;
        }
        udelay(10);
    }
}

static void tvia_accel_setup(struct display *p)
{
    dispsw->setup(p);
}

static void
tvia_accel_bmove(struct display *p, int sy, int sx, int dy, int dx,
                 int height, int width)
{
    unsigned long src, dst, chwidth = p->var.xres_virtual * fontheight(p);
    int v = 0x8000;

    if (sx < dx) {
        sx += width - 1;
        dx += width - 1;
        v |= 4;
    }

    if (sy < dy) {
        sy += height - 1;
        dy += height - 1;
        v |= 2;
    }

    sx *= fontwidth(p);
    dx *= fontwidth(p);
    src = sx + sy * chwidth;
    dst = dx + dy * chwidth;
    width = width * fontwidth(p) - 1;
    height = height * fontheight(p) - 1;

    tvia_accel_wait();
    tvia_outb(0x00, 0xbf011);
    tvia_outb(0x03, 0xbf048);
    tvia_outw(width, 0xbf060);

    if (p->var.bits_per_pixel != 24) {
        tvia_outl(dst, 0xbf178);
        tvia_outl(src, 0xbf170);
    } else {
        tvia_outl(dst * 3, 0xbf178);
        tvia_outb(dst, 0xbf078);
        tvia_outl(src * 3, 0xbf170);
    }

    tvia_outw(height, 0xbf062);
    tvia_outw(v, 0xbf07c);
    tvia_outw(0x2800, 0xbf07e);
}

static void
tvia_accel_clear(struct vc_data *conp, struct display *p, int sy, int sx,
                 int height, int width)
{
    unsigned long dst;
    u32 bgx = attr_bgcol_ec(p, conp);

    dst = sx * fontwidth(p) + sy * p->var.xres_virtual * fontheight(p);
    width = width * fontwidth(p) - 1;
    height = height * fontheight(p) - 1;

    tvia_accel_wait();
    tvia_outb(0x00, 0xbf011);
    tvia_outb(0x03, 0xbf048);
    tvia_outw(width, 0xbf060);
    tvia_outw(height, 0xbf062);

    switch (p->var.bits_per_pixel) {
    case 16:
        bgx = ((u16 *) p->dispsw_data)[bgx];
    case 8:
        tvia_outl(dst, 0xbf178);
        break;

    case 24:
        tvia_outl(dst * 3, 0xbf178);
        tvia_outb(dst, 0xbf078);
        bgx = ((u32 *) p->dispsw_data)[bgx];
        break;
    }

    tvia_outl(bgx, 0xbf058);
    tvia_outw(0x8000, 0xbf07c);
    tvia_outw(0x0800, 0xbf07e);
}

static void
tvia_accel_putc(struct vc_data *conp, struct display *p, int c, int yy,
                int xx)
{
    tvia_accel_wait();
    dispsw->putc(conp, p, c, yy, xx);
}

static void
tvia_accel_putcs(struct vc_data *conp, struct display *p,
                 const unsigned short *s, int count, int yy, int xx)
{
    tvia_accel_wait();
    dispsw->putcs(conp, p, s, count, yy, xx);
}

static void tvia_accel_revc(struct display *p, int xx, int yy)
{
    tvia_accel_wait();
    dispsw->revc(p, xx, yy);
}

static void
tvia_accel_clear_margins(struct vc_data *conp, struct display *p,
                         int bottom_only)
{
    dispsw->clear_margins(conp, p, bottom_only);
}

static struct display_switch fbcon_cyber_accel = {
    tvia_accel_setup,
    tvia_accel_bmove,
    tvia_accel_clear,
    tvia_accel_putc,
    tvia_accel_putcs,
    tvia_accel_revc,
    NULL,
    NULL,
    tvia_accel_clear_margins,
    FONTWIDTH(8) | FONTWIDTH(16)
};


/*------------------------------------------
 *	  Hardware Cursor
*------------------------------------------ */
static const u_long tviafb_hwcursor_shape[] = {
    0xAAA80200, 0xAAA85454,
    0xAAA80540, 0xAAAA810A,
    0xAAAAA12A, 0xAAAAA12A,
    0xAAAAA12A, 0xAAAAA12A,
    0xAAAAA12A, 0xAAAAA12A,
    0xAAAAA12A, 0xAAAAA12A,
    0xAAAA810A, 0xAAA80540,
    0xAAA85454, 0xAAA80200,
};

static void tviafb_move_hwcursor(void)
{
    short x, y;

    tvia_outb(0x56, 0x3ce);
    if (current_par.crsr.mode != FB_CURSOR_ON) {
        tvia_outb(tvia_inb(0x3cf) & 0xfe, 0x3cf);
        return;
    }
    tvia_outb(tvia_inb(0x3cf) | 0x01, 0x3cf);

    x = current_par.crsr.xoffset;
    y = current_par.crsr.yoffset;
    tvia_grphw(x & 0xff, 0x50);
    tvia_grphw((x & 0xff00) >> 8, 0x51);
    tvia_grphw(y & 0xff, 0x53);
    tvia_grphw((y & 0xff00) >> 8, 0x54);
}

static int tviafb_get_fix_cursorinfo(struct fb_fix_cursorinfo *fix,
                                     int con)
{
    fix->crsr_width = fix->crsr_xsize = 64;
    fix->crsr_height = fix->crsr_ysize = 64;
    fix->crsr_color1 = 17;
    fix->crsr_color2 = 18;
    return 0;
}

static int tviafb_get_var_cursorinfo(struct fb_var_cursorinfo *var,
                                     u_char * data, int con)
{
    var->width = var->height = var->xspot = var->yspot =
        current_par.crsr.crsr_xsize;
    return 0;
}

static int tviafb_set_var_cursorinfo(struct fb_var_cursorinfo *var,
                                     u_char * data, int con)
{
    int i;
    unsigned char shape;
    unsigned char *base;

    base =
        (unsigned char *) (current_par.screen_base +
                           current_par.screen_size - 0x100);

    if ((i = verify_area(VERIFY_READ, (void *) data, 256)))
        return i;
    for (i = 0; i < 256; i++, data++) {
        get_user(shape, data);
        *(base + i) = shape;
    }
    return 0;
}

static int tviafb_get_cursorstate(struct fb_cursorstate *state, int con)
{
    state->xoffset = current_par.crsr.xoffset;
    state->yoffset = current_par.crsr.xoffset;
    state->mode = current_par.crsr.mode;
    return 0;
}

static int tviafb_set_cursorstate(struct fb_cursorstate *state, int con)
{
    current_par.crsr.xoffset = state->xoffset;
    current_par.crsr.yoffset = state->yoffset;
    current_par.crsr.mode = state->mode;
    tviafb_move_hwcursor();
    return 0;
}

static void tvia_init_hwcursor(void)
{
    int i;
    volatile u_long *hwcursor_base;
    u8 reg_56;

    /* Disable hardware cursor */
    tvia_outb(0x56, 0x3ce);
    tvia_outb(tvia_inb(0x3cf) | 0x01, 0x3cf);

    tvia_outb(tvia_inb(0x3cf) & 0xfd, 0x3cf);
    tvia_grphw(0x20, 0x52);
    tvia_grphw(0x20, 0x55);

    /*Set hardware cursor color */
    tvia_outb(0x56, 0x3ce);
    reg_56 = tvia_inb(0x3cf);
    tvia_outb(reg_56 | 0x04, 0x3cf);
    tvia_outb(0, 0x3c8);
    tvia_outb(0xff, 0x3c9);
    tvia_outb(0xff, 0x3c9);
    tvia_outb(0xff, 0x3c9);
    tvia_outb(1, 0x3c8);
    tvia_outb(0x00, 0x3c9);
    tvia_outb(0x00, 0x3c9);
    tvia_outb(0x00, 0x3c9);
    tvia_outb(reg_56, 0x3cf);

    /* Initialize hardware cursor */
    hwcursor_base =
        (u_long *) (current_par.screen_base + current_par.screen_size -
                    0x100);

    for (i = 0; i < 64; i++)
        *(hwcursor_base + i) = 0xaaaaaaaa;
    for (i = 0; i < 32; i += 2)
        *(hwcursor_base + i) = tviafb_hwcursor_shape[i / 2];

    tvia_grphw((unsigned char) ((current_par.screen_size - 0x400) >> 10),
               0x7e);
    tvia_grphw((unsigned char) ((current_par.screen_size - 0x400) >> 18),
               0x7f);
    tviafb_move_hwcursor();
}

/*-----------------------------------------
 * 	Palette operations 
 *----------------------------------------*/
static int
tvia_getcolreg(u_int regno, u_int * red, u_int * green, u_int * blue,
               u_int * transp, struct fb_info *info)
{
    int t;

    if (regno >= 256)
        return 1;

    t = current_par.palette[regno].red;
    *red = t << 10 | t << 4 | t >> 2;

    t = current_par.palette[regno].green;
    *green = t << 10 | t << 4 | t >> 2;

    t = current_par.palette[regno].blue;
    *blue = t << 10 | t << 4 | t >> 2;

    *transp = 0;

    return 0;
}

static int
tvia_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
               u_int transp, struct fb_info *info)
{
    if (regno > 255)
        return 1;

    current_par.palette[regno].red = red >> 10;
    current_par.palette[regno].green = green >> 10;
    current_par.palette[regno].blue = blue >> 10;

    switch (fb_display[current_par.currcon].var.bits_per_pixel) {
#ifdef FBCON_HAS_CFB8
    case 8:
        tvia_outb(regno, 0x3c8);
        tvia_outb(red >> 10, 0x3c9);
        tvia_outb(green >> 10, 0x3c9);
        tvia_outb(blue >> 10, 0x3c9);
        break;
#endif

#ifdef FBCON_HAS_CFB16
    case 16:
        if (regno < 64) {
            /* write green */
            tvia_outb(regno << 2, 0x3c8);
            tvia_outb(current_par.palette[regno >> 1].red, 0x3c9);
            tvia_outb(green, 0x3c9);
            tvia_outb(current_par.palette[regno >> 1].blue, 0x3c9);
        }

        if (regno < 32) {
            /* write red,blue */
            tvia_outb(regno << 3, 0x3c8);
            tvia_outb(red, 0x3c9);
            tvia_outb(current_par.palette[regno << 1].green, 0x3c9);
            tvia_outb(blue, 0x3c9);
        }

        if (regno < 16)
            current_par.c_table.cfb16[regno] = ((red & 0xf800) |
                                                ((green & 0xf800) >> 5) |
                                                ((blue & 0xf800) >> 11));
        break;
#endif

#ifdef FBCON_HAS_CFB24
    case 24:
        red >>= 8;
        green >>= 8;
        blue >>= 8;

        if (regno < 16)
            current_par.c_table.cfb24[regno] =
                red << 16 | green << 8 | blue;
        break;
#endif

    default:
        return 1;
    }

    return 0;
}

/*------------------------------------
* Set  TV mode --please refer to SDK1
*------------------------------------*/
static void BypassMode(u16 iOnOff)
{
    u8 bTmp;

    tvia_outb(0x56, 0x3ce);
    bTmp = tvia_inb(0x3cf);
    tvia_outb(bTmp | 0x04, 0x3cf);

    if (iOnOff == ON)
        tvia_outb(tvia_inb(0x3c6) | 0x10, 0x3c6);
    else
        tvia_outb(tvia_inb(0x3c6) & ~0x10, 0x3c6);

    tvia_outb(bTmp & ~0x04, 0x3cf);
}

#if 0
static void EnableTV(u16 iOnOff)
{
    u8 iTmp;

    tvia_outb(0xFA, 0x3ce);     /*Banking I/O control */
    tvia_outb(0x05, 0x3cf);

    if (iOnOff == ON) {
        tvia_outb(0x4E, 0x3ce);
        iTmp = tvia_inb(0x3cf) & ~0x05; /*bit <2,0> */

        if (ReadTVReg(0xE438) & 0x1000) /*1: NTSC; 0: PAL */
            tvia_outb(iTmp | 0x04, 0x3cf);
        else
            tvia_outb(iTmp | 0x05, 0x3cf);
    } else {
        tvia_outb(0x4E, 0x3ce);
        tvia_outb(tvia_inb(0x3cf) & ~0x04, 0x3cf);
        WriteTVReg(0xE430, ReadTVReg(0xE430) & ~0x0102);
    }
}

#endif
static void EnableTV(u16 iOnOff)
{
    u8 iTmp;

    tvia_outb(0xFA, 0x3ce);     /*Banking I/O control */
    tvia_outb(0x05, 0x3cf);

    if (iOnOff == ON) {
        tvia_outb(0x4E, 0x3ce);
        iTmp = tvia_inb(0x3cf) & ~0x05; /*bit <2,0> */

        if (ReadTVReg(0xE438) & 0x1000) { /*1: NTSC; 0: PAL */
	    trace ("EnableTV: NTSC\n");
            tvia_outb(iTmp | 0x04, 0x3cf);
	}
        else
	{
            trace ("EnableTV: PAL\n");

//		  trace("EnableTV PAL but Disable RGB DAC and SCART");
//          tvia_outb((iTmp|0x25)&0x2f, 0x3cf);   // Disable RGB DAC and SCART
//		  trace("EnableTV PAL");
//          tvia_outb(iTmp | 0x05, 0x3cf); 
//		  trace("EnableTV PAL but all the other q");
//          tvia_outb(0x4E, 0x3ce);
//          tvia_outb(0xeb, 0x3cf);
 		  deb("EnableTV PAL ??? iTmp=%x",iTmp);
		  iTmp=(iTmp&0x2f)|0x15;
//		  iTmp=iTmp|0xff;
 		  deb("iTmp=%x",iTmp);
          tvia_outb(iTmp | 0x25, 0x3cf); 

#if 0 /* To directly test RGB output */
 		  trace("Test RGB");
  		  tvia_outb(0xF7, 0x3ce);     // Banking I/O control 
		  tvia_outb(0x00, 0x3cf);
  		  tvia_outb(0xFA, 0x3ce);     // Banking I/O control 
		  tvia_outb(0x01, 0x3cf);

  		  tvia_outb(0x4d, 0x3ce);     // test rgb 
		  tvia_outb(0x20, 0x3cf);

  		  tvia_outb(0xF7, 0x3ce);     // Banking I/O control 
		  tvia_outb(0x00, 0x3cf);
  		  tvia_outb(0xFA, 0x3ce);     // Banking I/O control 
		  tvia_outb(0x00, 0x3cf);

		  tvia_outb(0xd0, 0x3ce);     // test rgb Blu val 
		  tvia_outb(0x70, 0x3cf);
		  tvia_outb(0xcf, 0x3ce);     // test rgb Green val 
		  tvia_outb(0x70, 0x3cf);
		  tvia_outb(0xce, 0x3ce);     // test rgb Red val 
		  tvia_outb(0x70, 0x3cf);
#endif		  
		  
		  }
    } else {
        tvia_outb(0x4E, 0x3ce);
        tvia_outb(tvia_inb(0x3cf) & ~0x04, 0x3cf);
        WriteTVReg(0xE430, ReadTVReg(0xE430) & ~0x0102);
    }
}

static void TVOn(u16 iOnOff)
{
    if (iOnOff == ON) {

     	dbg("TVOn");
	  
        tvia_outb(0x5C, 0x3ce);
        tvia_outb(tvia_inb(0x3cf) | 0x20, 0x3cf);    //ORIG 

//	        tvia_outb(tvia_inb(0x3cf) & ~0x20, 0x3cf); //!!!raf
	  
	  
        if (current_par.device_id == 0x5000) {

            dbg("TVOn: id 5000");
		  
            tvia_outb(0xFA, 0x3ce);
            tvia_outb(0x05, 0x3cf);
            tvia_outb(0x4e, 0x3ce);
            tvia_outb(tvia_inb(0x3cf) | 0x04, 0x3cf);
        }
    } else {
        tvia_outb(0x5C, 0x3ce);
        tvia_outb(tvia_inb(0x3cf) & ~0x20, 0x3cf);
        if (current_par.device_id == 0x5000) {
            tvia_outb(0xFA, 0x3ce);
            tvia_outb(0x05, 0x3cf);
            tvia_outb(0x4e, 0x3ce);
            tvia_outb(tvia_inb(0x3cf) & ~0x04, 0x3cf);
        }
    }
}

static void SetTVReg(u16 * wPtr, u16 iCounter)
{
    u16 wTVIndex, wTVValue;
    u16 i;

    iCounter >>= 1;
    for (i = 0; i < iCounter; i++) {
        wTVIndex = *wPtr++;
        wTVValue = *wPtr++;
        WriteTVReg(wTVIndex, wTVValue);
    }
}

static void tvia_set_tvcolor(void)
{
    int i;
    /*New algorithm to detect TV color */
    unsigned short tmp;
    unsigned short tmp_e440;
    unsigned short tmp_e448;
    unsigned short tmp_e430;

    tmp_e440 = tvia_inw(0xbE440);
    tmp_e448 = tvia_inw(0xbE448);
    tmp_e430 = tvia_inw(0xbE430);

    /* Reset FSC: 3CE_4E<1>=1 */
    tvia_grphw(0x05, 0xFA);
    tvia_outb(0x4E, 0x3ce);
    tvia_outb(tvia_inb(0x3cf) | 0x02, 0x3cf);

    /*make TV screen is not visible to avoid flash */
    tvia_outw(tmp_e430 | 0x0003, 0xbe430);

    /*Make the TV blank area nerrow */
    tvia_outw(0x20, 0xbE440);
    tvia_outw(0x20, 0xbE448);

    for (i = 0; i < 10; i++)
        udelay(1000);

    /* set test logic to blue */
    tvia_outw(0xff40, 0xbE604);
    tvia_outw(0x0000, 0xbE606);

    /* Wait for Vertical Display Enable and wait 2 frames */
#ifdef __arm__
	do {
		timedelay(1000);
		tmp = tvia_inb(0x3da);
	} while((tmp & 0x08) == 0);
	do {
		timedelay(1000);
		tmp = tvia_inb(0x3da);
	} while((tmp & 0x08) != 0);
	do {
		timedelay(1000);
		tmp = tvia_inb(0x3da);
	} while((tmp & 0x08) == 0);
	do {
		timedelay(1000);
		tmp = tvia_inb(0x3da);
	} while((tmp & 0x08) != 0);
#else
    while ((tvia_inb(0x3da) & 0x08) == 0x00);   /*search until 1 */
    while ((tvia_inb(0x3da) & 0x08) != 0x00);   /*search until 0  active low */
    while ((tvia_inb(0x3da) & 0x08) == 0x00);   /*search until 1 */
    while ((tvia_inb(0x3da) & 0x08) != 0x00);   /*search until 0  active low */
#endif
    /*
       if (E454<8>=1)
       the color is O.K., do nothing.
       else
       the Color needs to be toggled through E46C<0>
     */
    for (i = 0; i < 10; i++)
        udelay(1000);

    tmp = tvia_inw(0xbE454);

    /* set test logic back to normal */
    tvia_outw(0x0000, 0xbE604);
    tvia_outw(0x0000, 0xbE606);

    tvia_outw(tmp_e440, 0xbe440);       /*restore them */
    tvia_outw(tmp_e448, 0xbe448);
    tvia_outw(tmp_e430, 0xbe430);


#if 1
    /* clear FSC: 3CE_4E<1>=0 and toggle TV_REG_E46C<0> accordingly */
    tvia_grphw(0x05, 0xFA);
    tvia_outb(0x4E, 0x3ce);
    tvia_outb((tvia_inb(0x3cf) & ~0x02), 0x3cf);

    if ((tmp & 0x0100) == 0x0000)
        tvia_outw(tvia_inw(0xbE46C) ^ 0x0001, 0xbe46c);

#else                           /*old code still work */
    /* clear FSC: 3CE_4E<1>=0 and toggle 3CE_4E<3> accordingly */
    if ((tmp & 0x0100) == 0x0100)
        tmp = 0x00;
    else
        tmp = 0x08;             /*need to toggle color bit<3> */

    tvia_outb(0xfa, 0x3ce);
    tvia_outb(0x05, 0x3cf);
    tvia_outb(0x4e, 0x3ce);
    tvia_outb((tvia_inb(0x3cf) & ~0x02) ^ ((u8) tmp), 0x3cf);
#endif

#if 0
    tvia_outb(0xfa, 0x3ce);
    tvia_outb(0x00, 0x3cf);
#endif
}

static void SetScartTV(u16 iOnOff)
{
    if (iOnOff == ON) {
        tvia_outb(0xFA, 0x3ce);
        tvia_outb(0x05, 0x3cf); /*banking register */
        tvia_outb(0x4E, 0x3ce);

	  tvia_outb(tvia_inb(0x3cf) | 0x90, 0x3cf);       /*bits 7 and 4 */
// !!!raf  trace("SetScartTV: Disable DAC RGB out");
// !!!raf  tvia_outb(tvia_inb(0x3cf) | 0xB0, 0x3cf);       /*bits 7, 5 and  4 */
        //tvia_outb(0x16, 0x3ce);
        //tvia_outb(0x0E, 0x3cf);

		WriteTVReg(0xE61E, 0x03);
        WriteTVReg(0xE61C, 0x00);
        WriteTVReg(0xE464, 0x5A);
        WriteTVReg(0xE524, 0x5B);
        WriteTVReg(0xE55A, 0x2500);
        WriteTVReg(0xE55E, 0x2500);
        WriteTVReg(0xE438, ReadTVReg(0xE438) | 0x8000);
        WriteTVReg(0xE626, 0x00);
        tvia_outb(0x16, 0x3ce);
        tvia_outb(0x0E, 0x3cf);
    } else {
        tvia_outb(0xFA, 0x3ce);
        tvia_outb(0x05, 0x3cf); /*banking register */
        tvia_outb(0x4E, 0x3ce);
        tvia_outb(tvia_inb(0x3cf) & ~0x90, 0x3cf);      /*bits 7 and 4 */
        tvia_outb(0x16, 0x3ce);
        tvia_outb(0x00, 0x3cf);
    }
}

static void ProgramTV(void)
{
    TVRegisters *bpTVReg;
    u16 reg_tv_size;

  /*-------------------------------------
  |*  TV Out Programming  
  -------------------------------------*/
    if ( (init_var.xres==640 && init_var.yres==480) \ 
         || (init_var.xres==720 && init_var.yres==576)  \
         || (init_var.xres==640 && init_var.yres==440) ) {

	trace("ProgramTv: init_mode=%d\n",init_mode);
    bpTVReg = TVModeReg_5202_SDRAM[init_mode];
//        reg_tv_size = REG_TV_SIZE_5202; //Wei  sizeof(bpTVReg->TVRegs);
//        reg_tv_size = sizeof(TVModeReg_5202_SDRAM[INIT_MODE].TVRegs);
        reg_tv_size = reg_tv_size_5202; //REG_TV_SIZE_5202; //Wei  sizeof(bpTVReg->TVRegs);

        if(bpTVReg==NULL) {
            EnableTV(OFF);
            return;
        }
        SetTVReg(bpTVReg->TVRegs, reg_tv_size);

	/* From SDK1/SETMODE/SRC/settv.c */
        if (init_var.xres==640 && init_var.yres==440) {
            /* 640x440 screen needs a little centering adjustment
            WriteTVReg(0xE468, ReadTVReg(0xE468) + 2);
            WriteTVReg(0xE46C, ReadTVReg(0xE46C) + 2);    Don't need this in 5300SDK1 */
        }

        EnableTV(ON);
        TVOn(ON);
        SetScartTV(OFF);
    } else                      /*Turn TV DAC off */
        EnableTV(OFF);
}

/*--------------------------------------------------------------------
  EnableDigitalCursor: turn on or turn off the cursor when digital
                     output is selected
              In: onoff - ON:  turn on cursor
                         OFF: turn off cursor
             Out:
  --------------------------------------------------------------------*/
static void EnableDigitalCursor(u16 iOnOff)
{
    if (iOnOff == ON)
    {
        tvia_outb(0xfa, 0x3ce);   /* banking */
        tvia_outb(0x20, 0x3cf);
        tvia_outb(0x46, 0x3c4);
        tvia_outb(0x09, 0x3c5);   /* enable alpha blending hardware cursor*/
#if 0
        if(init_var.bits_per_pixel >= 16)
        {
            WriteTVReg(0xE60C, ReadTVReg(0xE60C) & ~0x02);
            WriteTVReg(0xE678, 0x0000);
            tvia_outb(0xfa, 0x3ce);   /* banking */ 
            tvia_outb(0x05, 0x3cf);
            tvia_outb(0x4e, 0x3ce);
            tvia_outb(tvia_inb(0x3cf) & ~0x10, 0x3cf);
        }
        else
        {
            WriteTVReg(0xE60C, ReadTVReg(0xE60C) | 0x02);
            WriteTVReg(0xE678, 0x0124);
            tvia_outb(0xfa, 0x3ce);   /* banking */
            tvia_outb(0x05, 0x3cf);
            tvia_outb(0x4e, 0x3ce);
            tvia_outb(tvia_inb(0x3cf) | 0x10, 0x3cf);
        }        
#endif
        /* set hardware cursor inside color to white */
        tvia_outb(0xfa, 0x3ce);
        tvia_outb(0x20, 0x3cf);
        tvia_outb(0x43, 0x3c4);
        tvia_outb(0xff, 0x3c5);
        tvia_outb(0x44, 0x3c4);
        tvia_outb(0xff, 0x3c5);
        tvia_outb(0x45, 0x3c4);
        tvia_outb(0xff, 0x3c5);
        tvia_outb(0xfa, 0x3ce);
        tvia_outb(0x00, 0x3cf);
        /* cursor horizontal position */
        tvia_outb(0x57, 0x3ce);
        tvia_outb(0x19, 0x3cf);
    }
    else
    {
        tvia_outb(0xfa, 0x3ce);
        tvia_outb(0x20, 0x3cf);
        tvia_outb(0x46, 0x3c4);
        tvia_outb(0x00, 0x3c5);
        tvia_outb(0x43, 0x3c4);
        tvia_outb(0x00, 0x3c5);
        tvia_outb(0x44, 0x3c4);
        tvia_outb(0x00, 0x3c5);
        tvia_outb(0x45, 0x3c4);
        tvia_outb(0x00, 0x3c5);
        tvia_outb(0xfa, 0x3ce);
        tvia_outb(0x00, 0x3cf);
    }
}

/*----------------------------------------------------------------------
  EnableDigitalRGB: Turn on or off digital RGB out
  In:  iOnOff: ON - enable digital RGB out
               OFF- disable digital RGB out
  Out: None
 -----------------------------------------------------------------------*/
static void EnableDigitalRGB(u16 iOnOff)
{
    if (iOnOff == ON) 
    {
        /* enable digital output */
        tvia_outb(0x1e, 0x3ce);
        tvia_outb(0x67, 0x3cf);
        tvia_outb(0x1f, 0x3ce);
        tvia_outb(0x84, 0x3cf);
        /* correct the color output for <= 8bpp case*/
        if(init_var.bits_per_pixel >= 16)
        {
            tvia_outb(0xfa, 0x3ce);
            tvia_outb(0x10, 0x3cf);
            tvia_outb(0x4f, 0x3c4);
            tvia_outb(0x00, 0x3c5);
            tvia_outb(0xfa, 0x3ce);
            tvia_outb(0x28, 0x3cf);
            tvia_outb(0x4c, 0x3c4);
            tvia_outb(0x00, 0x3c5);
            tvia_outb(0xfa, 0x3ce);
            tvia_outb(0x20, 0x3cf);
            tvia_outb(0x47, 0x3c4);
            tvia_outb(0x10, 0x3c5);

            WriteTVReg(0xE60C, ReadTVReg(0xE60C) & ~0x02);
            WriteTVReg(0xE678, 0x0000);
            tvia_outb(0xfa, 0x3ce);   /* banking */
            tvia_outb(0x05, 0x3cf);
            tvia_outb(0x4e, 0x3ce);
            tvia_outb(tvia_inb(0x3cf) & ~0x10, 0x3cf);   /* Disable Scart RGB output */      
        }
        else
        {
            tvia_outb(0xfa, 0x3ce);
            tvia_outb(0x10, 0x3cf);
            tvia_outb(0x4f, 0x3c4);
            tvia_outb(0x81, 0x3c5); /* select 8bits index alpha blending */
            tvia_outb(0xfa, 0x3ce);
            tvia_outb(0x28, 0x3cf);
            tvia_outb(0x4c, 0x3c4); /* alpha pipeline adjustment */
            tvia_outb(0x80, 0x3c5);
            tvia_outb(0xfa, 0x3ce);
            tvia_outb(0x20, 0x3cf);
            tvia_outb(0x47, 0x3c4);
            tvia_outb(0x14, 0x3c5); /* data pass data path is from sram palette */

	   		WriteTVReg(0xE60C, ReadTVReg(0xE60C) | 0x02);
            WriteTVReg(0xE678, 0x0124);
            tvia_outb(0xfa, 0x3ce);   /* banking */
            tvia_outb(0x05, 0x3cf);
            tvia_outb(0x4e, 0x3ce);
            tvia_outb(tvia_inb(0x3cf) | 0x10, 0x3cf);  /* Enable Scart RGB output */
		}
    }
    else
    {
        tvia_outb(0x1e, 0x3ce);
        tvia_outb(0x00, 0x3cf); 
        tvia_outb(0x1f, 0x3ce);
        tvia_outb(0x00, 0x3cf); // RGB digital port disabling
        tvia_outb(0xfa, 0x3ce);
        tvia_outb(0x10, 0x3cf);
        tvia_outb(0x4f, 0x3c4);
        tvia_outb(0x00, 0x3c5);
        tvia_outb(0xfa, 0x3ce);
        tvia_outb(0x28, 0x3cf);
        tvia_outb(0x4c, 0x3c4);
        tvia_outb(0x00, 0x3c5);
        tvia_outb(0xfa, 0x3ce);
        tvia_outb(0x20, 0x3cf);
        tvia_outb(0x47, 0x3c4);
        tvia_outb(0x00, 0x3c5);
    }
}

/*------------------------------------------------------
*
*   	Misc founctions needed by frame buffer operations functions
*
*------------------------------------------------------*/
static int
tviafb_decode_var(struct fb_var_screeninfo *var, int *visual,
                  struct sModeParam *sm0)
{
    int i;
    mode_table *modetable;
    int total_num;
    int xres, yres, bpp, clock;

    xres = var->xres;
    yres = var->yres;

    bpp = var->bits_per_pixel;

    dbg("tviafb_decode_var: start");

    if (var->pixclock == 50000)
        clock = 50;
    else if (var->pixclock == 60000)
        clock = 60;
    else
        clock = var->pixclock;
    if (clock != 50 && clock != 60)
        clock = 50;

    switch (var->bits_per_pixel) {
#ifdef FBCON_HAS_CFB8
    case 8:
        *visual = FB_VISUAL_PSEUDOCOLOR;
        break;
#endif
#ifdef FBCON_HAS_CFB16
    case 16:
	dbg("tviafb_decode_var: CFB16");
        *visual = FB_VISUAL_TRUECOLOR;
        break;
#endif
#ifdef FBCON_HAS_CFB24
    case 24:
        *visual = FB_VISUAL_TRUECOLOR;
        break;
#endif
    default:
        return -EINVAL;
    }

    /*underscan--don't support */
    /*int underscan; */
    modetable = Cyberpro_5202;
    total_num = sizeof(Cyberpro_5202) / sizeof(mode_table);

    if (total_num <= 0)
        return -EINVAL;

#ifdef CONFIG_FBCON_CFB16 
    dbg("tviafb_decode_var: CONFIG_FBCON_CFB16 defined");
#endif 

    /*find the mode we needed--i is the modenum */
    for (i = 0; i < total_num; i++) {
        if (xres == (modetable + i)->xres && yres == (modetable + i)->yres
            && bpp == (modetable + i)->bpp
            && clock == (modetable + i)->clock)
            break;
    }

    dbg("tviafb_decode_var: mode=%d,total_num=%d",i,total_num);

    /*underscan--don't support */
    /*if(underscan) */
    if (i >= total_num) {
        for (i = 0; i < total_num; i++) {
            if (xres == (modetable + i)->xres
                && yres == (modetable + i)->yres
                && bpp == (modetable + i)->bpp)
                break;
        }
    }


    if (i >= total_num)
        return -EINVAL;

    dbg("tviafb_decode_var: ok mode=%d",i);

    clock = (modetable + i)->clock;

    sm0->wModeID = i;
    if ((clock == 50 || clock == 60) && (var->xres != 1024))
        sm0->wTVOn = ON;
    else
        sm0->wTVOn = OFF;
    sm0->wCRTOn = ON;
    sm0->bpp = bpp;

    if (var->xres_virtual < var->xres)
        var->xres_virtual = var->xres;
    if (var->yres_virtual < var->yres)
        var->yres_virtual = var->yres;

    var->pixclock = clock;

    return 0;
}


static inline void tviafb_update_start(struct fb_var_screeninfo *var)
{
}


/*--------------------------------------------------------
*
*	Frame buffer operations
*	
*---------------------------------------------------------*/

static int tviafb_open(struct fb_info *info, int user)
{
    MOD_INC_USE_COUNT;
    return 0;
}

static int tviafb_release(struct fb_info *info, int user)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

static int
tviafb_get_fix(struct fb_fix_screeninfo *fix, int con,
               struct fb_info *info)
{
    struct display *display;

    memset(fix, 0, sizeof(struct fb_fix_screeninfo));
    strcpy(fix->id, "CyberPro5k");

    if (con >= 0)
        display = fb_display + con;
    else
        display = &global_disp;

    fix->smem_start = current_par.screen_base_p;
    fix->smem_len = current_par.screen_size;
    fix->mmio_start = current_par.regs_base_p;
    fix->mmio_len = 0x00100000;
    fix->type = display->type;
    fix->type_aux = display->type_aux;
    fix->xpanstep = 0;
    fix->ypanstep = display->ypanstep;
    fix->ywrapstep = display->ywrapstep;
    fix->visual = display->visual;
    fix->line_length = display->line_length;
    fix->accel = 22;
/*
    if (current_par.device_id == 0x5000)
        fix->accel = FB_ACCEL_IGS_CYBER5000;
    else
        fix->accel = FB_ACCEL_NONE;
*/
    return 0;
}

static int
tviafb_get_var(struct fb_var_screeninfo *var, int con,
               struct fb_info *info)
{
    if (con == -1)
        *var = global_disp.var;
    else
        *var = fb_display[con].var;

    *var = init_var;

    return 0;
}

static int
tviafb_set_var(struct fb_var_screeninfo *var, int con,
               struct fb_info *info)
{
    struct display *display;
    int err, chgvar = 0, visual;
    struct sModeParam sm0;

    dbg("tvia_fb_get_var started");

    if (con >= 0)
        display = fb_display + con;
    else
        display = &global_disp;

    dbg("tvia_fb_get_var started 1");

    err = tviafb_decode_var(var, &visual, &sm0);
    if (err)
        return err;

    dbg("tvia_fb_get_var started 1.5"); 

    switch (var->activate & FB_ACTIVATE_MASK) {
    case FB_ACTIVATE_TEST:
        return 0;

    case FB_ACTIVATE_NXTOPEN:
    case FB_ACTIVATE_NOW:
        break;

    default:
        return -EINVAL;
    }

    dbg("tvia_fb_get_var started 2");

    if (con >= 0) {
        if (display->var.xres != var->xres)
            chgvar = 1;
        if (display->var.yres != var->yres)
            chgvar = 1;
        if (display->var.xres_virtual != var->xres_virtual)
            chgvar = 1;
        if (display->var.yres_virtual != var->yres_virtual)
            chgvar = 1;
        if (display->var.accel_flags != var->accel_flags)
            chgvar = 1;
        if (memcmp(&display->var.red, &var->red, sizeof(var->red)))
            chgvar = 1;
        if (memcmp(&display->var.green, &var->green, sizeof(var->green)))
            chgvar = 1;
        if (memcmp(&display->var.blue, &var->blue, sizeof(var->green)))
            chgvar = 1;
    }

    display->var = *var;

    display->screen_base = (char *) current_par.screen_base;
    display->visual = visual;
    display->type = FB_TYPE_PACKED_PIXELS;
    display->type_aux = 0;
    display->ypanstep = 0;
    display->ywrapstep = 0;
    display->line_length =
        display->next_line = (var->xres_virtual * var->bits_per_pixel) / 8;
    display->can_soft_blank = 1;
    display->inverse = 0;

    var->red.msb_right = 0;
    var->green.msb_right = 0;
    var->blue.msb_right = 0;

    switch (display->var.bits_per_pixel) {
#ifdef FBCON_HAS_CFB8
    case 8:
	dbg ("tviafb_set_var: setting fbcon_cfb8 to %p",fbcon_cfb8);
        dispsw = &fbcon_cfb8;
        display->dispsw_data = NULL;
        var->bits_per_pixel = 8;
        var->red.offset = 0;
        var->red.length = 8;
        var->green.offset = 0;
        var->green.length = 8;
        var->blue.offset = 0;
        var->blue.length = 8;
        break;
#endif
#ifdef FBCON_HAS_CFB16
    case 16:
        dbg ("tviafb_set_var: setting fbcon_cfb16",fbcon_cfb16);
        dispsw = &fbcon_cfb16;
        display->dispsw_data = current_par.c_table.cfb16;
        var->bits_per_pixel = 16;
        var->red.offset = 11;
        var->red.length = 5;
        var->green.offset = 5;
        var->green.length = 6;
        var->blue.offset = 0;
        var->blue.length = 5;
        break;
#endif
#ifdef FBCON_HAS_CFB24
    case 24:
        dispsw = &fbcon_cfb24;
        display->dispsw_data = current_par.c_table.cfb24;
        var->bits_per_pixel = 24;
        var->red.offset = 16;
        var->red.length = 8;
        var->green.offset = 8;
        var->green.length = 8;
        var->blue.offset = 0;
        var->blue.length = 8;
        break;
#endif
    default:
        printk(KERN_WARNING "CyberPro5k: no support for %dbpp\n",
               display->var.bits_per_pixel);
        dispsw = &fbcon_dummy;
        break;
    }

    if (display->var.accel_flags & FB_ACCELF_TEXT &&
        dispsw != &fbcon_dummy) {
        display->dispsw = &fbcon_cyber_accel;
	dbg("tvia_fb_set_var: setting &fbcon_cyber_accel"); 
    }
    else {
        display->dispsw = dispsw;
        dbg("tvia_fb_set_var: setting dispsw");
    }

    if (chgvar && info && info->changevar)
        info->changevar(con);

    if (con == current_par.currcon) {
        struct fb_cmap *cmap;

        tviafb_update_start(var);
        //tviafb_set_timing(var, &sm0);

        if (display->cmap.len)
            cmap = &display->cmap;
        else {
            cmap = fb_default_cmap(current_par.palette_size);
		}

        fb_set_cmap(cmap, 1, tvia_setcolreg, info);
    }
    return 0;
}

static int
tviafb_get_cmap(struct fb_cmap *cmap, int kspc, int con,
                struct fb_info *info)
{
    int err = 0;

    if (con == current_par.currcon)     /* current console? */
        err = fb_get_cmap(cmap, kspc, tvia_getcolreg, info);
    else if (fb_display[con].cmap.len)  /* non default colormap? */
        fb_copy_cmap(&fb_display[con].cmap, cmap, kspc ? 0 : 2);
    else
        fb_copy_cmap(fb_default_cmap
                     (1 << fb_display[con].var.bits_per_pixel), cmap,
                     kspc ? 0 : 2);
    return err;
}

static int
tviafb_set_cmap(struct fb_cmap *cmap, int kspc, int con,
                struct fb_info *info)
{
    struct display *disp = &fb_display[con];
    int err = 0;

    if (!disp->cmap.len) {      /* no colormap allocated? */
        int size;

        if (disp->var.bits_per_pixel == 16)
            size = 32;
        else
            size = 256;

        err = fb_alloc_cmap(&disp->cmap, size, 0);
    }
    if (!err) {
        if (con == current_par.currcon) /* current console? */
            err = fb_set_cmap(cmap, kspc, tvia_setcolreg, info);
        else
            fb_copy_cmap(cmap, &disp->cmap, kspc ? 0 : 1);
    }

    return err;
}

static int tviafb_pan_display(struct fb_var_screeninfo *var, int con,
                              struct fb_info *info)
{
    u_int y_bottom;

    y_bottom = var->yoffset;

    if (!(var->vmode & FB_VMODE_YWRAP))
        y_bottom += var->yres;

    if (var->xoffset > (var->xres_virtual - var->xres))
        return -EINVAL;
    if (y_bottom > fb_display[con].var.yres_virtual)
        return -EINVAL;
    return -EINVAL;

    tviafb_update_start(var);

    fb_display[con].var.xoffset = var->xoffset;
    fb_display[con].var.yoffset = var->yoffset;
    if (var->vmode & FB_VMODE_YWRAP)
        fb_display[con].var.vmode |= FB_VMODE_YWRAP;
    else
        fb_display[con].var.vmode &= ~FB_VMODE_YWRAP;

    return 0;
}

void Tvia_TestVideo(u8 type){

   trace("TestVideo: 3cf/58=0x0f and after 0xff");
   tvia_outb(0x58,0x3ce);
   udelay(1000);
   tvia_outb(0x0f,0x3cf);
   udelay(1000);
   tvia_outb(0xff,0x3cf);
   udelay(1000);

}

/*  */
void  Fb_line_copy(u8 val){
}

/* Entry point for user defined ioctl */
void My_Tvia_ioctl(u8 type,u8 index,u8 val, u16 indexword, u16 valword){
   int idx;
   u8 tmp;
   u8 iTmpFA,iTmpF7,iTmp; 
   u16 val16;

  if (type==1) 
  {
   trace("Starting DumpTotalTest");
   trace("Dumping Tvia Cyberpro 5202");
	
   tmp=In_Video_Reg(0x33);
   tmp|=0x40;
   Out_Video_Reg(0x33,tmp);
   trace("BYTE enable mode: 3cf/33[6].3cf/3c = %x",In_Video_Reg(0x3c));
   tmp=In_Video_Reg(0x33);
   tmp &= ~0x40;
   Out_Video_Reg(0x33,tmp);
	
	
   trace("3cc=%x",InByte(0x3CC));
   trace("3c2=%x",InByte(0x3C2));


  //   trace("3ba=%x",InByte(0x3ba));
   //   trace("3da=%x",InByte(0x3da));
   //   trace("3c3=%x",InByte(0x3ce));
   //   trace("46e8=%x",InByte(0x46e8));

   trace("CRT Registers");
   for(idx=0x10;idx<=0x1f;idx++)
     trace("3cf/%x=%x",idx,In_Video_Reg(idx));

   trace("BUS INTERFACE UNIT REGISTERS");
   for(idx=0x30;idx<=0x3f;idx++)
     trace("3cf/%x=%x",idx,In_Video_Reg(idx));

   trace("ATTRIBUTE REGISTERS");
   for(idx=0x50;idx<=0x5f;idx++)
     trace("3cf/%x=%x",idx,In_Video_Reg(idx));

   trace("SEQUENCER REGISTERS");
   for(idx=0x70;idx<=0x7f;idx++)
     trace("3cf/%x=%x",idx,In_Video_Reg(idx));

   trace("GRAPHICS CONTROL REGISTERS");
   for(idx=0x90;idx<=0x9E;idx++)
     trace("3cf/%x=%x",idx,In_Video_Reg(idx));

   trace("VIDEO AND MEMORY CLOCK REGISTERS");
   for(idx=0xb0;idx<=0xbf;idx++)
     trace("3cf/%x=%x",idx,In_Video_Reg(idx));
   Out_Video_Reg(0xbf,0x01);
   for(idx=0xb0;idx<=0xbe;idx++)
     trace("3cf/bf01.3cf/%x=%x",idx,In_Video_Reg(idx));
   Out_Video_Reg(0xbf,0x02);
   for(idx=0xb0;idx<=0xba;idx++)
     trace("3cf/bf02.3cf/%x=%x",idx,In_Video_Reg(idx));
   Out_Video_Reg(0xbf,0x03);
   for(idx=0xb0;idx<=0xbc;idx++)
    trace("3cf/bf03.3cf/%x=%x",idx,In_Video_Reg(idx));
 }
 else if (type==2) // Video 3cf/
 {
   trace("Read 3cf/%x=%x",index,In_Video_Reg(index));
 }
 else if (type==3) // SEQ 3c5/ 
 {	
   trace("Read 3c5");
   trace("Read 3c5/%x=%x",index,In_SEQ_Reg(index));
 }
 else if (type==4) // CRT
 {
   trace("Read 3d5");
   *((volatile unsigned char *)(CyberRegs + 0x3d4)) = index;
   trace("Read 3d5-1");
   val=tvia_inb(0x3d5); 
	 trace("Read 3d5/%x=%x",index,val); // 
//   trace("Read 3d5/%x=%x",index,In_CRT_Reg(index));
 }
 else if (type==5) // WR Video 3cf/
 {
   Out_Video_Reg(index,val);
   trace("Written 3cf/%x=%x",index,In_Video_Reg(index));
 }
 else if (type==6) // WR SEQ 3c5/ 
 {	
   Out_SEQ_Reg(index,val);
   trace("Written 3c5/%x=%x",index,In_SEQ_Reg(index));
 }
 else if (type==7) // WR CRT
 {
   Out_CRT_Reg(index,val);
   trace("Written 3d5/%x=%x",index,In_CRT_Reg(index));
 }
 else if (type==8) // Video banking registers 3cf/
 {
   trace("3cf/fa05.3cf/4e");
   
   tvia_outb(0xFA, 0x3ce);     /*Banking I/O control */
   iTmpFA = tvia_inb(0x3cf);
   tvia_outb(0x05, 0x3cf);
   
   tvia_outb(0x4E, 0x3ce);
   iTmp = tvia_inb(0x3cf);
   trace("3cf/fa05.3cf/4e=%x",iTmp);
   iTmp = val;//(iTmp|0x25)&0x2f;
   trace("3cf/fa05.3cf/4e=%x",iTmp);
   tvia_outb(iTmp, 0x3cf);

   tvia_outb(0xFA, 0x3ce);     /*Banking I/O control */
   tvia_outb(iTmpFA, 0x3cf);
   
   trace("done");
 }
 else if (type==9) // Reading RGB DAC enabling bit
 {
   trace("3cf/bf02.3cf/b1");
   
   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
   iTmpFA = tvia_inb(0x3cf);
   tvia_outb(0x02, 0x3cf);
   
   tvia_outb(0xB1, 0x3ce);
   iTmp = tvia_inb(0x3cf);
   trace("3cf/bf02.3cf/b1=%x",iTmp);

   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
   tvia_outb(iTmpFA, 0x3cf);
   
   trace("done");
 }
 else if (type==10) // Changing RGB DAC enabling bit
 {
   trace("3cf/bf02.3cf/b1");
   
   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
   iTmpFA = tvia_inb(0x3cf);
   tvia_outb(0x02, 0x3cf);
   
   tvia_outb(0xB1, 0x3ce);
   iTmp = tvia_inb(0x3cf);
   trace("3cf/bf02.3cf/b1=%x",iTmp);
   iTmp = (iTmp&0xc0)|(val&0x3f);
   trace("3cf/fa05.3cf/b1=%x",iTmp);
   tvia_outb(iTmp, 0x3cf);

   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
   tvia_outb(iTmpFA, 0x3cf);
   
   trace("done");
 }
 else if (type==11) // Dumping TVReg
 {
   trace("Dumping with ReadTVReg %x",indexword);
   val16=ReadTVReg(indexword);
   trace("ReadTVReg: reg[%x]=%x",indexword,val16);
 }
 else if (type==12) // Writing TVReg
 {
   trace("Writing WriteTVReg %x equal to %x",indexword,valword);
   WriteTVReg(indexword,valword);
   val16=ReadTVReg(indexword);
   trace("ReadTVReg: reg[%x]=%x",indexword,val16);
 }
 else if (type==13) // Changing interpolation
 {
   //-Bank is 0 
   //-3cf/db[4:3] = 00 :  scale up , 0.5 Linearity approach  
   //-3cf/db[4:3] = 01 :  scale up , Duplicate previouse pixel  
	 
   iTmpF7 = In_Video_Reg(0xF7);
   iTmpFA = In_Video_Reg(0xFA);
   Out_Video_Reg(0xF7, 0x00);
   Out_Video_Reg(0xFA, 0x00);

	 if (val==0)
     Out_Video_Reg_M(0xDB,0x00,~0x18); //3cf/db[4:3] = 00 :  scale up , 0.5 Linearity approach  
	 else 
     Out_Video_Reg_M(0xDB,0x08,~0x18); //3cf/db[4:3] = 01 :  scale up , Duplicate previouse pixel  
	 
   /* Restore banking */
   Out_Video_Reg(0xF7, iTmpF7);
   Out_Video_Reg(0xFA, iTmpFA);
 }
 else if (type==14) // double buffer
 {
	 Tvia_EnableDoubleBuffer(ON);
 }
 else if (type==15) // Graphic frame buffer: copying 2nd line to first, 4th to 3rd and so on ..
 {
  	Fb_line_copy(val);
 }


/* else if (type==8) // read normal
 {
   Out_CRT_Reg(index,val));
   trace("Written 3d5/%x=%x",index,In_CRT_Reg(index));
 }
 else if (type==7) // write normal
 {
   Out_CRT_Reg(index,val));
   trace("Written 3d5/%x=%x",index,In_CRT_Reg(index));
 }
/bin/bash: nv: command not found
 {
//   u16 InTVReg(u32 dwReg)
//{
//	return *((volatile unsigned short *)(CyberRegs + dwReg + 0x000B0000));
//}

   trace("Read TV reg %x = %x",index,InTVReg(index)); 
 }
 else if (type==7) // write normal
 {
//   void OutTVReg(u32 dwReg, u16 wIndex){
//     *((volatile unsigned short *)(CyberRegs + dwReg + 0x000B0000)) = wIndex;
//   } 
   OutTVReg(index,val);
   trace("Written 3d5/%x",index,In_CRT_Reg(index));
 }
 */

}


/*-----------------------------------------------------------------------------
  tvia_testvmem: To test 32 bytes video memory.
  In:  iBankNum: 0 - 63  (one bank = 64 KB, maximum 64 banks = 4 MB)
  Out: Return 0 if pass or non-zero if fail
-----------------------------------------------------------------------------*/
static unsigned short tvia_testvmem(unsigned short banknum)
{
    unsigned short i,flag;
    unsigned char data0, data1;
    unsigned char temp0, temp1;
    unsigned char      *fptemp;

    fptemp = (unsigned char *)current_par.screen_base;
    fptemp += banknum*64*1024;

    flag = 0;

    data0 = 0x55;
    data1 = 0xAA;
    for (i=0; i<16; i++)
    {
     *fptemp++ = data0;
     *fptemp++ = data1;
    }


    fptemp = (unsigned char *)current_par.screen_base;

    fptemp += banknum*64*1024;

    for (i=0; i<16; i++)
    {
      temp0 = *fptemp++;
      temp1 = *fptemp++;
      if ( (temp0 != data0) || (temp1 != data1) )
	flag = 1;
    }

    return flag;
}

// CARLOS DMA
/*
 * Our DMA interrupt handlers
 */
static void video_rdma_irq(int ch, void *dev_id, struct pt_regs *regs)
{
	TVIA5202_DMACONF *pconf = &RConf;
	
	if (DCSR(r_dma_ch) & 0x00000004){
		DCSR(r_dma_ch) |= 0x00000004; // Clear END Interrupt Bit
		pconf->height--;
		if (pconf->height > 0){	
			DCMD(r_dma_ch) = 0xC0230000 | (pconf->width & 0x00001FFF);
	
			/* Lanza acceso DMA */
			DCSR(r_dma_ch) |= DCSR_RUN;
		} else{
			wake_up_interruptible(&r_queue);
		}
	}
	
	if (DCSR(r_dma_ch) & 0x00000001){
		trace(" VIDEO DMA READ ERROR INT ");
		trace("DTADR=%x DSADR=%x DCSR=%x DCMD=%x ", DTADR(r_dma_ch), DSADR(r_dma_ch),DCSR(r_dma_ch), DCMD(r_dma_ch));
		DCSR(r_dma_ch) |= 0x00000001; // Clear ERROR Interrupt Bit
	}
}

static void video_wdma_irq(int ch, void *dev_id, struct pt_regs *regs)
{
	TVIA5202_DMACONF *pconf = &WConf;
	
	if (DCSR(w_dma_ch) & 0x00000004){
		DCSR(w_dma_ch) |= 0x00000004; // Clear END Interrupt Bit
		pconf->height--;
		if (pconf->height > 0){	
			pconf->dtadr += pconf->pitch;
			DTADR(w_dma_ch) = pconf->dtadr; /* Alineada a 8 bytes (2:0 = 000)*/
	
			DCMD(w_dma_ch) = 0xC0230000 | (pconf->width & 0x00001FFF);	
	
			/* Lanza acceso DMA */
			DCSR(w_dma_ch) |= DCSR_RUN;
		} else{
			wake_up_interruptible(&w_queue);
		}		
	}
	else if (DCSR(w_dma_ch) & 0x00000001){		
		trace(" VIDEO DMA WRITE ERROR INT.");
		trace("DTADR=%x DSADR=%x DCSR=%x DCMD=%x", DTADR(w_dma_ch), DSADR(w_dma_ch),DCSR(w_dma_ch), DCMD(w_dma_ch));
		DCSR(w_dma_ch) |= 0x00000001; // Clear ERROR Interrupt Bit
	}
}

static int Tvia_DMAReadFrame()
{	
	TVIA5202_DMACONF *pconf = &RConf;
	
	//Controla que DMA channel esta parado
	if ((DCSR(r_dma_ch) & DCSR_STOPSTATE) == 0){
		trace("Running Previous Video Read DMA Access. DMA CHANNEL=%d", r_dma_ch);	 
		return 1;
	}
		
	/* Config sig. acceso DMA */
	DCSR(r_dma_ch) = DCSR_NODESC;	/* Configure NO DESC MODE */
	
	DTADR(r_dma_ch) = virt_to_bus(dsta);
	//DTADR(r_dma_ch) = 0xa3f00000 + 1*320; // RAFFA PROC BUFFER MEM PXA (DMA)
	 
	DSADR(r_dma_ch) = pconf->dsadr; /* Alineada a 8 bytes (2:0 = 000)*/
	//dsta = (int) ((int)&dst[7] & 0xFFFFFFF8);
	//DSADR(r_dma_ch) = virt_to_bus(dsta);
	
	DCMD(r_dma_ch) = 0xC0230000 | (pconf->width & 0x00001FFF);
	
	/* Lanza acceso DMA */
	DCSR(r_dma_ch) |= DCSR_RUN;
	
	wait_event_interruptible(r_queue, RConf.height==0);
	if(signal_pending(current)){
		return(-ERESTARTSYS);
	}		
	
	return 0;
}


static int Tvia_DMAWriteFrame()
{	
	TVIA5202_DMACONF *pconf = &WConf;
	
	//Controla que DMA channel esta parado
	if ((DCSR(w_dma_ch) & DCSR_STOPSTATE) == 0){
		trace("Running Previous Video Write DMA Access. DMA CHANNEL=%d", w_dma_ch);	 
		return 1;
	}	
	
	/* Config sig. acceso DMA */
	DCSR(w_dma_ch) = DCSR_NODESC;	/* Configure NO DESC MODE */
	
	DSADR(w_dma_ch) = virt_to_bus(orga);
	//DSADR(w_dma_ch) = virt_to_bus(pconf->dsadr);
	
	DTADR(w_dma_ch) = pconf->dtadr; /* Alineada a 8 bytes (2:0 = 000)*/
	
	DCMD(w_dma_ch) = 0xC0230000 | (pconf->width & 0x00001FFF);
	
	/* Lanza acceso DMA */
	DCSR(w_dma_ch) |= DCSR_RUN;
	
	wait_event_interruptible(w_queue, WConf.height==0);
	if(signal_pending(current)){
		return(-ERESTARTSYS);
	}
		
	return 0;
}
// END CARLOS DMA


static int tviafb_ioctl(struct inode *inode, struct file *file,
                        u_int cmd, u_long arg, int con,
                        struct fb_info *info)
{
    int i;

    switch (cmd) {
    case FBIOGET_FCURSORINFO:{
            struct fb_fix_cursorinfo crsrfix;

            i = verify_area(VERIFY_WRITE, (void *) arg, sizeof(crsrfix));
            if (!i) {
                i = tviafb_get_fix_cursorinfo(&crsrfix, con);
                copy_to_user((void *) arg, &crsrfix, sizeof(crsrfix));
            }
            return i;
        }
    case FBIOGET_VCURSORINFO:{
            struct fb_var_cursorinfo crsrvar;

            i = verify_area(VERIFY_WRITE, (void *) arg, sizeof(crsrvar));
            if (!i) {
                i = tviafb_get_var_cursorinfo(&crsrvar,
                                              ((struct fb_var_cursorinfo *)
                                               arg)->data, con);
                copy_to_user((void *) arg, &crsrvar, sizeof(crsrvar));
            }
            return i;
        }
    case FBIOPUT_VCURSORINFO:{
            struct fb_var_cursorinfo crsrvar;

            i = verify_area(VERIFY_READ, (void *) arg, sizeof(crsrvar));
            if (!i) {
                copy_from_user(&crsrvar, (void *) arg, sizeof(crsrvar));
                i = tviafb_set_var_cursorinfo(&crsrvar,
                                              ((struct fb_var_cursorinfo *)
                                               arg)->data, con);
            }
            return i;
        }
    case FBIOGET_CURSORSTATE:{
            struct fb_cursorstate crsrstate;

            i = verify_area(VERIFY_WRITE, (void *) arg, sizeof(crsrstate));
            if (!i) {
                i = tviafb_get_cursorstate(&crsrstate, con);
                copy_to_user((void *) arg, &crsrstate, sizeof(crsrstate));
            }
            return i;
        }
    case FBIOPUT_CURSORSTATE:{
            struct fb_cursorstate crsrstate;

            i = verify_area(VERIFY_READ, (void *) arg, sizeof(crsrstate));
            if (!i) {
                copy_from_user(&crsrstate, (void *) arg,
                               sizeof(crsrstate));
                i = tviafb_set_cursorstate(&crsrstate, con);
            }
            return i;
        }

    /* ARub: set one I2C register */
    case FBIO_TVIA5202_I2C:
    {
	    extern u8 SendOneByte(u8 bAdr, u8 bSubAdr, u8 bData);
	    SendOneByte(0x42, (arg >> 8) & 0xff, arg & 0xff);
	    return 0;
    }


	case FBIO_TVIA5202_InitDecoder: {
			TVIA5202_DECODER_INFO di;
			copy_from_user(&di, (void *)arg, sizeof(TVIA5202_DECODER_INFO));
			trace("Whichdecoder=%x",di.nWhichDecoder);
			i = InitDecoder7114(di.nWhichDecoder, di.nVideoSys, di.nTuner, di.nVBI);
			return i;
		}
	case FBIO_TVIA5202_ReleaseDecoder: {
			int nWhichDecoder;
			copy_from_user(&nWhichDecoder, (void *)arg, sizeof(int));
			i = ReleaseDecoder7114(nWhichDecoder);
			return i;
		}
	case FBIO_TVIA5202_SelectCaptureIndex: {
			u16 wIndex;
			copy_from_user(&wIndex, (void *)arg, sizeof(u16));
			debug_printf("FBIO_TVIA5202_SelectCaptureIndex: wIndex=%x",wIndex);
			Tvia_SelectCaptureIndex(wIndex);
			return 0;
		}
	case FBIO_TVIA5202_GetCaptureIndex: {
			u16 wIndex;
			wIndex = Tvia_GetCaptureIndex();
			copy_to_user((void *)arg, &wIndex, sizeof(u16));
			return 0;
		}
	case FBIO_TVIA5202_InitCapture: {
			u16 wCCIR656;
			copy_from_user(&wCCIR656, (void *)arg, sizeof(u16));
			Tvia_InitCapture(wCCIR656);
			return 0;
		}
	case FBIO_TVIA5202_SetCapSrcWindow: {
			TVIA5202_RECT rect;
			copy_from_user(&rect, (void *)arg, sizeof(TVIA5202_RECT));
			Tvia_SetCapSrcWindow(rect.wLeft, rect.wRight, rect.wTop, rect.wBottom);
			return 0;
		}
	case FBIO_TVIA5202_SetCapDstAddr: {
			TVIA5202_CAPDSTADDR addr;
			copy_from_user(&addr, (void *)arg, sizeof(TVIA5202_CAPDSTADDR));
			Tvia_SetCapDstAddr(addr.dwMemAddr, addr.wX, addr.wY, addr.wPitchWidth);
			return 0;
		}
	case FBIO_TVIA5202_SetCaptureScale: {
			TVIA5202_CAPTURESCALE sc;
			copy_from_user(&sc, (void *)arg, sizeof(TVIA5202_CAPTURESCALE));
			Tvia_SetCaptureScale(sc.wSrcXExt, sc.wDstXExt, sc.wSrcYExt, sc.wDstYExt, sc.bInterlaced);
			return 0;
		}
	case FBIO_TVIA5202_CaptureOn: {
			Tvia_CaptureOn();
			return 0;
		}
	case FBIO_TVIA5202_CaptureOff: {
			Tvia_CaptureOff();
			return 0;
		}
	case FBIO_TVIA5202_SetCapSafeGuardAddr: {
			u32 dwOffAddr;
			copy_from_user(&dwOffAddr, (void *)arg, sizeof(u32));
			Tvia_SetCapSafeGuardAddr(dwOffAddr);
			return 0;
		}
	case FBIO_TVIA5202_CaptureCleanUp: {
			Tvia_CaptureCleanUp();
			return 0;
		}
#if 0
	case FBIO_TVIA5202_EnableDoubleBuffer: {
			u8 bEnable;
			copy_from_user(&bEnable, (void *)arg, sizeof(u8));
			Tvia_EnableDoubleBuffer(bEnable);
			return 0;
		}
	case FBIO_TVIA5202_SetBackBufferAddr: {
			TVIA5202_BACKBUFFERADDR addr;
			copy_from_user(&addr, (void *)arg, sizeof(TVIA5202_BACKBUFFERADDR));
			Tvia_SetBackBufferAddr(addr.dwOffAddr, addr.wX, addr.wY, addr.wPitchWidth);
			return 0;
		}
#endif
	case FBIO_TVIA5202_InvertFieldPolarity: {
			u8 bInvert;
			copy_from_user(&bInvert, (void *)arg, sizeof(u8));
			Tvia_InvertFieldPolarity(bInvert);
			return 0;
		}
	case FBIO_TVIA5202_SelectCaptureEngineIndex: {
			u16 wIndex;
			copy_from_user(&wIndex, (void *)arg, sizeof(u16));
			Tvia_SelectCaptureEngineIndex(wIndex);
			return 0;
		}
	case FBIO_TVIA5202_SetCapturePath: {
			TVIA5202_CAPTUREPATH path;
			dbg("FBIO_TVIA5202_SetCapturePath called");
			copy_from_user(&path, (void *)arg, sizeof(TVIA5202_CAPTUREPATH));
			Tvia_SetCapturePath(path.bWhichPort, path.bWhichCapEngine);
			return 0;
		}
#if 0
	case FBIO_TVIA5202_EnableGeneralDoubleBuffer: {
			u8 bEnable;
			copy_from_user(&bEnable, (void *)arg, sizeof(u8));
			Tvia_EnableGeneralDoubleBuffer(bEnable);
			return 0;
		}
	case FBIO_TVIA5202_DoubleBufferSelectRivals: {
			TVIA5202_DOUBLEBUFFERSELECT db;
			copy_from_user(&db, (void *)arg, sizeof(TVIA5202_DOUBLEBUFFERSELECT));
			Tvia_DoubleBufferSelectRivals(db.wWhichCaptureEngine, db.wWhichOverlay);
			return 0;
		}
	case FBIO_TVIA5202_EnableCapDoubleBuffer: {
			TVIA5202_CAPDOUBLEBUFFER cdb;
			copy_from_user(&cdb, (void *)arg, sizeof(TVIA5202_CAPDOUBLEBUFFER));
			Tvia_EnableCapDoubleBuffer(cdb.wWhichCaptureEngine, cdb.bEnable, cdb.bInterleaved);
			return 0;
		}
	case FBIO_TVIA5202_SyncCapDoubleBuffer: {
			TVIA5202_CAPDOUBLEBUFFER2 cdb2;
			copy_from_user(&cdb2, (void *)arg, sizeof(TVIA5202_CAPDOUBLEBUFFER2));
			Tvia_SyncCapDoubleBuffer(cdb2.wWhichCaptureEngine, cdb2.bEnable);
			return 0;
		}
	case FBIO_TVIA5202_SetCapBackBufferAddr: {
			TVIA5202_CAPBACKBUFFERADDR cbb;
			copy_from_user(&cbb, (void *)arg, sizeof(TVIA5202_CAPBACKBUFFERADDR));
			Tvia_SetCapBackBufferAddr(cbb.wWhichCaptureEngine, cbb.dwOffAddr, cbb.wX, cbb.wY, cbb.wPitchWidth);
			return 0;
		}
#endif
	case FBIO_TVIA5202_EnableAlphaBlend: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableAlphaBlend(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableFullScreenAlphaBlend: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableFullScreenAlphaBlend(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableWhichAlphaBlend: {
			TVIA5202_WHICHALPHABLEND wab;
			copy_from_user(&wab, (void *)arg, sizeof(TVIA5202_WHICHALPHABLEND));
			EnableWhichAlphaBlend(wab.OnOff, wab.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_CleanUpAlphaBlend: {
			CleanUpAlphaBlend();
			return 0;
		}
	case FBIO_TVIA5202_SetBlendWindow: {
			TVIA5202_BLENDWINDOW bw;
			copy_from_user(&bw, (void *)arg, sizeof(TVIA5202_BLENDWINDOW));
			SetBlendWindow(bw.wLeft, bw.wTop, bw.wRight, bw.wBottom, bw.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_InvertFunction: {
			TVIA5202_INVERTFUNCTION in;
			copy_from_user(&in, (void *)arg, sizeof(TVIA5202_INVERTFUNCTION));
			InvertFunction(in.bInvtSrcA, in.bInvtSrcB, in.bInvtAlpha, in.bInvtResult);
			return 0;
		}
	case FBIO_TVIA5202_InvertVideo: {
			u8 bInvtVideo;
			copy_from_user(&bInvtVideo, (void *)arg, sizeof(u8));
			InvertVideo(bInvtVideo);
			return 0;
		}
	case FBIO_TVIA5202_EnableV2AlphaMap: {
			TVIA5202_V2ALPHAMAP v2;
			copy_from_user(&v2, (void *)arg, sizeof(TVIA5202_V2ALPHAMAP));
			EnableV2AlphaMap(v2.OnOff, v2.bAlphaBpp);
			return 0;
		}
	case FBIO_TVIA5202_DisableV2DDA: {
			DisableV2DDA();
			return 0;
		}
	case FBIO_TVIA5202_SelectRAMAddr: {
			u8 bRAMAddrSel;
			copy_from_user(&bRAMAddrSel, (void *)arg, sizeof(u8));
			SelectRAMAddr(bRAMAddrSel);
			return 0;
		}
	case FBIO_TVIA5202_SetAlphaRAM: {
			TVIA5202_ALPHARAM ar;
			copy_from_user(&ar, (void *)arg, sizeof(TVIA5202_ALPHARAM));
			SetAlphaRAM(ar.bRAMType, ar.bR, ar.bG, ar.bB);
			return 0;
		}
	case FBIO_TVIA5202_SetAlphaRAMReg: {
			TVIA5202_ALPHARAMREG ar;
			copy_from_user(&ar, (void *)arg, sizeof(TVIA5202_ALPHARAMREG));
			SetAlphaRAMReg(ar.bIndex, ar.bR, ar.bG, ar.bB);
			return 0;
		}
	case FBIO_TVIA5202_ReadAlphaRAMReg: {
			TVIA5202_ALPHARAMREG2 ar2;
			copy_from_user(&ar2, (void *)arg, sizeof(TVIA5202_ALPHARAMREG2));
			ar2.dwReg = ReadAlphaRAMReg(ar2.bIndex);
			copy_to_user((void *)arg, &ar2, sizeof(TVIA5202_ALPHARAMREG2));
			return 0;
		}
	case FBIO_TVIA5202_EnableSWIndexToRAM: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableSWIndexToRAM(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_RAMLoop: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			RAMLoop(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_SelectWaitCount: {
			u8 bWaitCount;
			copy_from_user(&bWaitCount, (void *)arg, sizeof(u8));
			SelectWaitCount(bWaitCount);
			return 0;
		}
	case FBIO_TVIA5202_RotateAlphaRAMReg: {
			RotateAlphaRAMReg();
			return 0;
		}
	case FBIO_TVIA5202_SelectAlphaSrc: {
			u8 bAlphaSrc;
			copy_from_user(&bAlphaSrc, (void *)arg, sizeof(u8));
			SelectAlphaSrc(bAlphaSrc);
			return 0;
		}
	case FBIO_TVIA5202_SetAlphaReg: {
			TVIA5202_ALPHAREG ar;
			copy_from_user(&ar, (void *)arg, sizeof(TVIA5202_ALPHAREG));
			SetAlphaReg(ar.bR, ar.bG, ar.bB, ar.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_SelectBlendSrcA: {
			u8 bSrcA;
			copy_from_user(&bSrcA, (void *)arg, sizeof(u8));
			SelectBlendSrcA(bSrcA);
			return 0;
		}
	case FBIO_TVIA5202_SelectBlendSrcB: {
			u8 bSrcB;
			copy_from_user(&bSrcB, (void *)arg, sizeof(u8));
			SelectBlendSrcA(bSrcB);
			return 0;
		}
	case FBIO_TVIA5202_EnableMagicAlphaBlend: {
			TVIA5202_MAGICALPHABLEND ma;
			copy_from_user(&ma, (void *)arg, sizeof(TVIA5202_MAGICALPHABLEND));
			EnableMagicAlphaBlend(ma.OnOff, ma.bGeneral);
			return 0;
		}
	case FBIO_TVIA5202_EnableMagicReplace: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableMagicReplace(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_SelectMagicAlphaSrc: {
			u8 bAlphaSrc;
			copy_from_user(&bAlphaSrc, (void *)arg, sizeof(u8));
			SelectMagicAlphaSrc(bAlphaSrc);
			return 0;
		}
	case FBIO_TVIA5202_SetMagicMatchReg: {
			TVIA5202_MAGICMATCHREG mm;
			copy_from_user(&mm, (void *)arg, sizeof(TVIA5202_MAGICMATCHREG));
			SetMagicMatchReg(mm.bR, mm.bG, mm.bB);
			return 0;
		}
	case FBIO_TVIA5202_SetMagicMatchRegRange: {
			TVIA5202_MAGICMATCHREGRANGE mm;
			copy_from_user(&mm, (void *)arg, sizeof(TVIA5202_MAGICMATCHREGRANGE));
			SetMagicMatchRegRange(mm.bRLow, mm.bGLow, mm.bBLow, mm.bRHigh, mm.bGHigh, mm.bBHigh);
			return 0;
		}
	case FBIO_TVIA5202_SetMagicPolicy: {
			u16 wMatch;
			copy_from_user(&wMatch, (void *)arg, sizeof(u16));
			SetMagicPolicy(wMatch);
			return 0;
		}
	case FBIO_TVIA5202_SetMagicReplaceReg: {
			TVIA5202_MAGICMATCHREG mm;
			copy_from_user(&mm, (void *)arg, sizeof(TVIA5202_MAGICMATCHREG));
			SetMagicReplaceReg(mm.bR, mm.bG, mm.bB);
			return 0;
		}
	case FBIO_TVIA5202_EnableExtendedMagicAlpha: {
			TVIA5202_EXTENDEDMAGICALPHA em;
			copy_from_user(&em, (void *)arg, sizeof(TVIA5202_EXTENDEDMAGICALPHA));
			EnableExtendedMagicAlpha(em.OnOff, em.b1On1, em.bByColor);
			return 0;
		}
	case FBIO_TVIA5202_EnableWhichMagicNumber: {
			TVIA5202_WHICHMAGICNUMBER wm;
			copy_from_user(&wm, (void *)arg, sizeof(TVIA5202_WHICHMAGICNUMBER));
			EnableWhichMagicNumber(wm.OnOff, wm.bWhich);
			return 0;
		}
	case FBIO_TVIA5202_EnableMagicNumberInWhichWindow: {
			TVIA5202_MAGICNUMBERWINDOW mm;
			copy_from_user(&mm, (void *)arg, sizeof(TVIA5202_MAGICNUMBERWINDOW));
			EnableMagicNumberInWhichWindow(mm.OnOff, mm.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_SetMagicMatchRegExt: {
			TVIA5202_MAGICMATCHREGEXT mm;
			copy_from_user(&mm, (void *)arg, sizeof(TVIA5202_MAGICMATCHREGEXT));
			SetMagicMatchRegExt(mm.bR, mm.bG, mm.bB, mm.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_SetMagicMatchRegRangeExt: {
			TVIA5202_MAGICMATCHREGRANGEEXT mm;
			copy_from_user(&mm, (void *)arg, sizeof(TVIA5202_MAGICMATCHREGRANGEEXT));
			SetMagicMatchRegRangeExt(mm.bRLow, mm.bGLow, mm.bBLow, mm.bRHigh, mm.bGHigh, mm.bBHigh, mm.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_SetMagicMatchAlphaReg: {
			TVIA5202_MAGICMATCHALPHAREG mm;
			copy_from_user(&mm, (void *)arg, sizeof(TVIA5202_MAGICMATCHALPHAREG));
			SetMagicMatchAlphaReg(mm.bR, mm.bG, mm.bB, mm.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_SetMagicPolicyExt: {
			TVIA5202_MAGICPOLICYEXT mp;
			copy_from_user(&mp, (void *)arg, sizeof(TVIA5202_MAGICPOLICYEXT));
			SetMagicPolicyExt(mp.wMatch, mp.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_SelectAlphaSrc2FromRam: {
			u8 bWhichWindow;
			copy_from_user(&bWhichWindow, (void *)arg, sizeof(u8));
			SelectAlphaSrc2FromRam(bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_EnablePaletteMode: {
			u8 bEnable;
			copy_from_user(&bEnable, (void *)arg, sizeof(u8));
			EnablePaletteMode(bEnable);
			return 0;
		}
	case FBIO_TVIA5202_EnableSLockAlphaBlend: {
			TVIA5202_SLOCKALPHABLEND ab;
			copy_from_user(&ab, (void *)arg, sizeof(TVIA5202_SLOCKALPHABLEND));
			EnableSLockAlphaBlend(ab.bEnable, ab.wTVMode, ab.wModenum);
			return 0;
		}
	case FBIO_TVIA5202_EnableDvideoAlphaBlend: {
			u8 bEnable;
			copy_from_user(&bEnable, (void *)arg, sizeof(u8));
			EnableDvideoAlphaBlend(bEnable);
			return 0;
		}
	case FBIO_TVIA5202_SetAlphaMapPtr: {
			u32 dwAlphamapPtr;
			copy_from_user(&dwAlphamapPtr, (void *)arg, sizeof(u32));
			SetAlphaMapPtr(dwAlphamapPtr);
			return 0;
		}
	case FBIO_TVIA5202_GetAlphaMapPtr: {
			u32 addr;
			addr = GetAlphaMapPtr();
			copy_to_user((void *)arg, &addr, sizeof(u32));
			return 0;
		}
	case FBIO_TVIA5202_SetAlphaBase: {
			TVIA5202_ALPHABASE ab;
			copy_from_user(&ab, (void *)arg, sizeof(TVIA5202_ALPHABASE));
			SetAlphaBase(ab.TVmode, ab.Modenum);
			return 0;
		}
	case FBIO_TVIA5202_InitSLockOverlay: {
			InitSLockOverlay();
			return 0;
		}
	case FBIO_TVIA5202_CleanupSLockOverlay: {
			CleanupSLockOverlay();
			return 0;
		}
	case FBIO_TVIA5202_SetSLockOverlayWindow: {
			TVIA5202_SLOCKOVERLAYWINDOW ow;
			copy_from_user(&ow, (void *)arg, sizeof(TVIA5202_SLOCKOVERLAYWINDOW));
			SetSLockOverlayWindow(ow.wLeft, ow.wTop, ow.wRight, ow.wBottom, ow.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_EnableSLockOverlay: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableSLockOverlay(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableWhichSLockOverlay: {
			TVIA5202_WHICHSLOCKOVERLAY ov;
			copy_from_user(&ov, (void *)arg, sizeof(TVIA5202_WHICHSLOCKOVERLAY));
			EnableWhichSLockOverlay(ov.OnOff, ov.bWhichWindow);
			return 0;
		}
	case FBIO_TVIA5202_EnableInterlaceAlpha: {
			u8 bEnable;
			copy_from_user(&bEnable, (void *)arg, sizeof(u8));
			EnableInterlaceAlpha(bEnable);
			return 0;
		}
	case FBIO_TVIA5202_EnableInterlaceRGBMagicNumber: {
			u8 bEnable;
			copy_from_user(&bEnable, (void *)arg, sizeof(u8));
			EnableInterlaceRGBMagicNumber(bEnable);
			return 0;
		}
	case FBIO_TVIA5202_SelectWindowBlendSrcA: {
			TVIA5202_WINDOWBLENDSRCA wb;
			copy_from_user(&wb, (void *)arg, sizeof(TVIA5202_WINDOWBLENDSRCA));
			SelectWindowBlendSrcA(wb.bSrcA, wb.bWin);
			return 0;
		}
	case FBIO_TVIA5202_SelectWindowBlendSrcB: {
			TVIA5202_WINDOWBLENDSRCB wb;
			copy_from_user(&wb, (void *)arg, sizeof(TVIA5202_WINDOWBLENDSRCB));
			SelectWindowBlendSrcB(wb.bSrcB, wb.bWin);
			return 0;
		}
	case FBIO_TVIA5202_SelectWindowAlphaSrc: {
			TVIA5202_WINDOWALPHASRC wa;
			copy_from_user(&wa, (void *)arg, sizeof(TVIA5202_WINDOWALPHASRC));
			SelectWindowAlphaSrc(wa.bAlphaSrc, wa.bWin);
			return 0;
		}
	case FBIO_TVIA5202_EnableWindowAlphaBlendSrcA: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableWindowAlphaBlendSrcA(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableWindowAlphaBlendSrcB: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableWindowAlphaBlendSrcB(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableWindowAlphaBlendAlpha: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableWindowAlphaBlendAlpha(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableWindowAlphaBlend: {
			u8 OnOff;
			copy_from_user(&OnOff, (void *)arg, sizeof(u8));
			EnableWindowAlphaBlend(OnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableDirectVideo: {
			TVIA5202_DIRECTVIDEO dv;
			copy_from_user(&dv, (void *)arg, sizeof(TVIA5202_DIRECTVIDEO));
			Tvia_EnableDirectVideo(dv.bWhichOverlay, dv.wTVMode, dv.wModenum, dv.bOnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableGenericDirectVideo: {
			TVIA5202_GENERICDIRECTVIDEO dv;
			copy_from_user(&dv, (void *)arg, sizeof(TVIA5202_GENERICDIRECTVIDEO));
			Tvia_EnableGenericDirectVideo(dv.wTVMode, dv.wModenum, dv.bOnOff);
			return 0;
		}
	case FBIO_TVIA5202_EnableWhichDirectVideo: {
			TVIA5202_WHICHDIRECTVIDEO dv;
			copy_from_user(&dv, (void *)arg, sizeof(TVIA5202_WHICHDIRECTVIDEO));
			Tvia_EnableWhichDirectVideo(dv.bWhichOverlay, dv.bOnOff);
			return 0;
		}
	case FBIO_TVIA5202_SelectOverlayIndex: {
			u16 wIndex;
			copy_from_user(&wIndex, (void *)arg, sizeof(u16));
			Tvia_SelectOverlayIndex(wIndex);
			return 0;
		}
	case FBIO_TVIA5202_GetOverlayIndex: {
			u16 wIndex;
			wIndex = Tvia_GetOverlayIndex();
			copy_to_user((void *)arg, &wIndex, sizeof(u16));
			return 0;
		}
	case FBIO_TVIA5202_InitOverlay: {
			Tvia_InitOverlay();
			return 0;
		}
	case FBIO_TVIA5202_SetOverlayFormat: {
			u8 bFormat;
			copy_from_user(&bFormat, (void *)arg, sizeof(u8));
			Tvia_SetOverlayFormat(bFormat);
			return 0;
		}
	case FBIO_TVIA5202_SetOverlayMode: {
			u8 bMode;
			copy_from_user(&bMode, (void *)arg, sizeof(u8));
			Tvia_SetOverlayMode(bMode);
			return 0;
		}
	case FBIO_TVIA5202_SetRGBKeyColor: {
			u32 cref;
			copy_from_user(&cref, (void *)arg, sizeof(u32));
			Tvia_SetRGBKeyColor(cref);
			return 0;
		}
	case FBIO_TVIA5202_SetOverlaySrcAddr: {
			TVIA5202_OVERLAYSRCADDR os;
			copy_from_user(&os, (void *)arg, sizeof(TVIA5202_OVERLAYSRCADDR));
			Tvia_SetOverlaySrcAddr(os.dwMemAddr, os.wX, os.wY, os.wFetchWidth, os.wPitchWidth);
			return 0;
		}
	case FBIO_TVIA5202_SetOverlayWindow: {
			TVIA5202_OVERLAYWINDOW ow;
			copy_from_user(&ow, (void *)arg, sizeof(TVIA5202_OVERLAYWINDOW));
			Tvia_SetOverlayWindow(ow.wLeft, ow.wTop, ow.wRight, ow.wBottom);
			return 0;
		}
	case FBIO_TVIA5202_SetOverlayScale: {
			TVIA5202_OVERLAYSCALE os;
			copy_from_user(&os, (void *)arg, sizeof(TVIA5202_OVERLAYSCALE));
			Tvia_SetOverlayScale(os.bEnableBob, os.wSrcXExt, os.wDstXExt, os.wSrcYExt, os.wDstYExt);
			return 0;
		}
	case FBIO_TVIA5202_ChangeOverlayFIFO: {
			Tvia_ChangeOverlayFIFO();
			return 0;
		}
	case FBIO_TVIA5202_WhichOverlayOnTop: {
			u8 bWhich;
			copy_from_user(&bWhich, (void *)arg, sizeof(u8));
			Tvia_WhichOverlayOnTop(bWhich);
			return 0;
		}
	case FBIO_TVIA5202_OverlayOn: {
			Tvia_OverlayOn();
			return 0;
		}
	case FBIO_TVIA5202_OverlayOff: {
			Tvia_OverlayOff();
			return 0;
		}
	case FBIO_TVIA5202_OverlayCleanUp: {
			Tvia_OverlayCleanUp();
			return 0;
		}
	case FBIO_TVIA5202_EnableChromaKey: {
			u8 bOnOff;
			copy_from_user(&bOnOff, (void *)arg, sizeof(u8));
			Tvia_EnableChromaKey(bOnOff);
			return 0;
		}
	case FBIO_TVIA5202_SetChromaKey: {
			TVIA5202_CHROMAKEY ck;
			copy_from_user(&ck, (void *)arg, sizeof(TVIA5202_CHROMAKEY));
			Tvia_SetChromaKey(ck.creflow, ck.crefhigh);
			return 0;
		}
#if 0
	case FBIO_TVIA5202_EnableOverlayDoubleBuffer: {
			TVIA5202_OVERLAYDOUBLEBUFFER od;
			copy_from_user(&od, (void *)arg, sizeof(TVIA5202_OVERLAYDOUBLEBUFFER));
			Tvia_EnableOverlayDoubleBuffer(od.wWhichOverlay, od.bEnable, od.bInterleaved);
			return 0;
		}
	case FBIO_TVIA5202_SyncOverlayDoubleBuffer: {
			TVIA5202_OVERLAYDOUBLEBUFFER2 od;
			copy_from_user(&od, (void *)arg, sizeof(TVIA5202_OVERLAYDOUBLEBUFFER2));
			Tvia_SyncOverlayDoubleBuffer(od.wWhichOverlay, od.bEnable);
			return 0;
		}
	case FBIO_TVIA5202_SetOverlayBackBufferAddr: {
			TVIA5202_OVERLAYBACKBUFFERADDR ob;
			copy_from_user(&ob, (void *)arg, sizeof(TVIA5202_OVERLAYBACKBUFFERADDR));
			Tvia_SetOverlayBackBufferAddr(ob.wWhichOverlay, ob.dwOffAddr, ob.wX, ob.wY, ob.wPitchWidth);
			return 0;
		}
#endif
	case FBIO_TVIA5202_DoSyncLock: {
			TVIA5202_SYNCLOCK sl;
			copy_from_user(&sl, (void *)arg, sizeof(TVIA5202_SYNCLOCK));
			Tvia_DoSyncLock(sl.wCaptureIndex, sl.wTVStandard, sl.wResolution, sl.bVideoFormat);
			return 0;
		}
	case FBIO_TVIA5202_CleanupSyncLock: {
			Tvia_CleanupSyncLock();
			return 0;
		}
	case FBIO_TVIA5202_SetTVColor: {
			Tvia_SetTVColor();
			return 0;
		}
	case FBIO_TVIA5202_ChangeToVideoClk: {
			TVIA5202_VIDEOCLK vc;
			copy_from_user(&vc, (void *)arg, sizeof(TVIA5202_VIDEOCLK));
			Tvia_ChangeToVideoClk(vc.wCaptureIndex, vc.wTVStandard, vc.bVideoFormat, vc.bSet, vc.wResolution);
			return 0;
		}
	case FBIO_TVIA5202_GetTVMode: {
			u16 wMode;
			wMode = Tvia_GetTVMode();
			copy_to_user((void *)arg, &wMode, sizeof(u16));
			return 0;
		}
	case FBIO_TVIA5202_SetVCRMode: {
			u8 bVCRMode;
			copy_from_user(&bVCRMode, (void *)arg, sizeof(u8));
			Tvia_SetVCRMode(bVCRMode);
			return 0;
		}
	case FBIO_TVIA5202_SetSyncLockGM: {
			TVIA5202_SYNCLOCKGM sl;
			copy_from_user(&sl, (void *)arg, sizeof(TVIA5202_SYNCLOCKGM));
			Tvia_SetSyncLockGM(sl.wCaptureIndex, sl.wTVStandard, sl.wResolution, sl.bVideoFormat);
			return 0;
		}
	case FBIO_TVIA5202_CleanupSyncLockGM: {
			Tvia_CleanupSyncLockGM();
			return 0;
		}
	case FBIO_TVIA5202_OutputSyncLockVideo: {
			TVIA5202_SYNCLOCKVIDEO sl;
			copy_from_user(&sl, (void *)arg, sizeof(TVIA5202_SYNCLOCKVIDEO));
			Tvia_OutputSyncLockVideo(sl.bOutput, sl.wTVStandard, sl.wResolution, sl.bVideoFormat);
			return 0;
		}
	case FBIO_TVIA5202_SetSynclockPath: {
			u8 bWhichPort;
			copy_from_user(&bWhichPort, (void *)arg, sizeof(u8));
			Tvia_SetSynclockPath(bWhichPort);
			return 0;
		}
	case FBIO_TVIA5202_DetectSynclockVideoSignal: {
			TVIA5202_SYNCLOCKVIDEOSIGNAL vs;
			copy_from_user(&vs, (void *)arg, sizeof(TVIA5202_SYNCLOCKVIDEOSIGNAL));
			vs.Ok = Tvia_DetectSynclockVideoSignal(vs.bVideoFormat);
			copy_to_user((void *)arg, &vs, sizeof(TVIA5202_SYNCLOCKVIDEOSIGNAL));
			return 0;
		}

	  // !!!raf ioctl di test
	case FBIO_TVIA5202_TestVideoMemoryRed: {
			u8 test_type;
			copy_from_user(&test_type, (void *)arg, sizeof(u8));
			Tvia_TestVideo(test_type);
			return 0;
		}

	case FBIO_TVIA5202_TestByteDecoder: {
	  		{
  			  u8 test_byte;
		  
			  TESTDECODER tstdec;
			  copy_from_user(&tstdec, (void *)arg, sizeof(TESTDECODER));

			  test_byte=DecoderTest(tstdec.type,tstdec.index,tstdec.val);

  			  copy_to_user((void *)arg, &test_byte, sizeof(u8));
			  return 0;
			}
		}

	case FBIO_TVIA5202_DumpTotalTest: {
      TESTTVIA tsttvia;
			copy_from_user(&tsttvia, (void *)arg, sizeof(TESTTVIA));
			My_Tvia_ioctl(tsttvia.type,tsttvia.index,tsttvia.val,tsttvia.indexword,tsttvia.valword);
			return 0;
		}

	case FBIO_TVIA5202_3cf_ba_7_down: {
			u8 tmp,tmpb;
                        tvia_outb(0xBF, 0x3ce);
			tmpb=tvia_inb(0x3cf);
	                tvia_outb(0x00, 0x3cf);

			tmp=In_Video_Reg(0xba);
			trace("Read 3cf/ba=%x",tmp);
			Out_Video_Reg(0xba,tmp & 0x7f);
			tmp=In_Video_Reg(0xba);

			tvia_outb(0xBF, 0x3ce);
	                tvia_outb(tmpb, 0x3cf);

			trace("Written/Read 3cf/ba=%x",tmp);
			return 0;
		}

	case FBIO_TVIA5202_testvmem: {
			u8 tmp;
			int idx,Num_Blocks=64;

			for(idx=0;idx<Num_Blocks;idx++) {
			  tmp=tvia_testvmem(idx);
			  trace("testvmem: bank[%x] %x",idx,tmp);
  		        }
			trace("Tested %i KB",Num_Blocks*64);
			return 0;
		}
		
	// CARLOS DMA
	case FBIO_TVIA5202_DMAReadRequest: {
			if (!r_dma_ch_flag){
				r_dma_ch = pxa_request_dma("video1R", DMA_PRIO_HIGH, 
						  video_rdma_irq, NULL);
				if (r_dma_ch < 0){
					trace("video1 Read DMA Channel not available");
				}
				//DEBUG
				else{
					r_dma_ch_flag = 1;
					//dsta = (int) ((int)&org[7] & 0xFFFFFFF8);
					dsta = (int)btweb_bigbuf; //First bigbuf
				}
			} else {
				trace("WARNING: video1 READ DMA Channel Already in Use");
			}
			return r_dma_ch;			
		}
		
	case FBIO_TVIA5202_DMAWriteRequest: {
			if (!w_dma_ch_flag){
				w_dma_ch = pxa_request_dma("video1W", DMA_PRIO_HIGH, 
						  video_wdma_irq, NULL);
				if (w_dma_ch < 0){
					trace("video1 Write DMA Channel not available");
				}
				//DEBUG
				else {
					w_dma_ch_flag = 1;
					//orga = (int) ((int)&org[7] & 0xFFFFFFF8); 		
					orga = (int)btweb_bigbuf1; //Second bigbuf
				}
			} else {
				trace("WARNING: video1 Write DMA Channel Already in Use");
			}
			return w_dma_ch;
		}		
		
	case FBIO_TVIA5202_DMAReadFree: {
			if (r_dma_ch_flag){
				pxa_free_dma(r_dma_ch);
				r_dma_ch_flag = 0;
			}
			return 0;
		}	
		
	case FBIO_TVIA5202_DMAWriteFree: {
			if (w_dma_ch_flag){
				pxa_free_dma(w_dma_ch);
				w_dma_ch_flag = 0;
			}
			return 0;
		}			

	case FBIO_TVIA5202_DMAReadFrame: {	
			if (r_dma_ch_flag){		
				copy_from_user(&RConf, (void *)arg, sizeof(TVIA5202_DMACONF));
		
				//DEBUG
				//copy_to_user(RConf.dtadr, (void *)dsta, RConf.width*RConf.height);
		
				return(Tvia_DMAReadFrame());		
			} else {
				trace("ERROR: video1 Read DMA Channel Has Not Been yet Requested");
				return 1;				
			}
			
		}			

	case FBIO_TVIA5202_DMAWriteFrame: {
			if (w_dma_ch_flag){
				copy_from_user(&WConf, (void *)arg, sizeof(TVIA5202_DMACONF));
		
				//DEBUG
				//copy_from_user((void *)orga, WConf.dsadr, WConf.width*WConf.height);
		
				return (Tvia_DMAWriteFrame());		
			} else {
				trace("ERROR: video1 Write DMA Channel Has Not Been yet Requested");
				return 1;
			}
		}	

	case FBIO_TVIA5202_DMAReadPolling: {
			if (r_dma_ch_flag){
				return(DCSR(r_dma_ch) & DCSR_STOPSTATE);
			} else {
				trace("ERROR: video1 Read DMA Channel Has Not Been yet Requested");
				return 0;				
			}
		}				
// END CARLOS DMA	

//CARLOS FLICKER FILTER
	case FBIO_TVIA5202_FLICKERFree: {
			u16 bOnOff;
			copy_from_user(&bOnOff, (void *)arg, sizeof(u16));
			FlickFree(bOnOff);
			return 0;
		}	

	case FBIO_TVIA5202_FLICKERWinInside: {
			u16 bOnOff;
			copy_from_user(&bOnOff, (void *)arg, sizeof(u16));
			FlickerWinInside(bOnOff);
			return 0;
		}	

	case FBIO_TVIA5202_FLICKERWin: {
			TVIA5202_FLICKERWIN FlickerWin;
			copy_from_user(&FlickerWin, (void *)arg, sizeof(TVIA5202_FLICKERWIN));
			SetTVFlickFreeWin(FlickerWin.wLeft, FlickerWin.wTop, FlickerWin.wRight, FlickerWin.wBottom);
			return 0;
		}		

	case FBIO_TVIA5202_FLICKERCtrlCoeffs: {
			TVIA5202_FLICKERCOEFFS FlickerCoeffs;
			copy_from_user(&FlickerCoeffs, (void *)arg, sizeof(TVIA5202_FLICKERCOEFFS));
			FlickerCtrlCoefficient(FlickerCoeffs.alpha,FlickerCoeffs.beta, FlickerCoeffs.gama);
			return 0;
		}				
//END CARLOS FLICKER FILTER		

    }
    return -EINVAL;
}
/*
static int tviafb_mmap(struct fb_info *info, struct file *file, 
                       struct vm_area_struct *vma)
{
	unsigned long start,off,len;

	if(vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	off = vma->vm_pgoff << PAGE_SHIFT;

	// frame buffer memory
	start = current_par.screen_base_p;
	len   = PAGE_ALIGN((start & ~PAGE_MASK) + current_par.screen_size);

	if(off >= len)
	{
		// memory mapped io
		off -= len;
		start = current_par.regs_base_p;
		len   = PAGE_ALIGN((start & ~PAGE_MASK) + 0x000c0000);
	}

	start &= PAGE_MASK;
	
	if((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;

	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	
#if defined(__i386__) || defined(__x86_64__)
	if(boot_cpu_data.x86 > 3)
		pgprot_val(vma->vm_page_prot) |= _PAGE_PCD;
#elif defined(__arm__) || defined(__mips__)
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	// This is an IO amp - tell maydump to skip this VMA
	vma->vm_flags |= VM_IO;
#elif defined(__sh__)
	pgprot_val(vma->vm_page_prot) &= ~_PAGE_CACHABLE;
#else
#warning tviafb_mmap: What do we have to do here?
#endif

	if(io_remap_page_range(vma->vm_start, off, 
				vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
	
	return 0;
}*/

static int
tviafb_mmap(struct fb_info *info, struct file *file, struct vm_area_struct *vma)
{
	unsigned long start,off,len;
  	unsigned char *mem;
    void * phy_buff = VMEM_HIGH_MEMORY_63MB;
//    static char Buffer_Motion_Jpeg[MEM_FOR_MOTION_JPEG];
	trace("\ntviafb_mmap 1.3");
    trace("phy_buff=%x",phy_buff);
    Buffer_Motion_Jpeg = (volatile unsigned char *)ioremap(phy_buff, 0x100000);
	trace("Buffer_Motion_Jpeg = %x",Buffer_Motion_Jpeg);

    if(vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	off = vma->vm_pgoff << PAGE_SHIFT;

////	lock_kernel();	
//	mem = kmalloc(MEM_FOR_MOTION_JPEG, GFP_KERNEL);
//	mem = Buffer_Motion_Jpeg;
//	trace("tviafb_mmap:virt=%x",mem);
//	if (!mem)
//		return -EINVAL;
//	phy_buff=virt_to_bus((void*)mem);
//	trace("tviafb_mmap:phy=%x",phy_buff);
	
	/* frame buffer memory */
	start = phy_buff;
	len   = PAGE_ALIGN((start & ~PAGE_MASK) + MEM_FOR_MOTION_JPEG);
	trace("tviafb_mmap:start=%x",start);
	trace("tviafb_mmap:len=%x",len);

#if 0
	if(off >= len)
	{
		/* memory mapped io */
		off -= len;
		start = current_par.regs_base_p;
		len   = PAGE_ALIGN((start & ~PAGE_MASK) + 0x000c0000);
	}
#endif

//	unlock_kernel();

	dbg("tviafb_mmap:5");
	start &= PAGE_MASK;
	

	if((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;

	dbg("tviafb_mmap:6");
	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	
#if defined(__i386__) || defined(__x86_64__)
	if(boot_cpu_data.x86 > 3)
		pgprot_val(vma->vm_page_prot) |= _PAGE_PCD;
#elif defined(__arm__) || defined(__mips__)
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/* This is an IO amp - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO;
#elif defined(__sh__)
	pgprot_val(vma->vm_page_prot) &= ~_PAGE_CACHABLE;
#else
#warning tviafb_mmap: What do we have to do here?
#endif

	trace("tviafb_mmap:vm_start=%x",vma->vm_start);
	trace("tviafb_mmap:vm_end=%x",vma->vm_end);
	trace("tviafb_mmap:off=%x",off);
	trace("tviafb_mmap:len=%x",len);

	if(io_remap_page_range(vma->vm_start, off, 
				vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	trace("tviafb_mmap: OK");
	
	return 0;
}

static struct fb_ops tviafb_ops = {
    owner:THIS_MODULE,
    fb_open:tviafb_open,
    fb_release:tviafb_release,
    fb_get_fix:tviafb_get_fix,
    fb_get_var:tviafb_get_var,
    fb_set_var:tviafb_set_var,
    fb_get_cmap:tviafb_get_cmap,
    fb_set_cmap:tviafb_set_cmap,
    fb_pan_display:tviafb_pan_display,
    fb_ioctl:tviafb_ioctl,
//    fb_mmap:tviafb_mmap,
};




/*-----------------------------------------------------
*
* 	Misc functions needs by fb_info structure
*
*-----------------------------------------------------*/
static int tviafb_updatevar(int con, struct fb_info *info)
{
    if (con == current_par.currcon)
        tviafb_update_start(&fb_display[con].var);
    return 0;
}

static int tviafb_switch(int con, struct fb_info *info)
{
    struct fb_cmap *cmap;

    if (current_par.currcon >= 0) {
        cmap = &fb_display[current_par.currcon].cmap;

        if (cmap->len)
            fb_get_cmap(cmap, 1, tvia_getcolreg, info);
    }

    current_par.currcon = con;

    fb_display[con].var.activate = FB_ACTIVATE_NOW;

    tviafb_set_var(&fb_display[con].var, con, info);

    return 0;
}

static void tviafb_blank(int blank, struct fb_info *info)
{
  //	volatile unsigned int *ioaddr;

    if (blank) {
      //	ioaddr = (volatile unsigned int *)CPLD_F0_P2V(0x0F000004);
      //	*ioaddr = 0x00;
    } else {
      //	ioaddr = (volatile unsigned int *)CPLD_F0_P2V(0x0F000004);
      //	*ioaddr = 0x07;
    }

}

/*------------------------------------------------------
*
*	tviafb driver initialization 
*
*------------------------------------------------------*/

void __init tviafb_setup(char *options, int *ints)
{
}


static void __init tviafb_init_fbinfo(void)
{
    static int first = 1;

    if (!first)
        return;
    first = 0;

    strcpy(fbinfo.modename, "CyberPro5202");
    strcpy(fbinfo.fontname, "Acorn8x8");

    fbinfo.node = -1;
    fbinfo.fbops = &tviafb_ops;
    fbinfo.disp = &global_disp;
    fbinfo.changevar = NULL;
    fbinfo.switch_con = tviafb_switch;
    fbinfo.updatevar = tviafb_updatevar;
    fbinfo.blank = tviafb_blank;
    fbinfo.flags = FBINFO_FLAG_DEFAULT;

    /* setup initial parameters */
    memset(&init_var, 0, sizeof(init_var));
    init_var.xres_virtual = init_var.xres = selected_xres;
    init_var.yres_virtual = init_var.yres = selected_yres;
    init_var.bits_per_pixel = DEFAULT_BPP;

    init_var.red.msb_right = 0;
    init_var.green.msb_right = 0;
    init_var.blue.msb_right = 0;

    switch (init_var.bits_per_pixel) {
    case 8:
        init_var.bits_per_pixel = 8;
        init_var.red.offset = 0;
        init_var.red.length = 8;
        init_var.green.offset = 0;
        init_var.green.length = 8;
        init_var.blue.offset = 0;
        init_var.blue.length = 8;
        break;

    case 16:
        init_var.bits_per_pixel = 16;
        init_var.red.offset = 11;
        init_var.red.length = 5;
        init_var.green.offset = 5;
        init_var.green.length = 6;
        init_var.blue.offset = 0;
        init_var.blue.length = 5;
        break;

    case 24:
        init_var.bits_per_pixel = 24;
        init_var.red.offset = 16;
        init_var.red.length = 8;
        init_var.green.offset = 8;
        init_var.green.length = 8;
        init_var.blue.offset = 0;
        init_var.blue.length = 8;
        break;
    }

    init_var.nonstd = 0;
    init_var.activate = FB_ACTIVATE_NOW;
    init_var.height = -1;
    init_var.width = -1;
    init_var.accel_flags = FB_ACCELF_TEXT;
    init_var.sync = FB_SYNC_COMP_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT;
    init_var.vmode = FB_VMODE_NONINTERLACED;
    init_var.pixclock = 60;
}

#ifdef CONFIG_PM
static int fb_pm_callback(struct pm_dev *dev, pm_request_t rqst, void *data)
{
	int w, b;
	//	volatile unsigned char *ioaddr;

	switch (rqst)
	{
	case PM_SUSPEND:
		video_buf = (unsigned char *)vmalloc(0x200000);
		if(video_buf) {
			memcpy(video_buf, (unsigned char *)current_par.screen_base, 0x200000);
		}
		cursor_buf = (unsigned char *)vmalloc(0x100);
		if(cursor_buf) {
			memcpy(cursor_buf, (unsigned char *)current_par.screen_base + 
				current_par.screen_size - 0x100, 0x100);
		}
		//		ioaddr = (volatile unsigned char *)CPLD_F0_P2V(0x0F000004);
		//*ioaddr = 0x00;
		break;

	case PM_RESUME:
	  //ioaddr = (volatile unsigned char *)CPLD_F0_P2V(0x0F000004);
	  //	*ioaddr = 0x07;

    	tvia_outb(0x18, 0x46e8);
    	udelay(1000);
    	tvia_outb(0x01, 0x102);
    	udelay(1000);
    	tvia_outb(0x08, 0x46e8);
    	udelay(1000);

		tvia_init_hw();
		udelay(1000);
		ProgramTV();

	    if (init_var.bits_per_pixel >= 16) {
    	    BypassMode(ON);         /*want bypass mode */
    	}
    	else {
        	BypassMode(OFF);        /*want index mode */
        	SetRamDac(RamDacData, sizeof(RamDacData) / 3);
    	}
    	SetDACPower(ON);            /*power on DAC to trun on screen */

		if(video_buf) {
			memcpy((unsigned char *)current_par.screen_base, video_buf, 0x200000);
			vfree(video_buf);
			video_buf = NULL;
		}

    	EnableDigitalRGB(ON);
    	EnableDigitalCursor(ON);
    	tvia_init_hwcursor();

	    if (init_var.bits_per_pixel == 8) {
    	    b = 0;
        	w = init_var.xres;
	    } else if (init_var.bits_per_pixel == 16) {
    	    b = 1;
        	w = init_var.xres;
	    } else if (init_var.bits_per_pixel == 24) {
    	    b = 2;
        	w = init_var.xres*3;
	    }

    	tvia_outw(w - 1, 0xbf018);      /* 2d */
    	tvia_outw(w - 1, 0xbf218);
    	tvia_outb(b, 0xbf01c);

		if(cursor_buf) {
			memcpy((unsigned char *)current_par.screen_base + current_par.screen_size 
				- 0x100, cursor_buf, 0x100);
			vfree(cursor_buf);
			cursor_buf = NULL;
		}
		SetDACPower(ON);

		break;
	}
	return 0;
}
#endif

static int __init tvia5202fb_init(void)
{
    //volatile unsigned char *ioaddr;
    int w, b;
    u8 tmp,iTmpFA,iTmp;

    current_par.device_id = 0x5202;
    current_par.memtype = 1;
    current_par.palette_size = 256;

    trace("Tvia5202 INIT 1.7.6");

    if ((btweb_globals.flavor==BTWEB_PE)||(btweb_globals.flavor==BTWEB_PI)) {
       /* Reset Tvia5202 */
       set_GPIO_mode(btweb_features.tvia_reset | GPIO_OUT);
       GPSR(btweb_features.tvia_reset) = GPIO_bit(btweb_features.tvia_reset);
    }
    else if ((btweb_globals.flavor==BTWEB_F453AV)||(btweb_globals.flavor==BTWEB_2F)|| \
                (btweb_globals.flavor==BTWEB_INTERFMM) ){
       /* Reset Tvia5202 */
       set_GPIO_mode(btweb_features.tvia_reset | GPIO_OUT);
       GPCR(btweb_features.tvia_reset) = GPIO_bit(btweb_features.tvia_reset); 
    }
    else
	return -ENODEV;

    trace("Resetting .1 sec");
    udelay(100000);


#if 1
  trace("MDREFR=%X",MDREFR);
  trace("MSC2=%X",MSC2);
  trace("Setting MDREFR to 0xDC018");
  MDREFR=0x000DC018;
  trace("MDREFR=%X",MDREFR);
  udelay(100000);
#endif

  trace("MSC2=%X\n",MSC2);
  trace("MDREFR=%X\n",MDREFR);

    if ((btweb_globals.flavor==BTWEB_PE)||(btweb_globals.flavor==BTWEB_PI)) {
      GPCR(btweb_features.tvia_reset) = GPIO_bit(btweb_features.tvia_reset); /* tvia5202 HW reset end */
    }
    else if ((btweb_globals.flavor==BTWEB_F453AV)||(btweb_globals.flavor==BTWEB_2F)|| \
                (btweb_globals.flavor==BTWEB_INTERFMM) ){
      GPSR(btweb_features.tvia_reset) = GPIO_bit(btweb_features.tvia_reset); /* tvia5202 HW reset end */
    }

    trace("GO tvia!");

    CyberRegs = (volatile unsigned char *)ioremap(VMEM_BASEADDR + 0x00800000, 0x100000);
    current_par.currcon = -1;
    current_par.screen_base_p = (unsigned long)VMEM_BASEADDR;
    current_par.screen_base = (unsigned long)ioremap(VMEM_BASEADDR, VMEM_SIZE);
    current_par.screen_size	= (unsigned long)VMEM_SIZE;
    current_par.regs_base_p = VMEM_BASEADDR + 0x00800000;

    fb_mem_addr = current_par.screen_base;
    x_res = selected_xres;

// ------------

#if 0
  //trace("Read MDREFR=%X",MDREFR);
  printk("MSC2=%X\n",MSC2);

  trace("Setting MDREFR to 0xDC018");
   MDREFR=0x000DC018; 
#endif

	/* Wake up the chip */
	tvia_outb(0x18, 0x46e8);
	tvia_outb(0x01, 0x102);
	tvia_outb(0x08, 0x46e8);  
	dbg("Wake up the chip");

	/* Enable linear address */
	//tvia_outb(0x33, 0x3ce);
	//tvia_outb(0x01, 0x3cf);
	dbg("Enable linear address SHOULD ALREADY ENABLED");

	dbg("Now enable NO_BE_MODE");

	/* Set NO_BE_MODE */	
	tvia_outb(0x33, 0x3ce);
	tvia_outb(0x40, 0x3cf); /* set the banking */

	dbg("tviafb: enable NO_BE_MODE 1");

	tvia_outb(0x3c, 0x3ce);
	tvia_outb(0x40, 0x3cf); /* set NO_BE_MODE */

	//trace("tviafb: enable NO_BE_MODE 2");

	if(tvia_inb(0x3cf) != 0x40)
	{
		trace("Error on enabling NO_BE_MODE");
		tvia_outb(0x33, 0x3ce);
		tvia_outb(0x00, 0x3cf);
		if (CyberRegs != 0) {
		iounmap((void *) CyberRegs);
		CyberRegs = 0;
		}
		if (current_par.screen_base != 0) {
			iounmap((void *) current_par.screen_base);
			current_par.screen_base = 0;
		}
		trace("Exiting from tviafb module");
		
		return 1;
	}
	tvia_outb(0x33, 0x3ce);
	tvia_outb(0x00, 0x3cf);  /* clear the banking */
	dbg("OK NO_BE_MODE");

    /* Verifying paramter from cmdline */
    trace ("tvia_mode=%d\n",tvia_mode);


    dbg("tviafb_init_fbinfo");
    switch (tvia_mode) {
    case 1:
	selected_xres = 720;
        selected_yres = 576;
	init_mode = 1;
	reg_tv_size_5202 = 141*2;
	trace ("Tvia mode PAL 720x576\n");
	break;
    case 2:
        selected_xres = 640;
        selected_yres = 480;
        init_mode = 2;
        reg_tv_size_5202 = 133*2;
        trace ("Tvia mode NTSC 640x480-16-50\n");
        break;
   case 3:
        selected_xres = 640;
        selected_yres = 480;
        init_mode = 3;
        reg_tv_size_5202 = 133*2;
        trace ("Tvia mode NTSC 640x480-16-60\n");
        break;
   case 4:
        selected_xres = 640;
        selected_yres = 440;
        init_mode = 4;
        reg_tv_size_5202 = 133*2;
	
        trace ("Tvia mode NTSC 640x440-16-60 : \n");
        break;
   case 5:
        selected_xres = 800;
        selected_yres = 600;
        init_mode = 0;
        reg_tv_size_5202 = 133*2; /* ? */
        trace ("Tvia mode NTSC 800x600-?-? : \n");
        break;

    }

    tviafb_init_fbinfo();

    dbg("tvia_init_hw");
    tvia_init_hw();

    /* Initialize cursor info */
    current_par.crsr.xoffset = 200;
    current_par.crsr.yoffset = 200;
    current_par.crsr.mode = FB_CURSOR_OFF;
    current_par.crsr.crsr_xsize = 32;
    current_par.crsr.crsr_ysize = 32;

    tviafb_set_var(&init_var, -1, &fbinfo);

    dbg("Going to ProgramTV");
    ProgramTV();
    dbg("ProgramTV done");

    if (init_var.bits_per_pixel >= 16)
        BypassMode(ON);         /*want bypass mode */
    else {
        BypassMode(OFF);        /*want index mode */
        SetRamDac(RamDacData, sizeof(RamDacData) / 3);
    }
   dbg("SetDACPower");
   SetDACPower(ON);            /*power on DAC to turn on screen */

#ifdef DISABLE_RGBDAC
   deb("Disabling RGBDAC");
   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
   iTmpFA = tvia_inb(0x3cf);
   tvia_outb(0x02, 0x3cf);
   
   tvia_outb(0xB1, 0x3ce);
   iTmp = tvia_inb(0x3cf);
   deb("read 3cf/bf02.3cf/b1=%x",iTmp);
   iTmp = (iTmp|0x0f);
   deb("writing 3cf/fa05.3cf/b1=%x",iTmp);
   tvia_outb(iTmp, 0x3cf);

   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
   tvia_outb(iTmpFA, 0x3cf);
   deb("Disabling RGBDAC done");
#else 
   deb("Enabling RGBDAC");
   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
   iTmpFA = tvia_inb(0x3cf);
   tvia_outb(0x02, 0x3cf);

   tvia_outb(0xB1, 0x3ce);
   iTmp = tvia_inb(0x3cf);
   deb("read 3cf/bf02.3cf/b1=%x",iTmp);
   iTmp = (iTmp&0xC0);/////per abilitare 3f
   deb("writing 3cf/fa05.3cf/b1=%x",iTmp);
   tvia_outb(iTmp, 0x3cf);

   tvia_outb(0xBF, 0x3ce);     /*Banking I/O control */
   tvia_outb(iTmpFA, 0x3cf);
   trace("Enabling RGBDAC done");
#endif

   deb("Disabling DigitalRGB");
   EnableDigitalRGB(OFF);
   deb("Disabling DigitalCursor");
   EnableDigitalCursor(OFF);

#ifdef DISABLE_RGBDAC
   trace("Disabling HSync,VSync");
    //HSYNC:  3CF/16 [1:0] = 01  ----- disable Hsync
    //00  ----- enable Hsync
    //VSYNC:  3CF/16 [3:2] = 01  ----- disable Vsync
    Out_Video_Reg_M(0x16,0x05,0xF0);
#else
   trace("Enabling HSync,VSync");
   //HSYNC:  3CF/16 [1:0] = 01  ----- disable Hsync
   //00  ----- enable Hsync
   //VSYNC:  3CF/16 [3:2] = 01  ----- disable Vsync
   Out_Video_Reg_M(0x16,0x00,0xF0);
#endif

   dbg("Disable hardware cursor");
   tvia_outb(0x56, 0x3ce);
   tvia_outb(tvia_inb(0x3cf) & 0xfe, 0x3cf);
   dbg("Hardware cursor done");

    if (init_var.bits_per_pixel == 8) {
        b = 0;
        w = init_var.xres;
    } else if (init_var.bits_per_pixel == 16) {
        b = 1;
        w = init_var.xres;
    } else if (init_var.bits_per_pixel == 24) {
        b = 2;
        w = init_var.xres*3;
    }

    tvia_outw(w - 1, 0xbf018);      /* 2d */
    tvia_outw(w - 1, 0xbf218);
    tvia_outb(b, 0xbf01c);

    dbg("SetDACPower ON");
    SetDACPower(ON);

    if (register_framebuffer(&fbinfo) < 0) {
        return -EINVAL;
    }

#ifdef CONFIG_PM
	fb_pm_dev = pm_register(PM_SYS_DEV, PM_SYS_VGA, fb_pm_callback);
#endif

    trace("CyberPro5202: %ldkB VRAM, using %dx%d",
           current_par.screen_size >> 10, init_var.xres, init_var.yres);

    return 0;
}

static void __exit tvia5202fb_exit(void)
{
    unregister_framebuffer(&fbinfo);

    if (CyberRegs != 0) {
        iounmap((void *) CyberRegs);
        CyberRegs = 0;
    }
    if (current_par.screen_base != 0) {
        iounmap((void *) current_par.screen_base);
        current_par.screen_base = 0;
    }

#ifdef CONFIG_PM
    if(!fb_pm_dev) {
    	pm_unregister(fb_pm_dev);
    	fb_pm_dev = NULL;
    }
#endif
}

module_init(tvia5202fb_init);
module_exit(tvia5202fb_exit);
