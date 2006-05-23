#ifndef _TVIA5202_SYNCTBL_H_
#define _TVIA5202_SYNCTBL_H_

/*Following tables are special TV register settings for SyncLock mode*/
u16 wTVReg16[66];
u32 dwTVReg32[59];

u16 wNTSCTVReg16[2][66] = /*16-bit TV registers from E428 to E52C*/
{
    {
        /*CCIR601*/
        0xe428, 0x0000, 0x0000, 0x0000, 0x3000, 0xe20c, 0xe012, 0xe20c, /*e428 - e444*/
        0xe012, 0x8037, 0x4461, 0x4109, 0xe300, 0xe000, 0xe03a, 0xe400, /*e448 - e464*/
        0xe40e, 0xe41d, 0xe042, 0xe061, 0xe344, 0xe07a, 0xe1f6, 0xe209, /*e468 - e484*/
        0xe1f6, 0xe209, 0xe1fd, 0xe202, 0xe1fd, 0xe202, 0xcccc, 0x48cc, /*e488 - e4a4*/
        0xe420, 0x1020, 0xe400, 0x0000, 0x603b, 0xe001, 0xe2ab, 0xe001, /*e4a8 - e4c4*/
        0x8700, 0x8787, 0x000d, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, /*e4c8 - e4e4*/
        0xe400, 0x0004, 0x0000, 0x0002, 0x0000, 0x0000, 0x0000, 0xe400, /*e4e8 - e504*/
        0x0000, 0xe000, 0xe400, 0xe000, 0xe000, 0xe400, 0xe400, 0xe401, /*e508 - e524*/
        0xe00e, 0xe028                                                  /*e528 - e52c*/
    },
    {
        /*CCIR656*/                          
        0xe428, 0x0000, 0x0000, 0x0000, 0x3000, 0xe20c, 0xe025, 0xe20c, /*e428 - e444*/
        0xe025, 0x9040, 0x4b6b, 0x0b00, 0xdd00, 0xe000, 0xe03f, 0xe411, /*e448 - e464*/
        0xe446, 0xe427, 0xe047, 0xe069, 0xe346, 0xe07d, 0xe20c, 0xe011, /*e468 - e484*/
        0xe20c, 0xe011, 0xe005, 0xe00b, 0xe005, 0xe00b, 0xcccc, 0x48cc, /*e488 - e4a4*/
        0xe420, 0x1020, 0xe400, 0x0000, 0x6041, 0x0001, 0xe2de, 0xe001, /*e4a8 - e4c4*/
        0x8700, 0x8787, 0x000D, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, /*e4c8 - e4e4*/
        0xe400, 0x0004, 0x0000, 0x0002, 0x0000, 0x0000, 0x0000, 0xe400, /*e4e8 - e504*/
        0x0000, 0xe000, 0xe400, 0xe000, 0xe000, 0xe400, 0xe400, 0xe412, /*e508 - e524*/
        
        0x007D,
        0xe011                                                          /*e528 - e52c*/
    }     
};

u32 dwNTSCTVReg32[2][59] = /*32-bit TV registers from E530 to E618*/
{
    {
        /*CCIR601*/
        /*e530-e54c*/0x00000000, 0xf8000000, 0xf800e400, 0x00000000, 0xfa000000, 0x00000000, 0x00000000, 0x00000000, 
        /*e550-e56c*/0xf800e000, 0xf800e000, 0xb3000000, 0xb3000000, 0x00000000, 0xf85de03f, 0x00000000, 0xf800e000,
        /*e570-e58c*/0xf800e000, 0xf80fe20a, 0xf80fe20a, 0xf80fe20a, 0xf80fe20a, 0xf800e000, 0xf800e000, 0xf800e000,
        /*e590-e5ac*/0xf800e000, 0xf800e000, 0xf800e000, 0x00000000, 0xf800e000, 0xf800e000, 0x00000000, 0x00020001,
        /*e5b0-e5cc*/0x01FD0001, 0x01f701f8, 0x001001fe, 0x003b0027, 0x00010043, 0x00010002, 0x01f801fd, 0x01fe01f7,
        /*e5d0-e5ec*/0x00270010, 0x0043003b, 0xfa00e400, 0xfa00e400, 0xfa00e400, 0xfa00e400, 0xfa00e400, 0xf800e4ff,
        /*e5f0-e60c*/0x0dd0e10f, 0x012af300, 0xf800e000, 0xf800e000, 0xf800e000, 0x0000ff00, 0x00000000, 0x03000000,
        /*e610-e618*/0xfa8be610, 0xf800e400, 0xf800e400
    },
    {
        /*CCIR656*/                       
        /*e530-e54c*/0x00000000, 0xf8000000, 0xf800e400, 0x00000000, 0xfa000000, 0x00000000, 0x00000000, 0x00000000,
        /*e550-e56c*/0xf800e000, 0xf800e000, 0xb3000000, 0xb3000000, 0x00000000, 0xf85de03f, 0x00000000, 0xf800e000,
        /*e570-e58c*/0xf800e000, 0xe004e200, 0xe005e200, 0xe004e200, 0xe005e200, 0xf800e000, 0xf800e000, 0xf800e000,
        /*e590-e5ac*/0xf800e000, 0xf800e000, 0xf800e000, 0x00000000, 0xf800e000, 0xf800e000, 0x00000000, 0x00020001,
        /*e5b0-e5cc*/0x01FD0001, 0x01f701f8, 0x001001fe, 0x003b0027, 0x00010043, 0x00010002, 0x01f801fd, 0x01fe01f7,
        /*e5d0-e5ec*/0x00270010, 0x0043003b, 0xfa00e400, 0xfa00e400, 0xfa00e400, 0xfa00e400, 0xfa00e400, 0x0800e4ff,
        /*e5f0-e60c*/0x0dd0e10f, 0x012af300, 0xf800e000, 0xf800e000, 0xf800e000, 0x0000FF00, 0x00000000, 0x03000000,
        /*e610-e618*/0xfa8be610, 0xf800e400, 0xf800e400
    }        
}; 

