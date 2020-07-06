# Intel 64 bit
include ../../../Config

SRCS = $(filter-out dbg.c,$(wildcard *.c))
ifeq ($(DEBUG),1)
SRCS += dbg.c
endif
SRCS+= $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
ECHO ?= echo
# a -Wno-pointer-arith azért kell, mert IDT generáláskor használunk mutató aritmetikát
CFLAGS += -DDEBUG=$(DEBUG) -DOPTIMIZE=$(OPTIMIZE) -D_OSZ_CORE_=1 \
 -DOSZ_ARCH='"$(ARCH)"' -D__$(ARCH)__ -DOSZ_PLATFORM='"$(PLATFORM)"' -D__$(PLATFORM)__ \
 -ansi -fpic -fno-builtin -ffreestanding -mcmodel=small -nostdlib -Wno-pointer-arith -fno-delete-null-pointer-checks \
 -nostdinc -I. -I./$(PLATFORM) -I../../../include
CFLAGS += -mno-red-zone

all: $(OBJS)

%.o: %.S ../../../Config
	@$(ECHO) "  src		$(ARCH)/$<"
	$(O)$(CC) -D_AS=1 $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c ../../../Config
	@$(ECHO) "  src		$(ARCH)/$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
