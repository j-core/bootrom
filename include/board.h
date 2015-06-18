#ifndef SGM_BOARD_H
#define SGM_BOARD_H

#define sys_IntTable (*(unsigned*)0x0)
#define sys_IntVectors 256
/* Some of interrupt is fixed vector */
#define Irq_MRES	0x02	/* Manual reset */
#define Irq_CPUERR	0x09
#define Irq_DMAERR	0x0a
#define Irq_NMI		0x0b
#if 0 
#define Irq_PIT		0x10	/* 100 Hz PIT */
#endif
#define Irq_EMAC	0x11	/* irqs(0) */
#define Irq_UART0	0x12	/* irqs(1) */
#define Irq_GPS		0x13	/* irqs(2) */
#define Irq_Ext		0x14	/* irqs(3)  use by CS42518*/
#define Irq_PIO		0x15	/* irqs(4) use by PIO */
#define Irq_UART1	0x17	/* irqs(6) */
#define Irq_I2C		0x18	/* irqs(7) */
#define Irq_TMR		0x19	/* a 12 bit countdown counter */
/* External interrupt IDs */
#define EIrqID_EMAC	0
#define EIrqID_UART0	1
#define EIrqID_GPS	2
#define EIrqID_Ext	3
#define EIrqID_FW	3	/* for SGM firmware test */
#define EIrqID_PIO	4
#define EIrqID_1pps	5
#define EIrqID_UART1	6
#define EIrqID_I2C	7
/* External Interrupt ID convert to interrupt vector */
/* Assume VBR == 0x0 */
#define ID2Vect(x)	(0x11 + (x))
/* Convert external interupt ID to priority value */
#define ID2Pri(id, pri)	((pri) << ((id) <<2))
/* Convert vector to interrupt entry address */
#define Vect2Tab(x)	((x) << 2)

/* End of interrupt definations */
#define sys_DDR_BASE 0x10000000
#define sys_PIO_BASE 0xabcd0000
#define sys_SPI_BASE 0xabcd0040
#define sys_I2C_BASE 0xabcd0080
#define sys_UART0_BASE 0xabcd0100
#define sys_SYS_BASE 0xabcd0200
#define sys_UART1_BASE 0xabcd0300
#define sys_GPS_BASE 0xabcd0400
#define sys_D2A_BASE 0xabcd0500
#define sys_Cache_BASE 0xabcd0600
#define sys_FW_BASE 0xabcd0e00
#define sys_WD_BASE 0xabcd1000
#define sys_EMAC_BASE 0xabce0000
#define sys_CPUID_BASE 0xabcdff00

#define uLSRDR		0x01
#define uLSROE		0x02
#define uLSRPE		0x04
#define uLSRFE		0x08
#define uLSRBI		0x10
#define uLSRTHRE	0x20
#define uLSRTEMT	0x40
#define uLSRRFE		0x80	/* Error in Revr FIFO */

#define B115200 0x000a
#define B38400	0x001e
#define B19200	0x003c
#define B9600	0x0078
#define B4800	0x00f0

/* the following is belong to sys_SYS_BASE */
#define Sys_IntCon	0x0
/* When SIC_BRKON is set, BreadAddress will compare with Bus address to generate NMI interrupt */
#define Sys_BRKADR	0x04
/* Interrupt priority is 4 bits width, irqs(0) is [3,0], irqs(1) is [7,4] ... */
#define Sys_IntPri	0x08
#define Sys_Debug	0x0c
#define Sys_Pitt	0x10	/* PIT Throttle */
#define Sys_PitCntr	0x14	/* PIT Counter */
#define Sys_RTCSecM	0x20	/* RealTime Clock Second MSW */
#define Sys_RTCSecL	0x24	/* RealTime Clock Second LSW */
#define Sys_RTCnsec	0x28	/* Real Time Clock nSec */

#define SID_pitflag	0x8000
#define SIC_ENMI	((unsigned int) 0x1<<31)	/* Emulate NMI */
#define SIC EIRQ	((unsigned int) 0x1<<30)	/* Emulate IRQ */
#define SIC_ECER	((unsigned int) 0x1<<29)	/* Emulate CPU Address Error */
#define SIC_EDER	((unsigned int) 0x1<<28)	/* Emulate DMA Address Error */
#define SIC_EMRS	((unsigned int) 0x1<<27)	/* Emulate Manual Reset */
#define SIC_EPIT	((unsigned int) 0x1<<26)	/* Enable Periodical interval timer(PIT) */
#define SIC_TMRON	((unsigned int) 0x1<<25)	/* Enable timer */
#define SIC_BRKON	((unsigned int) 0x1<<24)	/* Break ON */
#define SIC_ILVL(x)	((unsigned int) (((x) & 0xF)<<20))	/* interrupt level for PIT */
#define SIC_IVEC(x)	((unsigned int) (((x) & 0xFF)<<12))	/* Interrupt Vector for PIT */
#define SIC_TMR		((unsigned int)0xFFF)		/* Interval Timer when 0x0, it request IRQ*/

/* PIO registers offset */
#define Poffset_IO	0x00
#define Poffset_imask	0x04
#define Poffset_redge	0x08
#define Poffset_changes 0x0c
/* Keys are connected to Parallel Input, Active low */
#define Pio_KeyEnter 	0x0001
#define Pio_KeyESC	0x0002
#define Pio_KeyNorth	0x0020
#define Pio_KeyEast	0x0040
#define Pio_KeySouth	0x0080
#define Pio_KeyWest	0x0100
/* SD_CD is active high of this bit */
#define Pio_SD_CD	0x00200000
#define Pio_1PPS	0x00800000
#define Pio_TCXO	0x00400000
/* IMPORTANT!!! Pio_DevRst is connected with with reset pins of USB, ETH-PHY
 *              and GPS. assert this bit will reset internal SMII as well as 
 *              making RST_n pin output low.
 */
