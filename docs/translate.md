OS/Z - Többnyelvűség támogatása
===============================

Előszó
------

A kívánt nyelv kódja megadható az [indítási konfigurációban](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md) a `lang`
kulcsszóval.

Szótárak
--------

A core, libc, sh és sys (minden initrd-n található program) fordítása a [/sys/lang](https://gitlab.com/bztsrc/osz/tree/master/etc/sys/lang)
mappában helyezkedik el.
A felhasználói programok szótárfájljai (amik az /usr alá kerülnek telepítésre) külön, saját könyvtárakban találhatók, amik a libc
`lang_init()` hívásával tölthetők be. Ennek egy fájlnév előtagot kell megadni (például "/usr/gcc/lang/ld"), a sztringek számát,
és egy sztringmutatókat tartalmazó tömb címét, ami feltöltésre kerül. A fordítók bármikor új nyelveket adhatnak hozzá, nem szükséges
a programokat újrafordítani. Csak a fontos, hogy minden sor pontosan egy, lefordított UTF-8 sztring (sortörés és más kódlap nem
megengedett).

Üzenetek
--------

Nagyon fontos, hogy **csakis a felhasználónak megjelenített üzenetek kerülnek fordításra**. A debug üzenetek és a naplófájlok
üzeneteinek MUSZÁJ angolul lenniük. Ennek egyszerű oka van.

A legtöbb forrásban minden változónév és függvénynév angolul van, valamint a legtöbb dokumentáció is angolul található meg a weben,
így jogos feltételezés, hogy egy fejlesztő legalább alapszérten megérti az angolt. Mivel a debug üzenetek gyakran tartalmaznak
változó és függvényneveket (amik angolul vannak), a fordítás kifejezetten hátrányos lenne, és roppantmód megnehezítené a debug
konzol és a forrás összekapcsolását.

Ami a napló (syslog) üzeneteket illeti, nagyon valószínű, hogy szkriptek is feldolgozzák, ezért a fordításból eredő változatok
nem kívánatosak. Ezen kívül fontos, hogy amikor egy rendszermérnök a naplókat böngészi - függetlenül attól, hogy honnan származik,
és hogy milyen fordítással fut az OS - pontosan ugyanazokat az üzeneteket lássa, és azokat ugyanúgy értelmezze a Föld bármely
pontján. További előny, hogy archaikus okokból az angol nyelv 7 bites ASCII karakterekkel leírható, ami teljesen kódlap független,
és nem igényel UNICODE betűtípust a megjelenítéséhez. Végezetül lássuk be, hogy az angol egy roppantul primitív nyelv, az egyik
legkönnyebben tanulható. Mégha valaki nem is beszél angolul, könnyű megjegyezni azt a néhány kifejezést, ami előfordulhat a
naplókban, mint például "not found", "error" vagy "failed"; az igazán nemzetközi "OK"-éról nem is beszélve.

Továbbá van néhány sztring, ami nem lefordítható, egész egyszerűen azért, mert még azelőtt íródnak ki, hogy a szótárfájl betöltésre
kerülhetne:
1. "CPU feature not supported" a core/(arch)/platform.S-ban (CPU funkció nem támogatott)
2. "BOOTBOOT environment corrupt" a core/env.c-ben (BOOTBOOT környezet érvénytelen)
3. "Unable to load language dictionary" a core/lang.c-ben (Nem lehet betölteni a szótárfájlt)

Ez minden!
