/*----------------------------------------------------------------------
            -----General Notice-----

  Copyright (C) 1997-2002, Tvia, Inc. (formerly IGS Technologies, Inc.)

  PERMISSION IS HEREBY GRANTED TO USE, COPY AND MODIFY THIS SOFTWARE
  ONLY FOR THE PURPOSE OF DEVELOPING IGS TECHNOLOGIES RELATED PRODUCTS.

  THIS SOFTWARE MAY NOT BE DISTRIBUTED TO ANY PARTY WHICH IS NOT COVERED
  BY IGS TECHNOLOGIES NON-DISCLOSURE AGREEMENT (NDA). UNAUTHORIZED DISCLOSURE
  IS AGAINST LAW AND STRICTLY PROHIBITED.

  THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
  EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

            -----About This File----

                 IGSTV.H

  The file contains settings for the integrated TV Encoder.

            --Modification History--

  1. 03-26-1999 Rev 2.00
  2. 06-23-1999 Scart TV support - add 0xE558 - 0xE55E for all TV tables.

  ---------------------------------------------------------------------*/

#include "tvia.h"

/*
  TV Register settings for specific resoultion and TV type (NTSC or PAL).
  All color depth (8/16/24 bpp) with same resolution use same TV Registers.
*/

static TVRegisters TV720x576x50;
static TVRegisters TV640x480x60_SDRAM;
static TVRegisters TV640x480x50_SDRAM;

TVRegisters *TVModeReg_5202_SDRAM[TOTAL_MODES_5202] = {
    NULL,                 /* 0 */
    &TV720x576x50,        /* 1: PAL */ 
    &TV640x480x50_SDRAM,  /* 2: doesn't work */
    &TV640x480x60_SDRAM,  /* 3: Advantech */
    &TV640x480x60_SDRAM   /* 4: ? */
};

