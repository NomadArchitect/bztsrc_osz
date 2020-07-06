OS/Z - operációs rendszer
=========================

<img align="left" style="margin-right:10px;" alt="OS/Z" src="https://gitlab.com/bztsrc/osz/raw/master/logo.png">
<a href="https://gitlab.com/bztsrc/osz/tree/master/bin/">Élő képek letöltése</a>,  <small>(~8 Mbájt)</small><br>
<a href="https://gitlab.com/bztsrc/osz/blob/master/docs/README.md">Dokumentáció</a> és <a href="https://bztsrc.gitlab.io/osz">weblap</a><br>
<a href="https://gitlab.com/bztsrc/osz/issues">Támogatás</a><br><br>

Az [OS/Z](https://bztsrc.gitlab.io/osz) egy modern, hatékony és skálázható Operációs Rendszer. Célja, hogy kicsi, elegáns,
[hordozható](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.md) legyen és hatalmas mennyiségű adatot legyen képes
kezelni felhasználóbarát módon.

Hogy ezt elérje, kikukáztam annyi limitációt, amennyit csak lehetett, emiatt csupán POSIX-szerű, de a [nem POSIX szabványos](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.md).
Például csak a tárolókapacitás szab határt a tárolható fájlok számának, és csakis a RAM mennyisége szabja meg, hogy hány
konkurens program futhat egyszerre. Ha nem sikerült kidobnom egy limitet, akkor [boot opciót](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md)
csináltam belőle, így a rendszer újrafordítás nélkül is átszabható. Emiatt az OS/Z egy rendkívül skálázható rendszer lett.

Jellemzők
---------

 - [GNU vagy LLVM eszköztár](https://gitlab.com/bztsrc/osz/blob/master/docs/compile.md)
 - Mikrokernel architektúra egy hatékony [üzenetküldő rendszer](https://gitlab.com/bztsrc/osz/blob/master/docs/messages.md)rel
 - Ugyanaz a lemezkép [bootolható](https://gitlab.com/bztsrc/osz/blob/master/docs/boot.md) BIOS, UEFI vagy Raspberry Pi gépen
 - [Magas címterű kernel](https://gitlab.com/bztsrc/osz/blob/master/docs/memory.md), komplett 64 bites támogatás
 - A [fájlrendszer](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.md)e YottaBájtnyi adatot képes kezelni (jelenleg elképzelhetetlen mennyiség)
 - ELF64 formátum támogatás
 - UNICODE támogatás UTF-8-al
 - Többnyelvűség

Hardver követelmények
---------------------

 - 10 Mb szabad lemez terület
 - 32 Mb RAM
 - 800 x 600 / ARGB monitor
 - IBM PC x86_64 processzorral  - vagy -  Raspberry Pi 3 AArch64 processzorral
 - [Támogatott eszközök](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.md)

Tesztelés
---------

A [legfrissebb lemezkép](https://gitlab.com/bztsrc/osz/raw/master/bin/osZ-latest-x86_64-ibmpc.img)ről indítható az OS/Z emulátorokon és igazi gépeken. Például

```shell
qemu-system-x86_64 -hda bin/osZ-latest-x86_64-ibmpc.img
```
Részletesebb információkért látogasd meg a [Hogyan teszteljük](https://gitlab.com/bztsrc/osz/blob/master/docs/howto1-testing.md) leírást.
A lemezképet [qemu](http://www.qemu.org/), [bochs](http://bochs.sourceforge.net/) és [VirtualBox](https://www.virtualbox.org/) alatt is ki szoktam próbálni.

Licensz
-------

| Rész                                         | Licensz                                                  |
| -------------------------------------------- |--------------------------------------------------------- |
| külső repós modulok és az FS/Z lemezformátum | MIT                                                      |
| eszközmeghajtók                              | dual licensz, CC-by-nc-sa vagy zárt forrású kereskedelmi |
| minden más ebben a repóban                   | CC-by-nc-sa                                              |

A betöltő program és a [BOOTBOOT](https://gitlab.com/bztsrc/bootboot) Protokoll, a [memória allokátor](http://gitlab.com/bztsrc/bztalloc),
az [SSFN](https://gitlab.com/bztsrc/scalable-font2) font formátum valamint az
[FS/Z lemezformátuma](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/fsZ.h) teljesen szabad, MIT licensz alatt kerül terjesztésre.
Mivel a libfuse GPL licenszű, ezért az [FS/Z FUSE](https://gitlab.com/bztsrc/osz/blob/master/tools/fsZ-fuse.c) meghajtója szintén GPL licenszet használ.
Minden más az OS/Z-ben (beleértve a natív [FS/Z](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.md) implementációt is) azonban CC-by-nc-sa-4.0
licenszű, az eszközmeghajtók kivételével, azoknál megengedett a zárt forráskódú, bináris terjesztés.

Ez azt jelenti, hogy bár az OS/Z **Nyílt Forráskódú**, de **nem Szabad Szoftver**. Használhatod és játszhatsz vele otthon, de nincs
semmi garancia a megfelelő működésre. Nem használhatod fel üzleti célra vagy üzleti környezetben a szerző előzetes írásos
hozzájárulása nélkül. Nyiss egy [jegyet](https://gitlab.com/bztsrc/osz/issues) vagy írj egy emailt, ha szükséged van erre.

 Copyright (c) 2016-2020 bzt (bztsrc@gitlab) [CC-by-nc-sa-4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)

**A művet szabadon**:

 - **Megoszthatod** — másolhatod és terjesztheted a művet bármilyen módon vagy formában

 - **Átdolgozhatod** — származékos műveket hozhatsz létre, átalakíthatod
     és új művekbe építheted be. A jogtulajdonos nem vonhatja vissza
     ezen engedélyeket míg betartod a licensz feltételeit.

**Az alábbi feltételekkel**:

 - **Nevezd meg!** — A szerzőt megfelelően fel kell tüntetned, hivatkozást
     kell létrehoznod a licenszre és jelezned, ha a művön változtatást
     hajtottál végre. Ezt bármilyen ésszerű módon megteheted, kivéve
     oly módon ami azt sugallná, hogy a jogosult támogat téged vagy a
     felhasználásod körülményeit.

 - **Ne add el!** — Nem használhatod a művet üzleti célokra.

 - **Így add tovább!** — Ha feldolgozod, átalakítod vagy gyűjteményes művet
     hozol létre a műből, akkor a létrejött művet ugyanazon licensz-
     feltételek mellett kell terjesztened, mint az eredetit.

Projekt struktúra
=================

```
(git repó)
|
|_ bin/                 generált fájlok
|   |_ ESP/             a boot partíció fájljai (FAT, betöltő programok)
|   |_ home/            a home partíció fájljai (privát felhasználói adatok)
|   |_ initrd/          az initrd fájljai (ESP/BOOTBOOT/INITRD vagy boot partíció)
|   |_ usr/             az usr partíció fájlai (rendszer partíció)
|   |_ var/             a var partíció fájlai (publikus adatok)
|   |_ *.part           partíciók képfájljai
|   \_ *.img            egész lemezképek
|
|_ docs/                dokumentációk
|   \_ README.md        dokumentáció fő indexfájlja
|
|_ etc/                 et cetera, mellékes fájlok
|   |_ root/            a rendszergazda home-jának váza, /root az initrd-n
|   |_ etc/             a /sys/etc váza az initrd-n
|   |_ lang/            a core és libc nyelvi fájlai, fordításai
|   \_ config           indulókörnyezet konfigurációja (ESP/BOOTBOOT/CONFIG, a "make config" generálja)
|
|_ include/             C header fájlok, a fordításkor használt (valamint az /usr/sys/include váza)
|
|_ loader/              előre lefordított betöltő programok, lásd BOOTBOOT Protokoll
|
|_ src/                 forrásfájlok
|   |_ core/            felügyelő
|   |   |_ aarch64/     architektúra függő források
|   |   |   |_ rpi3/    platform függő források
|   |   |   |_ rpi4/    platform függő források
|   |   \_ x86_64/      architektúra függő források
|   |       |_ ibmpc/   platform specifikus források
|   |       \_ apic/    platform specifikus források
|   |_ drivers/         eszközmeghajtók forrásai
|   |   |_input/        példa eszköz kategória
|   |   |   \_ps2/      egy eszközmeghajtó
|   |   |_display/      példa eszköz kategória
|   |   |   \_bga/      egy eszközmeghajtó
|   |   \_*             további eszköz kategóriák
|   |_ libc/            a C függvénykönyvtár forrása
|   |_ sh/              a parancsértelmező forrása
|   |_ */               nélkülözhetetlen rendszer szolgáltatások, függvénykönyvtárak és alkalmazások
|   \_ *.ld             linkelő szkriptek
|
|_ tools/               a fordításhoz használt szkriptek és segédprogramok forrásai
|
|_ usr/                 felhasználói csomagok, az usr partícióra kerülnek telepítésre
|   \_sys/etc/          az /usr/sys/etc váza
|
|_ Config               a fordítókörnyezet konfigurációs fájlja ("make config" generálja)
|_ TODO.txt             generált teendők fájl
\_ Makefile             a fő projekt GNU make fájl
```

A kigenerált fájlrendszer struktúra leírását a [VFS](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.md) címszónál találod.

Szerzők
-------

qsort: Copyright The Regents of the University of California

BOOTBOOT, FS/Z, OS/Z, bztalloc: bzt

Hozzájárulások
--------------

Geri: tesztelés

jahboater: tesztelés, ARM optimalizációs javaslatok

Octocontrabass: tesztelés

BenLunt: konstruktív FS/Z kritika
