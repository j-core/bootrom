/* Memory Test Library
 * written by Michael Barr
 * made actually correct by D. Dionne / J. McCann */

#include <stdio.h>
#include "memtest.h"

#define MSG(X)  printf("memtest: %s...\n", (X))
#define REP(X)  printf("       : %.8lx\n", (X))
#define REP_DATUM(X)  printf("       : %.8" DATUM_FORMAT "\n", (X))
#define PASS    printf("pass.\n\n")
#define FAIL_DATUM(X) do { printf("fail %.8" DATUM_FORMAT "\n\n", (X)); return (X); } while (0);
#define FAIL_ADDR(X) do { printf("fail %p\n\n", (X)); return (X); } while (0);
/* ; return (X) */

/*typedef unsigned long datum;*/

/*******************************************************************
 *
 * Function:    memTestDataBus()
 *
 * Description: Test the data bus wiring in a memory region by
 *              performing a walking 1's test at a fixed address
 *              within that region. The address (and hence the
 *              memory region) is selected by the caller.
 *
 * Notes:
 *
 * Returns:     0 if the test succeeds.
 *              A non-zero result is the first pattern that failed.
 *
 *******************************************************************/
datum memTestDataBus(volatile datum * address)
{
  datum pattern;
  
  MSG("Walking 1s");
  /*
   * Perform a walking 1's test at the given address
   */
  for (pattern = 1; pattern != 0; pattern <<= 1)
  {
    REP_DATUM(pattern);
    /*
     * Write the test pattern
     */
    *address = pattern;
    
    /*
     * Read it back (immediately is okay for this test).
     */
    if (*address != pattern)
    {
      FAIL_DATUM(pattern);
    }
  }

  PASS;  
  return (0);
} /* memTestDataBus() */


/**********************************************************************
 *
 * Function:    memTestAddressBus()
 *
 * Description: Test the address bus wiring in a memory region by
 *              performing a walking 1's test on the relevant bits
 *              of the address and checking for aliasing.  The test
 *              will find single-bit address failures such as stuck
 *              -high, stuck-low, and shorted pins.  The base address
 *              and size of the region are selected by the caller.
 *
 * Notes:       For best results, the selected base address should
 *              have enough LSB 0's to guarantee single address bit
 *              changes.  For example, to test a 64-Kbyte region, 
 *              select a base address on a 64-Kbyte boundary.  Also, 
 *              select the region size as a power-of-two--if at all 
 *              possible.
 *
 * Returns:     0 if the test succeeds.  
 *              A non-zero result is the first address at which an
 *              aliasing problem was uncovered.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
 **********************************************************************/
datum *memTestAddressBus(volatile datum * baseAddress, unsigned long nBytes)
{
  unsigned long addressMask = ((nBytes/sizeof(datum)) - 1);
  unsigned long index;
  unsigned long testIndex;
  
  datum pattern     = (datum) 0xAAAAAAAA;
  datum antipattern = (datum) 0x55555555;
  
  MSG("Addressing write");
  /*
   * Write the default pattern at each of the power-of-two indexs.
   */
  for (index = 1; (index & addressMask) != 0; index <<= 1)
  {
    REP(index);
    baseAddress[index] = pattern;
  }

  MSG("Check stuck high");
  /*
   * Check for address bits stuck high
   */
  testIndex = 0;
  baseAddress[testIndex] = antipattern;
  
  for (index = 1; (index & addressMask) != 0; index <<= 1)
  {
    REP(index);
    if (baseAddress[index] != pattern)
    {
      FAIL_ADDR((datum *) &baseAddress[index]);
    }
  }
  
  baseAddress[testIndex] = pattern;

  MSG("Check stuck low");
  /*
   * Check for address bits stuck low or shorted
   */
  for (testIndex = 1; (testIndex & addressMask) != 0; testIndex <<= 1)
  {
    REP(testIndex);
    baseAddress[testIndex] = antipattern;
    
    for (index = 1; (index & addressMask) != 0; index <<= 1)
    {
      if ((baseAddress[index] != pattern) && (index != testIndex))
      {
        FAIL_ADDR((datum *) &baseAddress[testIndex]);
      }
    }
    
    baseAddress[testIndex] = pattern;
  }
  PASS;
  return (datum *) 0;
  
} /* memTestAddressBus() */

volatile long woof;

void bark(void)
{ woof = 0; }

/**********************************************************************
 *
 * Function:    memTestDevice()
 *
 * Description: Test the integrity of a physical memory device by
 *              performing an increment/decrement test over the
 *              entire region.  In the process every storage bit 
 *              in the device is tested as a zero and a one.  The
 *              base address and the size of the region are
 *              selected by the caller.
 *
 * Notes:       
 *
 * Returns:     0 if the test succeeds.  Also, in that case, the
 *              entire memory region will be filled with zeros.
 *
 *              A non-zero result is the first address at which an
 *              incorrect value was read back.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
 **********************************************************************/
datum *memTestDevice(volatile datum * baseAddress, unsigned long nBytes)	
{
  unsigned long index;
  unsigned long nWords = nBytes / sizeof(datum);

  datum pattern;
  datum antipattern;

  MSG("Pattern test fill");
  /*
   * Fill memory with a known pattern.
   */
  for (pattern = 0x01010101, index = 0; index < nWords; pattern += 0x01010101, index++)
  {
    baseAddress[index] = pattern;
bark();
  }

  MSG("Check pattern");
  /*
   * Check each location and invert it for the second pass.
   */
  for (pattern = 0x01010101, index = 0; index < nWords; pattern += 0x01010101, index++)
  {
bark();
    if (baseAddress[index] != pattern)
    {
      printf("pattern %x !=\nvalue   %x\n", pattern, baseAddress[index]);
      FAIL_ADDR((datum *) &baseAddress[index]);
    }

    antipattern = ~pattern;
    baseAddress[index] = antipattern;
  }

  MSG("Check inverse");
  /*
   * Check each location for the inverted pattern and zero it.
   */
  for (pattern = 0x01010101, index = 0; index < nWords; pattern += 0x01010101, index++)
  {
    antipattern = ~pattern;
    if (baseAddress[index] != antipattern)
    {
      printf("pattern %x !=\nvalue   %x\n", antipattern, baseAddress[index]);
      FAIL_ADDR((datum *) &baseAddress[index]);
    }

    baseAddress[index] = 0;
  }

  PASS;
  return (datum *) 0;

} /* memTestDevice() */

/**********************************************************************
 *
 * Function:    memTest(volatile datum * baseAddress, unsigned long nBytes)
 *
 * Description: Test a power of 2 size block of RAM.
 * 
 * Notes:       
 *
 * Returns:     0 on success.
 *              Otherwise -1 indicates failure.
 *
 **********************************************************************/
int memTest(volatile datum *baseAddress, unsigned long nBytes)
{
  if ((memTestDataBus(baseAddress) != 0) ||
      (memTestAddressBus(baseAddress, nBytes) != (datum *) 0) ||
      (memTestDevice(baseAddress, nBytes) != (datum *) 0))
  {
    return (-1);
  }
  else
  {
    return (0);
  }		
} /* memTest() */

