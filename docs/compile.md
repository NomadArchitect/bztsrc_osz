OS/Z Fordítása
==============

Szükséges eszközök
------------------

- GNU eszköztár (make, gcc, binutils (gas, ld, objcopy, strip), lásd [tools/cross-gcc.sh](https://gitlab.com/bztsrc/osz/blob/master/tools/cross-gcc.sh))
- gcc és ld helyett LLVM Clang és LLVM lld is használható

Konfigurálás
------------

A `core` mindig egy adott [gépre](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.md) fordul, amit a
[Config](https://gitlab.com/bztsrc/osz/blob/master/Config) fájlban az `ARCH` és `PLATFORM` változókkal állíthatunk be.
Az érvényes kombinációk a következők:

| ARCH    | PLATFORM | Leírás |
| ------- | -------- | ----------- |
| x86_64  | ibmpc    | Régi gépekhez, PIC, PIT, RTC, PCI buszon detektál |
| x86_64  | acpi     | Új gépekhez, LAPIC, IOAPIC, HPET és ACPI táblákat értelmez |
| aarch64 | rpi      | Raspberry Pi 3+ |

Ha van telepítve "dialog" segédprogram a gépedre, akkor a következő parancs egy kényelmes ncurses alapú felülettel segíti a beállítást:

```shell
$ make config
```

<img height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszcfg1.png" alt="make config">


Fordítás
--------

A fordítási környezet úgy lett kialakítva, hogy majdnem minden könyvtárban kiadhatod a `make all` parancsot. Ha ezt a főkönyvtárban
teszed, akkor mindent lefordít és a lemezképeket is kigenerálja neked, de részletes üzenetek nélkül, csak a hibákat megjelenítve.
Alkönyvtárakban kiadva a make mindig megjeleníti a teljes parancsot amit a fordításkoz és linkeléshez használ.

Például ha belépsz az `src/fs` mappába és ott adod ki a `make all` parancsot, akkor csak a fájlrendszer szolgáltatás fordítódik le,
és látni fogod a kiadott parancsokat is.

Szóval ha mindent szeretnél fordítani, akkor csak add ki a főkönyvtárban a

```shell
$ make all
```
parancsot. Valami ilyesmit kell hogy láss:

```
$ make all
SEGÉDPROGRAMOK
  src		mkfs.c
  src		elftool.c
RENDSZERMAG
  lnk		sys/core (x86_64-ibmpc)
  lnk		sys/lib/libc.so (x86_64)
ALAPRENDSZER
  src		sys/ui
  src		sys/bin/sh
  src		sys/bin/test
  src		sys/bin/sys
  src		sys/syslog
  src		sys/sound
  src		sys/fs
  src		sys/inet
  src		sys/init
ESZKÖZMEGHAJTÓK
  src		sys/drv/fs/gpt.so
  src		sys/drv/fs/pax.so
  src		sys/drv/fs/vfat.so
  src		sys/drv/fs/fsz.so
  src		sys/drv/fs/tmpfs.so
  src		sys/drv/input/ps2.so
  src		sys/drv/display/fb.so
  src		sys/driver
ALKALMAZÁSOK
LEMEZKÉPEK
  mkfs		initrd
  mkfs		boot (ESP)
  mkfs		usr
  mkfs		var
  mkfs		home
  mkfs		bin/osZ-latest-x86_64-ibmpc.img
```

Nem-EFI betöltő
---------------

Ha újra akarod fordítani a `loader/boot.bin` és `loader/bootboot.bin` fájlokat, akkor szükséged lesz a [fasm](http://flatassembler.net)-ra.
Sajnos a GAS nem eléggé okos 16, 32 és 64 bites utasítások keverésében, ami elengedhetetlen a BIOS-ról induláskor. Hogy megtarthassam
az ígéretem, hogy csak GNU eszköztárra lesz szükség, hozzáadtam az előre lefordított BOOTBOOT binárisokat a forrásokhoz.

Nézd mit csináltál!
-------------------

Az indítható lemezkép [bin/osZ-(ver)-(arch)-(platform).img](https://gitlab.com/bztsrc/osz/blob/master/bin) néven generálódik ki. Ezt az
[USBImager](https://gitlab.com/bztsrc/usbimager) alkalmazással vagy a `dd` paranccsal kiírhatod pendrájvra vagy SD kártyára, illetve
futtathatod qemu, bochs vagy VirtualBox alatt.

```
$ dd if=bin/osZ-latest-x86_64-acpi.img of=/dev/sdc
$ make testq
$ make testb
$ make testv
```

Részletes [ismertető a tesztelésről](https://gitlab.com/bztsrc/osz/blob/master/docs/howto1-testing.md).
