/*
 * ISP116x HCD (Host Controller Driver) for USB.
 * 
 * (C) Copyright 2001 Roman Weissgaerber <weissg@vienna.at>
 * 
 *
 */
#define PIO_USED

#ifdef CONFIG_USB_ISP116x_DMA
#define DMA_USED
#endif
//comment out #define DMA_USED to train PIO lines
//comment out #define PIO_USED to train DMA lines
//Normal case, leave both defined...


/*
 * Maximum number of root hub ports.
 */
#define MAX_ROOT_PORTS	15	/* maximum OHCI root hub ports */


/* control and status registers */
#define HcRevision		0x00
#define HcControl 		0x01
#define HcCommandStatus		0x02
#define HcInterruptStatus	0x03
#define HcInterruptEnable	0x04
#define HcInterruptDisable	0x05
#define HcFmInterval		0x0D
#define HcFmRemaining		0x0E
#define HcFmNumber		0x0F
#define HcLSThreshold		0x11
#define HcRhDescriptorA		0x12
#define HcRhDescriptorB		0x13
#define HcRhStatus		0x14
#define HcRhPortStatus		0x15

#define HcHardwareConfiguration 0x20
#define HcDMAConfiguration	0x21
#define HcTransferCounter	0x22
#define HcuPInterrupt		0x24
#define HcuPInterruptEnable	0x25
#define HcChipID		0x27
#define HcScratch		0x28
#define HcSoftwareReset		0x29
#define HcITLBufferLength	0x2A
#define HcATLBufferLength	0x2B
#define HcBufferStatus		0x2C
#define HcReadBackITL0Length	0x2D
#define HcReadBackITL1Length	0x2E
#define HcITLBufferPort		0x40
#define HcATLBufferPort		0x41

/* OHCI CONTROL AND STATUS REGISTER MASKS */

/*
 * HcControl (control) register masks
 */
#define OHCI_CTRL_HCFS	(3 << 6)	/* BUS state mask */
#define OHCI_CTRL_RWC	(1 << 9)	/* remote wakeup connected */
#define OHCI_CTRL_RWE	(1 << 10)	/* remote wakeup enable */

/* pre-shifted values for HCFS */
#define OHCI_USB_RESET	(0 << 6)
#define OHCI_USB_RESUME	(1 << 6)
#define OHCI_USB_OPER	(2 << 6)
#define OHCI_USB_SUSPEND	(3 << 6)

/*
 * HcCommandStatus (cmdstatus) register masks
 */
#define OHCI_HCR	(1 << 0)	/* host controller reset */
#define OHCI_SOC  	(3 << 16)	/* scheduling overrun count */

/*
 * masks used with interrupt registers:
 * HcInterruptStatus (intrstatus)
 * HcInterruptEnable (intrenable)
 * HcInterruptDisable (intrdisable)
 */
#define OHCI_INTR_SO	(1 << 0)	/* scheduling overrun */

#define OHCI_INTR_SF	(1 << 2)	/* start frame */
#define OHCI_INTR_RD	(1 << 3)	/* resume detect */
#define OHCI_INTR_UE	(1 << 4)	/* unrecoverable error */
#define OHCI_INTR_FNO	(1 << 5)	/* frame number overflow */
#define OHCI_INTR_RHSC	(1 << 6)	/* root hub status change */
#define OHCI_INTR_ATD	(1 << 7)	/* scheduling overrun */

#define OHCI_INTR_MIE	(1 << 31)	/* master interrupt enable */

/*
 * HcHardwareConfiguration
 */
#define InterruptPinEnable 	(1 << 0)
#define InterruptPinTrigger 	(1 << 1)
#define InterruptOutputPolarity	(1 << 2)
#define DataBusWidth16		(1 << 3)
#define DREQOutputPolarity	(1 << 5)
#define DACKInputPolarity	(1 << 6)
#define EOTInputPolarity	(1 << 7)
#define DACKMode		(1 << 8)
#define AnalogOCEnable		(1 << 10)
#define SuspendClkNotStop	(1 << 11)
#define DownstreamPort15KRSel	(1 << 12)

/* 
 * HcDMAConfiguration
 */
#define DMAC_READ 0
#define DMAC_WRITE 		(1 << 0)
#define DMAC_ITL 0
#define DMAC_ATL 		(1 << 1)
#define DMACounterSelect	(1 << 2)
#define DMAEnable		(1 << 4)
#define BurstLen_1		0
#define BurstLen_4		(1 << 5)
#define BurstLen_8		(2 << 5)


/*
 * HcuPInterrupt
 */
#define SOFITLInt		(1 << 0)
#define ATLInt			(1 << 1)
#define AllEOTInterrupt		(1 << 2)
#define OPR_Reg			(1 << 4)
#define HCSuspended		(1 << 5)
#define ClkReady		(1 << 6)