u16 wPALTVReg16[2][66] = /*16-bit TV registers from E428 to E52C*/
{
    {
        /*CCIR601*/ 
        0xe428, 0x0000, 0x4000, 0x0000, 0x2000, 0xe245, 0xe255, 0xe245, /*e428 - e444*/ 
        0xe255, 0x703b, 0x4461, 0xf800, 0xe917, 0xe000, 0xe03f, 0xe425, /*e448 - e464*/
        0xe426, 0xe41b, 0xe04b, 0xe069, 0xe34f, 0xe08e, 0xe245, 0xe254, /*e468 - e484*/
        0xe245, 0xe254, 0xe24a, 0xe24f, 0xe24a, 0xe24f, 0xcccc, 0x48cc, /*e488 - e4a4*/
        0xe420, 0x1024, 0xe400, 0x0000, 0xe04d, 0xe001, 0xe2f7, 0xe001, /*e4a8 - e4c4*/
        0x8700, 0x8787, 0x000D, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, /*e4c8 - e4e4*/
        0xe400, 0x0004, 0x0000, 0x0002, 0x0000, 0x0000, 0x0000, 0xe400, /*e4e8 - e504*/
        0x0000, 0xe50d, 0xe400, 0xe000, 0xe000, 0xe400, 0xe400, 0xe425, /*e508 - e524*/
        0xe028, 0xe010                                                  /*e528 - e52c*/
    },                            
    {
        /*CCIR656*/ 
        0xe428, 0x0000, 0x4000, 0x0000, 0x2000, 0xe265, 0xe005, 0xe265, /*e428 - e444*/ 
        0xe005, 0x703b, 0x4461, 0xf800, 0xe917, 0xe000, 0xe03f, 0xe425, /*e448 - e464*/
        0xe426, 0xe41b, 0xe04b, 0xe069, 0xe348, 0xe08e, 0xe265, 0xe003, /*e468 - e484*/
        0xe265, 0xe003, 0xe26a, 0xe26f, 0xe26a, 0xe26f, 0xcccc, 0x48cc, /*e488 - e4a4*/
        0xe420, 0x1024, 0xe400, 0x0000, 0xe04d, 0xe001, 0xe2f7, 0xe001, /*e4a8 - e4c4*/
        0x8700, 0x8787, 0x000D, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, /*e4c8 - e4e4*/
        0xe400, 0x0004, 0x0000, 0x0002, 0x0000, 0x0000, 0x0000, 0xe400, /*e4e8 - e504*/
        0x0000, 0xe50d, 0xe400, 0xe000, 0xe000, 0xe400, 0xe400, 0xe425, /*e508 - e524*/
        0xe028, 0xe010                                                  /*e528 - e52c*/
    }
};

