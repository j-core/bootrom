#include "uart.h"

#if CONFIG_DDR == 1
#include "ddr.h"
#endif
#if CONFIG_LOAD_ELF == 1
#include "loadelf.h"
#endif
#include "lcd.h"
#if CONFIG_TEST_MEM == 1
#include <stdio.h>
#include "memtest.h"
#elif CONFIG_TEST_MEM == 2
#include <stdio.h>
#endif
#if CONFIG_GDB_STUB == 1
#include "gdb.h"
#endif
#if CONFIG_GPS_ROLLOVER_TEST == 1
#include <stdio.h>
#endif

#define LEDPORT (*(volatile unsigned long  *)0xabcd0000)

#ifndef MEM_TEST_ADDR
#define MEM_TEST_ADDR 0x10000000
#endif
#ifndef MEM_TEST_SIZE
/* 64 MB */
#define MEM_TEST_SIZE 64*1024*1024
#endif

extern char version_string[];

char ram0[256]; /* working ram for CPU tests */

void
putstr (char *str)
{
  while (*str)
    {
      if (*str == '\n')
	uart_tx (0, '\r');
      uart_tx (0, *(str++));
    }
}

void
led(int v)
{
//  LEDPORT = v;
}

#if CONFIG_TEST_MEM == 2
#define uint16 unsigned short
void memtest2(void) {
  int i;
  uint16 r, w, v, *p;
  unsigned int writes, fails;

  for (writes = fails = 0;;) {
    // loop over all 128MB of DDR RAM
    for (w = 0, p = (uint16 *)0x10000000; p < (uint16 *)0x18000000; w++, p++) {
      v = w;
      for (i = 0; i < 2; i++) {
        *p = v;
        r = *p;
        if (r != v)
          fails++;
        v = ~w;
      }
    }
    printf("%u writes\t%u fails\n", writes++, fails);
  }
}
#endif

void
main_sh (void)
{
  led(0x40);

  lcd_init();
  lcd_clear();

#if CONFIG_GPS_ROLLOVER_TEST == 1
#define PPSTIME_BASE 0xabcd0030
#define PPSTIME_STATUS_CTRL_REG ((volatile unsigned int *) (PPSTIME_BASE + 0))
#define PPSTIME_SEC_LO_REG      ((volatile unsigned int *) (PPSTIME_BASE + 4))
#define PPSTIME_NSEC_REG        ((volatile unsigned int *) (PPSTIME_BASE + 8))
  while (1) {
    char s[100];
    if (*PPSTIME_STATUS_CTRL_REG) {
      sprintf(s, "%u s %u ns\n", *PPSTIME_SEC_LO_REG, *PPSTIME_NSEC_REG);
      putstr(s);
    }
  }
#endif

  /* Ensure DDR is initialized before the gdb stub startup to allow
     programs to be loaded into DDR by GDB. */
#if CONFIG_DDR == 1
  ddr_init ();
  led(0x041);
#endif

#if CONFIG_GPIO_INIT == 1
  LEDPORT = CONFIG_GPIO_INIT_VALUE;
#endif

#if CONFIG_GDB_STUB == 1
#if CONFIG_LCD_LOGO == 1
  lcd_logo_draw();
#else
  lcd_print(0, "GDB stub");
  lcd_print(1, "Probe...");
#endif
  if (gdb_startup() == 1) {
    /* return to hit trapa in entry.c */
#if CONFIG_LCD == 1
    lcd_print(0, "GDB stub");
    lcd_print(1, "Connect ");
#endif
    return;
  }
#endif

  /* TODO: In what configuration do we set the baudrate? */
  uart_set_baudrate (0);
  uart_set_baudrate (1);
  led(0x042);

  putstr ("CPU tests passed\n");
  lcd_clear();
  lcd_print(0, "SEI SoC ");
  lcd_print(1, "CPU 2J0a");
  led(0x043);

#if CONFIG_TEST_MEM == 1
  int tests, dataFailures, addrFailures, deviceFailures;
  char s[100];

  lcd_print(1, "Mem test 1");
  for (tests = dataFailures = addrFailures = deviceFailures = 0; ; tests++) {
    if (memTestDataBus((datum *)MEM_TEST_ADDR))
      dataFailures++;
    if (memTestAddressBus((datum *)MEM_TEST_ADDR, MEM_TEST_SIZE))
      addrFailures++;
    if (memTestDevice((datum *)MEM_TEST_ADDR, MEM_TEST_SIZE))
      deviceFailures++;
    sprintf(s, "%d tests run\tdataFail = %d\taddrFail = %d\tdeviceFail = %d\n", tests, dataFailures, addrFailures, deviceFailures);
    putstr(s);
  }
  return;
#elif CONFIG_TEST_MEM == 2
  lcd_print(1, "Mem test 2");
  memtest2();
  return;
#endif

#if CONFIG_GDB_STUB == 1
  putstr ("GDB Stub for ");
#endif
  putstr("HS-2J0 SH2 ROM\n");
  putstr (version_string);
  led(0x50);

#if CONFIG_LOAD_ELF == 1
  putstr("Booting\n");
  if (loadelf_load("vmlinux") == -1) {
    putstr("\nRun elf FAILED.\n");
    lcd_clear();
    lcd_print(0, "Pgm load");
    lcd_print(1, "Failed");
  }
#endif
}
