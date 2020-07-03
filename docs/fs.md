FS/Z Fájlrendszer
=================

Úgy lett megtervezve, hogy elképzelhetetlen mennyiségű adatot legyen képes tárolni. Klasszikus UNIX fájlrendszer, inode-okkal
és szuperblokkal, de a meglévők limitációi nélkül. Nincs felső határ szabva a fájlok vagy könyvtárak számának FS/Z-ben (kivéve
a fizikai kapacitást). A logikai szektorszámozás 2^128 biten van tárolva hogy jövőbiztos legyen. A jelenlegi implementáció csak
2^64 bitet használ (elegendő 64 Zettabájtos lemezekhez 4096-os logikai szektormérettel, és 1 Yottabájtosakhoz 64k-s logikai
szektormérettel).

A logikai szektorméret 2048, 4096, 8192... stb. lehet. A javasolt méret megegyezik a mindenkori architektúra memória lapméretével.
Ez 4096 x86_64-on és AArch64-en.

A fájlrendszer új lett kialakítva, hogy sérült lemez esetén az `fsck` a lehető legtöbbet visszanyerhesse. Egy teljesen elrontott
szuperblokk esetén az adatai visszanyerhetők a többi szektor elemzésével, inode-okat és metaadatokat keresve. Szoftver szintű
RAID tükrözés szintén lehetséges ezzel a fájlrendszerrel.

A pontos, bitszintű specifikáció és a lemezen lévő formátum megtalálható az [fsZ.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/fsZ.h)
fájlban. A magasabb szintű absztrakciós réteg leírásáért pedig lásd [VFS](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.md).

Implementációk
--------------
A fájlrendszer 3 különböző helyen került implementálásra:
 1. a betöltő [loader](https://gitlab.com/bztsrc/osz/blob/master/loader)-ben, ami GPT-t, FAT-et használ az initrd megtalálására, és FS/Z-t, cpio-t, tar-t, sfs-t a kernel keresésére az initrd-ben. Ez egy csak olvasni tudó, töredezettségmentes fájlrendszer implementáció.
 2. a [core](https://gitlab.com/bztsrc/osz/blob/master/src/core/fs.c)-ban, ami az indulás korai szakaszában tölt be fájlokat az initrd-ről (például az FS/Z fájl rendszer meghajtót). Ez szintén egy csak olvasni tudó, töredezettségmentes fájlrendszer implementáció.
 3. mint az FS taszk [fájlrendszer meghajtója](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/fs/fsz/main.c), ami egy teljes értékű implementáció, írás/olvasás és tördezettség támogatással.

FS/Z Szuperblokk
----------------

A szuperblokk a legelső logikai szektor a médián és opcionálisan a legutolsó szektoron megismételve. Ha a két szuperblokk eltér,
akkor a CRC ellenőrző összeg alapján eldönthető, melyik a helyes.

Logikai Szektorszám (LSN)
-------------------------

A logikai szektor mérete a szuperblokkban van rögzítve. A szektorszám egy skaláris, szuperblokkhoz viszonyított, relatív szám
2^128-on ábrázolva. A szuperblokk ezért mindig a 0-ás LSN-en található, függetlenül attól, hogy a lemez tartalmaz-e
partíciókat vagy sem, és a 0-ás LSN egyben 0-ás LBA-e. Ez azt jelenti, hogy a fájlrendszert kedvedre átmozgathatod
annélkül, hogy a szektorhivatkozások újra kéne számolni benne. Mivel a szuperblokkot tartalmazó 0-ás LSN nem hivatkozható
fájlokból, ezért a leképezésekben a 0-ás LSN egy csupa nullákat tartalmazó szektort jelent, lehetővé téve lyukak tárolását
a fájlokban.

Fájl Azonosító (fid)
--------------------

A fájl azonosító (vagy fid röviden) egy olyan logikai szektorszám, ami egy inode-ot tartalmazó szektorra mutat. Ez ellenőrizhető
azzal, hogy a szektor az "FSIN" azonosítóval kezdődik.

Csomópont (inode)
-----------------

A legfontosabb struktúra az FS/Z-ben. A lemez egy részét írja le mint könyvtárbejegyzés vagy fájl tartalma stb. Ez a logikai
szektor első 1024 bájtja (vagy 2048 ha a szuperblokkban a nagy inode opció van megjelölve). Több változatot is tárol, ezzel
lehetővé téve nemcsak partíciós szintű pillanatképeket, hanem fájl szintű pillanatképeket és gyors visszaállítást is.

Az FS/Z továbbá tárol a fájlok tartalmáról metainformációkat (mint például mime típus vagy ellenőrzőösszeg), és cimkéket is kezel.

A tartalmak egy saját címtérbe vannak leképezve (ami eltér az LSN-től). Ehhez az FS/Z két, különböző leképezési mechanizmust is
használ. Az első nagyon hasonló a virtuális memória leképezéshez, a második változó méretű területeket, ún. extent-eket alkalmaz.

Szektor Lista (sl)
------------------

Ez használatos az extent-ek, valamint a hibás és szabad szektorok listájához is. Egy listaelem áll egy kezdő LSN-ből és a
logikai szektorok számából, amivel így egy változó méretű, egybefüggő területet ír le a lemezen. Minden extent 32 bájt hosszú.

Szektor Könyvtár (sd)
---------------------

Egy olyan logikai szektor, amiben (logikai szektorméret)/16 bejegyzés található. A memórialeképezéshez hasonlító címzés alapegysége.
Akárcsak a szektor listák, 2^128 bites LSN-t tárolnak (kivéve, ha a tartalomellenörző funkció engedélyezve van, ekkor "csak" 2^96),
de a listával ellentétben fix méretű, azonban nem összefüggő területet ír le a lemezen. Nem összekeverendő a sima könyvtárakkal,
amik fid-et rendelnek a fájlnevekhez.

Könyvtár
--------

Minden könyvtárban 128 bájtos bejegyzések találhatók, ami azt jelenti, hogy maximum 2^121-1 bejegyzés lehet egy könyvtárban.
Az első bejegyzés speciális, a könyvtárlista fejléce, a többi sima fid és fájlnév összerendelés. A bejegyzések ábécé szerint
vannak rendezve, ami rendkívül gyors, 0(log n) keresést tesz lehetővé a `libc` függvénykönyvtár
[bsearch](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c) függvényét használva.

16 bájt tárolja a fid-et, egy bájt a fájlnévben található többbájtos karakterek számát. Ez 111 bájtot hagy a névnek (ami lehet,
kevesebb karakterben, mivel az UTF-8 változó hosszúságú kódolás). Ez a legkomolyabb megszorítás az FS/Z-ben, de lássuk be, elég
jó eséllyel sose találkoztál 42 karakternél hosszabb fájlnévvel. A legtöbb fájlnév 16 karakternél rövidebb.

