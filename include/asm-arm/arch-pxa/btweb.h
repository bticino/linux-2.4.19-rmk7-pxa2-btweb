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
	int abil_mod_hifi;
	int abil_fon;


};
#define BTWEB_NAMELEN 32
struct btweb_globals {
	int flavor;
	int hw_version;
	char name[BTWEB_NAMELEN]; /* can't be a char * because of sysctl */
	unsigned long last_touch;
	int rtc_invalid;
};

extern struct btweb_features btweb_features;
extern struct btweb_globals btweb_globals;

extern void btweb_kd_mksound(unsigned int hz, unsigned int ticks);

extern void btweb_backlight(int onoff);

#endif /* __KERNEL__ */

#define BTWEB_UNKNOWN 0
#define BTWEB_F452    1
#define BTWEB_TS      2
#define BTWEB_FPGA    3
#define BTWEB_F453AV  4
/* more flavors to come */


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
#define BTWEB_ABIL_MOD_VIDEO 14 /* video mod/demod power */
#define BTWEB_ABIL_MOD_HIFI  15 /* audio hifi mod/demod power */
#define BTWEB_ABIL_FON  16 /* "fork" power */



#endif
