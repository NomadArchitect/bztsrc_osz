OS/Z Memóriakezelés
===================

Címtér
------
```
FFFFFFFF_FFFFFFFF +------------+---------------------+-----------------------------+
                  |            |                     |  2 M core kód + adat        |  (betöltő biztosítja)
                  |            |   1 G CPU globális  +-----------------------------+  CORE_ADDRESS
                  |            |                     |  1022 M core dinamikus adat |
FFFFFFFF_C0000000 |   512 G    +---------------------+-----------------------------+  CDYN_ADDRESS
                  | megosztott |   1 G CPU lokális   |  1024 M core dinamikus adat |
FFFFFFFF_80000000 |            +---------------------+-----------------------------+  LDYN_ADDRESS
                  |            |  510 G megosztott   | 522240 M felhasználói adat  |
                  |            |  dinamikus memória  |         smalloc()        ^  |
FFFFFF80_00000000 +------------+---------------------+-----------------------------+  SDYN_ADDRESS
0000007F_FFFFFFFF +------------+---------------------+-----------------------------+
                  |            |    4 G leképzett    |   4096 M speciális buffer   |
0000007F_00000000 |            +---------------------+-----------------------------+  BUF_ADDRESS
                  |            |     504 G taszk     | 516096 M felhasználói adat  |
                  |            |  dinamikus memória  |          malloc()        ^  |
00000001_00000000 |            +---------------------+-----------------------------+  DYN_ADDRESS
                  |   512 G    |                     |  4090 M felhasználói kód    |
00000000_00400000 |  lokális   |                     +-----------------------------+  TEXT_ADDRESS
                  |            |                     |  4M verem                v  |
                  |            |      4 G taszk      +-----------------------------+
                  |            |   statikus memória  |  4M-4K üzenet sor        ^  |
                  |            |                     +-----------------------------+  __PAGESIZE
                  |            |                     |  4K Taszk Kontroll Blokk    |
00000000_00000000 +------------+---------------------+-----------------------------+  TCB_ADDRESS
```

