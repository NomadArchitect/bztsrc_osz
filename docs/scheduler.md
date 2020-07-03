OS/Z Ütemező
============

Az operációs rendszer feladata biztosítani, hogy egyetlen taszk se legyen kiéheztetve. A Linux ezt úgy proóbálja
megoldani, hogy egy dinamikusan számolt értékkel korrigálja a prioritást. OS/Z egy alapvetően eltérő irányból
közelíti meg a kérdést, nevezetesen rekurzív prioritási szinteket használ. Ez azt jelenti, hogy minden prioritási
szinten egy időszelet egy eggyel alacsonyabb prioritású taszk számára van fenntartva.

Például legyen 3 taszkuk az 5-ös prioritási szinten, és tegyük fel, nincs prioritásosabb taszk, ami futhatna, ekkor
a teljes idő 4 szeletet fog tartalmazni. Ebből a négyből három az 5-ös szintű taszkoknak van, egy pedig a 6-os vagy
alacsonyabb szintűeknek. Egy szinten belül a taszkok mindig körkörösen kerülnek kiválasztásra, de az egy processzhez
tartozó taszkok egymás után vannak csoportosítva, hogy ezáltal csökkenjen a TLB gyorsítótár kiüresítésének gyakorisága.

Prioritási szintek
------------------

OS/Z-ben 8 szint van definiálva:

| Ssz. | Szint    | Leírás |
| ---- | -------- | ----------- |
|    0 | PRI_SYS  | Rendszer |
|    1 | PRI_RT   | Valós idejű taszkok (Real Time) |
|    2 | PRI_DRV  | Eszközmeghajtók |
|    3 | PRI_SRV  | Szolgáltatások |
|    4 | PRI_APPH | Alkalmazások magas (High) |
|    5 | PRI_APP  | Alkalmazások normál |
|    6 | PRI_APPL | Alkalmazások alacsony (Low) |
|    7 | PRI_IDLE | Üresjárat |

A legmagasabb szint a PRI_SYS (nulla). Ez a szint általában üres, vészhelyzeti folyamatok számára fenntartott.

Ezt követi a PRI_RT (1) a valós idejű taszkok számára. Ez a legmagasabb szint, amit egy felhasznlói folyamat elérhet.

Ezután jön a PRI_DRV (2) az eszközmeghjatók szintje. Ez a felső 3 szint nem megszakítható, ami azt jelenti, hogy az
ütemező sosem fogja félbeszakítani a bennük futó taszkokat.

A következő szint a PRI_SRV (3) a szolgáltatások, vagy unix démonok szintje. Ez az első időosztásos, megszakítható szint.

A sima alkalmazások három szint valamelyikén futnak, ami lehetővé teszi, hogy súlyozzunk a programok között. Ezek a
PRI_APPH (4, magas prioritás), mint például a videólejátszók és játékok, PRI_APP (5, normál prioritás) például
szövegszerkesztő és böngésző, és a PRI_APPL (6, alacsony prioritás) mint például a ritkán frissülő időjárás kijezés.

Végezetül van a PRI_IDLE (7). Ez a szint csak akkor kerül ütemezésre, ha egyetlen egy másik, magasabb szintű folyamat
se futtatható (mind blokkolt). Ezen szinten fut a képernyőkímélő és a töredezettségmentesítő például. Ha még ez a szint is
üres, vagy csak blokkolódott folyamatokat tartalmaz, akkor egy speciális taszk, az
["IDLE"](https://gitlab.com/bztsrc/osz/tree/master/src/core/x86_64/platform.S) leállítja a processzort, míg megszakítás nem történik.

Példa
-----

Képzeljük el a következő taszkokat:

 - 4. prioritási szint: A, B, C
 - 5. prioritási szint: D
 - 6. prioritási szint: E, F

Ezek a következő sorrendben kerülnek beütemezésre:

 A B C D A B C E A B C D A B C F

Minden taszknak volt esélye futni, így egyik sem került kiéheztetésre, és az idő a következőképp került felosztásra:

 A: 4-szer,
 B: 4-szer,
 C: 4-szer,
 D: 2-szer,
 E: 1-szer,
 F: 1-szer

Ami pont azt az előfordulási mintát adja, amit ezektől a szintektől várunk.

Láthatóság
----------

A prioritási szintek fejmutatói a [CPU Kontroll Blokk](https://gitlab.com/bztsrc/osz/tree/master/src/core/x86_64/ccb.h)ban vannak
definiálva. Ez a blokk CPU-ként kerül leképezésre, ami azt jelenti, hogy minden processzornak saját prioritási listái vannak.

Taszk állapotok
---------------

Háromféle állapotot vehetnek fel. A következő ábra demonstrálja ezeket és a közöttük lévő átmeneteket.

```
        { tcb_state_hybernated }        hibernált
             ^              |
   sched_hybernate()        |
             |              |
    { tcb_state_blocked }   |           blokkolt
             ^      |       |
   sched_block()  sched_awake()
             |      v
      { tcb_state_running }             futó
             ^      |
  [syscall, IRQ]  sched_pick()
             |      v
     (CPU erőforrás allokált)
```
A futó állapotnak két alállapota van: futásra várakozó és ténylegesen futó. A taszk állapota szempontjából ez a különbség nem
lényeges. Amikor egy taszk ténylegesen fut, akkor a címtérben az ő Taszk Kontroll blokkja látszik a 0-ás címen.
