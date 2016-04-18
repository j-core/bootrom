###################################################################
# Config these to change what's included in the bootloader. You can
# override these variables on the command line when calling make.
# Ex:
# make CONFIG_BOOT0=1 CONFIG_CPU_TESTS=1 DDR_TYPE=lpddr
#
###################################################################

# If BOOT0=1 then binary will be contain the CPU vector table
# and will be linked to go in SRAM
CONFIG_BOOT0=1
# If CPU_TESTS=1 then the first thing run will be a a series of tests
# of the CPU instructions. If any test fails the CPU will enter an
# infinite loop. If CONFIG_BOOT0=0 then CPU_TEST=1 has no effect.
CONFIG_CPU_TESTS=1
CONFIG_GDB_STUB=1
CONFIG_LOAD_ELF=1
CONFIG_LOAD_ELF_TIME=0
CONFIG_OLDLCD=0
CONFIG_GRLCD=1
CONFIG_LCD_LOGO=1
CONFIG_KEYS=0
CONFIG_SDCARD=1
CONFIG_SPIFLASH=0

# If CONFIG_GPIO_INIT is 1, then CONFIG_GPIO_INIT_VALUE is
# assigned to the GPIO register early in the boot
CONFIG_GPIO_INIT=0
CONFIG_GPIO_INIT_VALUE=0

# If CONFIG_TEST_MEM=1 then the program 
CONFIG_TEST_MEM=0

# Determines if the UART function will use uartlite or uart16550
# registers. Set to 0 for uart16550, or to 1 for uartlite.
CONFIG_UARTLITE=1

# If CONFIG_DDR=1 the DDR will be initialized. If CONFIG_BOOT0=0 then
# this option is ignored.
CONFIG_DDR=1

# CONFIG_GDB_IO = "UART0" or "UART1"
CONFIG_GDB_IO=UART0
# CONFIG_LIBC_IO = "UART0", "UART1", or "GDB"
CONFIG_LIBC_IO=UART0

CONFIG_DEVTREE=1

CONFIG_VERSION_DATE=1

# Always safe to enable dcache when DMA is not used. Enabling icache
# requires kernel is aware of caches so it will invalidate icache
# after loading code
CONFIG_ENABLE_DCACHE=1
CONFIG_ENABLE_ICACHE=0

# Type of DDR. One of "ddr8", "ddr16", "lpddr", "lpddr2". Only used when
# BOOT0=1 to initialize the DDR.
DDR_TYPE=ddr16

# Name of the binary to build
BIN_NAME=boot.elf

BIN_NAME:=$(addprefix bin/,$(BIN_NAME))

###################################################################
# End of configuration
###################################################################

# Validate configuration

# These undefine directive isn't available until make 3.82. Instead to
# ensure they aren't passed to C they are filtered out before being
# added to CFLAGS.
#override undefine CONFIG_LIBC_GDB_UART0
#override undefine CONFIG_LIBC_GDB_UART1
#override undefine CONFIG_LIBC_IO_UART0
#override undefine CONFIG_LIBC_IO_UART1
#override undefine CONFIG_LIBC_IO_GDB

ifeq ($(CONFIG_LIBC_IO),UART0)
else ifeq ($(CONFIG_LIBC_IO),UART1)
else ifeq ($(CONFIG_LIBC_IO),GDB)
else
$(error Unrecognized CONFIG_LIBC_IO option: "$(CONFIG_LIBC_IO)")
endif

ifeq ($(CONFIG_GDB_IO),UART0)
else ifeq ($(CONFIG_GDB_IO),UART1)
else
$(error Unrecognized CONFIG_GDB_IO option: "$(CONFIG_GDB_IO)")
endif

ifeq ($(CONFIG_OLDLCD),1)
	ifeq ($(CONFIG_GRLCD),1)
$(error Both OLDLCD and GRLCD cannot be enabled at the same time)
	endif
endif

ifeq ($(CONFIG_LCD_LOGO),1)
	ifeq ($(CONFIG_GRLCD),0)
$(error LCD_LOGO is only supported when GRLCD is enabled)
	endif
endif

CROSS_COMPILE ?= sh2-elf-

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
LDFLAGS = -T linker/sh32.x
LIBGCC := $(shell $(CC) -print-libgcc-file-name)
LIBGCC += $(shell $(CC) -print-file-name=libgcc-Os-4-200.a)

