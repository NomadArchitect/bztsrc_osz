include ../../../Config
SRCS = $(wildcard *.c)
SRCS+= $(filter-out crt0.S,$(wildcard *.S))
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
LIBS =
ECHO ?= echo

CFLAGS += -D__$(ARCH)__ -ansi -mcmodel=small -fPIC -nostdinc -I../../../include
ifneq (,$(findstring -gcc,$(CC)))
CFLAGS += -Wno-nonnull-compare
endif

all: $(OBJS)

%.o: %.S ../../../Config *.h ../*.h
	@$(ECHO) "  src		$<"
	$(O)$(CC) -D_AS=1 $(OPT) $(CFLAGS) -I../../core/$(ARCH) -c $< -o $@

%.o: %.c ../../../Config *.h ../*.h
	@$(ECHO) "  src		$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
