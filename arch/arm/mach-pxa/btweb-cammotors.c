/*
 *  linux/arch/arm/mach-pxa/btweb-cammotors.c
 *
 *  Support for the step by step camera motor control
 *  Developed by Develer 2007
 *  
 *  Improvements made by Raffaele Recalcati in 2007,2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */


#include <linux/sched.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include "btweb-cammotors.h"

/* DEBUG */
#undef DEBUG

#ifdef DEBUG
	#define dbg(format, arg...) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg)
#else
	#define dbg(format, arg...) do {} while (0)
#endif

#define trace(format, arg...) printk(KERN_INFO __FILE__ ": " format "\n" , ## arg)



/*
 * Define this for a more "soft" mode for driving motors
 */
//#define CAM_HALFSTEP 1

/* Motor states */
#define ST_IDLE 0
#define ST_FWD  1
#define ST_BACK 2

struct motor_state {
	int coil1a;
	int coil1b;
	int coil2a;
	int coil2b;
};

static int init=0;

#ifdef CAM_HALFSTEP
	#define CAM_NSTEPS 8  // Note: irq handler assumes this is a power of 2!
	static struct motor_state motor_steps[CAM_NSTEPS] = {
		{ 1, 0, 1, 0 },
		{ 1, 0, 0, 0 },
		{ 1, 0, 0, 1 },
		{ 0, 0, 0, 1 },
		{ 0, 1, 0, 1 },
		{ 0, 1, 0, 0 },
		{ 0, 1, 1, 0 },
		{ 0, 0, 1, 0 }
	};
#else
	#define CAM_NSTEPS 4  // Note: irq handler assumes this is a power of 2!
	static struct motor_state motor_steps[CAM_NSTEPS] = {
		{ 1, 0, 1, 0 },
		{ 1, 0, 0, 1 },
		{ 0, 1, 0, 1 },
		{ 0, 1, 1, 0 }
	};
#endif

static struct motor_state motor_standby = { 0, 0, 0, 0 };
//static struct motor_state motor_brake   = { 1, 1, 1, 1 };

static int pan_status = ST_IDLE;
static int tilt_status = ST_IDLE;
static int pan_move = ST_IDLE;
static int tilt_move = ST_IDLE;
static int pan_step;
static int tilt_step;
static int tilt_step_cur=-1;
static int pan_step_cur=-1;
static int tilt_step_set=-1;
static int pan_step_set=-1;
static int tilt_step_cur_ok=-1;
static int pan_step_cur_ok=-1;


static int pulse_ticks = CLOCK_TICK_RATE / CAM_HZ;

inline static void set_panmotor_state(const struct motor_state *st)
{
	if (st->coil1a)
		GPSR(btweb_features.MIN1A) = GPIO_bit(btweb_features.MIN1A);
	else
		GPCR(btweb_features.MIN1A) = GPIO_bit(btweb_features.MIN1A);

	if (st->coil1b)
		GPSR(btweb_features.MIN1B) = GPIO_bit(btweb_features.MIN1B);
	else
		GPCR(btweb_features.MIN1B) = GPIO_bit(btweb_features.MIN1B);

	if (st->coil2a)
		GPSR(btweb_features.MIN2A) = GPIO_bit(btweb_features.MIN2A);
	else
		GPCR(btweb_features.MIN2A) = GPIO_bit(btweb_features.MIN2A);

	if (st->coil2b)
		GPSR(btweb_features.MIN2B) = GPIO_bit(btweb_features.MIN2B);
	else
		GPCR(btweb_features.MIN2B) = GPIO_bit(btweb_features.MIN2B);
}

inline static void set_tiltmotor_state(const struct motor_state *st)
{
	if (st->coil1a)
		GPSR(btweb_features.MIN3A) = GPIO_bit(btweb_features.MIN3A);
	else
		GPCR(btweb_features.MIN3A) = GPIO_bit(btweb_features.MIN3A);

	if (st->coil1b)
		GPSR(btweb_features.MIN3B) = GPIO_bit(btweb_features.MIN3B);
	else
		GPCR(btweb_features.MIN3B) = GPIO_bit(btweb_features.MIN3B);

	if (st->coil2a)
		GPSR(btweb_features.MIN4A) = GPIO_bit(btweb_features.MIN4A);
	else
		GPCR(btweb_features.MIN4A) = GPIO_bit(btweb_features.MIN4A);

	if (st->coil2b)
		GPSR(btweb_features.MIN4B) = GPIO_bit(btweb_features.MIN4B);
	else
		GPCR(btweb_features.MIN4B) = GPIO_bit(btweb_features.MIN4B);
}

