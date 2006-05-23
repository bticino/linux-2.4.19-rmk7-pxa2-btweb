#ifndef _TVIAFB_H_
#define	_TVIAFB_H_

/*
 * tviafb.h
 *
 * TVia,Inc. CyberPro5k frame buffer device
 */
#include "tvia.h"

#define arraysize(x)    	(sizeof(x)/sizeof(*(x)))
#define tvia_outb(dat,reg)	((*(unsigned char  *)(CyberRegs + (reg))) = dat)
#define tvia_outw(dat,reg)	((*(unsigned short *)(CyberRegs + (reg))) = dat)
#define tvia_outl(dat,reg)	((*(unsigned long  *)(CyberRegs + (reg))) = dat)

#define WriteTVReg(reg,dat)	tvia_outw(dat,reg+0x000B0000)


// ORIG#define tvia_inb(reg)	(*(unsigned char  *)(CyberRegs + (reg)))  /* ORIG */
#define tvia_inb(reg) (*(volatile unsigned char  *)(CyberRegs + (reg | ( (reg & 0x03) << 12) )  ))   /* NO BYTE ENABLE MODE */


//#define tvia_inb_nbem(reg) (*(unsigned char  *)(CyberRegs + (reg | ( (reg & 0x03) << 12) )  ))   /* VLIO */
#define tvia_inb_novlio(reg)	(*(unsigned char  *)(CyberRegs + (reg)))  /* ORIG */
/*#define tvia_inb(reg)	(*(unsigned char  *)(CyberRegs + (reg | ( (reg & 0x03) << 12) )  ))   VLIO */
/* NEPPURE COSI' #define tvia_inb(reg)	(*(unsigned char  *)(CyberRegs + ( (reg | ((reg & 0x03) << 12)) & (~0x3) )))  VLIO */

//#define tvia_inw(reg)	(*(unsigned short *)(CyberRegs + (reg))) // ORIG
#define tvia_inw(reg)	(*(volatile unsigned short *)(CyberRegs + (reg | ( (reg & 0x03) << 12) ))) // ORIG
//#define tvia_inl(reg)	(*(unsigned long  *)(CyberRegs + (reg))) // ORIG
#define tvia_inl(reg)	(*(volatile unsigned long  *)(CyberRegs + (reg | ( (reg & 0x03) << 12) ))) // ORIG

#define ReadTVReg(reg)	tvia_inw( (reg+0x000B0000) )

#define ON 	    1
#define OFF     0

//#define INIT_MODE	0		/* Mode_800x600x16x60 */
#define INIT_MODE	1		/* Mode_640x480x16x60 */

struct tviafb_par {
    unsigned short device_id;
    int memtype;                /* 1--SDRAM 0--SGRAM */
    unsigned long screen_base;
    unsigned long screen_base_p;
    unsigned long regs_base;
    unsigned long regs_base_p;
    unsigned long screen_end;
    unsigned long screen_size;
    unsigned int palette_size;
    signed int currcon;

    /*
     * hardware cursor
     */
    struct {
        short xoffset;          /* cursor position */
        short yoffset;
        unsigned short mode;    /* OnOff */
        unsigned short crsr_xsize;      /* cursor size in display pixel */
        unsigned short crsr_ysize;
    } crsr;

    /*
     * palette
     */
    struct {
        u8 red;
        u8 green;
        u8 blue;
    } palette[256];
    /*
     * colour mapping table
     */
    union {
#ifdef FBCON_HAS_CFB16
        u16 cfb16[16];
#endif
#ifdef FBCON_HAS_CFB24
        u32 cfb24[16];
#endif
    } c_table;
};


struct tv_reg {
    unsigned short tv_regs[64];
};

struct tv_ereg {
    unsigned short tv_eregs[59];
};

struct res {
    int xres;
    int yres;
    unsigned char crtc_regs[18];
    unsigned char crtc_ofl;
    unsigned char clk_regs[12];
    struct tv_reg *tv_reg_ptr;
    struct tv_ereg *tv_ereg_ptr;
};

