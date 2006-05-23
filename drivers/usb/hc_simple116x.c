/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*
 * simple generic USB HCD frontend Version 0.8 (9/3/2001)
 * for embedded HCs (eg. isp1161, MPC850, ...)
 *
 * USB URB handling, hci_ hcs_
 * URB queueing, qu_
 * Transfer scheduling, sh_
 *
 * Roman Weissgaerber weissg@vienna.at (C) 2001
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
/* URB HCD API function layer
 * * * */
/*-------------------------------------------------------------------------*/
/* set actual length to 0 and set status
 * queue URB
 * */
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

#include <linux/usb.h>

#include "hc_isp116x.h"
#include "hc_simple116x.h"
#include "pxa-dmaWork.h"
/*-------------------------------------------------------------------------*/
/* URB queueing:
 *
 * There is a single list of active URBs for all types.
 * For every endpoint the head URB of the queued URBs has been
 * called as a parmaeter to AddToWorkList.
 *
 * The rest of the queued URBs of an endpoint are linked into a
 * private URB list for each endpoint. (ep->urb_queue)
 * ep->pipe_head .. points to the head URB which is
 * in the active URB lists.
 *
 * The index of an endpoint consists of its number and its direction.
 *
 * The state of an intr and iso URB is 0.
 * For ctrl URBs the states are US_CTRL_SETUP, US_CTRL_DATA, US_CTRL_ACK
 * Bulk URBs states are US_BULK and US_BULK0 (with 0-len packet)
 *
 * * * */

static inline int qu_pipeindex (__u32 pipe)
{
	return (usb_pipeendpoint (pipe) << 1) | (usb_pipecontrol (pipe) ?
				0 : usb_pipeout (pipe));
}
/*-------------------------------------------------------------------------*/

static inline void qu_queue_active_urb (hci_t * hci, urb_t * urb)
{
	int urb_state = 0;

	switch (usb_pipetype (urb->pipe)) {
		case PIPE_CONTROL:
			urb_state = US_CTRL_SETUP;
			break;

		case PIPE_BULK:
			if (urb->transfer_flags & USB_ZERO_PACKET) urb_state = US_BULK0;
			break;

		case PIPE_INTERRUPT:
			urb->start_frame = hci->frame_no;
			break;
		case PIPE_ISOCHRONOUS:
		{
			if ((urb->transfer_flags & USB_ISO_ASAP)==0) {
				short i = hci->frame_no - urb->start_frame;
				if (i<0) {
					list_add(&urb->urb_list, &hci->waiting_intr_list);
					hc116x_enable_sofint(hci);
					return;
				}
			}
			break;
		}
	}
	list_add (&urb->urb_list, &hci->active_list);
	AddToWorkList(hci,urb,urb_state);
	hc116x_enable_sofint(hci);
}

static inline int checkAlreadyActive(hci_t * hci, urb_t * urb,struct hci_endpoint * ep)
{
	if (!ep->pipe_head) {ep->pipe_head = urb; return 0;}
	if (!list_empty (&(ep->urb_queue))) return 1;
	if (usb_pipeisoc (urb->pipe)) {
		if ((urb->transfer_flags & USB_ISO_ASAP)==0) return 1;
		return QueuePartner(hci,urb,ep->pipe_head);
	}
	return 1;
}
static inline int qu_queue_urb (hci_t * hci, urb_t * urb)
{
	struct hci_endpoint * ep = &usb_to_ep(urb->dev,qu_pipeindex(urb->pipe));

	unsigned long flags;
	int i;
	spin_lock_irqsave (&hci->urb_list_lock, flags);
//	if (qu_pipeindex(urb->pipe)==2) printk(KERN_ERR __FILE__  ": pipe 2\n");

	i =checkAlreadyActive(hci,urb,ep);
	if (i==1) {
		__list_add (&urb->urb_list,ep->urb_queue.prev, &(ep->urb_queue));
	} else if (i==0) {
		qu_queue_active_urb (hci, urb);
	} else {
		list_add (&urb->urb_list, &hci->active_list);
		hc116x_enable_sofint(hci);
	}
	spin_unlock_irqrestore (&hci->urb_list_lock, flags);
	return 0;
}

