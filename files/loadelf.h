#ifndef LOADELF_H
#define LOADELF_H

#include "pff.h"

typedef void (*entry_fn)(void *);

struct elf_image {
  entry_fn entry;
  void *min_addr;
  void *max_addr;
};

void loadelf_initfs(FATFS *fs);

int loadelf_load(char *ch, struct elf_image *img);

int load_dtb(char *name, void *dest, int max_bytes);

#endif