u32 dwPALTVReg32[2][59] = /*32-bit TV registers from E530 to E618*/
{
    {
        /*CCIR601*/  
        /*e530-e54c*/0x00000000, 0xf8000000, 0xf800e400, 0x00000000, 0xfa000000, 0x00000000, 0x00000000, 0x00000000,  
        /*e550-e56c*/0xf800e000, 0xf800e000, 0xb3000000, 0xb3000000, 0x00000000, 0xf85de03f, 0x00000000, 0xf800e000,  
        /*e570-e58c*/0xf800e000, 0xf255e243, 0xf254e242, 0xf256e244, 0xf256e243, 0xf800e000, 0xf800e000, 0xf800e000,  
        /*e590-e5ac*/0xf800e000, 0xf800e000, 0xf800e000, 0x00000000, 0xf800e000, 0xf800e000, 0x00000000, 0xfbffe401,  
        /*e5b0-e5cc*/0xfbfce5fd, 0xfa01e5fd, 0xfa17e40a, 0xfa2de424, 0xfa01e431, 0xfbfde5ff, 0xfbfde5fc, 0xfa0ae401,  
        /*e5d0-e5ec*/0xfa24e417, 0xfa31e42d, 0xfa01e401, 0xfa05e5fd, 0xfa0de5f7, 0xfa15e5ef, 0xfa1ae5e8, 0x0828e4e5,  
        /*e5f0-e60c*/0x080de150, 0x012aed0d, 0xf800e000, 0xf800e000, 0xf800e000, 0x0000FF00, 0x00000000, 0x03000000,  
        /*e610-e618*/0xfa8be610, 0xf800e400, 0xf800e400                                                                 
    },  
    {
        /*CCIR656*/  
        /*e530-e54c*/0x00000000, 0xf8000000, 0xf800e400, 0x00000000, 0xfa000000, 0x00000000, 0x00000000, 0x00000000,  
        /*e550-e56c*/0xf800e000, 0xf800e000, 0xb3000000, 0xb3000000, 0x00000000, 0xf85de03f, 0x00000000, 0xf800e000,  
        /*e570-e58c*/0xf800e000, 0xf805e262, 0xf803e262, 0xf803e264, 0xf805e264, 0xf800e000, 0xf800e000, 0xf800e000,  
        /*e590-e5ac*/0xf800e000, 0xf800e000, 0xf800e000, 0x00000000, 0xf800e000, 0xf800e000, 0x00000000, 0xfbffe401,  
        /*e5b0-e5cc*/0xfbfce5fd, 0xfa01e5fd, 0xfa17e40a, 0xfa2de424, 0xfa01e431, 0xfbfde5ff, 0xfbfde5fc, 0xfa0ae401,  
        /*e5d0-e5ec*/0xfa24e417, 0xfa31e42d, 0xfa01e401, 0xfa05e5fd, 0xfa0de5f7, 0xfa15e5ef, 0xfa1ae5e8, 0x0828e4e5,  
        /*e5f0-e60c*/0x080de150, 0x013aed0d, 0xf800e000, 0xf800e000, 0xf800e000, 0x0000FF00, 0x00000000, 0x03000000,  
        /*e610-e618*/0xfa8be610, 0xf800e400, 0xf800e400                                                               
    }
};

/* TV add-on registers index */
u16 wAddonTVindex[34] =
{
    /*green bar*/
    0xe5f6,
    /*PED_AMP*/
    0xe454,
    /*H_graphics position*/
    0xe468, 0xe46c,
    /*equalization & serration pulses*/
    0xe4b8, 0xe4bc, 0xe4c0, 0xe4c4,
    /*H_sync Adjust*/
    0xe45c, 0xe460, 0xe470, 0xe474, 0xe478, 0xe47c,
    /*V_sync adjust*/ 
    0xe480, 0xe488, 0xe490, 0xe498, 0xe494, 0xe49c, 0xe484, 0xe48c, 0xe440, 
    0xe448, 0xe43c, 0xe444,
    /*V_Burst adjust*/
    0xe574, 0xe576, 0xe578, 0xe57a, 0xe57c, 0xe57e, 0xe580, 0xe582
};

/*
  NTSC add-on TV registers value tables in the order of wAddonTVindex based on 
  bVideoFormat and wResolution. 
 */