/*
 * HcBufferStatus
 */
#define ITL0BufferFull		(1 << 0)
#define ITL1BufferFull		(1 << 1)
#define ATLBufferFull		(1 << 2)
#define ITL0BufferDone		(1 << 3)
#define ITL1BufferDone		(1 << 4)
#define ATLBufferDone		(1 << 5)



/* OHCI ROOT HUB REGISTER MASKS */
 
/* roothub.portstatus [i] bits */
#define RH_PS_CCS            0x00000001   	/* current connect status */
#define RH_PS_PES            0x00000002   	/* port enable status*/
#define RH_PS_PSS            0x00000004   	/* port suspend status */
#define RH_PS_POCI           0x00000008   	/* port over current indicator */
#define RH_PS_PRS            0x00000010  	/* port reset status */
#define RH_PS_PPS            0x00000100   	/* port power status */
#define RH_PS_LSDA           0x00000200    	/* low speed device attached */
#define RH_PS_CSC            0x00010000 	/* connect status change */
#define RH_PS_PESC           0x00020000   	/* port enable status change */
#define RH_PS_PSSC           0x00040000    	/* port suspend status change */
#define RH_PS_OCIC           0x00080000    	/* over current indicator change */
#define RH_PS_PRSC           0x00100000   	/* port reset status change */

/* roothub.status bits */
#define RH_HS_LPS	     0x00000001		/* local power status */
#define RH_HS_OCI	     0x00000002		/* over current indicator */
#define RH_HS_DRWE	     0x00008000		/* device remote wakeup enable */
#define RH_HS_LPSC	     0x00010000		/* local power status change */
#define RH_HS_OCIC	     0x00020000		/* over current indicator change */
#define RH_HS_CRWE	     0x80000000		/* clear remote wakeup enable */

/* roothub.b masks */
#define RH_B_DR		0x0000ffff		/* device removable flags */
#define RH_B_PPCM	0xffff0000		/* port power control mask */

/* roothub.a masks */
#define	RH_A_NDP	(0xff << 0)		/* number of downstream ports */
#define	RH_A_PSM	(1 << 8)		/* power switching mode */
#define	RH_A_NPS	(1 << 9)		/* no power switching */
#define	RH_A_DT		(1 << 10)		/* device type (mbz) */
#define	RH_A_OCPM	(1 << 11)		/* over current protection mode */
#define	RH_A_NOCP	(1 << 12)		/* no over current protection */
#define	RH_A_POTPGT	(0xff << 24)		/* power on to power good time */

//#define min(a,b) (((a)<(b))?(a):(b))


#define URB_DEL 1

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Virtual Root HUB
 * */


struct virt_root_hub {
	int devnum; 		/* Address of Root Hub endpoint */
	void * urb;			/* interrupt URB of root hub */
	int send;			/* active flag */
 	int interval;		/* intervall of roothub interrupt transfers */
	struct timer_list rh_int_timer; /* intervall timer for rh interrupt EP */
};

struct dmaWork;

struct worklist
{
	spinlock_t lock;		//for chain access
	struct dmaWork* chain;		//chain of transactions.
};
struct frameList
{
	struct dmaWork* chain;		//this is only accessed by interrupt rtn, no spin needed
	struct dmaWork* dwHead;	//temporary while building chain
	__u16 reqCount;		//if the last PTD is an IN  then not all of the request  buffer needs to be written
	__u16 rspCount;		//if the last PTD is an OUT then not all of the response buffer needs to be read
	__u16 prevReqCount;
	__u16 prevRspCount;
	
	__u16 limit;
#define STATE_READ_RESPONSE 0
#define STATE_SCHEDULE_WORK 1
#define STATE_WRITE_REQUEST 2
	__u8 state;
	__u8 active;
	unsigned char full;
	unsigned char done;
	unsigned char port;
	unsigned char dmac;
	struct dmaWork* idle_chain;	//this is only accessed by interrupt rtn, no spin needed
	struct frameList* iso_partner;
	struct worklist work;
	int    sofCnt;		//# of SOFs since last transfer started
#define USE_FM_REMAINING	//this is used for debugging info, and to avoid chip buffer_done lost bug
#ifdef USE_FM_REMAINING
	__u16 fmRemainingRsp;
	__u16 fmRemainingReq;
	__u16 spareW;

	__u8 lastTransferType;	//used to train the pio or dma lines
#define TT_PIO 0
#define TT_DMA 1
#define TT_REQ 0
#define TT_RSP 2
#define TT_PIO_REQ (TT_PIO|TT_REQ)
#define TT_DMA_REQ (TT_DMA|TT_REQ)
#define TT_PIO_RSP (TT_PIO|TT_RSP)
#define TT_DMA_RSP (TT_DMA|TT_RSP)

