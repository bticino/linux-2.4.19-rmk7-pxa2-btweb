/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*
 * DMA support for use with ISP1161
 * Author: Troy Kisky - troy.kisky@boundarydevices.com
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
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/dma.h>

#include <linux/usb.h>

#include "hc_isp116x.h"
#include "hc_simple116x.h"
#include "pxa-dmaWork.h"

#define DEBUG_LEVEL 1

//BURST are tricky when used with a chain of descriptors.
//2 cycle burst would work because the 1161 forces aligment on 4 byte boundary,
//    but that is not an option on ISP1161 or pxa250
//
//
//In order to get burst 8 to work we would need to enforce that requested transfer lengths mod 8 != (5,6,7,0)
//    only appear as the last in the chain or
//    have the trailing bytes followed by a dummy padded PTD as the last link of the transfer

//In order to get burst 16 to work we would need to enforce that requested transfer lengths mod 16 != (13,14,15,0)
//    only appear as the last in the chain or
//    have the trailing bytes followed by a dummy padded PTD as the last link of the transfer
//    AND we would need to prepend a 0 length (non-active) dummy PTD to the real PTD to get to 16 bytes
//Bottom line, you can't be in the middle of a burst when a descriptor chains.
//**************************


#if BURST_TRANSFER_SIZE==16
#define DCMD_BURST_CODE DCMD_BURST16
#define MAX_BUFFER_SIZE 48	//max space needed, 8 dummy + 8 ptd, 12 trail + 20 dummy

#elif BURST_TRANSFER_SIZE==8
#define DCMD_BURST_CODE DCMD_BURST8
#define MAX_BUFFER_SIZE 24	//max space needed, 8 ptd, 4 trail + 12 dummy

#else
#define DCMD_BURST_CODE 0	//no dma
#define MAX_BUFFER_SIZE 16	//max space needed, 8 ptd 8 setup data
#endif


struct dmaDesc
{
	dma_addr_t next;	//-  points to req_next within this structure
	dma_addr_t source;	//-  physical address of ptd within this structure
	dma_addr_t target;	//-
	int	   cmd;		//b
};
//this structure is in none-cacheable memory and aligned on 16 byte boundary
//it is pointed to by urb_t.hcpriv
struct dmaWork;
struct dmaWork
{
#if BURST_TRANSFER_SIZE >= 8
	struct dmaDesc req[3];
	struct dmaDesc rsp[3];
#endif
	unsigned int req_buffer[MAX_BUFFER_SIZE>>2] __attribute__ ((aligned (8)));
	unsigned int rsp_buffer[MAX_BUFFER_SIZE>>2] __attribute__ ((aligned (8)));


#if BURST_TRANSFER_SIZE >= 8
	unsigned char last_dmaReq_index;	//# of dmaDesc's used -1
	unsigned char last_dmaRsp_index;
	short	len_trailing;		//# of bytes copied in update_item rtn
	dma_addr_t dmaAddr;		//s
#endif
	short	len_ptd;		//usually 8 bytes, 16 bytes for setup
	short	len_virt;
	short	orig_lengthInPtd;
	unsigned char itl_index;
	unsigned char spare;

#if BURST_TRANSFER_SIZE >= 8
	dma_addr_t dma_base;
#else
	int	freeable;
#endif
	short	unit_count;		//amount of ATL or ITL buffer space used by entry as currently configured
	short minsize;			//if len_virt is < this, then I cannot short it more
	unsigned char error_count;
	unsigned char cc;
	unsigned char cancel;
	unsigned char pipetype;


	struct dmaWork* virt_next;	//ib chain
	char*	virt_buffer;		//b should stay aligned on 8 byte boundary
	int	remaining;		//sb
	int	transferred_cnt;	//sb
	int	pipe;
	urb_t*	owner_urb;		//-
	int	cur_iso_frame_index;
	struct dmaWork* iso_next;
	wait_queue_head_t waitq;	// used with unlink
};

#define NOT_IN_CHAIN ((struct dmaWork*)-1)
#define MIN(a,b) (( (a)< (b) ) ? (a) : (b))
#define MAX(a,b) (( (a)> (b) ) ? (a) : (b))

#if 0
#define REQ_NO_FIXED DCMD_INCTRGADDR	//rom and flash external peripherals don't support fixed addresses
#define RSP_NO_FIXED DCMD_INCSRCADDR
#else
#define REQ_NO_FIXED 0
#define RSP_NO_FIXED 0
#endif

//#define REQ_CMD_PTD (REQ_NO_FIXED|DCMD_INCSRCADDR|DCMD_BURST_CODE|DCMD_FLOWTRG|DCMD_WIDTH2|DCMD_STARTIRQEN|DCMD_ENDIRQEN)  //|DCMD_ENDIRQEN if last && IN
#define REQ_CMD_PTD (REQ_NO_FIXED|DCMD_INCSRCADDR|DCMD_BURST_CODE|DCMD_FLOWTRG|DCMD_WIDTH2)  //|DCMD_ENDIRQEN if last && IN
#define REQ_CMD_IN  (REQ_NO_FIXED|                DCMD_BURST_CODE|DCMD_FLOWTRG|DCMD_WIDTH2)  //                             src zero
#define REQ_CMD_OUT (REQ_NO_FIXED|DCMD_INCSRCADDR|DCMD_BURST_CODE|DCMD_FLOWTRG|DCMD_WIDTH2)  //|DCMD_ENDIRQEN if last && OUT, src PCI_MAP_SINGLE

//follow along with response to minimize lag to the next request
#define RSP_CMD_PTD (RSP_NO_FIXED|DCMD_INCTRGADDR|DCMD_BURST_CODE|DCMD_FLOWSRC|DCMD_WIDTH2|DCMD_STARTIRQEN)                 //|DCMD_ENDIRQEN if last && OUT
#define RSP_CMD_IN  (RSP_NO_FIXED|DCMD_INCTRGADDR|DCMD_BURST_CODE|DCMD_FLOWSRC|DCMD_WIDTH2) //|DCMD_ENDIRQEN if last && IN target PCI_MAP_SINGLE
#define RSP_CMD_OUT (RSP_NO_FIXED|                DCMD_BURST_CODE|DCMD_FLOWSRC|DCMD_WIDTH2) //target trash


/*-------------------------------------------------------------------------*/
/* condition (error) CC codes and mapping OHCI like
 * */

#define TD_CC_NOERROR      0x00
#define TD_CC_CRC          0x01
#define TD_CC_BITSTUFFING  0x02
#define TD_CC_DATATOGGLEM  0x03
#define TD_CC_STALL        0x04
#define TD_DEVNOTRESP      0x05
#define TD_PIDCHECKFAIL    0x06
#define TD_UNEXPECTEDPID   0x07
#define TD_DATAOVERRUN     0x08
#define TD_DATAUNDERRUN    0x09
#define TD_BUFFEROVERRUN   0x0C
#define TD_BUFFERUNDERRUN  0x0D
#define TD_NOTACCESSED     0x0F
#define TD_BAD_RESPONSE    0x10
#define TD_INIT_VALUE      0x11
#define TD_TOO_BIG	   0x12
#define TD_INTERVAL	   0x13
static int cc_to_error[] = {

/* mapping of the OHCI CC status to error codes */
	/* No  Error  */               USB_ST_NOERROR,		//0x00 TD_CC_NOERROR
	/* CRC Error  */               USB_ST_CRC,		//0x01 TD_CC_CRC
	/* Bit Stuff  */               USB_ST_BITSTUFF,		//0x02 TD_CC_BITSTUFFING
	/* Data Togg  */               USB_ST_CRC,		//0x03 TD_CC_DATATOGGLEM
	/* Stall      */               USB_ST_STALL,		//0x04 TD_CC_STALL
	/* DevNotResp */               USB_ST_NORESPONSE,	//0x05 TD_DEVNOTRESP
	/* PIDCheck   */               USB_ST_BITSTUFF,		//0x06 TD_PIDCHECKFAIL
	/* UnExpPID   */               USB_ST_BITSTUFF,		//0x07 TD_UNEXPECTEDPID
	/* DataOver   */               USB_ST_DATAOVERRUN,	//0x08 TD_DATAOVERRUN
	/* DataUnder  */               USB_ST_DATAUNDERRUN,	//0x09 TD_DATAUNDERRUN
	/* reservd    */               USB_ST_NORESPONSE,	//0x0A
	/* reservd    */               USB_ST_NORESPONSE,	//0x0B
	/* BufferOver */               USB_ST_BUFFEROVERRUN,	//0x0C TD_BUFFEROVERRUN
	/* BuffUnder  */               USB_ST_BUFFERUNDERRUN,	//0x0D TD_BUFFERUNDERRUN
	/* Not Access */               USB_ST_NORESPONSE,	//0x0E
	/* Not Access */               USB_ST_NORESPONSE,	//0x0F TD_NOTACCESSED
	/* Not Access */               USB_ST_NORESPONSE,	//0x10 TD_BAD_RESPONSE
	/* Not Access */               USB_ST_NORESPONSE,	//0x11 TD_INIT_VALUE
	/* Not Access */               USB_ST_NORESPONSE,	//0x12 TD_TOO_BIG
	/* Not Access */               USB_ST_NORESPONSE	//0x13 TD_INTERVAL
};

struct dmaWork* AllocDmaWork(hci_t * hci)
{
	struct dmaWork* ret=NULL;
	if (hci->free.chain) {
		unsigned long flags;
		spin_lock_irqsave (&hci->free.lock, flags);
		rmb();
		ret = hci->free.chain;
		if (ret) {
			hci->free.chain = ret->virt_next;
			ret->virt_next = NOT_IN_CHAIN;
		}
		spin_unlock_irqrestore (&hci->free.lock, flags);
	}
	if (!ret) {
#if BURST_TRANSFER_SIZE >= 8
		dma_addr_t dma_handle;
		ret = pci_alloc_consistent(NULL, PAGE_SIZE, &dma_handle);
#else
		ret = (struct dmaWork*) kmalloc (PAGE_SIZE, GFP_KERNEL);
#endif
		if (ret) {
			int size= (sizeof (struct dmaWork) + 0x0f) & ~0x0f;
			struct dmaWork* dw = ret;
			struct dmaWork* end = (struct dmaWork*)( ((unsigned char*)ret) + PAGE_SIZE);
			struct dmaWork* q;
			memset(ret, 0, PAGE_SIZE);
#if BURST_TRANSFER_SIZE >= 8
			if (hci->zero==0) {
				dma_addr_t zero = dma_handle+PAGE_SIZE-BURST_TRANSFER_SIZE;
				dma_addr_t trash = zero-BURST_TRANSFER_SIZE;
				struct dmaDesc* desc;
				end = (struct dmaWork*)(((unsigned char*)end) - ((BURST_TRANSFER_SIZE+sizeof(struct dmaDesc))<<1));
				desc = (struct dmaDesc*)end;

				hci->zero = zero;
				desc->next   = DDADR_STOP;
				desc->source = zero;				//src zero
				desc->target = hci->hp.dmaport;
				desc->cmd    = REQ_CMD_IN+EXTRA_DMA_REQ_CNT;
				hci->zeroDesc = trash - (sizeof(struct dmaDesc)<<1);

				desc++;
				hci->trash = trash;
				desc->next   = DDADR_STOP;
				desc->source = hci->hp.dmaport;
				desc->target = trash;				//dest trash
				desc->cmd    = RSP_CMD_OUT+EXTRA_DMA_RSP_CNT;
				hci->trashDesc = trash - (sizeof(struct dmaDesc));
			}
#else
			ret->freeable = 1;
#endif
			end = (struct dmaWork*)(((unsigned char*)end) - size);
			do {
				init_waitqueue_head (&dw->waitq);
#if BURST_TRANSFER_SIZE >= 8
				dw->dma_base = dma_handle;
				dw->req[0].source = dma_handle + (int)( ((struct dmaWork*)0)->req_buffer);
				dw->rsp[0].target = dma_handle + (int)( ((struct dmaWork*)0)->rsp_buffer);
				dw->rsp[2].source = dw->rsp[1].source = dw->rsp[0].source = dw->req[2].target = dw->req[1].target = dw->req[0].target= hci->hp.dmaport;
#endif
				q  = (struct dmaWork*)(((unsigned char*)dw)+size);
				if (q > end) break;
				dw = dw->virt_next = q;
#if BURST_TRANSFER_SIZE >= 8
				dma_handle+=size;
#endif
			} while (1);
			dw->virt_next = NULL;
			q = ret->virt_next;
			ret->virt_next= NOT_IN_CHAIN;
			if (q){
				unsigned long flags;
				spin_lock_irqsave (&hci->free.lock, flags);
				rmb();
				dw->virt_next = hci->free.chain;
				hci->free.chain = q;
				spin_unlock_irqrestore (&hci->free.lock, flags);
			}
		}
	}
	return ret;
}