A memóriafoglalás több szinten történik:
 - [pmm_alloc()](https://gitlab.com/bztsrc/osz/tree/master/src/core/pmm.c) fizikai RAM lapok (csak core)
 - [vmm_map()](https://gitlab.com/bztsrc/osz/tree/master/src/core/vmm.c) virtuális lapok (csak core)
 - [kmalloc()](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c), krealloc(), kfree() core (kernel) dynbss memória
 - [malloc()](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c), realloc(), calloc(), free() dynbss memória
 - [smalloc()](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c), srealloc(), scalloc(), sfree() megosztott memória

A címtér két részre lett osztva, alsó és felső félre. Az alsó fél taszk specifikus, és taszkkapcsoláskor az egész alsó fél
lecserélődik. A felső fél globális, minden taszk esetén ugyanaz, kivéve 1G-nyi területet, ami CPU-nként eltér.

Felhasználói taszkok
--------------------

| Virtuális cím    | Láthatóság | Leírás |
| ---------------- | ---------- | ----------- |
| 2^48-4G .. 2^48  | [processz](https://gitlab.com/bztsrc/osz/tree/master/docs/process.md) | előre lefoglalt / leképezett rendzser bufferek (képernyő, initrd, MMIO, kód betöltés átmeneti terület stb.) |
|    4G .. 2^48-4G | processz   | dinamikusan [lefoglalt tls memória](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c) (felfele nő, írható/olvasható) |
|     x .. 4G-1    | processz   | megosztott függvénykönyvtárak (kód csak olvasható / adat írható/olvasható) |
|    4M .. x       | processz   | felhasználói program (kód csak olvasható / adat írható/olvasható) |
|    2M .. 4M-1    | taszk      | lokális verem (írható/olvasható, lefelé növekszik) |
| 16384 .. 2M-1    | taszk      | üzenet sor körkörös buffer (csak olvasható, felfelé növekszik) |
|  4096 .. 16383   | taszk      | üzenet sor (csak olvasható)) |
|     0 .. 4095    | taszk      | Taszk Kontroll Blokk (TCB, csak felügyeleti szintről látható, a NULL hivatkozások kivételt generálnak) |
| -512G .. -2G-1   | globális   | globális [megosztott memória](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c) (felhasználói elérés, írható/olvasható) |

Ha két taszk címtér leképezése azonos, kivéve a TCB-t, az üzenet sort és a vermet, akkor ugyanahhoz a processzhez tartoznak.

### UI

A felhasználói felület a lineáris frame buffert képezi le 2^48-4G-nél. Akkor használja, amikor a kompizítor frissíti a képet.

### FS

A fájlrendszer szolgáltatás az initrd-t képezi le 2^48-4G-nél. Az initrd maximális mérete indulásnál 16M, de futás közben
dinamikusan növekedhet 4G-ig. A /dev/root eszköz használja, ami a "devfs"-ben mint memória lemezkép van implementálva.

### Eszközmeghajtók

Az eszközmeghajtók az MMIO-t és az ACPI táblákat képezik le 2^48-4G-nél, ha az támogatott az adott platformon.

Processz memória
----------------

Meg van osztva a taszkok között.

| Virtuális cím   | Láthatóság | Leírás |
| --------------- | ---------- | ----------- |
|    ...          | ...        | további függvénykönyvtárak jöhetnek ezután |
|    c ..         | taszk      | libc adatszegmens, írható/olvasható |
|    b .. c-1     | processz   | libc kódszegmens, csak olvasható |
|    a .. b-1     | taszk      | az ELF futtatható program adatszegmense, írható/olvasható |
|   4M .. a-1     | processz   | az ELF futtatható kódszegmense, csak olvasható |

Mivel az elfek adatszegmense egyből a kódszegmenst követi, ezért ki tudják használni a rip-relatív (pozíció független) címzés
előnyeit. Ez váltakozó processz / taszk leképezést eredményez.

Core memória
------------

A lapok csak felügyeleti szint bitje be van állítva, ami azt jelenti, felhasználói programok nem érhetik el. Ezen kívül globálisan
vannak leképezve minden egyes taszk címterébe, akárcsak a megosztott memória. Két része van, az egyik CPU-nként egyedi.

| Virtuális cím       | Láthatóság | Leírás |
| ------------------- | ---------- | ----------- |
|     -x .. 0         | cpu        | induláskori CPU-nkénti core vermek, 1k mind (lefele növekszik) |
|     -y .. -x        | globális   | [szabad memória bejegyzések](https://gitlab.com/bztsrc/osz/tree/master/src/core/pmm.c) (felfele nő) |
|    -2M .. -y        | globális   | [Core kódszegmens](https://gitlab.com/bztsrc/osz/tree/master/src/core/main.c) |
|   -64M .. -2M-1     | globális   | induló konzol frame buffer leképezés (a [kprintf](https://gitlab.com/bztsrc/osz/tree/master/src/core/kprintf.c) és kpanic használja) |
|  -128M .. -64M-1    | globális   | MMIO terület (a platform specifikus kód használja) |
| -1G+4M .. -128M-1   | globális   | Dinamikus memória (kernel heap, CDYN_ADDRESS) |
|    -1G .. -1G+4M-1  | globális   | CPU Kontroll Blokkok, mind egy lap (CCBS_ADDRESS) |
| -2G+4M .. -2G+6M-1  | cpu        | átmenetileg leképzett cél üzenet sor (LBSS_tmpmq) |
| -2G+2M .. -2G+4M-1  | cpu        | átmenetileg leképzett memória blokk (LBSS_tmpslot) |
| -2G+5p .. -2G+6p-1  | cpu        | átmenetileg leképzett memória lap (LBSS_tmpmap1) |
| -2G+4p .. -2G+5p-1  | cpu        | 4K leképzés ehhez a maghoz (-2G..-2G+2M) |
| -2G+3p .. -2G+4p-1  | cpu        | 2M leképzés ehhez a maghoz (-2G..-1G) |
| -2G+2p .. -2G+3p-1  | cpu        | 1G leképzés ehhez a maghoz (-2G..0) |
| -2G+1p .. -2G+2p-1  | cpu        | a következő riasztandó taszk TCB-je a riasztási sorban (LBSS_tcbalarm) |
|    -2G .. -2G+1p-1  | cpu        | az aktuális mag CPU Kontrol Blokkja (LBSS_ccb) |

A core dynbss lefoglalását a [kalloc()](https://gitlab.com/bztsrc/osz/tree/master/src/core/core.h) végzi.

A core három különböző vermet is használ:
1. induláskori verem (1k, -1M..0): a betöltő állítja be, nem igazán használjuk, csak a címtér leképezés gyökerét adjuk át benne az AP-knak
2. taszkonkénti verem (512b, 3584..4096): a taszk állapotának tárolására (iretq) az ISR-ekben
3. cpu-nkénti verem (3k, -2G+1024..-2G): rendszerhívások használják