u16 wAddonNTSCTVReg[2][3][34] =
{
    {
        /*CCIR601*/
        {
            /*640x480 NTSC*/
            0xffff, /*e5f6*/      
            0x3700, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/
            0x603f, /*e4b8*/
            0xffff, /*e4bc*/
            0xe2db, /*e4c0*/ 
            0xffff, /*e4c4*/ 
            0xffff, /*e45c*/
            0xe03f, /*e460*/  
            0xe046, /*e470*/  
            0xe067, /*e474*/  
            0xffff, /*e478*/  
            0xe07e, /*e47c*/  
            0xe1fb, /*e480*/  
            0xe1fb, /*e488*/  
            0xe201, /*e490*/ 
            0xe201, /*e498*/ 
            0xe207, /*e494*/ 
            0xe207, /*e49c*/ 
            0xe000, /*e484*/ 
            0xe000, /*e48c*/ 
            0xe013, /*e440*/ 
            0xe013, /*e448*/ 
            0xe1fb, /*e43c*/ 
            0xe1fb, /*e444*/ 
            0xe1fb, /*e574*/ 
            0x0000, /*e576*/ 
            0xe1fc, /*e578*/ 
            0x0000, /*e57a*/ 
            0xe1fb, /*e57c*/ 
            0x0000, /*e57e*/ 
            0xe1fc, /*e580*/ 
            0x0000  /*e582*/                                                                         
        },         
        {
            /*720x480 NTSC*/
            0xffff, /*e5f6*/
            0x3700, /*e454*/ 
            0xe43c, /*e468*/
            0xe44b, /*e46c*/
            0x603f, /*e4b8*/ 
            0xffff, /*e4bc*/ 
            0xe2db, /*e4c0*/ 
            0xffff, /*e4c4*/ 
            0xffff, /*e45c*/ 
            0xe03f, /*e460*/ 
            0xe046, /*e470*/ 
            0xe067, /*e474*/ 
            0xffff, /*e478*/ 
            0xe07e, /*e47c*/ 
            0xe1f9, /*e480*/   
            0xe1f9, /*e488*/  
            0xe1ff, /*e490*/  
            0xe1ff, /*e498*/  
            0xe205, /*e494*/  
            0xe205, /*e49c*/  
            0xe20b, /*e484*/  
            0xe20b, /*e48c*/  
            0xe011, /*e440*/  
            0xe011, /*e448*/  
            0xe1f9, /*e43c*/  
            0xe1f9, /*e444*/  
            0xe1f9, /*e574*/ 
            0x020b, /*e576*/ 
            0xe1fa, /*e578*/ 
            0x020b, /*e57a*/ 
            0xe1f9, /*e57c*/ 
            0x020b, /*e57e*/ 
            0xe1fa, /*e580*/ 
            0x020b  /*e582*/                           
        },
        {
            /*640x440 NTSC*/
            0xffff, /*e5f6*/      
            0x3700, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/
            0x603f, /*e4b8*/
            0xffff, /*e4bc*/
            0xe2db, /*e4c0*/ 
            0xffff, /*e4c4*/ 
            0xffff, /*e45c*/
            0xe03f, /*e460*/  
            0xe046, /*e470*/  
            0xe067, /*e474*/  
            0xffff, /*e478*/  
            0xe07e, /*e47c*/  
            0xe1fb, /*e480*/  
            0xe1fb, /*e488*/  
            0xe201, /*e490*/ 
            0xe201, /*e498*/ 
            0xe207, /*e494*/ 
            0xe207, /*e49c*/ 
            0xe000, /*e484*/ 
            0xe000, /*e48c*/ 
            0xe013, /*e440*/ 
            0xe013, /*e448*/ 
            0xe1fb, /*e43c*/ 
            0xe1fb, /*e444*/ 
            0xe1fb, /*e574*/ 
            0x0000, /*e576*/ 
            0xe1fc, /*e578*/ 
            0x0000, /*e57a*/ 
            0xe1fb, /*e57c*/ 
            0x0000, /*e57e*/ 
            0xe1fc, /*e580*/ 
            0x0000  /*e582*/
        }
    },
    {
        /*CCIR656*/
        {
            /*640x480 NTSC*/
            0xffff, /*e5f6*/
            0xffff, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/
            0x603f, /*e4b8*/
            0xffff, /*e4bc*/
            0xe2dc, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xffff, /*e478*/
            0xffff, /*e47c*/
            0xe002, /*e480*/
            0xe002, /*e488*/
            0xe008, /*e490*/
            0xe008, /*e498*/
            0xe00e, /*e494*/
            0xe00e, /*e49c*/
            0xe014, /*e484*/
            0xe014, /*e48c*/
            0xe025, /*e440*/
            0xe025, /*e448*/
            0xe20c, /*e43c*/
            0xe20c, /*e444*/
            0xe002, /*e574*/
            0xf814, /*e576*/
            0xe002, /*e578*/
            0xf814, /*e57a*/
            0xe002, /*e57c*/
            0xf814, /*e57e*/
            0xe002, /*e580*/
            0xf814  /*e582*/
        },
#if 0
        {
            /*640x480 NTSC vertical underscan*/
            0x13a, /*e5f6*/
            0xffff, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/
            0xffff, /*e4b8*/
            0xffff, /*e4bc*/
            0xffff, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xffff, /*e478*/
            0xffff, /*e47c*/
            0xe208, /*e480*/
            0xe208, /*e488*/
            0xe001, /*e490*/
            0xe001, /*e498*/
            0xe007, /*e494*/
            0xe007, /*e49c*/
            0xe00d, /*e484*/
            0xe00d, /*e48c*/
            0xe00d, /*e440*/
            0xe00d, /*e448*/
            0xe208, /*e43c*/
            0xe208, /*e444*/
            0xe208, /*e574*/
            0x000c, /*e576*/
            0xe208, /*e578*/
            0x000d, /*e57a*/
            0xe208, /*e57c*/
            0x000c, /*e57e*/
            0xe208, /*e580*/
            0x000d  /*e582*/
        },
#endif
        {
            /*720x480 NTSC*/
            0xffff, /*e5f6*/
            0xffff, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/
            0xffff, /*e4b8*/
            0xffff, /*e4bc*/
            0xffff, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xffff, /*e478*/
            0xffff, /*e47c*/
            0xe008, /*e480*/
            0xe008, /*e488*/
            0xe00e, /*e490*/
            0xe00e, /*e498*/
            0xe014, /*e494*/
            0xe014, /*e49c*/
            0xe01a, /*e484*/
            0xe01a, /*e48c*/
            0xe025, /*e440*/
            0xe025, /*e448*/
            0xe009, /*e43c*/
            0xe009, /*e444*/
            0xe008, /*e574*/
            0xf81a, /*e576*/
            0xe008, /*e578*/
            0xf81a, /*e57a*/
            0xe008, /*e57c*/
            0xf81a, /*e57e*/
            0xe008, /*e580*/
            0x0018  /*e582*/
        },
        {
            /*640x440 NTSC*/
            0xffff, /*e5f6*/
            0xffff, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/
            0x603f, /*e4b8*/
            0xffff, /*e4bc*/
            0xe2dc, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xffff, /*e478*/
            0xffff, /*e47c*/
            0xe002, /*e480*/
            0xe002, /*e488*/
            0xe008, /*e490*/
            0xe008, /*e498*/
            0xe00e, /*e494*/
            0xe00e, /*e49c*/
            0xe014, /*e484*/
            0xe014, /*e48c*/
            0xe025, /*e440*/
            0xe025, /*e448*/
            0xe20c, /*e43c*/
            0xe20c, /*e444*/
            0xe002, /*e574*/
            0xf814, /*e576*/
            0xe002, /*e578*/
            0xf814, /*e57a*/
            0xe002, /*e57c*/
            0xf814, /*e57e*/
            0xe002, /*e580*/
            0xf814  /*e582*/
        }
    }
};