static int hcs_urb_queue (hci_t * hci, urb_t * urb)
{
	int i;

	if (usb_pipeisoc (urb->pipe)) {
		for (i = 0; i < urb->number_of_packets; i++) {
			urb->iso_frame_desc[i].actual_length = 0;
			urb->iso_frame_desc[i].status = -EXDEV;
		}
	}
	i = InitDmaWork(hci,urb);

	if (i==0) {
		urb->status = USB_ST_URB_PENDING;
		urb->actual_length = 0;
		qu_queue_urb (hci, urb);
	}
	return i;
}



/*-------------------------------------------------------------------------*/
/* got a transfer request
 * */
//transfer buffer must be aligned on an 8 bytes boundary
static int hci_submit_urb (urb_t * urb)
{
	hci_t * hci;
	unsigned int pipe = urb->pipe;
	unsigned long flags;
	int ret;

	if (!urb->dev || !urb->dev->bus || urb->hcpriv) return -EINVAL;
#if BURST_TRANSFER_SIZE >= 8
	if ( ((int)urb->transfer_buffer) & 7) {
		if ( (urb->transfer_buffer_length >8) ||
		     ( ( ((int)urb->transfer_buffer) & 1 ) && (urb->transfer_buffer_length >1) ) ) {
			printk(KERN_ERR __FILE__  ": improper alignment, size %d bytes\n",urb->transfer_buffer_length);
			return -EINVAL;
		}
	}
#endif
	if (usb_endpoint_halted (urb->dev,
				usb_pipeendpoint (pipe), usb_pipeout (pipe)))
		return -EPIPE;

	hci = (hci_t *) urb->dev->bus->hcpriv;

	/* a request to the virtual root hub */
	if (usb_pipedevice (pipe) == hci->rh.devnum) {
		return rh_submit_urb (urb);
	}

	if (urb_debug)
		urb_print (urb, "SUB", usb_pipein (pipe));

	/* queue the URB to its endpoint-queue */

	spin_lock_irqsave (&hci->urb_list_lock, flags);
	ret = hcs_urb_queue (hci, urb);
	spin_unlock_irqrestore (&hci->urb_list_lock, flags);

	return ret;

}

/*-------------------------------------------------------------------------*/
/* unlink URB: mark the URB to unlink
 * */

static int hci_unlink_urb (urb_t * urb)
{
	hci_t * hci;

	if (!urb) /* just to be sure */
		return -EINVAL;

	if (!urb->dev || !urb->dev->bus)
		return -ENODEV;

	hci = (hci_t *) urb->dev->bus->hcpriv;

	/* a request to the virtual root hub */
	if (usb_pipedevice (urb->pipe) == hci->rh.devnum) {
		return rh_unlink_urb (urb);
	}


	if (!list_empty (&urb->urb_list)) { /* URB active? */
		unsigned long flags;
		spin_lock_irqsave (&hci->urb_list_lock, flags);
		if (SetCancel(hci,urb)) {
			//not in active DMA chain
			list_del (&urb->urb_list);	//remove from probable endpoint queue
			list_add (&urb->urb_list, &hci->return_list);
		}
		spin_unlock_irqrestore (&hci->urb_list_lock, flags);
		if (!(urb->transfer_flags & (USB_ASYNC_UNLINK | USB_TIMEOUT_KILLED)) ) {
			/* synchron without callback */
			WaitForIdle(hci, urb);
		}
	} else { /* hcd does not own URB but we keep the driver happy anyway */
		if (urb->complete && (urb->transfer_flags & USB_ASYNC_UNLINK)) {
//			unsigned long flags;
			urb->status = -ENOENT;
			urb->actual_length = 0;

//			spin_lock_irqsave (&hci->urb_list_lock, flags);
			urb->complete (urb);
//			spin_unlock_irqrestore (&hci->urb_list_lock, flags);

			urb->status = 0;
		} else {
			urb->status = -ENOENT;
		}
	}

	return 0;
}

