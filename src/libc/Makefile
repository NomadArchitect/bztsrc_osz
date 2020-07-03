include ../../Config
TARGET = sys/lib/libc.so

BINTARGET = ../../bin/initrd/$(TARGET)
SRCS = $(filter-out dbg.c,$(wildcard *.c))
OBJS = $(SRCS:.c=.o)
LIBS =
ECHO ?= echo

CFLAGS += -D__$(ARCH)__ -ansi -mcmodel=small -fPIC -nostdinc  -I../../include -I. -I../core/$(ARCH)
ifneq (,$(findstring -gcc,$(CC)))
CFLAGS += -Wno-nonnull-compare
endif
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG=1
LDFGS += -E
endif

all: $(OBJS) $(BINTARGET)

%.o: %.c ../../Config *.h
	@$(ECHO) "  src		$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

$(BINTARGET): $(SRCS)
	@cp -f $(ARCH)/platform.h ../../include/osZ/platform.h 2>/dev/null 1>/dev/null || true
	@make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true
	@$(ECHO) "  lnk		$(ARCH)-crt0.o"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $(ARCH)/crt0.S -o ../../bin/initrd/sys/lib/crt0.o || true
	@echo "  lnk		$(TARGET) ($(ARCH))"
	$(O)$(LD) -nostdlib -shared -Bsymbolic -Bshareable -x -L../../bin/initrd/sys/lib $(LDFLGS) -e init $(ARCH)/*.o *.o -T ../link.ld -o $(BINTARGET) $(LIBS)
	@../../tools/elftool.o -s 0 $(BINTARGET) | sort >../../bin/libc.sym
	@#../../tools/elftool.o -s 403000 $(BINTARGET) | sort >../../bin/libc.sym
	@#echo -n "OS/Z" | dd conv=notrunc of=$(BINTARGET) bs=1 seek=0 1>/dev/null 2>/dev/null
	@#osabi OS/Z
	@echo -ne '\005' | dd conv=notrunc of=$(BINTARGET) bs=1 seek=7 1>/dev/null 2>/dev/null

clean:
	@rm *.o *.so $(ARCH)/*.o $(BINTARGET) ../../bin/initrd/sys/lib/crt0.o 2>/dev/null || true
