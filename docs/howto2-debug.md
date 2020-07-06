OS/Z - Hogyan Sorozat #2 - Debuggolás
=====================================

Előszó
------

Ezelőtt szó volt arról, hogyan [teszteljük az OS/Z-t](https://gitlab.com/bztsrc/osz/blob/master/docs/howto1-testing.md) virtuális
gépen. Ebben a részben megismerkedünk azzal, hogy hogyan debuggolhatjuk. Egy soros vonali RS-232 kábellel könnyedén összeköthetjük
a gépünket egy másikkal, és máris igazi vason is debuggolhatunk.

De legelőször, engedélyezni kell a debuggolást a [Config](https://gitlab.com/bztsrc/osz/blob/master/Config) fájlban `DEBUG = 1`
megadásával, majd újrafordítani.

Debug üzenetek
--------------

Erre való a `debug` opció az [indítási környezet](https://gitlab.com/bztsrc/osz/blob/master/etc/config) fájlban. A lehetséges
flageket megtalálod a [leírásban](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md).
Azonban a `DEBUG = 1` megadásával az összes fontos információ bekerül a syslogba, ami a soros vonalra is kiküldésre kerül.

Debuggolás GDB-vel
------------------

Virtuális gép indítása készenléti módban

```sh
make debug
```

majd egy másik terminálban ki kell adni a

```sh
make gdb
```

parancsot a [GDB](https://www.sourceware.org/gdb/) indításához és az OS/Z vmhez csatlakoztatásához. Üsd le a <kbd>c</kbd> majd
<kbd>Enter</kbd> gombokat a szimuláció elindításához. Ebben az ablakban bármikor leütöd a <kbd>Ctrl</kbd>+<kbd>C</kbd>-t, akkor
gdb parancssort kapsz.

### A pid lekérése

Nem lehetséges, legalábbis nem találtam megbízható módot. De dumpolhatod a qword-öt a 640-es címen.

Debuggolás bochs-al
-------------------

Indítás

```sh
make testb
```

Üsd le a <kbd>c</kbd> majd <kbd>Enter</kbd> gombokat a szimuláció indításához. Később <kbd>Ctrl</kbd>+<kbd>C</kbd> leütésére
a bochs debug konzol jön elő.

### A pid lekérése

Hogy az aktuálisan futó programot megtudd, írd be `page 0`.

<pre>
&lt;bochs:2> page 0
PML4: 0x0000000000033007    ps         a pcd pwt U W P
PDPE: 0x0000000000034007    ps         a pcd pwt U W P
 PDE: 0x8000000000035007 XD ps         a pcd pwt U W P
 PTE: 0x8000000000031005 XD    g pat d a pcd pwt U R P
linear page 0x0000000000000000 maps to physical page 0x<i><b>000000031</b></i>000
&lt;bochs:3>
</pre>

És a lista végén a legutolsó számból hagyd el az utolsó három 0-át, az lesz a pid. A fenti példában `0x31`.

### Pid ellenőrzése

Hogy érvényes-e, azt leellenőrizheted:

```
<bochs:3> xp /5bc 0x31000
[bochs]:
0x0000000000031000 <bogus+       0>:  T    A    S    K   \2
<bochs:4>
```

Itt látható, hogy a mágikus `'TASK'` azonosítóval kezdődik, tehát ez egy Taszk Kontroll Blokk. A szám megmondja, hogy
2-es prioritási szinten fut.

Az első 8 bájtot leszámítva a TCB mezői architektúrafüggőek, mivel CPU állapotot is tartalmaz. Minden architektúrának van
egy struktúradefiníciója a platform könyvtárban, pl. [src/core/(platform)/tcb.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/tcb.h).

Debuggolás VirtualBox-al
------------------------

Ez eléggé nehézkes, és nem is igazán dokumentált, de a VirtualBox-ban van beépített debugger, ami előcsalogatható környezeti
változókkal.

### A pid lekérése

Hogy az aktuálisan futó programot megtudd, írd be `dphg 0`.
Hasonlóan a bochs-hoz, a lapcímfordítási fa utolsó száma tartalmazza, a legutolsó három 0 nélkül.

Debuggolás az OS/Z Debuggerrel
------------------------------

<img align="left" style="padding-right:10px;" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg2.png" alt="OS/Z Beépített Debugger">
Bármikor ha <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Esc</kbd>-t ütsz a virtuális gépben, vagy soros vonalon egy <kbd>Break</kbd> szignált
küldesz, a beépített debugger meghívódik.

Ha beállítod a `debug=prompt` opciót az [indulási környezet](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md)ben, akkor
a bootolás meg fog állni a legelső taszkkapcsolás előtt.

Továbbá a kódodból is meghívhatod, ha a kívánt helyre a következő parancsot beszúrod:

```c
/* debugger hívása C kódból */
breakpoint;
```

### Felületek

A beépített debugger direktben kezeli a frémbuffert, és alacsony szinten billentyűleütéseket fogad (nem használ eszközmeghajtót
hozzá).

Továbbá a soros vonalat is beállítja 115200,8N1 paraméterekkel és oda is ír, és onnan is várja a parancsokat. Alapból feltételezi,
hogy a soros vonalon egy sornyomtató található. A VT100-as videó mód engedélyezéséhez ki kell adni a következő debugger parancsot
(text user interface):
```
dbg> tui
```


<img align="left" style="padding-right:10px;" height="180" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg3.png" alt="OS/Z Beépített Debugger" title="OS/Z Beépített Debugger">
<img align="left" style="padding-right:10px;" height="180" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbgA.png" alt="OS/Z Beépített Debugger Soros Konzol" title="OS/Z Beépített Debugger Soros Konzol">
<img height="180" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbgB.png" alt="OS/Z Beépített Debugger VT interfész" title="OS/Z Beépített Debugger VT interfész">

Bármikor kaphatsz segítséget az <kbd>F1</kbd> lenyomásával vagy a `help` parancs kiadásával.

### A pid ellenőrzése

A képernyő jobb alsó sarkában van feltüntetve.

Soroy konzol módban a `dbg>` prompt előtt látható.

A debuggeren belül a `dbg> p pid` paranccsal vagy a <kbd>&larr;</kbd> és <kbd>&rarr;</kbd> gombokkal lehet taszkot váltani.

### Panelek (tabok)

| Név  | Leírás |
| ---- | ----------- |
| Code | <img align="left" style="padding-right:10px;" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg3.png">Regiszterek dumpolása és kód visszafejtés |
| Data | <img align="left" style="padding-right:10px;" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg4.png">Memória vagy verem dumpolása |
| [Messages](https://gitlab.com/bztsrc/osz/blob/master/docs/messages.md) | <img align="left" style="padding-right:10px;" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg5.png">Taszk üzenetsorának dumpolása |
| [TCB](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/tcb.h)  | <img align="left" style="padding-right:10px;" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg6.png">Az aktuális Taszk Kontroll Blokk dumpolása |
| [CCB](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ccb.h) | <img align="left" style="padding-right:10px" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg7.png">A CPU Kontroll Blokk dumpja (taszk prioritási sorok) |
| [RAM](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.h) | <img align="left" style="padding-right:10px;" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg8.png">A fizikai memóriakezelő adatainak dumpolása |
| [Sysinfo](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c) | <img align="left" style="padding-right:10px;" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszdbg9.png">Rendszer információ és korai syslog buffer |
| [Environment](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md) | Indulási környezet konfiguráció dumpolása |

### Gyorsbillentyűk

| Gomb  | Leírás      |
| ----- | ----------- |
| F1    | súgó |
| Esc   | (üres parancs esetén) kilépés a debuggerből, folytatás (újraindítás pánik esetén) |
| Esc   | (ha van parancs) parancs törlése |
| Tab   | váltás panelek között |
| Enter | (üres parancs esetén) végrehajtás léptetés |
| Enter | (ha van parancs) parancs végrehajtása |
| Bal   | (üres parancs esetén) váltás az előző taszkra |
| Bal   | (ha van parancs) kurzor mozgatása |
| Jobb  | (üres parancs esetén) váltás a következő taszkra |
| Jobb  | (ha vanparancs) kurzor mozgatása |
| Fel   | ha <kbd>Shift</kbd>-el, akkor az előző parancs előhozása |
| Fel   | felfele görgetés |
| Le    | ha <kbd>Shift</kbd>-el, akkor a következő parancs előhozása |
| Le    | lefele görgetés |
| PgUp  | data panelen, egy lapnyit görget felfelé |
| PgUp  | adat panelen <kbd>Shift</kbd>-el, egy blokknyit görget felfelé |
| PgDn  | adat panelen, egy lapnyit görget lefelé |
| PgDn  | adat panelen <kbd>Shift</kbd>-el, egy blokknyit görget lefelé |

### Debugger parancsok

Minden parancsot elég addig kiírni, míg egyértelművé nem válnak, a legtöbb esetben ez egy betűt jelent. A listában nagybetűket
használtam ennek jelölésére.

| Parancs    | Leírás |
| ---------- | ----------- |
| `Step`     | végrehajtás léptetés |
| `Continue` | végrehajtás folytatása |
| `REset`, `REboot` | újraindítás |
| `HAlt`     | számítógép kikapcsolása |
| `Help`     | súgó megjelenítése |
| `Pid X`    | váltás X taszkra |
| `Prev`     | váltás az előző taszkra |
| `Next`     | váltás a következő taszkra |
| `Tcb`      | akruális Taszk Kontroll Blokk elemzése |
| `TUi`      | videó terminál mód ki/bekapcsolása soros vonalon (Text User Interface) |
| `Messages` | üzenetsor elemzése |
| `All, CCb`   | az összes ütemezési sor és a CPU Kontroll Blokk vizsgálata |
| `Ram`      | RAM foglalás vizsgálata |
| `Instruction X`, `Disasm X` | utasítás diszasszemblálás (vagy váltás bájtdump és mnemonik mód között) |
| `eXamine [/b1w2d4q8s] X`   | X memória cím vizsgálata byte, word, dword, qword vagy stack (verem) formátumban |
| `Break [/b12d4q8rwx] X`   | brakpoint-ok listázása vagy beállítása X címen byte, word, dword, qword hosszon |
| `SYsinfo` `Log` | rendszerinformáció és korai syslog mutatása |
| `Full`     | teljes képernyős mód kapcsolása, a fő információ kerül a teljes képernyőre |

A diszasszemblálás parancsnak van paramétere (amiben a 0x előtag elhagyható). Ez csak a megjelenítő ablakot mozgatja,
a futást nem befolyásolja. Lehetséges értékek:
 * (üres) ha nincs megadva, akkor bájt / mnemonik váltás
 * `+X`, `-X` utasításszámlálóhoz képesti relatív cím
 * `X` abszolút cím
 * `symbol+X` szimbólumhoz relatív cím

A memória vizsgálatnak két paramétere lehet, egy opció és egy cím. A cím ugyanolyan, mint fentebb írtuk, annyi eltéréssel,
hogy az adat ablakot mozgatja.
 * (üres) visszatérés az eredeti verem tetejére
 * `+X`, `-X` aktuális címhez képest relatív cím
 * `X` abszolút cím
 * `symbol+X` szimbólumhoz relatív cím

A memória vizsgálatnak van egy párja, az `xp`, ami virtuális (címtér) címek helyett fizikai (RAM) címeket vár.

A megszakítás paramétere pont ugyanilyen, kivéve hogy:
 * (üres) breakpoint-ok listázása
 * w jelző írás megszakítást definiál (a word mérethez a /2 opciót kell használni)

A második paraméter, a jelző opció a következő lehet:
 * 1,b - 16 bájt egy sorban (byte)
 * 2,w - 8 szó egy sorban (word)
 * 4,d - 4 dupla szó egy sorban (dword)
 * 8,q - 2 kvuadripla szó egy sorban (quad)
 * s   - egy kvuadripla szó egy sorban, szimbólumfeloldással (verem nézet)
 * r   - olvasáskor megszakítás (break esetén)
 * w   - íráskor megszakítás (break esetén)
 * p   - io port eléréskor megszakítás (break esetén)
 * x   - futtatáskor megszakítás (break esetén, ez az alapértelmezett)

Az ELF szimbólumokon túl a következők is használhatóak:
 * tcb   - Taszk Kontroll Blokk
 * mq    - üzenetsor
 * stack - lokális verem kezdete
 * text  - kódszegmens kezdete, lokális verem vége
 * bss   - dinamikusan allokált memória kezdete
 * sbss/shared - megosztott memória kezdete
 * core  - kernel kódszegmense
 * buff  - buffer terület
 * bt    - verem visszakövetés kezdete (backtrace stack)
 * bármilyen általános célú regiszter neve (plusz cr2, cr3 x86_64 esetén)

#### Példák

```
p               előző taszkra kapcsolás
p 29            a 29-es pidű taszkra kapcsolás
i               bájtdump és mnemonik megjelenítés között kapcsolgatás
i intr_1+3      egy funkción belüli utasítás megtekintése
i +7F           diszasszembláló ablak mozgatása előre 127 bájttal
x 1234          a 0x1234 címen lévő memória dumpolása
x /q            dumpolás quad egységekben
x /s rsp        verem kiiratása szimbólumfeloldással
/b              megjelenítési egységek bájtra kapcsolása
x /4 rsi        regiszter által mutatott címen dwordök dumpolása
i               vissza a diszasszembláló tabra
b /qw tcb+F0    írás figyelése quad hosszan 000F0h címen
b /bp 60        a billyentyűport monitorozása
s               egy utasítás lépésenkénti végrehajtása, léptetés
sy              sysinfo megjelenítése
f               teljesképernyős mód kapcsolása, csak a logok megjelenítése a sysinfo-n
```

A következő epizód arról fog szólni, hogyan [fejlesszünk](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-driver.md) OS/Z alá.
