/*
 *   Philips 7114 video decoder driver
 */

#include "tvia5202-def.h"
#include "tvia5202-i2c.h"
#include "tvia5202-misc.h"
#include "saa7114.h"

#define DEBUG0		printk

#define P7114_Count     169

static u8 I2CPort[4] = {0x48, 0x42, 0x4A, 0x40};

static u8 Phil7114_NTSC[2][P7114_Count*2] =
{
    /*Task A is VBI ( if we want to capture raw data )*/
    /*Task B is video*/
    { 
        0x01, 0x08,
        0x02, 0xc2,
        0x03, 0x10,
        0x04, 0x90,
        0x05, 0x90,
        0x06, 0xd0,
        0x07, 0xf6,
        0x08, 0x60, /*0x68, 2003/03/26 */
        0x09, 0x40,
        0x0a, 0x80,
        0x0b, 0x44,
        0x0c, 0x40,
        0x0d, 0x00,
        0x0e, 0x01, /*ntsc = 89, pal=81*/
        0x0f, 0x2a,
        0x10, 0x0e,
        0x11, 0x01,
        0x12, 0x00,
        0x13, 0x40,
        0x14, 0x00,
        0x15, 0x11,
        0x16, 0xfe,
        0x17, 0xc0,
        0x18, 0x40,
        0x19, 0x80,
        0x1a, 0x00,
        0x1b, 0x00,
        0x1c, 0x00,
        0x1d, 0x00,
        0x1e, 0x00,
        0x30, 0xbc,
        0x31, 0xdf,
        0x32, 0x02,
        0x34, 0xcd,
        0x35, 0xcc,
        0x36, 0x3a,
        0x38, 0x03,
        0x39, 0x20,
        0x3a, 0x00,
        0x40, 0x00,
        0x41, 0xff,
        0x42, 0xff,
        0x43, 0xff,
        0x44, 0xff,
        0x45, 0xff,
        0x46, 0xff,
        0x47, 0xff,
        0x48, 0xff,
        0x49, 0xff,
        0x4a, 0xff,
        0x4b, 0xff,
        0x4c, 0xff,
        0x4d, 0xff,
        0x4e, 0xff,
        0x4f, 0xff,
        0x50, 0xff,
        0x51, 0xff,
        0x52, 0xff,
        0x53, 0xff,
        0x54, 0xff,
        0x55, 0xff,
        0x56, 0xff,
        0x57, 0xff,
        0x58, 0x00,
        0x59, 0x47,
        0x5a, 0x06,
        0x5b, 0x03,
        0x5c, 0x00,
        0x5d, 0x00,
        0x5e, 0x00,
        0x5f, 0x00,
        0x63, 0x00,
        0x64, 0x00,
        0x65, 0x00,
        0x66, 0x00,
        0x67, 0x00,
        0x68, 0x00,
        0x80, 0x10, /*10 for task A, 21 for task B*/
        0x83, 0x01, /*00 xport disabled, 01 enabled*/
        0x84, 0xa5,
        0x85, 0x14,
        0x86, 0xc5, /*c5 = text and video, 45 = video*/
        0x87, 0x00, /*00 iport disabled, 01 enabled, for old board, set 01*/
        0x88, 0xd0,
        0x8f, 0x4b,
        
        /*Task A*/
        0x90, 0x00,
        0x91, 0x08,
        0x92, 0x10,
        0x93, 0x80,
        0x94, 0x08,
        0x95, 0x00,
        0x96, 0xd0,
        0x97, 0x02,
        0x98, 0x10,
        0x99, 0x00,
        0x9a, 0xf4,
        0x9b, 0x00,
        0x9c, 0xd0,
        0x9d, 0x02,
        0x9e, 0xf2,
        0x9f, 0x00,
        0xa0, 0x01,
        0xa1, 0x00,
        0xa2, 0x00,
        0xa4, 0x80,
        0xa5, 0x40,
        0xa6, 0x40,
        0xa8, 0x00,
        0xa9, 0x04,
        0xaa, 0x00,
        0xac, 0x00,
        0xad, 0x02,
        0xae, 0x00,
        0xb0, 0x00,
        0xb1, 0x04,
        0xb2, 0x00,
        0xb3, 0x04,
        0xb4, 0x01,
        0xb8, 0x00,
        0xb9, 0x00,
        0xba, 0x00,
        0xbb, 0x00,
        0xbc, 0x00,
        0xbd, 0x00,
        0xbe, 0x00,
        0xbf, 0x00,
        
        /*Task B*/
        0xc0, 0x00,
        0xc1, 0x18, /*18 for xport, 08 for decoder*/
        0xc2, 0x59,
        0xc3, 0x80,
        0xc4, 0x00,
        0xc5, 0x00,
        0xc6, 0xd0,
        0xc7, 0x02,
        0xc8, 0x01,
        0xc9, 0x00,
        0xca, 0xf2, /*ntsc = f2, pal = 38*/
        0xcb, 0x00, /*ntsc = 00, pal = 01*/
        0xcc, 0xd0,
        0xcd, 0x02,
        0xce, 0xf0, /*ntsc = f0, pal = 35*/
        0xcf, 0x00, /*ntsc = 00, pal = 01*/
        0xd0, 0x01,
        0xd1, 0x00,
        0xd2, 0x00,
        0xd4, 0x80,
        0xd5, 0x3f,
        0xd6, 0x3f,
        0xd8, 0x00,
        0xd9, 0x04,
        0xda, 0x00,
        0xdc, 0x00,
        0xdd, 0x02,
        0xde, 0x00,
        0xe0, 0x00,
        0xe1, 0x04,
        0xe2, 0x00,
        0xe3, 0x04,
        0xe4, 0x01,
        0xe8, 0x00,
        0xe9, 0x00,
        0xea, 0x00,
        0xeb, 0x00,
        0xec, 0x00,
        0xed, 0x00,
        0xee, 0x00,
        0xef, 0x00,
        0x88, 0xf0
    },
    
    /*Sliced data - NCI setting*/
    /*Task A is VBI ( sliced data )*/
    /*Task B is video*/
    {
        0x01, 0x08,
        0x02, 0xc2, /*0x82,85*/
        0x03, 0x10,
        0x04, 0x90,
        0x05, 0xa0,
        0x06, 0xd0,
        0x07, 0xf6,
        0x08, 0xC0, /*0xC8, 2003/03/26 */
        0x09, 0x40,
        0x0a, 0x78,
        0x0b, 0x40,
        0x0c, 0x48,
        0x0d, 0x00,
        0x0e, 0x00,
        0x0f, 0x00,
        0x10, 0x06,
        0x11, 0x17,
        0x12, 0x00,
        0x13, 0x40,
        0x14, 0x00,
        0x15, 0x11,
        0x16, 0xfe,
        0x17, 0xc0,
        0x18, 0x40,
        0x19, 0x80,
        0x1a, 0x00,
        0x1b, 0x00,
        0x1c, 0x00,
        0x1d, 0x00,
        0x1e, 0x00,
        0x30, 0xbc,
        0x31, 0xdf,
        0x32, 0x02,
        0x34, 0xcd,
        0x35, 0xcc,
        0x36, 0x3a,
        0x38, 0x03,
        0x39, 0x20,
        0x3a, 0x00,
        0x40, 0x40,
        0x41, 0xff, /*2*/
        0x42, 0xff, /*3*/
        0x43, 0xff, /*4*/
        0x44, 0xff, /*5*/
        0x45, 0xff, /*6*/
        0x46, 0xff, /*7*/
        0x47, 0xcc, /*8*/
        0x48, 0xcc, /*9*/
        0x49, 0xcc, /*10*/
        0x4a, 0xcc, /*11*/
        0x4b, 0xcc, /*12*/
        0x4c, 0xcc, /*13*/
        0x4d, 0xcc, /*14*/
        0x4e, 0xcc, /*15*/
        0x4f, 0xcc, /*16*/
        0x50, 0xcc, /*17*/
        0x51, 0xcc, /*18*/
        0x52, 0xcc, /*19*/
        0x53, 0xcc, /*20*/
        0x54, 0x55, /*21*/
        0x55, 0x55, /*22*/
        0x56, 0xff, /*23*/
        0x57, 0xff, /*24*/
        0x58, 0xe7,
        0x59, 0x47,
        0x5a, 0x06,
        0x5b, 0x83,
        0x5c, 0x00,
        0x5d, 0x3e,
        0x5e, 0x00,
        0x5f, 0x00,
        0x63, 0x00,
        0x64, 0x00,
        0x65, 0x00,
        0x66, 0x00,
        0x67, 0x00,
        0x68, 0x00,
        0x83, 0x00,
        0x84, 0xa0,
        0x85, 0x10,
        0x86, 0xc5,
        0x87, 0x01,
        0x8f, 0x0b,
        
        /*Task A*/
        0x90, 0x01,
        0x91, 0x48,
        0x92, 0x40, /*invert field, 0x40 not invert*/
        0x93, 0x84,
        0x94, 0x02,
        0x95, 0x00,
        0x96, 0xd0,
        0x97, 0x02,
        0x98, 0x05,
        0x99, 0x00,
        0x9a, 0x0c,
        0x9b, 0x00,
        0x9c, 0xf7,
        0x9d, 0x05,
        0x9e, 0x0c,
        0x9f, 0x00,
        0xa0, 0x01,
        0xa1, 0x00,
        0xa2, 0x00,
        0xa4, 0x70,
        0xa5, 0x38,
        0xa6, 0x48,
        0xa8, 0xe2,
        0xa9, 0x01,
        0xaa, 0x00,
        0xac, 0xf1,
        0xad, 0x00,
        0xae, 0x00,
        0xb0, 0x00,
        0xb1, 0x04,
        0xb2, 0x00,
        0xb3, 0x04,
        0xb4, 0x00,
        0xb8, 0x00,
        0xb9, 0x00,
        0xba, 0x00,
        0xbb, 0x00,
        0xbc, 0x00,
        0xbd, 0x00,
        0xbe, 0x00,
        0xbf, 0x00,
        
        /*Task B*/
        0xc0, 0x00,
        0xc1, 0x08,
        0xc2, 0x40,
        0xc3, 0x80,
        0xc4, 0x00,
        0xc5, 0x00,
        0xc6, 0xD0,
        0xc7, 0x02,
        0xc8, 0x0f,
        0xc9, 0x00,
        0xca, 0xf8,
        0xcb, 0x00,
        0xcc, 0xD0,
        0xcd, 0x02,
        0xce, 0xf6,
        0xcf, 0x00,
        0xd0, 0x01,
        0xd1, 0x00,
        0xd2, 0x00,
        0xd4, 0x70,
        0xd5, 0x40,
        0xd6, 0x48,
        0xd8, 0x00,
        0xd9, 0x04,
        0xda, 0x00,
        0xdc, 0x00,
        0xdd, 0x02,
        0xde, 0x00,
        0xe0, 0x00,
        0xe1, 0x04,
        0xe2, 0x00,
        0xe3, 0x04,
        0xe4, 0x01,
        0xe8, 0x00,
        0xe9, 0x00,
        0xea, 0x00,
        0xeb, 0x00,
        0xec, 0x00,
        0xed, 0x00,
        0xee, 0x00,
        0xef, 0x00,
        0x88, 0xf0,
        0x80, 0x60,
        0xff, 0xff
    }
};