/*
  PAL add-on TV registers value tables in the order of (index, value) based 
  on bVideoFormat and wResolution. 
*/
u16 wAddonPALTVReg[2][4][34] =
{
    {
        /*CCIR601*/
        {
            /*640x480 PAL*/
            0xffff, /*e5f6*/  
            0x00fa, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/  
            0x6042, /*e4b8*/ 
            0xffff, /*e4bc*/ 
            0xe2e2, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xe348, /*e478*/
            0xffff, /*e47c*/
            0xe248, /*e480*/
            0xe248, /*e488*/
            0xe24d, /*e490*/  
            0xe24d, /*e498*/  
            0xe252, /*e494*/  
            0xe252, /*e49c*/  
            0xe257, /*e484*/  
            0xe257, /*e48c*/  
            0xe258, /*e440*/  
            0xe258, /*e448*/  
            0xe248, /*e43c*/  
            0xe248, /*e444*/  
            0xe247, /*e574*/   
            0x0259, /*e576*/  
            0xe246, /*e578*/  
            0x0258, /*e57a*/  
            0xe245, /*e57c*/  
            0x0256, /*e57e*/  
            0xe247, /*e580*/  
            0x025a  /*e582*/                                                                           
        },                                                          
        {
            /*720x540 PAL*/
            0x013a, /*e5f6*/
            0x00fb, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/  
            0x6041, /*e4b8*/  
            0xffff, /*e4bc*/  
            0xe2e2, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xe341, /*e478*/
            0xffff, /*e47c*/
            0xe239, /*e480*/
            0xe239, /*e488*/
            0xe23e, /*e490*/  
            0xe23e, /*e498*/  
            0xe243, /*e494*/  
            0xe243, /*e49c*/  
            0xe248, /*e484*/  
            0xe248, /*e48c*/  
            0xe249, /*e440*/  
            0xe249, /*e448*/  
            0xe239, /*e43c*/  
            0xe239, /*e444*/  
            0xe237, /*e574*/    
            0x0249, /*e576*/   
            0xe236, /*e578*/   
            0x0248, /*e57a*/   
            0xe238, /*e57c*/   
            0x024a, /*e57e*/   
            0xe237, /*e580*/   
            0x024a  /*e582*/                                       
        },                                                                    
        {
            /*768x576 PAL*/                              
            0xffff, /*e5f6*/  
            0xffff, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/
            0x6046, /*e4b8*/
            0xffff, /*e4bc*/
            0xffff, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xe38e, /*e478*/  
            0xe092, /*e47c*/ 
            0xE25E, /*e480*/  
            0xE25C, /*e488*/  
            0xE261, /*e490*/  
            0xE261, /*e498*/  
            0xE266, /*e494*/  
            0xE266, /*e49c*/  
            0xE26B, /*e484*/  
            0xE26B, /*e48c*/  
            0xE26D, /*e440*/  
            0xE26D, /*e448*/  
            0xE25C, /*e43c*/  
            0xE25C, /*e444*/  
            0xE25B, /*e574*/    
            0x026C, /*e576*/   
            0xE25A, /*e578*/   
            0x026B, /*e57a*/   
            0xE259, /*e57c*/   
            0x026A, /*e57e*/   
            0xE25C, /*e580*/   
            0x026D  /*e582*/                            
        },
        {
            /*720x576 PAL*/
            0x013a, /*e5f6*/
            0x00fb, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/  
            0x6041, /*e4b8*/  
            0xffff, /*e4bc*/  
            0xe2e2, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xe341, /*e478*/
            0xffff, /*e47c*/
            0xe239, /*e480*/
            0xe239, /*e488*/
            0xe23e, /*e490*/  
            0xe23e, /*e498*/  
            0xe243, /*e494*/  
            0xe243, /*e49c*/  
            0xe248, /*e484*/  
            0xe248, /*e48c*/  
            0xe249, /*e440*/  
            0xe249, /*e448*/  
            0xe239, /*e43c*/  
            0xe239, /*e444*/  
            0xe237, /*e574*/    
            0x0249, /*e576*/   
            0xe236, /*e578*/   
            0x0248, /*e57a*/   
            0xe238, /*e57c*/   
            0x024a, /*e57e*/   
            0xe237, /*e580*/   
            0x024a  /*e582*/                                       
        }
    },
    {
        /*CCIR656*/
        {
            /*640x480 PAL*/                                             
            0xffff, /*e5f6*/
            0xe4fb, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/  
            0x6040, /*e4b8*/  
            0xffff, /*e4bc*/
            0xe2e3, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xe347, /*e478*/
            0xffff, /*e47c*/
            0xe25c, /*e480*/
            0xe25c, /*e488*/
            0xe261, /*e490*/
            0xe261, /*e498*/
            0xe266, /*e494*/
            0xe266, /*e49c*/
            0xe26b, /*e484*/
            0xe26b, /*e48c*/
            0xe26d, /*e440*/
            0xe26d, /*e448*/
            0xe25c, /*e43c*/
            0xe25c, /*e444*/
            0xe25a, /*e574*/
            0xe26d, /*e576*/
            0xe25a, /*e578*/
            0xe26b, /*e57a*/
            0xe259, /*e57c*/
            0xe26b, /*e57e*/
            0xe25c, /*e580*/
            0xe26d  /*e582*/              
        },                                                          
        {
            /*720x540 PAL*/
            0x013a, /*e5f6*/
            0x00fb, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/  
            0x6041, /*e4b8*/  
            0xffff, /*e4bc*/
            0xe2e2, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xe340, /*e478*/
            0xffff, /*e47c*/
            0xe24d, /*e480*/   
            0xe24d, /*e488*/   
            0xe252, /*e490*/   
            0xe252, /*e498*/   
            0xe257, /*e494*/   
            0xe257, /*e49c*/   
            0xe25c, /*e484*/   
            0xe25c, /*e48c*/   
            0xe25e, /*e440*/   
            0xe25e, /*e448*/   
            0xe24d, /*e43c*/   
            0xe24d, /*e444*/   
            0xe24a, /*e574*/ 
            0xe25c, /*e576*/ 
            0xe24a, /*e578*/ 
            0xe25c, /*e57a*/ 
            0xe24c, /*e57c*/ 
            0xe25e, /*e57e*/ 
            0xe24c, /*e580*/ 
            0xe25e  /*e582*/                                                   
        },                                                        
        {
            /*768x576 PAL*/
            0xffff, /*e5f6*/
            0xffff, /*e454*/
            0xffff, /*e468*/
            0xffff, /*e46c*/
            0x6046, /*e4b8*/
            0xffff, /*e4bc*/
            0xffff, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xe045, /*e460*/
            0xe051, /*e470*/
            0xffff, /*e474*/
            0xe38d, /*e478*/
            0xe09b, /*e47c*/
            0xe25b, /*e480*/
            0xe25b, /*e488*/
            0xe260, /*e490*/
            0xe260, /*e498*/
            0xe265, /*e494*/
            0xe265, /*e49c*/
            0xe26a, /*e484*/
            0xe26a, /*e48c*/
            0xe26b, /*e440*/
            0xe26b, /*e448*/
            0xe25a, /*e43c*/
            0xe25b, /*e444*/
            0xe25a, /*e574*/
            0x026c, /*e576*/
            0xe25a, /*e578*/
            0x026c, /*e57a*/
            0xe259, /*e57c*/
            0x026a, /*e57e*/
            0xe258, /*e580*/
            0x0269  /*e582*/                                                            
        },
        {
            /*720x576 PAL*/
            0x012a, /*e5f6*/
            0x00FC, /*e454*/
            0xE417, /*e468*/
            0xE40C, /*e46c*/  
            0x6041, /*e4b8*/  
            0xffff, /*e4bc*/
            0xe2e2, /*e4c0*/
            0xffff, /*e4c4*/
            0xffff, /*e45c*/
            0xffff, /*e460*/
            0xffff, /*e470*/
            0xffff, /*e474*/
            0xe340, /*e478*/
            0xffff, /*e47c*/
#if 0        
            0xe24d, /*e480*/   
            0xe24d, /*e488*/   
            0xe252, /*e490*/   
            0xe252, /*e498*/   
            0xe257, /*e494*/   
            0xe257, /*e49c*/   
            0xe25c, /*e484*/   
            0xe25c, /*e48c*/   
            0xe25e, /*e440*/   
            0xe25e, /*e448*/   
            0xe24d, /*e43c*/   
            0xe24d, /*e444*/   
            0xe24a, /*e574*/ 
            0xe25c, /*e576*/ 
            0xe24a, /*e578*/ 
            0xe25c, /*e57a*/ 
            0xe24c, /*e57c*/ 
            0xe25e, /*e57e*/ 
            0xe24c, /*e580*/ 
            0xe25e  /*e582*/                                                   
#else
            0xe270, /*e480*/
            0xe270, /*e488*/
            0xe004, /*e490*/
            0xe004, /*e498*/
            0xe009, /*e494*/
            0xe009, /*e49c*/
            0xe00E, /*e484*/
            0xe00E, /*e48c*/
            0xe031, /*e440*/
            0xe031, /*e448*/
            0xe270, /*e43c*/
            0xe270, /*e444*/
            0xe26f, /*e574*/
            0x0010, /*e576*/
            0xe26D, /*e578*/
            0x000f, /*e57a*/
            0xe26d, /*e57c*/
            0x000f, /*e57e*/
            0xe26F, /*e580*/
            0x000d  /*e582*/                                                            
#endif        
        }
    }
};

