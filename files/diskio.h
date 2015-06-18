/* This header file provide disk access functions for PetitFatFS */
#ifndef _DISKIO

#include "pff.h"

/* Method of SPI Flash access in spi.c */
extern DSTATUS spif_initialize (void);
extern DRESULT spif_readp (BYTE* des, DWORD sector, WORD offset, WORD bytecount);

/* Method of SD Card access in spi_mmc.c */
extern DSTATUS sdc_initialize(void);
extern DRESULT sdc_readp(BYTE* dest, DWORD sector, WORD offset, WORD bytecount);

extern unsigned long pf_read_long(void *, unsigned long, unsigned long *);
#define _DISKIO
#endif
