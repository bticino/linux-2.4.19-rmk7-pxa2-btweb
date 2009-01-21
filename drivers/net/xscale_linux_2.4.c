/***************************************************************************
 *
 * Copyright (C) 2004-2005  SMSC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ***************************************************************************
 * File: xscale.c
 ***************************************************************************
 * 04/11/2005 Bryan Whitehead, rev 2
 *    updated platform code to support version 1.14 platform changes
 * 04/15/2005 Bryan Whitehead, rev 3
 *    migrated to perforce
 * 06/14/2005 Bryan Whitehead, rev 4
 *    merged .h file into .c file
 *
 ***************************************************************************
 * NOTE: When making changes to platform specific code please remember to 
 *   update the revision number in the PLATFORM_NAME macro. This is a 
 *   platform specific version number and independent of the 
 *   common code version number. The PLATFORM_NAME macro should be found in
 *   your platforms ".h" file.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/timer.h>
#include <asm/irq.h>
#include <asm/dma.h>
#include <asm/bitops.h>
#include <linux/version.h>
#include <asm/io.h>

#include "smsc911x_common.h"

static const char date_code[] = "061405";

#ifndef USING_LINT
#include <asm/hardware.h>
//#include <asm/arch/mainstone.h>
#include <asm/arch/pxa-regs.h>
#endif //not USING_LINT



/* for register access */
typedef volatile unsigned char  Reg8;
typedef volatile unsigned short Reg16;
typedef volatile DWORD  Reg32;
typedef volatile unsigned char  * pReg8;
typedef volatile unsigned short * pReg16;
typedef volatile unsigned long *  pReg32;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
//#ifndef MST_EXP_BASE
//#define MST_EXP_BASE (0xF5000000)
//#endif
//#ifndef MST_EXP_PHYS
//#define MST_EXP_PHYS (PXA_CS5_PHYS)
//#endif
//#ifndef MAINSTONE_nExBRD_IRQ
//#define MAINSTONE_nExBRD_IRQ MAINSTONE_IRQ(7)
//#endif
#endif

// Base address (Virtual Address)
// #define LAN_CSBASE 			MST_EXP_BASE

#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
//#define LAN_IRQ				MAINSTONE_nExBRD_IRQ
#else
////#define LAN_IRQ				MAINSTONE_EXBRD_IRQ
//#define LAN_IRQ				(167)
#endif

#define LAN_DMACH			(0UL)





#ifdef USING_LINT
void CleanCacheLine(DWORD addr);
void DrainWriteBuffers(void);
#else //not USING_LINT
static inline void CleanCacheLine(DWORD addr)
{
	addr = addr;		// let lint be happy :-)
	__asm("mov r0, %0" : : "r" (addr): "r0");
	__asm("mcr p15, 0, r0, c7, c10, 1");
	__asm("mcr p15, 0, r0, c7, c6, 1");
}

static inline void DrainWriteBuffers(void)
{
	__asm("mcr p15, 0, r0, c7, c10, 4");
}





#endif //not USING_LINT

BOOLEAN Platform_IsValidDmaChannel(DWORD dwDmaCh)
{
	if(dwDmaCh<=16) {
		return TRUE;
	}
	return FALSE;
}