CFLAGS := -m2 -g -Os -Wall -Werror
CFLAGS += -std=gnu99
# Pass CONFIG_ variables to C
CONF_VARS:=$(filter CONFIG_%,$(.VARIABLES))

# filter out LIBC and GDB IO variables. They are added below
CONF_VARS:=$(filter-out CONFIG_GDB_IO_%,$(CONF_VARS))
CONF_VARS:=$(filter-out CONFIG_LIBC_IO_%,$(CONF_VARS))
CONF_VARS:=$(sort $(CONF_VARS))

# Create a variable containing -D$(VAR_NAME)=$(VAR_VALUE) for each
# config variable
CFLAGS_CONFIG += $(foreach v,$(CONF_VARS),-D$(v)=$($v))

# add GDB and LIBC IO variables that are more useful to use in C
CFLAGS_CONFIG += -DCONFIG_GDB_IO_$(CONFIG_GDB_IO)=1
CFLAGS_CONFIG += -DCONFIG_LIBC_IO_$(CONFIG_LIBC_IO)=1

# if either LCD is enabled, set combined LCD config variable
ifeq ("$(CONFIG_OLDLCD)$(CONFIG_GRLCD)","00")
CFLAGS_CONFIG += -DCONFIG_LCD=0
else
CFLAGS_CONFIG += -DCONFIG_LCD=1
endif

CFLAGS += $(CFLAGS_CONFIG)
CFLAGS += -Iinclude
CFLAGS += $(EXTRA_CFLAGS)

ifeq ($(DDR_TYPE),lpddr)
	CFLAGS += -DLPDDR
else ifeq ($(DDR_TYPE),lpddr2)
	CFLAGS += -DLPDDR -DLPDDR2
else ifeq ($(DDR_TYPE),ddr8)
	CFLAGS += -DDDR_BL4
else ifeq ($(DDR_TYPE),ddr16)
# add nothing
else
$(error Unknown DDR type $(DDR_TYPE))
endif

GDB_OBJS := gdb.o
GDB_OBJS += sh2.o
GDB_OBJS := $(addprefix gdb/,$(GDB_OBJS))

FILES_OBJS := spi.o
FILES_OBJS += spi_mmc.o
FILES_OBJS += pff.o
FILES_OBJS += pf_read_long.o
FILES_OBJS += loadelf.o
FILES_OBJS := $(addprefix files/,$(FILES_OBJS))

TESTS_OBJS := testbra.o
TESTS_OBJS += testmov.o testmov2.o
TESTS_OBJS += testalu.o
TESTS_OBJS += testshift.o
TESTS_OBJS += testmul.o testmulu.o testmuls.o testmull.o testdmulu.o testdmuls.o testmulconf.o
TESTS_OBJS += testdiv.o
TESTS_OBJS += testmacw.o testmacl.o
TESTS_OBJS := $(addprefix tests/,$(TESTS_OBJS))

LIBC_OBJS := vsprintf.o
LIBC_OBJS += setjmp.o
LIBC_OBJS += conio.o
LIBC_OBJS += libsyscall.o
LIBC_OBJS += read_write.o
LIBC_OBJS += strtoul.o
LIBC_OBJS += strtol.o
LIBC_OBJS += strcpy.o
LIBC_OBJS += strncpy.o
LIBC_OBJS += strcat.o
LIBC_OBJS += strncat.o
LIBC_OBJS += strcmp.o
LIBC_OBJS += strncmp.o
LIBC_OBJS += strchr.o
LIBC_OBJS += strlen.o
LIBC_OBJS += strnlen.o
LIBC_OBJS += strspn.o
LIBC_OBJS += strpbrk.o
LIBC_OBJS += strtok.o
LIBC_OBJS += memset.o
LIBC_OBJS += bcopy.o
LIBC_OBJS += memcpy.o
LIBC_OBJS += memmove.o
LIBC_OBJS += memcmp.o
LIBC_OBJS += memscan.o
LIBC_OBJS += strstr.o
LIBC_OBJS += ctype.o
LIBC_OBJS += bzero.o
LIBC_OBJS := $(addprefix libc/,$(LIBC_OBJS))

# determine dependencies of configuration
NEED_LIBC := 0
NEED_UART := 0

