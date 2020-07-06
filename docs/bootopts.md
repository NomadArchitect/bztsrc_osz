OS/Z Indítási opciók
====================

Fordításkori opciók
-------------------

Egynéhány opció (főként sebességnövelés miatt) a [Config](https://gitlab.com/bztsrc/osz/blob/master/Config)-ba került. Ha ezeken
változtatsz, akkor újra kell fordítanod. De a legtöbb funkció induláskor állítható. A következő parancsot futtatva könnyedén
konfigurálhatsz:

```sh
$ make config
```

<img height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszcfg1.png" alt="make config">


Indulási konfigurációs fájl (environment)
-----------------------------------------

Ez az, amit a Linux commandline-nak hív. Az indítási opciók az első bootolható lemez első bootolható partícióján, a
`FS0:\BOOTBOOT\CONFIG` vagy `/sys/config` alatt találhatóak. Amikor a lemezképek készülnek, akkor az
[etc/config](https://gitlab.com/bztsrc/osz/blob/master/etc/config) tartalma másolódik ide.

Ez egy sima szöveges UTF-8 fájl `kulcs=érték` párokkal, amit a [core/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c)
és a [libc/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/env.c) értelmez. Tagolókarakterek nem megengedettek, és a
párokat újsor (0x0A) karakter választja el egymástól. A fájl teljes egésze nem lehet nagyobb, mint egy lap (ez 4096 bájt x86_64-en
és AArch64-on). Kommenteket lehet tenni bele, a '#", '//' and '/*' karakterekkel.

A kulcsok ASCII nevek szóközök nélkül, az értékek lehetnek decimális vagy hexa [számok, logikai értékek vagy UTF-8 sztringek](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-driver.md).

Kulcsok
-------

| Paraméter  | Alapért.  | Típus     | Alrendszer | Leírás |
| ---------- | :-------: | --------- | ---------- | ----------- |
| screen     | 1024x768  | szám<i>x</i>szám | [loader](https://gitlab.com/bztsrc/osz/blob/master/loader) | betöltő használja, a kért felbontás, legalább 640x480 |
| kernel     | sys/core  | sztring   | loader   | a kernel futtatható neve az initrd-ben |
| tz         | default   | sztring/szám | core  | időzóna percekben (-1440..1440) vagy "default" (alapért.) vagy "ask" (rákérdez), lásd alább |
| lang       | en        | sztring   | core     | nyelvkód, a felhasználói felület [nyelve](https://gitlab.com/bztsrc/osz/blob/master/docs/translate.md), "xx" vagy "xx_XX" formátumban |
| debug      | 0         | speciális | core     | debug üzenetek szűrése (ha [debug opcióval fordítottuk](https://gitlab.com/bztsrc/osz/blob/master/Config), lásd alább) |
| dmabuf     | 0         | szám      | core     | DMA buffer mérete lapokban (az alsó 16M-nyi RAM-ból lefoglalva, alapból 256 (1M) az ibmpc-n) |
| quantum    | 1000      | szám      | core     | maximum mennyi mikroszekundumig birtokolhatja a CPU-t egy taszk, 1/(quantum) taszkkapcsolás másodpercenként |
| display    | mc     | típus[,megh] | core     | megjelenítő típusa, és opcionálisan az eszközmeghajtója, lásd alább |
| aslr       | false     | logikai | core     | engedélyezi a véletlenszerű címtereket (Address Space Layout Randomization) |
| syslog     | true      | logikai | core     | kikapcsolja a rendszer napló [szolgáltatás](https://gitlab.com/bztsrc/osz/blob/master/docs/services.md)t |
| internet   | true      | logikai | core     | kikapcsolja a hálózat szolgáltatást (internet) |
| sound      | true      | logikai | core     | kikapcsolja a hangkeverő szolgáltatást |
| print      | true      | logikai | core     | kikapcsolja a nyomtatási sor szolgáltatást |
| rescueshell | false    | logikai | core     | ha true, akkor a `/bin/sh`-t indítja a felhasználói szolgáltatások helyett |
| spacemode  | false     | logikai | core     | ha kijavíthatatlan hiba lépne fel, az OS magától újraindul |
| pathmax    | 512       | szám    | fs       | elérési út maximális hossza bájtban, minimum 256 |
| cachelines | 16        | szám    | fs       | a blokk gyorsítótár vonalainak a száma, minimum 16 |
| cachelimit | 5        | százalék | fs       | blokkgyorsítótár kiírása és kiürítése ha a szabad RAM ezalá csökkenne, 1%-50% |
| keyboard   | pc105,en_us | szt,szt[,szt] | ui | billentyúzet típusa és kiosztása(i), lásd [etc/kbd](https://gitlab.com/bztsrc/osz/blob/master/etc/etc/kbd) |
| focusnew   | true      | logikai | ui       | új ablakok automatikusan fókuszálva lesznek |
| dblbuf     | true      | logikai | ui       | kettős buffer alkalmazása (memóriaigényes, de gyors) |
| lefthanded | false     | logikai | ui       | mutatók felcserélése, balkezes mód |
| identity   | false     | logikai | init     | első induláskori gép azonosság bekérő kényszerített indítása |
| hpet       | -         | hexa    | platform | x86_64-acpi detektált HPET cím felülbírálása |
| apic       | -         | hexa    | platform | x86_64-acpi detektált LAPIC cím felülbírálása |
| ioapic     | -         | hexa    | platform | x86_64-acpi detektált IOAPIC cím felülbírásása |

Debuggolás
----------

Ez lehet egy szám, vagy vesszővel felsorolt jelzők, lásd [debug.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/debug.h).

| Érték  | Jelző | Define | Leírás |
| -----: | ----- | ------ | ----------- |
| 0      |       | DBG_NONE | nincs debug információ |
| 1      | pr    | DBG_PROMPT | induláskor a legelső taszkkapcsolás előtt debug parancssort ad |
| 2      | me    | DBG_MEMMAP | memóriatérkép dumpolás a rendszer napló tartalmazza) |
| 4      | ta    | DBG_TASKS | taszk létrehozás, pl [rendszer szolgáltatások](https://gitlab.com/bztsrc/osz/blob/master/docs/services.md) betöltésének debuggolása |
| 8      | el    | DBG_ELF | [ELF értelmező](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c) debuggolása |
| 16     | ri    | DBG_RTIMPORT | [futás idejű linkelő](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c) importált változók dumpja |
| 32     | re    | DBG_RTEXPORT | futás idejű linkelő exportált változók dumpja |
| 64     | ir    | DBG_IRQ | az [IRQ Átirányító Tábla](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c) (IRT) dumpolása |
| 128    | de    | DBG_DEVICES | a [rendszerleíró táblák](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/acpi/acpi.c) és a [PCI eszközök](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pci.c) listázása |
| 256    | sc    | DBG_SCHED | az [ütemező](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c) debuggolása |
| 512    | ms    | DBG_MSG | rendszer [üzenetek](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c) küldésének debuggolása |
| 1024   | lo    | DBG_LOG | a korai [rendszer napló](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c) kiküldése a korai konzolra |
| 2048   | pm    | DBG_PMM | a [fizikai memória kezelő](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c) debuggolása |
| 4096   | vm    | DBG_VMM | a [virtuális memória kezelő](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c) debuggolása |
| 8192   | ma    | DBG_MALLOC | a [libc memóriafoglalás](https://gitlab.com/bztsrc/osz/blob/master/src/libc/bztalloc.c) nyomonkövetése |
| 16384  | bl    | DBG_BLKIO | a [blokk szintű B/K](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c) műveletek debuggolása |
| 32768  | fi    | DBG_FILEIO | a [fájl szintű B/K](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c) műveletek nyomonkövetése |
| 65536  | fs    | DBG_FS | a [fájlrendszer](https://gitlab.com/bztsrc/osz/blob/master/src/fs/main.c) műveletek listázása |
| 131072 | ca    | DBG_CACHE | a [blokk gyorsítótár](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c) nyomonkövetése |
| 262144 | ui    | DBG_UI | a [felhasználói felület](https://gitlab.com/bztsrc/osz/blob/master/src/ui/main.c) üzeneteinek debuggolása |
| 524288 | te    | DBG_TESTS | rendszer [teszt](https://gitlab.com/bztsrc/osz/blob/master/src/test) taszk indítása az [init](https://gitlab.com/bztsrc/osz/blob/master/src/init) helyett |

A legtöbb ezek közül csak akkor használható, ha [DEBUG = 1](https://gitlab.com/bztsrc/osz/blob/master/Config) opcióval fordítottunk.
Annélkül mindössze két opció elérhető a rendszerindulás problémáinak felderítése végett: DBG_DEVICES és a DBG_LOG.

Megjelenítő
-----------

Egy szám, vagy pontosan egy jelző, ami a vizualizáció típusát adja meg, illetve opcionálisan az eszközmeghajtó neve egy vesszővel elválasztva.

| Érték | Jelző  | Define | Leírás |
| ----: | -----  | ------ | ----------- |
| 0     | mm     | PBT_MONO_MONO | sima 2D pixelbuffer 8 bites monokróm pixelekkel |
| 1     | mc     | PBT_MONO_COLOR | sima 2D pixelbuffer 32 bites ARGB pixelekkel |
| 2     | sm,an  | PBT_STEREO_MONO | két 2D pixelbuffer*, szürkeárnyalatos és vörös-cián szűróvel ellátva, anaglif sztereó 3D |
| 3     | sc,re  | PBT_STEREO_COLOR | két 2D pixelbuffer*, a bal és jobb szem színes képének összehangolása az eszközmeghajtó dolga, igazi 3D |

(* a két buffer össze van fűzve egyetlen dupla magasságú bufferben)

Az eszközmeghajtók helye a "sys/drv/display/", példa: `display=mc,nvidia` a "sys/drv/display/nvidia.so"-t fogja keresni. Ha
nincs megadva, akkor az első detektált videokártya meghajtót használja. Ha nem talált ilyent, akkor a CPU-ból lekezelt lineáris
frame buffer meghajtót tölti be, ami lassú, de mindig működik. Alternatívaként minden ablak átirányítható egy távoli
gépre a "remote:(ip)" megadásával, például `display=mc,remote:192.168.1.1`.

Az OS/Z több képernyőt is támogat, az amelyik az UI taszkba töltődik be, a "0", ami az első helyi képernyő.

Időzóna
-------

Akár egy szám az -1440..1440 tartományban (eltolás percekben) vagy a "default" illetve "ask" sztring. A "default" azt jelenti, hogy
csak akkor kérdezzen rá, ha nem sikerült az idózónát detektálni (nincs RTC, azaz valós idejű óra). Másrészről az "ask" rákérdez,
mindig bekéri a felhasználótól (mert például az RTC lehet pontatlan). Ha "tz=0"-át adunk meg, akkor az időt UTC-ben (Greenwichi
középidőben) fogja kezelni a rendszer, és mindenképp továbblép az indulásnkor, akkor is, ha nem sikerült semmit detekátlni. Ez
utóbbi esetben "0000-00-00T00:00:00+00:00" fog látszódni a rendszer naplóban, míg az ntpd be nem állítja a pontos időt.

Alacsony memória fogysztás
--------------------------

Az olyan helyeken, ahol drága a memória, mint például a beágyazott rendszereknél, beállíthatod az OS/Z-t a következőképp (figyelembe
véve hogy az UI taszk eszi messze a legtöbb memóriát):
1. kettős bufferelés kikapcsolása `dblbuf=false`-val. Ez felére csökkenti a kompozítor memóriaigényét, de maszatolhat tőle a kép.
2. használj monokróm, alacsony felbontást. Kicsit lassabb, de csak 8 bitet eszik pixelenként 32 helyett, amivel még tovább, a negyedére csökkenthető a memóriaigény.
3. különleges esetekben cseréld le a `logind`-t egy egyedi alkalmazással hogy elkerüld a felesleges ablakokat és alkalmazásokat.
4. alternatívaként használj távoli megjelenítőt, hogy a beégyazott rendszeren egyáltalán ne kelljen pixel buffert lefoglalni.

Sebességre optimalizálás
------------------------

Ha a cél a sebesség, és van rengeteg RAM-od illetve támogatott az SSSE3 (x86_64) vagy a Neon (AArch64), akkor:
1. fordítsd újra `OPTIMIZE = 1` opcióval a [Config](https://gitlab.com/bztsrc/osz/blob/master/Config)ban. Ez kihasználja a SIMD utasításokat.
2. használj truecolor vizuális típust, olyan kártyán, ami natívan támogatja az ARGB pixel formátumot (és nem az ABGR-t). Ebben az esetben egy külön erre optimalizált blittelőt fog használni (ami tovább optimalizálható az 1. ponttal).