static void cammotors_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	OSSR = OSSR_M2;

	int next_match;

	/* Loop until we get ahead of the free running timer. */
	do {
		OSSR = OSSR_M2;  /* Clear match on timer 2 */
		next_match = (OSMR2 += pulse_ticks);
	} while ( (signed long)(next_match - OSCR) <= 0 );

	if (pan_status != ST_IDLE)
	{
		set_panmotor_state(&motor_steps[pan_step]);
		if (pan_status == ST_FWD) {
			pan_step++;
			pan_step_cur++;
		}
		else {
			pan_step--;
			pan_step_cur--;
		}
		pan_step &= CAM_NSTEPS-1;
	}

	if (tilt_status != ST_IDLE)
	{
		set_tiltmotor_state(&motor_steps[tilt_step]);
		if (tilt_status == ST_FWD) {
			tilt_step++;
			tilt_step_cur++;
		}
		else {
			tilt_step--;
			tilt_step_cur--;
		}
		tilt_step &= CAM_NSTEPS-1;
	}

	/* Check end sensors : pan */
	if ( (pan_status == ST_FWD && CAMPAN_FC1)
		|| (pan_status == ST_BACK && CAMPAN_FC2) )
	{
		trace("Tlc FC CAMPAN reached: pan_status=%s",(pan_status==ST_FWD) ? "FORWARD":"BACK");
		if (CAMPAN_FC2) 
			pan_step_cur = 0;
		if (CAMPAN_FC1)
			dbg("Tlc FC CAMPAN max value %d",pan_step_cur); 
			
		pan_status = ST_IDLE;
		pan_move = ST_IDLE;
		set_panmotor_state(&motor_standby);

		pan_step_cur_ok = pan_step_cur;
	}

	/* Check end sensors : tilt */
	if ( (tilt_status == ST_FWD && CAMTILT_FC2)
		|| (tilt_status == ST_BACK && CAMTILT_FC1) )
	{
		if (CAMTILT_FC1)
			tilt_step_cur=0;
		if (CAMTILT_FC2)
			 trace("Tlc FC CAMTILT max value %d",tilt_step_cur);
		trace("Tlc FC CAMTILT reached: tilt_status=%s",(tilt_status==ST_FWD) ? "FORWARD":"BACK");
		tilt_status = ST_IDLE;
		tilt_move = ST_IDLE;
		set_tiltmotor_state(&motor_standby);

		tilt_step_cur_ok = tilt_step_cur;
	}

	/* Check set pan position */
	if ( ( (pan_move == ST_FWD ) && ( pan_step_set<=pan_step_cur) ) 
		||  ( ( pan_move == ST_BACK ) && ( pan_step_set>=pan_step_cur) ) )
	{
		dbg("Pan position reached %d",pan_step_cur);
		pan_status = ST_IDLE;
		pan_move = ST_IDLE;
		set_panmotor_state(&motor_standby);
		pan_step_cur_ok = pan_step_cur;
	}

	/* Check set tilt position */
	if ( ( (tilt_move == ST_FWD ) && ( tilt_step_set<=tilt_step_cur) )
		|| ( ( tilt_move == ST_BACK ) && ( tilt_step_set>=tilt_step_cur) ) )
	{
		dbg("Tilt position reached %d",tilt_step_cur);
		tilt_status = ST_IDLE;
		tilt_move = ST_IDLE;
		set_tiltmotor_state(&motor_standby);
		tilt_step_cur_ok = tilt_step_cur;
	}

}

static void start_intr(void)
{
	if (!(OIER & OIER_E2))
	{
		OSMR2 = OSCR + pulse_ticks;	/* set initial match */
		OSSR = OSSR_M2;			/* clear status on timer 2 */
                if (!init) {
                    request_irq(IRQ_OST2, cammotors_interrupt, SA_INTERRUPT, "cammotors", NULL);
                    init = 1;
                }
		OIER |= OIER_E2;		/* enable match on timer 2 to cause interrupts */
	}
}

static void stop_intr(void)
{
	if (pan_status == ST_IDLE && tilt_status == ST_IDLE)
	{
		OIER &= ~OIER_E2;		/* disable match on timer 2 intr */
//		free_irq(IRQ_OST2, NULL);
	}
}

void init_cammotors(void)
{
	/* Activate motor control chip */
	GPCR(btweb_features.abil_motor_ctrl) = GPIO_bit(btweb_features.abil_motor_ctrl);

	/* Motors in standby */
	set_panmotor_state(&motor_standby);
	set_tiltmotor_state(&motor_standby);

	/* Set all GPIO directions */
	GPDR(btweb_features.abil_motor_ctrl) |= GPIO_bit(btweb_features.abil_motor_ctrl);
	GPDR(btweb_features.MIN1A) |= GPIO_bit(btweb_features.MIN1A);
	GPDR(btweb_features.MIN1B) |= GPIO_bit(btweb_features.MIN1B);
	GPDR(btweb_features.MIN2A) |= GPIO_bit(btweb_features.MIN2A);
	GPDR(btweb_features.MIN2B) |= GPIO_bit(btweb_features.MIN2B);
	GPDR(btweb_features.MIN3A) |= GPIO_bit(btweb_features.MIN3A);
	GPDR(btweb_features.MIN3B) |= GPIO_bit(btweb_features.MIN3B);
	GPDR(btweb_features.MIN4A) |= GPIO_bit(btweb_features.MIN4A);
	GPDR(btweb_features.MIN4B) |= GPIO_bit(btweb_features.MIN4B);

	GPDR(btweb_features.fc_pan) &= ~GPIO_bit(btweb_features.fc_pan);
	GPDR(btweb_features.fc2_pan) &= ~GPIO_bit(btweb_features.fc2_pan);
	GPDR(btweb_features.fc_tilt) &= ~GPIO_bit(btweb_features.fc_tilt);
	GPDR(btweb_features.fc2_tilt) &= ~GPIO_bit(btweb_features.fc2_tilt);

	btweb_features.cammotor_pan_set = 0;
	btweb_features.cammotor_tilt_set = 0;

	trace("Camera motors GPIO initialized");

}