DWORD Platform_Initialize(
	PPLATFORM_DATA platformData,
	DWORD dwLanBase,DWORD dwBusWidth)
{
	DWORD mode=0;
	DWORD test=0;
	SMSC_TRACE("--> Platform_Initialize()");

	if(dwLanBase==0x0UL) {
		dwLanBase=0x0c000000;
		SMSC_TRACE("Using dwLanBase==0x%08lX",dwLanBase);
	}

//	if(dwLanBase!=LAN_CSBASE) {
//		SMSC_WARNING("PlatformInitialize: resetting dwLanBase from 0x%08lX to 0x%08X", dwLanBase, LAN_CSBASE);
//		dwLanBase=LAN_CSBASE;
//	}

#define BYTE_TEST_OFFSET	(0x64)
#define ID_REV_OFFSET		(0x50)
	switch(dwBusWidth) {
	case 16:
		mode=2;	
		MSC1&=0x0000FFFFUL;
		MSC1|=0x8FF90000UL;
		test=(*(volatile unsigned long *)(dwLanBase+BYTE_TEST_OFFSET));
		if(test==0x87654321) {
			SMSC_TRACE(" 16 bit mode assigned and verified");
		} else {
			SMSC_WARNING(" failed to verify assigned 16 bit mode");
			dwLanBase=0;
		};break;
	case 32:
		mode=1;
		MSC1&=0x0000FFFFUL;
		MSC1|=0x8FF10000UL;
		//MSC1|=0x7FF10000UL;

		test=(*(volatile unsigned long *)(dwLanBase+BYTE_TEST_OFFSET));
		if(test==0x87654321) {
			SMSC_TRACE(" 32 bit mode assigned and verified");
		} else {
			SMSC_WARNING(" failed to verify assigned 32 bit mode %x",(unsigned int)test);
			dwLanBase=0;
		};break;
	default:
		mode=1;
		MSC1&=0x0000FFFFUL;
		MSC1|=0x8FF10000UL;
		{
			WORD dummy=0;
			test=(*(volatile unsigned long *)(dwLanBase+BYTE_TEST_OFFSET));
			dummy=(*(volatile unsigned short *)(dwLanBase+BYTE_TEST_OFFSET+2));
			if(test==0x87654321UL) {
				SMSC_TRACE(" 32 bit mode detected");
			} else {
				SMSC_TRACE(" 32 bit mode not detected, test=0x%08lX",test);
				MSC1|=0x00080000UL;
				test=(*(volatile unsigned long *)(dwLanBase+BYTE_TEST_OFFSET));
				if(test==0x87654321UL) {
					SMSC_TRACE(" 16 bit mode detected");
					mode=2;
				} else {
//					SMSC_WARNING(" test=0x%08lX",test);
					SMSC_WARNING(" neither 16 nor 32 bit mode detected.");
					dwLanBase=0;
				}
			}
			dummy=dummy;//make lint happy
		};break;
	}
		
	if(dwLanBase!=0) {
		DWORD idRev=(*(volatile unsigned long *)(dwLanBase+ID_REV_OFFSET));
		platformData->dwIdRev=idRev;
		switch(idRev&0xFFFF0000) {
		case 0x01180000UL:
		case 0x01170000UL:
		case 0x01120000UL:
			switch(mode) {
			case 1://32 bit mode
		  		MSC1 &= 0x0000FFFFUL;
			  	MSC1 |= 0x83710000UL;
				break;
			case 2://16 bit mode
				MSC1 &= 0x0000FFFFUL;
				MSC1 |= 0x83790000UL;
				break;
			default:break;//make lint happy
			};break;
		default:break;//make lint happy
		}
	}
	

	SMSC_TRACE(" MSC1=0x%08X", MSC1);
	SMSC_TRACE("<-- Platform_Initialize()");
	return dwLanBase;
}

void Platform_CleanUp(
	PPLATFORM_DATA platformData)
{
}

BOOLEAN Platform_Is16BitMode(PPLATFORM_DATA platformData)
{
	SMSC_ASSERT(platformData!=NULL);

	if (MSC1 & 0x00080000)
		return TRUE;
	else
		return FALSE;
}

