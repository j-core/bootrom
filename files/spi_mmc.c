/*------------------------------------------------------------------------*/
/* MMCv3/SDv1/SDv2 (SPI mode) control module                              */
/*------------------------------------------------------------------------*/
/*
/  Copyright (C) 2010, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/
/* Port by Weibin Nov 15, 2011, This code does NOT check CSD and CID for size nor set_blocklen, just assume
 * default blocklen is 512 */

#include "board.h"
#include "pff.h"
#include "spi.h"

/* MMC/SD command */
#define CMD0	(0)		/* GO_IDLE_STATE */
#define CMD1	(1)		/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)		/* SEND_IF_COND */
#define CMD9	(9)		/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD52	(52)		/* IO_RW_DIRECT */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

extern void putstr(char *);
/* MMC control port and function controls  (Platform dependent) */

#define CS_LOW()	spi_cs(SpiData_Chip)	/* MMC CS = L */
#define	CS_HIGH()	spi_cs(0)	/* MMC CS = H */

#define	FCLK_SLOW()	spi_setspeed(0)	/* Set slow clock (100k-400k) */
#define	FCLK_FAST()	spi_setspeed(1)	/* SPI maxiumn speed is 12.5 MHz only, Set fast clock (MMC:20MHz max, SD:25MHz max(Class 2)) */


/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

static volatile DSTATUS Stat = STA_NOINIT;	/* Physical drive status anytime found not attached, reset */
static BYTE CardType;			/* Card type flags */

/*-----------------------------------------------------------------------*/
/* Receive data from MMC  (Platform dependent)                           */
/*-----------------------------------------------------------------------*/

#define dbg(x) putstr(x)
#if 0
#include <stdio.h>
void devdbg(const char *fmt, ...) {
	va_list args;
	char buf[256];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	putstr(buf);
}
#else
void devdbg(const char *fmt, ...) {}
#endif
/* Receive multiple byte */
static
void rcvr_spi_multi (
	BYTE *buff,		/* Pointer to data buffer */
	UINT ignore_start,	/* Number of initial bytes to ignore */
	UINT n,			/* Number of bytes to receive */
	UINT ignore_end		/* Number of final bytes to receive */
)
{
	unsigned int d;
	while (ignore_start--){
		xronebyte(0xff);
	}

	for (; (unsigned int)buff % 4 && n; n--) {
		*buff++ = xronebyte(0xff);
	}

	typedef unsigned int __attribute__((__may_alias__)) u32;

	for (; n >= 4; buff += 4, n-=4) {
		d = xronebyte(0xff);
		d <<= 8;
		d |= xronebyte(0xff);
		d <<= 8;
		d |= xronebyte(0xff);
		d <<= 8;
		d |= xronebyte(0xff);
		*(u32 *)buff = d;
	}
	if (n&2) {
		*buff++ = xronebyte(0xff);
		*buff++ = xronebyte(0xff);
	}
	if (n&1) {
		*buff++ = xronebyte(0xff);
	}
	while (ignore_end--) {
		xronebyte(0xff);
	}
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/
/* try to delay 10 uS per call */
static void dly_us(UINT n) {
	volatile unsigned uv;
	for(; n; n--) {
		for(uv = 12; uv; uv--);
	}
}
/* Fixed 500 mS for now 
 * 1:Ready, 0:Timeout */
static
int wait_ready (void) {	
	UINT count;
	unsigned char rv;
	
	for(count = 50000; count; count--) {
		rv = xronebyte(0xff);
		if (rv == 0xFF) {
		       	return 1;	/* Card goes ready */
		}
		dly_us(10);
	}
	return 0;	/* Timeout occured */
}



/*-----------------------------------------------------------------------*/
/* Deselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/

static
void deselect (void)
{
	CS_HIGH();		/* CS = H */
}

/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/

static
int select (void)	/* 1:OK, 0:Timeout */
{
	CS_LOW();		/* CS = H */
	if (wait_ready())
	       	return 1;	/* OK */
	deselect();
	return 0;	/* Timeout */
}

static
void power_off (void)	/* Disable SPI function */
{
	select();				/* Wait for card ready */
	deselect();
}


/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/

static
int rcvr_datablock (	/* 1:OK, 0:Error */
	BYTE *buff,	/* Data buffer */
	UINT ignore_start,	/* Number of initial bytes to ignore */
	UINT n,				/* Number of bytes to receive */
	UINT ignore_end		/* Number of final bytes to receive */
)
{
	BYTE token;
	UINT count;
	/* Wait for DataStart token in timeout of 200ms */
	for(count = 20000; count; count--) {
		token = xronebyte(0xff);
		if(token != 0xff)
			break;
		dly_us(10);
	}
	if(token != 0xFE)
	       	return 0;	/* Function fails if invalid DataStart token or timeout */

	rcvr_spi_multi(buff, ignore_start, n, ignore_end); /* Store trailing data to the buffer */
	xronebyte(0xff); xronebyte(0xff);	/* Discard CRC */

	return 1;		/* Function succeeded */
}

