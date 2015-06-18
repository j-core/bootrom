#include "pff.h"

unsigned long
pf_read_long (void *buff, 		/* Pointer to the read buffer */
		unsigned long blength, 	/* Number of bytes to read */
		unsigned long * br 	/* Pointer to number of bytes read */
	)
{
#define u16_max 65535
  unsigned short len;
  unsigned short brs;
  FRESULT res = FR_OK;

  *br = 0;
  while(blength) {
	  if(blength > u16_max)
		  len = u16_max;
	  else
		  len = blength;
	  res = pf_read(buff, len, &brs);
	  if(res == FR_OK) {
		  *br += brs;
		  buff += len;
		  blength -= len;
	  } else
		  break;
  }
  return res;
}