void Platform_GetFlowControlParameters(
	PPLATFORM_DATA platformData,
	PFLOW_CONTROL_PARAMETERS flowControlParameters,
	BOOLEAN useDma)
{
	memset(flowControlParameters,0,sizeof(FLOW_CONTROL_PARAMETERS));
	flowControlParameters->BurstPeriod=100;
	flowControlParameters->IntDeas=0;
	if(useDma) {
		if(Platform_Is16BitMode(platformData)) {
			switch(platformData->dwIdRev&0xFFFF0000) {
			case 0x01180000UL:
			case 0x01170000UL:
			case 0x01120000UL:
				//117/118,16 bit,DMA
				flowControlParameters->MaxThroughput=0x10C210UL;
				flowControlParameters->MaxPacketCount=0x2DCUL;
				flowControlParameters->PacketCost=0x82UL;
				flowControlParameters->BurstPeriod=0x75UL;
				flowControlParameters->IntDeas=0x1CUL;
				break;
			case 0x01160000UL:
			case 0x01150000UL:
				//115/116,16 bit,DMA
				flowControlParameters->MaxThroughput=0x96CF0UL;
				flowControlParameters->MaxPacketCount=0x198UL;
				flowControlParameters->PacketCost=0x00UL;
				flowControlParameters->BurstPeriod=0x73UL;
				flowControlParameters->IntDeas=0x31UL;
				break;
			default:break;//make lint happy
			}
		} else {
			switch(platformData->dwIdRev&0xFFFF0000) {
			case 0x01180000UL:
			case 0x01170000UL:
			case 0x01120000UL:
				//117/118,32 bit,DMA
				flowControlParameters->MaxThroughput=0x12D75AUL;
				flowControlParameters->MaxPacketCount=0x332UL;
				flowControlParameters->PacketCost=0xDBUL;
				flowControlParameters->BurstPeriod=0x15UL;
				flowControlParameters->IntDeas=0x0BUL;
				break;
			case 0x01160000UL:
			case 0x01150000UL:
				//115/116,32 bit,DMA
				flowControlParameters->MaxThroughput=0xF3EB6UL;
				flowControlParameters->MaxPacketCount=0x29BUL;
				flowControlParameters->PacketCost=0x00UL;
				flowControlParameters->BurstPeriod=0x41UL;
				flowControlParameters->IntDeas=0x21UL;
				break;
			default:break;//make lint happy
			}
		}
	} else {
		if(Platform_Is16BitMode(platformData)) {
			switch(platformData->dwIdRev&0xFFFF0000) {
			case 0x01180000UL:
			case 0x01170000UL:
			case 0x01120000UL:

				//117/118,16 bit,PIO
				flowControlParameters->MaxThroughput=0xAC3F4UL;
				flowControlParameters->MaxPacketCount=0x1D2UL;
				flowControlParameters->PacketCost=0x5FUL;
				flowControlParameters->BurstPeriod=0x72UL;
				flowControlParameters->IntDeas=0x02UL;
				break;
			case 0x01160000UL:
			case 0x01150000UL:
				//115/116,16 bit,PIO
				flowControlParameters->MaxThroughput=0x7295CUL;
				flowControlParameters->MaxPacketCount=0x136UL;
				flowControlParameters->PacketCost=0x00UL;
				flowControlParameters->BurstPeriod=0x6FUL;
				flowControlParameters->IntDeas=0x38UL;
				break;
			default:break;//make lint happy
			}
		} else {
			switch(platformData->dwIdRev&0xFFFF0000) {
			case 0x01180000UL:
			case 0x01170000UL:
			case 0x01120000UL:
				//117/118,32 bit,PIO
				flowControlParameters->MaxThroughput=0xCB4BCUL;
				flowControlParameters->MaxPacketCount=0x226;
				flowControlParameters->PacketCost=0x00UL;
				flowControlParameters->BurstPeriod=0x77UL;
				flowControlParameters->IntDeas=0x36UL;
				break;
			case 0x01160000UL:
			case 0x01150000UL:
				//115/116,32 bit,PIO
				flowControlParameters->MaxThroughput=0x9E338UL;
				flowControlParameters->MaxPacketCount=0x1ACUL;
				flowControlParameters->PacketCost=0x00UL;
				flowControlParameters->BurstPeriod=0x73UL;
				flowControlParameters->IntDeas=0x31UL;
				break;
			default:break;//make lint happy
			}
		}
	}
}

void PlatformDisplayInfo(void);
void PlatformDisplayInfo(void)
{
}

BOOLEAN PlatformPossibleIRQ(int Irq);
BOOLEAN PlatformPossibleIRQ(int Irq)
{
	Irq=Irq;//make lint happy
	return TRUE;
}

BOOLEAN Platform_RequestIRQ(
	PPLATFORM_DATA platformData,
	DWORD dwIrq,
	irqreturn_t (*pIsr)(int a,void *b,struct pt_regs *c),
	void * dev_id)
{
	SMSC_ASSERT(platformData!=NULL);
	
	if(dwIrq==0xFFFFFFFFUL) {
//		dwIrq=LAN_IRQ;
	}

	//if(request_irq(dwIrq, pIsr, SA_INTERRUPT, "SMSC_LAN911x_ISR", dev_id)!=0) // all other interrupt disable during handler
	if(request_irq(dwIrq, pIsr, 0, "SMSC_LAN911x_ISR", dev_id)!=0) //!!!raf
	{
		SMSC_WARNING("Unable to use IRQ = %ld",dwIrq);
		//	return FALSE;
	}


	platformData->dwIrq = dwIrq;
	platformData->dev_id = dev_id;
	return TRUE;
}

DWORD Platform_CurrentIRQ(PPLATFORM_DATA platformData)
{
	SMSC_ASSERT(platformData!=NULL);
	return platformData->dwIrq;
}
void Platform_FreeIRQ(PPLATFORM_DATA platformData)
{
	if(platformData!=NULL) {
	  	free_irq(platformData->dwIrq,platformData->dev_id);
		platformData->dwIrq=0;
	} else {
		SMSC_ASSERT(FALSE);
	}
}

BOOLEAN Platform_DmaDisable(
	PPLATFORM_DATA platformData, const DWORD dwDmaCh)
{	
	// To avoid Lint error
	DWORD	temp;
	temp = dwDmaCh;
	temp = temp;

	platformData=platformData;//make lint happy

	DCSR(dwDmaCh) &= ~DCSR_RUN;
	while (!(DCSR(dwDmaCh) & DCSR_STOPSTATE)) {};

	return TRUE;
}

