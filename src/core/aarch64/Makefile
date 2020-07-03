# ARMv8 64 bit
include ../../../etc/sys/etc/os-release
include ../../../Config

SRCS = $(filter-out dbg.c,$(wildcard *.c))
ifeq ($(DEBUG),1)
SRCS += dbg.c
endif
SRCS+= $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
ECHO ?= echo
CFLAGS += -DDEBUG=$(DEBUG) -DOPTIMIZE=$(OPTIMIZE) -D_OSZ_CORE_=1 -DOSZ_NAME='$(NAME)' -DOSZ_VER='$(VERSION)' \
 -DOSZ_ARCH='"$(ARCH)"' -D__$(ARCH)__ -DOSZ_PLATFORM='"$(PLATFORM)"' -D__$(PLATFORM)__ -DOSZ_BUILD='$(BUILD)' \
 -ansi -fpic -fno-builtin -ffreestanding -mcmodel=small -nostdlib \
 -nostdinc -I. -I./$(PLATFORM) -I../../../include
ifneq (,$(findstring -gcc,$(CC)))
CFLAGS += -fstack-limit-symbol=STACK_LIMIT
endif

all: $(OBJS)

%.o: %.S ../../../Config
	@$(ECHO) "  src		$(ARCH)/$<"
	$(O)$(CC) -D_AS=1 $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c ../../../Config
	@$(ECHO) "  src		$(ARCH)/$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