/*-------------------------------------------------------------------------*/
/* allocate private data space for a usb device
 * */

static int hci_alloc_dev (struct usb_device * usb_dev)
{
	struct hci_device * dev;
	int i;

	dev = kmalloc (sizeof (*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	memset (dev, 0, sizeof (*dev));

	for (i = 0; i < NUM_EDS; i++) {
		INIT_LIST_HEAD (&(dev->ep[i].urb_queue));
		dev->ep[i].pipe_head = NULL;
	}

	usb_dev->hcpriv = dev;

	if (hc_verbose)
		printk ("USB HC dev alloc %d bytes\n", sizeof (*dev));

	return 0;
}

/*-------------------------------------------------------------------------*/
/* tell us the current USB frame number
 * */

static int hci_get_current_frame_number (struct usb_device *usb_dev)
{
	hci_t * hci = usb_dev->bus->hcpriv;
	return hci->frame_no;
}
/* debug| print the main components of an URB
 * small: 0) header + data packets 1) just header */

void urb_print (urb_t * urb, char * str, int small)
{
	unsigned int pipe= urb->pipe;
	int i, len;

	if (!urb->dev || !urb->dev->bus) {
		dbg("%s URB: no dev", str);
		return;
	}

	printk("%s URB:[%4x] dev:%2d,ep:%2d-%c,type:%s,flags:%4x,len:%d/%d,stat:%d(%x)\n",
			str,
		 	hci_get_current_frame_number (urb->dev),
		 	usb_pipedevice (pipe),
		 	usb_pipeendpoint (pipe),
		 	usb_pipeout (pipe)? 'O': 'I',
		 	usb_pipetype (pipe) < 2? (usb_pipeint (pipe)? "INTR": "ISOC"):
		 		(usb_pipecontrol (pipe)? "CTRL": "BULK"),
		 	urb->transfer_flags,
		 	urb->actual_length,
		 	urb->transfer_buffer_length,
		 	urb->status, urb->status);
	if (!small) {
		if (usb_pipecontrol (pipe)) {
			printk ( __FILE__ ": cmd(8):");
			for (i = 0; i < 8 ; i++)
				printk (" %02x", ((__u8 *) urb->setup_packet) [i]);
			printk ("\n");
		}
		if (urb->transfer_buffer_length > 0 && urb->transfer_buffer) {
			printk ( __FILE__ ": data(%d/%d):",
				urb->actual_length,
				urb->transfer_buffer_length);
			len = usb_pipeout (pipe)?
						urb->transfer_buffer_length: urb->actual_length;
			for (i = 0; i < 16 && i < len; i++)
				printk (" %02x", ((__u8 *) urb->transfer_buffer) [i]);
			printk ("%s stat:%d\n", i < len? "...": "", urb->status);
		}
	}
}



/*-------------------------------------------------------------------------*/
/* return path (complete - callback) of URB API
 * also handles resubmition of intr URBs
 * */

static int hcs_return_urb (hci_t * hci, urb_t * urb, int resub_ok)
{
	int resubmit = 0;
	struct dmaWork* dw=NULL;

	if (urb_debug)
		urb_print (urb, "RET", usb_pipeout (urb->pipe));

  	resubmit = urb->interval && resub_ok;


	if (!resubmit) {
		urb->dev = NULL;
		dw = urb->hcpriv;
		urb->hcpriv = NULL;
	}
	if (urb->complete) {
		if ((!resubmit)||!IntervalStatus(urb)) {
			urb->complete (urb); /* call complete */
		}
	}
	if (resubmit) { /* requeue the URB */
		if (!usb_pipeint (urb->pipe) || (urb->interval==0)) {
			urb->start_frame = hci->frame_no;
			hcs_urb_queue (hci, urb);
		} else {
			urb->start_frame = hci->frame_no + urb->interval;
			list_add (&urb->urb_list, &hci->waiting_intr_list);
			hc116x_enable_sofint(hci);
		}
	} else {
		if (dw) FreeDmaWork(hci,dw);		//this also wakes anyone waiting on its destruction
	}

	return 0;
}

//interrupts disabled
//out: 1 cancel called on at least 1 urb
static int cancel_ep(hci_t *hci,struct hci_endpoint* ep,int ep_num)
{
	struct list_head * list_lh = &ep->urb_queue;
	struct list_head * lh;
	struct list_head * lhTemp;
	urb_t * urb;
	int ret=0;

	list_for_each_safe (lh, lhTemp, list_lh) {
		urb = list_entry (lh, urb_t, urb_list);
		urb->status = -ENODEV;
		urb->complete = NULL;
		ret=1;
		if (SetCancel(hci,urb)) {
			//not in an active chain, this should always be true
			list_del(&urb->urb_list);
			list_add(&urb->urb_list, &hci->return_list);
		}
//		urb->dev = NULL;	//let urb disappear naturally
        }
	urb = ep->pipe_head;
	if (urb) {
////		ep->pipe_head = NULL;	//this chain is needed to properly dispose of returned work, wait until it disappears on its own
		if ( ((int)(urb->hcpriv)) & 0xf) {
			printk(KERN_ERR __FILE__ ": urb released before unlink ep:%d\n",ep_num);
		} else {
			urb->status = -ENODEV;
			urb->complete = NULL;
			ret=1;
			if (SetCancel(hci,urb)) {
				//not in an active chain, this should always be false
				list_del(&urb->urb_list);
				list_add(&urb->urb_list, &hci->return_list);
			}
//			urb->dev = NULL;
		}
	}
	return ret;
}
/*-------------------------------------------------------------------------*/
/* free private data space of usb device
 * */
static int hci_purge_dev (hci_t *hci,struct hci_device * hci_dev)
{
	int i;
	unsigned long flags;
	int ret=0;
	spin_lock_irqsave (&hci->urb_list_lock, flags);

	for (i=0; i<NUM_EDS; i++) {
		ret |= cancel_ep(hci,&hci_dev->ep[i],i);
	}

	spin_unlock_irqrestore (&hci->urb_list_lock, flags);
	if (ret) tasklet_schedule(&hci->bottomHalf);	//make sure it's still alive
	return ret;
}
static int hci_purge (struct usb_device * usb_dev)
{
	int i = 100;
	struct hci_device * hci_dev;
	do {
		hci_dev = usb_to_hci_dev(usb_dev);
		if (!hci_dev) break;
		if (!hci_purge_dev(usb_dev->bus->hcpriv, hci_dev)) break;
		set_current_state (TASK_UNINTERRUPTIBLE);
		schedule_timeout(1);
	} while (--i);
	if (i==0)
		printk(KERN_ERR __FILE__ ": purge timeout\n");

	return 0;
}
static int hci_free_dev (struct usb_device * usb_dev)
{
	struct hci_device * hci_dev = usb_to_hci_dev(usb_dev);
	if (hc_verbose)
		printk ("USB HC dev free\n");

	if (hci_dev) {
		hci_purge(usb_dev);
		usb_dev->hcpriv = NULL;
		kfree (hci_dev);
	}

	return 0;
}

/*-------------------------------------------------------------------------*/
/* make a list of all io-functions
 * */

struct usb_operations hci_device_operations = {
	hci_alloc_dev,
	hci_free_dev,
	hci_get_current_frame_number,
	hci_submit_urb,
	hci_unlink_urb,
	hci_purge
};

//now this task is run by a kernel deamon
void sh_scan_return_list(void* __hci)
{
	hci_t *hci = __hci;
	struct list_head * lh = &hci->return_list;

	while (1) {
		urb_t * urb = NULL;
		unsigned long flags;
		spin_lock_irqsave (&hci->urb_list_lock, flags);
		if (!list_empty (lh)) {
			urb = list_entry (lh->next, urb_t, urb_list);
			list_del_init (&urb->urb_list);
		}
		spin_unlock_irqrestore (&hci->urb_list_lock, flags);
		if (urb) hcs_return_urb(hci, urb, !cancelled_request(urb));
		else break;
	}
}
/*-------------------------------------------------------------------------*/

/////////////////////////////////////////////////////////////////////////////
// Routines below are in interrupt context

/*-------------------------------------------------------------------------*/
/* return path (after transfer)
 * remove URB from queue and return it to the api layer
 * */
urb_t* qu_getPipeHead(hci_t * hci, urb_t * urb)
{
#if 0
	struct hci_endpoint * ep = &usb_to_ep(urb->dev,qu_pipeindex(urb->pipe));
#else
	struct hci_device * hci_dev = (urb->dev) ? usb_to_hci_dev(urb->dev) : NULL;
	struct hci_endpoint * ep;
	if (!hci_dev) {
		printk(KERN_ERR __FILE__ ": No device\n");
		return NULL;
	}
	ep = &hci_dev->ep[qu_pipeindex(urb->pipe)];
#endif
	return ep->pipe_head;
}

void qu_return_urb (hci_t * hci, urb_t * urb, urb_t * urb_next,int bNeedMore,int resub_ok)
{
#if 0
	struct hci_endpoint * ep = &usb_to_ep(urb->dev,qu_pipeindex(urb->pipe));
#else
	struct hci_device * hci_dev = (urb->dev) ? usb_to_hci_dev(urb->dev) : NULL;
	struct hci_endpoint * ep;
	if (!hci_dev) {
		if (SetCancel(hci,urb)) {
			if (urb != urb_next) {
				list_del(&urb->urb_list);
				list_add(&urb->urb_list, &hci->return_list);
			}
		}
		printk(KERN_ERR __FILE__ ": No device\n");
		return;
	}
	ep = &hci_dev->ep[qu_pipeindex(urb->pipe)];
#endif
	if (ep->pipe_head == urb) ep->pipe_head = urb_next;

	if (ep->pipe_head) {
		if (bNeedMore) {
			if (!list_empty (&(ep->urb_queue))) {
				urb_t* urb1 = list_entry (ep->urb_queue.next, urb_t, urb_list);
				if (urb1->transfer_flags & USB_ISO_ASAP) {
					if (QueuePartner(hci,urb1,urb_next)==2) {
						list_del(&urb1->urb_list);
						list_add(&urb1->urb_list, &hci->active_list);
						hc116x_enable_sofint(hci);
					}
				}
			}
		}
		if (urb != urb_next) {
			list_del(&urb->urb_list);
			list_add(&urb->urb_list, &hci->return_list);
		}
	} else {
		list_del_init (&urb->urb_list);
		if (!list_empty (&(ep->urb_queue))) {
			urb_t* urb1 = list_entry (ep->urb_queue.next, urb_t, urb_list);
			list_del_init (&urb1->urb_list);
			ep->pipe_head = urb1;
			qu_queue_active_urb (hci, urb1);
			list_add (&urb->urb_list, &hci->return_list);
		} else {
#if 0
			hcs_return_urb(hci, urb, resub_ok);	//return now because it may request more work
								//and endpoint is inactive
#else
			list_add (&urb->urb_list, &hci->return_list);
#endif
		}
	}
}


void sh_scan_waiting_intr_list(hci_t *hci)
{
	struct list_head * list_lh = &hci->waiting_intr_list;
	struct list_head * lh;
	struct list_head * lhTemp;
	urb_t * urb;

	list_for_each_safe (lh, lhTemp, list_lh) {
		urb = list_entry (lh, urb_t, urb_list);
		if ( cancelled_request(urb) ) {
			list_del (&urb->urb_list);
			list_add (&urb->urb_list, &hci->return_list);
//			urb->complete = NULL;
		}
		else {
			short i = hci->frame_no - urb->start_frame;
			if (i>=0) {
				list_del_init (&urb->urb_list);
				hcs_urb_queue (hci, urb);
			}
		}
        }
}