#define Pio_DevRst	0x80000000
#define Pio_LEDPwr	0x00000010
#define Pio_LEDErr	0x00000020
#define Pio_TP70	0x00000040

#if 0
/* I2C is just a 32 bit register offset on sys_I2C_BASE with bits define the following */
/* To check if this interface persent, read this register would alway return 0xabcdxxxx.
 * when I2c_busy = 0, any write to this register will start the transmittion; when I2c_busy = 1
 * on assert of I2c_next, write a short with I2c_busy will stop the transmittion 
 */
#define I2c_busy	0x8000
#define I2c_next	0x4000
#define I2c_ack		0x2000
#define I2c_timeout	0x1000
#define I2c_timer	0x0800	/* outdated */
#define I2c_mask 	0xf800
#define I2c_rxbytemask	0x00ff
#else
#define I2cO_ctrl 	0x00
#define I2cO_slen	0x04
#define I2cO_speed	0x08
#define I2cO_word	0x0C

/* for I2cO_ctrl */
#define I2cC_busy	0x01
#define I2cC_timeout	0x02
#define I2cC_complete	0x04
#define I2cC_reset	0x08
#define I2cC_run	0x10
#define I2cC_irqen	0x20
#define I2cC_dat	0x80
#define I2cC_MaskDelay	0xff00
#define I2c_delay(x) ((x)<< 8)
#define I2cC_MaskAckTimeout	0xf0000
#define I2c_timeout(x)	((x) << 16)
/* for I2cO_slen */
#define I2cL_MaskXlen	0x1f
#define I2cL_MaxLen	20
#define I2cL_Maskwordcount	0xf80000
/* for I2cO_speed */
#define I2cS_MaskSpeed	0x3
#define I2cS_100k	0x0
#define I2cS_400k	0x1
#define I2cS_1m		0x2
#define I2cS_3m4	0x3
#define I2cS_clk	0x40
#endif

#define Emac_Rbuf	0x1000
#define Emac_Xbuf	0x1800
#define Emac_Ctrl	0x0000
#define Emac_xlen	0x0004
#define Emac_MACL	0x0008
#define Emac_MACH	0x000c
/* Emac_TSsec: Tx SOF time stamp seconds(64 bit)
 * Emac_TSnsec: TX SOF time stamp nSec(32 bit)
 * Emac_RSsec : Rx SOF time stamp seconds(64 bit)
 * Emac_RSnsec : RX SOF time stamp nSec(32 bit);
*/
#define Emac_TSsec	0x0020
#define Emac_TSnsec	0x0028
#define Emac_RSsec	0x0030
#define Emac_RSnsec	0x0038
#define ECtrl_TxBusy		0x04	/* was 0x4 */
#define ECtrl_RecvEnable	0x2	/* Receive enable */
#define ECtrl_Xmit		0x4	/* Read: always 0; Write: Start transmit(1) */
#define ECtrl_Multicast		0x8	/* MultiCast Enable bit */
#define ECtrl_Read		0x10	/* complete read from Receive FIFO */
#define ECtrl_RIntEnable	0x20	/* Receive interrupt enable(1) */
#define ECtrl_XIntEnable	0x40	/* Transmit interrupt enable(1) */
#define ECtrl_PROM		0x80	/* Promiscuous Mode enable(1)/disable(0) */
#define ECtrl_Complete		0x100	/* Receive packet waiting in Buffer */
#define ECtrl_CRC		0x200	/* Receive packet has CRC error */
#define ECtrl_FDuplex		0x400	/* On Full Duplex, no collision detect on sending */
#define ECtrl_RxStatus		0xF000	/* Image of ECtrl_Complete in each of 4 receiving buffer */
#define ECtrl_getrxlen(x) ((x) >> 16)	/* Length of received package, when ECtrl_Complete is set */
/* The following is defination for SPI interface registers */
/* These are the offsets of SPI registers base on sys_SPI_BASE: 0xabcd0040 */
#define Spi_Ctrl	0x0
#define Spi_Data	0x4
/* These are the bit field of Spi_Ctrl register */
#define SpiCtrl_ACS	0x01 	/* chipselect for applcation data */
#define SpiCtrl_CCS	0x04	/* chipselect for FPGA configure */
#define SpiCtrl_DCS	0x10	/* chipselect for D2A or extra SPI device */
#define SpiCtrl_setDiv(x) ((x) << 27)	/* Div contrl SPI_CK = 12.5/(div + 1), Min: 400KHz for now*/
#define SpiCtrl_Xmit	0x02
#define SpiCtrl_Busy	0x02
#define SpiCtrl_Loop	0x08	/* When it assert, mosi will connect to miso */
/* by default SPI run at 12.5 MHz,maxium speed for Spartan 3E, for SPI Flash
 * We need a DDS delay for different devices
 */
/* The following relate to sys_FW_BASE communication with AFE via UART 115200 with bridge of Manchester 
 * The byte send and receive define as followed */
#define AFEw_clk	0x01
#define AFEw_mosi	0x02
#define AFEw_cs		0x04
#define AFEw_spiLED	0x08
#define AFEr_clk	0x01
#define AFEr_mosi	0x02
#define AFEr_cs		0x04
#define AFEr_miso	0x08
/* The upper 4 bit is loop back with function of select DSM output channel to test point via AFEw_chid0, AFEw_chid1 
 * when 0x80 present */
#define AFEw_chidupdate 0x80
#define AFEc_chid(x)	(((x) << 5) | 0x80)

#endif
