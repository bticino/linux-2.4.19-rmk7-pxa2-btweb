#undef USING_LINT
#define USE_DEBUG 

#ifdef USE_DEBUG
//select debug modes
#define USE_WARNING
#define USE_TRACE
#define USE_ASSERT
#endif //USE_DEBUG



typedef void irqreturn_t;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned char BOOLEAN;
typedef struct _PLATFORM_DATA {
	DWORD		dwIdRev;
	DWORD 		dwIrq;
	void 		*dev_id;
} PLATFORM_DATA, *PPLATFORM_DATA;

#define PLATFORM_NAME		"XScale with Linux 2.4 r4"

/* DEFAULT PARAMETERS */
#define LAN_BASE (0x0c000000)
#define BUS_WIDTH (32)
#define LINK_MODE (0x02) /* 10 baseT */
#define PLATFORM_RX_DMA		256 //TRANSFER_REQUEST_DMA
#define PLATFORM_TX_DMA		256 //TRANSFER_REQUEST_DMA //(256)
#define	PLATFORM_IRQ		(3)

// cache organization parameters
#define CACHE_SIZE_KB		32UL
#define CACHE_WAYS			32UL
#define CACHE_LINE_BYTES	32UL
#define CACHE_ALIGN_MASK		(~(CACHE_LINE_BYTES - 1UL))
#define CACHE_BYTES_PER_WAY		((CACHE_SIZE_KB * 1024UL) / CACHE_WAYS)

#define PLATFORM_CACHE_LINE_BYTES (CACHE_LINE_BYTES)

#define PLATFORM_IRQ_POL	(0UL)
#define PLATFORM_IRQ_TYPE	(0UL)
#define PLATFORM_DMA_THRESHOLD (200)



#define TRUE	((BOOLEAN)1)
#define FALSE	((BOOLEAN)0)
#define TRANSFER_PIO			(256UL)
#define TRANSFER_REQUEST_DMA	(255UL)
#define RX_DATA_FIFO	    (0x00UL)

#define TX_DATA_FIFO        (0x20UL)
#define		TX_CMD_A_INT_ON_COMP_		(0x80000000UL)
#define		TX_CMD_A_INT_BUF_END_ALGN_	(0x03000000UL)
#define		TX_CMD_A_INT_4_BYTE_ALGN_	(0x00000000UL)
#define		TX_CMD_A_INT_16_BYTE_ALGN_	(0x01000000UL)
#define		TX_CMD_A_INT_32_BYTE_ALGN_	(0x02000000UL)
#define		TX_CMD_A_INT_DATA_OFFSET_	(0x001F0000UL)
#define		TX_CMD_A_INT_FIRST_SEG_		(0x00002000UL)
#define		TX_CMD_A_INT_LAST_SEG_		(0x00001000UL)
#define		TX_CMD_A_BUF_SIZE_			(0x000007FFUL)
#define		TX_CMD_B_PKT_TAG_			(0xFFFF0000UL)
#define		TX_CMD_B_ADD_CRC_DISABLE_	(0x00002000UL)
#define		TX_CMD_B_DISABLE_PADDING_	(0x00001000UL)
#define		TX_CMD_B_PKT_BYTE_LENGTH_	(0x000007FFUL)

#define RX_STATUS_FIFO      (0x40UL)
#define		RX_STS_ES_			(0x00008000UL)
#define		RX_STS_MCAST_		(0x00000400UL)
#define RX_STATUS_FIFO_PEEK (0x44UL)
#define TX_STATUS_FIFO		(0x48UL)
#define TX_STATUS_FIFO_PEEK (0x4CUL)
#define ID_REV              (0x50UL)
#define		ID_REV_CHIP_ID_		(0xFFFF0000UL)	// RO
#define		ID_REV_REV_ID_		(0x0000FFFFUL)	// RO

