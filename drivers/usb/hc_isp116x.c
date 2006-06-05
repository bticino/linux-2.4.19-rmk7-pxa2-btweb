/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*
 * isp1161 USB HCD for Linux Version 0.8 (9/3/2001)
 *
 * Roman Weissgaerber - weissg@vienna.at (C) 2001
 *
 * Modified to add isochronous and dma by
 * Troy Kisky  - troy.kisky@boundarydevices.com (C) 2003
 *
 *-------------------------------------------------------------------------*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *-------------------------------------------------------------------------*/

#include <linux/config.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
//#include <linux/malloc.h>
#include <linux/errno.h>
#include <linux/init.h>
//#include <linux/interrupt.h> //in_interrupt()
#include <linux/smp_lock.h>
#include <linux/list.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/irq.h>

// My Define to verify !!!raf
#define DEBUG
//#undef  USE_DMA
#undef CONFIG_CRIND
//#define USE_LOAD_SELECTABLE_DELAY
//#define USE_MEM_FENCE
//#define CONFIG_USB_ISP116x_DMA


#ifdef CONFIG_CRIND
	/****************************************************
	* oldham
	* For the crind, include the correct PPC dma header
	****************************************************/
	#include <asm/dma.h>
	#include <asm/mpc8xx.h>
	#include <asm/8xx_immap.h>      //immap, immap_t
	#include <asm/irq.h>
	#include "crind-usb.h"			//Local header required for replacement outw/inw functions

	#define REQUEST_IRQ request_8xxirq	//for crind use the 8xx interrupt register routine
	#define RELEASE_REGION(ptr,flags) release_region((int)ptr, flags)
	#define REQUEST_REGION(base,cnt,name) request_region((int)base, cnt, name)
	#define IOREMAP(base,flags) ioremap((int)base,flags);

	/**********************************************************
	 * oldham
	 * interrupt setup
	 * Pulled from Kelly's driver.
	 **********************************************************/
	static inline void initIrqLine(void )
	{
	    /* Request the Interrupt line (shared) */
	    //pointer to registers
	    crind_mmap = (immap_t *) IMAP_ADDR;
	#ifdef INTR_EDGE
	    //set INT3,INT4 to edge triggered
	    crind_mmap ->im_siu_conf.sc_siel |= 0x02800000;
	#else
	    //set INT3,INT4 to level triggered
	    crind_mmap ->im_siu_conf.sc_siel &= ~0x02800000;
	#endif
	    //disable INT3,INT4 mask bits
	    crind_mmap ->im_siu_conf.sc_simask &= ~0x02800000;
	    //reset INT3,INT4 flags by writing ones to them
	    crind_mmap ->im_siu_conf.sc_sipend |= 0x02800000;
	}


	/**********************************************************
	 * oldham
	 * Memory Map setup so the driver can access HC registers.
	 * Pulled from Kelly's original driver.
	 **********************************************************/
	static inline void initMemMap(void)
	{
		//pointer to registers
		crind_mmap = (immap_t *)IMAP_ADDR;
		//set pins as actively driven outputs, no tristate
		CLEAR_BITS(crind_mmap->im_cpm.cp_pbodr, USB_ALL);
		//set as outputs
		SET_BITS(crind_mmap->im_cpm.cp_pbdir, USB_ALL);
		//set aw sgpio pins
		CLEAR_BITS(crind_mmap->im_cpm.cp_pbpar, USB_ALL);
		//put chip into reset
		CLEAR_BITS(crind_mmap->im_cpm.cp_pbdat, USB_RESET);
		//set for HC ports
		CLEAR_BITS(crind_mmap->im_cpm.cp_pbdat, USB_A0 |USB_A1 | USB_TP1);
		//Philips spec requires minimum of 166us
		udelay(200);
		//pull chip out of reset
		SET_BITS(crind_mmap->im_cpm.cp_pbdat, USB_RESET);
		//Can't seem to find this value in the spec but it must be > 0
		udelay(30);
	}
	#define MY_OUT_CMD(regindex,base) ISP116x_OUTW_COMMAND(regindex,base+8);
	#define MY_INW(port) ISP116x_INW((volatile long*)port)
	#define MY_OUTW(value,port) ISP116x_OUTW(value,(volatile int*)port)
	#define MY_DELAY_OUTW(value,port) MY_OUTW(value,port)
	#define ITL_BUFFER_SIZE 256

	#undef CONFIG_USB_ISP116x_DMA_REQ_HIGH		//low active dma request, FIXME: should be in config file

	#undef CONFIG_USB_ISP116x_INTERRUPT_HIGH_ACTIVE //low active interrupt
	#define CONFIG_USB_ISP116x_CLK_NOT_STOP		//clock can not be stopped
	#define get_lr(regs) ((regs)->link)
#else
	#include <asm/arch/dma.h>
	#define REQUEST_IRQ request_irq
	#define RELEASE_REGION(ptr,flag) release_region((int)ptr, flag)
	#define REQUEST_REGION(base,cnt,name) request_region((int)base, cnt, name)
	#define IOREMAP(base,flags) ioremap((unsigned int)base,flags);
	static inline void initIrqLine(void ) { }
	static inline void initMemMap(void ) { }


// ORIG !!!raf	#define MY_OUT_CMD(regindex,base) outw(regindex, base+2);
	#define MY_OUT_CMD(regindex,base) outw(regindex, base+4); //!!!raf
	#define MY_INW(port) inw(port)
	#define MY_OUTW(value,port) outw(value,port)
	#define MY_DELAY_OUTW(value,port) MY_OUTW(value,port)
	#define DEBUG

	#define CONFIG_USB_ISP116x_INTERRUPT_HIGH_ACTIVE			//FIXME: should be in config file
	#undef  CONFIG_USB_ISP116x_CLK_NOT_STOP		//clock can be stopped
	#define get_lr(regs) ((regs)->ARM_lr)
#endif

#ifndef ITL_BUFFER_SIZE
	#define ITL_BUFFER_SIZE (1024+8)
#endif

#ifndef DEBUG
#ifdef CONFIG_USB_DEBUG
   #define DEBUG
#endif
#endif

#include <linux/usb.h>

#ifndef dbg
#ifdef DEBUG
	#define dbg(format, arg...) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg)
#else
	#define dbg(format, arg...) do {} while (0)
#endif
#endif

#include "hc_isp116x.h"
#include "hc_simple116x.h"
#include "pxa-dmaWork.h"

int hc_verbose = 1;
int hc_error_verbose = 0;
int urb_debug = 0;
#define NODMA -1
#define ALLOC_DMA -2


#ifdef CONFIG_USB_ISP116x_DMA
#define DEFAULT_DMA_BASE CONFIG_USB_ISP116x_DMA_PHYS_BASE
#define DEFAULT_DMA ALLOC_DMA
#define STR_DEFAULT_DMA "-2"
#else
#define DEFAULT_DMA_BASE 0
#define DEFAULT_DMA NODMA
#define STR_DEFAULT_DMA "-1"
#endif

#ifdef CONFIG_USB_ISP116x_DMA_REQ_HIGH
#define HC_DREQOutputPolarity DREQOutputPolarity	//dma request active high
#else
#define HC_DREQOutputPolarity 0				//dma request active low
#endif

#ifdef CONFIG_USB_ISP116x_15KR
#define HC_DOWN_STREAM_15KR DownstreamPort15KRSel
#else
#define HC_DOWN_STREAM_15KR 0
#endif

#ifdef CONFIG_USB_ISP116x_LEVEL
//this chip gives a pulse of 167ns on the interrupt, pxa250 requires 1000ns
//so you must use level triggered for pxa250 or extra hardware to extend the pulse
#define HC_INTERRUPT_PIN_TRIGGER 0
#else
#define HC_INTERRUPT_PIN_TRIGGER InterruptPinTrigger
#endif

#ifdef CONFIG_USB_ISP116x_EXTERNAL_OC
#define HC_OC_DETECTION 0
#else
#define HC_OC_DETECTION AnalogOCEnable
#endif

#ifdef CONFIG_USB_ISP116x_CLK_NOT_STOP
#define HC_CLK_NOT_STOP SuspendClkNotStop
#else
#define HC_CLK_NOT_STOP 0
#endif

#ifdef CONFIG_USB_ISP116x_INTERRUPT_HIGH_ACTIVE
#define HC_INTERRUPT_OUTPUT_POLARITY InterruptOutputPolarity
#else
#define HC_INTERRUPT_OUTPUT_POLARITY 0
#endif

#define HcHardwareConfiguration_SETTING ( HC_CLK_NOT_STOP | InterruptPinEnable| HC_INTERRUPT_PIN_TRIGGER \
		| HC_INTERRUPT_OUTPUT_POLARITY | DataBusWidth16 | HC_OC_DETECTION | HC_DREQOutputPolarity | HC_DOWN_STREAM_15KR)


#define PORT_TYPE_IO 0
#define PORT_TYPE_PHYS 1
#define PORT_TYPE_VIRT 2
#define PORT_TYPE_NONE 3
#define PORT_TYPE_MEMORY 4
#define PORT_TYPE_UDELAY 5

#ifdef CONFIG_USB_ISP116x_HC_IO
#define DEFAULT_HC_BASE CONFIG_USB_ISP116x_HC_IO_BASE
#define DEFAULT_HC_TYPE PORT_TYPE_IO
#endif
#ifdef CONFIG_USB_ISP116x_HC_PHYS
#define DEFAULT_HC_BASE CONFIG_USB_ISP116x_HC_PHYS_BASE
#define DEFAULT_HC_TYPE PORT_TYPE_PHYS
#endif
#ifdef CONFIG_USB_ISP116x_HC_VIRT
#define DEFAULT_HC_BASE CONFIG_USB_ISP116x_HC_VIRT_BASE
#define DEFAULT_HC_TYPE PORT_TYPE_VIRT
#endif


#ifdef CONFIG_USB_ISP116x_MEM_FENCE_IO
#define DEFAULT_MEM_FENCE_BASE CONFIG_USB_ISP116x_MEM_FENCE_IO_BASE
#define DEFAULT_MEM_FENCE_TYPE PORT_TYPE_IO
#define DEFAULT_MEM_FENCE_ACCESS_COUNT CONFIG_USB_ISP116x_MEM_FENCE_IO_ACCESS_COUNT
#endif
#ifdef CONFIG_USB_ISP116x_MEM_FENCE_PHYS
#define DEFAULT_MEM_FENCE_BASE CONFIG_USB_ISP116x_MEM_FENCE_PHYS_BASE
#define DEFAULT_MEM_FENCE_TYPE PORT_TYPE_PHYS
#define DEFAULT_MEM_FENCE_ACCESS_COUNT CONFIG_USB_ISP116x_MEM_FENCE_PHYS_ACCESS_COUNT
#endif
#ifdef CONFIG_USB_ISP116x_MEM_FENCE_VIRT
#define DEFAULT_MEM_FENCE_BASE CONFIG_USB_ISP116x_MEM_FENCE_VIRT_BASE
#define DEFAULT_MEM_FENCE_TYPE PORT_TYPE_VIRT
#define DEFAULT_MEM_FENCE_ACCESS_COUNT CONFIG_USB_ISP116x_MEM_FENCE_VIRT_ACCESS_COUNT
#endif
#ifdef CONFIG_USB_ISP116x_MEM_FENCE_NONE
#define DEFAULT_MEM_FENCE_BASE 0
#define DEFAULT_MEM_FENCE_TYPE PORT_TYPE_NONE
#define DEFAULT_MEM_FENCE_ACCESS_COUNT 1
#endif
#ifdef CONFIG_USB_ISP116x_MEM_FENCE_MEMORY
#define DEFAULT_MEM_FENCE_BASE 0
#define DEFAULT_MEM_FENCE_TYPE PORT_TYPE_MEMORY
#define DEFAULT_MEM_FENCE_ACCESS_COUNT CONFIG_USB_ISP116x_MEM_FENCE_MEMORY_ACCESS_COUNT
#endif
#ifdef CONFIG_USB_ISP116x_MEM_FENCE_UDELAY
#define DEFAULT_MEM_FENCE_BASE 0
#define DEFAULT_MEM_FENCE_TYPE PORT_TYPE_UDELAY
#define DEFAULT_MEM_FENCE_ACCESS_COUNT 1
#endif


#ifdef CONFIG_USB_ISP116x_WU_IO
#define DEFAULT_WU_BASE CONFIG_USB_ISP116x_WU_IO_BASE
#define DEFAULT_WU_TYPE PORT_TYPE_IO
#endif
#ifdef CONFIG_USB_ISP116x_WU_PHYS
#define DEFAULT_WU_BASE CONFIG_USB_ISP116x_WU_PHYS_BASE
#define DEFAULT_WU_TYPE PORT_TYPE_PHYS
#endif
#ifdef CONFIG_USB_ISP116x_WU_VIRT
#define DEFAULT_WU_BASE CONFIG_USB_ISP116x_WU_VIRT_BASE
#define DEFAULT_WU_TYPE PORT_TYPE_VIRT
#endif
#ifdef CONFIG_USB_ISP116x_WU_NONE
#define DEFAULT_WU_BASE 0
#define DEFAULT_WU_TYPE PORT_TYPE_NONE
#endif

#define DEFAULT_IRQ CONFIG_USB_ISP116x_IRQ		//GPIO_2_80_TO_IRQ(4)


#define __STR1__(a) #a
#define __STR__(a) __STR1__(a)

// Note:
// The command port is write ONLY.
//
// For 16 bit access, this means that the address
// must be saved so that the interrupt routine can restore
// it before exiting.
//
// For 32 bit or buffer ram access, interrupts
// must be prevented between the write of the address to the command port
// and the read/write of actual data. There is no way for the
// interrupt handler to restore the previous state.


#ifdef USE_MEM_FENCE

#ifdef USE_LOAD_SELECTABLE_DELAY
#define S_INLINE
#define DO_DELAY(hci) if (hci->hp.delay) hci->hp.delay(hci)
#else

#define S_INLINE inline

#if (DEFAULT_MEM_FENCE_TYPE == PORT_TYPE_UDELAY)
	#define DO_DELAY(hci) delayUDELAY(hci)
#elif (DEFAULT_MEM_FENCE_TYPE != PORT_TYPE_NONE)
	#if (DEFAULT_MEM_FENCE_ACCESS_COUNT==1)
		#define DO_DELAY(hci) delay1(hci)
	#elif (DEFAULT_MEM_FENCE_ACCESS_COUNT==2)
		#define DO_DELAY(hci) delay2(hci)
	#elif (DEFAULT_MEM_FENCE_ACCESS_COUNT==3)
		#define DO_DELAY(hci) delay3(hci)
	#elif (DEFAULT_MEM_FENCE_ACCESS_COUNT>3)
		#define DO_DELAY(hci) delayN(hci)
	#endif
