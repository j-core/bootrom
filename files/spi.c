/* This file come from fwprg.c providing Bootloader SPI read feature only for code size reason */
#include "board.h"
#include "spi.h"
#include "pff.h"

#define SPI_RES		0xAB
#define SPI_RDID	0x9F
#define SPI_READ	0x03
#define SPI_FAST_READ	0x0B

#define SPIDATA	(DEVICE_FLASH->data)
#define SPICTL	(DEVICE_FLASH->ctrl)

static unsigned chipselect_status;
static unsigned spi_speed;

#ifdef Digilent_Boot
	#define Sectors	32
#else
	#define Sectors 64
#endif
#define Page_size	256
#define Pages		256

/* chip: 0 none of cs is active; 1: Data FLASH cs is actived 2: FPGA configure cs is active */
void spi_cs(int chip) {
	unsigned rv;
	/* just in case for pervious transmission if not finished */
	rv = SPICTL;
	while(rv & SpiCtrl_Xmit) {
		rv = SPICTL;
	}
	if(chip) {
		if(chip == SpiData_Chip) {
			chipselect_status = spi_speed;
		} else if(chip == SpiConf_Chip) {
			chipselect_status = spi_speed | SpiCtrl_CCS | SpiCtrl_ACS;
		}
	} else
		chipselect_status = SpiCtrl_ACS;
	SPICTL = chipselect_status;
}
/* if speed == 0: 400KHz, if speed = true: 12.5 MHz */
void spi_setspeed(int speed) {
	if(!speed)
		spi_speed = SpiCtrl_setDiv(31);
	else
		spi_speed = SpiCtrl_setDiv(0);
}
unsigned char xronebyte(unsigned char c) {
	unsigned uv;
	SPIDATA = c;
	SPICTL = chipselect_status | SpiCtrl_Xmit;
	uv = SPICTL;
	while(uv & SpiCtrl_Busy) 
		uv = SPICTL;
	uv = SPIDATA;
	return uv & 0xff;
}
int spi_read(int chip, unsigned addr, unsigned char *bufptr, unsigned len) {
	unsigned char *ptr;

	ptr = bufptr;
	spi_cs(chip);
	xronebyte(SPI_READ);
	xronebyte(addr >> 16);
	xronebyte(addr >> 8);
	xronebyte(addr);
	while(len) {
		*ptr++ = xronebyte(0);
		len--;
	}
	spi_cs(0);
	
	return 1;  
}
#if 0
unsigned char get_sr(int chip) {
	unsigned char rc;
	spi_cs(chip);
	xronebyte(SPI_RDSR);
	rc = xronebyte(0);
	spi_cs(0);
	return rc;
}
#endif
/* return manufacturer ID | MemoryType | MemoryCapacity | UID format */
static unsigned get_rdid(int chip) {
	unsigned rv;
	int i;
	spi_cs(chip);
	xronebyte(SPI_RDID);
	rv = 0;
	for(i = 0; i < 4; i++) {
		rv <<= 8;
		rv |= xronebyte(0);
	}
	spi_cs(0);
	return rv;
}
static char get_res(int chip) {
	char rc;
	spi_cs(chip);
	xronebyte(SPI_RES);
	xronebyte(0);
	xronebyte(0);
	xronebyte(0);
	rc = xronebyte(0);
	spi_cs(0);
	return rc;
}
/* Start of diskio.h functions, These are default SPI Flash readonly drive */
static unsigned dataflash_size = 0;
DSTATUS spif_initialize(void) {
	unsigned uv;
	/*  use get_rdid to test the max size of FLASH */
	spi_setspeed(1);
	uv = get_res(SpiData_Chip);	/* SPI Flash wakeup */
	if(!uv)
		return STA_NOINIT;
	uv = get_rdid(SpiData_Chip);
	uv >>= 8;
	uv &= 0xff;	/* get the cap byte */
	if(uv == 0x16){	/* M25P32 */
		dataflash_size = 4194304;
	} else if(uv == 0x15) {	/* M25P16 */
		dataflash_size = 2097152;	/* Sectors * Pages * Page_size */
	} else if(uv == 0x17) { /* MX25L64 */
		dataflash_size = 8388608;
	} else {
		dataflash_size = 0;
		return STA_NOINIT;
	}
	return 0;
}
DRESULT spif_readp (BYTE* dest, DWORD sector, WORD soffset, WORD bytecount) {
	unsigned uv;
	if(!dataflash_size)
		return RES_PARERR;
	uv = sector * 512 + soffset;
	if((uv + bytecount) >= dataflash_size)
		return RES_PARERR;
	spi_read(SpiData_Chip, uv, dest, bytecount);
	return RES_OK;
}
