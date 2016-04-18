#include "loadelf.h"
#include "pff.h"
#include "diskio.h"
#include "board.h"
#include "lcd.h"

/* elf.h contains a #pragma pack(1) so include it last to avoid
   affecting other headers */
#include "elf.h"

extern void putstr(char *str);

#ifndef NULL
  #define NULL (void*)0
#endif

static char* _strcat(char* dst, char* src)
{
  if(src == NULL || src[0] == '\0' || dst == NULL) return dst;

  char* p = dst;
  char* q = src;

  while(p[0] != '\0') p++;
  while(q[0] != '\0') *p++ = *q++;
  *p++ = '\0';

  return p;
}

/* report progress of load based on number of sections loaded */
static void report_load_progress(unsigned long progress, unsigned long total) {
#if CONFIG_GRLCD == 1
  static unsigned long prev_percent = 101;
  unsigned long percent = progress * 100 / total;
  char str[4];
  if (percent >= 100) percent = 99;
  /* only update when changed */
  if (percent != prev_percent) {
    prev_percent = percent;
    str[0] = '0' + (percent / 10);
    str[1] = '0' + (percent % 10);
    str[2] = '%';
    str[3] = 0;
    /* 4 + 8 * 8 places the column just after "Loading " string
       previous printed to lcd */
    lcd_print_at(4 + 8 * 8, 3, str);
  }
#elif CONFIG_SEVENSEGMENT == 1
  const static unsigned long numbers[] = {
    // bits 15-9 control the seven segments of a digit display.
    // bit 8 is the period after the digit. A 0 bit is on, a 1 bit is on.
    0x0300,
    0x9F00,
    0x2500,
    0x0D00,
    0x9900,
    0x4900,
    0x4100,
    0x1F00,
    0x0100,
    0x1900,

    0x0000
  };
#define GPIO ((volatile unsigned long *)0xabcd0000)
  unsigned long percent = progress * 100 / total;

  if (percent >= 100) {
    *GPIO = 0x0FD00; // straight line through all 3
  } else {
    if (1) {
      // draw 10s digit
      // bits 18-16 select which of the 3 digits to enable, active-low.
      *GPIO = 0x50000 | numbers[percent / 10];
    } else {
      // draw 1s digit
      *GPIO = 0x60000 | numbers[percent % 10];
    }
  }
#endif
}

void loadelf_initfs(FATFS *fs) {
#if CONFIG_SPIFLASH == 1
  fs->ops.disk_initialize = &spif_initialize;
  fs->ops.disk_readp = &spif_readp;
  fs->ops.disk_writep = 0;
#elif CONFIG_SDCARD == 1
  fs->ops.disk_initialize = &sdc_initialize;
  fs->ops.disk_readp = &sdc_readp;
  fs->ops.disk_writep = 0;
#else
#error "No storage device defined"
#endif
}

/* Read data in chunks from the storage and report progress */
static unsigned long
read_progress(void *dst, 		/* Pointer to the read buffer */
              unsigned long blength, 	/* Number of bytes to
                                           read */
              /* progress and total are used to track and report
                 progress */
              unsigned long *progress,
              unsigned long total)
{
  unsigned short len;
  unsigned short brs;
  FRESULT res = FR_OK;

  while (blength) {
    if (blength > 0xFFFF)
      len = 0xFFFF;
    else
      len = blength;
    res = pf_read(dst, len, &brs);
    if (res != FR_OK) {
      break;
    }
    *progress += len;
    dst += len;
    blength -= len;
    report_load_progress(*progress, total);
  }
  return res;
}