typedef struct {
    int xres;
    int yres;
    int bpp;
    int clock;
    int underscan;              /*0, -- no-underscan; 1, -- underscan; */
} mode_table;

mode_table Cyberpro_5202[] = {
    {800, 600, 16, 60, 0},
    {640, 480, 16, 60, 0}
};

struct sModeParam {
    int wModeID;                /*Mode ID */
    u16 wTVOn;                  /*Turn ON/OFF TV DAC */
    u16 wCRTOn;                 /*Turn ON/OFF CRT DAC */
    int bpp;                    /*use to justify whether bypass */
};

 /*------------------------------------------------------------
   The look up table below will only be needed for 256 color mode
   For 16/24/32 bpp modes, a bypass mode can not enabled (see
   code example)

   Note: value 0x00 is lowest intensity.
  value 0xFF is highest intensity.
  ------------------------------------------------------------*/
u8 RamDacData[256 * 3] = {
    /*   Red   Green  Blue  */
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x2A,
    0x00, 0x2A, 0x00,
    0x00, 0x2A, 0x2A,
    0x2A, 0x00, 0x00,
    0x2A, 0x00, 0x2A,
    0x2A, 0x15, 0x00,
    0x2A, 0x2A, 0x2A,
    0x15, 0x15, 0x15,
    0x15, 0x15, 0x3F,
    0x15, 0x3F, 0x15,
    0x15, 0x3F, 0x3F,
    0x3F, 0x15, 0x15,
    0x3F, 0x15, 0x3F,
    0x3F, 0x3F, 0x15,
    0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00,
    0x05, 0x05, 0x05,
    0x08, 0x08, 0x08,
    0x0B, 0x0B, 0x0B,
    0x0E, 0x0E, 0x0E,
    0x11, 0x11, 0x11,
    0x14, 0x14, 0x14,
    0x18, 0x18, 0x18,
    0x1C, 0x1C, 0x1C,
    0x20, 0x20, 0x20,
    0x24, 0x24, 0x24,
    0x28, 0x28, 0x28,
    0x2D, 0x2D, 0x2D,
    0x32, 0x32, 0x32,
    0x38, 0x38, 0x38,
    0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x3F,
    0x10, 0x00, 0x3F,
    0x1F, 0x00, 0x3F,
    0x2F, 0x00, 0x3F,
    0x3F, 0x00, 0x3F,
    0x3F, 0x00, 0x2F,
    0x3F, 0x00, 0x1F,
    0x3F, 0x00, 0x10,
    0x3F, 0x00, 0x00,
    0x3F, 0x10, 0x00,
    0x3F, 0x1F, 0x00,
    0x3F, 0x2F, 0x00,
    0x3F, 0x3F, 0x00,
    0x2F, 0x3F, 0x00,
    0x1F, 0x3F, 0x00,
    0x10, 0x3F, 0x00,
    0x00, 0x3F, 0x00,
    0x00, 0x3F, 0x10,
    0x00, 0x3F, 0x1F,
    0x00, 0x3F, 0x2F,
    0x00, 0x3F, 0x3F,
    0x00, 0x2F, 0x3F,
    0x00, 0x1F, 0x3F,
    0x00, 0x10, 0x3F,
    0x1F, 0x1F, 0x3F,
    0x27, 0x1F, 0x3F,
    0x2F, 0x1F, 0x3F,
    0x37, 0x1F, 0x3F,
    0x3F, 0x1F, 0x3F,
    0x3F, 0x1F, 0x37,
    0x3F, 0x1F, 0x2F,
    0x3F, 0x1F, 0x27,
    0x3F, 0x1F, 0x1F,
    0x3F, 0x27, 0x1F,
    0x3F, 0x2F, 0x1F,
    0x3F, 0x37, 0x1F,
    0x3F, 0x3F, 0x1F,
    0x37, 0x3F, 0x1F,
    0x2F, 0x3F, 0x1F,
    0x27, 0x3F, 0x1F,
    0x1F, 0x3F, 0x1F,
    0x1F, 0x3F, 0x27,
    0x1F, 0x3F, 0x2F,
    0x1F, 0x3F, 0x37,
    0x1F, 0x3F, 0x3F,
    0x1F, 0x37, 0x3F,
    0x1F, 0x2F, 0x3F,
    0x1F, 0x27, 0x3F,
    0x2D, 0x2D, 0x3F,
    0x31, 0x2D, 0x3F,
    0x36, 0x2D, 0x3F,
    0x3A, 0x2D, 0x3F,
    0x3F, 0x2D, 0x3F,
    0x3F, 0x2D, 0x3A,
    0x3F, 0x2D, 0x36,
    0x3F, 0x2D, 0x31,
    0x3F, 0x2D, 0x2D,
    0x3F, 0x31, 0x2D,
    0x3F, 0x36, 0x2D,
    0x3F, 0x3A, 0x2D,
    0x3F, 0x3F, 0x2D,
    0x3A, 0x3F, 0x2D,
    0x36, 0x3F, 0x2D,
    0x31, 0x3F, 0x2D,
    0x2D, 0x3F, 0x2D,
    0x2D, 0x3F, 0x31,
    0x2D, 0x3F, 0x36,
    0x2D, 0x3F, 0x3A,
    0x2D, 0x3F, 0x3F,
    0x2D, 0x3A, 0x3F,
    0x2D, 0x36, 0x3F,
    0x2D, 0x31, 0x3F,
    0x00, 0x00, 0x1C,
    0x07, 0x00, 0x1C,
    0x0E, 0x00, 0x1C,
    0x15, 0x00, 0x1C,
    0x1C, 0x00, 0x1C,
    0x1C, 0x00, 0x15,
    0x1C, 0x00, 0x0E,
    0x1C, 0x00, 0x07,
    0x1C, 0x00, 0x00,
    0x1C, 0x07, 0x00,
    0x1C, 0x0E, 0x00,
    0x1C, 0x15, 0x00,
    0x1C, 0x1C, 0x00,
    0x15, 0x1C, 0x00,
    0x0E, 0x1C, 0x00,
    0x07, 0x1C, 0x00,
    0x00, 0x1C, 0x00,
    0x00, 0x1C, 0x07,
    0x00, 0x1C, 0x0E,
    0x00, 0x1C, 0x15,
    0x00, 0x1C, 0x1C,
    0x00, 0x15, 0x1C,
    0x00, 0x0E, 0x1C,
    0x00, 0x07, 0x1C,
    0x0E, 0x0E, 0x1C,
    0x11, 0x0E, 0x1C,
    0x15, 0x0E, 0x1C,
    0x18, 0x0E, 0x1C,
    0x1C, 0x0E, 0x1C,
    0x1C, 0x0E, 0x18,
    0x1C, 0x0E, 0x15,
    0x1C, 0x0E, 0x11,
    0x1C, 0x0E, 0x0E,
    0x1C, 0x11, 0x0E,
    0x1C, 0x15, 0x0E,
    0x1C, 0x18, 0x0E,
    0x1C, 0x1C, 0x0E,
    0x18, 0x1C, 0x0E,
    0x15, 0x1C, 0x0E,
    0x11, 0x1C, 0x0E,
    0x0E, 0x1C, 0x0E,
    0x0E, 0x1C, 0x11,
    0x0E, 0x1C, 0x15,
    0x0E, 0x1C, 0x18,
    0x0E, 0x1C, 0x1C,
    0x0E, 0x18, 0x1C,
    0x0E, 0x15, 0x1C,
    0x0E, 0x11, 0x1C,
    0x14, 0x14, 0x1C,
    0x16, 0x14, 0x1C,
    0x18, 0x14, 0x1C,
    0x1A, 0x14, 0x1C,
    0x1C, 0x14, 0x1C,
    0x1C, 0x14, 0x1A,
    0x1C, 0x14, 0x18,
    0x1C, 0x14, 0x16,
    0x1C, 0x14, 0x14,
    0x1C, 0x16, 0x14,
    0x1C, 0x18, 0x14,
    0x1C, 0x1A, 0x14,
    0x1C, 0x1C, 0x14,
    0x1A, 0x1C, 0x14,
    0x18, 0x1C, 0x14,
    0x16, 0x1C, 0x14,
    0x14, 0x1C, 0x14,
    0x14, 0x1C, 0x16,
    0x14, 0x1C, 0x18,
    0x14, 0x1C, 0x1A,
    0x14, 0x1C, 0x1C,
    0x14, 0x1A, 0x1C,
    0x14, 0x18, 0x1C,
    0x14, 0x16, 0x1C,
    0x00, 0x00, 0x10,
    0x04, 0x00, 0x10,
    0x08, 0x00, 0x10,
    0x0C, 0x00, 0x10,
    0x10, 0x00, 0x10,
    0x10, 0x00, 0x0C,
    0x10, 0x00, 0x08,
    0x10, 0x00, 0x04,
    0x10, 0x00, 0x00,
    0x10, 0x04, 0x00,
    0x10, 0x08, 0x00,
    0x10, 0x0C, 0x00,
    0x10, 0x10, 0x00,
    0x0C, 0x10, 0x00,
    0x08, 0x10, 0x00,
    0x04, 0x10, 0x00,
    0x00, 0x10, 0x00,
    0x00, 0x10, 0x04,
    0x00, 0x10, 0x08,
    0x00, 0x10, 0x0C,
    0x00, 0x10, 0x10,
    0x00, 0x0C, 0x10,
    0x00, 0x08, 0x10,
    0x00, 0x04, 0x10,
    0x08, 0x08, 0x10,
    0x0A, 0x08, 0x10,
    0x0C, 0x08, 0x10,
    0x0E, 0x08, 0x10,
    0x10, 0x08, 0x10,
    0x10, 0x08, 0x0E,
    0x10, 0x08, 0x0C,
    0x10, 0x08, 0x0A,
    0x10, 0x08, 0x08,
    0x10, 0x0A, 0x08,
    0x10, 0x0C, 0x08,
    0x10, 0x0E, 0x08,
    0x10, 0x10, 0x08,
    0x0E, 0x10, 0x08,
    0x0C, 0x10, 0x08,
    0x0A, 0x10, 0x08,
    0x08, 0x10, 0x08,
    0x08, 0x10, 0x0A,
    0x08, 0x10, 0x0C,
    0x08, 0x10, 0x0E,
    0x08, 0x10, 0x10,
    0x08, 0x0E, 0x10,
    0x08, 0x0C, 0x10,
    0x08, 0x0A, 0x10,
    0x0B, 0x0B, 0x10,
    0x0C, 0x0B, 0x10,
    0x0D, 0x0B, 0x10,
    0x0F, 0x0B, 0x10,
    0x10, 0x0B, 0x10,
    0x10, 0x0B, 0x0F,
    0x10, 0x0B, 0x0D,
    0x10, 0x0B, 0x0C,
    0x10, 0x0B, 0x0B,
    0x10, 0x0C, 0x0B,
    0x10, 0x0D, 0x0B,
    0x10, 0x0F, 0x0B,
    0x10, 0x10, 0x0B,
    0x0F, 0x10, 0x0B,
    0x0D, 0x10, 0x0B,
    0x0C, 0x10, 0x0B,
    0x0B, 0x10, 0x0B,
    0x0B, 0x10, 0x0C,
    0x0B, 0x10, 0x0D,
    0x0B, 0x10, 0x0F,
    0x0B, 0x10, 0x10,
    0x0B, 0x0F, 0x10,
    0x0B, 0x0D, 0x10,
    0x0B, 0x0C, 0x10,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00
};
#endif /*_TVIAFB_H_*/
