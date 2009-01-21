/*-------------------------------------------------------------------------*/
/* list of all controllers using this driver
 * */

static LIST_HEAD (hci_hcd_list);


/* URB states (urb_state) */
/* isoc, interrupt single state */

/* bulk transfer main state and 0-length packet */
#define US_BULK 	0
#define US_BULK0 	1
/* three setup states */
#define US_CTRL_SETUP 	2
#define US_CTRL_DATA	1
#define US_CTRL_ACK		0


/*-------------------------------------------------------------------------*/
/* HC private part of a device descriptor
 * */

#define NUM_EDS 32
struct hci_endpoint {
	urb_t * pipe_head;
	struct list_head urb_queue;
};
struct hci_device {
	struct hci_endpoint ep[NUM_EDS];
};
#define usb_to_hci_dev(usb)		(((struct hci_device *)(usb)->hcpriv))
#define usb_to_ep(usb,index)	usb_to_hci_dev(usb)->ep[index]


#if 1
/* USB HUB CONSTANTS (not OHCI-specific; see hub.h and USB spec) */
 
/* destination of request */
#define RH_INTERFACE               0x01
#define RH_ENDPOINT                0x02
#define RH_OTHER                   0x03

#define RH_CLASS                   0x20
#define RH_VENDOR                  0x40

/* Requests: bRequest << 8 | bmRequestType */
#define RH_GET_STATUS           0x0080
#define RH_CLEAR_FEATURE        0x0100
#define RH_SET_FEATURE          0x0300
#define RH_SET_ADDRESS			0x0500
#define RH_GET_DESCRIPTOR		0x0680
#define RH_SET_DESCRIPTOR       0x0700
#define RH_GET_CONFIGURATION	0x0880
#define RH_SET_CONFIGURATION	0x0900
#define RH_GET_STATE            0x0280
#define RH_GET_INTERFACE        0x0A80
#define RH_SET_INTERFACE        0x0B00
#define RH_SYNC_FRAME           0x0C80
/* Our Vendor Specific Request */
#define RH_SET_EP               0x2000


/* Hub port features */
#define RH_PORT_CONNECTION         0x00
#define RH_PORT_ENABLE             0x01
#define RH_PORT_SUSPEND            0x02
#define RH_PORT_OVER_CURRENT       0x03
#define RH_PORT_RESET              0x04
#define RH_PORT_POWER              0x08
#define RH_PORT_LOW_SPEED          0x09

#define RH_C_PORT_CONNECTION       0x10
#define RH_C_PORT_ENABLE           0x11
#define RH_C_PORT_SUSPEND          0x12
#define RH_C_PORT_OVER_CURRENT     0x13
#define RH_C_PORT_RESET            0x14

/* Hub features */
#define RH_C_HUB_LOCAL_POWER       0x00
#define RH_C_HUB_OVER_CURRENT      0x01

#define RH_DEVICE_REMOTE_WAKEUP    0x00
#define RH_ENDPOINT_STALL          0x01

#endif



/*-------------------------------------------------------------------------*/
/* PID - packet ID
 * */

#define PID_SETUP	0
#define PID_OUT		1
#define PID_IN		2

/*-------------------------------------------------------------------------*/
/* misc
 * */
#undef min
#define min(a,b) (((a)<(b))?(a):(b))
// #define GET_FRAME_NUMBER(hci) (hci)->frame_no

/*-------------------------------------------------------------------------*/

/* functions
 * */

/* urb interface functions */
void urb_print (urb_t * urb, char * str, int small);
//static int hci_unlink_urb (urb_t * urb);

//static int qu_queue_urb (hci_t * hci, urb_t * urb);
void qu_return_urb (hci_t * hci, urb_t * urb, urb_t * urb_next,int bNeedMore,int resub_ok);
urb_t* qu_getPipeHead(hci_t * hci, urb_t * urb);


/* schedule functions */
void sh_scan_waiting_intr_list(hci_t *hci);
void sh_scan_return_list(void * __hci);

extern struct usb_operations hci_device_operations;