#define INT_CFG				(0x54UL)
#define		INT_CFG_INT_DEAS_	(0xFF000000UL)	// R/W
#define		INT_CFG_IRQ_INT_	(0x00001000UL)	// RO
#define		INT_CFG_IRQ_EN_		(0x00000100UL)	// R/W
#define		INT_CFG_IRQ_POL_	(0x00000010UL)	// R/W Not Affected by SW Reset
#define		INT_CFG_IRQ_TYPE_	(0x00000001UL)	// R/W Not Affected by SW Reset

#define             INT_CFG_DEAS_CLR_BIT 14


#define INT_STS				(0x58UL)
#define		INT_STS_SW_INT_		(0x80000000UL)	// R/WC
#define		INT_STS_TXSTOP_INT_	(0x02000000UL)	// R/WC
#define		INT_STS_RXSTOP_INT_	(0x01000000UL)	// R/WC
#define		INT_STS_RXDFH_INT_	(0x00800000UL)	// R/WC
#define		INT_STS_RXDF_INT_	(0x00400000UL)	// R/WC
#define		INT_STS_TX_IOC_		(0x00200000UL)	// R/WC
#define		INT_STS_RXD_INT_	(0x00100000UL)	// R/WC
#define		INT_STS_GPT_INT_	(0x00080000UL)	// R/WC
#define		INT_STS_PHY_INT_	(0x00040000UL)	// RO
#define		INT_STS_PME_INT_	(0x00020000UL)	// R/WC
#define		INT_STS_TXSO_		(0x00010000UL)	// R/WC
#define		INT_STS_RWT_		(0x00008000UL)	// R/WC
#define		INT_STS_RXE_		(0x00004000UL)	// R/WC
#define		INT_STS_TXE_		(0x00002000UL)	// R/WC
#define		INT_STS_ERX_		(0x00001000UL)	// R/WC
#define		INT_STS_TDFU_		(0x00000800UL)	// R/WC
#define		INT_STS_TDFO_		(0x00000400UL)	// R/WC
#define		INT_STS_TDFA_		(0x00000200UL)	// R/WC
#define		INT_STS_TSFF_		(0x00000100UL)	// R/WC
#define		INT_STS_TSFL_		(0x00000080UL)	// R/WC
#define		INT_STS_RDFO_		(0x00000040UL)	// R/WC
#define		INT_STS_RDFL_		(0x00000020UL)	// R/WC
#define		INT_STS_RSFF_		(0x00000010UL)	// R/WC
#define		INT_STS_RSFL_		(0x00000008UL)	// R/WC
#define		INT_STS_GPIO2_INT_	(0x00000004UL)	// R/WC
#define		INT_STS_GPIO1_INT_	(0x00000002UL)	// R/WC
#define		INT_STS_GPIO0_INT_	(0x00000001UL)	// R/WC

#define INT_EN				(0x5CUL)
#define		INT_EN_SW_INT_EN_		(0x80000000UL)	// R/W
#define		INT_EN_TXSTOP_INT_EN_	(0x02000000UL)	// R/W
#define		INT_EN_RXSTOP_INT_EN_	(0x01000000UL)	// R/W
#define		INT_EN_RXDFH_INT_EN_	(0x00800000UL)	// R/W
#define		INT_EN_RXDF_INT_EN_		(0x00400000UL)	// R/W
#define		INT_EN_TIOC_INT_EN_		(0x00200000UL)	// R/W
#define		INT_EN_RXD_INT_EN_		(0x00100000UL)	// R/W
#define		INT_EN_GPT_INT_EN_		(0x00080000UL)	// R/W
#define		INT_EN_PHY_INT_EN_		(0x00040000UL)	// R/W
#define		INT_EN_PME_INT_EN_		(0x00020000UL)	// R/W
#define		INT_EN_TXSO_EN_			(0x00010000UL)	// R/W
#define		INT_EN_RWT_EN_			(0x00008000UL)	// R/W
#define		INT_EN_RXE_EN_			(0x00004000UL)	// R/W
#define		INT_EN_TXE_EN_			(0x00002000UL)	// R/W
#define		INT_EN_ERX_EN_			(0x00001000UL)	// R/W
#define		INT_EN_TDFU_EN_			(0x00000800UL)	// R/W
#define		INT_EN_TDFO_EN_			(0x00000400UL)	// R/W
#define		INT_EN_TDFA_EN_			(0x00000200UL)	// R/W
#define		INT_EN_TSFF_EN_			(0x00000100UL)	// R/W
#define		INT_EN_TSFL_EN_			(0x00000080UL)	// R/W
#define		INT_EN_RDFO_EN_			(0x00000040UL)	// R/W
#define		INT_EN_RDFL_EN_			(0x00000020UL)	// R/W
#define		INT_EN_RSFF_EN_			(0x00000010UL)	// R/W
#define		INT_EN_RSFL_EN_			(0x00000008UL)	// R/W
#define		INT_EN_GPIO2_INT_		(0x00000004UL)	// R/W
#define		INT_EN_GPIO1_INT_		(0x00000002UL)	// R/W
#define		INT_EN_GPIO0_INT_		(0x00000001UL)	// R/W