#endif

#endif		//USE_LOAD_SELECTABLE_DELAY

static S_INLINE void delayUDELAY(hci_t* hci)
{
	udelay (1);
}

static S_INLINE void delay1(hci_t* hci)
{
	MY_DELAY_OUTW(0,hci->hp.memFencePort);	//80ns access time for 1st access
}

static S_INLINE void delay2(hci_t* hci)
{
	port_t port=hci->hp.memFencePort;
	MY_DELAY_OUTW(0,port);	//80ns access time for 1st access
	MY_DELAY_OUTW(0,port);	//140ns delay between CS, 80 access time for 2nd access
}

static S_INLINE void delay3(hci_t* hci)
{
	port_t port=hci->hp.memFencePort;
	MY_DELAY_OUTW(0,port);	//80ns access time for 1st access
	MY_DELAY_OUTW(0,port);	//140ns delay between CS, 80 access time for 2nd access
	MY_DELAY_OUTW(0,port);	//1 more for luck
}

static S_INLINE void delayN(hci_t* hci)
{
	port_t port = hci->hp.memFencePort;
	int cnt = hci->hp.memCnt;
//	printk("memCnt=%d\n",cnt);
	do { MY_DELAY_OUTW(0,port); } while (--cnt);
}
#endif

#ifndef DO_DELAY
#define DO_DELAY(hci) do {} while(0)
#endif

static inline void ReadReg0(hci_t* hci, int regindex)
{
	hci->hp.cur_regIndex = regindex;
	MY_OUT_CMD(regindex, hci->hp.hcport);
	DO_DELAY(hci);
}

static inline void ReadReg0_(hci_t* hci, int regindex, port_t port)
{
	hci->hp.cur_regIndex = regindex;
	MY_OUT_CMD(regindex, port);
	DO_DELAY(hci);
}

static inline int ReadReg16(hci_t* hci, int regindex)
{
	int val = 0;
	port_t port = hci->hp.hcport;	//loading to a variable generates better code because of the volatiles involved
	ReadReg0_(hci,regindex,port);
	val = MY_INW(port);
	return val;
}

static inline int ReadReg32(hci_t* hci, int regindex)
{
	int val16, val;
	port_t port = hci->hp.hcport;
	ReadReg0_(hci,regindex,port);
	val16 = MY_INW(port);
	val = val16;
	val16 = MY_INW(port);
	val += val16 << 16;
	return val;
}

static inline void WriteReg0(hci_t* hci, int regindex)
{
	regindex |= 0x80;
	hci->hp.cur_regIndex = regindex;
	MY_OUT_CMD(regindex, hci->hp.hcport);
	DO_DELAY(hci);
}

static inline void WriteReg0_(hci_t* hci, int regindex, port_t port)
{
	regindex |= 0x80;
	hci->hp.cur_regIndex = regindex;
	MY_OUT_CMD(regindex, port);
	DO_DELAY(hci);
}

static inline void WriteReg16(hci_t* hci, unsigned int value, int regindex)
{
	port_t port = hci->hp.hcport;	//loading to a variable generates better code because of the volatiles involved
	WriteReg0_(hci,regindex,port);
	MY_OUTW(value, port);
}

static inline void WriteReg32(hci_t* hci, unsigned int value, int regindex)
{
	port_t port = hci->hp.hcport;
	WriteReg0_(hci,regindex,port);
	MY_OUTW(value, port);
	MY_OUTW(value >> 16, port);
}

//////////////////
static int hc116x_read_reg16(hci_t* hci, int regindex)
{
	int ret;
#ifdef USE_COMMAND_PORT_RESTORE
	hci->hp.command_regIndex = regindex;		//only called from bh, so spin not needed
	wmb();
	ret = ReadReg16(hci,regindex);
	mb();
	hci->hp.command_regIndex = CP_INVALID;
#else
	unsigned long flags;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	ret = ReadReg16(hci,regindex);
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
#endif
	return ret;
}

static void hc116x_write_reg16(hci_t* hci, unsigned int value, int regindex)
{
#ifdef USE_COMMAND_PORT_RESTORE
	hci->hp.command_regIndex = regindex | 0x80;
	wmb();
	WriteReg16(hci, value, regindex);
	mb();
	hci->hp.command_regIndex = CP_INVALID;
#else
	unsigned long flags;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	WriteReg16(hci, value, regindex);
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
#endif
}

#ifdef USE_DMA
#define CHIP_BUSY ((hci->extraFlags & EXM_PIO) || (hci->transferState & TF_DmaInProgress))
#else
#define CHIP_BUSY (hci->extraFlags & EXM_PIO)
#endif

static int NoteResetFields(hci_t * hci, struct frameList * fl, int resetMask, int bstat,const char* message)
{
	int resetIdleMask = hci->resetIdleMask;
	hci->resetIdleMask = resetIdleMask | resetMask;
	if (resetIdleMask==0) {
		hci->reset_fl = fl;
		hci->resetBstat = bstat;
		hci->resetReason = message;
		hci->resetExtraFlags = hci->extraFlags;
		hci->resetTransferState = hci->transferState;
	}
	return resetIdleMask;
}


//only hc116x_read_reg32 and hc116x_write_reg32 are to be used
//by non-interrupt routines because a spin is needed in this case.
int hc116x_read_reg32(hci_t* hci, int regIndex)
{
	__u8 cp=CP_INVALID;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&hci->command_port_lock, flags);
	if (CHIP_BUSY) {
		unsigned long startTime= jiffies;
		while (1) {
			unsigned long i=jiffies-startTime;
			spin_unlock_irqrestore(&hci->command_port_lock, flags);
			if (i > 2) printk(KERN_ERR "hc116x_read_reg32 timeout\n");
			spin_lock_irqsave(&hci->command_port_lock, flags);

			if (!CHIP_BUSY) goto doWork;
			if (regIndex != CP_INVALID) {
				if (hci->pending_read_regIndex==CP_INVALID) {
					hci->pending_read_regIndex = regIndex;
					regIndex = CP_INVALID;
				}
			} else if (hci->pending_read_regIndex==CP_READ_DONE) {
				ret = hci->pending_read_val;
				hci->pending_read_regIndex = CP_INVALID;
				break;
			}
			if (i > 2) {
				if (hci->extraFlags & EXM_PIO) cp = hci->hp.cur_regIndex;
				NoteResetFields(hci,&hci->frames[FRAME_ATL],0x80,hci->bstat,"hc116x_read_reg32 timeout");
				hci->extraFlags = 0;
				hci->transferState= TF_IDLE;
				hci->transfer.progress = NULL;
				tasklet_schedule(&hci->bottomHalf);
				goto doWork;
			}
		}
	} else {
doWork:
		if (hci->pending_write_regIndex!=CP_INVALID) {
			WriteReg32(hci, hci->pending_write_val, hci->pending_write_regIndex);
			hci->pending_write_regIndex = CP_INVALID;
		}
		if (hci->pending_read_regIndex!=CP_INVALID) if (hci->pending_read_regIndex!=CP_READ_DONE) {
			hci->pending_read_val = ReadReg32(hci, hci->pending_read_regIndex);
			hci->pending_read_regIndex = CP_READ_DONE;
		}
		if (regIndex != CP_INVALID) ret = ReadReg32(hci,regIndex);
		else {
			ret = hci->pending_read_val;
			hci->pending_read_regIndex = CP_INVALID;
		}
		if (cp != CP_INVALID) {
			ReadReg0 (hci, cp);				//command_regIndex includes the r/w flag
		}
	}
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
	return ret;
}

void hc116x_write_reg32(hci_t* hci, unsigned int value, int regIndex)
{
	__u8 cp=CP_INVALID;
	unsigned long flags;

	spin_lock_irqsave(&hci->command_port_lock, flags);
	if (CHIP_BUSY) {
		unsigned long startTime= jiffies;
		while (1) {
			unsigned long i=jiffies-startTime;
			spin_unlock_irqrestore(&hci->command_port_lock, flags);
			if (i > 2) printk(KERN_ERR "hc116x_write_reg32 timeout\n");
			spin_lock_irqsave(&hci->command_port_lock, flags);

			if (!CHIP_BUSY) goto writeVals;
			if (hci->pending_write_regIndex==CP_INVALID) {
				hci->pending_write_val = value;
				hci->pending_write_regIndex = regIndex;
				break;
			}
			if (i > 2) {
				if (hci->extraFlags & EXM_PIO) cp = hci->hp.cur_regIndex;
				NoteResetFields(hci,&hci->frames[FRAME_ATL],0x80,hci->bstat,"hc116x_write_reg32 timeout");
				hci->extraFlags = 0;
				hci->transferState= TF_IDLE;
				hci->transfer.progress = NULL;
				tasklet_schedule(&hci->bottomHalf);
				goto writeVals;
			}
		}
	} else {
writeVals:
		if (hci->pending_write_regIndex!=CP_INVALID) {
			WriteReg32(hci, hci->pending_write_val, hci->pending_write_regIndex);
			hci->pending_write_regIndex = CP_INVALID;
		}
		WriteReg32(hci, value, regIndex);
		if (cp != CP_INVALID) {
			ReadReg0 (hci, cp);				//cur_regIndex includes the r/w flag
		}
	}
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
}

static inline void hci_dump ( hci_t * hci, int i)
{
	printk("/******************************************************/\n\n");
	printk("ISP1161 Register Dump:\n");
	printk("HcControl=0x%x\n",ReadReg32 (hci, HcControl) );
	printk("HcCommandStatus=0x%x\n",ReadReg32 (hci, HcCommandStatus) );
	printk("HcInterruptStatus=0x%x\n",ReadReg32 (hci, HcInterruptStatus) );
	printk("HcInterruptEnable=0x%x\n",ReadReg32 (hci, HcInterruptEnable) );
	printk("HcInterruptDisable=0x%x\n",ReadReg32 (hci, HcInterruptDisable) );
	printk("HcFmInterval=0x%x\n",ReadReg32 (hci, HcFmInterval) );
	printk("HcFmRemaining=0x%x\n",ReadReg32 (hci, HcFmRemaining) );
	printk("HcFmNumber=0x%x\n",ReadReg32 (hci, HcFmNumber) );
	printk("HcLSThreshold=0x%x\n",ReadReg32 (hci, HcLSThreshold) );
	printk("HcRhDescriptorA=0x%x\n",ReadReg32 (hci, HcRhDescriptorA) );
	printk("HcRhDescriptorB=0x%x\n",ReadReg32 (hci, HcRhDescriptorB) );
	printk("HcRhStatus=0x%x\n",ReadReg32 (hci, HcRhStatus) );
	printk("HcRhPortStatus=0x%x\n",ReadReg32 (hci, HcRhPortStatus) );
	printk("HcHardwareConfiguration=0x%x\n",ReadReg16 (hci, HcHardwareConfiguration) );
	printk("HcDMAConfiguration=0x%x\n",ReadReg16 (hci, HcDMAConfiguration) );
	printk("HcTransferCounter=0x%x\n",ReadReg16 (hci, HcTransferCounter) );
	printk("HcuPInterrupt=0x%x\n",ReadReg16 (hci, HcuPInterrupt) );
	printk("HcuPInterruptEnable=0x%x\n",ReadReg16 (hci, HcuPInterruptEnable) );
	printk("HcChipID=0x%x\n",ReadReg16 (hci, HcChipID) );
	printk("HcScratch=0x%x\n",ReadReg16 (hci, HcScratch) );
	printk("HcITLBufferLength=0x%x\n",ReadReg16 (hci, HcITLBufferLength) );
	printk("HcATLBufferLength=0x%x\n",ReadReg16 (hci, HcATLBufferLength) );
	printk("HcBufferStatus=0x%x\n",ReadReg16 (hci, HcBufferStatus) );
	printk("HcReadBackITL0Length=0x%x\n",ReadReg16 (hci, HcReadBackITL0Length) );
	printk("HcReadBackITL1Length=0x%x\n",ReadReg16 (hci, HcReadBackITL1Length) );
	printk("HcITLBufferPort=0x%x\n",ReadReg16 (hci, HcITLBufferPort) );
	printk("HcATLBufferPort=0x%x\n",ReadReg16 (hci, HcATLBufferPort) );
	printk("/******************************************************/\n\n");
}

__u8 SaveState(hci_t* hci)
{
#ifdef USE_COMMAND_PORT_RESTORE
	return hci->hp.command_regIndex;
#else
	return  CP_INVALID;
#endif
}

void RestoreState(hci_t* hci,__u8 cp)
{
#ifdef USE_COMMAND_PORT_RESTORE
	if (cp != CP_INVALID) {
		hci->hp.command_regIndex = cp;				//restore previous state
		ReadReg0 (hci, cp);			//command_regIndex includes the r/w flag
	}
#endif
}

/////////////////////////////////////////////////
static inline int CheckForPlaceHolderNeeded(struct frameList * fl)
{
	if (fl->chain) return 1;
	if (fl->idle_chain) return 1;
	if (fl->iso_partner) {
		if (fl->iso_partner->chain) return 1; //next iso frame has work queued
		if (fl->iso_partner->idle_chain) return 1;
	}
	return 0;
}

static inline void TransferActivate(hci_t* hci,struct frameList * fl,int chain)
{
	hci->transfer.chain = chain;
	hci->transfer.progress = fl->chain;
	wmb();
	hci->transferState = TFM_EotPending;
}

static int checkAllowedRegion(struct timing_lines* t,int cnt,int rem)
{
	int high,low;
	//calculate max time to finish
	high = (cnt * t->high.slope) + t->high.b;
	if ((rem<<16) > high) return 1;
	low  = (cnt * t->low.slope ) + t->low.b;
	if ((rem<<16) < low) return 1;
	return 0;
}

static int checkAllowedRegionHigh(struct timing_lines* t,int cnt,int rem)
{
	int high;
	if (rem>100) rem-=100;		//allow time to make sure the ATL starts NEXT frame
	else rem=0;
	//calculate max time to finish
	high = (cnt * t->high.slope) + t->high.b;
	if ((rem<<16) > high) return 1;
	return 0;
}

void TestPoint(int x, int y, struct timing_lines* t,hci_t* hci);
#ifdef CONFIG_USB_ISP116x_TRAINING
void TestPoint1(int x, int y, struct timing_lines* t,hci_t* hci);
#endif


