#include "uart.h"
#include "board.h"
#include "rtc.h"

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
putuint(unsigned int x)
{
  char buf[11];
  int i = 10;
  if (x == 0) {
    uart_tx(0, '0');
  } else {
    buf[10] = 0;
    while (x) {
      buf[--i] = '0' + (x % 10);
      x = x / 10;
    }
    putstr(buf + i);
  }
}

void
putinterval(unsigned int sec_delta, unsigned int nsec_delta)
{
  // nsec_delta = nsec1 - nsec0
  // so if the nsec wrappped between two measurements, then the
  // subtraction will underflow. Adjust sec and nsec to normalize the
  // number of nsecs.
  if (nsec_delta > 1000000000) {
    nsec_delta += 1000000000;
    sec_delta -= 1;
  }
  putuint(sec_delta);
  putstr("s ");
  putuint(nsec_delta);
  putstr("ns");
}

void
led(int v)
{
//  DEVICE_GPIO->value = v;
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

#if CONFIG_LOAD_ELF == 1

static int boot_linux(void) {
  FRESULT r;
  FATFS fs;
  struct elf_image elf_img;
  unsigned int dtb_addr;
  putstr("Booting\n");
  loadelf_initfs(&fs);
  r = pf_mount(&fs);
  if (r != FR_OK) {
    goto err;
  }
  putstr("Storage device initialized\n");
  lcd_print(1, "Dev init");

  {
#if CONFIG_LOAD_ELF_TIME == 1
    unsigned int sec0;
    unsigned int nsec0;
    unsigned int sec1;
    unsigned int nsec1;
    rtc_get_time(&sec0, &nsec0);
#endif
    // load vmlinux ELF binary
    if (loadelf_load("vmlinux", &elf_img) == -1) {
      putstr("Probe storage device failed\n");
      goto err_mnt;
    }
#if CONFIG_LOAD_ELF_TIME == 1
    rtc_get_time(&sec1, &nsec1);
    putstr("Loaded vmlinux in ");
    putinterval(sec1 - sec0, nsec1 - nsec0);
    putstr("\n");
#endif
  }

  dtb_addr = (unsigned int) elf_img.max_addr;
  // round up to align
  dtb_addr = (dtb_addr + 3) & ~3;

  {
#if CONFIG_LOAD_ELF_TIME == 1
    unsigned int sec0;
    unsigned int nsec0;
    unsigned int sec1;
    unsigned int nsec1;
    rtc_get_time(&sec0, &nsec0);
#endif
    // load device tree DTB file
    if (load_dtb("dt.dtb", (void *) dtb_addr, 20 * 1024) == 0) {
#if CONFIG_LOAD_ELF_TIME == 1
      rtc_get_time(&sec1, &nsec1);
      putstr("Loaded dt.dtb in ");
      putinterval(sec1 - sec0, nsec1 - nsec0);
      putstr("\n");
#endif
    } else {
      dtb_addr = 0;
    }
  }

  pf_mount(0); // unmount

  putstr("RUN program\n");
#if CONFIG_LCD_LOGO == 1
  lcd_logo_draw();
#else
  lcd_print(0, "Loaded  ");
  lcd_print(1, "Booting ");
#endif

  elf_img.entry((void *) dtb_addr);

  // should not reach here
  putstr("Loaded program has ended!\n");
  lcd_clear();
  lcd_print(0, "Pgm exit");

  return 0;
 err_mnt:
  pf_mount(0);
 err:
  putstr("\nRun elf FAILED.\n");
  lcd_clear();
  lcd_print(0, "Pgm load");
  lcd_print(1, "Failed");
  return -1;
}
#endif

void
main_sh (void)
{
  led(0x40);

  lcd_init();
  lcd_clear();


  /* Ensure DDR is initialized before the gdb stub startup to allow
     programs to be loaded into DDR by GDB. */
#if CONFIG_DDR == 1
  ddr_init ();
  led(0x041);
#endif

#ifdef DEVICE_CACHE_CTRL
  // enable cpu0's caches
  DEVICE_CACHE_CTRL->ctrl0 = (CONFIG_ENABLE_DCACHE << 1) | CONFIG_ENABLE_ICACHE;
#endif

#if CONFIG_GPIO_INIT == 1
  DEVICE_GPIO->value = CONFIG_GPIO_INIT_VALUE;
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
  boot_linux();
#endif
}