int loadelf_load(char *ch, struct elf_image *img)
{
  unsigned long i, j;
  struct elf_hdr *ElfHdr = 0;	/*elf header struct  */
  unsigned long p_e_entry;	/* elf progamme start address */
  unsigned long p_e_phoff;	/*   Start of program headers   */
  unsigned short p_e_phentsize;	/*  Size of program headers */
  unsigned short p_e_phnum;	/*   number of program headers */

  struct elf_phdr *ElfPhdr = 0;	/* ELF program Header */
  struct elf_phdr *phdr = 0;	/* ELF program Header */
  unsigned char *lptmp;		/* load temp address */

  WORD brs;
  unsigned long br;
  FRESULT res;

  /* Need to load portions of the ELF file to read metadata. Where
     should it be loaded? Choose high area of DDR.
     TODO: Read metadata into SRAM (stack?) instead?
  */
  unsigned char *readbuf = (unsigned char *)0x13000000;

  img->entry = 0;
  img->min_addr = (void *) 0xFFFFFFF0;
  img->max_addr = (void *) 0;

  /* ELF loader */
  /* mount the volume  */

  lcd_clear();
  lcd_print(0, "Pgm load");
  lcd_print(1, ch);

  res = pf_open(ch);
  if (res != FR_OK) {
    putstr("ELF not found\n");
    lcd_print(1, "not fnd ");
    return -1;
  }
  char str[32]; str[0] = '\0';
  _strcat(str, "Open "); _strcat(str, ch); _strcat(str, " OK");
  _strcat(str, "\n");
  putstr(str);

  res = pf_lseek(0);
  if (res != FR_OK)
    return -1;
/* read ELF header length */
  pf_lseek(40);			//ELF header length stored at 40
  res = pf_read(readbuf, 2, &brs);
/*read elf magic number */
  pf_lseek(0);
  res = pf_read(readbuf, ((readbuf[0]) << 8) + readbuf[1], &brs);
  if (res != FR_OK)
    return -1;
  ElfHdr = (struct elf_hdr *)readbuf;
  if ((ElfHdr->e_ident[0] != 0x7f) || (ElfHdr->e_ident[1] != 'E')
      || (ElfHdr->e_ident[2] != 'L') || (ElfHdr->e_ident[3] != 'F'))
    return -1;
  if (ElfHdr->e_type != ET_EXEC)
    return -1;
/*    ELF executable programe running start address     */
  p_e_entry = ElfHdr->e_entry;
  p_e_phoff = ElfHdr->e_phoff;	/*Start of program headers */
  p_e_phentsize = ElfHdr->e_phentsize;	/*size of program headers */
  p_e_phnum = ElfHdr->e_phnum;	/*number of program headers */

  /* relocate readbuf to avoid conflict */
  /*
    Don't need to play these games as readbuf is now high in DDR
    memory.
    if(p_e_entry <= p_e_phentsize + (unsigned)readbuf) {
	  if((p_e_entry - p_e_phentsize) > 0x100000200)
		  readbuf = (unsigned char*)(p_e_entry - p_e_phentsize - 256);
	  else
		  readbuf = (unsigned char*)0x12a00000;
    }*/

  lcd_print(1, "Loading");

  /* load all program headers */
  pf_lseek(p_e_phoff);
  res = pf_read_long(readbuf, p_e_phentsize * p_e_phnum, &br);
  if (res != FR_OK)
    return -1;

  /* calculate total in memory size for progress indicator */
  unsigned long mem_total = 0;
  unsigned long mem_progress = 0;
  ElfPhdr = (struct elf_phdr *)readbuf;
  for (i = 0; i < p_e_phnum; i++) {
    if (ElfPhdr[i].p_type == PT_LOAD) {
      mem_total += ElfPhdr[i].p_memsz;
    }
  }

  /* copy segment section  */
  for (i = 0; i < p_e_phnum; i++) {	/*  number of programme header  */
    phdr = ElfPhdr + i;
    if (phdr->p_type == PT_LOAD) {	/* load  to running address  */
      /* loading  */
      lptmp = (unsigned char *)phdr->p_paddr;	/* assigned physAddr or virtAddr ? */
      /* update min and max addresses */
      if (img->min_addr > (void*) lptmp) {
        img->min_addr = lptmp;
      }
      if (img->max_addr < (void*)(lptmp + phdr->p_memsz)) {
        img->max_addr = lptmp + phdr->p_memsz;
      }
      if (phdr->p_filesz > 0) {
        pf_lseek(phdr->p_offset);

        res = read_progress(lptmp, phdr->p_filesz, &mem_progress, mem_total);
        if (res != FR_OK)
          return -1;
      }

      /* clean any memory that wasn't loaded */
      if (phdr->p_filesz < phdr->p_memsz) {
        lptmp += phdr->p_filesz;
        for (j = phdr->p_filesz; j < phdr->p_memsz; j++) {	/* clean memory to 0 */
          /* TODO: align and write zero ints instead of zero chars */
          *lptmp++ = 0;
        }
        mem_progress += phdr->p_memsz - phdr->p_filesz;
        report_load_progress(mem_progress, mem_total);
      }

#ifdef DEBUG
      printf("loading Segment section No: %lu PhysAdd @ 0x%lX,FileSize=%lu,MemSize=%lu\n", i, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz);
#endif
    }
  }

  img->entry = (void *) p_e_entry;
  return 0;
}				/* ELF loader  */

int load_dtb(char *name, void *dest, int max_bytes) {
#if CONFIG_DEVTREE == 1
  FRESULT res = FR_OK;
  unsigned int len;
  unsigned long bytes_read;
  res = pf_open(name);
  if (res != FR_OK) {
    putstr("DTB not found\n");
    goto err;
  }
  char str[32];
  str[0] = '\0';
  _strcat(str, "Open "); _strcat(str, name); _strcat(str, " OK");
  _strcat(str, "\n");
  putstr(str);

  len = pf_size();
  if (len > max_bytes || len == 0) {
    putstr("Invalid DTB length\n");
    goto err;
  }

  res = pf_lseek(0);
  if (res != FR_OK) {
    putstr("DTB lseek(0) failed");
    goto err;
  }
  res = pf_read_long(dest, len, &bytes_read);
  if (res != FR_OK) {
    putstr("DTB read failed");
    goto err;
  }

  putstr("Loaded DTB\n");
  return 0;
 err:
  putstr("Failed to load DTB\n");
  return -1;
#else
  return -1;
#endif
}
