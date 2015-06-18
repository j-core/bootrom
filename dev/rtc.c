#include "rtc.h"

#include "board.h"

#define READ_MEM(A) (*(volatile unsigned int*)(A))

#define SEC_HI READ_MEM(sys_SYS_BASE + Sys_RTCSecM)
#define SEC_LO READ_MEM(sys_SYS_BASE + Sys_RTCSecL)

void rtc_get_seconds(unsigned int *hi, unsigned int *lo) {
  unsigned int t;
  *hi = SEC_HI;
  *lo = SEC_LO;
  t = SEC_HI;
  if (t != *hi) {
    /* High seconds rolled over, potentially between hi read and low
       read. Reread lo to get consistent reading. It can't have
       rolled over again yet. */
    *lo = SEC_LO;
  }
}

unsigned int rtc_get_seconds_lo() {
  return SEC_LO;
}