BOOLEAN Platform_DmaInitialize(PPLATFORM_DATA platformData, DWORD dwDmaCh)
{
	if(dwDmaCh<256)
		if(!Platform_DmaDisable(platformData, dwDmaCh)) {
			return FALSE;
		}

	return TRUE;
}

void PurgeCache(
	const void * const pStartAddress, 
	const DWORD dwLengthInBytes);
void PurgeCache(
	const void * const pStartAddress, 
	const DWORD dwLengthInBytes)
{
	DWORD dwCurrAddr, dwEndAddr, dwLinesToGo;
	
	dwCurrAddr = (DWORD)pStartAddress & CACHE_ALIGN_MASK;
	dwEndAddr = (((DWORD)pStartAddress) + dwLengthInBytes + (CACHE_LINE_BYTES - 1UL)) & CACHE_ALIGN_MASK;

	dwLinesToGo = (dwEndAddr - dwCurrAddr) / CACHE_LINE_BYTES;
	while (dwLinesToGo)
	{
		CleanCacheLine(dwCurrAddr);
		dwCurrAddr += CACHE_LINE_BYTES;
		dwLinesToGo--;
	}
	DrainWriteBuffers();

	return;	
}

void Platform_CacheInvalidate(PPLATFORM_DATA platformData, const void * const pStartAddress, const DWORD dwLengthInBytes)
{
	platformData=platformData;//make lint happy
	PurgeCache(pStartAddress, dwLengthInBytes);
}

void Platform_CachePurge(PPLATFORM_DATA platformData, const void * const pStartAddress, const DWORD dwLengthInBytes)
{
	platformData=platformData;//make lint happy
	PurgeCache(pStartAddress, dwLengthInBytes);
}

/*
 * SRAM Mapping on Linux:
 *		Physical Address = 0xA000_0000 - 0xA3FF_FFFF (Size = 64MB)
 *		Virtual Address  = 0xC000_0000 - 0xC3FF_FFFF (Size = 64MB)
 *
 * Expansion Card (LAN911x) Mapping on Linux 2.4 - btweb:
 *		Physical Address = 0x0c00_0000 - 0x0c0F_FFFF (Size = 1MB)
 *      Virtual Address  = 0xF100_0000 - 0xF10F_FFFF (Size = 1MB)
 */
DWORD CpuToPhysicalAddr(const void * const pvCpuAddr)
{

	if ((u32)pvCpuAddr > 0xf0000000) /* device memory */
		return ((u32)pvCpuAddr & 0xfffff) | LAN_BASE;
	else /* ram */
		return virt_to_bus(pvCpuAddr);
}