/*Following are CRT related registers*/
u8 bOldRegCrt[14]; /*used to contain old values*/
u8 bRegIndex[14] = /*contains 3D4/? index value*/
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09, 0x10, 0x11, 
    0x12, 0x15, 0x16
};                                                                                

/*
  NTSC mode CRT registers value tables in the order the index from 
  wResolution and bRegIndex 
 */
u8 bNTSCRegCrt[3][14] =
{
    {
        /*640x480 NTSC*/
        0x66, 0x4F, 0x4F, 0x81, 0x56, 0x82, 0x0B, 0x3E, 0x40, 0xE3, 0x88,
        0xDF, 0xE6, 0x04
    },
#if 0                               
    {
        /*640x480 NTSC vertical underscan*/                        
        0x66, 0x4F, 0x4F, 0x81, 0x52, 0x9E, 0x3B, 0x3E, 0x40, 0xFF, 0x0B,
        0xDF, 0xE0, 0x3B                                
    },
#endif    
    {
        /*720x480 NTSC*/                        
        0x66, 0x59, 0x59, 0x88, 0x60, 0x80, 0x0B, 0x3E, 0x40, 0xE2, 0x8B,
        0xDF, 0xE1, 0x0A
    },
    {
        /*640x440 NTSC*/
        0x66, 0x4F, 0x4F, 0x81, 0x56, 0x9e, 0x0B, 0x3E, 0x40, 0xC9, 0x87, 
        0xb7, 0xC2, 0x04
    }
};