/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (		/* Return value: R1 resp (bit7==1:Failed to send) */
	BYTE cmd,		/* Command index */
	DWORD arg		/* Argument */
)
{
	BYTE n, res;


	if (cmd & 0x80) {	/* Send a CMD55 prior to ACMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select card */
	deselect();
	if (!select()) return 0xFF;

	/* Send command packet */
	xronebyte(0x40 | cmd);			/* Start + command index */
	xronebyte((BYTE)(arg >> 24));		/* Argument[31..24] */
	xronebyte((BYTE)(arg >> 16));		/* Argument[23..16] */
	xronebyte((BYTE)(arg >> 8));		/* Argument[15..8] */
	xronebyte((BYTE)arg);			/* Argument[7..0] */
	n = 0x01;				/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;		/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;		/* Valid CRC for CMD8(0x1AA) */
	xronebyte(n);

	/* Receive command resp */
	if (cmd == CMD12) xronebyte(0xff);	/* Diacard following one byte when CMD12 */
	n = 10;					/* Wait for response (10 bytes max) */
	do
		res = xronebyte(0xff);
	while ((res & 0x80) && --n);

	return res;				/* Return received response */
}

/*-----------------------------------------------------------------------*/
/* Initialize disk drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS sdc_initialize (void )
{
	BYTE n, cmd, ty, ocr[4];
	UINT count;

	FCLK_SLOW();						/* Set slow clock */
	send_cmd(CMD52, 1);	/* Since WatchDog could restart CPU while SD card in unknow state, just make sure SD card in reset mode */
	for (n = 10; n; n--) xronebyte(0xff);	/* Send 80 dummy clocks */

	ty = 0;
	dbg("SD:start\n");
	n = send_cmd(CMD0, 0);
	if (n == 1) {			/* Put the card SPI/Idle state */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
			for (n = 0; n < 4; n++) ocr[n] = xronebyte(0xff);	/* Get 32 bit return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {		/* Is the card supports vcc of 2.7-3.6V? */
				count = 100000;		/* Initialization timeout = 1 sec */
				for(; count; count--) {
					if(send_cmd(ACMD41, 1UL << 30) == 0)
						break;
					dly_us(10);
				}
				if (count && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) 
						ocr[n] = xronebyte(0xff);
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* Card id SDv2 */
				}
			}
		} else {	/* Not SDv2 card */
			if (send_cmd(ACMD41, 0) <= 1) 	{	/* SDv1 or MMC? */
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 (ACMD41(0)) */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 (CMD1(0)) */
			}
			count = 100000;
			for(;count;count--) {		/* Wait for end of initialization */
				if(send_cmd(cmd, 0) == 0)
					break;
				dly_us(10);
			}
			if (!count || send_cmd(CMD16, 512) != 0)	/* Set block length: 512 */
				ty = 0;
		}
	} else {
		dbg("SD: failed CMD0 return ");
		cmd = n >> 4;
		if(cmd <= 9)
			cmd += '0';
		else 
			cmd += 'A' - 10;
		ocr[0] = cmd;
		cmd = n & 0xf;
		if(cmd <= 9)
			cmd += '0';
		else
			cmd += 'A' - 10;
		ocr[1] = cmd;
		ocr[2] = 'n';
		ocr[3] = 0;
		dbg((char*)ocr);
		dbg("\n");
	}
	CardType = ty;	/* Card type */
	deselect();

	if (ty) {			/* OK */
		dbg("SD:init good\n");
		FCLK_FAST();			/* Set fast clock */
		Stat &= ~STA_NOINIT;	/* Clear STA_NOINIT flag */
	} else {			/* Failed */
		power_off();
		Stat = STA_NOINIT;
	}
	return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/

#if 0
DSTATUS disk_status (
	BYTE drv		/* Physical drive number (0) */
)
{
	if (drv) return STA_NOINIT;		/* Supports only drive 0 */

	return Stat;	/* Return disk status */
}
#endif


/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT sdc_readp(
	BYTE *buff,		/* Pointer to the data buffer to store read data */
	DWORD sector,	/* Start sector number (LBA) */
	WORD offset,	/* byte offset in the sector to start to read */
	WORD bcount		/* Number of byte to read (1..512) */
)
{
	WORD total;

	if (!bcount) 
		return RES_PARERR;		/* Check parameter */
	if (Stat & STA_NOINIT) 
		return RES_NOTRDY;	/* Check if drive is ready */
	if (!(CardType & CT_BLOCK))
	       	sector *= 512;	/* LBA ot BA conversion (byte addressing cards) */
	total = offset + bcount;
	if(total <= 512) { /* Single sector read */
		if ((send_cmd(CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, offset, bcount, 512 - total)) {
			bcount = 0;
		}
	} else { /* read two blocks */
		if(send_cmd(CMD18, sector) == 0) {
			total -= 512;
			if (rcvr_datablock(buff, offset, 512-offset, 0) &&
				rcvr_datablock(buff+512-offset, 0, total, 512 - total)) {
				bcount = 0;
			}
			send_cmd(CMD12, 0);	/* STOP_TRANSMISSION */
		}
	}
#if 0
	if (count == 1) {	/* Single sector read */
		if ((send_cmd(CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, 512))
			count = 0;
	} else {				/* Multiple sector read */
		if (send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, 512))
				       	break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);	/* STOP_TRANSMISSION */
		}
	}
#endif
	deselect();

	return bcount ? RES_ERROR : RES_OK;	/* Return result */
}