static inline int LockedRequest(hci_t* hci,struct frameList * fl)
{
	int rem;
#ifdef CONFIG_USB_ISP116x_TRAINING
	int remEnd=0;
#endif

	int cnt = fl->reqCount;
	int (*checkFunction)(struct timing_lines* t,int cnt,int rem);

	if (hci->sofint) return 0;		//start of frame interrupt has happened, check for responses to read
	if (hci->extraFlags) {
			NoteResetFields(hci,fl,fl->full,hci->bstat,"LockedRequest bug");
			return 0;
	}
	if (cnt <= 1) {
#if 0
		if (CheckForPlaceHolderNeeded(fl)) {
			fl->reqCount = 1;	//mark as something sent

			TransferActivate(hci,fl,REQ_CHAIN);
			WriteReg16(hci, 8, HcTransferCounter);
			WriteReg32(hci, LAST_MARKER, fl->port);
			WriteReg32(hci, ISO_MARKER, fl->port);
			hci->transfer.progress = NULL;
			if (1) {
				int i=0;
				if ((fl->chain==NULL) && (fl->idle_chain==NULL)) {fl = fl->iso_partner; i=1;}
				if (fl) printk(KERN_DEBUG __FILE__ ": i:%i,chain:%p,idle:%p",i,fl->chain,fl->idle_chain);
			}
		}
#endif
		return 0;
	}

	fl->sofCnt = 0;
	hci->transfer.fl = fl;
//if ITL buffers are active, then don't start an ATL request unless it will finish transfering THIS frame
//because we need to guarantee that an ATLINT will happen this frame or next frame
	if (fl->done==ATLBufferDone) {
		if (hci->frames[FRAME_ITL0].reqCount || hci->frames[FRAME_ITL1].reqCount) {
//			fl->reqWaitForSof=3;		//uncomment this to make only 1 ATL req in any given frame
			if ((hci->fmUsingWithItl & FM_ITL)==0) {
				hci->fmUsingWithItl=FM_ITL|FM_SWITCHING;
				//takes effect next frame
				WriteReg32(hci, (((unsigned)hci->fmLargestBitCntWithItl)<<16)|FI_BITS, HcFmInterval);
			}
			checkFunction = checkAllowedRegionHigh;
		} else {
			if (hci->fmUsingWithItl & FM_ITL) {
				hci->fmUsingWithItl=FM_SWITCHING;
				//takes effect next frame
				WriteReg32(hci, (INIT_fmLargestBitCnt << 16) | FI_BITS, HcFmInterval);
			}
			checkFunction = checkAllowedRegion;
		}
	} else {
		if (hci->atlPending) if ((hci->fmUsingWithItl & FM_ITL)==0) return 0;
		checkFunction=checkAllowedRegionHigh;
	}

#ifdef USE_FM_REMAINING

// *********************************************
//burst of 4 timings (8 bytes)
//1 tAS
//2 tCES
//4*(RDF+1) pwe low time
//3*(RDN+1) pwe high time
//RRR*2 +1
//___________
//4(RDF) + 3(RDN) + 2(RRR) + 11 memclks   + time needed to read 8 bytes from RAM
//4*4    + 3*5    + 2*2 + 11 = 46
// 460ns/8 = 57.5ns/byte
// *********************************************
//Programmed I/O data points
//data points,
//Bytes,   fmRemainingReq
//RDF+RDN+2 == 3 + 10 + 2 = 150ns/2bytes + (ram read time) = 75ns/byte
// *********************************************
//rem is in units of 83 ns

	do {
		int extraPio = 0;
		int transferState = TFM_EotPending;
		int extraFlags = EX_WORD_WRITE;
		rem = ReadReg16(hci, HcFmRemaining);		//can chip handle 16 bit only read????

		if (rem > hci->lastFmRem) return 0;		//start of frame interrupt wants to happen

		hci->lastFmRem = rem;
		if (hci->atlPending) if (fl->done!=ATLBufferDone) {
			//itl buffer
			//increase byte count written
			if (hci->resetIdleMask==0) {
				transferState = TFM_EotPending|TFM_ExtraPending;
				extraPio = EXTRA_PIO_REQ_CNT;
				extraFlags = EX_WORD_WRITE_W_ATL;
////				goto tryPio;
			} else goto programmed_io;
		}
#ifdef USE_DMA
#ifdef TRY_PIO
		if (cnt<=72) if (checkFunction(&hci->pioLinesReq,cnt,rem)) goto programmed_io;
#endif

//		if (fl->done==ATLBufferDone) if (hci->frames[FRAME_ITL0].reqCount || hci->frames[FRAME_ITL1].reqCount)
//				goto tryPio;
		if (checkFunction(&hci->dmaLinesReq,cnt,rem)) {
dma_io:
			if (hci->resetIdleMask==0) {
				int extraDma = 0;
				int dmaTransferState = TF_StartDma;
				if (extraPio) {
//					goto programmed_io;
					extraDma = ((cnt+1)&(BURST_TRANSFER_SIZE-2));
					if (extraDma) extraDma = BURST_TRANSFER_SIZE - extraDma;
					if (extraDma > EXTRA_DMA_REQ_CNT) extraDma = EXTRA_DMA_REQ_CNT;
					hci->extraPioCnt = extraDma;
					if (extraDma >= EXTRA_DMA_REQ_CNT) {
						dmaTransferState = TF_StartDmaExtra;
					} else {
						dmaTransferState = TF_StartDmaExtraDma;
						extraDma += EXTRA_DMA_REQ_CNT;
					}
				}
				if (StartDmaChannel(hci,fl,REQ_CHAIN,dmaTransferState)==0) {
					WriteReg16(hci, (cnt+extraDma+1)& ~1, HcTransferCounter); 	//round to be safe
					WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_WRITE|fl->dmac, HcDMAConfiguration);
					fl->transferState=dmaTransferState;
					if (extraDma) hci->extraFlags = extraFlags;		//next interrupt finish the transfer
					fl->extraFlags=hci->extraFlags;
					break;
				}
			}
			if (extraPio) if (fl->done==ATLBufferDone) goto programmed_io;
		}
#endif
////tryPio:
#ifdef TRY_PIO
		if (checkFunction(&hci->pioLinesReq,cnt,rem)) {
programmed_io:
			WriteReg16(hci, (cnt+extraPio+1)& ~1, HcTransferCounter); 	//round to be safe
			WriteReg0(hci, fl->port);
			fl->transferState=transferState;
			FakeDmaReqTransfer(hci->hp.hcport, cnt, fl->chain,hci,transferState);
			if (extraPio) hci->extraFlags = extraFlags|EXM_PIO;		//next interrupt finish the transfer
#ifdef CONFIG_USB_ISP116x_TRAINING
			else {
				int bits;
				remEnd = ReadReg16(hci, HcFmRemaining);		//can chip handle 16 bit only read????
				bits = rem - remEnd;
				if (bits<0) bits += FI_BITS;
				TestPoint(cnt,bits,&hci->pioLinesReq,hci);
			}
#endif
			fl->extraFlags=hci->extraFlags;
			break;
		}
#endif
		if (fl->done==ATLBufferDone) {
			if (hci->frames[FRAME_ITL0].reqCount || hci->frames[FRAME_ITL1].reqCount) {
				//atl requests MUST finish the transfer THIS frame if ITL active
				return 0;
			}
		} else return 0;		//itl request must always finish THIS frame

		if (hci->resetIdleMask) goto programmed_io;
		//increase byte count written
		transferState = TFM_EotPending|TFM_ExtraPending;
		extraPio = EXTRA_PIO_REQ_CNT;
#ifdef USE_DMA
#ifdef TRY_PIO
//		goto programmed_io;
		if (cnt<=72) goto programmed_io;
#endif
		goto dma_io;
#else
		goto programmed_io;
#endif
	} while (0);
	fl->fmRemainingReq = rem;
	fl->extraFmRemaining = 0;
	hci->lastTransferFrame = fl;
#else
	do {
		WriteReg16(hci, (cnt+1)& ~1, HcTransferCounter); 	//round to be safe
		if (cnt>72)
			if (StartDmaChannel(hci,fl,REQ_CHAIN,0)==0) {
				WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_WRITE|fl->dmac, HcDMAConfiguration);
				break;
			}
		WriteReg0(hci, fl->port);
		FakeDmaReqTransfer(hci->hp.hcport, cnt, fl->chain,hci,TFM_EotPending);
	} while (0);
#endif

#ifdef INT_HISTORY_SIZE
	if (hci->resetIdleMask==0) {
		int i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
		struct history_entry* h = &hci->intHistory[i];
		h->type = HIST_TYPE_TRANSFER;
		h->transferState = fl->transferState;
		h->uP = 0;
		h->bstat = 0;
		h->pc = NULL;
		h->lr = NULL;
		h->elapsed = 0;
		h->fmRemaining = rem;
#ifdef CONFIG_USB_ISP116x_TRAINING
		h->fmRemainingEnd = remEnd;
#endif
		h->byteCount = cnt;
		h->done = fl->done;
		h->extraFlags = hci->extraFlags;
	}
#endif
	if (fl->done==ATLBufferDone) {
		hci->atlPending=1;
	}
	return 1;
}

static inline int StartRequestTransfer(hci_t* hci,struct frameList * fl)
{
	int ret;
	unsigned long flags;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	ret = LockedRequest(hci,fl);
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
	return ret;
}

static inline int LockedResponse(hci_t* hci,struct frameList * fl)
{
	int cnt = fl->rspCount;
	int rem=1<<14;
#ifdef CONFIG_USB_ISP116x_TRAINING
	int remEnd=0;
#endif
#ifdef USE_DMA
	int dmaAllowed = ( hci->resetIdleMask || ((fl->done==ATLBufferDone) && (hci->frames[FRAME_ITL0].reqCount || hci->frames[FRAME_ITL1].reqCount))) ? 0 : 1;
#endif
	if (hci->extraFlags) {
			NoteResetFields(hci,fl,fl->full,hci->bstat,"LockedResponse bug");
	}
	fl->sofCnt = 0;
	hci->transfer.fl = fl;
	if (cnt > 0) {
#ifdef USE_FM_REMAINING
		do {
			int extraPio = 0;
			int transferState = TFM_EotPending|TFM_RSP;
			int extraFlags = EX_WORD_READ;
			if (fl->done==ATLBufferDone) {
				rem = ReadReg16(hci, HcFmRemaining);		//can chip handle 16 bit only read????
				hci->lastFmRem = rem;
			}
			else if (hci->atlPending) {
				//itl buffer
				//increase byte count written
				if (hci->resetIdleMask==0) {
					transferState = TFM_EotPending|TFM_RSP|TFM_ExtraPending;
					extraPio = EXTRA_PIO_RSP_CNT;
					extraFlags = EX_WORD_READ_W_ATL;
				}
#ifdef USE_DMA
				if (!dmaAllowed) goto programmed_io;
#endif
			}
#ifdef USE_DMA
#ifdef TRY_PIO
			if (cnt<=72) if (checkAllowedRegion(&hci->pioLinesRsp,cnt,rem)) goto programmed_io;
#endif
			if (checkAllowedRegion(&hci->dmaLinesRsp,cnt,rem)) {
dma_io:
				if (dmaAllowed) {
					int extraDma = 0;
					int dmaTransferState = TF_StartDma|TFM_RSP;
					if (extraPio) {
						goto programmed_io;
						extraDma = ((cnt+1)&(BURST_TRANSFER_SIZE-2));
						if (extraDma) extraDma = BURST_TRANSFER_SIZE - extraDma;
						if (extraDma > EXTRA_DMA_RSP_CNT) extraDma = EXTRA_DMA_RSP_CNT;
						hci->extraPioCnt = extraDma;
						if (extraDma >= EXTRA_DMA_RSP_CNT) {
							dmaTransferState = TF_StartDmaExtra|TFM_RSP;
						} else {
							dmaTransferState = TF_StartDmaExtraDma|TFM_RSP;
							extraDma += EXTRA_DMA_RSP_CNT;
						}
					}
					if (StartDmaChannel(hci,fl,RSP_CHAIN,dmaTransferState)==0) {
						WriteReg16(hci, (cnt+extraDma+1)& ~1, HcTransferCounter);	//round to be safe
						WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_READ|fl->dmac, HcDMAConfiguration);
						fl->transferState=dmaTransferState;
						if (extraDma) hci->extraFlags = extraFlags;		//next interrupt finish the transfer
						fl->extraFlags=hci->extraFlags;
						break;
					}
				}
				if (extraPio) goto programmed_io;
			}
//tryPio:
#endif

#ifdef TRY_PIO
			if (checkAllowedRegion(&hci->pioLinesRsp,cnt,rem)) {
programmed_io:
				WriteReg16(hci, (cnt+extraPio+1)& ~1, HcTransferCounter);	//round to be safe
				ReadReg0(hci, fl->port);
				fl->transferState=transferState;
				FakeDmaRspTransfer(hci->hp.hcport, cnt, fl->chain,hci,transferState);
				if (extraPio) hci->extraFlags = extraFlags|EXM_PIO;		//next interrupt finish the transfer
	#ifdef CONFIG_USB_ISP116x_TRAINING
				else if (rem<=FI_BITS) {
					int bits;
					remEnd = ReadReg16(hci, HcFmRemaining);		//can chip handle 16 bit only read????
					bits = rem - remEnd;
					if (bits<0) bits += FI_BITS;
					TestPoint1(cnt,bits,&hci->pioLinesRsp,hci);
				}
	#endif
				fl->extraFlags=hci->extraFlags;
				break;
			}
#endif
			if (hci->resetIdleMask) goto programmed_io;
			//increase byte count written
			transferState = TFM_EotPending|TFM_RSP | TFM_ExtraPending;
			extraPio = EXTRA_PIO_RSP_CNT;
#ifdef USE_DMA
	#ifdef TRY_PIO
			if (cnt<=72) goto programmed_io;
	#endif
			goto dma_io;
#else
			goto programmed_io;
#endif
		} while (0);
		fl->fmRemainingRsp = rem;
		fl->extraFmRemaining = 0;
		hci->lastTransferFrame = fl;
#else
		do {
			WriteReg16(hci, (cnt+1)& ~1, HcTransferCounter);	//round to be safe
			if (cnt>72)
				if (StartDmaChannel(hci,fl,RSP_CHAIN,0)==0) {
					WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_READ|fl->dmac, HcDMAConfiguration);
					break;
				}
			ReadReg0(hci, fl->port);
			FakeDmaRspTransfer(hci->hp.hcport, cnt, fl->chain,hci,TFM_EotPending);
		} while (0);
#endif
	} else {
	//read 2 bytes to clear the done bit...
		TransferActivate(hci,fl,RSP_CHAIN);
		WriteReg16(hci, 2, HcTransferCounter);
		ReadReg16 (hci, fl->port);
		hci->transfer.progress = NULL;
		if (fl->reqCount==0) {
			printk(KERN_DEBUG __FILE__ ": extra done bit?? done:%x",fl->done);
		}
	}