/*
  PAL mode CRT registers value tables in the order the index from 
  wResolution and bRegIndex 
 */
u8 bPALRegCrt[4][14] =
{
    {
        /*640x480 PAL*/                                         
        0x6A, 0x4F, 0x4F, 0x8D, 0x59, 0x81, 0x6F, 0xBA, 0x40, 0x16, 0x86, 
        0xDF, 0xE6, 0x6F
    },                                                       
    {
        /*720x540 PAL*/                                         
        0x77, 0x59, 0x59, 0x19, 0x6C, 0x95, 0x6F, 0xF0, 0x60, 0x33, 0x8A, 
        0x1B, 0x1C, 0x6F                             
    },
    {
        /*768x576 PAL*/
        0x71, 0x5f, 0x5f, 0x93, 0x6D, 0x8B, 0x6F, 0xF0, 0x60, 0x42, 0x83,
        0x3F, 0x41, 0x6E
    },
    {
        /*720x576 PAL*/                                         
        0x6F, 0x59, 0x59, 0x8F, 0x64, 0x80, 0x6F, 0xF0, 0x60, 0x45, 0x8C,
        0x3F, 0x3F, 0x6F
    }
};

/*
  NTSC mode synclock registers value tables in the order the index from 
  bVideoFormat and wResolution. These values will be used to set 3d4/28,29,
  2A,2B accordingly.
 */
u8 bNTSCRegSync[2][3][4] = 
{
    {
        /*CCIR601*/
        {
            /*640x480 NTSC*/                                             
            0x5c, 0xe0, 0xea, 0x81                           
        },                                                          
        {
            /*720x480 NTSC*/                              
            0x54, 0xe0, 0xed, 0x81                          
        },
        {
            /*640x440 NTSC*/                                             
            0x5c, 0xe0, 0xea, 0x81
        }
    },
    {
        /*CCIR656*/
        {
            /*640x480 NTSC*/
            0x51, 0xe0, 0xE8, 0x81
        },
#if 0                                                            
        {
            /*640x480 NTSC vertical underscan*/                          
            0x51, 0xe0, 0xfc, 0x81                                
        },  
#endif    
        {
            /*720x480 NTSC*/                              
            0x5C, 0xe0, 0xEC, 0x81                          
        },
        {
            /*640x440 NTSC*/
            0x5C, 0xe0, 0xce, 0x81
        }
    }
};

/*
  PAL mode synclock registers value tables in the order the index from 
  bVideoFormat and wResolution. These values will be used to set 3d4/28,29,
  2A,2B accordingly.
 */
