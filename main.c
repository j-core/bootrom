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
#ifdef DEVICE_CACHE_CTRL_WSBU_ADDR
#define DEVICE_CACHE_CTRL ((volatile struct cache_ctrl_wsbu_regs *) DEVICE_CACHE_CTRL_WSBU_ADDR)
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
putstr (const char *str)
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

void * localmemcpy(void * dest, const void *src, unsigned long count) {
  char *d = (char *) dest;
  char *s = (char *) src;
  // TODO: copy a 4 bytes at a time instead of 1 byte?
  while (count--) {
    *d++ = *s++;
  }
  return dest;
}

#if CONFIG_LOAD_ELF == 1

/* Copies device tree to dest pointer. Device tree can come from an SD
   card file if present or be linked into the bootloader. */
static int prepare_dtb(const char *filename, void *dest) {
  int result = -1;

#if CONFIG_DEVTREE_READ == 1
  if (pf_open(filename) == FR_OK) {
    // 20kB is arbitrary limit to avoid trying to read huge file into memory
    if (load_open_dtb(dest, 20 * 1024) == 0) {
      result = 0; // success
      putstr("Using ");
      putstr(filename);
      putstr(" from SD card\n");
    } else {
      // file existed, but could not be read
      putstr("Failed to read ");
      putstr(filename);
      putstr("\n");
    }
  }
#endif

#ifdef CONFIG_DEVTREE_ASM_FILE
  if (result != 0) {
    extern char dt_blob_start;
    extern char dt_blob_end;
    // A device tree was linked into the bootloader starting at
    // dt_blob_start and ending at dt_blob_end. memcpy it into dram.
    // TODO: Should we tell the kernel to read it directly from the
    // boot ROM? Probably not if kernel is not aware of ROM and not
    // all cpus may be able to read the ROM.
    putstr("Use bootloader's DTB\n");
    localmemcpy(dest, &dt_blob_start, &dt_blob_end - &dt_blob_start);
    result = 0;
  }
#endif

  return result;
}

#ifdef CONFIG_BOOT_ID
static int simple_strcmp(const char *a, const char *b) {
  for (; *a && *a==*b; a++, b++);
  return (unsigned char)*a - (unsigned char)*b;
}
#endif

static int select_and_mount_fs(FATFS *fs) {
#ifdef CONFIG_BOOT_ID
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x
#define BOOT_ID STRINGIFY(CONFIG_BOOT_ID)
  char buf[sizeof BOOT_ID], buf2[3];
  WORD bytes_read;
  int i, r;
  buf[sizeof buf - 1] = 0;
  putstr("Searching for boot id: " BOOT_ID "\n");
  for (i=2; i<=3; i++) {
    r = pf_mount_part(fs, i);
    if (r == FR_OK && pf_open("id") == FR_OK
        && pf_read(buf, sizeof buf, &bytes_read) == FR_OK
        && bytes_read >= sizeof buf - 1         /* Reject id file that's too short. */
        && buf[sizeof buf - 1] < 0x20) {        /* Reject false prefix matches. */
      buf[sizeof buf - 1] = 0;
      if (!simple_strcmp(buf, BOOT_ID)) {
        putstr("Found on partition ");
        buf2[0] = '0'+i;
        buf2[1] = '\n';
        buf2[2] = 0;
        putstr(buf2);
        return FR_OK;
      }
    }
  }
  putstr("Not found; booting partition 1\n");
#endif
  return pf_mount(fs);
}

static int boot_linux(void) {
  FRESULT r;
  FATFS fs;
  struct elf_image elf_img;
  unsigned int dtb_addr;
  putstr("Booting\n");
  loadelf_initfs(&fs);
  r = select_and_mount_fs(&fs);
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
#define XSTR(s) STR(s)
#define STR(s) #s
    if (prepare_dtb(XSTR(CONFIG_DEVTREE_FILENAME), (void *) dtb_addr) != 0) {
      putstr("DTB not found\n");
      dtb_addr = 0;
    }
#if CONFIG_LOAD_ELF_TIME == 1
    rtc_get_time(&sec1, &nsec1);
    putstr("Loaded dtb in ");
    putinterval(sec1 - sec0, nsec1 - nsec0);
    putstr("\n");
#endif
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