#define BYTE_TEST				(0x64UL)
#define FIFO_INT				(0x68UL)
#define		FIFO_INT_TX_AVAIL_LEVEL_	(0xFF000000UL)	// R/W
#define		FIFO_INT_TX_STS_LEVEL_		(0x00FF0000UL)	// R/W
#define		FIFO_INT_RX_AVAIL_LEVEL_	(0x0000FF00UL)	// R/W
#define		FIFO_INT_RX_STS_LEVEL_		(0x000000FFUL)	// R/W

#define RX_CFG					(0x6CUL)
#define		RX_CFG_RX_END_ALGN_		(0xC0000000UL)	// R/W
#define			RX_CFG_RX_END_ALGN4_		(0x00000000UL)	// R/W
#define			RX_CFG_RX_END_ALGN16_		(0x40000000UL)	// R/W
#define			RX_CFG_RX_END_ALGN32_		(0x80000000UL)	// R/W
#define		RX_CFG_RX_DMA_CNT_		(0x0FFF0000UL)	// R/W
#define		RX_CFG_RX_DUMP_			(0x00008000UL)	// R/W
#define		RX_CFG_RXDOFF_			(0x00001F00UL)	// R/W
#define		RX_CFG_RXBAD_			(0x00000001UL)	// R/W

#define TX_CFG					(0x70UL)
#define		TX_CFG_TX_DMA_LVL_		(0xE0000000UL)	// R/W
#define		TX_CFG_TX_DMA_CNT_		(0x0FFF0000UL)	// R/W Self Clearing
#define		TX_CFG_TXS_DUMP_		(0x00008000UL)	// Self Clearing
#define		TX_CFG_TXD_DUMP_		(0x00004000UL)	// Self Clearing
#define		TX_CFG_TXSAO_			(0x00000004UL)	// R/W
#define		TX_CFG_TX_ON_			(0x00000002UL)	// R/W
#define		TX_CFG_STOP_TX_			(0x00000001UL)	// Self Clearing

#define HW_CFG					(0x74UL)
#define		HW_CFG_TTM_				(0x00200000UL)	// R/W
#define		HW_CFG_SF_				(0x00100000UL)	// R/W
#define		HW_CFG_TX_FIF_SZ_		(0x000F0000UL)	// R/W
#define		HW_CFG_TR_				(0x00003000UL)	// R/W
#define     HW_CFG_PHY_CLK_SEL_		(0x00000060UL)  // R/W
#define         HW_CFG_PHY_CLK_SEL_INT_PHY_	(0x00000000UL) // R/W
#define         HW_CFG_PHY_CLK_SEL_EXT_PHY_	(0x00000020UL) // R/W
#define         HW_CFG_PHY_CLK_SEL_CLK_DIS_ (0x00000040UL) // R/W
#define     HW_CFG_SMI_SEL_			(0x00000010UL)  // R/W
#define     HW_CFG_EXT_PHY_DET_		(0x00000008UL)  // RO
#define     HW_CFG_EXT_PHY_EN_		(0x00000004UL)  // R/W
#define		HW_CFG_32_16_BIT_MODE_	(0x00000004UL)	// RO
#define     HW_CFG_SRST_TO_			(0x00000002UL)  // RO
#define		HW_CFG_SRST_			(0x00000001UL)	// Self Clearing

