#ifdef USE_DMA
#define BURST_TRANSFER_SIZE 8		 //2 (single cycle, NOT supported on PXA250) used with no dma
					 //8 (4 cycle burst), 16 (8 cycle burst)
#else
#define BURST_TRANSFER_SIZE 2
#endif
//**************************

#if BURST_TRANSFER_SIZE==16
#define ISP_BURST_CODE (2<<5)

#elif BURST_TRANSFER_SIZE==8
#define ISP_BURST_CODE (1<<5)

#else
#define ISP_BURST_CODE (0<<5)
#endif

#define PTD_ACTUAL(d0,d1)    (  (d0) & 0x3ff)
#define PTD_TOGGLE(d0,d1)    (( (d0) >>10)&1)
#define PTD_ACTIVE(d0,d1)    (( (d0) >>11)&1)
#define PTD_CC(d0,d1)        (( (d0) >>12)&0xf)
#define PTD_MAXPS(d0,d1)     (( (d0) >>16)&0x3ff)
#define PTD_SLOW(d0,d1)      (( (d0) >>26)&1)
#define PTD_LAST(d0,d1)      (( (d0) >>27)&1)
#define PTD_ENDPOINT(d0,d1)  (  (d0) >>28)

#define PTD_ALLOTED(d0,d1)   (  (d1) &0x3ff)
#define PTD_PID(d0,d1)       (( (d1) >>10)&3)
#define PTD_ADDRESS(d0,d1)   (( (d1) >>16)&0x7f)
#define PTD_FORMAT(d0,d1)    (( (d1) >>23)&1)

#define X0PTD_ACTUAL(d)    (d)
#define X0PTD_TOGGLE(d)    ( (d) <<10)
#define X0PTD_ACTIVE(d)    ( (d) <<11)
#define X0PTD_CC(d)        ( (d) <<12)
#define X0PTD_MAXPS(d)     ( (d) <<16)
#define X0PTD_SLOW(d)      ( (d) <<26)
#define X0PTD_LAST(d)      ( (d) <<27)
#define X0PTD_ENDPOINT(d)  ( (d) <<28)

#define X1PTD_ALLOTED(d)   (d)
#define X1PTD_PID(d)       ( (d) <<10)
#define X1PTD_ADDRESS(d)   ( (d) <<16)
#define X1PTD_FORMAT(d)    ( (d) <<23)

#define F0PTD_ACTUAL    X0PTD_ACTUAL(0x3ff)
#define F0PTD_TOGGLE    X0PTD_TOGGLE(1)
#define F0PTD_ACTIVE    X0PTD_ACTIVE(1)
#define F0PTD_CC        X0PTD_CC(0xf)
#define F0PTD_MAXPS     X0PTD_MAXPS(0x3ff)
#define F0PTD_SLOW      X0PTD_SLOW(1)
#define F0PTD_LAST      X0PTD_LAST(1)
#define F0PTD_ENDPOINT  X0PTD_ENDPOINT(0xf)

#define F1PTD_ALLOTED   X1PTD_ALLOTED(0x3ff)
#define F1PTD_PID       X1PTD_PID(3)
#define F1PTD_ADDRESS   X1PTD_ADDRESS(0x7f)
#define F1PTD_FORMAT    X1PTD_FORMAT(1)


#define LAST_MARKER cpu_to_le32(X0PTD_LAST(1))
#define ISO_MARKER cpu_to_le32(X1PTD_FORMAT(1)|X1PTD_PID(2))
#define REQ_CHAIN 0
#define RSP_CHAIN 1

#ifdef USE_DMA
int GetDmaChannel(hci_t * hci,int dma,char* name);
void FreeDmaChannel(int dma);
int StartDmaChannel(hci_t * hci,struct frameList* fl,int chain,int extra);
int StartDmaExtra(hci_t * hci,int read);
#endif

int ScheduleWork(hci_t * hci, struct frameList* fl,struct ScheduleData * sd);
void FakeDmaReqTransfer(port_t hcport, int reqCount, struct dmaWork* dw, hci_t * hci,int transferState);
void FakeDmaRspTransfer(port_t hcport, int rspCount, struct dmaWork* dw, hci_t * hci,int transferState);

void ReleaseDmaWork(struct dmaWork* dw);
struct dmaWork* AllocDmaWork(hci_t * hci);
void FreeDmaWork(hci_t * hci,struct dmaWork* free);
////////////////////////
int InitDmaWork(hci_t * hci, urb_t * urb);
void AddToWorkList(hci_t * hci, urb_t * urb,int state);
int SetCancel(hci_t * hci,urb_t * urb);
int IntervalStatus(urb_t * urb);
void WaitForIdle(hci_t * hci, urb_t * urb);
int cancelled_request(urb_t* urb);
int QueuePartner(hci_t * hci, urb_t * urb,urb_t * head);
void ScanNewIsoWork(hci_t * hci,int i1);
void RemoveCancelled(hci_t * hci, struct frameList* fl);