# Do we need UART support?
ifeq ($(CONFIG_BOOT0),1)
	NEED_UART := 1
endif
ifeq ($(CONFIG_GDB_STUB),1)
# GDB stub always needs UARTs
	NEED_UART := 1
endif
ifneq ($(CONFIG_TEST_MEM),0)
	NEED_LIBC := 1
endif
ifeq ($(NEED_LIBC),1)
	ifneq ($(CONFIG_LIBC_IO),GDB)
# libc needs UART unless using GDB stub for IO
		NEED_UART := 1
	endif
endif

DEV_OBJS := rtc.o
ifeq ($(NEED_UART),1)
	ifeq ($(CONFIG_UARTLITE),1)
		DEV_OBJS += uartlite.o
	else
		DEV_OBJS += uart16550.o
	endif
endif
ifeq ($(CONFIG_DDR),1)
	DEV_OBJS += ddr.o
endif
ifeq ($(CONFIG_OLDLCD),1)
	DEV_OBJS += lcd.o
endif
ifeq ($(CONFIG_GRLCD),1)
	DEV_OBJS += aqm1248ok.o
	DEV_OBJS += lcd_font.o
endif
ifeq ($(CONFIG_LCD_LOGO),1)
	DEV_OBJS += lcd_logo.o
endif
DEV_OBJS := $(addprefix dev/,$(DEV_OBJS))

OBJS := main.o
OBJS += version.o
OBJS += entry.o
ifeq ($(CONFIG_GDB_STUB),1)
	OBJS += $(GDB_OBJS)
	CFLAGS += -Igdb
endif
OBJS += $(DEV_OBJS)
CFLAGS += -Idev

ifeq ($(CONFIG_LOAD_ELF),1)
	OBJS += $(FILES_OBJS)
	CFLAGS += -Ifiles
endif

ifeq ($(CONFIG_TEST_MEM),1)
	OBJS += memtest.o
endif

LIBS :=

ifeq ($(CONFIG_CPU_TESTS),1)
	LIBS += lib/libtests.a
endif

ifeq ($(NEED_LIBC),1)
	LIBS += lib/libc.a
endif

all: $(BIN_NAME)

$(OBJS) $(TESTS_OBJS) $(LIBC_OBJS): config.log

$(BIN_NAME): $(OBJS) $(LIBS) | bin
	$(LD) $(LDFLAGS) -Map $@.map $^ $(LIBGCC) -o $@

lib/libtests.a: $(TESTS_OBJS)

lib/libc.a: $(LIBC_OBJS)
	$(AR) -cur $@ $(LIBC_OBJS)
	$(RANLIB) $@

bin/lcdtest: dev/aqm1248ok.o libc/libsyscall.o lcdtest.o | bin
	$(LD) -e _main -T shelf.x -Map bin/lcdtest.map $^ $(LIBGCC) -o $@

# This REVISION value is usually passed in from the soc_hw Makefile so
# that the tag and commit ID are from soc_hw. Set a default value here
# that uses the boot repo's values.
REVISION="open"

VERSION_STRING:=revision: $(REVISION)\\\\n
ifeq ($(CONFIG_VERSION_DATE),1)
VERSION_STRING:=$(VERSION_STRING)build: $(shell date)\\\\n
endif

version.c:
	@printf "char version_string[] = \"$(VERSION_STRING)\";\n" > $@

#version.c:
#	@printf "char version_string[] = \"revision: $(REVISION)\\\\nbuild: $(shell date)\\\\n\";\n" > $@

# store the configuration in a file that is updated only when the
# contents change to force a recompliation
config.log: force
	@echo '$(CFLAGS) CROSS_COMPILE=$(CROSS_COMPILE)' | cmp -s - $@ || echo '$(CFLAGS) CROSS_COMPILE=$(CROSS_COMPILE)' > $@

# create empty directories that are deleted by clean
bin:
	mkdir bin

lib:
	mkdir lib

.SUFFIXES:            # Delete the default suffixes

lib/%.a: | lib
	$(AR) -cur $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o */*.o
	rm -f version.c
	rm -rf bin lib
	rm -f config.log

memtest:
	make CONFIG_TEST_MEM=1 CONFIG_LOAD_ELF=0 CONFIG_GDB_STUB=0 CONFIG_LIBC_IO=UART0

.PHONY: all clean version.c memtest force
