/*
 *  TVIA CyberPro 5202 utility include file
 *
 */

#ifndef _TVIA5202_MISC_H_
#define _TVIA5202_MISC_H_

#define MEMORY_START_L       0xC0
#define MEMORY_START_M       0xC1
#define MEMORY_START_H       0xC2
#define MEMORY_PITCH_L       0xC3
#define MEMORY_PITCH_H       0xC4
#define DEST_RECT_LEFT_L     0xC5
#define DEST_RECT_LEFT_H     0xC6
#define DEST_RECT_RIGHT_L    0xC7
#define DEST_RECT_RIGHT_H    0xC8
#define DEST_RECT_TOP_L      0xC9
#define DEST_RECT_TOP_H      0xCA
#define DEST_RECT_BOTTOM_L   0xCB
#define DEST_RECT_BOTTOM_H   0xCC
#define MEMORY_OFFSET_PHASE  0xCD
#define COLOR_CMP_RED        0xCE
#define COLOR_CMP_GREEN      0xCF
#define COLOR_CMP_BLUE       0xD0
#define DDA_X_INIT_L         0xD1
#define DDA_X_INIT_H         0xD2
#define DDA_X_INC_L          0xD3
#define DDA_X_INC_H          0xD4
#define DDA_Y_INIT_L         0xD5
#define DDA_Y_INIT_H         0xD6
#define DDA_Y_INC_L          0xD7
#define DDA_Y_INC_H          0xD8
#define FIFO_TIMING_CTL_L    0xD9
#define FIFO_TIMING_CTL_H    0xDA
#define VIDEO_FORMAT         0xDB
#define DISP_CTL_I           0xDC
#define FIFO_CTL_I           0xDD
#define MISC_CTL_I           0xDE
#define BOB_WEAVE_CTL        0xA7

#define VIDEO_ROM_UCB4G_HI   0xE5
#define VFAC_CTL_MODE_I      0xE8
#define VFAC_CTL_MODE_II     0xE9
#define VFAC_CTL_MODE_III    0xEA
#define CAPTURE_ADDR_L       0xEB
#define CAPTURE_ADDR_M       0xEC
#define CAPTURE_ADDR_H       0xED
#define CAPTURE_PITCH_L      0xEE
#define CAPTURE_CTL_MISC     0xEF

#define CAPTURE_TEST_CTL     0x43
#define CAPTURE_X2_CTL_I     0x49
#define CAPTURE_H_START_L    0x60
#define CAPTURE_H_START_H    0x61
#define CAPTURE_H_END_L      0x62
#define CAPTURE_H_END_H      0x63
#define CAPTURE_V_START_L    0x64
#define CAPTURE_V_START_H    0x65
#define CAPTURE_V_END_L      0x66
#define CAPTURE_V_END_H      0x67
#define CAPTURE_DDA_X_INIT_L 0x68
#define CAPTURE_DDA_X_INIT_H 0x69
#define CAPTURE_DDA_X_INC_L  0x6A
#define CAPTURE_DDA_X_INC_H  0x6B
#define CAPTURE_DDA_Y_INIT_L 0x6C
#define CAPTURE_DDA_Y_INIT_H 0x6D
#define CAPTURE_DDA_Y_INC_L  0x6E
#define CAPTURE_DDA_Y_INC_H  0x6F
#define CAPTURE_NEW_CTL_I    0x88
#define CAPTURE_NEW_CTL_II   0x89
#define CAPTURE_MODE_I       0xA4
#define CAPTURE_MODE_II      0xA5
#define CAPTURE_TV_CTL       0xAE
#define CAPTURE_BANK_CTL     0xFA

/*for source chroma key*/
#define CHROMA_CMP_RED_HI    0x8A
#define CHROMA_CMP_RED_LO    0x8B
#define CHROMA_CMP_GREEN_HI  0x8C
#define CHROMA_CMP_GREEN_LO  0x8D
#define CHROMA_CMP_BLUE_HI   0x8E
#define CHROMA_CMP_BLUE_LO   0x8F

#define YUV_OFS_LO  0xFC
#define YUV_OFS_HI  0xFD

/*for Picture in Picture*/
#define CAP_PIP_HSTART_L     0x80
#define CAP_PIP_HSTART_H     0x81
#define CAP_PIP_HEND_L       0x82
#define CAP_PIP_HEND_H       0x83
#define CAP_PIP_VSTART_L     0x84
#define CAP_PIP_VSTART_H     0x85
#define CAP_PIP_VEND_L       0x86
#define CAP_PIP_VEND_H       0x87

extern void OutTVReg(u32 dwReg, u16 wIndex);
extern void OutTVRegM(u32 dwReg, u16 wIndex, u16 wMask);
extern u16 InTVReg(u32 dwReg);
extern void OutByte(u16 wReg, u8 bIndex);
extern u8 InByte(u16 wReg);
extern void OutWord(u16 wReg, u16 wIndex);
extern void WriteReg(u16 wReg, u8 bIndex, u8 bData);
extern u8 ReadReg(u16 wReg, u8 bIndex);
extern void WriteRegM(u16 wReg, u8 bIndex, u8 bData, u8 bMask);
extern u8 In_Video_Reg(u8 bIndex);
extern void Out_Video_Reg(u8 bIndex, u8 bValue);
extern void Out_Video_Reg_M(u8 bIndex, u8 bValue, u8 bMask);
extern u8 In_SEQ_Reg(u8 bIndex);
extern void Out_SEQ_Reg(u8 bIndex, u8 bValue);
extern void Out_SEQ_Reg_M(u8 bIndex, u8 bValue, u8 bMask);
extern u8 In_CRT_Reg(u8 bIndex);
extern void Out_CRT_Reg(u8 bIndex, u8 bValue);
extern void Out_CRT_Reg_M(u8 bIndex, u8 bValue, u8 bMask);

extern u16 GetBpp(void);
#endif
