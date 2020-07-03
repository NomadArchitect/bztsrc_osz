include ../Config
DRVS = $(wildcard drivers/*/*)
LIBS = $(filter-out libc drivers Makefile README.md,$(wildcard lib*))
APPS = $(filter-out core libc libui drivers Makefile README.md,$(wildcard *))
export O

all: system $(LIBS) $(APPS)

drivers: $(DRVS)

apps: $(APPS)

libs: $(LIBS)

system:
	@make -e --no-print-directory -C core -Oline all | grep -v 'Nothing to be done' || true
	@make -e --no-print-directory -C libc -Oline all | grep -v 'Nothing to be done' || true

.PHONY: Makefile

%: Makefile
	@test -f $@/Makefile && (test ! -f $@/platforms || grep -q "$(ARCH)-$(PLATFORM)" $@/platforms || grep -q "$(ARCH)-\*" $@/platforms) && make -e -C $@ -Oline all | grep -v 'Nothing to be done' || true

clean:
	@rm ../bin/*.o */*.o */*/*.o */*/*/*.o */*/*/*/*.o 2>/dev/null || true
