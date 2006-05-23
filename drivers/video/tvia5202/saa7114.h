/*
 *   Philips 7114 video decoder include file
 */

#ifndef _SAA7114_H_
#define _SAA7114_H_

#define Philips7111     0
#define Philips7114     1
#define Philips7113     2
#define Philips7114_40  3

extern int InitDecoder7114(int nWhichDecoder, int nVideoSys, int nTuner, int nVBI);
extern int ReleaseDecoder7114(int nWhichDecoder);
extern u8 DecoderTest(u8 type, u8 index, u8 val);
#endif