/* Set the motor stepping speed in Hz */
void cam_sethz(int hz)
{
	pulse_ticks = CLOCK_TICK_RATE / hz;
	trace("Camera motors pulse_ticks changed: %d", pulse_ticks);
}

int cam_gethz(void)
{
	return (CLOCK_TICK_RATE / pulse_ticks);
}

int cam_getpan(void)
{
	return pan_step_cur_ok; 
}

int cam_gettilt(void)
{
	return tilt_step_cur_ok;
}

void cam_panfwd(void)
{
        if (pan_step_cur_ok==-1){
            dbg("initing: pan fwd to fc");
        } else {
            pan_step_cur_ok=-2;
        }
	pan_status = ST_FWD;
	start_intr();
}

void cam_panback(void)
{
        if (pan_step_cur_ok==-1){
            dbg("initing: pan back to fc");
        } else {
            pan_step_cur_ok=-2;
        }
	pan_status = ST_BACK;
	start_intr();
}

int cam_panstop(void)
{
        if (pan_step_cur_ok!=-1){
            pan_status = ST_IDLE;
            stop_intr();
            set_panmotor_state(&motor_standby);
            pan_step_cur_ok = pan_step_cur;
            return 0;
        }
        return -EAGAIN;
}

void cam_tiltfwd(void)
{
        if (tilt_step_cur_ok==-1){
            dbg("initing: tilt fwd to fc");
        } else {
            tilt_step_cur_ok=-2;
        }
	tilt_status = ST_FWD;
	start_intr();
}

void cam_tiltback(void)
{
        if (tilt_step_cur_ok==-1){
            dbg("initing: tilt back to fc");
        } else {
            tilt_step_cur_ok=-2;
        }
        tilt_status = ST_BACK;
	start_intr();
}

int cam_tiltstop(void)
{
        if (tilt_step_cur_ok!=-1){
            tilt_status = ST_IDLE;
            stop_intr();
            set_tiltmotor_state(&motor_standby);
            tilt_step_cur_ok = tilt_step_cur;
            return 0;            
        }
        return -EAGAIN;
}

int cam_tiltmove(int loc_tilt_step_set)
{
	tilt_step_set = loc_tilt_step_set;
	dbg(KERN_INFO "cam_tiltmove to %d",tilt_step_set);
	if (tilt_step_cur_ok==-2){
		dbg("already moving");
		return -EAGAIN;
	} else if (tilt_step_cur_ok!=-1){
		dbg("cam_tiltmove");
		if (tilt_step_set>tilt_step_cur_ok){
			dbg("cam_tiltmove: fwd");
			tilt_status = ST_FWD;
			tilt_move = ST_FWD;
			tilt_step_cur_ok=-2;
			start_intr();
		} else if (tilt_step_set<tilt_step_cur_ok){
			dbg("cam_tiltmove: back");
			tilt_status = ST_BACK;
			tilt_move = ST_BACK;
			tilt_step_cur_ok=-2;
			start_intr();
		}
	} else {
		dbg("initing: tilt back to fc");
		tilt_status = ST_BACK;
		tilt_step_cur_ok=-2;
		start_intr();
	}
	return 0;
}

int cam_panmove(int loc_pan_step_set)
{
	pan_step_set = loc_pan_step_set;
	dbg("cam_panmove to %d",pan_step_set);
	if (pan_step_cur_ok==-2){
		dbg("already moving");
		return -EAGAIN;
	} else if (pan_step_cur_ok!=-1){
		dbg("cam_panmove");
		if (pan_step_set>pan_step_cur_ok){
			dbg("cam_panmove: fwd");
			pan_status = ST_FWD;
			pan_move = ST_FWD;
			pan_step_cur_ok=-2;
			start_intr();
		} else if (pan_step_set<pan_step_cur_ok){
			dbg("cam_panmove: back");
			pan_status = ST_BACK;
			pan_move = ST_BACK;
			pan_step_cur_ok=-2;
			start_intr();
		}
	}
	else {
		dbg("initing: pan back to fc");
		pan_status = ST_BACK;
		pan_step_cur_ok=-2;
		start_intr();
	}
	return 0;
}