u8 bPALRegSync[2][4][4] =
{
    {
        /*CCIR601*/
        {
            /*640x480 PAL*/                                           
            0x56, 0x90, 0x07, 0x82
        },                                                       
        {
            /*720x540 PAL*/                                         
            0x63, 0x90, 0x1D, 0x82
        },
        {
            /*768x576 PAL*/
            0x5D, 0x90, 0x31, 0x82
        },
        {
            /*720x576 PAL*/
            0x63, 0x90, 0x1D, 0x82
        }
    },
    {
        /*CCIR656*/
        {
            /*640x480 PAL*/
            0x57, 0x90, 0x19, 0x82
        },                                                       
        {
            /*720x540 PAL*/
            0x6B, 0x90, 0x1F, 0x82
        },
        {
            /*768x576 PAL*/
            0x65, 0x90, 0x30, 0x82 
        },
        {
            /*720x576 PAL*/
            0x63, 0x90, 0x41, 0x82
        }
    }                                                      
};

u8 bRegExtSyncIndex[13] = /*contains extended synclock register index 
                             value(3c4/??)*/
{
    0xe0, 0xe1, 0xe2, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xec,  
    0xee, 0xef
};     

/*
  NTSC mode extended synclock registers value tables in the order the index 
  from bVideoFormat and wResolution
 */
u8 bNTSCRegExtSync[2][3][13] =
{
    {
        /*CCIR601*/
        {
            /*640x480 NTSC*/                                             
            0x02, 0x60, 0xB0, 0x50, 0x03, 0x00, 0x94, 0x88, 0x00, 0x13, 0x00,
            0x01, 0x33          
        },                                          
        {
            /*720x480 NTSC*/                              
            0x02, 0x60, 0xB0, 0x50, 0x03, 0x00, 0x0A, 0x88, 0x00, 0x13, 0x00,
            0x09, 0x13
        },
        {
            /*640x440 NTSC*/                                             
            0x02, 0x60, 0xB0, 0x50, 0x03, 0x00, 0x94, 0x88, 0x00, 0x13, 0x00,
            0x01, 0x33
        }
    },
    {
        /*CCIR656*/
        {
            /*640x480 NTSC*/                                             
            0x03, 0x60, 0xD3, 0x50, 0x03, 0x00, 0x13, 0x61, 0xD3, 0x13, 0x00,
            0x00, 0x3B
        },  
#if 0                                                    
        {
            /*640x480 NTSC vertical underscan*/                          
            0x03, 0x60, 0xB0, 0x50, 0x03, 0x00, 0xef, 0x60, 0xB0, 0x13, 0x00,
            0x00, 0x3B
        },
#endif    
        {
            /*720x480 NTSC*/                              
            0x03, 0x60, 0xD3, 0x50, 0x03, 0x00, 0xBC, 0x60, 0xD3, 0x5D, 0x00,
            0x00, 0x3B
        },
        {
            /*640x440 NTSC*/
            0x03, 0x60, 0xD3, 0x50, 0x03, 0x00, 0xef, 0x60, 0xD3, 0x2f, 0x00,
            0x00, 0x3B
        }
    }
};

/*
  PAL mode extended synclock registers value tables in the order the index 
  from bVideoFormat and wResolution
 */
u8 bPALRegExtSync[2][4][13] =
{
    {
        /*CCIR601*/
        {
            /*640x480 PAL*/                                           
            0x02, 0x60, 0xA0, 0x56, 0x03, 0x00, 0x9A, 0x60, 0x24, 0x7F, 0x00,
            0x81, 0x33
        },                                                   
        {
            /*720x540 PAL*/                                         
            0x02, 0x60, 0xA0, 0x56, 0x03, 0x00, 0xAA, 0x60, 0x24, 0x7F, 0x00,
            0x81, 0x33
        },
        {
            /*768x576 PAL*/                                         
            0x02, 0x70, 0xFF, 0xA6, 0x03, 0x06, 0x11, 0x70, 0xFF, 0x00, 0x00,
            0x89, 0x03
        },
        {
            /*720x576 PAL*/
            0x02, 0x60, 0xA0, 0x56, 0x03, 0x00, 0xAA, 0x60, 0x24, 0x7F, 0x00,
            0x81, 0x33
        }
    },
    {
        /*CCIR656*/
        {
            /*640x480 PAL*/                                                                     
            0x03, 0x60, 0xA0, 0x56, 0x03, 0x06, 0x8F, 0x60, 0x20, 0x7F, 0x00,
            0x00, 0x3B
        },                                                   
        {
            /*720x540 PAL*/                                          
            0x03, 0x60, 0xA0, 0x56, 0x03, 0x06, 0xAA, 0x60, 0x20, 0x7F, 0x00,
            0x00, 0x3B
        },                                                   
        {
            /*768x576 PAL*/
            0x03, 0x70, 0xFF, 0xA6, 0x03, 0x06, 0x9F, 0x70, 0xFF, 0xC2, 0x00,
            0x08, 0x3B
        },
        {
            /*720x576 PAL*/                                          
            0x03, 0x60, 0xA0, 0x56, 0x03, 0x06, 0xAA, 0x60, 0x20, 0x6B, 0x00,
            0x00, 0x3B
        }
    }                                                      
};

#endif
