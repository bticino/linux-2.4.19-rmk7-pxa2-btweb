/*
 * Alessandro Rubini, 2005. GPL 2 or later.
 * Parts (i2c) by Simon G. Vogl, Frodo Looijaard. GPL 2 or later.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/sysctl.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <asm/hardware.h>

#define trace(format, arg...) printk(KERN_INFO __FILE__ ": " format "\n" , ## arg)

/*
 * The buzzer is always there; the pointer is set in btweb.c::btweb_init.
 * The structure of this code comes from drivers/char/vt.c
 */
static int btsys_buz[2];
static int btsys_benable = 1; /* enabled by default */
static int btsys_upsidedown;
static int btsys_tago;

static void
btweb_kd_nosound(unsigned long ignored)
{
	/* turn the gpio to input mode */
	set_GPIO_mode( 16 | GPIO_IN);
	/* mark that everything it quiet */
	btsys_buz[0] = 0;
}

void
btweb_kd_mksound(unsigned int hz, unsigned int ticks)
{
	static struct timer_list sound_timer = { function: btweb_kd_nosound };
	unsigned int count = 0;


	/* 3.6864 MHz / prescaler; prescaler must be 64 so we can go low */
	PWM_CTRL0 = 63;

	if (hz > 20 && hz < 32767)
		count = ((3686400+32)/64) /hz;
	if (count > 1023) {count = 1023;}
	del_timer(&sound_timer);

	if (!btsys_benable)
		count=0;

	if (count) {
		PWM_PERVAL0 = count;
		PWM_PWDUTY0 = count/2;

		/* set as PWM output */
		set_GPIO_mode( GPIO16_PWM0_MD );
		if (ticks) {
			sound_timer.expires = jiffies+ticks;
			add_timer(&sound_timer);
		}
	} else
		btweb_kd_nosound(0);
}

#if 0 
static int btsys_lcd = 1;
void btweb_backlight(int onoff)
{
	btsys_lcd = onoff;
	if (btsys_lcd)
		GPSR(btweb_features.backlight) =
			GPIO_bit(btweb_features.backlight);
	else
		GPCR(btweb_features.backlight) =
			GPIO_bit(btweb_features.backlight);
}
#endif


#ifdef CONFIG_SYSCTL

static int btsys_led = 0;
static int btsys_cntr = 255;
static int btsys_bright = 255;
static int btsys_color = 255;
static int btsys_vol = 255;
static int btsys_ctrl_hifi = 0;
static int btsys_ctrl_video = 0;
static int btsys_virt_conf = 0;
static int btsys_abil_mod_video = 0;
static int btsys_abil_dem_video = 0;
static int btsys_abil_mod_hifi = 0;
static int btsys_abil_dem_hifi = 0;
static int btsys_abil_fon = 0;
static int btsys_abil_amp = 0;
static int btsys_amp_left = 0;
static int btsys_amp_right = 0;
static int btsys_abil_fon_ip = 0;
static int btsys_mute_speaker = 0;
static int btsys_abil_mic = 0;
static int btsys_led_serr = 0;
static int btsys_led_escl = 0;
static int btsys_led_mute = 0;
static int btsys_led_conn = 0;
static int btsys_led_alarm = 0;
static int btsys_led_tlc = 0;
static int btsys_power_sense = 0;
static int btsys_lcd = 0;
static int btsys_speaker_vol = 0;
static int btsys_mic_vol = 0;
static int btsys_abil_tlk = 0;
static int btsys_lighting_level = 0;
#ifdef USE_RX_TX_485_BTSYS
static int btsys_rx_tx_485 = 0;
#endif

static int bool_min[] = {0};
static int bool_max[] = {1};
static int byte_min[] = {0};
static int byte_max[] = {255};

#ifdef CONFIG_I2C
/* This part copied by i2c-dev.c (Simon G. Vogl, Frodo Looijaard) */

int btsys_detach_client(struct i2c_client *client)
{
        return 0;
}

int btsys_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
        return -1;
}

static struct i2c_client btsys_client;

int btsys_attach_adapter(struct i2c_adapter *adap)
{
	if (btsys_client.adapter) {
		printk("btweb-sysctl: supporting only one I2C adapter\n");
		return -EBUSY;
	}
	btsys_client.adapter = adap;
	return 0;
}

static struct i2c_driver btsys_driver = {
        name:           "btweb-sysctl dummy driver",
        id:             I2C_DRIVERID_I2CDEV, /* Hmm... */
        flags:          I2C_DF_DUMMY,
        attach_adapter: btsys_attach_adapter,
        detach_client:  btsys_detach_client,
        command:        btsys_command,
};

static struct i2c_client btsys_client = {
        name:           "btweb-sysctl entry",
        id:             1,
        flags:          0,
        addr:           -1,
        driver:         &btsys_driver,
};