/* This table is for PAL 720x576 */
static TVRegisters TV720x576x50 = { //Wei
    {
     0xE430, 0x0000,
     0xE434, 0x0000,
     0xE438, 0x2000,
     0xE43C, 0x026A,
     0xE440, 0x000B,
     0xE444, 0x026A,
     0xE448, 0x000B,
     0xE44C, 0x8036,
     0xE450, 0x3850,
     0xE454, 0x01F9,
     0xE458, 0xF10F,
     0xE45C, 0x0000,
     0xE460, 0x0045,
     0xE464, 0x045A,
#if SCARTTV
     0xE468, 0x0429,
#else
     0xE468, 0x0431,
#endif
     0xE46C, 0x0411,
     0xE470, 0x004e,
     0xE474, 0x006e,
     0xE478, 0x0368,
     0xE47C, 0x00A0,
     0xE480, 0x026E,
     0xE484, 0x000B,
     0xE488, 0x026D,
     0xE48C, 0x000B,
     0xE490, 0x0001,
     0xE494, 0x0006,
     0xE498, 0x0001,
     0xE49C, 0x0006,
     0xE4A0, 0xCCCC,
     0xE4A4, 0x48CC,
     0xE4A8, 0x0420,
     0xE4AC, 0x1020,
     0xE4B0, 0x0000,
     0xE4B4, 0x0000,
     0xE4B8, 0x0048,
     0xE4BC, 0x0001,
     0xE4C0, 0x0310,
     0xE4C4, 0x0001,
     0xE4C8, 0x8700,
     0xE4CC, 0x8787,
     0xE4D0, 0x0030,
     0xE4D4, 0x4000,
     0xE4D8, 0x0000,
     0xE4DC, 0x0000,
     0xE4E0, 0x0000,
     0xE4E4, 0x0000,
     0xE4E8, 0x0000,
     0xE4EC, 0x0004,
     0xE4F0, 0x0000,
     0xE4F4, 0x0002,
     0xE4F8, 0x0000,
     0xE4FC, 0x0000,
     0xE500, 0x0000,
     0xE504, 0x0000,
     0xE508, 0x0000,
     0xE50C, 0x0000,
     0xE510, 0x0000,
     0xE514, 0x0000,
     0xE518, 0x0000,
     0xE51C, 0x0000,
     0xE520, 0x0000,
     0xE524, 0x045B,
     0xE528, 0x0052,
     0xE52C, 0x0011,
     0xE558, 0x0000,
     0xE55A, 0xB300,
     0xE55C, 0x0000,
     0xE55E, 0xB300,
     0xE574, 0x026A,
     0xE576, 0x000C,
     0xE578, 0x026A,
     0xE57A, 0x000C,
     0xE57C, 0x026C,
     0xE57E, 0x000A,
     0xE580, 0x026C,
     0xE582, 0x000E,
     0xE584, 0x0000,
     0xE586, 0x0002,
     0xE588, 0x0042,
     0xE58A, 0x004A,
     0xE58C, 0x0001,
     0xE58E, 0x000A,
     0xE590, 0x004B,
     0xE592, 0x0000,
     0xE594, 0x004B,
     0xE59E, 0x8F86,
     /* 5300 new filter data */
     0xE5AC, 0x01FF,
     0xE5AE, 0x01FF,
     0xE5B0, 0x0000,
     0xE5B2, 0x0002,
     0xE5B4, 0x0005,
     0xE5B6, 0x000A,
     0xE5B8, 0x0010,
     0xE5BA, 0x0016,
     0xE5BC, 0x001C,
     0xE5BE, 0x0020,
     0xE5C0, 0x0021,
     0xE5C2, 0x01FF,
     0xE5C4, 0x01FF,
     0xE5C6, 0x0000,
     0xE5C8, 0x0002,
     0xE5CA, 0x0005,
     0xE5CC, 0x000A,
     0xE5CE, 0x0010,
     0xE5D0, 0x0016,
     0xE5D2, 0x001C,
     0xE5D4, 0x0020,
     0xE5D6, 0x0021,
     0xE5D8, 0x0000,
     0xE5DA, 0x0000,
     0xE5DC, 0x0000,
     0xE5DE, 0x0000,
     0xE5E0, 0x0000,
     0xE5E2, 0x0000,
     0xE5E4, 0x0000,
     0xE5E6, 0x0000,
     0xE5E8, 0x0000,
     0xE5EA, 0x0000,
     0xE5EC, 0x00FF,
     0xE5EE, 0x0028,
     0xE5F0, 0x013E,
     0xE5F2, 0x110D,
     0xE5F4, 0xF48D,
     0xE5F6, 0x000E,
     0xE5F8, 0x0000,
     0xE5FA, 0x0000,
     0xE5FC, 0x0000,
     0xE5FE, 0x0000,
     0xE600, 0x0000,
     0xE602, 0x0000,
     0xE604, 0xFF00,
     0xE606, 0x0000,
     0xE60C, 0x0000,
     0xE60E, 0x830F,
     0xE61C, 0x0014,
     0xE61E, 0x001C,
     0xE622, 0x0210,
     0xE624, 0x7EA0,
     0xE628, 0x8040,
     0xE62A, 0x0040,
     0xE690, 0x4461             /*Ub and Vb gain */
     }
};