	__u8 spare;
#endif
};

struct ActiveDma
{
	struct dmaWork* progress;	//this is updated by hc_dma_interrupt
					//NULL means done, otherwise the 1st dmaWork incomplete
	struct frameList* fl;
	int    chain;
};

struct ScheduleData {
	struct dmaWork** pdw;

	struct dmaWork* dLarge;
	struct dmaWork* dLargeBefore;

	struct dmaWork* dBefore;
	struct dmaWork* dLast;
	struct dmaWork* dwLastReduced;

	struct dmaWork** pIdledw;
	struct dmaWork* lastIdle;

	int units_left;
	int unstartedIndex;
	int otherIndex;
#define MAX_UNSTARTED 16
#define MAX_OTHER 16
	struct dmaWork* unstartedReads[MAX_UNSTARTED];
	struct dmaWork* other[MAX_OTHER];
};

struct hci;

typedef struct hcipriv {
	int hcport;
	void (*delay)(struct hci * hci);
	int memFencePort;
	int memCnt;

	int wuport;
	int dmaChannel;
	dma_addr_t dmaport;
//	int intrstatus;
//	__u32 hc_control;		/* copy of the hc control reg */
//	int frame;

	int uPinterruptEnable;
	int irq;
//	int disabled;			/* e.g. got a UE, we're hung */
//	atomic_t resume_count;		/* defending against multiple resumes */
//	struct ohci_regs * regs;	/* OHCI controller's memory */

	int hcType;
	int hc;
	int memFenceType;
	int memFence;
	int wuType;
	int wu;
} hcipriv_t;
#define MAX_SLOPE (3<<16)
#define MAX_B (80<<16)			//if point is above this line, then ignore
#define MIN_SLOPE (1<<16)
#define MIN_B (0<<16)			//if point is below this line, then ignore

#if 1			//pretrained values, for an arm PXA255 at 400Mhz with cache writeback, burst 8
#ifdef DMA_USED
#define DMA_LOW_X1_REQ 8
#define DMA_LOW_Y1_REQ 28
#define DMA_LOW_X2_REQ 536
#define DMA_LOW_Y2_REQ 1000	//slope 1.840896 b 13.272705
#define DMA_HIGH_X1_REQ 8
#define DMA_HIGH_Y1_REQ 94
#define DMA_HIGH_X2_REQ 648
#define DMA_HIGH_Y2_REQ 1479	//slope 2.164062 b 76.6875

#define DMA_LOW_X1_RSP 520
#define DMA_LOW_Y1_RSP 815
#define DMA_LOW_X2_RSP 1616
#define DMA_LOW_Y2_RSP 2516	//slope 1.552001 b 7.951049
#define DMA_HIGH_X1_RSP 984
#define DMA_HIGH_Y1_RSP 1866
#define DMA_HIGH_X2_RSP 1592
#define DMA_HIGH_Y2_RSP 2929	//slope 1.748367 b 145.621582
#endif

#define PIO_LOW_X1_REQ 8
#define PIO_LOW_Y1_REQ 35
#define PIO_LOW_X2_REQ 768
#define PIO_LOW_Y2_REQ 1256	//slope 1.606 b 22.6098
#define PIO_HIGH_X1_REQ 144
#define PIO_HIGH_Y1_REQ 479
#define PIO_HIGH_X2_REQ 800
#define PIO_HIGH_Y2_REQ 1647	//slope 1.780502 b 222.610

#define PIO_LOW_X1_RSP 8
#define PIO_LOW_Y1_RSP 24
#define PIO_LOW_X2_RSP 8
#define PIO_LOW_Y2_RSP 24
#define PIO_HIGH_X1_RSP 8
#define PIO_HIGH_Y1_RSP 73
#define PIO_HIGH_X2_RSP 8
#define PIO_HIGH_Y2_RSP 73

#else
#ifdef DMA_USED
#define DMA_LOW_X1_REQ 0	//let it train itself
#define DMA_LOW_Y1_REQ (1<<14)
#define DMA_LOW_X2_REQ 0
#define DMA_LOW_Y2_REQ (1<<14)
#define DMA_HIGH_X1_REQ 0
#define DMA_HIGH_Y1_REQ 0
#define DMA_HIGH_X2_REQ 0
#define DMA_HIGH_Y2_REQ 0

#define DMA_LOW_X1_RSP 0	//let it train itself
#define DMA_LOW_Y1_RSP (1<<14)
#define DMA_LOW_X2_RSP 0
#define DMA_LOW_Y2_RSP (1<<14)
#define DMA_HIGH_X1_RSP 0
#define DMA_HIGH_Y1_RSP 0
#define DMA_HIGH_X2_RSP 0
#define DMA_HIGH_Y2_RSP 0
#endif