static u8 Phil7114_PAL[P7114_Count*2] = 
{
    0x88, 0x58, /* analog chn 2 disabled, audio disabled */

    /* Video Decoder */
    0x01, 0x08, /* Recommemded position */
    0x02, 0xc0, /* Set to AI11 input for F453AV */
    0x03, 0x20, /* 15/11/2005 Cislaghi */
    0x04, 0x90,
    0x05, 0x90,
    0x06, 0xeb,
    0x07, 0xe0,
    0x08, 0x80,  /* 15/11/2005 - poor quality 0xa0 - per misura LLC 0x84*/
    0x09, 0x40,
    0x0a, 0x80, /* ITU level 15/11/2005 Cislaghi */
    0x0b, 0x44, /* ITU level 15/11/2005 Cislaghi */
    0x0c, 0x40,
    0x0d, 0x00,
    0x0e, 0x81,
    0x0f, 0x0b, /* 15/11/2005 Cislaghi */
    0x10, 0x08, /* smallest crominance bandwidth 15/11/2005 Cislaghi */
    0x11, 0x48, /* 15/11/2005 Cislaghi */
    0x12, 0x22, /* RTS0,RTS1: real-time status sync information 15/11/2005 Cislaghi */
    0x13, 0x90, /* 15/11/2005 Cislaghi */
    0x14, 0x00, /* To have AOUT  connected to AD1 for DEBUG: 0x10*/
    0x15, 0x11,
    0x16, 0xfe,
    0x17, 0xc0, /* 15/11/2005 Cislaghi */
    0x18, 0x40,
    0x19, 0x80,
    0x1a, 0x00,
    0x1b, 0x00,
    0x1c, 0x00,
    0x1d, 0x00,
    0x1e, 0x00,
 /*   0x1f, 0xb1, ? è read-only 15/11/2005 Cislaghi */
    0x30, 0xbc,
    0x31, 0xdf,
    0x32, 0x02,
    0x33, 0x00, /* Add as amadahmad@gmail.com says */
    0x34, 0xcd,
    0x35, 0xcc,
    0x36, 0x3a,
    0x37, 0x00, /* Add as amadahmad@gmail.com says */ 
    0x38, 0x03,
    0x39, 0x20,
    0x3a, 0x00,
    0x3b, 0x00, /* Add as amadahmad@gmail.com says */ 
    0x3c, 0x00, /* Add as amadahmad@gmail.com says */ 
    0x3d, 0x00, /* Add as amadahmad@gmail.com says */ 
    0x3e, 0x00, /* Add as amadahmad@gmail.com says */ 
    0x3f, 0x00, /* Add as amadahmad@gmail.com says */ 
    0x40, 0x00,
    0x41, 0xff,
    0x42, 0xff,
 /*   0x43, 0xff,
    0x44, 0xff,
    0x45, 0xff,
    0x46, 0xff,
    0x47, 0xff,
    0x48, 0xff,
    0x49, 0x77,
    0x4a, 0x77,
    0x4b, 0x77,
    0x4c, 0x77,
    0x4d, 0x77,
    0x4e, 0x77,
    0x4f, 0x77,
    0x50, 0x77,
    0x51, 0x77,
    0x52, 0x77,
    0x53, 0x77,
    0x54, 0x77,
    0x55, 0xff,
	*/
    0x43, 0x00, /* 15/11/2005 Cislaghi inizio */
    0x44, 0x00,
    0x45, 0x00,
    0x46, 0x00,
    0x47, 0x00,
    0x48, 0x00,
    0x49, 0x00,
    0x4a, 0x00,
    0x4b, 0x00,
    0x4c, 0x00,
    0x4d, 0x00,
    0x4e, 0x00,
    0x4f, 0x00,
    0x50, 0x00,
    0x51, 0x00,
    0x52, 0x00,
    0x53, 0x00,
    0x54, 0x00,
    0x55, 0x00, /* 15/11/2005 Cislaghi fine */
    0x56, 0xff,
    0x57, 0xff,
    0x58, 0x00,
    0x59, 0x47,
    0x5a, 0x07, /* sembrerebbe 0x03: 15/11/2005 Cislaghi */
    0x5b, 0x03,
    0x5c, 0x00,
    0x5d, 0x00,
    0x5e, 0x00,
    0x5f, 0x00,
/*    0x60, 0x00, ma sono read-only ?? 15/11/2005 Cislaghi 
    0x61, 0x06,
    0x62, 0x5f,
*/	
/*  0x63, 0x00, Read-only
    0x64, 0x00,
    0x65, 0x00,
    0x66, 0x00,
    0x67, 0x00,
    0x68, 0x00,
*/

    /* Global Settings */
    /* 0x80, 0x30, PCM7230 */ 
    /* 0x80, 0x34,  For F453AV */
    /* 0x80, 0x34,  TEA/TEB  enable - F453AV */
    0x80, 0x54, /* VBI e TEA  enable - 15/11/2005 Cislaghi*/
    0x83, 0x00,	 /* !!!Raf da verificare X port disable */
    /* 0x83, 0x11, ? 15/11/2005 Cislaghi X port abilitato */
    0x84, 0xa0, /* ? 15/11/2005 Cislaghi 0x40 */
    0x85, 0x10, /* ? 15/11/2005 Cislaghi 0x00 */
    0x86, 0x40, /* only video on I port :15/11/2005 Cislaghi FIFO level low */
    0x87, 0x41, /*For F453AV I port activation - IDQ inverted */
    /*    0x8f, 0x0b, ? Read-only */

    /* Task A settings -- NOT USED */
    0x90, 0x00, /* 15/11/2005 trigger forever */
    0x91, 0x08,
    0x92, 0x10, 
    0x93, 0x80, /* I port 8 bit */
    0x94, 0x0a, /* 15/11/2005 Cislaghi */
    0x95, 0x00,
    0x96, 0xd0, /* 15/11/2005 Cislaghi */
    0x97, 0x02,  /* 15/11/2005 Cislaghi */
/*    0x98, 0x11,  Skip 17 lines */
    0x98, 0x00, /* 15/11/2005 Cislaghi */
    0x99, 0x00,
#if 0 /*NTSC*/
    0x9a, 0x0e,
    0x9b, 0x00,
    0x9c, 0x00,
    0x9d, 0x06,
    0x9e, 0x0c,
    0x9f, 0x00,
#else /*PAL*/
/*    0x9a, 0x38, */
    0x9a, 0x37, /* 15/11/2005 Cislaghi */
    0x9b, 0x01,
    0x9c, 0xd0,
    0x9d, 0x02,
/*    0x9e, 0x35,  orig 0x36 */
    0x9e, 0x38, /* 15/11/2005 Cislaghi */
    0x9f, 0x01,
#endif
    0xa0, 0x01,
    0xa1, 0x00,
    0xa2, 0x00,
    0xa4, 0x80,
    0xa5, 0x40,
    0xa6, 0x40,
    0xa8, 0x00, /* 15/11/2005 Cislaghi */
    0xa9, 0x04, /* 15/11/2005 Cislaghi */
    0xaa, 0x00,
    0xac, 0x00, /* 15/11/2005 Cislaghi */
    0xad, 0x02, /* 15/11/2005 Cislaghi */
    0xae, 0x00,
    0xb0, 0x00,
    0xb1, 0x04,
    0xb2, 0x00,
    0xb3, 0x04,
    0xb4, 0x00,
    0xb8, 0x00,
    0xb9, 0x00,
    0xba, 0x00,
    0xbb, 0x00,
    0xbc, 0x00,
    0xbd, 0x00,
    0xbe, 0x00,
    0xbf, 0x00,

    /* Task B registers */
    0xc0, 0x00, /* !!!raf 15/11/2005 non verificata perchè spenta */
    0xc1, 0x08,
    0xc2, 0x10,
    0xc3, 0x80, /* I port 8 bit */
    0xc4, 0x00,
    0xc5, 0x00,
    0xc6, 0x00,
    0xc7, 0x03,
    0xc8, 0x11, /* Skip 17 lines */
    0xc9, 0x00,
#if 0 /*NTSC*/
    0xca, 0x22,
    0xcb, 0x01,
    0xcc, 0x00,
    0xcd, 0x03,
    0xce, 0x20,
    0xcf, 0x01,
#else /*PAL*/
    0xca, 0x38,
    0xcb, 0x01,
    0xcc, 0xd0,
    0xcd, 0x02,
    0xce, 0x35,
    0xcf, 0x01,
#endif
    0xd0, 0x01,
    0xd1, 0x00,
    0xd2, 0x00,
    0xd4, 0x80,
    0xd5, 0x40,
    0xd6, 0x40,
    0xd8, 0x00,
    0xd9, 0x04,
    0xda, 0x00,
    0xdc, 0x00,
    0xdd, 0x02,
    0xde, 0x00,
    0xe0, 0x00,
    0xe1, 0x04,
    0xe2, 0x00,
    0xe3, 0x04,
    0xe4, 0x00,
    0xe8, 0x00,
    0xe9, 0x00,
    0xea, 0x00,
    0xeb, 0x00,
    0xec, 0x00,
    0xed, 0x00,
    0xee, 0x00,
    0xef, 0x00,

    /* Reset Sequence */
    0x88, 0x58, /* reset analog chn 2 disabled, audio disabled */
    0x88, 0x78  /* not reset  analog chn 2 disabled, audio disabled */

};

