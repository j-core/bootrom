#ifndef MEMTEST_H
#define MEMTEST_H

// MemTestLib header file
// Version 1.00 Written by AA 7/31/00

/*
#define _16BITDATABUS

#ifdef _16BITDATABUS
typedef unsigned int datum;
#else
typedef unsigned char datum;
#endif
*/

typedef unsigned int datum;
#define DATUM_FORMAT "x"

extern datum memTestDataBus(volatile datum *address);
extern datum *memTestAddressBus(volatile datum *baseAddress, unsigned long nBytes);
extern datum *memTestDevice(volatile datum *baseAddress, unsigned long nBytes);
extern int memTest(volatile datum *baseAddress, unsigned long nBytes);
 
#endif