BOOLEAN Platform_DmaStartXfer(PPLATFORM_DATA platformData, const DMA_XFER * const pDmaXfer)
{

	DWORD dwDmaCmd;
	DWORD dwLanPhysAddr, dwMemPhysAddr;
	DWORD dwAlignMask;

	platformData=platformData;//make lint happy

	//Check if the channel is currently running
	if (!(DCSR(pDmaXfer->dwDmaCh) & DCSR_STOPSTATE))
	{
		if (DCSR(pDmaXfer->dwDmaCh) & DCSR_RUN) 
		{
			SMSC_TRACE("DmaStartXfer -- requested channel (%ld) is still running", pDmaXfer->dwDmaCh);
			return FALSE;
		}
		else 
		{
			// DMA is not running yet.
			// Keep going..
			SMSC_WARNING("DmaStartXfer -- requested channel (%ld) is weird status", pDmaXfer->dwDmaCh);
		}
	}

	// calculate the physical transfer addresses
	dwLanPhysAddr = CpuToPhysicalAddr((void *)pDmaXfer->dwLanReg);
	dwMemPhysAddr = CpuToPhysicalAddr((void *)pDmaXfer->pdwBuf);

	//dwLanPhysAddr += 0x800;

	
	SMSC_TRACE("DMA: channel %i, %p %p (%i)", pDmaXfer->dwDmaCh, dwLanPhysAddr, dwMemPhysAddr, pDmaXfer->dwDwCnt*4);

	// need CL alignment for CL bursts
	dwAlignMask = (CACHE_LINE_BYTES - 1UL);

	if ((dwLanPhysAddr & dwAlignMask) != 0UL)
	{
		SMSC_WARNING("DmaStartXfer -- bad dwLanPhysAddr (0x%08lX) alignment",
			dwLanPhysAddr);
		return FALSE;
	}

	//if ((dwMemPhysAddr & dwAlignMask) != 0UL)
	if ((dwMemPhysAddr & 0x03UL) != 0UL)
	{
		SMSC_WARNING("DmaStartXfer -- bad dwMemPhysAddr (0x%08lX) alignment",
			dwMemPhysAddr);
			return FALSE;
	}

	//validate the transfer size, On XScale Max size is 8k-1
	if (pDmaXfer->dwDwCnt >= 8192UL)
	{
		SMSC_WARNING("DmaStartXfer -- dwDwCnt =%ld is too big (1^20 bytes max on panax)", pDmaXfer->dwDwCnt);
		return FALSE;
	}

	//Note: The DCSR should be cleared, before writing the Target and Source addresses
	DCSR(pDmaXfer->dwDmaCh) = DCSR_NODESC; //get ready for the DMA

	//Set the Source and Target addresses
	dwDmaCmd = 0UL;
	if (pDmaXfer->fMemWr)
	{
		//RX
		DTADR(pDmaXfer->dwDmaCh) = dwMemPhysAddr;
		DSADR(pDmaXfer->dwDmaCh) = dwLanPhysAddr;
		dwDmaCmd |= DCMD_INCTRGADDR; //always increment the memory address
		//dwDmaCmd |= DCMD_INCSRCADDR;
	}
	else
	{
		//TX
		DTADR(pDmaXfer->dwDmaCh) = dwLanPhysAddr;
		DSADR(pDmaXfer->dwDmaCh) = dwMemPhysAddr;
		dwDmaCmd |= DCMD_INCSRCADDR; //always increment the memory address
		//dwDmaCmd |= DCMD_INCTRGADDR;
	}

	//Set the burst size, if cache line burst set to 32Bytes else set to the smallest
	dwDmaCmd |= DCMD_BURST32;

	dwDmaCmd |= (DCMD_LENGTH & (pDmaXfer->dwDwCnt << 2));
	DCMD(pDmaXfer->dwDmaCh) = dwDmaCmd ;
	DCSR(pDmaXfer->dwDmaCh) |= DCSR_RUN;
	
	return TRUE;
}

DWORD Platform_DmaGetDwCnt( PPLATFORM_DATA platformData, const DWORD dwDmaCh)
{
	platformData=platformData;//make lint happy
	return (((DCMD_LENGTH & DCMD(dwDmaCh)) >> 2));	
}

void Platform_DmaComplete(PPLATFORM_DATA platformData, const DWORD dwDmaCh)
{
	DWORD dwTimeOut=1000000;

	platformData=platformData;//make lint happy

	while((Platform_DmaGetDwCnt(platformData,dwDmaCh))&&(dwTimeOut))
	{
		udelay(1);
		dwTimeOut--;
	}
	if(!Platform_DmaDisable(platformData,dwDmaCh)) {
		SMSC_WARNING("Failed Platform_DmaDisable");
	}
	if(dwTimeOut==0) {
		SMSC_WARNING("Platform_DmaComplete: Timed out");
	}
}

static void handler(int irq, void *data, struct pt_regs *regs)
{
	int i = *((int *)data);
	printk("DMA irq %i %08lx\n", i, DCSR(i));
	DCSR(i) = DCSR(i) & ~DCSR_STOPIRQEN;
}

DWORD Platform_RequestDmaChannel(PPLATFORM_DATA platformData)
{
	int *ch = kmalloc(sizeof(int), GFP_ATOMIC);
	int channel = pxa_request_dma("eth0", DMA_PRIO_HIGH, handler, (void *)ch);
	*ch = channel;
	printk("request dma: channel is %i\n", channel);
	return channel;
}

void Platform_ReleaseDmaChannel(
	PPLATFORM_DATA platformData,
	DWORD dwDmaChannel)
{
	pxa_free_dma(dwDmaChannel);
}

void Platform_WriteFifo(
	DWORD dwLanBase,
	DWORD *pdwBuf,
	DWORD dwDwordCount)
{
	volatile DWORD *pdwReg;
	pdwReg = (volatile DWORD *)(dwLanBase+TX_DATA_FIFO);
	while(dwDwordCount)
	{
		*pdwReg = *pdwBuf++;
		dwDwordCount--;
	}

}
void Platform_ReadFifo(
	DWORD dwLanBase,
	DWORD *pdwBuf,
	DWORD dwDwordCount)
{
	const volatile DWORD * const pdwReg = 
		(const volatile DWORD * const)(dwLanBase+RX_DATA_FIFO);
	
	while (dwDwordCount)
	{
		*pdwBuf++ = *pdwReg;
		dwDwordCount--;
	}
}

