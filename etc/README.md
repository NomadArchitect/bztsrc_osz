Tartozék fájlok
===============

Könyvtárvázak

- *root* a root felhasználó könyvtára
- *sys* rendszer fájlok, sys/etc az initrd-be kerül
- *sys/lang* nyelvi fájlok forrásfájlai

Fájlok

- *sys/config* a boot konfiguráció (environment). A mérete nem haladhatja meg a 4096 bájtot.
  Innen másolódik az [FS0:\BOOTBOOT\CONFIG](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md) fájl a lemezkép összeállításakor.
- *bochs.rc* [bochs](http://bochs.sourceforge.net/) konfigurációs fájl
- *logo.png* *logo.tga* OS/Z logó (tga mentés GIMP opciói: rle compressed, color-mapped, origin: top left)
- *script.ARCH.gdb* indító szkriptek a [GDB](https://www.sourceware.org/gdb/)-hez
- *system.8x16.sfn* [Scalable Screen Font](https://gitlab.com/bztsrc/scalable-font2), alapértelmezett UNICODE font a kprintf, debugger és a tty ablakok számára
- *cc-by-nc-sa-icon.png* CC-by-nc-sa logó
