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
#include <linux/smp_lock.h>
#include <linux/list.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/dma.h>

#define DEBUG
#include <linux/usb.h>

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


#define HcHardwareConfiguration_SETTING (InterruptPinEnable| HC_INTERRUPT_PIN_TRIGGER \
		|InterruptOutputPolarity| DataBusWidth16| HC_OC_DETECTION | HC_DREQOutputPolarity | HC_DOWN_STREAM_15KR)

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

static void delayUDELAY(hci_t* hci)
{
	udelay (1);
}
static void delay1(hci_t* hci)
{
	int port=hci->hp.memFencePort;
	outw(0,port);	//80ns access time for 1st access
}
static void delay2(hci_t* hci)
{
	int port=hci->hp.memFencePort;
	outw(0,port);	//80ns access time for 1st access
	outw(0,port);	//140ns delay between CS, 80 access time for 2nd access
}
static void delay3(hci_t* hci)
{
	int port=hci->hp.memFencePort;
	outw(0,port);	//80ns access time for 1st access
	outw(0,port);	//140ns delay between CS, 80 access time for 2nd access
	outw(0,port);	//1 more for luck
}
static void delayN(hci_t* hci)
{
	int port = hci->hp.memFencePort;
	int cnt = hci->hp.memCnt;
//	printk("memCnt=%d\n",cnt);
	do { outw(0,port); } while (--cnt);
}
static inline void ReadReg0(hci_t* hci, int regindex)
{
	outw (regindex, hci->hp.hcport + 4);
	if (hci->hp.delay) hci->hp.delay(hci);
}

static inline int ReadReg16(hci_t* hci, int regindex)
{
	int val = 0;
	ReadReg0(hci,regindex);
	val = inw (hci->hp.hcport);

	return val;
}
static inline int ReadReg32(hci_t* hci, int regindex)
{
	int val16, val;
	ReadReg0(hci,regindex);
	val16 = inw (hci->hp.hcport);
	val = val16;
	val16 = inw (hci->hp.hcport);
	val += val16 << 16;

	return val;
}

static inline void WriteReg0(hci_t* hci, int regindex)
{
	outw (regindex | 0x80, hci->hp.hcport + 4);
	if (hci->hp.delay) hci->hp.delay(hci);
}
static inline void WriteReg16(hci_t* hci, unsigned int value, int regindex)
{
	WriteReg0(hci,regindex);
	outw (value, hci->hp.hcport);
}
static inline void WriteReg32(hci_t* hci, unsigned int value, int regindex)
{
	WriteReg0(hci,regindex);
	outw (value, hci->hp.hcport);
	outw (value >> 16, hci->hp.hcport);
}
//////////////////
#define CP_INVALID 0xff
static int hc116x_read_reg16(hci_t* hci, int regindex)
{
	int ret;
#ifdef USE_COMMAND_PORT_RESTORE
	hci->command_port = regindex;		//only called from bh, so spin not needed
	wmb();
	ret = ReadReg16(hci,regindex);
	mb();
	hci->command_port = CP_INVALID;
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
	hci->command_port = regindex | 0x80;
	wmb();
	WriteReg16(hci, value, regindex);
	mb();
	hci->command_port = CP_INVALID;
#else
	unsigned long flags;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	WriteReg16(hci, value, regindex);
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
#endif
}

//only read_reg32 and write_reg32 are to be used by non-interrupt routines
//because a spin is needed in this case.
int hc116x_read_reg32(hci_t* hci, int regindex)
{
	unsigned long flags;
	int ret;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	ret = ReadReg32(hci,regindex);
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
	return ret;
}
void hc116x_write_reg32(hci_t* hci, unsigned int value, int regindex)
{
	unsigned long flags;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	WriteReg32(hci, value, regindex);
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
}

__u8 SaveState(hci_t* hci)
{
#ifdef USE_COMMAND_PORT_RESTORE
	return hci->command_port;
#else
	return  CP_INVALID;
#endif
}

void RestoreState(hci_t* hci,__u8 cp)
{
#ifdef USE_COMMAND_PORT_RESTORE
	if (cp != CP_INVALID) {
		hci->command_port = cp;				//restore previous state
		ReadReg0 (hci, cp);			//command_port includes the r/w flag
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
	hci->transferState = TF_TransferInProgress;
}
static inline int checkAllowedRegion(struct timing_lines* t,int cnt,int rem)
{
	int high,low;
	//calculate max time to finish
	high = (cnt * t->high.slope) + t->high.b;
	if ((rem<<16) > high) return 1;
	low  = (cnt * t->low.slope ) + t->low.b;
	if ((rem<<16) < low) return 1;
	return 0;
}
static inline void StartRequestTransfer(hci_t* hci,struct frameList * fl)
{
	int cnt = fl->reqCount;
	fl->sofCnt = 0;
	hci->transfer.fl = fl;
	if (cnt > 1) {
		unsigned long flags;
		int rem;
		spin_lock_irqsave(&hci->command_port_lock, flags);
		WriteReg16(hci, (cnt+1)& ~1, HcTransferCounter); 	//round to be safe
#ifdef USE_FM_REMAINING
// *********************************************
//DMA data points
//data points,
//Bytes,   fmRemainingReq
//    8    28,29,30,31,33,38,42,55,66,68,72,74,77,78,79	//old values 17,19,22,28,29,41,51,53
//   96    253,254,264,269,275		//old values 209
//  144    335,336
//  216    476
//  328    675,677
//  336    689
//  392    749*,790
//  416    840
//  448    894
//  480    945
//  512    1009
//  544    1056,1064
//  560    1099
//  720    1393*
//  784    1513*
//  808    1734
//
//So far, the limit lines contain the points
//(8,28) (544,1056) giving line y = (1056-28)/(544-8)x + b = 1028/536 x + b = 1.9179 x + 12.65 : scale up by 65536 * y = 125692 x + 829471
//and
//(8,79) (96,275) giving line y = (275-79)/(96-8)x + b = 196/88 + b = 2.228x + 61.182 : scale up by 65536 * y = 145967 x + 4009612
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
//    8    91, 92, 93,97,98,103,105,107,109,110
//RDF+RDN+2 == 3 + 10 + 2 = 150ns/2bytes + (ram read time) = 75ns/byte
// *********************************************
//rem is in units of 83 ns
		do {
			rem = ReadReg16(hci, HcFmRemaining);

#if defined(DMA_USED)
#if defined(PIO_USED)
			if (cnt<=72) if (checkAllowedRegion(&hci->pioLinesReq,cnt,rem)) goto programmed_io;
#endif
			if (checkAllowedRegion(&hci->dmaLinesReq,cnt,rem)) {
				if (StartDmaChannel(hci,fl,REQ_CHAIN)==0) {
					WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_WRITE|fl->dmac, HcDMAConfiguration);
					fl->lastTransferType=TT_DMA_REQ;
					break;
				}
			}
#endif

#if defined(PIO_USED)
			if (checkAllowedRegion(&hci->pioLinesReq,cnt,rem)) {
#if defined(DMA_USED)
programmed_io:
#endif
				WriteReg0(hci, fl->port);
				fl->lastTransferType=TT_PIO_REQ;
				FakeDmaReqTransfer(hci->hp.hcport, cnt, fl->chain,hci);
				hci->transfer.progress = NULL;
				break;
			}
#endif
		} while (1);
		fl->fmRemainingReq = rem;
		hci->lastTransferFrame = fl;
#else
		do {
			if (cnt>72)
				if (StartDmaChannel(hci,fl,REQ_CHAIN)==0) {
					WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_WRITE|fl->dmac, HcDMAConfiguration);
					break;
				}
			WriteReg0(hci, fl->port);
			FakeDmaReqTransfer(hci->hp.hcport, cnt, fl->chain,hci);
			hci->transfer.progress = NULL;
		} while (0);
#endif
		spin_unlock_irqrestore(&hci->command_port_lock, flags);
	}
