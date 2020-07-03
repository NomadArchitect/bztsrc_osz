A fordítási környezet segédprogramjai
=====================================

Ezek a C források a hoszt rendszerre fordulnak, nem arra, amire az oprendszert fordítjuk.

- *cross-gcc.sh* shell szkript a gcc keresztfordító letöltésére és lefordítására (egyszer hívd meg)
- *config.sh* shell szkript, amit a `make config` hív
- *elftool.c* gyors és piszkos eszköz ELF formátumú fájlok kezelésére
- *mkfs.c* segédprogram [FS/Z](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.md) fájlrendszer és lemezképek generálására
- *fsZ-fuse.c* FUSE meghajtó FS/Z fájlrendszerek Linux alatti felcsatolására
- *sloc.sh* SLoC (forráskód sorok száma) riport generálása
- *txt2psf.sh* szöveges fájlt konvertál PSF2 formátumúra (lásd etc/system.8x16.txt)
