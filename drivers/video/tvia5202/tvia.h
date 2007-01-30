#ifndef __TVIA_H
#define __TVIA_H

/*
 * tvia.h  *
 * TVia,Inc. CyberPro5k frame buffer device*
*/

#include <linux/types.h>
/*
typedef unsigned char u8;
typedef unsigned short u16;
*/
#define TOTAL_MODES_5202 4

/* this part are the same in 5202*/
#define   SEQCOUNT  0x05
#define   MISCCOUNT 0x01
#define   CRTCOUNT  0x19
#define   ATTRCOUNT 0x15
#define   GRACOUNT  0x9
#define   EXTPARTIALCOUNT 8     /* define 8 extended regs for color depth change */

#define   SREGCOUNT (SEQCOUNT+MISCCOUNT+CRTCOUNT+ATTRCOUNT+GRACOUNT)
#define   EREGCOUNT (EXTPARTIALCOUNT * 2 + 1)

typedef struct _StandardRegs {
    u8 VGARegs[SREGCOUNT];
} StandardRegs;

typedef struct _ExtendedRegs {
    u8 TVIARegs[EREGCOUNT];
} ExtendedRegs;

typedef struct _ModeInit {
    StandardRegs *bpStdReg;     /*VGA standard registers */
    ExtendedRegs *bpExtReg;     /*TVIA extended registers */
} ModeInit;

/*
  TV Register settings for specific resoultion and TV type (NTSC or PAL).
  All color depth (8/16/24 bpp) with same resolution use same TV Registers.
*/

#define REG_TV_SIZE_5202      141*2   // Counted on TV720x576x50
															// for 640x480 (144*2) 
															// orig !!!raf	(133*2)



typedef struct _TVRegisters {
    u16 TVRegs[REG_TV_SIZE_5202];
} TVRegisters;

extern ModeInit *TVIAModeReg_5202[TOTAL_MODES_5202];
extern u8 ExtRegData_5202[114];
extern u8 ExtSGRegData_5202[32];

extern TVRegisters *TVModeReg_5202_SDRAM[TOTAL_MODES_5202];

#endif