#if 0
	 else if (CheckForPlaceHolderNeeded(fl)) {
			unsigned long flags;
			fl->reqCount = 1;	//mark as something sent

			spin_lock_irqsave(&hci->command_port_lock, flags);
				TransferActivate(hci,fl,REQ_CHAIN);
				WriteReg16(hci, 8, HcTransferCounter);
				WriteReg32(hci, LAST_MARKER, fl->port);
				WriteReg32(hci, ISO_MARKER, fl->port);
				hci->transfer.progress = NULL;
			spin_unlock_irqrestore(&hci->command_port_lock, flags);
			if (1) {
				int i=0;
				if ((fl->chain==NULL) && (fl->idle_chain==NULL)) {fl = fl->iso_partner; i=1;}
				if (fl) printk(KERN_DEBUG __FILE__ ": i:%i,chain:%p,idle:%p",i,fl->chain,fl->idle_chain);
			}
	}
#endif
}
static inline void StartResponseTransfer(hci_t* hci,struct frameList * fl)
{
	int cnt = fl->rspCount;
	unsigned long flags;
	int rem=1<<14;
	fl->sofCnt = 0;
	hci->transfer.fl = fl;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	if (cnt > 0) {
		WriteReg16(hci, (cnt+1)& ~1, HcTransferCounter);	//round to be safe
#ifdef USE_FM_REMAINING
		do {
			if (fl->done==ATLBufferDone) rem = ReadReg16(hci, HcFmRemaining);
#if defined(DMA_USED)
#if defined(PIO_USED)
			if (cnt<=72) if (checkAllowedRegion(&hci->pioLinesRsp,cnt,rem)) goto programmed_io;
#endif
			if (checkAllowedRegion(&hci->dmaLinesRsp,cnt,rem)) {
				if (StartDmaChannel(hci,fl,RSP_CHAIN)==0) {
					WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_READ|fl->dmac, HcDMAConfiguration);
					fl->lastTransferType=TT_DMA_RSP;
					break;
				}
			}
#endif

#if defined(PIO_USED)
			if (checkAllowedRegion(&hci->pioLinesRsp,cnt,rem)) {
#if defined(DMA_USED)
programmed_io:
#endif
				ReadReg0(hci, fl->port);
				fl->lastTransferType=TT_PIO_RSP;
				FakeDmaRspTransfer(hci->hp.hcport, cnt, fl->chain,hci);
				hci->transfer.progress = NULL;
				break;
			}
#endif
		} while (1);
		fl->fmRemainingRsp = rem;
		hci->lastTransferFrame = fl;
#else
		do {
			if (cnt>72)
				if (StartDmaChannel(hci,fl,RSP_CHAIN)==0) {
					WriteReg16(hci, ISP_BURST_CODE|0x14|DMAC_READ|fl->dmac, HcDMAConfiguration);
					break;
				}
			ReadReg0(hci, fl->port);
			FakeDmaRspTransfer(hci->hp.hcport, cnt, fl->chain,hci);
			hci->transfer.progress = NULL;
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
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
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


static void PrintFl(struct frameList * fl,struct frameList * fl2,struct frameList * flTransfer)
{
#ifdef USE_FM_REMAINING
	printk(KERN_DEBUG __FILE__ ":%c%c state:%i, preq:%i, prsp:%i, req:%i, rsp:%i full:%x, done:%x, fmRemRsp:%i, fmRemReq:%i tt:%i\n",
		((fl==flTransfer) ? 'x' : ' '),((fl==fl2) ? '*' : ' '),
		fl->state,fl->prevReqCount,fl->prevRspCount,fl->reqCount,fl->rspCount,fl->full,fl->done,
		fl->fmRemainingRsp,fl->fmRemainingReq,fl->lastTransferType);
#else
	printk(KERN_DEBUG __FILE__ ": %c state:%i, preq:%i, prsp:%i, req:%i, rsp:%i full:%x, done:%x\n",
		((fl==fl2) ? '*' : ' '),fl->state,fl->prevReqCount,fl->prevRspCount,fl->reqCount,fl->rspCount,fl->full,fl->done);
#endif
}
static void PrintStatusHistory(hci_t * hci, struct frameList * fl, int bstat,const char* message)
{
	printk(KERN_DEBUG __FILE__ ": %s, bstat:%x tp:%i eot_pc:%p\n",
		message,bstat,((hci->transfer.progress)? 1 : 0),hci->last_eot_pc);
	PrintFl(&hci->frames[FRAME_ITL0],fl,hci->lastTransferFrame);
	PrintFl(&hci->frames[FRAME_ITL1],fl,hci->lastTransferFrame);
	PrintFl(&hci->frames[FRAME_ATL],fl,hci->lastTransferFrame);

#ifdef INT_HISTORY_SIZE
	printk(KERN_DEBUG ": HcuPInterrupt read history, elapsed time in usec, value, + bottom half entry/exit\n" KERN_DEBUG);
	{
		int i = (hci->intHistoryIndex) & (INT_HISTORY_SIZE-1);
		int last = i;
		do {
			printk("%5i",hci->intHistory_elapsed[i]);
			i = (i+1) & (INT_HISTORY_SIZE-1);
		} while (i != last);
		printk("\n" KERN_DEBUG);
		do {
			int j=hci->intHistory_uP[i];
			if (j==0xfe) printk("   BH");
			else if (j==0xff) printk("    X");
			else printk("%5x",j);
			i = (i+1) & (INT_HISTORY_SIZE-1);
		} while (i != last);
		printk("\n" KERN_DEBUG);
		do {
			int j=hci->intHistory_uP[i];
			if ((j==0xfe)||(j==0xff)) printk("     ");
			else printk("%5x",hci->intHistory_bstat[i]);
			i = (i+1) & (INT_HISTORY_SIZE-1);
		} while (i != last);
	}
	printk("\n");
#endif
}
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
				if (OffInTheWeeds(fl,5)) if ((hci->resetIdleMask & fl->full)!=fl->full) {
//this chip is off in the weeds, get ready to reset
					if (hci->resetIdleMask==0) {
#ifdef USE_FM_REMAINING
						if (hci->lastEOTuP & SOFITLInt) NewPoint(hci,fl);
#endif
						if (fl->done==ATLBufferDone) PrintStatusHistory(hci,fl,bstat,"ATL BufferDone lost1");
						else PrintStatusHistory(hci,fl,bstat,"..ITL BufferDone lost2"); //this would have been caught earlier
					}
					hci->resetIdleMask |= fl->full;
				}
	}
	return 0;
}
static void hc_release_dma(hcipriv_t * hp);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int CheckClearInterrupts(hci_t * hci,struct pt_regs * regs);
static inline int CheckClear(hci_t * hci,struct pt_regs * regs)
{
	unsigned long flags;
	int i;
	spin_lock_irqsave(&hci->command_port_lock, flags);
	i = CheckClearInterrupts(hci,regs);
	spin_unlock_irqrestore(&hci->command_port_lock, flags);
	return i;
}