int InitDecoder7114(int nWhichDecoder, int nVideoSys, int nTuner, int nVBI)
{
    u8 bSubAddr = 0x02;
    //    u8 bData;
    int nCounter;

    printk("InitDecoder7114\n");
    InitI2C();

    if(DetectI2C(I2CPort[nWhichDecoder])==0) {
    	DEBUG0("DetectI2C failed\n");
        return -1;
    }

    if(nVideoSys==NTSC) {
		if(nWhichDecoder==Philips7114) {
            for(nCounter=0; nCounter<P7114_Count; nCounter++) {
                if(nVBI==0) {  
                    if(SendOneByte(I2CPort[nWhichDecoder], 
                        Phil7114_NTSC[0][nCounter * 2], 
                        Phil7114_NTSC[0][nCounter * 2+1]) == 0) {
                        DEBUG0("Failed to initialize Philips 7114(1) decoder!\n");
                        return -1;
                    }
                }
                else {
                    if(SendOneByte(I2CPort[nWhichDecoder], 
                        Phil7114_NTSC[1][nCounter * 2], 
                        Phil7114_NTSC[1][nCounter * 2+1])==0) {
                        DEBUG0("Failed to initialize Philips 7114(1) decoder!\n");
                        return -1;
                    }
                }
            }
            if(nTuner==1) SendOneByte(I2CPort[nWhichDecoder], bSubAddr, 0xC0);
            DEBUG0("Initialized Philips 7114(1) to NTSC mode successfully!\n");
        }
        else if(nWhichDecoder==Philips7114_40) {
            for(nCounter=0; nCounter<P7114_Count; nCounter++) {
                if(nVBI==0) {
                    if(SendOneByte(I2CPort[nWhichDecoder], 
                        Phil7114_NTSC[0][nCounter * 2], 
                        Phil7114_NTSC[0][nCounter * 2+1])==0) {
                        DEBUG0("Failed to initialize Philips 7114(2) decoder!\n");
                        return -1;
                    }
                 
                }
                else {
                    if(SendOneByte(I2CPort[nWhichDecoder], 
                        Phil7114_NTSC[1][nCounter * 2], 
                        Phil7114_NTSC[1][nCounter * 2+1]) == 0) {
                        DEBUG0("Failed to initialize Philips 7114(2) decoder!\n");
                        return -1;
                    }
                }
            }
			WriteReg(0x3d4, 0xe1, ReadReg(0x3d4, 0xe1) | 0x0c);
            DEBUG0("Initialized Philips 7114(2) to NTSC mode successfully!\n");
        }
    }
    else {
        if(nWhichDecoder==Philips7114) {

          DEBUG0("Initializing Philips 7114(1) to PAL mode ...\n");

		  		for(nCounter=0; nCounter<P7114_Count; nCounter++) {
                if(SendOneByte(I2CPort[nWhichDecoder], Phil7114_PAL[nCounter * 2], 
                    Phil7114_PAL[nCounter * 2+1])==0) {
                    DEBUG0("Failed to initialize Philips 7114(1) decoder!\n");
                    return -1;
                }
					//
					// Read back registers to verify (R.Recalcati)
					//
					{ u8 tmp;
			  	  if (ReadOneByte(I2CPort[nWhichDecoder], Phil7114_PAL[nCounter * 2], &tmp)){
							if (tmp!=Phil7114_PAL[nCounter * 2+1]){
			          DEBUG0("Problem configuring Philips 7114(1) at byte %x : read %x, correct %x\n",Phil7114_PAL[nCounter * 2], tmp, Phil7114_PAL[nCounter * 2+1]);
				  		//return -1;
						}
//					else
//					  DEBUG0("\t\t(%x) = %x\t\tOK\n",Phil7114_PAL[nCounter * 2],tmp);
				  }
				  else{
						DEBUG0("Problem configuring Philips 7114(1) at byte %x : not read\n",Phil7114_PAL[nCounter * 2]);
				  	return -1;
				  }
					}
            }
            DEBUG0("\n");
			
            if(nVBI==1) SendOneByte(I2CPort[nWhichDecoder], 0x87, 0x01);
            if(nTuner==1) SendOneByte(I2CPort[nWhichDecoder], bSubAddr, 0xC0);
            DEBUG0("Initialized Philips 7114(1) to PAL mode successfully!\n");
        }
        else if(nWhichDecoder==Philips7114_40) {
            for(nCounter=0; nCounter<P7114_Count; nCounter++) {
                if(SendOneByte(I2CPort[nWhichDecoder], Phil7114_PAL[nCounter * 2], 
                    Phil7114_PAL[nCounter * 2+1])==0) {
                    DEBUG0("Failed to initialize Philips 7114(2) decoder!\n");
                    return -1;
                }
            }
	    WriteReg(0x3d4, 0xe1, ReadReg(0x3d4, 0xe1) | 0x0c); 
            DEBUG0("Initialized Philips 7114(2) to PAL mode successfully!\n");
        }
    }

	//
	// Alcuni test 
	//
  { u8 tmp;
      if (ReadOneByte(0x42, 0x88, &tmp))
			printk("SAA7114: Power Save Control (0x88) = %x\n",tmp);
      else
			printk("SAA7114: Power Save Control not read\n");
      tmp |= 0x01;
      SendOneByte(0x42,0x88,tmp);
      if (ReadOneByte(0x42, 0x88, &tmp))
			printk("SAA7114: Power Save Control (0x88) = %x\n",tmp);
      else
			printk("SAA7114: Power Save Control not read\n");
 
      if (ReadOneByte(0x42, 0x8f, &tmp))
			printk("SAA7114: Decoder byte test (0x8f) = %x\n",tmp);
      else
      {
			printk("SAA7114: Decoder byte test not read\n");
			printk("SAA7114: Startup may be failed\n");
      }

      if (ReadOneByte(0x42, 0x00, &tmp))
			printk("SAA7114: Chip Version (0x00) = %x\n",tmp);
      else
			printk("SAA7114: Chip Version not read\n");

      if (ReadOneByte(0x42, 0x1f, &tmp))
			printk("SAA7114: Status Byte (0x1f) = %x\n",tmp);
      else
			printk("SAA7114: Decoder Status Byte not read\n");
    }

  //
  // Necessario per avviare le task del SAA7114
  //
  printk("SAA7114 Reset\n");
  SendOneByte(0x42,0x88,0x58);
  udelay(100000);           // 0.1 sec 
  udelay(100000);           // 0.1 sec 
  udelay(100000);           // 0.1 sec  
  udelay(100000);           // 0.1 sec  
  udelay(100000);           // 0.1 sec  
  SendOneByte(0x42,0x88,0x78);
  printk("SAA7114 GOOO!!!\n");

    return 0;
}