void FreeDmaWork(hci_t * hci,struct dmaWork* free)
{
	struct dmaWork* free1;
	if (free) {
		unsigned long flags;
//even while free, waitq remains valid....
		wake_up(&free->waitq);
		spin_lock_irqsave (&hci->free.lock, flags);
		rmb();
		do {
			if (free->virt_next == NOT_IN_CHAIN) {
				free1 = free->iso_next;
				free->iso_next=NULL;
				free->virt_next = hci->free.chain;
				hci->free.chain = free;
				free = free1;
			} else {
				printk(KERN_ERR __FILE__ ": buffer is not free\n");
				break;
			}
		} while (free);
		spin_unlock_irqrestore (&hci->free.lock, flags);
	}
}
void WaitForIdle(hci_t * hci, urb_t * urb)
{
	struct dmaWork* dw;
	unsigned long flags;
	spin_lock_irqsave (&hci->urb_list_lock, flags);
	dw = (struct dmaWork*) urb->hcpriv;
	if (dw) {
		int i=0;
		DECLARE_WAITQUEUE (wait, current);

		do {
			//iso urbs can be queued too early
			if (dw) add_wait_queue (&dw->waitq, &wait);
			hci->scanForCancelled=1;
			tasklet_schedule(&hci->bottomHalf);	//make sure it's still alive
			spin_unlock_irqrestore (&hci->urb_list_lock, flags);

			set_current_state (TASK_UNINTERRUPTIBLE);
			schedule_timeout(HZ/5);		//wait up to .2 seconds
			if (dw) remove_wait_queue (&dw->waitq, &wait);
			spin_lock_irqsave (&hci->urb_list_lock, flags);
			i++;
			rmb();
			dw = (struct dmaWork*) urb->hcpriv;
		} while ((dw || urb->dev) && (i<25));

		rmb();
		if (!list_empty (&urb->urb_list))
			list_del_init (&urb->urb_list);
//		urb->complete = NULL;
		if (urb->hcpriv || urb->dev) {
			urb->hcpriv = NULL;
			printk(KERN_ERR __FILE__  ": improper unlink\n");
		}
	}
	spin_unlock_irqrestore (&hci->urb_list_lock, flags);
}

void ReleaseDmaWork(struct dmaWork* dw)
{
	struct dmaWork** dwp;
	struct dmaWork* dwStart=NULL;
	dwp = &dwStart;
	while (dw) {
		struct dmaWork* p = dw->virt_next;
#if BURST_TRANSFER_SIZE >= 8
		if ( (((unsigned int)dw) & (PAGE_SIZE-1)) == 0 )		//strip all non page aligned structures
#else
		if (dw->freeable)
#endif
		{
			dw->virt_next=NULL;
			*dwp = dw;
			dwp = &dw->virt_next;
		}
		dw=p;
	}
	while (dwStart) {
		struct dmaWork* p = dwStart->virt_next;
#if BURST_TRANSFER_SIZE >= 8
		pci_free_consistent(NULL, PAGE_SIZE, dwStart, dwStart->dma_base);
#else
		kfree(dwStart);
#endif
		dwStart=p;
	}
}


static inline void* update_memcpy_align(void* dest,void* src, int length)
{
	__u8* p=dest;
	memcpy(p,src,length);
	p+=length;
	while ( ((int)p) & 3) *p++ = 0;
	return p;
}
static inline void* skip_align(void* dest,int length)
{
	__u8* p=dest;
	p+=((length+3)& ~3);
	return p;
}