static int DoProcess(hci_t * hci, struct frameList * fl,struct ScheduleData * sd)
{
	int bScheduleRan=0;
	switch (fl->state) {
		case STATE_READ_RESPONSE: {
			int bstat = hci->bstat;
			if (hci->transferState==TF_TransferInProgress) break;
			if (hci->transfer.progress) break;

			if (BufferDone(hci,fl,bstat)==0) {
				int bd = (fl->done & (ITL0BufferDone|ITL1BufferDone));
				if (bd==0) break;
				if (hci->resetIdleMask==0) {
					if (hci->lastEOTuP & SOFITLInt) {
						if (bd != (ITL0BufferDone|ITL1BufferDone)) {
							NewPoint(hci,hci->lastTransferFrame);
						}
#if 1
						PrintStatusHistory(hci,fl,bstat,"ITL BufferDone lost3");
#else
						printk(KERN_DEBUG __FILE__ ": ITL BufferDone lost4\n");
#endif
					} else {
#if 1
						PrintStatusHistory(hci,fl,bstat,"ITL BufferDone but not on EOT");
#else
						printk(KERN_DEBUG __FILE__ ": ITL BufferDone but not on EOT\n");
#endif
					}
				}
				hci->resetIdleMask |= fl->full;
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
				StartResponseTransfer(hci,fl);
				fl->prevReqCount = fl->reqCount;
				fl->prevRspCount = fl->rspCount;
				fl->reqCount=0;
				fl->rspCount = 0;
			}
			fl->state = STATE_SCHEDULE_WORK;

		}
		case STATE_SCHEDULE_WORK: {
			if (hci->sofint) if (fl->done==ATLBufferDone) break;  //itl has priority
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
			if (hci->transferState==TF_TransferInProgress) break;
			if (hci->transfer.progress) break;

			if (fl->iso_partner) {
				if ((bstat & (ITL0BufferFull|ITL1BufferFull))==0) {
					//if both buffers are empty, I don't know which will be written.
					fl->iso_partner->done = fl->done = (ITL0BufferDone|ITL1BufferDone);
					fl->iso_partner->full = fl->full = (ITL0BufferFull|ITL1BufferFull);
				}
			}

			fl->active=0;
			if (hci->sofint) {
				if ((fl->done & (ITL0BufferDone|ITL1BufferDone))) fl->state = STATE_SCHEDULE_WORK; //if missed SOF and ITL buffer, then reschedule work
				break;	//sof read has priority.
			}
			if ( (bstat & fl->full) == fl->full) {
				if (hci->resetIdleMask==0) {
#if 1
					PrintStatusHistory(hci,fl,bstat,"full clear lost");
#else
					printk(KERN_DEBUG __FILE__ ": full clear lost\n");
#endif
				}
				hci->resetIdleMask |= fl->full;
				break;
			}
			if (hci->resetIdleMask) break;
			StartRequestTransfer(hci,fl);
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
#define BSTAT_WORK_PENDING 0x10000

static void ScanCancelled(hci_t * hci);
static void hc_interrupt_bh(unsigned long __hci)
{
	hci_t * hci = (hci_t *)__hci;
	int bScheduleRan;
#ifdef USE_COMMAND_PORT_RESTORE
	__u8 cp = hci->command_port;
	if (cp != CP_INVALID) if (hci->hp.delay) hci->hp.delay(hci);	//This is probably only needed in the real interrupt routine, not bottom half
#endif

#ifdef INT_HISTORY_SIZE
	{
		unsigned long flags;
		int i;
		unsigned int elapsed;
		spin_lock_irqsave(&hci->command_port_lock, flags);
			elapsed = CheckElapsedTime(hci);
			i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
			hci->intHistory_elapsed[i] = elapsed;
			hci->intHistory_uP[i] = 0xfe;
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
			if ((CheckClear(hci,NULL) & BSTAT_WORK_PENDING)==0) break;
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
		{
			unsigned long flags;
			int i;
			spin_lock_irqsave(&hci->command_port_lock, flags);
			i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
			hci->intHistory_elapsed[i] = 0;
			hci->intHistory_uP[i] = 0xff;
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
//		if (0)
		if (list_empty(&hci->active_list)) {
			if (list_empty(&hci->waiting_intr_list)) {
				if (!hci->resetIdleMask) {
					//disable SOF interrupts
					__u32 mask = hci->hp.uPinterruptEnable & ~(SOFITLInt|OPR_Reg);
					hc116x_write_reg16(hci, mask, HcuPInterruptEnable);
					hci->hp.uPinterruptEnable = mask;
				}
			}
		}
	}

#ifdef USE_COMMAND_PORT_RESTORE
	if (cp != CP_INVALID) {
		hci->command_port = cp;				//restore previous state
		ReadReg0 (hci, cp);				//command_port includes the r/w flag
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
	hci->resetIdleMask=0x80;
	return 1;
}

static void inline EotLost1(hci_t* hci,struct frameList * fl)
{
	int bstat = ReadReg16(hci, HcBufferStatus);
	if (fl->state==STATE_SCHEDULE_WORK) {
		if (bstat & fl->done) fl->state = STATE_READ_RESPONSE;
	} else {
		if ((bstat & fl->full)==0) fl->state = STATE_WRITE_REQUEST;
	}
	hci->transferState= TF_Aborted;
	PrintStatusHistory(hci,fl,bstat,"Transfer timeout");
//	if ((bstat & fl->full) && !(bstat & fl->done)) {
//		hci->resetIdleMask|=fl->full;
//	}
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
	hci->transferState= TF_Aborted;
	printk(KERN_DEBUG __FILE__ ": Transfer should not still be in progress, port:%i, chain:%i, bstat:%x\n",
					fl->port,hci->transfer.chain,bstat);
	PrintStatusHistory(hci,fl,bstat,"EOT lost");
//	if ((bstat & fl->full) && !(bstat & fl->done)) {
//		hci->resetIdleMask|=fl->full;
//	}
	return 1;
}

static void AbortTransferInProgress(hci_t* hci)
{
	struct frameList * fl = hci->transfer.fl;
	if (hci->transfer.progress) {
		hci->transfer.progress = NULL;
		if (hci->hp.dmaChannel>=0) {
			hc_release_dma(&hci->hp);
			printk(KERN_DEBUG __FILE__ ": Dma is not working, turning it off\n");
		}
	}
	{
		int cnt = ReadReg16(hci, HcTransferCounter);
		printk(KERN_DEBUG __FILE__ ": Transfer should not still be in progress, cnt:%i, port:%i, chain:%i\n",
					cnt,fl->port,hci->transfer.chain);
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
	EotLost1(hci,fl);
}

static inline int ReadAgainNeeded(hci_t * hci,int bstat,int changebstat)
{
	//atlint happened between the 2 reads, read again
	if (hci->atlint==0) if (bstat & ATLBufferDone) return 1;

	//an EOT has been missed, read HcuPInterrupt again to be sure
	if (hci->transferState==TF_TransferInProgress) if (changebstat || (!hci->transfer.progress)) return 1;

	//since interrupts are set to level triggered but pxa250 is edge triggered
	//I need to make sure SOF didn't happen since I read HcuPInterrupt
	if (hci->sofint==0) if (bstat & (ITL0BufferDone|ITL1BufferDone)) return 1;
	return 0;
}

//interrupts are disabled
static int CheckClearInterrupts(hci_t * hci,struct pt_regs * regs)
{
	hcipriv_t * hp = &hci->hp;
	int bstat;
	int ret=0;
	int ints_uP = ReadReg16(hci, HcuPInterrupt);
	int sum_ints_uP = ints_uP;
	do {
		if (ints_uP==0) break;
tryagain:
		if (ints_uP & OPR_Reg) {
			int ints = ReadReg32 (hci, HcInterruptStatus);
			if (ints) {
				ret = doInts(hci,ints);
				WriteReg32(hci, ints, HcInterruptStatus);	//clear bits
				WriteReg32(hci, OHCI_INTR_MIE|OHCI_INTR_SF|OHCI_INTR_SO, HcInterruptEnable);
			}
		}
		WriteReg16(hci, ints_uP, HcuPInterrupt);	//clear bits



		if (hci->transferState==TF_TransferInProgress) {
			if (ints_uP & AllEOTInterrupt) {
				hci->transferState=TF_Done;
				ret=1;
				hci->last_eot_pc = (regs)? ((void*)(instruction_pointer(regs))) : NULL;
				hci->lastEOTuP = sum_ints_uP;

			} else {
				if (OffInTheWeeds(hci->transfer.fl,3)) {
					AbortTransferInProgress(hci);
					ret=1;
				}
			}
		} else {
			if (hci->transfer.progress) {
				if (OffInTheWeeds(hci->transfer.fl,3)) {
					printk(KERN_DEBUG __FILE__ ": Dma interrupt lost\n");
					hci->transfer.progress = NULL;
					ret=1;
				}
			}
		}
		if ((ints_uP & hp->uPinterruptEnable) == 0) break;
		ret=1;


		if (ints_uP & SOFITLInt)
		{
			hci->frame_no = ReadReg16(hci, HcFmNumber); //why read all 32 bits, high 16 bits are reserved
			hci->sofint = 1;
			hci->frames[FRAME_ITL0].sofCnt++;
			hci->frames[FRAME_ITL1].sofCnt++;
			hci->frames[FRAME_ATL].sofCnt++;
		}
		if (ints_uP & ATLInt) {
			hci->atlint = 1;
		}
	} while (0);

	if (ints_uP)
	{
		int changebstat;
		bstat = ReadReg16(hci, HcBufferStatus);
#ifdef INT_HISTORY_SIZE
		{
			unsigned int elapsed = CheckElapsedTime(hci);
			int i = (hci->intHistoryIndex++) & (INT_HISTORY_SIZE-1);
			if (elapsed > 0xffff) elapsed = 0xffff;
			hci->intHistory_elapsed[i] = elapsed;
			hci->intHistory_uP[i] = ints_uP;
			hci->intHistory_bstat[i] = bstat;
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
			if (hci->transferState==TF_TransferInProgress) if (!hci->transfer.progress) {
				ret |= EotLost2(hci,hci->transfer.fl,bstat);
			}
		}
	} else bstat = hci->bstat;



	if (sum_ints_uP & ATLInt) if ((bstat&ATLBufferDone)==0) if (bstat&ATLBufferFull) {
		//atlint happened simultaneously with an itl EOT, leading to a race condition which loses ATLBufferDone
		if (sum_ints_uP & AllEOTInterrupt) { //just a double check as to what happened
			bstat = ReadReg16(hci, HcBufferStatus);	//triple check
			if ((bstat&ATLBufferDone)==0) {
				if (hci->resetIdleMask==0) {
#if 1
					PrintStatusHistory(hci,&hci->frames[FRAME_ATL],bstat,"ATLBufferDone lost5");
#else
					printk(KERN_DEBUG __FILE__ ": ATLBufferDone lost6\n");
#endif
				}
				hci->resetIdleMask |= ATLBufferFull;
			}
		} else {
			PrintStatusHistory(hci,&hci->frames[FRAME_ATL],bstat,"ATLBufferDone lost7 without EOT should not happen");
//			printk(KERN_DEBUG __FILE__ ": ATLBufferDone lost8 without EOT should not happen\n");
		}
	}

	if (ret) bstat |= BSTAT_WORK_PENDING;
	return bstat;
}


void hc_interrupt (int irq, void * __hci, struct pt_regs * r)
{
	hci_t * hci = __hci;
	int i;
	
#ifdef USE_COMMAND_PORT_RESTORE
	__u8 cp = hci->command_port;
	if (cp != CP_INVALID) if (hci->hp.delay) hci->hp.delay(hci);	//I don't know if this is needed, but it may explain weird problems if so
#endif
	hci->intHappened=1;
loop:
	i = CheckClear(hci,r);
	if (hci->inInterrupt==0) {
		hci->inInterrupt=1;	//serialize section
		if (hci->bhActive==0) {
			if (hci->resetIdleMask) if (hci->sofint) {
				do {
					if ((i & (ATLBufferFull|ITL0BufferFull|ITL1BufferFull)) & ~hci->resetIdleMask) break;
					if (hci->transferState==TF_TransferInProgress) break;
					if (hci->transfer.progress) break;
					if ((i&ATLBufferDone) && (hci->frames[FRAME_ATL].state == STATE_READ_RESPONSE)) break;
					if ((i&hci->frames[FRAME_ITL0].done) && (hci->frames[FRAME_ITL0].state == STATE_READ_RESPONSE)) break;
					if ((i&hci->frames[FRAME_ITL1].done) && (hci->frames[FRAME_ITL1].state == STATE_READ_RESPONSE)) break;
		//Hopefully, resetting on a SOF boundary will cause less USB disruption by having
		//a less weird length frame, also outstanding transactions to buffers that are still working
		//can be cleaned up.
					hc_init_regs(hci,0);
				} while (0);
			}
			DoItl(hci);	//ensure that ITL buffers are processed if some other tasklet is being serviced
			if (hci->transfer.progress==NULL) tasklet_schedule(&hci->bottomHalf);
		}
		hci->inInterrupt=0;	//serialize section
		if (hci->doubleInt) {
			hci->doubleInt = 0;
			if (hci->transfer.progress==NULL) goto loop;
		}
	} else {
		hci->doubleInt=1;
	}
#ifdef USE_COMMAND_PORT_RESTORE
	if (cp != CP_INVALID) {
		hci->command_port = cp;
		ReadReg0 (hci, hci->command_port);	//restore previous state
							//command_port includes the r/w flag
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
	hc116x_write_reg16(hci,HcHardwareConfiguration_SETTING,HcHardwareConfiguration);	//latch the zero
	hc116x_write_reg16(hci,HcHardwareConfiguration_SETTING& ~InterruptPinEnable,HcHardwareConfiguration);	//disable latch
//now ISP116x interrupts can't happen
//and we can use faster versions of functions
	dbg ("USB HC reset_hc usb-: ctrl = 0x%x ;",ReadReg32 (hci, HcControl));

#if 0
	WriteReg0(hci, HcSoftwareReset);	//just selecting this port causes a reset
#else
	WriteReg16(hci, 0xf6, HcSoftwareReset);	//0xf6 causes reset, ISP1161a, I don't think this is a problem for earlier versions of silicon
#endif
//	udelay (10);		//resets are flakey, maybe this will help
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
static void PrintLineInfo(hci_t * hci);
static void ScanCancelled(hci_t * hci)
{
	hci->scanForCancelled = 0;
	RemoveCancelled(hci,&hci->frames[FRAME_ITL0]);
	RemoveCancelled(hci,&hci->frames[FRAME_ITL1]);
	RemoveCancelled(hci,&hci->frames[FRAME_ATL]);
}

static int hc_init_regs(hci_t * hci,int bUsbReset)
{
	hcipriv_t * hp = &hci->hp;
	__u32 mask;
#define FI_BITS 11999	//0x2edf
//#define FREE_TIME 2000
#define FREE_TIME 0
	unsigned int fminterval = ((((FI_BITS - 210 - FREE_TIME) * 6) / 7) << 16) | FI_BITS;
	int i,j;
	hci->resetIdleMask = 0;
	i = (bUsbReset) ? hc_reset(hci) : hc_software_reset(hci);
	printk(KERN_DEBUG __FILE__ ": reset device\n");
	if ( i < 0) {
		err ("reset failed");
		return -ENODEV;
	}
	i = 1024+64;	//itl buffer length
	j = 4096 - (i<<1); //atl buffer length

	WriteReg16(hci, i, HcITLBufferLength);
	WriteReg16(hci, j, HcATLBufferLength);
	hci->errorCnt=0;
	WriteReg16(hci, 0, HcDMAConfiguration);

	hci->bstat=0;
	WriteReg32(hci, fminterval, HcFmInterval);
#define LS_BITS 1576	//0x628, low speed comparison of remaining to start packet
	WriteReg32(hci, LS_BITS, HcLSThreshold);


	if (bUsbReset) {
		// move from suspend to reset
		WriteReg32(hci, OHCI_USB_RESET, HcControl);
		wait_ms (1000);
	}

	/* start controller operations */
 	WriteReg32(hci, OHCI_USB_OPER, HcControl);

	/* Choose the interrupts we care about now, others later on demand */
	mask = OHCI_INTR_MIE |
//	OHCI_INTR_ATD |
	OHCI_INTR_SO |
	OHCI_INTR_SF;

	WriteReg32(hci, mask, HcInterruptEnable);
	WriteReg32(hci, mask, HcInterruptStatus);
#if 0
	mask = SOFITLInt | ATLInt | OPR_Reg;	// | AllEOTInterrupt;
#else
	mask = SOFITLInt | ATLInt | OPR_Reg | AllEOTInterrupt; // this is slower, but needed for itl/atl combined
#endif
	WriteReg16(hci, mask, HcuPInterrupt);
	hp->uPinterruptEnable = mask;
	WriteReg16(hci, mask, HcuPInterruptEnable);

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

	hci->frames[FRAME_ITL1].limit = hci->frames[FRAME_ITL0].limit = i;
	hci->frames[FRAME_ATL].limit = j;

	hci->transfer.progress = NULL;
	hci->transferState= TF_Aborted;
	ScanCancelled(hci);
	hci->frames[FRAME_ITL0].reqCount = 0;
	hci->frames[FRAME_ITL0].rspCount = 0;
	hci->frames[FRAME_ITL1].reqCount = 0;
	hci->frames[FRAME_ITL1].rspCount = 0;
	hci->frames[FRAME_ATL].reqCount = 0;
	hci->frames[FRAME_ATL].rspCount = 0;
	sh_scan_waiting_intr_list(hci);
	sh_scan_return_list(hci);
	if (hci->timingsChanged) PrintLineInfo(hci);

	WriteReg16(hci, HcHardwareConfiguration_SETTING,HcHardwareConfiguration);	//enable interrupt latch
	hci->chipReady=1;
	return 0;
}

void hc116x_enable_sofint(hci_t* hci)
{
	__u32 mask = hci->hp.uPinterruptEnable;
	if ((mask & SOFITLInt)==0) {
		unsigned long flags;
		mask |= SOFITLInt | ATLInt | OPR_Reg;
		spin_lock_irqsave(&hci->command_port_lock, flags);	//spin needed, process may be preempted
		WriteReg16(hci, mask, HcuPInterruptEnable);
		hci->hp.uPinterruptEnable = mask;
		spin_unlock_irqrestore(&hci->command_port_lock, flags);
		if (hci->transfer.progress==NULL) tasklet_schedule(&hci->bottomHalf);
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
static void PrintLine(struct timing_line* t,int roundUp,int transferType)
{
	printk(KERN_DEBUG "%s %s %s line:(%i,%i)-(%i,%i), slope:%i.%06i,b:%i.%06i\n",
		((transferType & TT_DMA)?"dma":"pio"),
		((transferType & TT_RSP)?"rsp":"req"),
		((roundUp)?"high":"low"),
		t->x1,t->y1,t->x2,t->y2,
		t->slope>>16,((t->slope & 0xffff)*(1000000>>6))>>(16-6),
		t->b    >>16,((t->b     & 0xffff)*(1000000>>6))>>(16-6));
}
static void initLine(struct timing_line* t,
	int x1,int y1,int x2,int y2,int roundUp)
{
	if (x1==x2) {
		if ( ((y1 > y2)? 1 : 0) ^ roundUp) y1 = y2;
		else y2 = y1;
	} else {
		if (x1 > x2) {
			int tempx = x1; int tempy = y1;
			x1 = x2;	y1 = y2;
			x2 = tempx;	y2 = tempy;
		}

		if (y1 > y2) {
			if (roundUp) {x2 = x1; y2 = y1;}
			else {x1 = x2; y1 = y2;}
		}
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
	if (t->x1 == 0)
	{
		x1 = x;
		y1 = y;
	} else {
		//choose point which gives maximum height line to keep
		int h1,h2;
		if (x < t->x1) h1 = t->y1 - y;
		else  h1 = y - t->y1;

		if (x < t->x2) h2 = t->y2 - y;
		else  h2 = y - t->y2;

		if (h2 > h1) {
			x1 = t->x2;
			y1 = t->y2;
		} else {
			x1 = t->x1;
			y1 = t->y1;
		}
	}
	initLine(t, x1, y1, x, y,roundUp);
}
static void NewPoint(hci_t * hci,struct frameList * fl)
{
	struct timing_lines* t;
	int x;
	int y;
	int max_v;
	int min_v;
	if (fl) {
		if (fl->lastTransferType & TT_RSP) {
			//after response reception is started, the count is moved to prevRspCount
			//and rspCount is zeroed
			x = fl->prevRspCount;
			y = fl->fmRemainingRsp;
#ifdef DMA_USED
			t = (fl->lastTransferType & TT_DMA) ? &hci->dmaLinesRsp : &hci->pioLinesRsp;
#else
			t = &hci->pioLinesRsp;
#endif
		} else {
			x = fl->reqCount;
			y = fl->fmRemainingReq;
#ifdef DMA_USED
			t = (fl->lastTransferType & TT_DMA) ? &hci->dmaLinesReq : &hci->pioLinesReq;
#else
			t = &hci->pioLinesReq;
#endif
		}
		max_v = MAX_SLOPE * x + MAX_B;
		min_v = MIN_SLOPE * x + MIN_B;
		if (((y<<16) > max_v) || ((y<<16) < min_v) || (x==0) || (y>FI_BITS)) return;

		if ((y<<16) > ((t->high.slope*x)+t->high.b)) IncludePoint(&t->high,x,y,1);
		if ((t->low.slope==0) || ((y<<16) < ((t->low.slope*x)+t->low.b))) IncludePoint(&t->low,x,y,0);
		PrintLine(&t->low,0,fl->lastTransferType);
		PrintLine(&t->high,1,fl->lastTransferType);
		hci->timingsChanged = 1;
	}
}
static void PrintLines(struct timing_lines* t,int transferType)
{
	PrintLine(&t->low,0,transferType);
	PrintLine(&t->high,1,transferType);
}
static void PrintLineInfo(hci_t * hci)
{
#ifdef DMA_USED
	PrintLines(&hci->dmaLinesReq,TT_DMA_REQ);
#endif
	PrintLines(&hci->pioLinesReq,TT_PIO_REQ);
#ifdef DMA_USED
	PrintLines(&hci->dmaLinesRsp,TT_DMA_RSP);
#endif
	PrintLines(&hci->pioLinesRsp,TT_PIO_RSP);
	hci->timingsChanged = 0;
}
static inline void initLines(struct timing_lines* t,
	int lowX1,int lowY1,int lowX2,int lowY2,
	int highX1,int highY1,int highX2,int highY2,int transferType)
{
	initLine(&t->low, lowX1, lowY1, lowX2, lowY2,0);
	initLine(&t->high,highX1,highY1,highX2,highY2,1);
//	PrintLine(&t->low,0,transferType);
//	PrintLine(&t->high,1,transferType);
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
	hp->dmaChannel = NODMA;

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
	hci->command_port = CP_INVALID;
#endif
	hci->itl_index = FRAME_ITL0;

#ifdef USE_FM_REMAINING
#ifdef DMA_USED
	initLines(&hci->dmaLinesReq,
		  DMA_LOW_X1_REQ, DMA_LOW_Y1_REQ, DMA_LOW_X2_REQ, DMA_LOW_Y2_REQ,
		  DMA_HIGH_X1_REQ,DMA_HIGH_Y1_REQ,DMA_HIGH_X2_REQ,DMA_HIGH_Y2_REQ,TT_DMA_REQ);
#endif
	initLines(&hci->pioLinesReq,
		  PIO_LOW_X1_REQ, PIO_LOW_Y1_REQ, PIO_LOW_X2_REQ, PIO_LOW_Y2_REQ,
		  PIO_HIGH_X1_REQ,PIO_HIGH_Y1_REQ,PIO_HIGH_X2_REQ,PIO_HIGH_Y2_REQ,TT_PIO_REQ);
#ifdef DMA_USED
	initLines(&hci->dmaLinesRsp,
		  DMA_LOW_X1_RSP, DMA_LOW_Y1_RSP, DMA_LOW_X2_RSP, DMA_LOW_Y2_RSP,
		  DMA_HIGH_X1_RSP,DMA_HIGH_Y1_RSP,DMA_HIGH_X2_RSP,DMA_HIGH_Y2_RSP,TT_DMA_RSP);
#endif
	initLines(&hci->pioLinesRsp,
		  PIO_LOW_X1_RSP, PIO_LOW_Y1_RSP, PIO_LOW_X2_RSP, PIO_LOW_Y2_RSP,
		  PIO_HIGH_X1_RSP,PIO_HIGH_Y1_RSP,PIO_HIGH_X2_RSP,PIO_HIGH_Y2_RSP,TT_PIO_RSP);
#endif
	hci->timingsChanged = 1;
	return hci;
}


/*-------------------------------------------------------------------------*/

/* De-allocate all resources.. */
static void hc_release_dma(hcipriv_t * hp)
{
	if (hp->dmaChannel>=0) {
		FreeDmaChannel(hp->dmaChannel);
		hp->dmaChannel = NODMA;
	}
	if (hp->dmaport) {
		release_region (hp->dmaport, 2);
		hp->dmaport = 0;
	}
}
static void unMapPhys(int type, int* base)
{
	if ((type==PORT_TYPE_PHYS)&&(*base)) {
		iounmap((char *)(*base));
		*base = 0;
	}
}

static void hc_release_hci (hci_t * hci)
{
	hcipriv_t * hp = &hci->hp;
	dbg ("USB HC release hci %d,%d", hci->frames[FRAME_ATL].reqCount,hci->frames[FRAME_ATL].rspCount);

	/* disconnect all devices */
	if (hci->bus->root_hub)
		usb_disconnect (&hci->bus->root_hub);

	if (hp->hc) {
		hc_reset (hci);
		release_region (hp->hc, 4);
		hp->hc = 0;
	}
	unMapPhys(hp->hcType,&hp->hcport);

	if (hp->memFenceType <= PORT_TYPE_VIRT) {
		if (hp->memFence) {
			release_region (hp->memFence, 4);
			hp->memFence = 0;
		}
		unMapPhys(hp->memFenceType,&hp->memFencePort);
	} else if (hp->memFenceType == PORT_TYPE_MEMORY) {
		struct dmaWork* dw = (struct dmaWork*)hci->hp.memFencePort;
		hci->hp.memFencePort = 0;
		if (dw) FreeDmaWork(hci,dw);
	}

	if (hp->wu) {
		release_region (hp->wu, 2);
		hp->wu = 0;
	}
	unMapPhys(hp->wuType,&hp->wuport);

	if (hp->irq >= 0) {
		free_irq (hp->irq, hci);
		hp->irq = -1;
	}
	hc_release_dma(hp);
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

static int MapPhys(int type, int base)
{
	int ret=base;
	if (type==PORT_TYPE_PHYS) {
		ret = (int) ioremap(base,8);
	}
	return ret;
}


static int __devinit hc_found_hci (int hcType,int hc, int memFenceType, int memFence, int memCnt,
		int wuType, int wu, int irq, dma_addr_t dmaaddr, int dma)
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
	hp->memFenceType = memFenceType;
	hp->memCnt = memCnt;
	hp->wuType = wuType;

	if (memFenceType <= PORT_TYPE_VIRT) {
		if (!request_region (memFence, 4, "ISP116x memFence")) {
			err ("fence request address 0x%x-0x%x failed", memFence, memFence+3);
			hc_release_hci (hci);
			return -EBUSY;
		}
		hp->memFence = memFence;
		hp->memFencePort = MapPhys(memFenceType,memFence);
	} else if (memFenceType == PORT_TYPE_MEMORY) {
		hp->memFencePort = (int)AllocDmaWork(hci);
	}

	if (memFenceType == PORT_TYPE_UDELAY) hp->delay = delayUDELAY;
	else if (memFenceType != PORT_TYPE_NONE) {
		if (memCnt==1) hci->hp.delay = delay1;
		else if (memCnt==2) hci->hp.delay = delay2;
		else if (memCnt==3) hci->hp.delay = delay3;
		else if (memCnt>3) hci->hp.delay = delayN;
	}

	if (!request_region (hc, 4, "ISP116x USB HOST")) {
		err ("hc request address 0x%x-0x%x failed", hc, hc+3);
		hc_release_hci (hci);
		return -EBUSY;
	}
	hp->hc = hc;
	hp->hcport = MapPhys(hcType,hc);


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

	if (wu) {
		if (!request_region (wu, 2, "ISP116x USB Wake up")) {
			err ("wu request address 0x%x-0x%x failed", wu, wu+1);
			hc_release_hci (hci);
			return -EBUSY;
		}
	}
	hp->wu = wu;
	hp->wuport = MapPhys(wuType,wu);

	if ( (dma != ALLOC_DMA)&&((unsigned)dma >= MAX_DMA_CHANNELS) ) dmaaddr = 0;
	if (dmaaddr) {
		if (!request_region (dmaaddr, 2, "ISP116x USB HOST DMA")) {
			err ("dma request address 0x%x-0x%x failed", dmaaddr, dmaaddr+1);
			hc_release_hci (hci);
			return -EBUSY;
		}
	} else dma=NODMA;
	hp->dmaport = dmaaddr;


	printk(KERN_INFO __FILE__ ": USB ISP116x at %x/%x IRQ %d Rev. %x ChipID: %x\n",
		hc, wu, irq, hc116x_read_reg32(hci, HcRevision), hc116x_read_reg16(hci, HcChipID));
	usb_register_bus (hci->bus);

	/* gpio36. i.e irq 59*/
	printk(KERN_INFO __FILE__ "USB host interrupt: rising edge. Gpio 36\n");
	set_GPIO_IRQ_edge(36, GPIO_RISING_EDGE);
	
	if (request_irq (irq, hc_interrupt, SA_INTERRUPT,
			"ISP116x", hci) != 0) {
		err ("request interrupt %d failed", irq);
		hc_release_hci (hci);
		return -EBUSY;
	}
	hp->irq = irq;

	if (dma!=NODMA) {
		int dmaCh = GetDmaChannel(hci, dma, "ISP116x USB HOST");
		if (dmaCh<0) {
			err ("request dma %d failed", dma);
			hc_release_dma(hp);
		}
		dma = dmaCh;
	}
	hp->dmaChannel = dma;

	if (hc_init_regs(hci,1) < 0) {
		err ("can't start usb-%x", hc);
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
static unsigned int hc = DEFAULT_HC_BASE;
static int memFenceType = DEFAULT_MEM_FENCE_TYPE;
static unsigned int memFence = DEFAULT_MEM_FENCE_BASE;
static int memCnt = DEFAULT_MEM_FENCE_ACCESS_COUNT;
static int wuType = DEFAULT_WU_TYPE;
static unsigned int wu = DEFAULT_WU_BASE;
static int irq = DEFAULT_IRQ;

static dma_addr_t dmaport = DEFAULT_DMA_BASE;
static int dma = DEFAULT_DMA;

MODULE_PARM(hcType,"i");
MODULE_PARM_DESC(hcType,"(0-io, 1-phys, 2-virt) " __STR__(DEFAULT_HC_TYPE));
MODULE_PARM(hc,"i");
MODULE_PARM_DESC(hc,"ISP116x PORT " __STR__(DEFAULT_HC_BASE));

MODULE_PARM(memFenceType,"i");
MODULE_PARM_DESC(memFenceType,"(0-io, 1-phys, 2-virt, 3-none, 4-memory, 5-udelay) " __STR__(DEFAULT_MEM_FENCE_TYPE));
MODULE_PARM(memFence,"i");
MODULE_PARM_DESC(memFence,"Fence " __STR__(DEFAULT_MEM_FENCE_BASE));
MODULE_PARM(memCnt,"i");
MODULE_PARM_DESC(memCnt,"Fence count " __STR__(DEFAULT_MEM_FENCE_ACCESS_COUNT));

MODULE_PARM(wuType,"i");
MODULE_PARM_DESC(wuType,"(0-io, 1-phys, 2-virt, 3-none) " __STR__(DEFAULT_WU_TYPE));
MODULE_PARM(wu,"i");
MODULE_PARM_DESC(wu,"WAKEUP PORT " __STR__(DEFAULT_WU_BASE));

MODULE_PARM(irq,"i");
MODULE_PARM_DESC(irq,"IRQ " __STR__(DEFAULT_IRQ));

MODULE_PARM(dmaport,"i");
MODULE_PARM_DESC(dmaport,"DMA physical port " __STR__(DEFAULT_DMA_BASE));

MODULE_PARM(dma,"i");
MODULE_PARM_DESC(dma,"DMA channel -2(ALLOC), -1(NONE), 0-15: " STR_DEFAULT_DMA "(default)");

static int __init hci_hcd_init (void)
{
	int ret;

	ret = hc_found_hci (hcType,hc, memFenceType,memFence,memCnt, wuType,wu, irq, dmaport, dma);

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