#ifdef INT_HISTORY_SIZE
	if (hci->resetIdleMask==0) {
		int i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
		struct history_entry* h = &hci->intHistory[i];
		h->type = HIST_TYPE_TRANSFER;
		h->transferState = fl->transferState;
		h->uP = 0;
		h->bstat = 0;
		h->pc = NULL;
		h->lr = NULL;
		h->elapsed = 0;
		h->fmRemaining = rem;
#ifdef CONFIG_USB_ISP116x_TRAINING
		h->fmRemainingEnd = remEnd;
#endif
		h->byteCount = cnt;
		h->done = fl->done;
		h->extraFlags = hci->extraFlags;
	}
#endif
	return 1;
}

static inline int StartResponseTransfer(hci_t* hci,struct frameList * fl)
{
	int ret;
	unsigned long flags;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	ret = LockedResponse(hci,fl);
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
	return ret;
}


static int hc_init_regs(hci_t * hci,int bUsbReset);

#if 1
static inline int OffInTheWeeds(struct frameList * fl,int limit)
{
	return (fl->sofCnt > limit);
}
#else
#define OffInTheWeeds(a,b) 0
#endif


static void NewPoint(hci_t * hci,struct frameList * fl);

static inline int BufferDone(hci_t * hci,struct frameList * fl,int bstat)
{
	if (bstat & fl->done) {
//		if (bstat & (ITL0BufferDone|ITL1BufferDone)) printk("bstat:%x ",bstat);
		return 1;
	}
	if (bstat & fl->full) {
		if (fl->reqCount)
//			if ((bstat & (ITL0BufferDone|ITL1BufferDone|ATLBufferDone))==0)
				if (OffInTheWeeds(fl,5)) {
//this chip is off in the weeds, get ready to reset
					if (NoteResetFields(hci,fl,fl->full,bstat,(fl->done==ATLBufferDone) ? "ATL BufferDone lost1" :
									 "..ITL BufferDone lost2")==0) {
						 //ITL would have been caught earlier
#ifdef USE_FM_REMAINING
						if (hci->lastEOTuP & SOFITLInt) NewPoint(hci,fl);
#endif
					}
				}
	}
	return 0;
}
#ifdef USE_DMA
static void hc_release_dma(hcipriv_t * hp);
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int CheckClearExtra(hci_t * hci,struct pt_regs * regs);

static void LowerFmBitCnt(hci_t * hci)
{
	if (hci->fmUsingWithItl==FM_ITL) {
		unsigned long flags;
		spin_lock_irqsave(&hci->command_port_lock, flags);
		if (!CHIP_BUSY) if (hci->fmLargestBitCntWithItl > INIT_fmLargestBitCntWithItlMin) {
			hci->fmLargestBitCntWithItl -= 10;
			hci->fmUsingWithItl=FM_ITL|FM_SWITCHING;
			//takes effect next frame
			WriteReg32(hci, (((unsigned)hci->fmLargestBitCntWithItl)<<16)|FI_BITS, HcFmInterval);
		}
		spin_unlock_irqrestore(&hci->command_port_lock, flags);
		printk(KERN_DEBUG __FILE__ ": fmLargestBitCntWithItl:%i\n",hci->fmLargestBitCntWithItl);
	}
}

static int DoProcess(hci_t * hci, struct frameList * fl,struct ScheduleData * sd)
{
	int bScheduleRan=0;
	switch (fl->state) {
		case STATE_READ_RESPONSE: {
			int bstat = hci->bstat;
			if (hci->transferState & TF_TransferInProgress) break;

			if (BufferDone(hci,fl,bstat)==0) {
				int bd = (fl->done & (ITL0BufferDone|ITL1BufferDone));
				if (bd==0) break;	//if not ITL

				if (NoteResetFields(hci,fl,fl->full,bstat, (hci->lastEOTuP & SOFITLInt) ? "ITL BufferDone lost3" :
																						 "ITL BufferDone but not on EOT")==0) {
					if (hci->lastEOTuP & SOFITLInt) {
//						if (bd != (ITL0BufferDone|ITL1BufferDone))
							NewPoint(hci,hci->lastTransferFrame);
					} else {
						if (fl->extraFlags & EXM_WAITING_ATLINT) LowerFmBitCnt(hci);
					}
				}
				if (bstat & (ITL0BufferDone|ITL1BufferDone)) {
					hci->sofint=1;
				}
				//if neither buffer done bit is set then buffer is not really lost
				//just a chip bug when ATL read finishes close to SOF
				//but the full bit will not be cleared by the read and
				//a software reset is still necessary
			}
			else {
				unsigned char done = bstat & fl->done;
				if (fl->done != done) {
					//set which buffer this is ITL0 or ITL1
					fl->done = done;
					fl->full = done >> 3;
					fl->iso_partner->done = done = done ^ (ITL0BufferDone|ITL1BufferDone);
					fl->iso_partner->full = done >> 3;
				}
				if (!StartResponseTransfer(hci,fl)) break;
				fl->prevRspCount = fl->rspCount;
				fl->reqCount=0;
				fl->rspCount = 0;
			}
			fl->state = STATE_SCHEDULE_WORK;

		}
		case STATE_SCHEDULE_WORK: {

			if (fl->done==ATLBufferDone) {
				fl->sofCnt = 0;
				if (hci->sofint || fl->reqWaitForSof) break;
			}  //itl has priority
			if (ScheduleWork(hci,fl,sd)!=0) break;
			bScheduleRan = 1;
			if (!fl->reqCount) {
				fl->active=0;
				if (fl->iso_partner) {
					fl->iso_partner->done = fl->done = (ITL0BufferDone|ITL1BufferDone);
					fl->iso_partner->full = fl->full = (ITL0BufferFull|ITL1BufferFull);
				}
				break;
			}
			fl->state = STATE_WRITE_REQUEST;
		}
		case STATE_WRITE_REQUEST: {
			int bstat = hci->bstat;
			if (hci->transferState & TF_TransferInProgress) break;

			if (fl->iso_partner) {
				if ((bstat & (ITL0BufferFull|ITL1BufferFull))==0) {
					//if both buffers are empty, I don't know which will be written.
					if (fl->iso_partner->done!=(ITL0BufferDone|ITL1BufferDone)) {
						if ((bstat & (ITL0BufferDone|ITL1BufferDone))==0) {
							fl->iso_partner->state = STATE_SCHEDULE_WORK;	//in case last write was too late
						}
					}
					fl->iso_partner->done = fl->done = (ITL0BufferDone|ITL1BufferDone);
					fl->iso_partner->full = fl->full = (ITL0BufferFull|ITL1BufferFull);
				}
			} else if (fl->reqWaitForSof) {
					fl->state = STATE_SCHEDULE_WORK;
					break;
			}

			fl->active=0;
			if (hci->sofint) {
				if ((fl->done & (ITL0BufferDone|ITL1BufferDone))) fl->state = STATE_SCHEDULE_WORK; //if missed SOF and ITL buffer, then reschedule work
				break;	//sof read has priority.
			}
			if ( (bstat & fl->full) == fl->full) {
				NoteResetFields(hci,fl,fl->full,bstat,"full clear lost");
				break;
			}
			if (hci->resetIdleMask) break;
			if (!StartRequestTransfer(hci,fl)) {
				if (fl->done!=ATLBufferDone) {
					LowerFmBitCnt(hci);
					fl->prevRspCount = fl->rspCount;
					fl->reqCount=0;
					fl->rspCount = 0;
					fl->state = STATE_SCHEDULE_WORK;
				}
				break;
			}
			fl->state = STATE_READ_RESPONSE;
		}
	}
	return bScheduleRan;
}

static int DoItl(hci_t * hci)
{
	int bScheduleRan=0;
	do {
		struct frameList * fl = hci->itlActiveFrame;
		if (fl) if (fl->active) {
			bScheduleRan |= DoProcess(hci, fl,&hci->sditl);
			if (fl->active) break;
		}

		if (hci->sofint) {
			hci->sofint=0;
//the ping-pong itl buffer access only toggles with the SOFINT
//therefore, when both buffers are empty, we can only write to 1 buffer
//then wait for the SOF before writing to the next buffer
			wmb();
			{
				int i = hci->itl_index;
				ScanNewIsoWork(hci,i);
				hci->itlActiveFrame = fl = &hci->frames[i];
				fl->active=1;
				hci->itl_index = i ^ (FRAME_ITL0 ^ FRAME_ITL1); //next SOF partner will be processed
				bScheduleRan |= (DoProcess(hci, fl,&hci->sditl));
				if (fl->active) break;
			}
		}
		bScheduleRan |= 0x80;
	} while (0);

	return bScheduleRan;
}

#ifdef INT_HISTORY_SIZE
static unsigned int CheckElapsedTime(hci_t * hci)
{
	struct timeval timeVal;
	unsigned int elapsed;
	do_gettimeofday(&timeVal);
	elapsed = timeVal.tv_sec - hci->timeVal.tv_sec;
	elapsed *= 1000000;
	elapsed += timeVal.tv_usec - hci->timeVal.tv_usec;
//	if (elapsed > 1900) printk("us:%i", elapsed);
	hci->timeVal.tv_sec = timeVal.tv_sec;
	hci->timeVal.tv_usec = timeVal.tv_usec;
	return elapsed;
}
#endif

#define BSTAT_WORK_PENDING 0x10000

static void PrintLineInfo(hci_t * hci);
static void ScanCancelled(hci_t * hci);