#define RX_DP_CTRL				(0x78UL)
#define		RX_DP_CTRL_RX_FFWD_		(0x00000FFFUL)	// R/W
#define		RX_DP_CTRL_FFWD_BUSY_	(0x80000000UL)	// RO

#define RX_FIFO_INF				(0x7CUL)
#define		RX_FIFO_INF_RXSUSED_	(0x00FF0000UL)	// RO
#define		RX_FIFO_INF_RXDUSED_	(0x0000FFFFUL)	// RO

#define TX_FIFO_INF				(0x80UL)
#define		TX_FIFO_INF_TSUSED_		(0x00FF0000UL)  // RO
#define		TX_FIFO_INF_TSFREE_		(0x00FF0000UL)	// RO
#define		TX_FIFO_INF_TDFREE_		(0x0000FFFFUL)	// RO

#define PMT_CTRL				(0x84UL)
#define		PMT_CTRL_PM_MODE_			(0x00018000UL)	// Self Clearing
#define		PMT_CTRL_PHY_RST_			(0x00000400UL)	// Self Clearing
#define		PMT_CTRL_WOL_EN_			(0x00000200UL)	// R/W
#define		PMT_CTRL_ED_EN_				(0x00000100UL)	// R/W
#define		PMT_CTRL_PME_TYPE_			(0x00000040UL)	// R/W Not Affected by SW Reset
#define		PMT_CTRL_WUPS_				(0x00000030UL)	// R/WC
#define			PMT_CTRL_WUPS_NOWAKE_		(0x00000000UL)	// R/WC
#define			PMT_CTRL_WUPS_ED_			(0x00000010UL)	// R/WC
#define			PMT_CTRL_WUPS_WOL_			(0x00000020UL)	// R/WC
#define			PMT_CTRL_WUPS_MULTI_		(0x00000030UL)	// R/WC
#define		PMT_CTRL_PME_IND_		(0x00000008UL)	// R/W
#define		PMT_CTRL_PME_POL_		(0x00000004UL)	// R/W
#define		PMT_CTRL_PME_EN_		(0x00000002UL)	// R/W Not Affected by SW Reset
#define		PMT_CTRL_READY_			(0x00000001UL)	// RO

#define GPIO_CFG				(0x88UL)
#define		GPIO_CFG_LED3_EN_		(0x40000000UL)	// R/W
#define		GPIO_CFG_LED2_EN_		(0x20000000UL)	// R/W
#define		GPIO_CFG_LED1_EN_		(0x10000000UL)	// R/W
#define		GPIO_CFG_GPIO2_INT_POL_	(0x04000000UL)	// R/W
#define		GPIO_CFG_GPIO1_INT_POL_	(0x02000000UL)	// R/W
#define		GPIO_CFG_GPIO0_INT_POL_	(0x01000000UL)	// R/W
#define		GPIO_CFG_EEPR_EN_		(0x00E00000UL)	// R/W
#define		GPIO_CFG_GPIOBUF2_		(0x00040000UL)	// R/W
#define		GPIO_CFG_GPIOBUF1_		(0x00020000UL)	// R/W
#define		GPIO_CFG_GPIOBUF0_		(0x00010000UL)	// R/W
#define		GPIO_CFG_GPIODIR2_		(0x00000400UL)	// R/W
#define		GPIO_CFG_GPIODIR1_		(0x00000200UL)	// R/W
#define		GPIO_CFG_GPIODIR0_		(0x00000100UL)	// R/W
#define		GPIO_CFG_GPIOD4_		(0x00000020UL)	// R/W
#define		GPIO_CFG_GPIOD3_		(0x00000010UL)	// R/W
#define		GPIO_CFG_GPIOD2_		(0x00000004UL)	// R/W
#define		GPIO_CFG_GPIOD1_		(0x00000002UL)	// R/W
#define		GPIO_CFG_GPIOD0_		(0x00000001UL)	// R/W