/* This table is for PAL 720x576 */
static TVRegisters TV720x576x50_from_sdk_3_0 =
{
    0xE430,  0x0000,
    0xE434,  0x0000,
    0xE438,  0x2000,
    0xE43C,  0x026B,
    0xE440,  0x0028,
    0xE444,  0x026C,
    0xE448,  0x0028,
    0xE44C,  0xA63C,
    0xE450,  0x2E42,
    0xE454,  0x01F8,
    0xE458,  0xF10E,
    0xE45C,  0x0000,
    0xE460,  0x0043,
    0xE464,  0x045A,
#if SCARTTV
    0xE468,  0x0429,
#else
    0xE468,  0x042F,
#endif
    0xE46C,  0x0411,
    0xE470,  0x004C,
    0xE474,  0x006A,
    0xE478,  0x0350,
    0xE47C,  0x0081,
    0xE480,  0x026E,
    0xE484,  0x000B,
    0xE488,  0x026D,
    0xE48C,  0x000B,
    0xE490,  0x0001,
    0xE494,  0x0006,
    0xE498,  0x0001,
    0xE49C,  0x0006,
    0xE4A0,  0xCCCC,
    0xE4A4,  0x48CC,
    0xE4A8,  0x0420,
    0xE4AC,  0x1020,
    0xE4B0,  0x0000,
    0xE4B4,  0x0000,
    0xE4B8,  0x0042,
    0xE4BC,  0x0001,
    0xE4C0,  0x02EA,
    0xE4C4,  0x0001,
    0xE4C8,  0x8700,
    0xE4CC,  0x8787,
    0xE4D0,  0x0030,
    0xE4D4,  0x4C00,
    0xE4D8,  0x0000,
    0xE4DC,  0x0000,
    0xE4E0,  0x0000,
    0xE4E4,  0x0000,
    0xE4E8,  0x0000,
    0xE4EC,  0x0004,
    0xE4F0,  0x0000,
    0xE4F4,  0x0002,
    0xE4F8,  0x0000,
    0xE4FC,  0x0000,
    0xE500,  0x0000,
    0xE504,  0x0000,
    0xE508,  0x0000,
    0xE50C,  0x0000,
    0xE510,  0x0000,
    0xE514,  0x0000,
    0xE518,  0x0000,
    0xE51C,  0x0000,
    0xE520,  0x0000,
    0xE524,  0x045B,
    0xE528,  0x004C,
    0xE52C,  0x0011,
    0xE558,  0x0000,
    0xE55A,  0xB300,
    0xE55C,  0x0000,
    0xE55E,  0xB300,
    0xE574,  0x026A,
    0xE576,  0x000C,
    0xE578,  0x026A,
    0xE57A,  0x000C,
    0xE57C,  0x026C,
    0xE57E,  0x000A,
    0xE580,  0x026C,
    0xE582,  0x000E,
    0xE584,  0x0000,
    0xE586,  0x0003,
    0xE588,  0x003E,
    0xE58A,  0x0043,
    0xE58C,  0x0001,
    0xE58E,  0x000A,
    0xE590,  0x004B,
    0xE592,  0x0000,
    0xE594,  0x004B,
    0xE598,  0x02E1,
    0xE59A,  0x02EA,
    0xE59C,  0x8600,
    0xE59E,  0x8C86,
    /* 5300 new filter data */
    0xE5AC,  0x01FF,
    0xE5AE,  0x01FF,
    0xE5B0,  0x0000,
    0xE5B2,  0x0002,
    0xE5B4,  0x0005,
    0xE5B6,  0x000A,
    0xE5B8,  0x0010,
    0xE5BA,  0x0016,
    0xE5BC,  0x001C,
    0xE5BE,  0x0020,
    0xE5C0,  0x0021,
    0xE5C2,  0x01FF,
    0xE5C4,  0x01FF,
    0xE5C6,  0x0000,
    0xE5C8,  0x0002,
    0xE5CA,  0x0005,
    0xE5CC,  0x000A,
    0xE5CE,  0x0010,
    0xE5D0,  0x0016,
    0xE5D2,  0x001C,
    0xE5D4,  0x0020,
    0xE5D6,  0x0021,
    0xE5D8,  0x0000,
    0xE5DA,  0x0000,
    0xE5DC,  0x0000,
    0xE5DE,  0x0000,
    0xE5E0,  0x0000,
    0xE5E2,  0x0000,
    0xE5E4,  0x0000,
    0xE5E6,  0x0000,
    0xE5E8,  0x0000,
    0xE5EA,  0x0000,
    0xE5EC,  0x00FF,
    0xE5EE,  0x0028,
    0xE5F0,  0x0150,
    0xE5F2,  0x080D,
    0xE5F4,  0xED0D,
    0xE5F6,  0x000E,
    0xE5F8,  0x0000,
    0xE5FA,  0x0000,
    0xE5FC,  0x0000,
    0xE5FE,  0x0000,
    0xE600,  0x0000,
    0xE602,  0x0000,
    0xE604,  0xFF00,
    0xE606,  0x0000,
    0xE60C,  0x0000,
    0xE60E,  0x830F,
    0xE61C,  0x0016, /*0x0015,*/
    0xE61E,  0x001C,
    0xE622,  0x0110,
    0xE624,  0x7EA0,
    0xE628,  0x8040,
    0xE62A,  0x0040,
    0xE690,  0x4461    /*Ub and Vb gain*/
};