static void hc_interrupt_bh(unsigned long __hci)
{
	hci_t * hci = (hci_t *)__hci;
	int bScheduleRan;
#ifdef INT_HISTORY_SIZE
	int resetIdleMask = hci->resetIdleMask;
#endif
#ifdef USE_COMMAND_PORT_RESTORE
	__u8 cp = hci->hp.command_regIndex;
	if (cp != CP_INVALID) {
		DO_DELAY(hci);	//This is probably only needed in the real interrupt routine, not bottom half
	}
#endif
#ifdef INT_HISTORY_SIZE
	if (resetIdleMask==0) {
		unsigned long flags;
		spin_lock_irqsave(&hci->command_port_lock, flags);
		{
			unsigned int elapsed = CheckElapsedTime(hci);
			int i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
			struct history_entry* h = &hci->intHistory[i];
			if (elapsed > ELAPSED_MAX) elapsed = ELAPSED_MAX;
			h->type = HIST_TYPE_BH;
			h->transferState = 0;
			h->uP = 0;
			h->bstat = 0;
			h->pc = NULL;
			h->lr = NULL;
			h->elapsed = elapsed;
			h->fmRemaining = 0;
#ifdef CONFIG_USB_ISP116x_TRAINING
			h->fmRemainingEnd = 0;
#endif
			h->byteCount = 0;
			h->done = 0;
			h->extraFlags = 0;
		}
		spin_unlock_irqrestore(&hci->command_port_lock, flags);
	}
#endif
Loop:
	hci->bhActive = 1;
	wmb();
	if (hci->scanForCancelled) ScanCancelled(hci);
	bScheduleRan=0;
	do {
		hci->intHappened = 0;
		bScheduleRan |= DoItl(hci);
		if (bScheduleRan&0x80) {
			bScheduleRan &= ~0x80;

//			if (hci->atlint || (hci->frames[FRAME_ATL].reqCount==0))
			{
				hci->atlint=0;
				bScheduleRan |= DoProcess(hci, &hci->frames[FRAME_ATL],&hci->sdatl);
			}
		}
		if (!hci->intHappened) //if ((hci->sofint==0) && (hci->atlint==0))
		{
			if (hci->transfer.progress) break;
			if ((CheckClearExtra(hci,NULL) & BSTAT_WORK_PENDING)==0) break;
		}
	} while (1);


	if (bScheduleRan) {
		sh_scan_waiting_intr_list(hci);
	}

	hci->bhActive = 0;		//mark as inactive this early so that interrupt can perform itl processing
					//while in the middle of callback completion routines if necessary
	mb();
	if (hci->intHappened) goto Loop;

#ifdef INT_HISTORY_SIZE
//don't use hci->resetIdleMask because we want to make sure entry and exit entries match
	if (resetIdleMask==0) {
		unsigned long flags;
		spin_lock_irqsave(&hci->command_port_lock, flags);
		{
			int i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
			struct history_entry* h = &hci->intHistory[i];
			h->type = HIST_TYPE_X;
			h->transferState = 0;
			h->uP = 0;
			h->bstat = 0;
			h->pc = NULL;
			h->lr = NULL;
			h->elapsed = 0;
			h->fmRemaining = 0;
#ifdef CONFIG_USB_ISP116x_TRAINING
			h->fmRemainingEnd = 0;
#endif
			h->byteCount = 0;
			h->done = 0;
			h->extraFlags = 0;
		}
		spin_unlock_irqrestore(&hci->command_port_lock, flags);
	}
#endif


	if (!list_empty(&hci->return_list)) {
#if 0
		schedule_task(&hci->return_task);	//make this non-interrupt context so SOF tick won't be missed
#else
		sh_scan_return_list(hci);	//call function directly
#endif
	} else {
		int newWait = 1;
//		if (0)
		if (list_empty(&hci->active_list)) {
			if (list_empty(&hci->waiting_intr_list)) {
				if (!hci->resetIdleMask) {
					if (hci->extraFlags==0) {	//don't disable if waiting for interrupt to finish transfer
						newWait = 0;
						if (hci->disableWaitCnt) {
							//disable SOF interrupts
							int mask = hci->hp.uPinterruptEnable & ~(SOFITLInt|OPR_Reg);
							hc116x_write_reg16(hci, mask, HcuPInterruptEnable);
							hci->hp.uPinterruptEnable = mask;
#ifdef CONFIG_USB_ISP116x_TRAINING
							if (hci->timingsChanged) PrintLineInfo(hci);
#endif
						}
					}
				}
			}
		}
		if (newWait) hci->disableWaitCnt = 5;
	}

	if (hci->resetNow==1) hc_init_regs(hci,0);

#ifdef USE_COMMAND_PORT_RESTORE
	if (cp != CP_INVALID) {
		hci->hp.command_regIndex = cp;				//restore previous state
		ReadReg0 (hci, cp);				//command_regIndex includes the r/w flag
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// In interrupt context.. or interrupts disabled

static inline int doInts(hci_t * hci,int ints)
{
	int left;
	if ((left = ints & ~(OHCI_INTR_SF|OHCI_INTR_FNO|OHCI_INTR_RHSC|OHCI_INTR_ATD))==0) return 0; //what is ATD?
	if (left & OHCI_INTR_SO) {
		dbg("USB Schedule overrun");
		if ((left &= ~OHCI_INTR_SO)==0) return 0;
	}
	if (left & OHCI_INTR_RD) {
		dbg("Resume detected");
		if ((left &= ~OHCI_INTR_RD)==0) return 0;
	}
	printk(KERN_DEBUG __FILE__ ": opr int:%8x,%8x\n",ints,left);
	NoteResetFields(hci,&hci->frames[FRAME_ATL],0x80,hci->bstat,"doInts");
	return 1;
}


static int inline EotLost2(hci_t* hci,struct frameList * fl,int bstat)
{
#if 0
	if (hci->transfer.chain==REQ_CHAIN) {
		if ((bstat & fl->full)==0) return 0;	//chip hasn't realized dmawrite is finished
	} else {
		if (bstat & fl->done) return 0;		//chip hasn't realized that dmaread is finished
	}
#endif
	if ((bstat & fl->full) && !(bstat & fl->done)) {
		NoteResetFields(hci,fl,fl->full,bstat,"EOT lost");
	}
	{
		int cnt = ReadReg16(hci, HcTransferCounter);
		printk(KERN_DEBUG __FILE__ ": Eot2 Transfer should not still be in progress, cnt:%i, port:%x, chain:%i, bstat:%x ts:%x ef:%x\n",
					cnt,fl->port,hci->transfer.chain,bstat,hci->transferState,hci->extraFlags);
	}
	hci->transferState= TF_IDLE;
	hci->extraFlags=0;
	hci->transfer.progress = NULL;
	return 1;
}

static void AbortTransferInProgress(hci_t* hci)
{
	int bstat;
	struct frameList * fl = hci->transfer.fl;
#ifdef USE_DMA
	if (hci->transferState & TF_DmaInProgress) {
		if (hci->hp.dmaChannel>=0) {
			hc_release_dma(&hci->hp);
			printk(KERN_DEBUG __FILE__ ": Dma is not working, turning it off\n");
		}
	}
#endif
	bstat = ReadReg16(hci, HcBufferStatus);
	{
		int cnt = ReadReg16(hci, HcTransferCounter);
		printk(KERN_DEBUG __FILE__ ": Transfer should not still be in progress, cnt:%i, port:%x, chain:%i, bstat:%x ts:%x ef:%x\n",
					cnt,fl->port,hci->transfer.chain,bstat,hci->transferState,hci->extraFlags);
#if 0
		if (hci->hp.dmaport) {
			WriteReg16(hci, 0, HcTransferCounter);	//try to abort transfer
			WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_WRITE|fl->dmac, HcDMAConfiguration);
			WriteReg16(hci, ISP_BURST_CODE|0x04|DMAC_WRITE|fl->dmac, HcDMAConfiguration);
		} else {
			WriteReg16(hci, 2, HcTransferCounter);	//try to resync
			ReadReg16(hci, fl->port);
		}
#endif
	}
	if (fl->state==STATE_SCHEDULE_WORK) {
		if (bstat & fl->done) fl->state = STATE_READ_RESPONSE;
	} else {
		if ((bstat & fl->full)==0) fl->state = STATE_WRITE_REQUEST;
	}

//	if ((bstat & fl->full) && !(bstat & fl->done)) {
//		NoteResetFields(hci,fl,fl->full,bstat,"Transfer timeout");
//	}
	hci->transferState= TF_IDLE;
	hci->extraFlags=0;
	hci->transfer.progress = NULL;
}

static inline int ReadAgainNeeded(hci_t * hci,int bstat,int changebstat)
{
	//atlint happened between the 2 reads, read again
	if (hci->atlint==0) if (bstat & ATLBufferDone) return 1;

	//an EOT has been missed, read HcuPInterrupt again to be sure
	if (hci->transferState & TFM_EotPending)
		if (changebstat || ((hci->transferState & TF_DmaInProgress)==0) ) return 1;

	//since interrupts are set to level triggered but pxa250 is edge triggered
	//I need to make sure SOF didn't happen since I read HcuPInterrupt
	if (hci->sofint==0) if (bstat & (ITL0BufferDone|ITL1BufferDone)) return 1;
	return 0;
}

//interrupts are disabled
static int CheckClearInterrupts(hci_t * hci,struct pt_regs * regs)
{
	int bstat;
	int ret=0;
	int ints_uP = ReadReg16(hci, HcuPInterrupt);
	int sum_ints_uP = ints_uP;
	do {
		if (ints_uP==0) break;
tryagain:
		if (ints_uP & OPR_Reg) {
			int ints = ReadReg32 (hci, HcInterruptStatus);	//do we need to read all 32 bits????, tests say yes
			if (ints) {
				ret = doInts(hci,ints);
				WriteReg32(hci, ints, HcInterruptStatus);	//clear bits, do we need to write all 32 bits???
//				WriteReg32(hci, OHCI_INTR_MIE|OHCI_INTR_SF|OHCI_INTR_SO, HcInterruptEnable); //MIE is bit 31
			}
		}
		WriteReg16(hci, ints_uP, HcuPInterrupt);	//clear bits



		if (hci->transferState & TFM_EotPending) {
			if (ints_uP & AllEOTInterrupt) {
				hci->transferState &= ~TFM_EotPending;
				ret=1;
				hci->lastEOTuP = sum_ints_uP;
			} else {
				if (OffInTheWeeds(hci->transfer.fl,3)) {
					AbortTransferInProgress(hci);
					ret=1;
				}
			}
		} else {
			if (hci->transferState & TF_DmaInProgress) {
				if (OffInTheWeeds(hci->transfer.fl,3)) {
					printk(KERN_DEBUG __FILE__ ": Dma interrupt lost\n");
					hci->transferState &= ~TF_DmaInProgress;
					hci->transfer.progress = NULL;
					ret=1;
				}
			}
		}
		if ((ints_uP & hci->hp.uPinterruptEnable) == 0) break;
		ret=1;


		if (ints_uP & SOFITLInt)
		{
			hci->frame_no = ReadReg16(hci, HcFmNumber); //why read all 32 bits, high 16 bits are reserved
			hci->sofint = 1;
			hci->frames[FRAME_ITL0].sofCnt++;
			hci->frames[FRAME_ITL1].sofCnt++;
			hci->frames[FRAME_ATL].sofCnt++;
			if ((hci->frames[FRAME_ATL].reqWaitForSof==1)|| (hci->frames[FRAME_ATL].sofCnt >=3)) hci->frames[FRAME_ATL].reqWaitForSof=0;
			hci->fmUsingWithItl &= ~FM_SWITCHING;
			if (hci->disableWaitCnt) hci->disableWaitCnt--;
			hci->lastFmRem = 1<<14;
		}
		if (ints_uP & ATLInt) {
			hci->atlint = 1;
			hci->atlPending=0;
			if (hci->frames[FRAME_ATL].reqWaitForSof) hci->frames[FRAME_ATL].reqWaitForSof=1;
		}
	} while (0);

	if (ints_uP)
	{
		int changebstat;
		bstat = ReadReg16(hci, HcBufferStatus);
#ifdef INT_HISTORY_SIZE
		if (hci->resetIdleMask==0) {
//interrupts are already disabled
			unsigned int elapsed = CheckElapsedTime(hci);
			int i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
			struct history_entry* h = &hci->intHistory[i];
			if (elapsed > ELAPSED_MAX) elapsed = ELAPSED_MAX;
			h->type = HIST_TYPE_INTERRUPT;
			h->transferState = 0;
			h->uP = ints_uP;
			h->bstat = bstat;
			h->pc = (regs)? ((void*)(instruction_pointer(regs))) : NULL;
			h->lr = (regs)? ((void*)(get_lr(regs))) : NULL;
			h->elapsed = elapsed;
			h->fmRemaining = 0;
#ifdef CONFIG_USB_ISP116x_TRAINING
			h->fmRemainingEnd = 0;
#endif
			h->byteCount = 0;
			h->extraFlags = 0;
		}
#endif
		changebstat = hci->bstat ^ bstat;
		hci->bstat = bstat;
		if (ReadAgainNeeded(hci,bstat,changebstat)) {
			ints_uP = ReadReg16(hci, HcuPInterrupt);
			if (ints_uP) {
				sum_ints_uP |= ints_uP;
//				if (sum_ints_uP & AllEOTInterrupt) hci->lastEOTuP = sum_ints_uP;
				goto tryagain;
			}
			if (hci->transferState & TFM_EotPending) if ( (hci->transferState & TF_DmaInProgress)==0) {
				ret |= EotLost2(hci,hci->transfer.fl,bstat);
			}
		}
	} else bstat = hci->bstat;



	if (sum_ints_uP & ATLInt) if ((bstat&ATLBufferDone)==0) if (bstat&ATLBufferFull) {
		//atlint happened simultaneously with an itl EOT, leading to a race condition which loses ATLBufferDone
		if (sum_ints_uP & AllEOTInterrupt) { //just a double check as to what happened
			bstat = ReadReg16(hci, HcBufferStatus);	//triple check
			if ((bstat&ATLBufferDone)==0) {
				NoteResetFields(hci,&hci->frames[FRAME_ATL],ATLBufferFull,bstat,"ATLBufferDone lost5");
			}
		} else {
			NoteResetFields(hci,&hci->frames[FRAME_ATL],ATLBufferFull,bstat,"ATLBufferDone lost7 without EOT should not happen");
		}
	}

	if (hci->pending_write_regIndex!=CP_INVALID) {
		WriteReg32(hci, hci->pending_write_val, hci->pending_write_regIndex);
		hci->pending_write_regIndex = CP_INVALID;
	}
	if (hci->pending_read_regIndex!=CP_INVALID) if (hci->pending_read_regIndex!=CP_READ_DONE) {
		hci->pending_read_val = ReadReg32(hci, hci->pending_read_regIndex);
		hci->pending_read_regIndex = CP_READ_DONE;
	}
	if (ret) bstat |= BSTAT_WORK_PENDING;
	return bstat;
}


#ifdef USE_COMMAND_PORT_RESTORE
#define SETUP_READ(hci, regindex,port) ReadReg0_(hci,regindex,port)
#define SETUP_WRITE(hci,regindex,port) WriteReg0_(hci,regindex,port)
#else
#define SETUP_READ(hci, regindex,port) do {} while (0)
#define SETUP_WRITE(hci,regindex,port) do {} while (0)
#endif

static inline int CheckExtra(hci_t * hci)
{
	int extraFlags=hci->extraFlags;
	if (extraFlags) {
		if (extraFlags & EXM_PIO) {
			int regIndex = hci->lastTransferFrame->port;
			int oldReg = hci->hp.cur_regIndex;
			port_t port = hci->hp.hcport;
			if (extraFlags & (EXM_WAITING_SOFINT|EXM_WAITING_ATLINT)) return -1;
			hci->extraFlags = 0;
			if (extraFlags & EXM_READ) {
				int cnt = EXTRA_PIO_RSP_CNT;
				if (regIndex != oldReg) {
#ifndef USE_COMMAND_PORT_RESTORE
					ReadReg0_(hci,regIndex,port);
#endif
					printk(KERN_DEBUG "pio read reg was %x required %x ef:%x ts:%x\n",oldReg,regIndex,extraFlags,hci->transferState);
				}
				SETUP_READ(hci,regIndex,port);
				while (cnt>0) {
					MY_INW(port);
					cnt-=2;
				}
//				printk(KERN_DEBUG "extra read pio\n");
			} else {
				int cnt = EXTRA_PIO_REQ_CNT;
				if ((regIndex|0x80) != oldReg) {
#ifndef USE_COMMAND_PORT_RESTORE
					WriteReg0_(hci,regIndex,port);
#endif
					printk(KERN_DEBUG "pio write reg was %x required %x ef:%x ts:%x\n",oldReg,regIndex|0x80,extraFlags,hci->transferState);
				}
				SETUP_WRITE(hci,regIndex,port);
				while (cnt>0) {
					MY_OUTW(0, port);
					cnt-=2;
				}
//				printk(KERN_DEBUG "extra write pio\n");
			}
			hci->transferState &= ~TFM_ExtraPending;
			hci->lastFmRem = ReadReg16(hci, HcFmRemaining);
		}

#ifdef USE_DMA
		else {
			int rem;
			int ints_uP;
			int bstat;
			if (hci->transferState & TFM_DmaPending) return 0;
			if (extraFlags & EXM_WAITING_ATLINT) return 0;		//can't predict

			rem = ReadReg16(hci, HcFmRemaining);		//can chip handle 16 bit only read????
			if (extraFlags & EXM_WAITING_SOFINT) {
				//return if in danger region
				if ((rem >= hci->extraFmRemainingLow)&&(rem <= hci->extraFmRemainingHigh)) return 0;
				extraFlags &= ~EXM_WAITING_SOFINT;
				hci->lastTransferFrame->extraFmRemaining = rem;
			}
			hci->extraFlags = 0;
			ints_uP = ReadReg16(hci, HcuPInterrupt);
			bstat = ReadReg16(hci, HcBufferStatus);
			{
				int cnt = hci->extraPioCnt;
				if (cnt) {
					port_t port = hci->hp.virtDmaport;
					if (port) {
						if (extraFlags & EXM_READ) {
							do {
								MY_INW(port);
								cnt-=2;
							} while (cnt>0);
//							printk(KERN_DEBUG "extra read dma\n");
						} else {
							do {
								MY_OUTW(0, port);
								cnt-=2;
							} while (cnt>0);
//							printk(KERN_DEBUG "extra write dma\n");
						}
					} else printk(KERN_ERR "No dmaport\n");
				}
			}
			hci->transferState &= ~TFM_ExtraPending;
			hci->lastFmRem = rem;
			if (hci->transferState & TFM_DmaExtraPending) {
				if (hci->transfer.extra==AE_extra_pending) {
					StartDmaExtra(hci,extraFlags & EXM_READ);
#ifdef INT_HISTORY_SIZE
					if (hci->resetIdleMask==0) {
						int i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
						struct history_entry* h = &hci->intHistory[i];
						h->type = HIST_TYPE_EXTRA_DMA;
						h->transferState = 0;
						h->uP = ints_uP;
						h->bstat = bstat;
						h->pc = NULL;
						h->lr = NULL;
						h->elapsed = 0;
						h->fmRemaining = rem;
#ifdef CONFIG_USB_ISP116x_TRAINING
						h->fmRemainingEnd = 0;
#endif
						h->byteCount = (extraFlags & EXM_READ)? EXTRA_DMA_RSP_CNT : EXTRA_DMA_REQ_CNT;
						h->done = 0;
						h->extraFlags = extraFlags;
					}
#endif
				}
			}
		}
#endif
	}
	return 0;
}

static int CheckClearExtra(hci_t * hci,struct pt_regs * regs)
{
	unsigned long flags;
	int i;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	if (hci->transferState & TF_DmaInProgress) {
		if (hci->transfer.progress==NULL) {
			hci->transferState &= ~TFM_DmaPending;
			if (hci->transfer.extra==AE_extra_done) {
				hci->transferState &= ~TFM_DmaExtraPending;
			}
		}
	}
	if (CheckExtra(hci)<0) {
		i = hci->bstat;
	} else {
#ifdef USE_DMA
		if (hci->transferState & TF_DmaInProgress) i = hci->bstat;
		else
#endif
			i = CheckClearInterrupts(hci,regs);
	}
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
	return i;
}

//this routine is also called by the dma completion handler
//that's why it is necessary to have the inInterrupt flag
void hc_interrupt (int irq, void * __hci, struct pt_regs * r)
{
	hci_t * hci = __hci;
	int i;

#ifdef USE_COMMAND_PORT_RESTORE
	__u8 cp = hci->hp.command_regIndex;
	if (cp != CP_INVALID) {
		DO_DELAY(hci);	//I don't know if this is needed, but it may explain weird problems if so
	}
#endif
	if (irq!=0)
		if (hci->extraFlags & (EXM_WAITING_SOFINT|EXM_WAITING_ATLINT))
			hci->extraFlags &= ~(EXM_WAITING_SOFINT|EXM_WAITING_ATLINT);
	hci->intHappened=1;
loop:
	i = CheckClearExtra(hci,r);		//this routine disables interrupts, so it's safe
	if (hci->inInterrupt==0) {
		hci->inInterrupt=1;	//serialize section
		if (hci->bhActive==0) {
			if (hci->resetIdleMask) if (hci->sofint) {
				do {
					if ((i & (ATLBufferFull|ITL0BufferFull|ITL1BufferFull)) & ~hci->resetIdleMask) break;
					if (hci->transferState & TF_TransferInProgress) break;
					if ((i&ATLBufferDone) && (hci->frames[FRAME_ATL].state == STATE_READ_RESPONSE)) break;
					if ((i&hci->frames[FRAME_ITL0].done) && (hci->frames[FRAME_ITL0].state == STATE_READ_RESPONSE)) break;
					if ((i&hci->frames[FRAME_ITL1].done) && (hci->frames[FRAME_ITL1].state == STATE_READ_RESPONSE)) break;
		//Hopefully, resetting on a SOF boundary will cause less USB disruption by having
		//a less weird length frame, also outstanding transactions to buffers that are still working
		//can be cleaned up.
					if (hci->resetNow==0) hci->resetNow = 1;	//next time bottom half starts
				} while (0);
			}
			DoItl(hci);	//ensure that ITL buffers are processed if some other tasklet is being serviced
			if ((hci->transferState & TF_TransferInProgress)==0) tasklet_schedule(&hci->bottomHalf);
		}
		hci->inInterrupt=0;	//serialize section
		if (hci->doubleInt) {
			hci->doubleInt = 0;
			if ((hci->transferState & TF_TransferInProgress)==0) goto loop;
		}
	} else {
		hci->doubleInt=1;
	}
#ifdef USE_COMMAND_PORT_RESTORE
	if (cp != CP_INVALID) {
		hci->hp.command_regIndex = cp;
		ReadReg0 (hci, hci->hp.command_regIndex);	//restore previous state
							//command_regIndex includes the r/w flag
	}
#endif
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*-------------------------------------------------------------------------*
 * HC functions
 *-------------------------------------------------------------------------*/
/* reset the HC */
static int hc_software_reset(hci_t * hci)
{
	hci->chipReady=0;
	/* Disable HC interrupts */
//early documentation says InterruptPinEnable is just an input to an AND gate.
//Later documentation calls InterruptPinEnable a latch enable. I wish the early documentation was right, but the later is right.
	hc116x_write_reg16(hci, 0, HcuPInterruptEnable);
#ifdef CONFIG_CRIND
	hc116x_write_reg16(hci, 0xffff, HcInterruptDisable);	//oldham
#endif
	hc116x_write_reg16(hci,HcHardwareConfiguration_SETTING & ~(SuspendClkNotStop),HcHardwareConfiguration);	//latch the zero
	hc116x_write_reg16(hci,HcHardwareConfiguration_SETTING & ~(SuspendClkNotStop|InterruptPinEnable),HcHardwareConfiguration);	//disable latch
//now ISP116x interrupts can't happen
//and we can use faster versions of functions
	dbg ("USB HC reset_hc usb-: ctrl = 0x%x ;",ReadReg32 (hci, HcControl));

#if 0
	WriteReg0(hci, HcSoftwareReset);	//just selecting this port causes a reset
	udelay (10);		//resets are flakey, maybe this will help
#else
	WriteReg16(hci, 0xf6, HcSoftwareReset);	//0xf6 causes reset, ISP1161a, I don't think this is a problem for earlier versions of silicon
#endif
	return 0;
}

static int hc_reset (hci_t * hci)
{
	int timeout = 30;

	hc_software_reset(hci);
	/* HC Reset requires max 10 us delay */
	WriteReg32(hci, OHCI_HCR, HcCommandStatus);

	do {
		if (--timeout == 0) {
			err ("USB HC reset timed out!");
			return -1;
		}
		udelay (10);
	} while ((ReadReg32 (hci, HcCommandStatus) & OHCI_HCR) != 0);

	WriteReg16(hci,HcHardwareConfiguration_SETTING& ~InterruptPinEnable,HcHardwareConfiguration);
	return 0;
}

/*-------------------------------------------------------------------------*/
/* Start host controller, set the BUS operational
 * enable interrupts
 */
static void ScanCancelled(hci_t * hci)
{
	hci->scanForCancelled = 0;
	RemoveCancelled(hci,&hci->frames[FRAME_ITL0]);
	RemoveCancelled(hci,&hci->frames[FRAME_ITL1]);
	RemoveCancelled(hci,&hci->frames[FRAME_ATL]);
}

#if 1
#define KERN_HISTORY KERN_DEBUG
#else
#define KERN_HISTORY KERN_INFO
#endif

static void PrintFl(struct frameList * fl,struct frameList * fl2,struct frameList * flTransfer)
{
#ifdef USE_FM_REMAINING
	printk(KERN_HISTORY __FILE__ ":%c%c state:%i, prsp:%i, req:%i, rsp:%i full:%x, done:%x, fmRemRsp:%i, fmRemReq:%i ts:%02x\n",
		((fl==flTransfer) ? 'x' : ' '),((fl==fl2) ? '*' : ' '),
		fl->state,fl->prevRspCount,fl->reqCount,fl->rspCount,fl->full,fl->done,
		fl->fmRemainingRsp,fl->fmRemainingReq,fl->transferState);
#else
	printk(KERN_HISTORY __FILE__ ": %c state:%i, prsp:%i, req:%i, rsp:%i full:%x, done:%x\n",
		((fl==fl2) ? '*' : ' '),fl->state,fl->prevRspCount,fl->reqCount,fl->rspCount,fl->full,fl->done);
#endif
}

static void PrintStatusHistory(hci_t * hci, struct frameList * fl, int bstat,const char* message)
{
	printk(KERN_HISTORY __FILE__ ": %s, bstat:%x ts:%x rts:%x ref:%x\n",
		message,bstat,hci->transferState,hci->resetTransferState,hci->resetExtraFlags);
	PrintFl(&hci->frames[FRAME_ITL0],fl,hci->lastTransferFrame);
	PrintFl(&hci->frames[FRAME_ITL1],fl,hci->lastTransferFrame);
	PrintFl(&hci->frames[FRAME_ATL],fl,hci->lastTransferFrame);

#ifdef INT_HISTORY_SIZE
	{
		int i = (hci->intHistoryIndex) & (INT_HISTORY_SIZE-1);
		int last = i;
		int total = 0;
		__u8 bstat = 0;
#ifdef CONFIG_USB_ISP116x_TRAINING
		printk(KERN_HISTORY "TYPE elaps total       pc       lr uP bs remain remEnd dma rsp buff  cnt ef\n");
#else
		printk(KERN_HISTORY "TYPE elaps total       pc       lr uP bs remain dma rsp buff  cnt ef\n");
#endif
		do {
			struct history_entry* e = &hci->intHistory[i];
			if (e->uP & 1) printk(KERN_HISTORY "\n");

			if (e->type == HIST_TYPE_BH)             printk(KERN_HISTORY "  BH:");
			else if (e->type == HIST_TYPE_X)         printk(KERN_HISTORY "   X:");
			else if (e->type == HIST_TYPE_INTERRUPT) printk(KERN_HISTORY " Int:");
			else if (e->type == HIST_TYPE_TRANSFER)  printk(KERN_HISTORY " xfr:");
			else if (e->type == HIST_TYPE_EXTRA_DMA) printk(KERN_HISTORY "edma:");
			else break;
			if (e->elapsed) {
				if (e->elapsed==ELAPSED_MAX) printk(" -overflow- ");
				else {
					total+=e->elapsed;
					printk("%5i%6i ",e->elapsed,total);
					if (e->uP & 1) total=0;
				}
			}
			else printk("            ");

			if (e->pc) printk("%08x ",(unsigned int)e->pc);
			else printk("         ");

			if (e->lr) printk("%08x ",(unsigned int)e->lr);
			else printk("         ");

			if (e->uP) printk("%02x ",e->uP);
			else printk("   ");

			if (e->bstat) printk("%02x ",e->bstat);
			else printk("   ");

			if (e->fmRemaining) printk("%6i ",e->fmRemaining);
			else {
				if (e->uP & 4) {
					bstat ^= e->bstat;
					if (bstat & ITL0BufferFull) if (e->bstat & ITL0BufferFull) printk("itl0W ");
					if (bstat & ITL1BufferFull) if (e->bstat & ITL1BufferFull) printk("itl1W ");
					if (bstat & ATLBufferFull) if (e->bstat & ATLBufferFull) printk("atlW  ");
					if (bstat & ITL0BufferDone) if ((e->bstat & ITL0BufferDone)==0) printk("itl0R ");
					if (bstat & ITL1BufferDone) if ((e->bstat & ITL1BufferDone)==0) printk("itl1R ");
					if (bstat & ATLBufferDone) if ((e->bstat & ATLBufferDone)==0) printk("atlR ");
				} else printk("      ");
			}

#ifdef CONFIG_USB_ISP116x_TRAINING
			if (e->fmRemainingEnd) printk("%6i ",e->fmRemainingEnd);
			else printk("       ");
#endif

			if (e->type == HIST_TYPE_TRANSFER) {
                 printk( (e->transferState & TFM_DmaPending) ? "dma " : "pio ");
                 printk( (e->transferState & TFM_RSP) ? "rsp " : "req ");
				 printk( (e->done==ATLBufferDone) ? "atl " :
				         ((e->done==ITL0BufferDone) ? "itl0" :
 				         ((e->done==ITL1BufferDone) ? "itl1" : "itlx")) );

			} else printk("    ");

			if (e->byteCount) printk("%5i",e->byteCount);
			if (e->extraFlags) printk("%3x",e->extraFlags);
			printk("\n");

			if (e->type == HIST_TYPE_INTERRUPT) bstat = e->bstat;
			i = (i+1) & (INT_HISTORY_SIZE-1);
		} while (i != last);
	}
#endif
}

#ifdef USE_DMA
#define PADDING (EXTRA_DMA_RSP_CNT+BURST_TRANSFER_SIZE-2)
#else
#define PADDING (EXTRA_PIO_RSP_CNT)
#endif

#define ROUND_UP(x) (((x)+63)&~63)
static int hc_init_regs(hci_t * hci,int bUsbReset)
{
	__u32 mask;
	unsigned int fminterval = (INIT_fmLargestBitCnt << 16) | FI_BITS;
	int i,j;

	hci->resetNow = 2;
	hci->pending_write_regIndex = CP_INVALID;
	hci->pending_read_regIndex = CP_INVALID;
	i = (bUsbReset) ? hc_reset(hci) : hc_software_reset(hci);
	printk(KERN_DEBUG __FILE__ ": reset device\n");
	if (hci->resetReason) {
		PrintStatusHistory(hci,hci->reset_fl,hci->resetBstat,hci->resetReason);
		hci->resetReason = NULL;
	}
	if ( i < 0) {
		err ("reset failed");
		return -ENODEV;
	}
	i = ROUND_UP(ITL_BUFFER_SIZE+PADDING);	//itl buffer length
	j = 4096 - (i<<1); //atl buffer length

	if (j>ROUND_UP(MAX_ATL_BUFFER_SIZE+PADDING)) j = ROUND_UP(MAX_ATL_BUFFER_SIZE+PADDING);

	WriteReg16(hci, i, HcITLBufferLength);
	WriteReg16(hci, j, HcATLBufferLength);

	hci->errorCnt=0;
	WriteReg16(hci, 0, HcDMAConfiguration);

	hci->bstat=0;
	hci->fmUsingWithItl=0;
	WriteReg32(hci, fminterval, HcFmInterval);

#define LS_BITS 1576	//0x628, low speed comparison of remaining to start packet
	WriteReg32(hci, LS_BITS, HcLSThreshold);

	if (bUsbReset) {
		// move from suspend to reset
		WriteReg32(hci, OHCI_USB_RESET, HcControl);
		wait_ms (1000);
#ifdef CONFIG_CRIND
		//after reset, resume until setting back to operational
		WriteReg32(hci, OHCI_USB_RESUME, HcControl);	//Oldham
#endif
	}
#ifdef CONFIG_CRIND
//oldham
	else {
 		// If a software reset, set back to operational
		WriteReg32(hci, OHCI_USB_OPER, HcControl);
	}
#else

	WriteReg32(hci, OHCI_USB_OPER, HcControl);
#endif

	/* Choose the interrupts we care about now, others later on demand */
	mask = OHCI_INTR_MIE |
//	OHCI_INTR_ATD |
	OHCI_INTR_SO |
	OHCI_INTR_SF;

	WriteReg32(hci, mask, HcInterruptEnable);
	WriteReg32(hci, mask, HcInterruptStatus);

#ifdef	CONFIG_USB_ISP116x_NPS
	WriteReg32(hci, (ReadReg32 (hci, HcRhDescriptorA) | RH_A_NPS) & ~RH_A_PSM,
		HcRhDescriptorA);
	WriteReg32(hci, RH_HS_LPSC, HcRhStatus);
#endif

	WriteReg16(hci, hci->frame_no, HcFmNumber);

	hci->frames[FRAME_ITL0].active=0;
	hci->frames[FRAME_ITL0].state = STATE_SCHEDULE_WORK;
	hci->frames[FRAME_ITL0].full = ITL0BufferFull|ITL1BufferFull;
	hci->frames[FRAME_ITL0].done = ITL0BufferDone|ITL1BufferDone;
	hci->frames[FRAME_ITL0].port = HcITLBufferPort;
	hci->frames[FRAME_ITL0].dmac = DMAC_ITL;
	hci->frames[FRAME_ITL0].iso_partner = &hci->frames[FRAME_ITL1];

	hci->frames[FRAME_ITL1].active=0;
	hci->frames[FRAME_ITL1].state = STATE_SCHEDULE_WORK;
	hci->frames[FRAME_ITL1].full = ITL0BufferFull|ITL1BufferFull;
	hci->frames[FRAME_ITL1].done = ITL0BufferDone|ITL1BufferDone;
	hci->frames[FRAME_ITL1].port = HcITLBufferPort;
	hci->frames[FRAME_ITL1].dmac = DMAC_ITL;
	hci->frames[FRAME_ITL1].iso_partner = &hci->frames[FRAME_ITL0];

	hci->frames[FRAME_ATL].active=0;
	hci->frames[FRAME_ATL].state = STATE_SCHEDULE_WORK;
	hci->frames[FRAME_ATL].full = ATLBufferFull;
	hci->frames[FRAME_ATL].done = ATLBufferDone;
	hci->frames[FRAME_ATL].port = HcATLBufferPort;
	hci->frames[FRAME_ATL].dmac = DMAC_ATL;

	hci->frames[FRAME_ITL1].limit = hci->frames[FRAME_ITL0].limit = i-PADDING;
	hci->frames[FRAME_ATL].limit = j-PADDING;

	hci->transfer.progress = NULL;
	hci->transferState= TF_IDLE;
	ScanCancelled(hci);
	hci->frames[FRAME_ITL0].reqCount = 0;
	hci->frames[FRAME_ITL0].rspCount = 0;
	hci->frames[FRAME_ITL1].reqCount = 0;
	hci->frames[FRAME_ITL1].rspCount = 0;
	hci->frames[FRAME_ATL].reqCount = 0;
	hci->frames[FRAME_ATL].rspCount = 0;
	hci->frames[FRAME_ITL0].reqWaitForSof = 0;
	hci->frames[FRAME_ITL1].reqWaitForSof = 0;
	hci->frames[FRAME_ATL].reqWaitForSof = 0;

	hci->atlPending=0;
	hci->extraFlags=0;

	sh_scan_waiting_intr_list(hci);
	sh_scan_return_list(hci);

	if (hci->timingsChanged) printk(KERN_DEBUG __FILE__ ": timings changed\n");
	PrintLineInfo(hci);

	hci->resetIdleMask = 0;
	hci->resetNow = 0;
	hci->chipReady=1;

	hci->hp.uPinterruptEnable=0;
	hc116x_enable_sofint(hci);

#ifdef CONFIG_CRIND
// Tell the Root Hub that everything is Okay
	WriteReg32(hci, RH_PS_CSC|RH_PS_PES, HcRhPortStatus); // oldham
#endif
	WriteReg16(hci, HcHardwareConfiguration_SETTING,HcHardwareConfiguration);	//enable interrupt latch
#ifdef CONFIG_CRIND
	if (bUsbReset) WriteReg32(hci, OHCI_USB_OPER, HcControl);	//oldham
#endif
	return 0;
}

void hc116x_enable_sofint(hci_t* hci)
{
	__u32 mask = hci->hp.uPinterruptEnable;
	if ((mask & SOFITLInt)==0) {
		unsigned long flags;
#if 0
		mask |= SOFITLInt | ATLInt | OPR_Reg;	// | AllEOTInterrupt;
#else
//OPR_Reg happens with SOFint usually, but occasionally SOFint happens 1st.
//With level triggered interrupts the edge triggered pxa255 doesn't see see the separate OPR_Reg edge, and interrupts are lost.
//Easiest solution is to not use OPR_Reg interrupts.
		mask |= SOFITLInt | ATLInt |  AllEOTInterrupt; //this is slower, but needed for itl/atl combined
#endif
		spin_lock_irqsave(&hci->command_port_lock, flags);	//spin needed, process may be preempted
		{
			int ints_uP = ReadReg16(hci, HcuPInterrupt);
			if (ints_uP & OPR_Reg) {
				int ints = ReadReg32 (hci, HcInterruptStatus);	//do we need to read all 32 bits????, tests say yes
				if (ints) {
					doInts(hci,ints);
					WriteReg32(hci, ints, HcInterruptStatus);	//clear bits, do we need to write all 32 bits???
//					WriteReg32(hci, OHCI_INTR_MIE|OHCI_INTR_SF|OHCI_INTR_SO, HcInterruptEnable); //MIE is bit 31
				}
			}
			WriteReg16(hci, ints_uP, HcuPInterrupt);	//clear bits to ensure nothing is pending
			WriteReg16(hci, mask, HcuPInterruptEnable);
			hci->hp.uPinterruptEnable = mask;
			hci->disableWaitCnt = 5;
		}
		spin_unlock_irqrestore(&hci->command_port_lock, flags);
		if ((hci->transferState & TF_TransferInProgress)==0) tasklet_schedule(&hci->bottomHalf);
	}
}
/*-------------------------------------------------------------------------*/
#ifdef USE_FM_REMAINING
static void CalcLine(struct timing_line* t,int roundUp)
{
	int x = t->x2 - t->x1;
	int y = t->y2 - t->y1;
	int slopeMax=0; int slopeMin = 0;
	if (x) {
		y <<= 16;
		slopeMin =  y/x;
		slopeMax = slopeMin;
		if ((slopeMin * x) < y) slopeMax++;
	}
	if (roundUp) {
		t->slope = slopeMax;
		t->b = (t->y1<<16) - (slopeMin * t->x1);
	} else {
		t->slope = slopeMin;
		t->b = (t->y1<<16) - (slopeMax * t->x1);
	}

}
static void PrintLine(struct timing_line* t,int roundUp,int transferState)
{
	int b = (int)t->b;
	printk(KERN_HISTORY "%s %s %s line:(%i,%i)-(%i,%i), slope:%i.%06i,b:%i.%06i\n",
		((transferState & TFM_DmaPending)?"dma":"pio"),
		((transferState & TFM_RSP)?"rsp":"req"),
		((roundUp)?"high":"low"),
		t->x1,t->y1,t->x2,t->y2,
		t->slope>>16,((t->slope & 0xffff)*(1000000>>6))>>(16-6),
		( (b>=0)? b >>16 : -((-b)>>16)),
		(( ((b>=0)? b :-b)    & 0xffff)*(1000000>>6))>>(16-6)
		);
}

static void initLine(struct timing_line* t,
	int x1,int y1,int x2,int y2,int roundUp)
{
	if ((x1 > x2) || ((x1==x2) && (y1<y2))) {
		int tempx = x1; int tempy = y1;
		x1 = x2;	y1 = y2;
		x2 = tempx;	y2 = tempy;
	}
	if (y1 >= y2) {
		if (roundUp) {x2 = x1; y2 = y1; x1 = 0;}
		else {x1 = 0; y1 = y2;}
	}
	t->x1  = (__u16)x1;
	t->y1  = (__u16)y1;
	t->x2  = (__u16)x2;
	t->y2  = (__u16)y2;
	CalcLine(t,roundUp);
}
static void IncludePoint(struct timing_line* t,int x, int y,int roundUp)
{
	int x1,y1;
	int w1,w2;
	if (t->x1) {
		//choose point which gives maximum width line to keep
		if (y < t->y1) w1 = t->x1 - x;
		else  w1 = x - t->x1;

		if (y < t->y2) w2 = t->x2 - x;
		else  w2 = x - t->x2;
	} else {
		w1 = 0;
		w2 = 1;
	}
	if (w2 > w1) {
		x1 = t->x2;
		y1 = t->y2;
	} else {
		x1 = t->x1;
		y1 = t->y1;
	}
	initLine(t, x1, y1, x, y,roundUp);
}

void TestPoint(int x, int y, struct timing_lines* t,hci_t * hci)
{
		int max_v = MAX_SLOPE * x + MAX_B;
		int min_v = MIN_SLOPE * x + MIN_B;
		if (((y<<16) > max_v) || ((y<<16) < min_v) || (x==0) || (y>FI_BITS)) return;

		if ((t->high.x1==0) || ((y<<16) > ((t->high.slope*x)+t->high.b))) {
			IncludePoint(&t->high,x,y,1);
			hci->timingsChanged = 1;
		}
		if ((t->low.x1==0) || ((y<<16) < ((t->low.slope*x)+t->low.b))) {
			IncludePoint(&t->low,x,y,0);
			hci->timingsChanged = 1;
		}
}

#ifdef CONFIG_USB_ISP116x_TRAINING
void TestPoint1(int x, int y, struct timing_lines* t,hci_t * hci)
{
		int max_v = MAX_SLOPE * x + MAX_B;
		int min_v = MIN_SLOPE * x + MIN_B;
		if (((y<<16) > max_v) || ((y<<16) < min_v) || (x==0) || (y>FI_BITS)) return;

		if ((t->high.x1==0) || ((y<<16) > ((t->high.slope*x)+t->high.b))) {
			IncludePoint(&t->high,x,y,1);
			hci->timingsChanged = 1;
		}
		if (y > 36) y -= 36;	//Eot update can happen this much early
		if ((t->low.x1==0) || ((y<<16) < ((t->low.slope*x)+t->low.b))) {
			IncludePoint(&t->low,x,y,0);
			hci->timingsChanged = 1;
		}
}
#endif

static void NewPoint(hci_t * hci,struct frameList * fl)
{
	struct timing_lines* t;
	int x;
	int y;

	if (fl) {
#ifdef USE_DMA
		if (fl->transferState & TFM_ExtraPending) {
			if (fl->extraFmRemaining) if (fl->transferState & TFM_DmaExtraPending) {
				if (hci->extraFmRemainingLow > fl->extraFmRemaining) {
					hci->extraFmRemainingLow = fl->extraFmRemaining;
					hci->timingsChanged = 1;
				}
				if (hci->extraFmRemainingHigh < fl->extraFmRemaining) {
					hci->extraFmRemainingHigh = fl->extraFmRemaining;
					hci->timingsChanged = 1;
				}
			}
			return;
		} else
#endif
		if (fl->transferState & TFM_RSP) {
			//after response reception is started, the count is moved to prevRspCount
			//and rspCount is zeroed
			x = fl->prevRspCount;
			y = fl->fmRemainingRsp;
#ifdef USE_DMA
			t = (fl->transferState & TFM_DmaPending) ? &hci->dmaLinesRsp : &hci->pioLinesRsp;
#else
			t = &hci->pioLinesRsp;
#endif
		} else {
			x = fl->reqCount;
			y = fl->fmRemainingReq;
#ifdef USE_DMA
			t = (fl->transferState & TFM_DmaPending) ? &hci->dmaLinesReq : &hci->pioLinesReq;
#else
			t = &hci->pioLinesReq;
#endif
		}
		TestPoint(x,y,t,hci);
	}
}

static void PrintLines(struct timing_lines* t,int transferState)
{
	PrintLine(&t->low,0,transferState);
	PrintLine(&t->high,1,transferState);
}

static void PrintLineInfo(hci_t * hci)
{
#ifdef USE_DMA
 	printk(KERN_HISTORY __FILE__ ": extraFm low:%i extraFm high:%i fmLargestBitCntWithItl:%i\n",
			hci->extraFmRemainingLow,hci->extraFmRemainingHigh,hci->fmLargestBitCntWithItl);

	PrintLines(&hci->dmaLinesReq,TFM_DmaPending);
	PrintLines(&hci->dmaLinesRsp,TFM_DmaPending|TFM_RSP);
#else
 	printk(KERN_HISTORY __FILE__ ": fmLargestBitCntWithItl:%i\n",
			hci->fmLargestBitCntWithItl);
#endif
	PrintLines(&hci->pioLinesReq,0);
	PrintLines(&hci->pioLinesRsp,TFM_RSP);
	hci->timingsChanged = 0;
}

static inline void initLines(struct timing_lines* t,
	int lowX1,int lowY1,int lowX2,int lowY2,
	int highX1,int highY1,int highX2,int highY2,int transferState)
{
	initLine(&t->low, lowX1, lowY1, lowX2, lowY2,0);
	initLine(&t->high,highX1,highY1,highX2,highY2,1);
//	PrintLine(&t->low,0,transferState);
//	PrintLine(&t->high,1,transferState);
}
#endif

/* allocate HCI */
static hci_t * __devinit hc_alloc_hci (void)
{
	hci_t * hci;
	hcipriv_t * hp;

	struct usb_bus * bus;

	hci = (hci_t *) kmalloc (sizeof (hci_t), GFP_KERNEL);
	if (!hci)
		return NULL;

	memset (hci, 0, sizeof (hci_t));
	hp = &hci->hp;

	hp->irq = -1;

#ifdef USE_DMA
	hp->dmaChannel = NODMA;
#endif

	INIT_LIST_HEAD(&hci->hci_hcd_list);
	list_add (&hci->hci_hcd_list, &hci_hcd_list);

	INIT_LIST_HEAD(&hci->active_list);
	INIT_LIST_HEAD(&hci->waiting_intr_list);
	INIT_LIST_HEAD(&hci->return_list);

	bus = usb_alloc_bus (&hci_device_operations);
	if (!bus) {
		kfree (hci);
		return NULL;
	}

	hci->bus = bus;
	bus->hcpriv = (void *) hci;
	hci->bottomHalf.func = hc_interrupt_bh;
	hci->bottomHalf.data = (unsigned long)hci;
	hci->return_task.routine = sh_scan_return_list;
	hci->return_task.data = hci;
#ifdef USE_COMMAND_PORT_RESTORE
	hci->hp.command_regIndex = CP_INVALID;
#endif
	hci->hp.cur_regIndex = CP_INVALID;
	hci->itl_index = FRAME_ITL0;

#ifdef USE_FM_REMAINING
#ifdef USE_DMA
	initLines(&hci->dmaLinesReq,
		  DMA_LOW_X1_REQ, DMA_LOW_Y1_REQ, DMA_LOW_X2_REQ, DMA_LOW_Y2_REQ,
		  DMA_HIGH_X1_REQ,DMA_HIGH_Y1_REQ,DMA_HIGH_X2_REQ,DMA_HIGH_Y2_REQ,TFM_DmaPending);
	initLines(&hci->dmaLinesRsp,
		  DMA_LOW_X1_RSP, DMA_LOW_Y1_RSP, DMA_LOW_X2_RSP, DMA_LOW_Y2_RSP,
		  DMA_HIGH_X1_RSP,DMA_HIGH_Y1_RSP,DMA_HIGH_X2_RSP,DMA_HIGH_Y2_RSP,TFM_DmaPending|TFM_RSP);
#endif
	initLines(&hci->pioLinesReq,
		  PIO_LOW_X1_REQ, PIO_LOW_Y1_REQ, PIO_LOW_X2_REQ, PIO_LOW_Y2_REQ,
		  PIO_HIGH_X1_REQ,PIO_HIGH_Y1_REQ,PIO_HIGH_X2_REQ,PIO_HIGH_Y2_REQ,0);
	initLines(&hci->pioLinesRsp,
		  PIO_LOW_X1_RSP, PIO_LOW_Y1_RSP, PIO_LOW_X2_RSP, PIO_LOW_Y2_RSP,
		  PIO_HIGH_X1_RSP,PIO_HIGH_Y1_RSP,PIO_HIGH_X2_RSP,PIO_HIGH_Y2_RSP,TFM_RSP);
#endif

#ifdef USE_DMA
	hci->extraFmRemainingLow = DMA_extraFmRemainingLow;
	hci->extraFmRemainingHigh = DMA_extraFmRemainingHigh;
#endif

	hci->timingsChanged = 1;
	hci->fmLargestBitCntWithItl = INIT_fmLargestBitCntWithItl;
	return hci;
}

/*-------------------------------------------------------------------------*/
static void unMapPhys(int type, port_t* base)
{
	if ((type==PORT_TYPE_PHYS)&&(*base)) {
		iounmap((char *)(*base));
		*base = 0;
	}
}

#ifdef USE_DMA
/* De-allocate all resources.. */
static void hc_release_dma(hcipriv_t * hp)
{
	if (hp->dmaChannel>=0) {
		FreeDmaChannel(hp->dmaChannel);
		hp->dmaChannel = NODMA;
	}
	if (hp->dmaport) {
		RELEASE_REGION(hp->dmaport, 2);
		hp->dmaport = 0;
	}
	unMapPhys(PORT_TYPE_PHYS,&hp->virtDmaport);
}
#endif

static void hc_release_hci (hci_t * hci)
{
	hcipriv_t * hp = &hci->hp;
	dbg ("USB HC release hci %d,%d", hci->frames[FRAME_ATL].reqCount,hci->frames[FRAME_ATL].rspCount);

	/* disconnect all devices */
	if (hci->bus->root_hub)
		usb_disconnect (&hci->bus->root_hub);

	if (hp->hc) {
		if (hp->found) hc_reset (hci);
		RELEASE_REGION(hp->hc, 4);
		hp->hc = 0;
	}

	unMapPhys(hp->hcType,&hp->hcport);

#ifdef USE_MEM_FENCE
	if (hp->memFenceType <= PORT_TYPE_VIRT) {
		if (hp->memFence) {
			RELEASE_REGION(hp->memFence, 4);
			hp->memFence = NULL;
		}
		unMapPhys(hp->memFenceType,&hp->memFencePort);
	} else if (hp->memFenceType == PORT_TYPE_MEMORY) {
		struct dmaWork* dw = (struct dmaWork*)hci->hp.memFencePort;
		hci->hp.memFencePort = NULL;
		if (dw) FreeDmaWork(hci,dw);
	}
#endif

#ifdef USE_WU
	if (hp->wu) {
		RELEASE_REGION(hp->wu, 2);
		hp->wu = 0;
	}
	unMapPhys(hp->wuType,&hp->wuport);
#endif

	if (hp->irq >= 0) {
		free_irq (hp->irq, hci);
		hp->irq = -1;
	}

#ifdef USE_DMA
	unMapPhys(PORT_TYPE_PHYS,&hp->virtDmaport);
	hc_release_dma(hp);
#endif

	if (hci->bus) {
		usb_deregister_bus(hci->bus);
		usb_free_bus (hci->bus);
	}

	list_del (&hci->hci_hcd_list);
	INIT_LIST_HEAD (&hci->hci_hcd_list);
	{
		struct dmaWork* dw = hci->free.chain;
		hci->free.chain = NULL;
		ReleaseDmaWork(dw);
	}

	kfree (hci);
}

/*-------------------------------------------------------------------------*/
static port_t MapPhys(int type, port_t base)
{
	port_t ret=base;
	if (type==PORT_TYPE_PHYS) {
		ret = (port_t) IOREMAP(base,4);
	}
	return ret;
}

static int __devinit hc_found_hci (int hcType,port_t hc,int irq
#ifdef USE_MEM_FENCE
		,int memFenceType, port_t memFence, int memCnt
#endif
#ifdef USE_WU
		,int wuType, port_t wu
#endif
#ifdef USE_DMA
		,dma_addr_t dmaaddr, int dma
#endif
		 )
{
	hci_t * hci;
	hcipriv_t * hp;
	int val;

	printk(KERN_INFO __FILE__ ": USB starting\n");
	dbg("dbg on");

	hci = hc_alloc_hci ();
	if (!hci) {
		return -ENOMEM;
	}

	hp = &hci->hp;
	hp->hcType = hcType;
#ifdef USE_MEM_FENCE
	hp->memFenceType = memFenceType;
	hp->memCnt = memCnt;
#endif
#ifdef USE_WU
	hp->wuType = wuType;
#endif

#ifdef USE_MEM_FENCE
	if (memFenceType <= PORT_TYPE_VIRT) {
		if (!REQUEST_REGION(memFence, 4, "ISP116x memFence")) {
			err ("fence request address 0x%p-0x%p failed", memFence, memFence+3);
			hc_release_hci (hci);
			return -EBUSY;
		}
		hp->memFence = memFence;
		hp->memFencePort = MapPhys(memFenceType,memFence);
	} else if (memFenceType == PORT_TYPE_MEMORY) {
		hp->memFencePort = (port_t)AllocDmaWork(hci);
	}

#ifdef USE_LOAD_SELECTABLE_DELAY
	if (memFenceType == PORT_TYPE_UDELAY) hp->delay = delayUDELAY;
	else if (memFenceType != PORT_TYPE_NONE) {
		if (memCnt==1) hci->hp.delay = delay1;
		else if (memCnt==2) hci->hp.delay = delay2;
		else if (memCnt==3) hci->hp.delay = delay3;
		else if (memCnt>3) hci->hp.delay = delayN;
	}
#endif
#endif

	if (!REQUEST_REGION(hc, 4, "ISP116x USB HOST")) {
		err ("hc request address 0x%p-0x%p failed", hc, hc+3);
		hc_release_hci (hci);
		return -EBUSY;
	}

	initMemMap();		//we have now been granted permission to access the memory, so perform any special mapping
	hp->hc = hc;
	hp->hcport = MapPhys(hcType,hc);

#ifdef CONFIG_USB_ISP116x_EXTERNAL_OC
	printk("External OverCurrent Enabled\n");
#endif
	
	
	outw (0x28|0x80, hci->hp.hcport + 4);
//	if (hci->hp.delay) hci->hp.delay(hci);
	udelay(20);
	outw (0xAA55, hci->hp.hcport);
	udelay(20);
	outw (0x28, hci->hp.hcport + 4);
//	if (hci->hp.delay) hci->hp.delay(hci);
	udelay(20);
	val=inw(hci->hp.hcport);
	printk("ScratchReg=%x\n",val);//!!!raf

	if ( val != 0xAA55) {
		hc_release_hci (hci);
		err ("Re-read Scratch faild  expected:0xAA55, read:0x%4.4x",val);
		return -ENODEV;
	}

	val = hc116x_read_reg16(hci, HcChipID);
	if (val==0) {
		hc_reset(hci);
		val = ReadReg16(hci, HcChipID);
	}
	if (( val & 0xff00) != 0x6100) {
		hc_release_hci (hci);
		err ("ChipID failed  expected:0x61nn, read:0x%4.4x",val);
		return -ENODEV;
	}

#ifdef USE_WU
	if (wu) {
		if (!REQUEST_REGION(wu, 2, "ISP116x USB Wake up")) {
			err ("wu request address 0x%x-0x%x failed", wu, wu+1);
			hc_release_hci (hci);
			return -EBUSY;
		}
	}
	hp->wu = wu;
	hp->wuport = MapPhys(wuType,wu);
#endif

#ifdef USE_DMA
	if ( (dma != ALLOC_DMA)&&((unsigned)dma >= MAX_DMA_CHANNELS) ) dmaaddr = 0;
	if (dmaaddr) {
		if (!REQUEST_REGION(dmaaddr, 2, "ISP116x USB HOST DMA")) {
			err ("dma request address 0x%x-0x%x failed", dmaaddr, dmaaddr+1);
			hc_release_hci (hci);
			return -EBUSY;
		}
	} else dma=NODMA;
	hp->dmaport = dmaaddr;
	hp->virtDmaport = MapPhys(PORT_TYPE_PHYS,(port_t)dmaaddr);
#endif

	printk(KERN_INFO __FILE__ ": USB ISP116x at %p"
#ifdef USE_WU
	 "/%p"
#endif
	 " IRQ %d Rev. %x ChipID: %x\n",
		hc,
#ifdef USE_WU
		wu,
#endif
		irq, hc116x_read_reg32(hci, HcRevision), hc116x_read_reg16(hci, HcChipID));
	usb_register_bus (hci->bus);

	initIrqLine();

	/* gpio36. i.e irq 59*/
	printk(KERN_INFO __FILE__ "USB host interrupt: rising edge. Gpio 36\n");
	set_GPIO_IRQ_edge(36, GPIO_RISING_EDGE);

	if (REQUEST_IRQ (irq, hc_interrupt, SA_INTERRUPT, "ISP116x", hci) != 0) {
		err ("request interrupt %d failed", irq);
		hc_release_hci (hci);
		return -EBUSY;
	}
	hp->irq = irq;

#ifdef USE_DMA
	if (dma!=NODMA) {
		int dmaCh = GetDmaChannel(hci, dma, "ISP116x USB HOST");
		if (dmaCh<0) {
			err ("request dma %d failed", dma);
			hc_release_dma(hp);
		}
		dma = dmaCh;
	}
	hp->dmaChannel = dma;
#endif

	hp->found=1;
	if (hc_init_regs(hci,1) < 0) {
		err ("can't start usb-%p", hc);
		hc_release_hci (hci);
		return -EBUSY;
	}

	// POTPGT delay is bits 24-31, in 2 ms units.
	mdelay ((hc116x_read_reg32(hci, HcRhDescriptorA) >> 23) & 0x1fe);

	rh_connect_rh (hci);

#ifdef	DEBUG
//	hci_dump (hci, 1);
#endif
	return 0;
}


/*-------------------------------------------------------------------------*/
static int hcType = DEFAULT_HC_TYPE;
static port_t hc = (port_t)DEFAULT_HC_BASE;

#ifdef USE_MEM_FENCE
static int memFenceType = DEFAULT_MEM_FENCE_TYPE;
static port_t memFence = (port_t)DEFAULT_MEM_FENCE_BASE;
static int memCnt = DEFAULT_MEM_FENCE_ACCESS_COUNT;
#endif

#ifdef USE_WU
static int wuType = DEFAULT_WU_TYPE;
static port_t wu = (port_t)DEFAULT_WU_BASE;
#endif
static int irq = DEFAULT_IRQ;

#ifdef USE_DMA
static dma_addr_t dmaport = DEFAULT_DMA_BASE;
static int dma = DEFAULT_DMA;
#endif

#ifndef CONFIG_CRIND
MODULE_PARM(hcType,"i");
MODULE_PARM_DESC(hcType,"(0-io, 1-phys, 2-virt) " __STR__(DEFAULT_HC_TYPE));
MODULE_PARM(hc,"i");
MODULE_PARM_DESC(hc,"ISP116x PORT " __STR__(DEFAULT_HC_BASE));
#endif

#ifdef USE_MEM_FENCE
MODULE_PARM(memFenceType,"i");
MODULE_PARM_DESC(memFenceType,"(0-io, 1-phys, 2-virt, 3-none, 4-memory, 5-udelay) " __STR__(DEFAULT_MEM_FENCE_TYPE));
MODULE_PARM(memFence,"i");
MODULE_PARM_DESC(memFence,"Fence " __STR__(DEFAULT_MEM_FENCE_BASE));
MODULE_PARM(memCnt,"i");
MODULE_PARM_DESC(memCnt,"Fence count " __STR__(DEFAULT_MEM_FENCE_ACCESS_COUNT));
#endif

#ifdef USE_WU
MODULE_PARM(wuType,"i");
MODULE_PARM_DESC(wuType,"(0-io, 1-phys, 2-virt, 3-none) " __STR__(DEFAULT_WU_TYPE));
MODULE_PARM(wu,"i");
MODULE_PARM_DESC(wu,"WAKEUP PORT " __STR__(DEFAULT_WU_BASE));
#endif

MODULE_PARM(irq,"i");
MODULE_PARM_DESC(irq,"IRQ " __STR__(DEFAULT_IRQ));

#ifdef USE_DMA
MODULE_PARM(dmaport,"i");
MODULE_PARM_DESC(dmaport,"DMA physical port " __STR__(DEFAULT_DMA_BASE));

MODULE_PARM(dma,"i");
MODULE_PARM_DESC(dma,"DMA channel -2(ALLOC), -1(NONE), 0-15: " STR_DEFAULT_DMA "(default)");
#endif

static int __init hci_hcd_init (void)
{
	int ret;

	ret = hc_found_hci (hcType,hc,irq
#ifdef USE_MEM_FENCE
			,memFenceType,memFence,memCnt
#endif
#ifdef USE_WU
			,wuType,wu
#endif
#ifdef USE_DMA
			,dmaport, dma
#endif
			);
	return ret;
}

/*-------------------------------------------------------------------------*/

static void __exit hci_hcd_cleanup (void)
{
	struct list_head *  hci_l;
	hci_t * hci;
	for (hci_l = hci_hcd_list.next; hci_l != &hci_hcd_list;) {
		hci = list_entry (hci_l, hci_t, hci_hcd_list);
		hci_l = hci_l->next;
		hc_release_hci(hci);
	}
}

module_init (hci_hcd_init);
module_exit (hci_hcd_cleanup);

MODULE_AUTHOR ("Roman Weissgaerber <weissg@vienna.at>; Troy Kisky<troy.kisky@BoundaryDevices.com>");
MODULE_DESCRIPTION ("USB ISP116x Host Controller Driver");

MODULE_PARM(hc_verbose,"i");
MODULE_PARM_DESC(hc_verbose,"verbose startup messages, default is 1 (yes)");
MODULE_PARM(hc_error_verbose,"i");
MODULE_PARM_DESC(hc_error_verbose,"verbose error/warning messages, default is 0 (no)");
MODULE_PARM(urb_debug,"i");
MODULE_PARM_DESC(urb_debug,"debug urb messages, default is 0 (no)");
MODULE_LICENSE("GPL");