#define PIO_LOW_X1_REQ 0	//let it train itself
#define PIO_LOW_Y1_REQ (1<<14)
#define PIO_LOW_X2_REQ 0
#define PIO_LOW_Y2_REQ (1<<14)
#define PIO_HIGH_X1_REQ 0
#define PIO_HIGH_Y1_REQ 0
#define PIO_HIGH_X2_REQ 0
#define PIO_HIGH_Y2_REQ 0

#define PIO_LOW_X1_RSP 0	//let it train itself
#define PIO_LOW_Y1_RSP (1<<14)
#define PIO_LOW_X2_RSP 0
#define PIO_LOW_Y2_RSP (1<<14)
#define PIO_HIGH_X1_RSP 0
#define PIO_HIGH_Y1_RSP 0
#define PIO_HIGH_X2_RSP 0
#define PIO_HIGH_Y2_RSP 0
#endif
struct timing_line {
	__u32 slope,b;
	__u16 x1,y1,x2,y2;
};

struct timing_lines {
	struct timing_line high;
	struct timing_line low;
};

typedef struct hci {
	struct virt_root_hub rh;		/* roothub */


#define TF_Aborted -1
#define TF_Done 0
#define TF_TransferInProgress 1
	__u8 transferState;
	__u8 resetIdleMask;
	__u8 itl_index;
	__u8 bstat;
	
	volatile __u8 bhActive;
	volatile __u8 intHappened;
	volatile __u8 inInterrupt;
	volatile __u8 doubleInt;

	volatile __u8 sofint;
	volatile __u8 atlint;
	volatile __u8 timingsChanged; //eotint;
	volatile __u8 intHistoryIndex;
#define INT_HISTORY_SIZE 32	//power of 2, comment out to speed up code

#ifdef INT_HISTORY_SIZE
	__u16 intHistory_elapsed[INT_HISTORY_SIZE];
	__u8  intHistory_uP[INT_HISTORY_SIZE];
	__u8  intHistory_bstat[INT_HISTORY_SIZE];
#endif
	void* last_eot_pc;
	__u16   lastEOTuP;
	volatile __u8 chipReady;
	volatile __u8 scanForCancelled;

//#define USE_COMMAND_PORT_RESTORE	//this seems to cause a chip bug
#ifdef USE_COMMAND_PORT_RESTORE
	__u8 command_port;	//so interrupt routine can restore state
	__u8 spare1,spare2,spare3;
#endif
#ifdef USE_FM_REMAINING
#ifdef DMA_USED
	struct timing_lines dmaLinesReq __attribute__ ((aligned (32)));
#endif
	struct timing_lines pioLinesReq __attribute__ ((aligned (32)));
#ifdef DMA_USED
	struct timing_lines dmaLinesRsp;
#endif
	struct timing_lines pioLinesRsp;
	struct frameList* lastTransferFrame;
#endif
	spinlock_t command_port_lock;
	spinlock_t urb_list_lock;
	struct list_head active_list;
	struct list_head waiting_intr_list;
	struct list_head return_list;
	struct tq_struct return_task;
	struct tasklet_struct	bottomHalf;
	struct timeval timeVal;

	dma_addr_t zero;
	dma_addr_t trash;

#define FRAME_ITL0 0	//I don't know and don't care which is ITL0 and which is ITL1
#define FRAME_ITL1 1
#define FRAME_ATL 2

	struct frameList* itlActiveFrame;
	struct frameList frames[3];
	struct ActiveDma transfer;

	struct worklist iso_new_work;
	struct worklist free;

	struct list_head hci_hcd_list;	/* list of all hci_hcd */
	struct usb_bus * bus;		/* our bus */
	struct ScheduleData sdatl;
	struct ScheduleData sditl;

	int frame_no;			/* frame # */
	hcipriv_t hp;			/* individual part of hc type */
	int	errorCnt;
} hci_t;

void hc116x_enable_sofint(hci_t* hci);
int hc116x_read_reg32(hci_t* hci, int regindex);
void hc116x_write_reg32(hci_t* hci, unsigned int value, int regindex);
__u8 SaveState(hci_t* hci);
void RestoreState(hci_t* hci,__u8 cp);
void hc_interrupt (int irq, void * __hci, struct pt_regs * r);


int rh_submit_urb (urb_t *urb);
int rh_unlink_urb (urb_t *urb);
int rh_connect_rh (hci_t * hci);

extern int hc_verbose;
extern int hc_error_verbose;
extern int urb_debug;

/*-------------------------------------------------------------------------*/
