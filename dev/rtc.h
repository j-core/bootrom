#ifndef DEV_RTC_H
#define DEV_RTC_H

/* Reads the low and high seconds counters. Avoids inconsistent
   measurements by rereading if necessary. */
void rtc_get_seconds(unsigned int *hi, unsigned int *lo);

unsigned int rtc_get_seconds_lo();

/* Reads low seconds and nsec counters */
void rtc_get_time(unsigned int *sec, unsigned int *nsec);

#endif
