
#include <linux/sched.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include "btweb-cammotors.h"

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
static struct motor_state motor_brake   = { 1, 1, 1, 1 };

static int pan_status = ST_IDLE;
static int tilt_status = ST_IDLE;
static int pan_step;
static int tilt_step;

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
		if (pan_status == ST_FWD)
			pan_step++;
		else
			pan_step--;
		pan_step &= CAM_NSTEPS-1;
	}

	if (tilt_status != ST_IDLE)
	{
		set_tiltmotor_state(&motor_steps[tilt_step]);
		if (tilt_status == ST_FWD)
			tilt_step++;
		else
			tilt_step--;
		tilt_step &= CAM_NSTEPS-1;
	}

	/* Check end sensors */
	if ( (pan_status == ST_FWD && CAMPAN_FC1)
		|| (pan_status == ST_BACK && CAMPAN_FC2) )
	{
		pan_status = ST_IDLE;
		set_panmotor_state(&motor_standby);
	}

	if ( (tilt_status == ST_FWD && CAMTILT_FC2)
		|| (tilt_status == ST_BACK && CAMTILT_FC1) )
	{
		tilt_status = ST_IDLE;
		set_tiltmotor_state(&motor_standby);
	}
}

static void start_intr(void)
{
	if (!(OIER & OIER_E2))
	{
		OSMR2 = OSCR + pulse_ticks;	/* set initial match */
		OSSR = OSSR_M2;			/* clear status on timer 2 */
		request_irq(IRQ_OST2, cammotors_interrupt, SA_INTERRUPT, "cammotors", NULL);
		OIER |= OIER_E2;		/* enable match on timer 2 to cause interrupts */
	}
}

static void stop_intr(void)
{
	if (pan_status == ST_IDLE && tilt_status == ST_IDLE)
	{
		OIER &= ~OIER_E2;		/* disable match on timer 2 intr */
		free_irq(IRQ_OST2, NULL);
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

	printk(KERN_INFO "Camera motors GPIO initialized\n");
}

/* Set the motor stepping speed in Hz */
void cam_sethz(int hz)
{
	pulse_ticks = CLOCK_TICK_RATE / hz;
	printk(KERN_DEBUG "Camera motors pulse_ticks changed: %d\n", pulse_ticks);
}

int cam_gethz(void)
{
	return (CLOCK_TICK_RATE / pulse_ticks);
}

void cam_panfwd(void)
{
	pan_status = ST_FWD;
	start_intr();
}

void cam_panback(void)
{
	pan_status = ST_BACK;
	start_intr();
}

void cam_panstop(void)
{
	pan_status = ST_IDLE;
	stop_intr();
	set_panmotor_state(&motor_standby);
}

void cam_tiltfwd(void)
{
	tilt_status = ST_FWD;
	start_intr();
}

void cam_tiltback(void)
{
	tilt_status = ST_BACK;
	start_intr();
}

void cam_tiltstop(void)
{
	tilt_status = ST_IDLE;
	stop_intr();
	set_tiltmotor_state(&motor_standby);
}