static int btsys_i2c_do(int addr, int reg, int val)
{
	static int registered;
	char data[2] = {reg, val};

	if ((addr<0)||(reg<0))
		return -EOPNOTSUPP;

	if (!registered) {
		/*
		 * We would like to add this driver at init time, but we
		 * need a process context as it sleeps on a semaphore.
		 */
		int res;
		if ((res = i2c_add_driver(&btsys_driver))) {
			printk("btsys: can't register i2c driver\n");
			return res;
		}
		registered++;
	}
	btsys_client.addr = addr;
	if (i2c_master_send(&btsys_client, data, 2) == 2)
		return 0;
	return -EIO;
}

static int btsys_i2c_read(int addr, int reg, char * val)
{
        static int registered;
	unsigned char ret;

        if ((addr<0)||(reg<0))
                return -EOPNOTSUPP;

        if (!registered) {
                /*
                 * We would like to add this driver at init time, but we
                 * need a process context as it sleeps on a semaphore.
                 */
                int res;
                if ((res = i2c_add_driver(&btsys_driver))) {
                        printk("btsys: can't register i2c driver\n");
                        return res;
                }
                registered++;
        }

        btsys_client.addr = addr;
        if (i2c_master_send(&btsys_client, (char *)&reg, 1) == 1){
		ret = i2c_master_recv(&btsys_client, val, 1);
		if (ret==1) 
			return 0;
	}
        
        return -EIO;
}

#else /* !I2C */
static int btsys_i2c_do(int addr, int reg, int val)
{
	return 0;
} 
#endif /* I2C */

