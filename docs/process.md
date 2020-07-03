OS/Z Processzek és Taszkok
==========================

Objektum formátum
-----------------

OS/Z az ELF64 formátumot használja a futtatható programok lemezen történő tárolására. Az ELF fájl 7. bájtja, az EI_OSABI
```
#define ELFOSABI_OSZ            5
```
Az ELF fájlban a Program Headers résznek teljes egészében az első 4k-ban (vagy bármennyi legyen a __PAGESIZE az adott
architektúrán) kell lennie, és a benne lévő első három szegmens sorrendje kötelezően: kód (text), adat+bss (data) és
dinamikus linkelés (dynamic).

Processzek
----------

Minden processznek külön [memória címtere](https://gitlab.com/bztsrc/osz/tree/master/docs/memory.md) van.
Különböző a kódszegmensük, ami a kódot és a megosztott függvénykönyvtárakat tartalmazza. Ugyancsak különböző a bss szegmensük,
a dinamikusan lefoglalt memória, csakis a globális megosztott memória van ugyanúgy leképezve a címterükbe.

Valahányszor az [ütemező](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c) egy olyan taszkot választ, ami egy másik
processzhez tartozik, mint a megszakított taszk processze, a teljes címfordító gyorsítótár, az egész TLB kiürítésre kerül.

Minden processznek legalább egy taszkja van. Amikor az utolsó taszk is kilép, a processz megszűnik.

Taszkok
-------

Az azonos processzhez tartozó taszkok címtere majdnem teljesen megegyezik. Csak az első két laptáblában különböznek, a Taszk
Kontrol Blokkban, az [üzenet sor](https://gitlab.com/bztsrc/osz/blob/master/docs/messages.md)ban és veremben.
A core optimalizált módon kapcsol a taszkok között (amikor a jelenlegi és az új taszk kódszegmense azonos). Ekkor csak az első
két memóriablokk (4M) kerül érvénytelenítésre, a TLB gyorsítótár nem lesz kiüresítve, ami sokkal gyorsabb működést eredményez.

Ez a koncepció közelebb áll a Solaris Könnyű Súlyú Processzeihez (Light Weight Processes), mint a jól ismert pthread
függvénykönyvtár, ahol a szálakról a kernelnek nincs tudomása. Vegyük észre, hogy OS/Z-ben a pid_t taszkokat jelöl, így a
felhasználói programok minden taszkot önálló processznek látnak, azaz az igazi processzek teljesen rejtve maradnak előlük.