#define GPT_CFG					(0x8CUL)
#define		GPT_CFG_TIMER_EN_		(0x20000000UL)	// R/W
#define		GPT_CFG_GPT_LOAD_		(0x0000FFFFUL)	// R/W

#define GPT_CNT					(0x90UL)
#define		GPT_CNT_GPT_CNT_		(0x0000FFFFUL)	// RO

#define FPGA_REV				(0x94UL)
#define		FPGA_REV_FPGA_REV_		(0x0000FFFFUL)	// RO

#define ENDIAN					(0x98UL)
#define FREE_RUN				(0x9CUL)
#define RX_DROP					(0xA0UL)
#define MAC_CSR_CMD				(0xA4UL)
#define		MAC_CSR_CMD_CSR_BUSY_	(0x80000000UL)	// Self Clearing
#define		MAC_CSR_CMD_R_NOT_W_	(0x40000000UL)	// R/W
#define		MAC_CSR_CMD_CSR_ADDR_	(0x000000FFUL)	// R/W

#define MAC_CSR_DATA			(0xA8UL)
#define AFC_CFG					(0xACUL)
#define		AFC_CFG_AFC_HI_			(0x00FF0000UL)	// R/W
#define		AFC_CFG_AFC_LO_			(0x0000FF00UL)	// R/W
#define		AFC_CFG_BACK_DUR_		(0x000000F0UL)	// R/W
#define		AFC_CFG_FCMULT_			(0x00000008UL)	// R/W
#define		AFC_CFG_FCBRD_			(0x00000004UL)	// R/W
#define		AFC_CFG_FCADD_			(0x00000002UL)	// R/W
#define		AFC_CFG_FCANY_			(0x00000001UL)	// R/W

#define E2P_CMD					(0xB0UL)
#define		E2P_CMD_EPC_BUSY_		(0x80000000UL)	// Self Clearing
#define		E2P_CMD_EPC_CMD_		(0x70000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_READ_	(0x00000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_EWDS_	(0x10000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_EWEN_	(0x20000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_WRITE_	(0x30000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_WRAL_	(0x40000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_ERASE_	(0x50000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_ERAL_	(0x60000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_RELOAD_	(0x70000000UL)  // R/W
#define		E2P_CMD_EPC_TIMEOUT_	(0x00000200UL)	// R
#define		E2P_CMD_MAC_ADDR_LOADED_	(0x00000100UL)	// RO
#define		E2P_CMD_EPC_ADDR_		(0x000000FFUL)	// R/W

#define E2P_DATA				(0xB4UL)
#define		E2P_DATA_EEPROM_DATA_	(0x000000FFUL)	// R/W
//end of lan register offsets and bit definitions

#define LAN_REGISTER_EXTENT		(0x00000100UL)