/* -------------------------------------------------------------
   TV registers' settings
   -------------------------------------------------------------*/
/* This table is for PAL 640x480 @50Hz mode */
static TVRegisters TV640x480x50_SDRAM =
{
    0xE430,  0x0000, 
// NON VA !!    0xE430,  0x0008, // !!!raf
    0xE434,  0x0000,  // ORIG 
//    0xE434,  0x0040, //!!!raf
// !!!raf 0xE434,  0x0020,
    0xE438,  0x2000,  /* ORIG */
//    0xE438,  0x2800, //!!!raf
    0xE43C,  0x023C,
    0xE440,  0x005A,
    0xE444,  0x023C,
    0xE448,  0x005A,
    0xE44C,  0xA63C,   /* ORIG */
//    0xE44C,  0x1020, // Y_BLANK_AMP[15:8] e SYNC_AMP[7:0]
    0xE450,  0x2E42,
    0xE454,  0xFEF8,
    0xE458,  0xF10E,
    0xE45C,  0x0000,
    0xE460,  0x0045,
    0xE464,  0x045A,
#if SCARTTV
    0xE468,  0x0429,
#else
    0xE468,  0x0430,
#endif
    0xE46C,  0x0411,
    0xE470,  0x0050,
    0xE474,  0x006F,
    0xE478,  0x033A,
    0xE47C,  0x00B6,
    0xE480,  0x026E,
    0xE484,  0x000B,
    0xE488,  0x026D,
    0xE48C,  0x000B,
    0xE490,  0x0001,
    0xE494,  0x0006,
    0xE498,  0x0001,
    0xE49C,  0x0006,
    0xE4A0,  0xCCCC,
    0xE4A4,  0x48CC,
    0xE4A8,  0x0420,
    0xE4AC,  0x1020,
    0xE4B0,  0x0000,
    0xE4B4,  0x0000,
    0xE4B8,  0x0046,
    0xE4BC,  0x0001,
    0xE4C0,  0x0300,
    0xE4C4,  0x0001,
    0xE4C8,  0x8700,
    0xE4CC,  0x8787,
    0xE4D0,  0x0030,
    0xE4D4,  0x4000,
    0xE4D8,  0x0000,
    0xE4DC,  0x3000,
    0xE4E0,  0x0000,
    0xE4E4,  0x0000,
    0xE4E8,  0x0000, 
// !!!raf    0xE4E8,  0x0100,
    0xE4EC,  0x0004,
    0xE4F0,  0x0000,
    0xE4F4,  0x0002,
    0xE4F8,  0x0000,
    0xE4FC,  0x0000,
    0xE500,  0x0000,
    0xE504,  0x0000,
    0xE508,  0x0000,
    0xE50C,  0x0000,
    0xE510,  0x0000,
    0xE514,  0x0000,
    0xE518,  0x0000,
    0xE51C,  0x0000,
    0xE520,  0x0000,
    0xE524,  0x045B,
    0xE528,  0x004C,
    0xE52C,  0x0014,
    0xE558,  0x0000,
    0xE55A,  0xB300,
    0xE55C,  0x0000,
    0xE55E,  0xB300,
    0xE574,  0x026A,
    0xE576,  0x000C,
    0xE578,  0x026A,
    0xE57A,  0x000C,
    0xE57C,  0x026C,
    0xE57E,  0x000A,
    0xE580,  0x026C,
    0xE582,  0x000E,
    0xE584,  0x0000,
    0xE586,  0x0003,
    0xE588,  0x0040,
    0xE58A,  0x0045,
    0xE58C,  0x0001,
    0xE58E,  0x000A,
    0xE590,  0x0048,
    0xE592,  0x0000,
    0xE594,  0x0048,
    0xE598,  0x02F7,
    0xE59A,  0x02FF,
    0xE59C,  0x8600,
    0xE59E,  0x8C86,
    0xE5AC,  0x01FF,
    0xE5AE,  0x01FF,
    0xE5B0,  0x0000,
    0xE5B2,  0x0002,
    0xE5B4,  0x0005,
    0xE5B6,  0x000A,
    0xE5B8,  0x0010,
    0xE5BA,  0x0016,
    0xE5BC,  0x001C,
    0xE5BE,  0x0020,
    0xE5C0,  0x0021,
    0xE5C2,  0x01FF,
    0xE5C4,  0x01FF,
    0xE5C6,  0x0000,
    0xE5C8,  0x0002,
    0xE5CA,  0x0005,
    0xE5CC,  0x000A,
    0xE5CE,  0x0010,
    0xE5D0,  0x0016,
    0xE5D2,  0x001C,
    0xE5D4,  0x0020,
    0xE5D6,  0x0021,
    0xE5D8,  0x0000,
    0xE5DA,  0x0000,
    0xE5DC,  0x0000,
    0xE5DE,  0x0000,
    0xE5E0,  0x0000,
    0xE5E2,  0x0000,
    0xE5E4,  0x0000,
    0xE5E6,  0x0000,
    0xE5E8,  0x0000,
    0xE5EA,  0x0000,
    0xE5EC,  0x00FF,
    0xE5EE,  0x0028,
    0xE5F0,  0x0147,
    0xE5F2,  0x05CD,
    0xE5F4,  0xEA0D,
    0xE5F6,  0x000E,
    0xE5F8,  0x0000,
    0xE5FA,  0x0000,
    0xE5FC,  0x0000,
    0xE5FE,  0x0000,
    0xE600,  0x0000,
    0xE602,  0x0000, 
    0xE604,  0xFF00, /*  ORIG */
//  0xE604,  0xd040, // !!!raf
    0xE606,  0x0000, /* ORIG */
//    0xE606,  0xa0f0, //!!!raf
    0xE60C,  0x0000,
    0xE60E,  0x830F,
    0xE61C,  0x0017, /* 0x0016, */
    0xE61E,  0x001C,
    0xE622,  0x0110,
    0xE624,  0x7EA0,
    0xE628,  0x8040,
    0xE62A,  0x0040,
    0xE690,  0x4461
};

