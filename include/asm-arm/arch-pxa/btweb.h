/*
 *  linux/include/asm-arm/arch-pxa/btweb.h
 *  Alessandro Rubini, 2004, 2005
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * (at your choice) any later version published by
 * the Free Software Foundation.
 */



#ifndef __ASSEMBLY__

#ifdef __KERNEL__
struct btweb_features {
	int tvia_reset;
	int eth_reset;
	int eth_irq;
	int e2_wp;
	int rtc_irq;
	int led;
	int usb_irq;
	int backlight;
	int pic_reset;
	int buzzer; /* 1 or 0 */
	int mdcnfg;
	int ctrl_hifi;
	int ctrl_video;
	int virt_conf;
	int abil_mod_video;
        int abil_dem_video;
	int abil_mod_hifi;
        int abil_dem_hifi;
	int abil_fon;
        int cf_irq;
	int usb_soft_enum_n;
	int usb_pxa_slave_connected;
	int bright_i2c_addr;
	int bright_port;
	int contr_i2c_addr;
	int contr_port;
	int color_i2c_addr;
	int color_port;
	int amp_vol_i2c_addr;
	int amp_vol_port;
        int speaker_vol_i2c_addr;
        int speaker_vol_port;
        int mic_volume_i2c_addr;
        int mic_volume_port;
	int abil_amp;
        int amp_left;
        int amp_right;
        int abil_fon_ip;
        int mute_speaker;
        int abil_mic;
	int abil_tlk_i2c_addr;
	int abil_tlk_reg;
	int lighting_level_i2c_addr;
	int lighting_level_reg;

	int led_serr;
	int led_escl;
	int led_mute;
	int led_conn;
	int led_alarm;
	int led_tlc;

	int low_voltage;
	int power_sense;

	int fc_tilt;
        int fc_pan;
        int fc2_tilt;
        int fc2_pan;
        int abil_motor_ctrl;
        int MIN1A;
        int MIN1B;
        int MIN2A;
        int MIN2B;
        int MIN3A;
        int MIN3B;
        int MIN4A;
        int MIN4B;

	int rx_tx_485;

	int cammotor_pan;
	int cammotor_tilt;
	int cammotor_fc1pan;
	int cammotor_fc2pan;
	int cammotor_fc1tilt;
	int cammotor_fc2tilt;

        int penirq;

};
#define BTWEB_NAMELEN 32
struct btweb_globals {
	int flavor;
	int hw_version;
	char name[BTWEB_NAMELEN]; /* can't be a char * because of sysctl */
	unsigned long last_touch;
	int rtc_invalid;
	int usb_pxa_slave_connected;
};

extern struct btweb_features btweb_features;
extern struct btweb_globals btweb_globals;

extern void btweb_kd_mksound(unsigned int hz, unsigned int ticks);

extern void btweb_backlight(int onoff);

#endif /* __KERNEL__ */

/* All different */
#define BTWEB_UNKNOWN	-2
#define BTWEB_ANY	-1
#define BTWEB_F453	0x0
#define BTWEB_F453AV  	0x1
#define BTWEB_F453AVA   0x1 /* master type "A", prototypes */
#define BTWEB_2F	0x2
#define BTWEB_PBX288exp	0x3
#define BTWEB_PBX288	0x4
#define BTWEB_PE	0x5
#define BTWEB_PI	0x6
#define BTWEB_H4684_IP	0x7
#define BTWEB_H4684_IP_8	0x8
#define BTWEB_CDP_HW	0x9
#define BTWEB_INTERFMM	0xa
#define BTWEB_MH500	0xb
#define BTWEB_MEGATICKER	0xc

/*
 * Sysctl numbers (under /proc/sys/dev/)
 */
#define DEV_BTWEB 0x4274 /* "Bt" */
#define BTWEB_LED        1 /* led on/off */
#define BTWEB_LCD        2 /* lcd backlight - used for lcd presence */
#define BTWEB_CNTR       3 /* lcd contrast */
#define BTWEB_BUZZER     4 /* buzzer frequency and time */
#define BTWEB_BENABLE    5 /* enabled or not */
#define BTWEB_UPSIDEDOWN 6 /* turn upside-down the lcd display */
#define BTWEB_TAGO       7 /* touch-ago (seconds) - only read */
#define BTWEB_RTCINVALID 8 /* rtc is invalid */
#define BTWEB_HWNAME     9 /* string: name of device */
#define BTWEB_HWVERSION 10 /* number (4 bits) */
#define BTWEB_CTRL_HIFI 11 /* audio source from modulator or AC97 - only first master */
#define BTWEB_CTRL_VIDEO 12 /* video source from demodulator or TVIA5202 */
#define BTWEB_VIRT_CONF  13 /* virtual conf button - only input */
#define BTWEB_ABIL_MOD_VIDEO 14 /* video mod power */
#define BTWEB_ABIL_DEM_VIDEO 15 /* video demod power */
#define BTWEB_ABIL_MOD_HIFI  16 /* audio hifi mod power */
#define BTWEB_ABIL_DEM_HIFI  17 /* audio hifi demod power */
#define BTWEB_ABIL_FON       18 /* "fork" power */
#define BTWEB_CF_IRQ         19 /* compact flash interrupt */
#define BTWEB_USB_SOFT_ENUM_N 20 /* usb soft enumeration control */
#define BTWEB_USB_PXA_SLAVE_CONNECTED 21 /* usb pxa slave is connected and recognized by a master device */
#define BTWEB_BRIGHTNESS	22 /* */
#define BTWEB_COLOR		23 /* */
#define BTWEB_VOL		24 /* */
#define BTWEB_ABIL_AMP		25 /* */
/* #define FREEEEE      	26  */
#define BTWEB_AMP_LEFT		27 /* */
#define BTWEB_AMP_RIGHT		28 /* */
#define BTWEB_ABIL_FON_IP	29 /* */
#define BTWEB_MUTE_SPEAKER	30 /* */
#define BTWEB_ABIL_MIC		31 /* */
#define BTWEB_LED_SERR		32 /* */
#define BTWEB_LED_ESCL		33 /* */
#define BTWEB_LED_MUTE		34 /* */
#define BTWEB_LED_CONN		35 /* */
#define BTWEB_LED_ALARM		36 /* */
#define BTWEB_LED_TLC		37 /* */
#define BTWEB_POWER_SENSE	38 /* */
#define BTWEB_SPEAKER_VOL       39 /* */
#define BTWEB_MIC_VOL		40 /* */
#define BTWEB_ABIL_TLK          41 /* */
#define BTWEB_LIGHTING_LEVEL    42 /* */
#define BTWEB_RX_TX_485         43 /* */
#define BTWEB_CAMMOTOR_PAN      44 /* Camera pan motor, activation and direction */
#define BTWEB_CAMMOTOR_TILT     45 /* Camera tilt motor, activation and direction */
#define BTWEB_CAMMOTOR_FC1PAN   46 /* Camera fc1 pan sensor */
#define BTWEB_CAMMOTOR_FC2PAN   47 /* Camera fc2 pan sensor */
#define BTWEB_CAMMOTOR_FC1TILT  48 /* Camera fc1 tilt sensor */
#define BTWEB_CAMMOTOR_FC2TILT  49 /* Camera fc2 tilt sensor */
#endif
