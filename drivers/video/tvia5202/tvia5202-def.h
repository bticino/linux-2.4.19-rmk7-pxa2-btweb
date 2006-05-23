/*
 *  TVIA CyberPro 5202 definition
 *
 */

#ifndef _TVIA5202_DEFINE_H_
#define _TVIA5202_DEFINE_H_

#include <linux/types.h>
/*
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
*/
#define NTSC640x480 0
#define NTSC720x480 1
#define NTSC640x440 2

#define PAL640x480  0
#define PAL720x540  1
#define PAL768x576  2
#define PAL720x576  3

#define NONINTERLACED 0
#define INTERLACED    1

#define CAPENGINE1 0
#define CAPENGINE2 1

#define ENABLED    1
#define DISABLED   0

#define ON      1
#define OFF     0

#define PAL     0
#define NTSC    1

#define PORTA      0
#define PORTB      1
#define PORTC      2
#define TVVIDEO    3

#define CCIR601    0
#define CCIR656    1

#define YUV422    0 /*captured data is YUV 422 format*/
#define RGB555    1
#define RGB565    2
#define RGB888    3 
#define RGB8888   4
#define RGB8      5
#define RGB4444   6
#define RGB8T     7

#define COLORKEY  0 /*Overlayed window is of color keying*/
#define WINDOWKEY 1 /*Overlayed window is of window keying*/

#define WEAVEMODE 0
#define BOBMODE   1

#define OVERLAY1 0
#define OVERLAY2 1

/* Note: R - readable, W - Writable, R/W - read/writable */
#define PORT46E8      0x46E8 /* R   */
#define PORT102       0x102  /* R/W */
#define MISCREAD      0x3CC  /* R   */
#define MISCWRITE     0x3C2  /* W   */
#define SEQINDEX      0x3C4  /* R/W */
#define SEQDATA       0x3C5  /* R/W */
#define CRTINDEX      0x3D4  /* R/W */
#define CRTDATA       0x3D5  /* R/W */
#define ATTRRESET     0x3DA  /* R/W */
#define ATTRINDEX     0x3C0  /* R/W */
#define ATTRDATAW     0x3C0  /* W, Attrib write data port */
#define ATTRDATAR     0x3C1  /* R, Attrib read data port  */
#define GRAINDEX      0x3CE  /* R/W */
#define GRADATA       0x3CF  /* R/W */
#define RAMDACMASK    0x3C6  /* R/W, Mask register */
#define RAMDACINDEXR  0x3C7  /* R/W, RAM read index port  */
#define RAMDACINDEXW  0x3C8  /* R/W, RAM write index port */
#define RAMDACDATA    0x3C9  /* R/W, RAM Date port */
#define TVIA3CEINDEX  0x3CE  /* R/W */
#define TVIA3CFDATA   0x3CF  /* R/W */
#define TVIA3D4INDEX  0x3D4  /* R/W */
#define TVIA3D5DATA   0x3D5  /* R/W */
#define TVIA3C4INDEX  0x3C4  /* R/W */
#define TVIA3C5DATA   0x3C5  /* R/W */

#define TVIA_ROUND_UP(x, align)		(((x) + ((align) - 1)) & ~((align)-1))
#define TVIA_ROUND_DOWN(x, align)	((x) & ~((align) - 1))

/* For get new masked vale */
#define TVIA_GET_MASKED(asgn, old, mask)	(((old) & (mask)) | ((asgn) & ~(mask)))

#define TVIA_MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define TVIA_MIN(a, b)	(((a) < (b)) ? (a) : (b))

#endif
