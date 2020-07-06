include ../../etc/etc/os-release
include ../../Config
TARGET = sys/core

BINTARGET = ../../bin/initrd/$(TARGET)
SRCS = $(filter-out dbg.c,$(wildcard *.c))
ifeq ($(DEBUG),1)
SRCS += dbg.c
endif
OBJS = $(SRCS:.c=.o)
EXTRAOBJS = font.o logo.o bztalloc.o
LIBS =
ECHO ?= echo
# max-page-size az LLVM lld miatt kell, hogy jól számolta a program headers-t, GNU ld-nél egyébként is jó
LDFLAGS = -nostdlib --build-id=none -x -z max-page-size=16
ifeq ($(LD),$(ARCH)-elf-ld)
LDFLAGS += -nostartfiles -nolibc -nodefaultlibs
endif
CFLAGS += -DDEBUG=$(DEBUG) -DOPTIMIZE=$(OPTIMIZE) -D_OSZ_CORE_=1 -DOSZ_NAME='$(NAME)' -DOSZ_VER='$(VERSION)' \
 -DOSZ_ARCH='"$(ARCH)"' -D__$(ARCH)__ -DOSZ_PLATFORM='"$(PLATFORM)"' -D__$(PLATFORM)__ -DOSZ_BUILD='$(BUILD)' \
 -ansi -fpic -fpie -fno-builtin -ffreestanding -mcmodel=small -nostdlib -fno-delete-null-pointer-checks \
 -nostdinc -I. -I./$(ARCH) -I./$(ARCH)/$(PLATFORM) -I../../include
ifeq ($(ARCH),x86_64)
CFLAGS += -mno-red-zone
endif
ifneq ($(DEBUG),1)
LDFLAGS += -s
endif

all: $(OBJS) $(BINTARGET)

main.o: main.c ../../etc/etc/os-release ../../Config
	@$(ECHO) "  src		$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c ../../Config *.h
	@$(ECHO) "  src		$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

bztalloc.o: ../libc/bztalloc.c ../libc/bztalloc.h ../../Config
	@$(ECHO) "  src		$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -I ../core -c ../libc/bztalloc.c -o bztalloc.o

font.o: ../../etc/system.8x16.sfn
	@$(ECHO) "  bin		$@"
	@cp ../../etc/system.8x16.sfn ./font
	$(O)$(LD) -r -b binary -o font.o font
	@rm font

logo.o: ../../etc/logo.tga
	@$(ECHO) "  bin		$@"
	@cp ../../etc/logo.tga ./logo
	$(O)$(LD) -r -b binary -o logo.o logo
	@rm logo

platform:
	$(O)make -e --no-print-directory -C $(ARCH)/$(PLATFORM) all | grep -v 'Nothing to be done' || true
	$(O)make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true

$(BINTARGET): platform $(EXTRAOBJS) $(SRCS)
	@echo "  lnk		$(TARGET) ($(ARCH)-$(PLATFORM))"
	@mkdir -p ../../bin/initrd/sys 2>/dev/null || true
	@touch ../../bin/initrd/sys/drivers
	$(O)$(LD) $(LDFLAGS) -T link.ld $(ARCH)/start.o $(filter-out ./$(ARCH)/start.o,$(shell find . -name \*.o)) -o $(BINTARGET) $(LIBS)
ifeq ($(DEBUG),1)
	@echo "0000000000400000 _start" >../../bin/core.sym
	@../../tools/elftool.o -s 0 $(BINTARGET) | sort | grep -ve '^0' >>../../bin/core.sym
else
	@test `du -b $(BINTARGET)|cut -f 1` -lt 262144 || echo "!!! WARNING: $(TARGET) TOO BIG !!!"
endif
	@../../tools/elftool.o -c $(BINTARGET)

clean:
	@rm *.o $(ARCH)/*.o $(ARCH)/$(PLATFORM)/*.o $(BINTARGET) ../../bin/*.sym 2>/dev/null || true