/* This table is for NTSC 640x480 mode */
static TVRegisters TV640x480x60_SDRAM = {
#if 1 // from TVIA SDK
    {
    0xE430,  0x0000,
    0xE434,  0x0000,
    0xE438,  0x3000,
    0xE43C,  0xE20C,
    0xE440,  0xE029, 
    0xE444,  0xE20C,
    0xE448,  0xE029,
    0xE44C,  0x8037,
    0xE450,  0x3850,
    0xE454,  0xFE06,
    0xE458,  0xE800,
    0xE45C,  0xE000,
    0xE460,  0xE03A,
    0xE464,  0xE401,
    0xE468,  0xE40F,
    0xE46C,  0xE41E,
    0xE470,  0xE042,
    0xE474,  0xE062,
    0xE478,  0xE30C,
    0xE47C,  0xE074,
    0xE480,  0xE20C,
    0xE484,  0xE011,
    0xE488,  0xE20C,
    0xE48C,  0xE011,
    0xE490,  0xE005,
    0xE494,  0xE00B,
    0xE498,  0xE005,
    0xE49C,  0xE00B,
    0xE4A0,  0x6666,
    0xE4A4,  0x2466,
    0xE4A8,  0xE420,
    0xE4AC,  0x1020,
    0xE4B0,  0xE400,
    0xE4B4,  0x0000,
    0xE4B8,  0x003B,
    0xE4BC,  0x0001,
    0xE4C0,  0x02AB,
    0xE4C4,  0x0001,
    0xE4C8,  0x8700,
    0xE4CC,  0x8787,
    0xE4D0,  0x0030,
    0xE4D4,  0x4000,
    0xE4D8,  0x0000,
    0xE4DC,  0x0000,
    0xE4E0,  0x0000,
    0xE4E4,  0x0000,
    0xE4E8,  0xE400,
    0xE4EC,  0x0004,
    0xE4F0,  0x0000,
    0xE4F4,  0x0002,
    0xE4F8,  0x0000,
    0xE4FC,  0x0000,
    0xE500,  0x0000,
    0xE504,  0xE400,
    0xE508,  0x0000,
    0xE50C,  0xE000,
    0xE510,  0xE400,
    0xE514,  0xE000,
    0xE518,  0xE000,
    0xE51C,  0xE400,
    0xE520,  0xE400,
    0xE524,  0xE402,
    0xE528,  0xE00E,
    0xE52C,  0xE02B,
    0xE558,  0x0000,
    0xE55A,  0xB300,
    0xE55C,  0x0000,
    0xE55E,  0xB300,
    0xE574,  0xE20C,
    0xE576,  0xE011,
    0xE578,  0xE20C,
    0xE57A,  0xE011,
    0xE57C,  0xE20C,
    0xE57E,  0xE011,
    0xE580,  0xE20C,
    0xE582,  0xE011,
    0xE5AC,  0xE401,
    0xE5AE,  0xE3FF,
    0xE5B0,  0xE5FD,
    0xE5B2,  0xE3FC,
    0xE5B4,  0xE5FD,
    0xE5B6,  0xE201,
    0xE5B8,  0xE40A,
    0xE5BA,  0xE217,
    0xE5BC,  0xE424,
    0xE5BE,  0xE22D,
    0xE5C0,  0xE431,
    0xE5C2,  0xE201,
    0xE5C4,  0xE5FF,
    0xE5C6,  0xE3FD,
    0xE5C8,  0xE5FC,
    0xE5CA,  0xE3FD,
    0xE5CC,  0xE401,
    0xE5CE,  0xE20A,
    0xE5D0,  0xE417,
    0xE5D2,  0xE224,
    0xE5D4,  0xE42D,
    0xE5D6,  0xE231,
    0xE5D8,  0xE400,
    0xE5DA,  0xE200,
    0xE5DC,  0xE400,
    0xE5DE,  0xE200,
    0xE5E0,  0xE400,
    0xE5E2,  0xE200,
    0xE5E4,  0xE400,
    0xE5E6,  0xE200,
    0xE5E8,  0xE400,
    0xE5EA,  0xE200,
    0xE5EC,  0xE4FF,
    0xE5EE,  0x0028,
    0xE5F0,  0xE123,
    0xE5F2,  0x0500,
    0xE5F4,  0xEC00,
    0xE5F6,  0x000E,
    0xE5F8,  0xE000,
    0xE5FA,  0xE000,
    0xE5FC,  0xE000,
    0xE5FE,  0xE000,
    0xE600,  0xE000,
    0xE602,  0xE000,
    0xE564,  0xE03F,
    0xE566,  0xE05D,
    0xE604,  0xFF00,
    0xE606,  0x0000,
    0xE60C,  0x0000,
    0xE60E,  0x830F,
    0xE61C,  0x0014,
    0xE61E,  0x001C,
    0xE622,  0x0110,
    0xE624,  0x7EA0,
    0xE628,  0x8040,
    0xE62A,  0x0040,
    0xE690,  0x4461
    }
#else // from Advantech
    {
     0xE430, 0x0000,
     0xE434, 0x0000,
     0xE438, 0x2000,
     0xE43C, 0x5A6B,
     0xE440, 0x582C,
     0xE444, 0x5A6B,
     0xE448, 0x582C,
     0xE44C, 0x8036,
     0xE450, 0x5072,
     0xE454, 0xFA06,
     0xE458, 0xE818,
     0xE45C, 0x5800,
     0xE460, 0x584A,
     0xE464, 0x5C64,
     0xE468, 0x5C46,
     0xE46C, 0x5C35,
     0xE470, 0x5858,
     0xE474, 0x587C,
     0xE478, 0x5BE7,
     0xE47C, 0x58A7,
     0xE480, 0x5A6E,
     0xE484, 0x580B,
     0xE488, 0x5A6D,
     0xE48C, 0x580B,
     0xE490, 0x5801,
     0xE494, 0x5806,
     0xE498, 0x5801,
     0xE49C, 0x5806,
     0xE4A0, 0xCCCC,
     0xE4A4, 0x48CC,
     0xE4A8, 0x5E20,
     0xE4AC, 0x1020,
     0xE4B0, 0x5E00,
     0xE4B4, 0x0000,
     0xE4B8, 0xE04D,
     0xE4BC, 0x0001,
     0xE4C0, 0xE36B,
     0xE4C4, 0xE001,
     0xE4C8, 0x8700,
     0xE4CC, 0x8787,
     0xE4D0, 0x0030,
     0xE4D4, 0x4000,
     0xE4D8, 0x0000,
     0xE4DC, 0x0000,
     0xE4E0, 0x0000,
     0xE4E4, 0x0000,
     0xE4E8, 0x5C00,
     0xE4EC, 0x0004,
     0xE4F0, 0x0000,
     0xE4F4, 0x0002,
     0xE4F8, 0x0000,
     0xE4FC, 0x0000,
     0xE500, 0x0000,
     0xE504, 0x5E00,
     0xE508, 0x0000,
     0xE50C, 0x5800,
     0xE510, 0x5E00,
     0xE514, 0x5800,
     0xE518, 0x5800,
     0xE51C, 0x5C00,
     0xE520, 0x5C00,
     0xE524, 0x5C65,
     0xE528, 0x5877,
     0xE52C, 0x5841,
     0xE558, 0x0000,
     0xE55A, 0xB300,
     0xE55C, 0x0000,
     0xE55E, 0xB300,
     0xE574, 0x5A6A,
     0xE576, 0x000C,
     0xE578, 0x5A6A,
     0xE57C, 0x000C,
     0xE57E, 0x000A,
     0xE580, 0x5A6C,
     0xE582, 0x000E,
     0xE5AC, 0x5E01,
     0xE5AE, 0x01FF,
     0xE5B0, 0x5FFD,
     0xE5B2, 0x01FC,
     0xE5B4, 0x5FFD,
     0xE5B6, 0x0001,
     0xE5B8, 0x5E0A,
     0xE5BC, 0x5E24,
     0xE5BE, 0x002D,
     0xE5C0, 0x5E31,
     0xE5C2, 0x0001,
     0xE5C4, 0x5FFF,
     0xE5C6, 0x01FD,
     0xE5C8, 0x5FFC,
     0xE5CA, 0x01FD,
     0xE5CC, 0x5E01,
     0xE5CE, 0x000A,
     0xE5D0, 0x5E17,
     0xE5D2, 0x0024,
     0xE5D4, 0x5E2D,
     0xE5D6, 0x0031,
     0xE5D8, 0x5E00,
     0xE5DA, 0x0000,
     0xE5DC, 0x5E00,
     0xE5DE, 0x0000,
     0xE5E0, 0x5E00,
     0xE5E2, 0x0000,
     0xE5E4, 0x5E00,
     0xE5E6, 0x0000,
     0xE5E8, 0x5E00,
     0xE5EA, 0x0000,
     0xE5EC, 0x5EFF,
     0xE5EE, 0x0028,
     0xE5F0, 0x591B,
     0xE5F2, 0x180D,
     0xE5F4, 0xF80D,
     0xE5F6, 0x000E,
     0xE5F8, 0x5800,
     0xE5FA, 0x0000,
     0xE5FC, 0x5800,
     0xE5FE, 0x0000,
     0xE600, 0x5800,
     0xE602, 0x0000,
     0xE564, 0x583F,
     0xE566, 0x005D,
     0xE604, 0xFF00,
     0xE606, 0x0000,
     0xE60C, 0x0000,
     0xE60E, 0x030F,
     0xE61C, 0x0014,
     0xE61E, 0x001C,
     0xE622, 0x0210,
     0xE624, 0x7EA0,
     0xE628, 0x8040,
     0xE62A, 0x0040,
     0xE690, 0x4461
     }
#endif
};