static void InitDmaStruct(struct dmaWork* dw,int d0,int d1,int* buf,hci_t * hci)
{
	int len_virt = 0;
#if BURST_TRANSFER_SIZE >= 8
	int len_trailing = 0;
	int last_dmaReq = 0;
	int last_dmaRsp = 0;
#endif
	int* rb = dw->req_buffer;
	char* rbstart = (char*) rb;
	char* rbTrail;
	int bOut = (PTD_PID(d0,d1)==PID_IN)? 0 : 1;
	int lengthInPtd = PTD_ALLOTED(d0,d1);

	int slack = MAX_BUFFER_SIZE - ( ((lengthInPtd+3) &~3) + 8);	//MAX_BUFFER_SIZE is a multiple of BURST_TRANSFER_SIZE
	dw->orig_lengthInPtd = lengthInPtd;
//	printk("sl:%d,%d,%d",slack,lengthInPtd,MAX_BUFFER_SIZE);

#if BURST_TRANSFER_SIZE >= 8
	if ((slack==0)||(slack>=8)) {
#else
	if ((slack>=0)&& bOut) {
#endif
		*rb++ = rb[MAX_BUFFER_SIZE>>2] = cpu_to_le32(d0);	//set default response as well
		*rb++ = rb[MAX_BUFFER_SIZE>>2] = cpu_to_le32(d1);

		if (lengthInPtd) {
			if (bOut) rb = update_memcpy_align(rb,buf,lengthInPtd);	//copy data here directly instead of using pci_map_single
			else {
//				printk("sa:%d",lengthInPtd);
				rb = skip_align(rb,lengthInPtd);		//will copy data in update item instead of using pci_map_single
			}
		}
#if BURST_TRANSFER_SIZE >= 8
		len_trailing = lengthInPtd;
		if (((int)rb)&(BURST_TRANSFER_SIZE-1)) {
			*rb++ = 0;
			*rb = cpu_to_le32(X1PTD_ALLOTED(-( ((int)rb)+4) & (BURST_TRANSFER_SIZE-1)));
			rb++;
			while ( ((int)rb)&(BURST_TRANSFER_SIZE-1) ) *rb++ = 0;
		}
#endif
		rbTrail = (char*)rb;
	}
	else {
#if BURST_TRANSFER_SIZE==16
		*rb++ = 0;
		*rb++ = 0;
#endif
		*rb++ = rb[MAX_BUFFER_SIZE>>2] = cpu_to_le32(d0);	//set default response as well
		*rb++ = rb[MAX_BUFFER_SIZE>>2] = cpu_to_le32(d1);
		len_virt = lengthInPtd;	//amount of space used for transaction
		rbTrail = (char*)rb;

#if BURST_TRANSFER_SIZE >= 8
		last_dmaRsp=last_dmaReq=1;
		dw->req[0].next = dw->dma_base + (int)( &((struct dmaWork*)0)->req[1].next);
		dw->rsp[0].next = dw->dma_base + (int)( &((struct dmaWork*)0)->rsp[1].next);

		if (bOut) {
			int ll;
			len_trailing =  ( ((lengthInPtd+3) &~3) & (BURST_TRANSFER_SIZE-1)) ? lengthInPtd & (BURST_TRANSFER_SIZE-1) : 0;
			len_virt -= len_trailing;

			ll = (len_virt+3) & ~3;	//only useful if trailing==0
			dw->req[1].source = dw->dmaAddr;
			dw->rsp[1].target = hci->trash;
			dw->req[1].cmd = ll + REQ_CMD_OUT;

			if (len_trailing) {
				last_dmaReq=2;
				dw->req[1].next = dw->dma_base + (int)( &((struct dmaWork*)0)->req[2].next);
				rb = update_memcpy_align(rb,((__u8*)buf)+len_virt,len_trailing);

				if (((int)rb)&(BURST_TRANSFER_SIZE-1)) {
					*rb++ = 0;
					*rb = cpu_to_le32(X1PTD_ALLOTED(-( ((int)rb)+4) & (BURST_TRANSFER_SIZE-1)));
					rb++;
					while ( ((int)rb)&(BURST_TRANSFER_SIZE-1) ) *rb++ = 0;
				}
				{
					int ll2 = ((char*)rb) - rbTrail;
					dw->req[2].source = dw->dma_base + ( rbTrail - ((char*)dw) );
					dw->req[2].cmd = ll2 + REQ_CMD_OUT;
					ll += ll2;
				}
			}
			dw->rsp[1].cmd = ll + RSP_CMD_OUT;
		} else {
////////////////////////////
			int ll;
			len_trailing = lengthInPtd & (BURST_TRANSFER_SIZE-1);
			len_virt -= len_trailing;

			ll = len_virt;
			dw->req[1].source = hci->zero;
			dw->rsp[1].target = dw->dmaAddr;
			dw->rsp[1].cmd = ll + RSP_CMD_IN;
			if (len_trailing) {
				last_dmaRsp=2;
				dw->rsp[1].next = dw->dma_base + (int)( &((struct dmaWork*)0)->rsp[2].next);
				rb = skip_align(rb,len_trailing); //will copy data in update item

				if (((int)rb)&(BURST_TRANSFER_SIZE-1)) {
					*rb++ = 0;
					*rb = cpu_to_le32(X1PTD_ALLOTED(-( ((int)rb)+4) & (BURST_TRANSFER_SIZE-1)));
					rb++;
					while ( ((int)rb)&(BURST_TRANSFER_SIZE-1) ) *rb++ = 0;

					last_dmaReq=2;
					dw->req[1].next = dw->dma_base + (int)( &((struct dmaWork*)0)->req[2].next);
					{
						int ll2 = ((char*)rb) - rbTrail;
						dw->req[2].source = dw->dma_base + ( rbTrail - ((char*)dw) );
						dw->req[2].cmd = ll2 + REQ_CMD_IN;
						dw->rsp[2].target = dw->dma_base + ( rbTrail - ((char*)dw) + MAX_BUFFER_SIZE );	//this data needs copied again in completion routine
						dw->rsp[2].cmd = ll2 + RSP_CMD_IN;
					}
				}
				else {
					int ll2 = ((char*)rb) - rbTrail;
					dw->rsp[2].target = dw->dma_base + ( rbTrail - ((char*)dw) + MAX_BUFFER_SIZE );	//this data needs copied again in completion routine
					dw->rsp[2].cmd = ll2 + RSP_CMD_IN;
					ll += ll2;
				}
			}
			dw->req[1].cmd = ll + REQ_CMD_IN;
		}

#endif
	}
	{
		int len_ptd = (rbTrail-rbstart);
#if BURST_TRANSFER_SIZE >= 8
		dw->req[0].cmd  = REQ_CMD_PTD + len_ptd;
		dw->rsp[0].cmd  = RSP_CMD_PTD + len_ptd;
		dw->len_trailing = len_trailing;
		dw->last_dmaReq_index = last_dmaReq;		//# of dmaDesc's used -1
		dw->last_dmaRsp_index = last_dmaRsp;
		dw->unit_count = ((((char*)rb)-rbstart) + len_virt + 3) & ~3;	//amount of space used for transaction
#else
		dw->unit_count = (len_ptd + len_virt + 3) & ~3;
#endif
		dw->len_virt = len_virt;
		dw->len_ptd = len_ptd;
//		printk("lengths: %d, %d:",len_ptd,len_virt);
	}
}

static inline int ChooseLength(int len,int maxps)
{
	if (len > 0x3ff) {
		if (len > 0x7ff)
			len = 0x7ff;
		len = (len>>1)  & ~( (MAX(BURST_TRANSFER_SIZE,maxps)<<1)-1);	//it will take at least 2 frames, so split between them
	}
	return len;
}
static inline void MapIfNeeded(struct dmaWork* dw,int* buf,int len, int bOut)
{
	dw->virt_buffer = (char*)buf;
	dw->remaining = len;
#if BURST_TRANSFER_SIZE >= 8
	{
		int slack = MAX_BUFFER_SIZE - ( ((len+3) &~3) + 8);	//MAX_BUFFER_SIZE is a multiple of BURST_TRANSFER_SIZE
		if ((slack==0)||(slack>=8))
			dw->dmaAddr = 0;
		else    dw->dmaAddr = pci_map_single(NULL, buf, len, (bOut) ?  PCI_DMA_TODEVICE: PCI_DMA_FROMDEVICE);
	}
#endif
}
static struct dmaWork* InitDmaWork1(struct dmaWork** pdw, int d0,int d1,int* buf,int len,hci_t * hci, urb_t * urb,int pipe,int pipetype,int maxps,int bOut)
{
	struct dmaWork* dw = *pdw;
	if (!dw) {
restart:
		*pdw = dw = AllocDmaWork(hci);
		if (dw) dw->cancel = 0;
	}
	if (dw) {
		if (dw->virt_next != NOT_IN_CHAIN) {
			printk(KERN_ERR __FILE__ ": chain corrupt in init\n");
			goto restart;
		}
		dw->owner_urb=urb;
		dw->iso_next = NULL;


		dw->transferred_cnt = 0;
		dw->pipe = pipe;
		dw->error_count = 0;
		dw->minsize = MAX(maxps,BURST_TRANSFER_SIZE);
		dw->cc = TD_INIT_VALUE;
		dw->itl_index = 0x7f;
		dw->pipetype = pipetype;
		MapIfNeeded(dw,buf,len,bOut);
		if (pipetype == PIPE_CONTROL) {
			buf = (int*)urb->setup_packet;  //the setup part with be copied, map_single not needed
		}
		InitDmaStruct(dw,d0,d1,buf,hci);
	}
	return dw;
}
int InitDmaWork(hci_t * hci, urb_t * urb)
{
	int len = urb->transfer_buffer_length;
	int* buf = urb->transfer_buffer;
	int pipe = urb->pipe;
	int bOut = usb_pipeout(pipe);
	int maxps = usb_maxpacket (urb->dev, pipe, bOut);
	int pipetype = usb_pipetype(pipe);
	int lengthInPtd;
	unsigned int d0,d1;
	int pid=bOut ? PID_OUT : PID_IN;
	struct dmaWork* dw;
	int i=0;

	if (pipetype == PIPE_BULK) {
		//maxps can be 8,16,32,64, fullspeed only (lowspeed not allowed)
		if (maxps<8) {
			printk(KERN_ERR __FILE__ ": maxps of %d is invalid\n",maxps);
			maxps = 8;
		}
		lengthInPtd = ChooseLength(len,maxps);
	} else if (pipetype == PIPE_CONTROL) {
		//maxps can be 8,16,32,64, fullspeed, 8 lowspeed
		if (maxps<8) {
			printk(KERN_ERR __FILE__ ": maxps of %d is invalid\n",maxps);
			maxps = 8;
		}
		pid = PID_SETUP;
		lengthInPtd = 8; //the setup part with be copied, map_single not needed
	} else {
		lengthInPtd = MIN(len,maxps);	//interrupt & iso types can only use 1 packet
		if (pipetype == PIPE_INTERRUPT) urb->start_frame = hci->frame_no;
	}

	d0 =    X0PTD_ACTIVE(1)      | X0PTD_CC(0xf) |
		X0PTD_MAXPS(maxps)   | X0PTD_SLOW(usb_pipeslow(pipe)) | X0PTD_ENDPOINT(usb_pipeendpoint(pipe));

	d1 =    X1PTD_ALLOTED(lengthInPtd)          | X1PTD_PID(pid) |
		X1PTD_ADDRESS(usb_pipedevice(pipe));

	if (pipetype == PIPE_ISOCHRONOUS) {
		if ((urb->transfer_flags & USB_ISO_ASAP)==0) i = hci->frame_no - urb->start_frame;
		if (i<urb->number_of_packets) {
			d1 |= X1PTD_FORMAT(1);
#if BURST_TRANSFER_SIZE >= 8
			{
				int j=i;
				do {
					buf = (int *)(((unsigned char *)urb->transfer_buffer) + urb->iso_frame_desc[j].offset);
					len = urb->iso_frame_desc[j].length;
					if ( (((int)buf) & 7) && (len>8)) {
						printk(KERN_ERR __FILE__ ": iso frame not aligned\n");
						return -EINVAL;
					}
					j++;
				} while (j<urb->number_of_packets);
			}
#endif

			buf = (int *)(((unsigned char *)urb->transfer_buffer) + urb->iso_frame_desc[i].offset);
			len = urb->iso_frame_desc[i].length;
			d1 = (d1 & ~F1PTD_ALLOTED) |  X1PTD_ALLOTED(MIN(len,maxps));
		} else return -EINVAL;
	}
	if ( (dw = InitDmaWork1((struct dmaWork**)(&urb->hcpriv),d0,d1,buf,len,hci,urb,pipe,pipetype,maxps,bOut))==0) return -ENOMEM;
	if (pipetype == PIPE_ISOCHRONOUS) {
		dw->cur_iso_frame_index = i;
		i++;
		if (i<urb->number_of_packets) {
			struct dmaWork* dp;
			buf = (int *)(((unsigned char *)urb->transfer_buffer) + urb->iso_frame_desc[i].offset);
			len = urb->iso_frame_desc[i].length;
			d1 = (d1 & ~F1PTD_ALLOTED) |  X1PTD_ALLOTED(MIN(len,maxps));
			if ( (dp = InitDmaWork1(&dw->iso_next,d0,d1,buf,len,hci,urb,pipe,pipetype,maxps,bOut))==0) return -ENOMEM;
			dp->cur_iso_frame_index = i;
		}
	}
	return 0;

}
static inline void SetState(struct dmaWork* dw,int state)
{
	dw->pipe &= ~0x3;
	dw->pipe |= state & 0x3;
}
static inline int GetState(struct dmaWork* dw)
{
	return dw->pipe & 0x3;
}


#if BURST_TRANSFER_SIZE >= 8
static inline void SetDmaL(struct dmaWork* dw,struct dmaWork* next)
{
	dw->req[dw->last_dmaReq_index].next = next->dma_base + (int)( &((struct dmaWork*)0)->req[0].next);
	dw->rsp[dw->last_dmaRsp_index].next = next->dma_base + (int)( &((struct dmaWork*)0)->rsp[0].next);
}
#endif
static inline void SetDmaLChk(struct dmaWork* dw,struct dmaWork* next)
{
#if BURST_TRANSFER_SIZE >= 8
	if (next) SetDmaL(dw,next);
#endif
}

static inline void SetDmaLinks(struct dmaWork* dw,struct dmaWork* next)
{
	dw->virt_next = next;
#if BURST_TRANSFER_SIZE >= 8
	SetDmaLChk(dw,next);
#endif
}
static inline void FixToggle(struct dmaWork* dw,int toggle)
{
	if (dw) {
		int* reqb = &dw->req_buffer[0];
#if BURST_TRANSFER_SIZE==16
		if (reqb[0]==0) {reqb+=2;}
#endif
		if (toggle) {
			reqb[0]  |= cpu_to_le32(F0PTD_TOGGLE);
			reqb[MAX_BUFFER_SIZE>>2]  |= cpu_to_le32(F0PTD_TOGGLE); //response also
		}
		else {
			reqb[0]  &= ~cpu_to_le32(F0PTD_TOGGLE);
			reqb[MAX_BUFFER_SIZE>>2]  &= ~cpu_to_le32(F0PTD_TOGGLE);
		}
	}
}
static inline int GetToggle(struct dmaWork* dw)
{
	if (dw) {
		int* reqb = &dw->req_buffer[0];
#if BURST_TRANSFER_SIZE==16
		if (reqb[0]==0) {reqb+=2;}
#endif
		return (reqb[0] & cpu_to_le32(F0PTD_TOGGLE));
	}
	return 0;
}
static void AddToWorkList1(struct dmaWork* dw,struct worklist* wl)
{
	struct dmaWork** pdc = &wl->chain;
	struct dmaWork* dAfter;
	int unit_count = dw->unit_count;
	int pipetype = dw->pipetype;
#if BURST_TRANSFER_SIZE >= 8
	struct dmaWork* dBefore=NULL;
#endif
	if (dw->virt_next != NOT_IN_CHAIN) {
		printk(KERN_ERR __FILE__ ": chain corrupt\n");
		return;
	}
	dAfter = *pdc;
	while (dAfter) {	//sort by pipetype, then length
		if (pipetype < dAfter->pipetype) break;
		if ((pipetype == dAfter->pipetype) && (unit_count < dAfter->unit_count)) break;
#if BURST_TRANSFER_SIZE >= 8
		dBefore = dAfter;
#endif
		pdc = &dAfter->virt_next;
		dAfter = *pdc;
	}
	SetDmaLinks(dw,dAfter);
#if BURST_TRANSFER_SIZE >= 8
	if (dBefore) SetDmaL(dBefore,dw);
#endif
	*pdc = dw;
}

//this is only used for iso transfers, so ignore toggle
//interrupts are locked out
//out: 2 - queued as partner, 1 - already has partner
int QueuePartner(hci_t * hci, urb_t * urb,urb_t * head)
{
	struct dmaWork* dh = (struct dmaWork*)head->hcpriv;
	struct dmaWork* dw;
	struct dmaWork* dp;
	struct worklist* wl;
	int i;
	if (dh->iso_next) return 1;

	dh->iso_next = dw = (struct dmaWork*)urb->hcpriv;
	dp = dw->iso_next;
	i = dh->itl_index;
	if (dp) dp->itl_index = i;

	i ^= (FRAME_ITL0 ^ FRAME_ITL1);
	dw->itl_index = i;
	if (i <= FRAME_ITL1 ) {
		wl = &hci->frames[i].work;
		AddToWorkList1(dw,wl);
	}
	return 2;
}
//	cur_iso_frame_index

//now add to chain of transactions waiting to be active
void AddToWorkList(hci_t * hci, urb_t * urb,int state)
{
	unsigned long flags;
	struct dmaWork* dw = (struct dmaWork*)urb->hcpriv;
	int pipetype = dw->pipetype;
	struct worklist* wl;

	SetState(dw,state);
	if (pipetype != PIPE_CONTROL) {
//no transfer descriptors are active for this endpoint
//the value stored in urb-dev has been updated.
		int toggle;
		if (pipetype == PIPE_ISOCHRONOUS) {
			wl = &hci->iso_new_work;
			spin_lock_irqsave (&wl->lock, flags);
			rmb();
			{
				if (dw->virt_next != NOT_IN_CHAIN) {
					printk(KERN_ERR __FILE__ ": chain corrupt\n");
				} else {
					dw->virt_next = wl->chain;
					wl->chain = dw;
				}
			}
			spin_unlock_irqrestore (&wl->lock, flags);
			return;
		}
		toggle = usb_gettoggle(urb->dev, usb_pipeendpoint(urb->pipe), usb_pipeout(urb->pipe));
		FixToggle(dw,toggle);
	}
	wl = &hci->frames[FRAME_ATL].work;
	spin_lock_irqsave (&wl->lock, flags);
	AddToWorkList1(dw,wl);
	spin_unlock_irqrestore (&wl->lock, flags);
}

void ScanNewIsoWork(hci_t * hci,int i1)
{
	struct dmaWork* dw = hci->iso_new_work.chain;
	if (dw) {
		int i2 = i1 ^(FRAME_ITL0 ^ FRAME_ITL1);
		struct worklist* wl1 = &hci->frames[i1].work;
		struct worklist* wl2 = &hci->frames[i2].work;
		hci->iso_new_work.chain = NULL;
		do {
			struct dmaWork* next = dw->virt_next;
			dw->virt_next = NOT_IN_CHAIN;
			dw->itl_index = i1;
			AddToWorkList1(dw,wl1);
			dw = dw->iso_next;
			if (dw) {
				dw->itl_index = i2;
				AddToWorkList1(dw,wl2);
				dw = dw->iso_next;
				if (dw) dw->itl_index = i1;
			}
			dw = next;
		} while (dw);
	}
}
static inline void SetCancel1(hci_t * hci,struct dmaWork* dw)
{
	dw->cancel=1;
	dw = dw->iso_next;
	if (dw) {
		dw->cancel = 1;
		dw = dw->iso_next;
		if (dw) dw->cancel = 1;
		hci->scanForCancelled=1;
	}
}
int SetCancel(hci_t * hci,urb_t * urb)
{
	struct dmaWork* dw = (struct dmaWork*) urb->hcpriv;
	if (dw) {
		SetCancel1(hci,dw);
		return dw->virt_next==NOT_IN_CHAIN;
	}
	return 1;
}
int IntervalStatus(urb_t * urb)
{
	struct dmaWork* dw = (struct dmaWork*) urb->hcpriv;
	if (dw) {
		if (dw->cc == TD_INTERVAL) return 1;
	}
	return 0;
}

/*-------------------------------------------------------------------------*/
/* functions only called in interrupt context */
int cancelled_request(urb_t* urb)
{
	struct dmaWork* dw = (struct dmaWork*)urb->hcpriv;
	return (dw) ? dw->cancel : 1;
}
void doprint(const char* title, unsigned d0, unsigned d1)
{
	if (d0 || d1) {
		int cc = PTD_CC(d0,d1);
		int actual = PTD_ACTUAL(d0,d1);
		if ( (cc==15) && (actual==0) ) {
			printk(KERN_DEBUG "%s: a:%i, ep:%i, pid:%i, t:%i,slow:%i,iso:%i,last:%i, len:%i, maxps:%i\n",title,
				PTD_ADDRESS(d0,d1), PTD_ENDPOINT(d0,d1), PTD_PID(d0,d1),  PTD_TOGGLE(d0,d1),
				PTD_SLOW(d0,d1), PTD_FORMAT(d0,d1), PTD_LAST(d0,d1),
				PTD_ALLOTED(d0,d1), PTD_MAXPS(d0,d1)
				 );
		}
		else {
			printk(KERN_DEBUG "%s: a:%i, ep:%i, pid:%i, t:%i,slow:%i,iso:%i,last:%i, len:%i, maxps:%i, cc:%i act:%i\n",title,
				PTD_ADDRESS(d0,d1), PTD_ENDPOINT(d0,d1), PTD_PID(d0,d1),  PTD_TOGGLE(d0,d1),
				PTD_SLOW(d0,d1), PTD_FORMAT(d0,d1), PTD_LAST(d0,d1),
				PTD_ALLOTED(d0,d1), PTD_MAXPS(d0,d1),
				cc, actual);
		}
	}
}

//keep disabled, for testing only
#if 0
#define ISP116x_INW(port) inw(port)
#define ISP116x_OUTW(val,port) outw(val,port)
#endif

//enable if byte swapping needed
#if 1
#define chk_cpu_to_le16(val) cpu_to_le16(val)
#define chk_le16_to_cpu(val) le16_to_cpu(val)
#else
#define chk_cpu_to_le16(val) (val)
#define chk_le16_to_cpu(val) (val)
#endif


#ifdef CONFIG_ISP116x_IO
//functions ISP116x_INW & ISP116x_OUTW need to be defined
#define my_inw(port)  chk_le16_to_cpu(ISP116x_INW( (volatile long*)port))
#define my_inw_le(port)  ISP116x_INW( (volatile long*)port)
#define my_inw_ns(port)  ISP116x_INW( (volatile long*)port)
#define my_insw(port,buf,length) ISP116x_INSW(port,(__u16*)buf,length)

#define my_outw(val,port) ISP116x_OUTW(chk_cpu_to_le16(val), (volatile int*)port)
#define my_outw_le(val,port) ISP116x_OUTW(val, (volatile int*)port)
#define my_outw_ns(val,port) ISP116x_OUTW(val, (volatile int*)port)
#define my_outsw(port,buf,length) ISP116x_OUTSW(port,(__u16*)buf,length)

#else
#define my_inw(port)  chk_le16_to_cpu(inw(port))
#define my_inw_le(port)  inw(port)
#define my_inw_ns(port)  inw(port)
#define my_insw(port,buf,length) insw((int)port,buf,length)

#define my_outw(val,port) outw(chk_cpu_to_le16(val),port)
#define my_outw_le(val,port) outw(val,port)
#define my_outw_ns(val,port) outw(val,port)
#define my_outsw(port,buf,length) outsw((int)port,buf,length)
#endif



static inline void ISP116x_INSW (port_t port, __u16 *buffer, int length)
{
	while (length) {
		*buffer++ = my_inw(port);
		length--;
	}
}
static inline void ISP116x_OUTSW (port_t port, __u16* buffer, int length)
{
	while (length) {
		my_outw(*buffer++,port);
		length--;
	}
}

#if 0
#define align_outsw(port,buf,length) my_outsw(port,buf,length)
#define align_insw(port,buf,length) my_insw(port,buf,length)
#else


static void align_outsw(port_t hcport, void* buf, int length)
{
	if ( (((int)buf) & 1) ==0) my_outsw(hcport,buf,length);
	else {
		__u8* buffer = (__u8*)buf;
		while (length>0) {
			int val = *buffer++;
			val |= (*buffer++)<<8;
			my_outw_le(val, hcport);
			length--;
		}
	}
}
static void align_insw(port_t hcport, void* buf, int length)
{
	if ( (((int)buf) & 1) ==0) my_insw(hcport,buf,length);
	else {
		__u8* buffer = (__u8*)buf;
		while (length>0) {
			int val = my_inw_le (hcport);
			*buffer++ = val;
			*buffer++ = val >> 8;
			length--;
		}
	}
}
#endif

#define Is_PID_IN(rb) (PTD_PID(cpu_to_le32(rb[0]),cpu_to_le32(rb[1]))==PID_IN)

#if 0
void DBG_STATUS(struct dmaWork* dw,int bReq)
{
//	if (PTD_ENDPOINT(dw->req_buffer[0],dw->req_buffer[1]) != 3) return;
#if BURST_TRANSFER_SIZE >= 8
	if (bReq) {
		printk(KERN_DEBUG "req:%d,%d,%d,%d,%d\n",dw->len_ptd,dw->len_virt,dw->len_trailing,dw->unit_count,dw->last_dmaReq_index);
		doprint(" ",dw->req_buffer[0],dw->req_buffer[1]);
	} else {
		printk(KERN_DEBUG "rsp:%d,%d,%d,%d,%d\n",dw->len_ptd,dw->len_virt,dw->len_trailing,dw->unit_count,dw->last_dmaRsp_index);
		doprint(" ",dw->rsp_buffer[0],dw->rsp_buffer[1]);
	}
#else
	if (bReq) {
		printk(KERN_DEBUG "req:%d,%d,%d\n",dw->len_ptd,dw->len_virt,dw->unit_count);
		doprint(" ",dw->req_buffer[0],dw->req_buffer[1]);
	} else {
		printk(KERN_DEBUG "rsp:%d,%d,%d\n",dw->len_ptd,dw->len_virt,dw->unit_count);
		doprint(" ",dw->rsp_buffer[0],dw->rsp_buffer[1]);
	}
#endif
}
#else
#define DBG_STATUS(dw,bReq) do {} while(0)
#endif
static inline void MarkTransferActive(hci_t* hci,struct dmaWork* dw,int chain,int transferState)
{
	hci->transfer.chain = chain;
	hci->transfer.progress = dw;
	hci->transfer.extra = (transferState & TFM_DmaExtraPending) ? AE_extra_pending : AE_extra_none;
	wmb();
	hci->transferState = transferState;
}

void FakeDmaReqTransfer(port_t hcport, int reqCount, struct dmaWork* dw, hci_t * hci,int transferState)
{
	int cnt=0;
	MarkTransferActive(hci,dw,REQ_CHAIN,transferState);
#if BURST_TRANSFER_SIZE >= 8
	while (dw && (cnt < reqCount)) {
		int i;
		char* rb = (char*)(&dw->req_buffer[0]);
		int pZero = hci->zero;
		int prb = dw->dma_base + (int)( &((struct dmaWork*)0)->req_buffer[0]);
		DBG_STATUS(dw,1);

		for (i=0; i <= dw->last_dmaReq_index; i++) {
			int len = dw->req[i].cmd & 0x7ff;	//can have a maximum value of 0x400 (1024) which is 1023 rounded up to 8 byte boundary
			if (len) {
				dma_addr_t src = dw->req[i].source;
				cnt += len;
				if (src == prb) {
					align_outsw(hcport, rb, (len+1)>>1);
					rb += len;
					prb += len;
				}
				else if (src == pZero) while (len>0) { my_outw_ns(0,hcport); len -= 2; }
				else align_outsw(hcport, dw->virt_buffer, (len+1)>>1);
			}
			if (dw->req[i].next & DDADR_STOP) {
				if (cnt==reqCount) {
					hci->transfer.progress = NULL;
					return;
				}
				goto error1;
			}
		}
		dw = dw->virt_next;
	}
error1:
#else
	while (dw && (cnt < reqCount)) {
		int bLast = dw->req_buffer[0] & cpu_to_le32(X0PTD_LAST(1));
		int len = dw->len_virt;
		DBG_STATUS(dw,1);
		if (Is_PID_IN(dw->req_buffer)) {
			if (bLast) {
				align_outsw(hcport, (char*)(&dw->req_buffer[0]), 8>>1);
				cnt += 8;
				if (cnt==reqCount) {
					hci->transfer.progress = NULL;
					return;
				}
				break;
			}
			align_outsw(hcport, (char*)(&dw->req_buffer[0]), dw->len_ptd>>1);
			cnt += dw->len_ptd;
			len = (len+3)&~3;
			cnt += len;
			while (len>0) { my_outw_ns(0,hcport); my_outw_ns(0,hcport); len-=4; }
		}
		else {
			align_outsw(hcport, (char*)(&dw->req_buffer[0]), dw->len_ptd>>1);
			cnt += dw->len_ptd;
			if (len) {
				if (len>>1) align_outsw(hcport, dw->virt_buffer, len>>1);
				if (len&1) {
					unsigned char* p = ((unsigned char*)(dw->virt_buffer)) + len -1;
					my_outw_le(*p,hcport);
					if (!bLast) len++;
				}
				if (!bLast) if (len&2) { my_outw_ns(0,hcport); len+=2;}
				cnt += len;
			}
			if (bLast) {
				if (cnt==reqCount) {
					hci->transfer.progress = NULL;
					return;
				}
				break;
			}
		}
		dw = dw->virt_next;
	}
#endif
	hci->transfer.progress = NULL;
	{
		int len = reqCount - cnt;
		while (len > 0) { my_outw_ns(0,hcport); len -= 2; }
	}
	printk(KERN_ERR __FILE__ ": Req count mismatch %i vs %i\n",cnt,reqCount);

}
void FakeDmaRspTransfer(port_t hcport, int rspCount, struct dmaWork* dw, hci_t * hci,int transferState)
{
	int cnt=0;
	MarkTransferActive(hci,dw,RSP_CHAIN,transferState);
#if BURST_TRANSFER_SIZE >= 8
	while (dw && (cnt < rspCount)) {
		int i;
		char* rb = (char*)(&dw->rsp_buffer[0]);
		int pTrash = hci->trash;
		int prb = dw->dma_base + (int)( &((struct dmaWork*)0)->rsp_buffer[0]);
		for (i=0; i <= dw->last_dmaRsp_index; i++) {
			int len = dw->rsp[i].cmd & 0x7ff;	//can have a maximum value of 0x400 (1024) which is 1023 rounded up to 8 byte boundary
			if (len) {
				dma_addr_t target = dw->rsp[i].target;
				cnt += len;
				if (target == prb) {
					align_insw(hcport, rb, (len+1)>>1);
					if (i==0) DBG_STATUS(dw,0);
					rb += len;
					prb += len;
				}
				else if (target == pTrash) while (len>0) { my_inw_ns(hcport); len -= 2; }
				else {
					if (len>>1) align_insw(hcport, dw->virt_buffer, len>>1);
					if (len&1) {
						unsigned char* p = ((unsigned char*)(dw->virt_buffer)) + len -1;
						*p = my_inw_le(hcport);
					}
				}
			}
			if (dw->rsp[i].next & DDADR_STOP) {
				if (cnt==rspCount) {
					hci->transfer.progress = NULL;
					return;
				}
				goto error1;
			}
		}
		dw = dw->virt_next;
	}
error1:
#else
	while (dw && (cnt < rspCount)) {
		int len = dw->len_virt;
		int bLast = dw->req_buffer[0] & cpu_to_le32(X0PTD_LAST(1));
		if (Is_PID_IN(dw->req_buffer)) {
			align_insw(hcport, (char*)(&dw->rsp_buffer[0]), dw->len_ptd>>1);
			DBG_STATUS(dw,0);
			cnt += dw->len_ptd;
			if (len) {
				if (len>>1) align_insw(hcport, dw->virt_buffer, len>>1);
				if (len&1) {
					unsigned char* p = ((unsigned char*)(dw->virt_buffer)) + len -1;
#if 1
					*p = my_inw_le(hcport);
#else
					unsigned short val = my_inw_le(hcport);
					*p = val;
					printk(KERN_ERR __FILE__ ": last word of odd length read: %4x,%4x",val,cpu_to_le16(val));
#endif

					if (!bLast) len++;
				}
				if (!bLast) if (len&2) { my_inw_ns(hcport); len+=2;}
				cnt += len;
			}
			if (bLast) {
				if (cnt==rspCount) {
					hci->transfer.progress = NULL;
					return;
				}
				break;
			}
		}
		else {
			if (bLast) {
				align_insw(hcport, (char*)(&dw->rsp_buffer[0]), 8>>1);
				DBG_STATUS(dw,0);
				cnt += 8;
				if (cnt==rspCount) {
					hci->transfer.progress = NULL;
					return;
				}
				break;
			}
			align_insw(hcport, (char*)(&dw->rsp_buffer[0]), dw->len_ptd>>1);
			DBG_STATUS(dw,0);
			cnt += dw->len_ptd;
			len = (len+3) & ~3;
			cnt += len;
			while (len>0) { my_inw_ns(hcport); my_inw_ns(hcport); len-=4; }
		}
		dw = dw->virt_next;
	}
#endif
	hci->transfer.progress = NULL;
	{
		int len = rspCount - cnt;
		while (len > 0) { my_inw_ns(hcport); len -= 2; }
	}
	printk(KERN_ERR __FILE__ ": Rsp count mismatch %i vs %i\n",cnt,rspCount);

}


/*-------------------------------------------------------------------------*/

#define LAST_PTD 1
#define STILL_ACTIVE 2
#define IDLE_BUT_ACTIVE 4
#if 0
		if (!MatchesReqRsp(d0,d1,d2,d3)) {
static inline int MatchesReqRsp(int d0,int d1,int d2, int d3)
{
	if (d1!=d3) return 0;
	if ((d0^d2)& ~0xffff)  return 0;
	return 1;
}
#endif

//out: bit 0 - item is last, bit 1 - item is still active, bit 2 - idle iso entry
static int update_item(struct dmaWork* dw, hci_t * hci)
{
	int* reqb = &dw->req_buffer[0];
	int* rspb = &dw->rsp_buffer[0];
	int state = GetState(dw);
	unsigned int d0,d1,d2,d3;
	int cc;
	int ret;
	int actbytes;
#if BURST_TRANSFER_SIZE==16
	if (reqb[0]==0) {reqb+=2; rspb+=2;}
#endif
	d0 = cpu_to_le32(reqb[0]);
	d1 = cpu_to_le32(reqb[1]);
	d2 = cpu_to_le32(rspb[0]);
	d3 = cpu_to_le32(rspb[1]);
	cc = PTD_CC(d2,d3);
//	doprint(" ",d2,d3);
#if 0
	if (cc) if (cc != TD_NOTACCESSED) if (cc != TD_DATAUNDERRUN)	//underrun means short packet, this is normal
	{
		doprint(" ",d2,d3);
	}
#endif
	actbytes = PTD_ACTUAL(d2,d3);

////////////////////////////////// iso entry
	if (d1 & X1PTD_FORMAT(1)) {		//if this is an ISO frame


		if ((d1!=d3) || ((d0^d2)& ~0xffff) ) {
#if DEBUG_LEVEL > 0
			if (actbytes) {
				printk(KERN_DEBUG __FILE__ ": iso request %8.8x,%8.8x doesn't match response %8.8x,%8.8x\n",d0,d1,d2,d3);
				actbytes = 0;
			}
			doprint("misReq",d0,d1); //request
			doprint("misRsp",d2,d3); //response
#else
			actbytes = 0;
#endif
			cc= TD_NOTACCESSED;	//	cc=TD_BAD_RESPONSE;
		}
#if 0
		else {
			if (d0!=d2) {doprint("goodRsp",d2,d3); }//response
		}
#endif

#if BURST_TRANSFER_SIZE >= 8
		if (actbytes) {
			if (usb_pipein(dw->pipe)) {
				int len = actbytes - dw->len_virt;
				if (len > 0) {
					if (len > dw->len_trailing) {
						printk(KERN_ERR __FILE__ ": too many bytes;actual:%d, len_virt:%d, len_trailing:%d\n",actbytes,dw->len_virt,dw->len_trailing);
						len = dw->len_trailing;
					}
					memcpy(dw->virt_buffer + dw->len_virt, &rspb[2], len);
				}
			}
		}
		{
			dma_addr_t addr = dw->dmaAddr;
			if (addr) {
				dw->transferred_cnt = dw->dmaAddr = 0;
				pci_unmap_single(NULL, addr , dw->remaining, (usb_pipein(dw->pipe)) ?  PCI_DMA_FROMDEVICE: PCI_DMA_TODEVICE);
			}
		}
#endif
		{
			int i = dw->cur_iso_frame_index;
			urb_t * urb = dw->owner_urb;
//			printk("!%i",i);
			urb->iso_frame_desc[i].actual_length = actbytes;
//			if (actbytes) printk("DATA!!!!%i",actbytes);
			urb->iso_frame_desc[i].status = 0;
			urb->actual_length += actbytes;
			dw->cc=0;

			dw->cur_iso_frame_index = i += 2;
			if (i<urb->number_of_packets) {
				int* buf = (int *)(((unsigned char *)urb->transfer_buffer) + urb->iso_frame_desc[i].offset);
				int len = urb->iso_frame_desc[i].length;
				len = MIN(len,PTD_MAXPS(d0,d1));
				if (len) {
					d0 &= ~X0PTD_LAST(1);	//toggle never changes for isochronous
					d1 = (d1 & ~F1PTD_ALLOTED) |  X1PTD_ALLOTED(len);
					MapIfNeeded(dw,buf,len,usb_pipeout(dw->pipe));
					InitDmaStruct(dw,d0,d1,buf,hci);
					return STILL_ACTIVE;	//never return last, even if entry
								//didn't make it on the bus, we still advance
				} else {
					//it wants to be idle for this frame
					reqb[0]  = cpu_to_le32( (d0 & ~(F0PTD_ACTUAL|F0PTD_LAST)));
					rspb[0]  = cpu_to_le32( (d2 & ~(F0PTD_ACTUAL|F0PTD_LAST)));
					return IDLE_BUT_ACTIVE;
				}
			}
			return 0;
		}
	}

////////////////////////////////// atl entry
	ret = PTD_LAST(d0,d1);
	{
		if ((d1!=d3) || ((d0^d2)& ~0xffff) ) {
#if DEBUG_LEVEL > 0
			printk(KERN_DEBUG __FILE__ ": request %8.8x,%8.8x doesn't match response %8.8x,%8.8x, tx:%i of %i\n",d0,d1,d2,d3,dw->transferred_cnt,dw->transferred_cnt+dw->remaining);
			doprint("misReq",d0,d1); //request
			doprint("misRsp",d2,d3); //response
#endif
			cc= TD_NOTACCESSED;	//	cc=TD_BAD_RESPONSE;
			d2 = d0 ^ X0PTD_TOGGLE(1);	//mess up toggle, like other errors are
			actbytes = 0;
		}
	}
	if (cc != TD_NOTACCESSED) {
		int active;
		if (state == US_CTRL_SETUP) {
			if (!cc) { // no error
				if (dw->remaining==0) {
					state = US_CTRL_DATA;	//NO data phase (so set data phase to completed)
					actbytes = 0;
				} else {
					SetState(dw, --state);
					d0 |= X0PTD_TOGGLE(1);
					d0 &= ~X0PTD_LAST(1);
					d1 = (d1 & ~(F1PTD_PID | F1PTD_ALLOTED)) |  X1PTD_ALLOTED(ChooseLength(dw->remaining,PTD_MAXPS(d0,d1))) |
						X1PTD_PID( (usb_pipein(dw->pipe))? PID_IN : PID_OUT);
					InitDmaStruct(dw,d0,d1,(int*)dw->virt_buffer,hci);
//					doprint("!!!",d0,d1);
					return ret|STILL_ACTIVE;
				}
			}
		}
		else {
			if (actbytes) {
#if BURST_TRANSFER_SIZE >= 8
				if (usb_pipein(dw->pipe)) {
					int len = actbytes - dw->len_virt;
					if (len > 0) {
						if (len > dw->len_trailing) {
							printk(KERN_ERR __FILE__ ": too many bytes;actual:%d, len_virt:%d, len_trailing:%d\n",actbytes,dw->len_virt,dw->len_trailing);
							len = dw->len_trailing;
						}
//						printk("len_virt:%d, len_trailing:%d ",dw->len_virt, len);
//						doprint("...",d0,d1);
						memcpy(dw->virt_buffer + dw->len_virt, &rspb[2], len);
					}
				}
#endif
				dw->virt_buffer += actbytes;
				dw->transferred_cnt += actbytes;
#if BURST_TRANSFER_SIZE >= 8
				dw->dmaAddr += actbytes;
#endif
				dw->remaining -= actbytes;
				dw->error_count = 0;
			}
		}
		active = actbytes && (dw->remaining > 0);

		if (cc == TD_DATAUNDERRUN) {
			active = 0;	//short packet ends transaction
			if (!(dw->owner_urb->transfer_flags & USB_DISABLE_SPD)) cc = 0;
		}


		if (cc) { /* last packet has an error */
			dw->cc=cc;
			if (hc_error_verbose)
				printk("hc USB %d error %d ep %d out %d addr %d\n",
					dw->error_count, cc,
						usb_pipeendpoint(dw->pipe),
						usb_pipeout(dw->pipe), usb_pipedevice(dw->pipe));
			d2 ^= X0PTD_TOGGLE(1);	//fix toggle
			rspb[0] = cpu_to_le32(d2);
			if (++dw->error_count > 3 || cc == TD_CC_STALL) {
#if DEBUG_LEVEL > 0
				if (hci->errorCnt++<10)
					doprint("err",d2^X0PTD_TOGGLE(1),d3);	//print original response toggle
#endif
				return ret;
			}
			active = 1;
		}

		d0 = (d0 & ~F0PTD_TOGGLE) | (d2 & F0PTD_TOGGLE);
		d1 = (d1 & ~F1PTD_ALLOTED) | X1PTD_ALLOTED(ChooseLength(dw->remaining,PTD_MAXPS(d0,d1)));

		if (!active) {
			if (!state) {
				dw->cc=cc;
				return ret;	//done
			}
			SetState(dw, --state);
			if ((state==US_CTRL_ACK) && (usb_pipetype(dw->pipe)==PIPE_CONTROL)) {
				d0 |= X0PTD_TOGGLE(1);
				d1 = (d1 & ~(F1PTD_PID | F1PTD_ALLOTED)) | X1PTD_PID((usb_pipein(dw->pipe))? PID_OUT : PID_IN); //reply in opposite direction
			}
		}
		d0 &= ~X0PTD_LAST(1);
		InitDmaStruct(dw,d0,d1,(int*)dw->virt_buffer,hci);
//		doprint("???",d0,d1);
	} else {
		if ((usb_pipetype(dw->pipe)==PIPE_INTERRUPT) && usb_pipein(dw->pipe)) {
			urb_t * urb = dw->owner_urb;
			if (urb && urb->interval) {
				dw->cc = TD_INTERVAL;
				return ret;
			}
		}
		if (ret) {
			d0 &= ~X0PTD_LAST(1);	//make sure Last is not set
			if (state != US_CTRL_SETUP) InitDmaStruct(dw,d0,d1,(int*)dw->virt_buffer,hci);	//reset dma links
			else
			{
				reqb[0] = cpu_to_le32(d0);
#if BURST_TRANSFER_SIZE >= 8
				dw->rsp[0].cmd = (dw->rsp[0].cmd & ~0x1fff)|(dw->req[0].cmd & 0x1fff);
#endif
			}
		}
	}
	return ret|STILL_ACTIVE;
}

//assumed: last_dmaReq_index >= 1
void ChangeUsage(struct dmaWork* dw,int allocSize)
{
	int* reqb = &dw->req_buffer[0];
	int d1;
#if BURST_TRANSFER_SIZE==16
	if (reqb[0]==0) {reqb+=2;}
#endif
	d1 = cpu_to_le32(reqb[1]);
	d1 = (d1 & ~F1PTD_ALLOTED) | X1PTD_ALLOTED(allocSize);
	reqb[1] = cpu_to_le32(d1);
#if BURST_TRANSFER_SIZE >= 8
	dw->req[1].cmd = (dw->req[1].cmd & ~0x1fff)|allocSize;
	dw->rsp[1].cmd = (dw->rsp[1].cmd & ~0x1fff)|allocSize;
	if (dw->last_dmaReq_index != 1) {
		dw->req[1].next = dw->req[dw->last_dmaReq_index].next;
		dw->last_dmaReq_index = 1;
	}
	if (dw->last_dmaRsp_index != 1) {
		dw->rsp[1].next = dw->rsp[dw->last_dmaRsp_index].next;
		dw->last_dmaRsp_index = 1;
	}
	dw->len_trailing = 0;
#endif
	dw->unit_count = MAX(BURST_TRANSFER_SIZE,8) + allocSize;
	dw->len_virt = allocSize;
}
//assumed: last_dmaReq_index >= 1
void ChangeUsageLast(struct dmaWork* dw,int allocSize,int bIn)
{
	int* reqb = &dw->req_buffer[0];
	int d1;
#if BURST_TRANSFER_SIZE==16
	if (reqb[0]==0) {reqb+=2;}
#endif
	d1 = cpu_to_le32(reqb[1]);
	d1 = (d1 & ~F1PTD_ALLOTED) | X1PTD_ALLOTED(allocSize);
	reqb[1] = cpu_to_le32(d1);
#if BURST_TRANSFER_SIZE >= 8
	if (bIn) {
		dw->rsp[1].cmd = (dw->rsp[1].cmd & ~0x1fff)|allocSize;
		dw->last_dmaReq_index = 0;
		dw->last_dmaRsp_index = 1;
	} else {
		dw->req[1].cmd = (dw->req[1].cmd & ~0x1fff)|allocSize;
		dw->last_dmaRsp_index = 0;
		dw->last_dmaReq_index = 1;
	}
	dw->len_trailing = 0;
#endif
	dw->unit_count = MAX(BURST_TRANSFER_SIZE,8) + allocSize;
	dw->len_virt = allocSize;
}

static int inline GetNewSize(struct dmaWork* dw,int max)
{
	int lengthInPtd = dw->orig_lengthInPtd;
	if (lengthInPtd > max) lengthInPtd = max & ~(dw->minsize-1);
	return lengthInPtd;
}
static int inline GetNewSizeMiddle(struct dmaWork* dw,int max)
{
	int lengthInPtd = dw->orig_lengthInPtd;
	if (lengthInPtd > max) lengthInPtd = max;

	lengthInPtd &= ~(dw->minsize-1);
#if BURST_TRANSFER_SIZE==16
	lengthInPtd &= ~(BURST_TRANSFER_SIZE-1);
#endif
	return lengthInPtd;
}

static inline struct dmaWork* GetHead(struct dmaWork** pdw)
{
	struct dmaWork* dwHead = *pdw;
	if (dwHead) {
		*pdw = NULL;
	}
	return dwHead;
}

static void ReturnWork(hci_t * hci, struct frameList* fl, struct dmaWork* dw)
{
	int* rspb = &dw->rsp_buffer[0];
	int bNeedMore=0;
	int toggle;
	int cc;
	int itl_index = dw->itl_index;
	int resubmit_ok = !dw->cancel;
	urb_t * urb = dw->owner_urb;
	urb_t * urb_next = NULL;
	struct dmaWork* dh;

	if (!urb) {
		printk(KERN_ERR __FILE__ ": no urb\n");
		return;
	}
#if BURST_TRANSFER_SIZE==16
	if (rspb[0]==0) {rspb+=2;}
#endif
	if (dw->cc < TD_INIT_VALUE) if (urb->dev) {
		toggle = PTD_TOGGLE(cpu_to_le32(rspb[0]),cpu_to_le32(rspb[1]));
		usb_settoggle (urb->dev, usb_pipeendpoint (urb->pipe),usb_pipeout (urb->pipe),toggle);
	}
	urb->actual_length += dw->transferred_cnt;
#if BURST_TRANSFER_SIZE >= 8
	{
		dma_addr_t addr = dw->dmaAddr - dw->transferred_cnt;
		if (addr) {
			dw->transferred_cnt = dw->dmaAddr = 0;
			pci_unmap_single(NULL, addr , urb->transfer_buffer_length, (usb_pipein(urb->pipe)) ?  PCI_DMA_FROMDEVICE: PCI_DMA_TODEVICE);
		}
	}
#endif
	cc = dw->cc;
	dh = urb->hcpriv;
	if ( dh != dw) {
		//this is an ISO urb that has been cancelled or odd # of frames
		struct dmaWork* dp;
		if (dh->iso_next != dw) {
			printk(KERN_ERR __FILE__ ": corrupt links\n");
			return;
		}
		dh->iso_next = dp = dw->iso_next;	//should always be NULL
		dw->iso_next = NULL;
		FreeDmaWork(hci,dw);
		urb_next = urb;
		if (dp) {				//should always be NULL
			dp->itl_index = itl_index;
			AddToWorkList1(dp,&fl->work);
			return;
		} else bNeedMore=1;
	} else {
		urb_t* uhead = qu_getPipeHead(hci,urb);
		dh = dw->iso_next;
		dw->iso_next = NULL;
		if (uhead != urb) if (uhead) {
			struct dmaWork* dhead = uhead->hcpriv;
			if ( dhead->iso_next != dw) {
				printk(KERN_ERR __FILE__ ": corrupt links3\n");
				return;
			}
			dhead->iso_next = dh;
		}
		if (dh) {
			struct dmaWork* dp = dh->iso_next;
			urb_next = dh->owner_urb;
			if (urb_next == urb) {
				urb->hcpriv = dh;
				FreeDmaWork(hci,dw);
			}
			if (dh->virt_next == NOT_IN_CHAIN) dp = dh;	//2nd URB removed before 1st URB, dw is not the pipe head
			if (dp) {
				dp->itl_index = itl_index;
				AddToWorkList1(dp,&fl->work);
				if (urb_next == urb) return;
			} else bNeedMore=1;
		}
	}
	if (urb != urb_next) {
		urb->status = cc_to_error[cc];
#if DEBUG_LEVEL > 0
		if (cc && (cc!=TD_INTERVAL))
			if (hci->errorCnt++<10)
				 printk(KERN_DEBUG __FILE__ ": urb status %d, cc:%d, good bytes before error: %i of %i\n",urb->status,cc,urb->actual_length,urb->transfer_buffer_length);
#endif
	}
	qu_return_urb(hci, urb, urb_next, bNeedMore, resubmit_ok); //(qu_return_urb removes URB from active list)
}

static inline int ScheduleLoop(hci_t * hci, struct frameList* fl, struct ScheduleData * sd,struct dmaWork* dw)
{
	int val = STILL_ACTIVE;
	while (1) {
		if (!dw) {
			dw = GetHead(&fl->work.chain);	//processing the responses may generate more work
			if (dw) val = STILL_ACTIVE+LAST_PTD; //not response pending yet
			else {
				dw = GetHead(&fl->chain);
				if (dw) val = STILL_ACTIVE;
				else break;
			}
			*sd->pdw = dw;
#if BURST_TRANSFER_SIZE >= 8
			if (sd->dBefore) SetDmaL(sd->dBefore,dw);
#endif
		}
		if ((val&LAST_PTD)==0) {
#if BURST_TRANSFER_SIZE >= 8
			if (hci->transfer.progress == dw)
			{
				//check to make sure DMA response has passed this point
#if 1
				rmb();
#else
				int i;
				for (i=0; i<30000; i++) {
					rmb();
					if (hci->transfer.progress != dw) break;
				}
#endif
				if (hci->transfer.progress == dw) {
					*sd->pdw = NULL;
					fl->chain = dw;
					return 1;
				}
			}
#endif
			val = update_item(dw,hci);
#if BURST_TRANSFER_SIZE >= 8
			if (val & STILL_ACTIVE) SetDmaLChk(dw,dw->virt_next);
#endif
			if (val & IDLE_BUT_ACTIVE) {
				//remove from chain
				struct dmaWork* next;
				*sd->pdw = next = dw->virt_next;
				dw->virt_next = NULL;	//new end of idle chain
#if BURST_TRANSFER_SIZE >= 8
				if (sd->dBefore) SetDmaLChk(sd->dBefore,next);
#endif
				*sd->pIdledw = dw;
				sd->pIdledw = &dw->virt_next;
#if BURST_TRANSFER_SIZE >= 8
				if (sd->lastIdle) SetDmaL(sd->lastIdle,dw);
#endif
				sd->lastIdle = dw;
				dw = next;
				continue;
			}
		}
		if ( ((val & STILL_ACTIVE)==0) || (dw->cancel) ){
			//remove from chain
remove_from_chain:
			{
				struct dmaWork* next;
				*sd->pdw = next = dw->virt_next;
				dw->virt_next = NOT_IN_CHAIN;
#if BURST_TRANSFER_SIZE >= 8
				if (sd->dBefore) SetDmaLChk(sd->dBefore,next);
#endif
				ReturnWork(hci,fl,dw);
				dw = next;
			}
		} else {
			if (!sd->dLast) {
//				printk("u%d",dw->unit_count);
				sd->units_left -= dw->unit_count;
				if (sd->units_left < 0) {
					do {
						struct dmaWork* h;
						if (sd->unstartedIndex) h = sd->unstartedReads[--(sd->unstartedIndex)];
						else if (dw->len_virt > dw->minsize) h = dw;
						else if (sd->otherIndex) h= sd->other[--(sd->otherIndex)];
						else {
							sd->dLast = sd->dBefore;
							sd->units_left += dw->unit_count;
							if (dw->iso_next || (sd->dBefore == NULL)) {	//has any item been scheduled?
no_room:
								printk(KERN_ERR __FILE__ ": no room or too big: %i\n",dw->unit_count);
								dw->cc = TD_TOO_BIG; //this entry will never fit
								SetCancel1(hci,dw);
								goto remove_from_chain;
							}
							break;
						}

						sd->units_left += h->unit_count;
//						printk("Change usage from %d to %d, units_left:%d, limit:%d\n",h->len_virt,h->minsize,sd->units_left,fl->limit);
						ChangeUsage(h,h->minsize);
						sd->dwLastReduced = h;
						sd->units_left -= h->unit_count;
					} while (sd->units_left < 0);
				}
				if (dw->len_virt > dw->minsize) {
					if (dw->len_virt >= 0x300 ) { sd->dLarge = dw; sd->dLargeBefore = sd->dBefore;}
					if ( usb_pipein(dw->pipe) && (dw->transferred_cnt==0) && (sd->unstartedIndex<MAX_UNSTARTED) ) {
						sd->unstartedReads[sd->unstartedIndex++] = dw;
					}
					else {
						if (sd->otherIndex >= MAX_OTHER) sd->otherIndex = MAX_OTHER-1;
						sd->other[sd->otherIndex++] = dw;
					}
				}
			} else if (dw->iso_next) goto no_room;
			sd->dBefore = dw;
			sd->pdw = &dw->virt_next;
			dw = *sd->pdw;
		}
	}
	return 0;
}
static struct dmaWork* RemoveIfCancelled(hci_t * hci, struct frameList* fl, struct dmaWork** pdw)
{
	struct dmaWork* dw = *pdw;
	struct dmaWork* dBefore = NULL;
	while (dw) {
		if (dw==NOT_IN_CHAIN) {
			printk(KERN_ERR __FILE__ ": ....chain corrupt\n");
			*pdw = NULL;
			break;
		}
		if (dw->cancel) {
			struct dmaWork* next;
			*pdw = next = dw->virt_next;
			dw->virt_next = NOT_IN_CHAIN;
#if BURST_TRANSFER_SIZE >= 8
			if (dBefore) SetDmaLChk(dBefore,next);
#endif
			ReturnWork(hci,fl,dw);
			dw = next;
		} else {
			dBefore = dw;
			pdw = &dw->virt_next;
			dw = *pdw;
		}
	}
	return dBefore;
}
void  RemoveCancelled(hci_t * hci, struct frameList* fl)
{
	struct dmaWork* dwHead;
	struct dmaWork* dBefore;
	RemoveIfCancelled(hci,fl,&fl->idle_chain);
	if (fl->state != STATE_READ_RESPONSE) {
		if ((hci->transfer.fl != fl)||(hci->transfer.progress==NULL)) {
			RemoveIfCancelled(hci,fl,&fl->chain);
			fl->state = STATE_SCHEDULE_WORK;
		}
	}
	do {
		dwHead = GetHead(&fl->work.chain);
		if (!dwHead) {
			dwHead = GetHead(&fl->dwHead);
			if (!dwHead) break;
		}
		dBefore = RemoveIfCancelled(hci,fl,&dwHead);
		if (dBefore) {
			struct dmaWork* next = fl->work.chain;
			fl->work.chain = dwHead;
			if (!next) {
				next = GetHead(&fl->dwHead);
			}
			dBefore->virt_next = next;
#if BURST_TRANSFER_SIZE >= 8
			SetDmaLChk(dBefore,next);
#endif
			if (!next) break;
		}
	} while (1);
}

int ScheduleWork(hci_t * hci, struct frameList* fl,struct ScheduleData * sdata)
{
	int reqCount = 0;
	int rspCount = 0;
	register struct dmaWork* dw;
	register struct ScheduleData * sd = sdata;
	if (fl->dwHead==NULL) {
		fl->dwHead = dw = GetHead(&fl->idle_chain);

		sd->pdw = &fl->dwHead;
		sd->pIdledw = &fl->idle_chain;
		sd->lastIdle = NULL;

		sd->dLarge = NULL;
		sd->dLargeBefore = NULL;

		sd->dBefore = NULL;
		sd->dLast = NULL;
		sd->dwLastReduced = NULL;

		sd->units_left=fl->limit;
		sd->unstartedIndex=0;
		sd->otherIndex=0;
	} else {
		dw = NULL;
	}

	if (ScheduleLoop(hci,fl,sd,dw)) return 1;
///////////////////////////////////////////////////////////
// now special processing for last

	dw = sd->dLast;
	if (!dw) dw = sd->dBefore;
	if (dw) {
		int* reqb;
		int allocSize;
		int bIn;

		if (sd->dLarge && (sd->dLarge != dw)) {
			//move it to last
			if (sd->dLargeBefore) {
				//remove from chain
				SetDmaLinks(sd->dLargeBefore,sd->dLarge->virt_next);
			} else {
				fl->dwHead = sd->dLarge->virt_next;
			}

			//insert after dw
			SetDmaLinks(sd->dLarge,dw->virt_next);
			SetDmaLinks(dw,sd->dLarge);
			dw = sd->dLarge;
		}


		reqb = &dw->req_buffer[0];
#if BURST_TRANSFER_SIZE==16
		if (reqb[0]==0) {reqb+=2;}
#endif
		bIn= (Is_PID_IN(reqb))? 1: 0;
		sd->units_left += dw->unit_count;
#if BURST_TRANSFER_SIZE >= 8
		if (dw->last_dmaReq_index) {
			sd->units_left -= MAX(BURST_TRANSFER_SIZE,8);
			allocSize = GetNewSize(dw,sd->units_left);
			ChangeUsageLast(dw,allocSize,bIn);
		} else {
			sd->units_left -= 8;
			allocSize = GetNewSize(dw,sd->units_left);
			if (bIn) {
				dw->req[0].cmd = (dw->req[0].cmd & ~0x1fff)|8;
				dw->rsp[0].cmd = (dw->rsp[0].cmd & ~0x1fff)|(allocSize+8);
			} else {
				dw->req[0].cmd = (dw->req[0].cmd & ~0x1fff)|(allocSize+8);
				dw->rsp[0].cmd = (dw->rsp[0].cmd & ~0x1fff)|8;
			}
			dw->unit_count = 8 + allocSize;
		}
#else
		sd->units_left -= 8;
		if (dw->len_virt) {
			allocSize = GetNewSize(dw,sd->units_left);
			{
				int d1 = cpu_to_le32(reqb[1]);
				d1 = (d1 & ~F1PTD_ALLOTED) | X1PTD_ALLOTED(allocSize);
				reqb[1] = cpu_to_le32(d1);
			}
//			printk("va:%d,%d:",dw->len_virt,allocSize);
			dw->len_virt = allocSize;
		} else {
//			printk("vb:%d,%d:",dw->len_ptd-8,allocSize);
			allocSize = dw->len_ptd - 8;
		}
		dw->unit_count = 8 + allocSize;
#endif

		sd->units_left -= allocSize;
		if (sd->dwLastReduced && (sd->dwLastReduced!=dw)) {
			//try to increase usage
			sd->units_left += sd->dwLastReduced->unit_count;
			{
				int newSize = GetNewSizeMiddle(sd->dwLastReduced,sd->units_left - MAX(BURST_TRANSFER_SIZE,8));
				if (newSize > sd->dwLastReduced->minsize) {
					ChangeUsage(sd->dwLastReduced,newSize);
				}
			}
			sd->units_left -= sd->dwLastReduced->unit_count;
		}

		{
			int i = fl->limit - sd->units_left;
			if (bIn) {
				reqCount = i-allocSize;
				rspCount = i;
#if BURST_TRANSFER_SIZE >= 8
//				dw->req[dw->last_dmaReq_index].cmd |= DCMD_ENDIRQEN;
//				dw->rsp[dw->last_dmaRsp_index].cmd |= DCMD_ENDIRQEN;
#endif
//				printk("i%d,%d",i,allocSize);
			} else {
				reqCount = i;
				rspCount = i-allocSize;
//				printk("o%d,%d",i,allocSize);
			}
			reqb[MAX_BUFFER_SIZE>>2] = reqb[0] |= LAST_MARKER;		//set LAST
#if BURST_TRANSFER_SIZE >= 8
			dw->req[dw->last_dmaReq_index].next = DDADR_STOP;
			dw->rsp[dw->last_dmaRsp_index].next = DDADR_STOP;
#endif
		}
	}
	fl->reqCount = reqCount;
	fl->rspCount = rspCount;
	fl->chain = fl->dwHead;
	fl->dwHead = NULL;
	if ( (reqCount<0) || (rspCount<0) ) {
		printk(KERN_ERR __FILE__ ": very weird, reqCount:%d, rspCount:%d\n",reqCount,rspCount);
	}
	if ( (reqCount || rspCount) && (fl->chain==NULL)) {
		printk(KERN_ERR __FILE__ ": very weird, reqCount:%d, rspCount:%d, but no chain\n",reqCount,rspCount);
	}
	return 0;
}


#ifdef USE_DMA
#if BURST_TRANSFER_SIZE >= 8
static inline struct dmaWork* UpdateDmaPosition(hci_t * hci,struct frameList* fl,int ch)
{
	struct dmaWork* dw = hci->transfer.progress;
//	printk("curr:%8p",dw);
	dma_addr_t nextDDADR = DDADR(ch);	//the current transfer in progress is the descriptor that points to this one
	if (hci->transfer.chain==REQ_CHAIN) {
		while (dw) {
			int i;
			struct dmaDesc* ds = &dw->req[0];
			for (i=0; i<=dw->last_dmaReq_index; i++,ds++) {
				if (ds->next == nextDDADR) {
					if (hci->transfer.progress != dw) {
//						printk("new:%8p\n",dw);
						hci->transfer.progress= dw;
						hci->intHappened = 1;
						if (hci->bhActive==0) tasklet_schedule(&hci->bottomHalf);
					}
					return dw;
				}
//				printk("next:%8x,",ds->next);
			}
			dw = dw->virt_next;
		}
	} else {
		while (dw) {
			int i;
			struct dmaDesc* ds = &dw->rsp[0];
			for (i=0; i<=dw->last_dmaRsp_index; i++,ds++) {
				if (ds->next == nextDDADR) {
					if (hci->transfer.progress != dw) {
//						printk("new:%8p\n",dw);
						hci->transfer.progress= dw;
						hci->intHappened = 1;
						if (hci->bhActive==0) tasklet_schedule(&hci->bottomHalf);
					}
					return dw;
				}
//				printk("next:%8x,",ds->next);
			}
			dw = dw->virt_next;
		}
	}
	printk(KERN_ERR __FILE__ ": dma Links are messed up; chain:%i, DDADR:%8x\n",hci->transfer.chain,nextDDADR);
	dw = hci->transfer.progress;
	while (dw) {
		printk(KERN_ERR __FILE__ ":req%i: %8x,%8x,%8x rsp%i: %8x,%8x,%8x\n",
			dw->last_dmaReq_index,dw->req[0].next,dw->req[1].next,dw->req[2].next,
			dw->last_dmaRsp_index,dw->rsp[0].next,dw->rsp[1].next,dw->rsp[2].next);
		dw = dw->virt_next;
	}
	return hci->transfer.progress;
}
static void KickStartHc(hci_t * hci,struct pt_regs *regs)
{
//AllEOTInterrupt may happen before this DMA interrupt if enabled
//so call hc_interrupt from here instead
	if (hci->bhActive==0) {
#if 1
		if (hci->inInterrupt) hci->doubleInt=1;
		else hc_interrupt(0,hci,regs);
#else
		tasklet_schedule(&hci->bottomHalf);
#endif
	}
	else {
		unsigned long flags;
		spin_lock_irqsave(&hci->command_port_lock, flags);
		if (hci->transferState & TF_DmaInProgress) {
			if (hci->transfer.progress==NULL) {
				hci->transferState &= ~TFM_DmaPending;
				if (hci->transfer.extra==AE_extra_done) {
					hci->transferState &= ~TFM_DmaExtraPending;
				}
			}
		}
		hci->intHappened = 1;
		spin_unlock_irqrestore(&hci->command_port_lock, flags);
	}
}
/*
 * Our DMA interrupt handler
 */
static void hc_dma_interrupt(int ch, void *dev_id, struct pt_regs *regs)
{
	hci_t * hci = dev_id;
	struct frameList* fl = hci->transfer.fl;
	u_int dcsr;

	if (fl==NULL) {
		printk(KERN_ERR __FILE__ ": wow... received IRQ for DMA channel %d but no buffer exists\n", ch);
		return;
	}
//	else printk(KERN_ERR __FILE__ ": good interrupt on DMA channel %d, DDADR:%8x, DSADR:%8x, DTADR:%8x, DCMD:%8x\n", ch,DDADR(ch),DSADR(ch),DTADR(ch),DCMD(ch));



	rmb();
	dcsr = DCSR(ch);
	if (dcsr & DCSR_STOPSTATE) {
		DCSR(ch) = dcsr & ~DCSR_STOPIRQEN;
		wmb();
		if (hci->transfer.progress) {
			hci->transfer.progress=NULL;
			KickStartHc(hci,regs);
		} else {
			if (hci->transfer.extra==AE_extra_inprogress) {
				hci->transfer.extra=AE_extra_done;
				KickStartHc(hci,regs);
			}
		}
	} else {
		DCSR(ch) = dcsr;	//try getting write as close to read as possible
					//so that a channel is less likely to stop between the read and write
		wmb();
		UpdateDmaPosition(hci,fl,ch);
	}
	if (dcsr & DCSR_BUSERR) {
		printk(KERN_ERR __FILE__ ": bus error interrupt on DMA channel %d, DDADR:%8x, DSADR:%8x, DTADR:%8x, DCMD:%8x\n", ch,DDADR(ch),DSADR(ch),DTADR(ch),DCMD(ch));
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 1
#define MAPPING_REG DRCMR0	//for DREQ 0, GP19
#else
#define MAPPING_REG DRCMR1	//for DREQ 1, GP20
#endif

int GetDmaChannel(hci_t * hci,int dma,char* name)	//value passed as dma is ignored
{
#if BURST_TRANSFER_SIZE >= 8
	volatile u32* mapping = &MAPPING_REG;
	int dma_ch = pxa_request_dma(name, DMA_PRIO_HIGH,  hc_dma_interrupt, hci);
	if (dma_ch>=0) 	*mapping = dma_ch | DRCMR_MAPVLD;
	return dma_ch;
#else
	return -1;
#endif
}
void FreeDmaChannel(int dma)
{
#if BURST_TRANSFER_SIZE >= 8
	volatile u32* mapping = &MAPPING_REG;
	*mapping = 0;
	DCSR(dma) = 0;	//stop channel
	pxa_free_dma(dma);
#endif
}

//chain 0 - request, 1 - response
int StartDmaChannel(hci_t * hci,struct frameList * fl,int chain,int transferState)
{
#if (BURST_TRANSFER_SIZE >= 8) && defined(TRY_DMA)
	if (hci->hp.dmaport) {
		int channel = hci->hp.dmaChannel;
		struct dmaWork* dw = fl->chain;
		if ((DCSR(channel) & DCSR_STOPSTATE)==0) {
			DCSR(channel) = 0;
			printk(KERN_ERR __FILE__ ": Channel is ALREADY active!! %i\n",chain);
			do { rmb(); } while ((DCSR(channel) & DCSR_STOPSTATE)==0);
		}
//		else printk(KERN_ERR __FILE__ ": Good Job %d\n",chain);


//		printk("init:%8p,",dw);
		if (dw) {
			unsigned long flags;
			spin_lock_irqsave(&hci->command_port_lock, flags);
			DDADR(channel) = dw->dma_base + ((chain==REQ_CHAIN)?(int)( &((struct dmaWork*)0)->req[0].next) :
																(int)( &((struct dmaWork*)0)->rsp[0].next));
//			printk("DDADR:%8x\n",DDADR(channel));
			DCSR(channel) = DCSR_RUN|DCSR_STOPIRQEN;
			MarkTransferActive(hci,dw,chain,transferState);
			spin_unlock_irqrestore(&hci->command_port_lock, flags);
		} else {
			printk(KERN_ERR __FILE__ ": Nothing queued!! chain:%i, req:%i, rsp:%i\n",chain,fl->reqCount,fl->rspCount);
			return -1;
		}
		return 0;
	}
#endif
//	printk("init1:0,");
	return -1;
}
int StartDmaExtra(hci_t * hci,int read)
{
#if (BURST_TRANSFER_SIZE >= 8) && defined(TRY_DMA)
	if (hci->hp.dmaport) {
		unsigned long flags;
		int channel = hci->hp.dmaChannel;
		if ((DCSR(channel) & DCSR_STOPSTATE)==0) {
			DCSR(channel) = 0;
			printk(KERN_ERR __FILE__ ": Channel is ALREADY active!!\n");
			do { rmb(); } while ((DCSR(channel) & DCSR_STOPSTATE)==0);
		}

		spin_lock_irqsave(&hci->command_port_lock, flags);
		DDADR(channel) = (read) ? hci->trashDesc : hci->zeroDesc;
		DCSR(channel) = DCSR_RUN|DCSR_STOPIRQEN;
		hci->transfer.extra=AE_extra_inprogress;
		spin_unlock_irqrestore(&hci->command_port_lock, flags);
//		printk(KERN_DEBUG __FILE__ ": extra dma started\n");
		return 0;
	}
#endif
	return -1;
}
#endif //defined(USE_DMA)
