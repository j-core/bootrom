#include "rtc.h"

#include "board.h"

void rtc_get_seconds(unsigned int *hi, unsigned int *lo) {
  unsigned int t;
  *hi = DEVICE_AIC0->rtc_sec_hi;
  *lo = DEVICE_AIC0->rtc_sec_lo;
  t = DEVICE_AIC0->rtc_sec_hi;
  if (t != *hi) {
    /* High seconds rolled over, potentially between hi read and low
       read. Reread lo to get consistent reading. It can't have
       rolled over again yet. */
    *lo = DEVICE_AIC0->rtc_sec_lo;
  }
}

unsigned int rtc_get_seconds_lo() {
  return DEVICE_AIC0->rtc_sec_lo;
}

void rtc_get_time(unsigned int *sec, unsigned int *nsec) {
  unsigned int t;
  *sec = DEVICE_AIC0->rtc_sec_lo;
  *nsec = DEVICE_AIC0->rtc_nsec;
  t = DEVICE_AIC0->rtc_sec_lo;
  if (t != *sec) {
    /* Low seconds rolled over, potentially between sec read and nsec
       read. Reread sec to get consistent reading. It can't have
       rolled over again yet. */
    *nsec = DEVICE_AIC0->rtc_nsec;
  }
}