//The following describes the synchronization policies used in this driver.
//Register Name				Policy
//RX_DATA_FIFO				Only used by the Rx Thread, Rx_ProcessPackets
//TX_DATA_FIFO				Only used by the Tx Thread, Tx_SendSkb
//RX_STATUS_FIFO			Only used by the Rx Thread, Rx_ProcessPackets
//RX_STATUS_FIFO_PEEK		Not used.
//TX_STATUS_FIFO			Used in	Tx_CompleteTx in Tx_UpdateTxCounters.
//							protected by TxCounterLock
//TX_STATUS_FIFO_PEEK		Not used.
//ID_REV					Read only
//INT_CFG					Set in Lan_Initialize, 
//							protected by IntEnableLock
//INT_STS					Sharable, 
//INT_EN					Initialized at startup, 
//							Used in Rx_ProcessPackets
//							otherwise protected by IntEnableLock
//BYTE_TEST					Read Only
//FIFO_INT					Initialized at startup,
//                          During run time only accessed by 
//                              Tx_HandleInterrupt, and Tx_SendSkb and done in a safe manner
//RX_CFG					Used during initialization
//                          During runtime only used by Rx Thread
//TX_CFG					Only used during initialization
//HW_CFG					Only used during initialization
//RX_DP_CTRL				Only used in Rx Thread, in Rx_FastForward
//RX_FIFO_INF				Read Only, Only used in Rx Thread, in Rx_PopRxStatus
//TX_FIFO_INF				Read Only, Only used in Tx Thread, in Tx_GetTxStatusCount, Tx_SendSkb, Tx_CompleteTx
//PMT_CTRL					Not Used
//GPIO_CFG					used during initialization, in Lan_Initialize
//                          used for debugging
//                          used during EEPROM access.
//                          safe enough to not require a lock
//GPT_CFG					protected by GpTimerLock
//GPT_CNT					Not Used
//ENDIAN					Not Used
//FREE_RUN					Read only
//RX_DROP					Used in Rx Interrupt Handler,
//                          and get_stats.
//                          safe enough to not require a lock.
//MAC_CSR_CMD				Protected by MacPhyLock
//MAC_CSR_DATA				Protected by MacPhyLock
//                          Because the two previous MAC_CSR_ registers are protected
//                            All MAC, and PHY registers are protected as well.
//AFC_CFG					Used during initialization, in Lan_Initialize
//                          During run time, used in timer call back, in Phy_UpdateLinkMode
//E2P_CMD					Used during initialization, in Lan_Initialize
//                          Used in EEPROM functions
//E2P_DATA					Used in EEPROM functions

//DMA Transfer structure
typedef struct _DMA_XFER 
{
	DWORD dwLanReg;
	DWORD *pdwBuf;
	DWORD dwDmaCh;
	DWORD dwDwCnt;
	BOOLEAN fMemWr;
} DMA_XFER;

typedef struct _FLOW_CONTROL_PARAMETERS
{
	DWORD MaxThroughput;
	DWORD MaxPacketCount;
	DWORD PacketCost;
	DWORD BurstPeriod;
	DWORD IntDeas;
} FLOW_CONTROL_PARAMETERS, *PFLOW_CONTROL_PARAMETERS;



//See readme.txt for a description of how these
//functions must be implemented
DWORD Platform_Initialize(
	PPLATFORM_DATA platformData,
	DWORD dwLanBase,
	DWORD dwBusWidth);
void Platform_CleanUp(
	PPLATFORM_DATA platformData);
BOOLEAN Platform_Is16BitMode(
	PPLATFORM_DATA platformData);
BOOLEAN Platform_RequestIRQ(
	PPLATFORM_DATA platformData,
	DWORD dwIrq,
	irqreturn_t (*pIsr)(int irq,void *dev_id,struct pt_regs *regs),
	void *dev_id);
DWORD Platform_CurrentIRQ(
	PPLATFORM_DATA platformData);
void Platform_FreeIRQ(
	PPLATFORM_DATA platformData);
BOOLEAN Platform_IsValidDmaChannel(DWORD dwDmaCh);
BOOLEAN Platform_DmaInitialize(
	PPLATFORM_DATA platformData,
	DWORD dwDmaCh);
BOOLEAN Platform_DmaDisable(
	PPLATFORM_DATA platformData,
	const DWORD dwDmaCh);
void Platform_CacheInvalidate(
	PPLATFORM_DATA platformData,
	const void * const pStartAddress,
	const DWORD dwLengthInBytes);
void Platform_CachePurge(
	PPLATFORM_DATA platformData,
	const void * const pStartAddress,
	const DWORD dwLengthInBytes);