u8 DecoderTest(u8 type, u8 index, u8 val)
{
  u8 tmp;
  int nCounter;
  int nWhichDecoder=1;
  
  if (type==1) {
	if (ReadOneByte(0x42, 0x1f, &tmp))
	  printk("Decoder Status Byte (0x1f) = %x\n",tmp);
	else
	  printk("Decoder Status Byte not read\n");
  }
  else if (type==2) {
	printk("PAL decoder Dump:\n");
	
	for(nCounter=0; nCounter<P7114_Count; nCounter++) {
	  if (ReadOneByte(I2CPort[nWhichDecoder], Phil7114_PAL[nCounter * 2], &tmp)){
		printk("\t\t(%x) = %2x\n",Phil7114_PAL[nCounter * 2],tmp);
	  } else {
		printk("\t\t(%x) = NOT READ FROM I2C\n",Phil7114_PAL[nCounter * 2]);
      }
	}
	printk("Go to the pub!\n");
  }
  else if (type==3) {
	
	if (ReadOneByte(0x42, index, &tmp))
	  printk("Read from Decoder (%x) = %x\n",index,tmp);
	else
	  printk("Decoder Register %x not read\n",index);
  }
  else if (type==4) {
    printk("Writing to Decoder (%x) = %x\n",index,val);
	
	if (SendOneByte(0x42, index, val)) {
	  if (ReadOneByte(0x42, index, &tmp))
		if (tmp==val)
		  printk("Write to Decoder (%x) = %x (re-read)\n",tmp,val);
		else
		  printk("Write to Decoder Register %x ko: correct=%x,read=%x\n",val,tmp);
	}
	else
	  printk("Decoder Register %x not read\n",index);

  }


  return tmp;
}

int ReleaseDecoder7114(int nWhichDecoder)
{
    InitI2C();

    if(DetectI2C(I2CPort[nWhichDecoder])==0) {
        return -1;
    }

    if(SendOneByte(I2CPort[nWhichDecoder], 0x83, 0x00))
    	if(SendOneByte(I2CPort[nWhichDecoder], 0x87, 0x00))
        	return 0;

    return -1;
}