static int btsys_apply(int name)
{
	switch(name) {
		case BTWEB_LED:
			if (btweb_features.led < 0)
				return -EOPNOTSUPP;
			if (!btsys_led)
				GPSR(btweb_features.led) =
					GPIO_bit(btweb_features.led);
			else
                                GPCR(btweb_features.led) =
                                        GPIO_bit(btweb_features.led);
			break;
 		case BTWEB_LCD:
			if (btweb_features.backlight < 0)
				return -EOPNOTSUPP;
			printk("btsys_lcd=%d\n",btsys_lcd); 
		        if (btsys_lcd)
		                GPSR(btweb_features.backlight) =
		                        GPIO_bit(btweb_features.backlight);
		        else
		                GPCR(btweb_features.backlight) =
		                        GPIO_bit(btweb_features.backlight);
/*			btweb_backlight(btsys_lcd); */
			break;
		case BTWEB_CNTR:
                        if (btweb_features.backlight < 0)
                                return -EOPNOTSUPP;
			return btsys_i2c_do(btweb_features.contr_i2c_addr, btweb_features.contr_port, btsys_cntr);
			break;
		case BTWEB_BUZZER:
            if (!btweb_features.buzzer)
                                return -EOPNOTSUPP;
			btweb_kd_mksound(btsys_buz[0], btsys_buz[1]);
			btsys_buz[1] = 0; /* to be safe next time */
			break;
		case BTWEB_BENABLE:
                        if (!btweb_features.buzzer)
                                return -EOPNOTSUPP;
			if (!btsys_benable)
				btweb_kd_nosound(0);
			break;
		case BTWEB_UPSIDEDOWN:
			if (!btweb_features.backlight)
				return -EOPNOTSUPP;
			if (btsys_upsidedown)
				GPSR(54) = GPIO_bit(54);
			else
				GPCR(54) = GPIO_bit(54);
			break;
		case BTWEB_TAGO:
			break;
		case BTWEB_CTRL_HIFI:
			if (btweb_features.ctrl_hifi < 0)
				return -EOPNOTSUPP;
			if (btsys_ctrl_hifi){
				GPSR(btweb_features.ctrl_hifi) =
					GPIO_bit(btweb_features.ctrl_hifi);
			}
			else{
				GPCR(btweb_features.ctrl_hifi) =
					GPIO_bit(btweb_features.ctrl_hifi);
			}
		break;
		case BTWEB_CTRL_VIDEO:
			if (btweb_features.ctrl_video < 0)
				return -EOPNOTSUPP;
			if (btsys_ctrl_video){
				GPSR(btweb_features.ctrl_video) =
					GPIO_bit(btweb_features.ctrl_video);
			}
			else{
				GPCR(btweb_features.ctrl_video) =
					GPIO_bit(btweb_features.ctrl_video);
			}
		break;
		case BTWEB_VIRT_CONF:
			break;
		break;
		case BTWEB_ABIL_MOD_VIDEO:
			if (btweb_features.abil_mod_video < 0)
				return -EOPNOTSUPP;
			if (btsys_abil_mod_video){
				GPSR(btweb_features.abil_mod_video) =
					GPIO_bit(btweb_features.abil_mod_video);
			}
			else{
				GPCR(btweb_features.abil_mod_video) =
					GPIO_bit(btweb_features.abil_mod_video);
			}
		break;
               case BTWEB_ABIL_DEM_VIDEO:
                        if (btweb_features.abil_dem_video < 0)
                                return -EOPNOTSUPP;
                        if (btsys_abil_dem_video){
                                GPSR(btweb_features.abil_dem_video) =
                                        GPIO_bit(btweb_features.abil_dem_video);
                        }
                        else{
                                GPCR(btweb_features.abil_dem_video) =
                                        GPIO_bit(btweb_features.abil_dem_video);
                        }
                break;
		case BTWEB_ABIL_MOD_HIFI:
			if (btweb_features.abil_mod_hifi < 0)
				return -EOPNOTSUPP;
			if (btsys_abil_mod_hifi){
				GPSR(btweb_features.abil_mod_hifi) =
					GPIO_bit(btweb_features.abil_mod_hifi);
			}
			else{
				GPCR(btweb_features.abil_mod_hifi) =
					GPIO_bit(btweb_features.abil_mod_hifi);
			}
		break;
                case BTWEB_ABIL_DEM_HIFI:
                        if (btweb_features.abil_dem_hifi < 0)
                                return -EOPNOTSUPP;
                        if (btsys_abil_dem_hifi){
                                GPSR(btweb_features.abil_dem_hifi) =
                                        GPIO_bit(btweb_features.abil_dem_hifi);
                        }
                        else{
                                GPCR(btweb_features.abil_dem_hifi) =
                                        GPIO_bit(btweb_features.abil_dem_hifi);
                        }
                break;
		case BTWEB_ABIL_FON:
			if (btweb_features.abil_fon < 0)
				return -EOPNOTSUPP;
			if (btsys_abil_fon){
				GPSR(btweb_features.abil_fon) =
					GPIO_bit(btweb_features.abil_fon);
			}
			else{
				GPCR(btweb_features.abil_fon) =
					GPIO_bit(btweb_features.abil_fon);
			}
		break;
		case BTWEB_BRIGHTNESS:
                        if (btweb_features.bright_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        return btsys_i2c_do(btweb_features.bright_i2c_addr, btweb_features.bright_port, btsys_bright);
                break;
                case BTWEB_COLOR:
                        if (btweb_features.color_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        return btsys_i2c_do(btweb_features.color_i2c_addr, btweb_features.color_port, btsys_color);
                break;
                case BTWEB_VOL:
			if (btweb_features.amp_vol_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        return btsys_i2c_do(btweb_features.amp_vol_i2c_addr, btweb_features.amp_vol_port, btsys_vol);
                break;
                case BTWEB_MIC_VOL:
                        if (btweb_features.mic_vol_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        return btsys_i2c_do(btweb_features.mic_vol_i2c_addr, btweb_features.mic_vol_port, btsys_mic_vol);
                break;
                case BTWEB_SPEAKER_VOL:
                        if (btweb_features.speaker_vol_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        return btsys_i2c_do(btweb_features.speaker_vol_i2c_addr, btweb_features.speaker_vol_port, btsys_speaker_vol);
                break;
		case BTWEB_ABIL_AMP:
			if (btweb_features.abil_amp< 0)
                               return -EOPNOTSUPP;
                        if (btsys_abil_amp){
                                GPSR(btweb_features.abil_amp) =
                                        GPIO_bit(btweb_features.abil_amp);
                        }
                        else{
                                GPCR(btweb_features.abil_amp) =
                                        GPIO_bit(btweb_features.abil_amp);
                        }
                break;
		case BTWEB_AMP_LEFT:
                        if (btweb_features.amp_left< 0)
                               return -EOPNOTSUPP;
                        if (btsys_amp_left){
                                GPSR(btweb_features.amp_left) =
                                        GPIO_bit(btweb_features.amp_left);
                        }
                        else{
                                GPCR(btweb_features.amp_left) =
                                        GPIO_bit(btweb_features.amp_left);
                        }
                break;
		case BTWEB_AMP_RIGHT:
                        if (btweb_features.amp_right< 0)
                               return -EOPNOTSUPP;
                        if (btsys_amp_right){
                                GPSR(btweb_features.amp_right) =
                                        GPIO_bit(btweb_features.amp_right);
                        }
                        else{
                                GPCR(btweb_features.amp_right) =
                                        GPIO_bit(btweb_features.amp_right);
                        }
                break;
		case BTWEB_ABIL_FON_IP:
                        if (btweb_features.abil_fon_ip< 0)
                               return -EOPNOTSUPP;
                        if (btsys_abil_fon_ip){
                                GPSR(btweb_features.abil_fon_ip) =
                                        GPIO_bit(btweb_features.abil_fon_ip);
                        }
                        else{
                                GPCR(btweb_features.abil_fon_ip) =
                                        GPIO_bit(btweb_features.abil_fon_ip);
                        }
                break;
		case BTWEB_MUTE_SPEAKER:
                        if (btweb_features.mute_speaker< 0)
                               return -EOPNOTSUPP;
                        if (btsys_mute_speaker){
                                GPSR(btweb_features.mute_speaker) =
                                        GPIO_bit(btweb_features.mute_speaker);
                        }
                        else{
                                GPCR(btweb_features.mute_speaker) =
                                        GPIO_bit(btweb_features.mute_speaker);
                        }
                break;
		case BTWEB_ABIL_MIC:
                        if (btweb_features.abil_mic< 0)
                               return -EOPNOTSUPP;
                        if (btsys_abil_mic){
                                GPSR(btweb_features.abil_mic) =
                                        GPIO_bit(btweb_features.abil_mic);
                        }
                        else{
                                GPCR(btweb_features.abil_mic) =
                                        GPIO_bit(btweb_features.abil_mic);
                        }
                break;
		case BTWEB_LED_SERR:
                        if (btweb_features.led_serr< 0)
                               return -EOPNOTSUPP;
                        if (btsys_led_serr){
                                GPSR(btweb_features.led_serr) =
                                        GPIO_bit(btweb_features.led_serr);
                        }
                        else{
                                GPCR(btweb_features.led_serr) =
                                        GPIO_bit(btweb_features.led_serr);
                        }
                break;
		case BTWEB_LED_ESCL:
                        if (btweb_features.led_escl< 0)
                               return -EOPNOTSUPP;
                        if (btsys_led_escl){
                                GPSR(btweb_features.led_escl) =
                                        GPIO_bit(btweb_features.led_escl);
                        }
                        else{
                                GPCR(btweb_features.led_escl) =
                                        GPIO_bit(btweb_features.led_escl);
                        }
                break;
		case BTWEB_LED_MUTE:
                        if (btweb_features.led_mute< 0)
                               return -EOPNOTSUPP;
                        if (btsys_led_mute){
                                GPSR(btweb_features.led_mute) =
                                        GPIO_bit(btweb_features.led_mute);
                        }
                        else{
                                GPCR(btweb_features.led_mute) =
                                        GPIO_bit(btweb_features.led_mute);
                        }
	        break;
		case BTWEB_LED_CONN:
                        if (btweb_features.led_conn< 0)
                               return -EOPNOTSUPP;
                        if (btsys_led_conn){
                                GPSR(btweb_features.led_conn) =
                                        GPIO_bit(btweb_features.led_conn);
                        }
                        else{
                                GPCR(btweb_features.led_conn) =
                                        GPIO_bit(btweb_features.led_conn);
                        }
                break;
		case BTWEB_LED_ALARM:
                        if (btweb_features.led_alarm< 0)
                               return -EOPNOTSUPP;
                        if (btsys_led_alarm){
                                GPSR(btweb_features.led_alarm) =
                                        GPIO_bit(btweb_features.led_alarm);
                        }
                        else{
                                GPCR(btweb_features.led_alarm) =
                                        GPIO_bit(btweb_features.led_alarm);
                        }
                break;
		case BTWEB_LED_TLC:
                        if (btweb_features.led_tlc< 0)
                               return -EOPNOTSUPP;
                        if (btsys_led_tlc){
                                GPSR(btweb_features.led_tlc) =
                                        GPIO_bit(btweb_features.led_tlc);
                        }
                        else{
                                GPCR(btweb_features.led_tlc) =
                                        GPIO_bit(btweb_features.led_tlc);
                        }
                break;
		case BTWEB_POWER_SENSE:
                break;
		case BTWEB_ABIL_TLK:
			if (btweb_features.abil_tlk_i2c_addr< 0)
                               return -EOPNOTSUPP;
			if (btsys_abil_tlk)
				return btsys_i2c_do(btweb_features.abil_tlk_i2c_addr, btweb_features.abil_tlk_reg, 0x03);
			else
				return btsys_i2c_do(btweb_features.abil_tlk_i2c_addr, btweb_features.abil_tlk_reg, 0x0F);
		break;
		case BTWEB_LIGHTING_LEVEL:
                break;
#ifdef USE_RX_TX_485_BTSYS
		case BTWEB_RX_TX_485:
                        if (btweb_features.rx_tx_485< 0)
                               return -EOPNOTSUPP;
                        if (btsys_rx_tx_485){
                                GPSR(btweb_features.rx_tx_485) =
                                        GPIO_bit(btweb_features.rx_tx_485);
                        }
                        else{
                                GPCR(btweb_features.rx_tx_485) =
                                        GPIO_bit(btweb_features.rx_tx_485);
                        }
                break;
#endif
		}
	return 0;
}

static int btsys_read(int name)
{
	char ret;

	switch(name) {
		case BTWEB_LED:
			if (btweb_features.led < 0)
				return -EOPNOTSUPP;
                        btsys_led=((GPLR(btweb_features.led)&GPIO_bit(btweb_features.led))!=0);
			return 0;
			break;
 		case BTWEB_LCD:
			if (btweb_features.backlight < 0)
				return -EOPNOTSUPP;
			btsys_lcd=((GPLR(btweb_features.backlight)&GPIO_bit(btweb_features.backlight))!=0);
			return 0;
			break;
		case BTWEB_CNTR:
                        if (btweb_features.backlight < 0)
                                return -EOPNOTSUPP;
			btsys_i2c_read(btweb_features.contr_i2c_addr, btweb_features.contr_port,&ret);
			btsys_cntr=(int)ret;
			return 0;
			break;
		case BTWEB_BUZZER:
			return 0;
			break;
		case BTWEB_BENABLE:
/*                        if (!btweb_features.buzzer)
                                return -EOPNOTSUPP;
			btsys_benable=((GPLR(btweb_features.buzzer)&GPIO_bit(btweb_features.buzzer))!=0);*/
			return 0;
			break;
		case BTWEB_UPSIDEDOWN:
/*			if (!btweb_features.backlight)
				return -EOPNOTSUPP;
			return btsys_upsidedown=GPLR(btweb_features.backlight);
*/
			break;
		case BTWEB_TAGO:
			break;
		case BTWEB_CTRL_HIFI:
			if (btweb_features.ctrl_hifi < 0)
				return -EOPNOTSUPP;
			btsys_ctrl_hifi=((GPLR(btweb_features.ctrl_hifi)&GPIO_bit(btweb_features.ctrl_hifi))!=0);
			return 0;
		break;
		case BTWEB_CTRL_VIDEO:
			if (btweb_features.ctrl_video < 0)
				return -EOPNOTSUPP;
			btsys_ctrl_video=((GPLR(btweb_features.ctrl_video)&GPIO_bit(btweb_features.ctrl_video))!=0);
			return 0;
		break;
		case BTWEB_VIRT_CONF:
			break;
		break;
		case BTWEB_ABIL_MOD_VIDEO:
			if (btweb_features.abil_mod_video < 0)
				return -EOPNOTSUPP;
			btsys_abil_mod_video=((GPLR(btweb_features.abil_mod_video)&GPIO_bit(btweb_features.abil_mod_video))!=0);
			return 0;
		break;
               case BTWEB_ABIL_DEM_VIDEO:
                        if (btweb_features.abil_dem_video < 0)
                                return -EOPNOTSUPP;
			btsys_abil_dem_video=((GPLR(btweb_features.abil_dem_video)&GPIO_bit(btweb_features.abil_dem_video))!=0);
                        return 0;
                break;
		case BTWEB_ABIL_MOD_HIFI:
			if (btweb_features.abil_mod_hifi < 0)
				return -EOPNOTSUPP;
			btsys_abil_mod_hifi=((GPLR(btweb_features.abil_mod_hifi)&GPIO_bit(btweb_features.abil_mod_hifi))!=0);
			return 0;
		break;
                case BTWEB_ABIL_DEM_HIFI:
                        if (btweb_features.abil_dem_hifi < 0)
                                return -EOPNOTSUPP;
			btsys_abil_dem_hifi=((GPLR(btweb_features.abil_dem_hifi)&GPIO_bit(btweb_features.abil_dem_hifi))!=0);
                        return 0;
                break;
		case BTWEB_ABIL_FON:
			if (btweb_features.abil_fon < 0)
				return -EOPNOTSUPP;
			btsys_abil_fon=((GPLR(btweb_features.abil_fon)&GPIO_bit(btweb_features.abil_fon))!=0);
			return 0;
		break;
		case BTWEB_BRIGHTNESS:
                        if (btweb_features.bright_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        btsys_i2c_read(btweb_features.bright_i2c_addr, btweb_features.bright_port, &ret);
			btsys_bright=(int)ret;
			return 0;	
                break;
                case BTWEB_COLOR:
                        if (btweb_features.color_i2c_addr < 0)
                                return -EOPNOTSUPP;
			btsys_i2c_read(btweb_features.color_i2c_addr, btweb_features.color_port, &ret);
			btsys_color=(int)ret;
			return 0;
                break;
                case BTWEB_VOL:
			if (btweb_features.amp_vol_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        btsys_i2c_read(btweb_features.amp_vol_i2c_addr, btweb_features.amp_vol_port,&ret); 
			btsys_cntr=(int)ret;
			return 0;
                break;

                case BTWEB_MIC_VOL:
                        if (btweb_features.mic_vol_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        btsys_i2c_read(btweb_features.mic_vol_i2c_addr, btweb_features.mic_vol_port, &ret);
			btsys_mic_vol=(int)ret;	
			return 0;
                break;
                case BTWEB_SPEAKER_VOL:
                        if (btweb_features.speaker_vol_i2c_addr < 0)
                                return -EOPNOTSUPP;
                        btsys_i2c_read(btweb_features.speaker_vol_i2c_addr, btweb_features.speaker_vol_port, &ret);
			btsys_speaker_vol=(int)ret;
			return 0;
                break;
		case BTWEB_ABIL_AMP:
			if (btweb_features.abil_amp< 0)
                               return -EOPNOTSUPP;
			btsys_abil_amp=((GPLR(btweb_features.abil_amp)&GPIO_bit(btweb_features.abil_amp))!=0);	
                        return 0;
                break;
		case BTWEB_AMP_LEFT:
                        if (btweb_features.amp_left< 0)
                               return -EOPNOTSUPP;
			btsys_amp_left	=((GPLR(btweb_features.amp_left)&GPIO_bit(btweb_features.amp_left))!=0);
                	return 0;
                break;
		case BTWEB_AMP_RIGHT:
                        if (btweb_features.amp_right< 0)
                               return -EOPNOTSUPP;
			btsys_amp_right=((GPLR(btweb_features.amp_right)&GPIO_bit(btweb_features.amp_right))!=0);
                	return 0;
                break;
		case BTWEB_ABIL_FON_IP:
                        if (btweb_features.abil_fon_ip< 0)
                               return -EOPNOTSUPP;
			btsys_abil_fon_ip = ((GPLR(btweb_features.abil_fon_ip)&GPIO_bit(btweb_features.abil_fon_ip))!=0);
        	       	return 0;
                break;
		case BTWEB_MUTE_SPEAKER:
                        if (btweb_features.mute_speaker< 0)
                               return -EOPNOTSUPP;
			btsys_mute_speaker = ((GPLR(btweb_features.mute_speaker)&GPIO_bit(btweb_features.mute_speaker))!=0);
                	return 0;
                break;
		case BTWEB_ABIL_MIC:
                        if (btweb_features.abil_mic< 0)
                               return -EOPNOTSUPP;
			btsys_abil_mic = ((GPLR(btweb_features.abil_mic)&GPIO_bit(btweb_features.abil_mic))!=0);
                	return 0;
                break;
		case BTWEB_LED_SERR:
                        if (btweb_features.led_serr< 0)
                               return -EOPNOTSUPP;
			btsys_led_serr = ((GPLR(btweb_features.led_serr)&GPIO_bit(btweb_features.led_serr))!=0);
                	return 0;
                break;
		case BTWEB_LED_ESCL:
                        if (btweb_features.led_escl< 0)
                               return -EOPNOTSUPP;
			btsys_led_escl = ((GPLR(btweb_features.led_escl)&GPIO_bit(btweb_features.led_escl))!=0);
                	return 0;  
                break;
		case BTWEB_LED_MUTE:
                        if (btweb_features.led_mute< 0)
                               return -EOPNOTSUPP;
			btsys_led_mute = ((GPLR(btweb_features.led_mute)&GPIO_bit(btweb_features.led_mute))!=0);
                	return 0;
	        break;
		case BTWEB_LED_CONN:
                        if (btweb_features.led_conn< 0)
                               return -EOPNOTSUPP;
			btsys_led_conn = ((GPLR(btweb_features.led_conn)&GPIO_bit(btweb_features.led_conn))!=0);
                	return 0;
                break;
		case BTWEB_LED_ALARM:
                        if (btweb_features.led_alarm< 0)
                               return -EOPNOTSUPP;
			btsys_led_alarm = ((GPLR(btweb_features.led_alarm)&GPIO_bit(btweb_features.led_alarm))!=0);
                	return 0;
                break;
		case BTWEB_LED_TLC:
                        if (btweb_features.led_tlc< 0)
                               return -EOPNOTSUPP;
			btsys_led_tlc = ((GPLR(btweb_features.led_tlc)&GPIO_bit(btweb_features.led_tlc))!=0);
                	return 0;
                break;
		case BTWEB_POWER_SENSE:
                        if (btweb_features.power_sense< 0)
                               return -EOPNOTSUPP;
			btsys_power_sense = ((GPLR(btweb_features.power_sense)&GPIO_bit(btweb_features.power_sense))!=0);
                        return 0;
                break;
		case BTWEB_ABIL_TLK:
                        if (btweb_features.abil_tlk_i2c_addr< 0)
                              return -EOPNOTSUPP;
			btsys_i2c_read(btweb_features.abil_tlk_i2c_addr, btweb_features.abil_tlk_reg,&ret);
                        if (ret==0x0F)
				return btsys_abil_tlk=0;
			else
				return btsys_abil_tlk=1;
                break;
                case BTWEB_LIGHTING_LEVEL:
			if (btweb_features.lighting_level_i2c_addr< 0)
                              return -EOPNOTSUPP;
			btsys_i2c_read(btweb_features.lighting_level_i2c_addr, btweb_features.lighting_level_reg,&ret);
			return btsys_lighting_level=(int)(255-ret);
                break;
#ifdef USE_RX_TX_485_BTSYS
                case BTWEB_RX_TX_485:
                        if (btweb_features.rx_tx_485< 0)
                               return -EOPNOTSUPP;
			btsys_rx_tx_485 = ((GPLR(btweb_features.rx_tx_485)&GPIO_bit(btweb_features.rx_tx_485))!=0);
                        return 0;
                break;
#endif
		}
	return 1;
}

/* checker function, uses minmax but takes hold of the values immediately */
int btsys_proc(ctl_table *table, int write, struct file *filp,
	       void *buff, size_t *lenp)
{
	int retval;

#if 1
	if (!write)
	{
		btsys_read(table->ctl_name);
	}
#endif

        /* first update the touch-ago value */

        btsys_tago = (jiffies - btweb_globals.last_touch) / HZ;

	if (table->extra1){
		retval = proc_dointvec_minmax(table, write, filp, buff, lenp);
	}

	else{
		retval = proc_dointvec(table, write, filp, buff, lenp);
	}
/*	trace("btsys_proc: retval=%d",retval);*/
	if (!write || retval < 0) return retval;
/*	trace("btsys_proc: writing");*/

	/* written: apply the change */
	return btsys_apply(table->ctl_name);
}

/* the same, for sysctl(2) I/O */
static int btsys_sysctl(ctl_table *table, int *name, int nlen,
			void *oldval, size_t *oldlenp,
			void *newval, size_t newlen, void **context)
{
	int retval;

	/* first update the touch-ago value */
	btsys_tago = (jiffies - btweb_globals.last_touch) / HZ;

	//trace("btsys_sysctl name %s\n",name);

	retval = sysctl_intvec(table, name, nlen, oldval, oldlenp,
			       newval, newlen, context);
	if (!newval || retval < 0) return retval;
	trace("Modifying %s",table->procname);
	return btsys_apply(table->ctl_name);
}

#if 0 
ctl_table btsys_table[] = {
	{
		.ctl_name =      BTWEB_LED,
		.procname =      "led",
		.data =          &btsys_led,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_LCD,
		.procname =      "backlight",
		.data =          &btsys_lcd,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_CNTR,
		.procname =      "contrast",
		.data =          &btsys_cntr,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        byte_min,
		.extra2 =        byte_max,
	},
	{
		.ctl_name =      BTWEB_BUZZER,
		.procname =      "buzzer",
		.data =          btsys_buz,
		.maxlen =        2*sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
	},
	{
		.ctl_name =      BTWEB_BENABLE,
		.procname =      "buzzer_enable",
		.data =          &btsys_benable,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_UPSIDEDOWN,
		.procname =      "upsidedown",
		.data =          &btsys_upsidedown,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_TAGO,
		.procname =      "touch_ago",
		.data =          &btsys_tago,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
	},
	{
		.ctl_name =      BTWEB_RTCINVALID,
		.procname =      "rtc_invalid",
		.data =          &btweb_globals.rtc_invalid,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{
		.ctl_name =      BTWEB_HWNAME,
		.procname =      "name",
		.data =          btweb_globals.name,
		.maxlen =        BTWEB_NAMELEN,
		.mode =          0444,
		.proc_handler =  proc_dostring,
		.strategy =      sysctl_string,
	},
	{
		.ctl_name =      BTWEB_HWVERSION,
		.procname =      "hw_version",
		.data =          &btweb_globals.hw_version,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{0,}
};
#endif 

ctl_table btsys_table[] = {
	{
		.ctl_name =      BTWEB_LED,
		.procname =      "led",
		.data =          &btsys_led,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
        {
                .ctl_name =      BTWEB_LCD,
                .procname =      "backlight",
                .data =          &btsys_lcd,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_CNTR,
                .procname =      "contrast",
                .data =          &btsys_cntr,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        byte_min,
                .extra2 =        byte_max,
        },
        {
                .ctl_name =      BTWEB_BUZZER,
                .procname =      "buzzer",
                .data =          btsys_buz,
                .maxlen =        2*sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
        },
        {
                .ctl_name =      BTWEB_BENABLE,
                .procname =      "buzzer_enable",
                .data =          &btsys_benable,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_UPSIDEDOWN,
                .procname =      "upsidedown",
                .data =          &btsys_upsidedown,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_TAGO,
                .procname =      "touch_ago",
                .data =          &btsys_tago,
                .maxlen =        sizeof(int),
                .mode =          0444,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
        },
	{
		.ctl_name =      BTWEB_RTCINVALID,
		.procname =      "rtc_invalid",
		.data =          &btweb_globals.rtc_invalid,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{
		.ctl_name =      BTWEB_HWNAME,
		.procname =      "name",
		.data =          btweb_globals.name,
		.maxlen =        BTWEB_NAMELEN,
		.mode =          0444,
		.proc_handler =  proc_dostring,
		.strategy =      sysctl_string,
	},
	{
		.ctl_name =      BTWEB_HWVERSION,
		.procname =      "hw_version",
		.data =          &btweb_globals.hw_version,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{
		.ctl_name =      BTWEB_CTRL_HIFI,
		.procname =      "ctrl_hifi",
		.data =          &btsys_ctrl_hifi,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_CTRL_VIDEO,
		.procname =      "ctrl_video",
		.data =          &btsys_ctrl_video,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_VIRT_CONF,
		.procname =      "virt_conf",
		.data =          &btsys_virt_conf,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{
		.ctl_name =      BTWEB_ABIL_MOD_VIDEO,
		.procname =      "abil_mod_video",
		.data =          &btsys_abil_mod_video,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
        {
                .ctl_name =      BTWEB_ABIL_DEM_VIDEO,
                .procname =      "abil_dem_video",
                .data =          &btsys_abil_dem_video,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
	{
		.ctl_name =      BTWEB_ABIL_MOD_HIFI,
		.procname =      "abil_mod_hifi",
		.data =          &btsys_abil_mod_hifi,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
        {
                .ctl_name =      BTWEB_ABIL_DEM_HIFI,
                .procname =      "abil_dem_hifi",
                .data =          &btsys_abil_dem_hifi,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
	{
		.ctl_name =      BTWEB_ABIL_FON,
		.procname =      "abil_fon",
		.data =          &btsys_abil_fon,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
        {
		.ctl_name =      BTWEB_USB_PXA_SLAVE_CONNECTED,
                .procname =      "usb_pxa_slave_connected",
                .data =          &btweb_globals.usb_pxa_slave_connected,
                .maxlen =        sizeof(int),
                .mode =          0444,
                .proc_handler =  proc_dointvec,
                .strategy =      sysctl_intvec,
	},
        {
                .ctl_name =      BTWEB_BRIGHTNESS,
                .procname =      "brightness",
                .data =          &btsys_bright,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        byte_min,
                .extra2 =        byte_max,
        },
        {
                .ctl_name =      BTWEB_COLOR,
                .procname =      "color",
                .data =          &btsys_color,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        byte_min,
                .extra2 =        byte_max,
        },
        {
                .ctl_name =      BTWEB_VOL,
                .procname =      "amp_volume",
                .data =          &btsys_vol,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        byte_min,
                .extra2 =        byte_max,
        },
        {
                .ctl_name =      BTWEB_MIC_VOL,
                .procname =      "mic_volume",
                .data =          &btsys_mic_vol,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        byte_min,
                .extra2 =        byte_max,
        },
        {
                .ctl_name =      BTWEB_SPEAKER_VOL,
                .procname =      "speaker_volume",
                .data =          &btsys_speaker_vol,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        byte_min,
                .extra2 =        byte_max,
        },

        {
                .ctl_name =      BTWEB_ABIL_AMP,
                .procname =      "abil_amp",
                .data =          &btsys_abil_amp,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_AMP_LEFT,
                .procname =      "amp_left",
                .data =          &btsys_amp_left,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_AMP_RIGHT,
                .procname =      "amp_right",
                .data =          &btsys_amp_right,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_ABIL_FON_IP,
                .procname =      "abil_fon_ip",
                .data =          &btsys_abil_fon_ip,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_MUTE_SPEAKER,
                .procname =      "mute_speaker",
                .data =          &btsys_mute_speaker,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_ABIL_MIC,
                .procname =      "abil_mic",
                .data =          &btsys_abil_mic,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_LED_SERR,
                .procname =      "led_serr",
                .data =          &btsys_led_serr,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_LED_ESCL,
                .procname =      "led_escl",
                .data =          &btsys_led_escl,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_LED_MUTE,
                .procname =      "led_mute",
                .data =          &btsys_led_mute,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_LED_CONN,
                .procname =      "led_conn",
                .data =          &btsys_led_conn,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_LED_ALARM,
                .procname =      "led_alarm",
                .data =          &btsys_led_alarm,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_LED_TLC,
                .procname =      "led_tlc",
                .data =          &btsys_led_tlc,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_POWER_SENSE,
                .procname =      "power_sense",
                .data =          &btsys_power_sense,
                .maxlen =        sizeof(int),
                .mode =          0444,
                .proc_handler =  btsys_proc,/*proc_dointvec,*/
                .strategy =      btsys_sysctl,/*sysctl_intvec,*/
        },
        {
                .ctl_name =      BTWEB_ABIL_TLK,
                .procname =      "abil_tlk",
                .data =          &btsys_abil_tlk,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
        {
                .ctl_name =      BTWEB_LIGHTING_LEVEL,
                .procname =      "lighting_level",
                .data =          &btsys_lighting_level,
                .maxlen =        sizeof(int),
                .mode =          0444,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
        },
#ifdef USE_RX_TX_485_BTSYS
	{
                .ctl_name =      BTWEB_RX_TX_485,
                .procname =      "rx_tx_485",
                .data =          &btsys_rx_tx_485,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
#endif
	{0,}
};


static ctl_table dev_table[] = {
	{
		.ctl_name =      DEV_BTWEB,
		.procname =      "btweb",
		.mode =          0555,
		.child =         btsys_table,
	},
	{0,}
};

static ctl_table root_table[] = {
	{
		.ctl_name =      CTL_DEV,
		.procname =      "dev",
		.mode =          0555,
		.child =         dev_table,
	},
	{0,}
};
  
static struct ctl_table_header *btsys_header;

int btsys_init(void)
{
	trace("sysctl MUST be aligned to real values set before: FIXME\n");
	trace("btweb_globals.name=%s",btweb_globals.name);

#if 0 
	if ((btweb_globals.flavor==BTWEB_F453AV)||(btweb_globals.flavor==BTWEB_F453AVA)|| \
		(btweb_globals.flavor==BTWEB_2F	)) {
		trace("Mapping gpio virtualization for btweb pxa kernel version 2_2_X");

		dev_table[0].child=btsys_table_F453AV;
	}
#endif

	btsys_header = register_sysctl_table(root_table, 0);

	if (btsys_header) return 0;
		
	
	return -EBUSY;
}

void btsys_exit(void)
{
	unregister_sysctl_table(btsys_header);
}

module_init(btsys_init);
module_exit(btsys_exit);

#endif /* CONFIG_SYSCTL */