DWORD Platform_RequestDmaChannel(
	PPLATFORM_DATA platformData);
void Platform_ReleaseDmaChannel(
	PPLATFORM_DATA platformData,
	DWORD dwDmaChannel);
BOOLEAN Platform_DmaStartXfer(
	PPLATFORM_DATA platformData,
	const DMA_XFER * const pDmaXfer);
DWORD Platform_DmaGetDwCnt(
	PPLATFORM_DATA platformData,
	const DWORD dwDmaCh);
void Platform_DmaComplete(
	PPLATFORM_DATA platformData,
	const DWORD dwDmaCh);
void Platform_GetFlowControlParameters(
	PPLATFORM_DATA platformData,
	PFLOW_CONTROL_PARAMETERS flowControlParameters,
	BOOLEAN useDma);
void Platform_WriteFifo(
	DWORD dwLanBase,
	DWORD *pdwBuf,
	DWORD dwDwordCount);
void Platform_ReadFifo(
	DWORD dwLanBase,
	DWORD *pdwBuf,
	DWORD dwDwordCount);


/*******************************************************
* Macro: SMSC_TRACE
* Description: 
*    This macro is used like printf. 
*    It can be used anywhere you want to display information
*    For any release version it should not be left in 
*      performance sensitive Tx and Rx code paths.
*    To use this macro define USE_TRACE and set bit 0 of debug_mode
*******************************************************/

#undef USING_LINT

#ifdef USING_LINT
extern void SMSC_TRACE(const char * a, ...);
#else //not USING_LINT
#ifdef USE_TRACE
extern DWORD debug_mode;
#ifndef USE_WARNING
#define USE_WARNING
#endif
#	define SMSC_TRACE(msg,args...)			\
	if(debug_mode&0x01UL) {					\
		printk("SMSC: " msg "\n", ## args);	\
	}
#else
#	define SMSC_TRACE(msg,args...)
#endif
#endif //not USING_LINT

/*******************************************************
* Macro: SMSC_WARNING
* Description: 
*    This macro is used like printf. 
*    It can be used anywhere you want to display warning information
*    For any release version it should not be left in 
*      performance sensitive Tx and Rx code paths.
*    To use this macro define USE_TRACE or 
*      USE_WARNING and set bit 1 of debug_mode
*******************************************************/
#ifdef USING_LINT
extern void SMSC_WARNING(const char * a, ...);
#else //not USING_LINT
#ifdef USE_WARNING
extern DWORD debug_mode;
#ifndef USE_ASSERT
#define USE_ASSERT
#endif
#	define SMSC_WARNING(msg, args...)				\
	if(debug_mode&0x02UL) {							\
		printk("SMSC_WARNING: " msg "\n",## args);	\
	}
#else
#	define SMSC_WARNING(msg, args...)
#endif
#endif //not USING_LINT


/*******************************************************
* Macro: SMSC_ASSERT
* Description: 
*    This macro is used to test assumptions made when coding. 
*    It can be used anywhere, but is intended only for situations
*      where a failure is fatal. 
*    If code execution where allowed to continue it is assumed that 
*      only further unrecoverable errors would occur and so this macro
*      includes an infinite loop to prevent further corruption.
*    Assertions are only intended for use during developement to 
*      insure consistency of logic through out the driver.
*    A driver should not be released if assertion failures are 
*      still occuring.
*    To use this macro define USE_TRACE or USE_WARNING or
*      USE_ASSERT
*******************************************************/
#ifdef USING_LINT
extern void SMSC_ASSERT(BOOLEAN condition);
#else //not USING_LINT
#ifdef USE_ASSERT
#	define SMSC_ASSERT(condition)													\
	if(!(condition)) {																\
		printk("SMSC_ASSERTION_FAILURE: File=" __FILE__ ", Line=%d\n",__LINE__);	\
	}
#else 
#	define SMSC_ASSERT(condition)
#endif
#endif //not USING_LINT

