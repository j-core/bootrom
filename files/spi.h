#ifndef __SPI_H__
#define __SPI_H__

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

/* TODO: how to implement this offset change */
/* Digilent 1600E: only have SpiData_Chip, offset 0
 * SGM100	SpiData_Chip offset 0
 * 		SpiConf_Chip use for FPGA configure
 * TestCPU:	SpiData_Chip use for SD card
 * 		SpiConf_Chip is for both Data and FPGA configure Offset TBD
 */
#ifdef SPIFLASHM25P32
#define SPIFLASH_FS_BASE_ADDRESS (0x0)   /* 4M FAT file system base address from 0  */
#else
#define SPIFLASH_FS_BASE_ADDRESS (0x180000)   /* supposed 512K FAT file system base address  */
#endif

#define SpiData_Chip	1
#define SpiConf_Chip	2
#define SPI_CODEC	3

extern void spi_cs(int chip);
extern unsigned char xronebyte(unsigned char c);
/* 0: 400KHz,else max: 12.5 MHz */
extern void spi_setspeed(int );
/* The following is SPI Flash write disable them for now */
#if 0
extern int spi_read(int chip, unsigned addr, unsigned char *bufptr, unsigned len);
extern unsigned char get_sr(int chip);
extern void enable_write(int chip);
extern void disable_write(int chip);
extern void set_sr(int chip, unsigned char c);
extern int erase_sector(int chip, unsigned char sector);
extern int spi_writepage(int chip, unsigned addr, unsigned char *ucptr, unsigned len);
#endif

#endif
