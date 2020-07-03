OS/Z Átültetés új platformokra
==============================

A Hardver Absztrakciós Réteg egy plusz három részre lett osztva:

 * [loader, betöltő](https://gitlab.com/bztsrc/osz/tree/master/loader) (nem része az OS-nek, de szükséges)
 * [core](https://gitlab.com/bztsrc/osz/tree/master/src/core)
 * [libc](https://gitlab.com/bztsrc/osz/tree/master/src/libc)
 * [eszközmeghajtók](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.md)

A betöltő átültetése
--------------------

Az OS/Z indításához szükség van egy [BOOTBOOT](https://gitlab.com/bztsrc/bootboot) kompatíbilis betöltőre az adott platformon.
A betöltő szigorúan nem része az OS-nek, mivel az OS/Z `core` helyett más rendszereket is be tud tölteni, valamint a
`core` szemszögéből egy [általános interfész](https://gitlab.com/bztsrc/osz/blob/master/loader/bootboot.h) révén teljesen
elrejti az indulási környezet sajátosságait.

A core átültetése
-----------------

Ahhoz hogy az OS/Z-t egy új platforma átültessük, annak minimálisan támogatnia kell

 * 64 bites memória címeket alacsony és magas címtérrel, 4k alapú lapozó táblákat használva
 * felügyeleti és felhasználói futtatási módot
 * konfigurálható megszakításvezérlőt (ahol az IRQ-kat külön-külön lehet engedélyezni és letiltani)
 * legalább két időzítőt (egy CPU-nkénti, és egy általános időzítő)

Fontos, hogy különbség van az **architektúra** és a **platform** között. Az előbbi a CPU-t jelenti néhány szükséges kiegészítővel,
mint az MMU vagy az FPU (általában bele vannak építve a CPU-ba, de nem feltétlenül), míg az utóbbi magában foglal mindent,
amit az alaplapra vagy a SoC-ba szereltek (tipikusan a végfelhasználó által nem cserélhető elemek), mint például a
megszakításvezérlők, időzítők, nvram-ok, PCI(e) busz vagy épp a BIOS illetve UEFI firmver stb.

A `core`-t a betöltő `loader` tölti be, ami aztán [elindítja](https://gitlab.com/bztsrc/osz/blob/master/docs/boot.md) az operációs
rendszer többi részét: alacsony szintű inicializálást végez, végignézi a rendszerbuszokat, betölti és inicializálja az
eszközmeghajtókat és a [rendszer szolgáltatások](https://gitlab.com/bztsrc/osz/blob/master/docs/services.md)at. Amint
mindezzel végzett, átadja a vezérlést az `FS` rendszer szolgáltatásnak.

A `core` nagyobb része, az [src/core](https://gitlab.com/bztsrc/osz/blob/master/src/core) plaform független és C-ben lett írva.
A hardver függő rész vagy C vagy Assembly nyelven íródott, és az [src/core/(arch)/(platform)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64)
alatt található. Az OS/Z egyetlen programja a `core`, ami felügyeleti jogosultságokkal fut (ring 0 x86_64-on, és EL1 AArch64-on),
és ezért kis méretre korlátozott. Jelenleg 100k körüli (amiből 32k a UNICODE rendszer betűtípus),
és a debugert belefordítva sem éri el a 200k-t. A mérete semmilyen körülmények között sem lépheti át az 512k-t.

Habár az OS/Z mikro-kernel, a `core` egy kicsit többet tesz a többi tipikus mikro-kernelnél, mint pl a Minix. Példának okáért
nem delegálja a memóriakezelést felhasználói szintre. A cél az volt, hogy gyors maradjon, és hogy egy minimális, de jól
definiált interfészt nyújtson minden platformon. Ezért a `core` minimalista szemlélettel rejti el a platform sajátosságait:
minden, ami ahhoz kell, hogy legyenek független, megszakítható, egymás között kommunikálni tudó taszkjaink, a `core`-ba került.
Minden más (például az eszközmeghajtók) ki lettek tolva felhasználói szintű, külön binárisokba, ahogy azt a mikro-kernel
architektúra elvárja.

Minden egyes forrásfájlnak (C-nek és Assembly-nek egyaránt) a következő include-ok egyikével kell kezdődnie:

```
#include <core.h>       /* ha teljesen platform független */
#include <arch.h>       /* ha architektúra függő részt igényel */
#include <platform.h>   /* ha platform függő részt igényel */
```

### CPU Kezelés

Eléggé architektúra független, csak egyetlen struct-ot kell átültetni az __src/core/(arch)/ccb.h__-ban. Ez, hogy kihasználja az
x86_64 hardver nyújtotta lehetőségeket, illeszkedik a TSS szegmensre. Ellenben AArch64-on csupán a struktúra általános mezőit
tartalmazza.

CPU függő libc függvények az __src/core/(arch)/libc.S__ fájlban kaptak helyet, elsősorban az atomikus `lockacquire()` és
`lockrelease()`. Minden más függvénynek van ANSI C implementációja. Kivételek kezelését a `fault_intr()` végzi, ami az
__src/core/(arch)/fault.c__ fájlban található.

Az alap inicializálást az __src/core/(arch)/start.S__-ben, Assemblyben kell megírni, ami végül a `main()` függvényt hívja
az indító processzoron (bootstrap processor). Részletes ellenőrzések - és amennyiben szükséges - a funkcióspecifikus inicializálás
helye a `platform_cpu()`, ami akkor hívódik, amikorra már a indító konzol elérhető, lásd lentebb.

### Virtuális Memória Kezelő

Az __src/core/(arch)/vmm.h, vmm.c__ fájlokat kell átírni. A kód java része architektúra független, így ezekben nincs más, mint
a lapfordítási leképező táblák definíciói és az induló adatokkal való feltöltés. Habár a kód PAGESIZE definícióra hivatkozik,
vannak számítások, amik feltételezik, hogy a mutató mérete 8 bájt (mivel az OS/Z 64 bites) és a lapkeret mérete pedig 4096 bájt,
ezért maradjunk ennél. A lapfordításhoz a memória négy részre oszlik: taszk lokális, megosztott globális, CPU-nkét leképzett
(felügyeleti szint), globálisan leképzett (felügyeleti szint), a részletekért lásd a közös src/core/vmm.h definícióit.

### Megszakítás Vezérlő és Időzítők

Az __src/core/(arch)/(platform)/intr.c__ alatt található, és a következő függvényeket kell megvalósítania:

 * intr_init() - a vezérlő inicializálása és az időzítők konfigurálása (lásd alább)
 * intr_enable(irq) - egy IRQ vonal engedélyezése
 * intr_disable(irq) - egy IRQ vonal letiltása
 * intr_nextsched(nsec) - ütemező megszakítás visszaigazolása, és új megszakítás konfigurálása
 * clock_ack() - falióra megszakítás visszaigazolása

Két különleges megszakításkiszolgáló van, egy az ütemezőnek és egy a faliórának (`sched_intr()` és `clock_intr()`). Az első
egy CPU magonkénti, egyszeri IRQ időzítő, és ez az egyetlen megszakítás, ami minden CPU magon meghívódhat, akár egy időben. A
másik egy periodikus időzítő (amit kiválthat egy egyszeri időzítő, ha a clock_ack() beállít egy új egyszeri IRQ-t, de ezáltal
veszít a falióra a pontosságából). Minden más megszakításnak az általános `drivers_intr()` függvényt kell hívnia, ha a megszakítás
a vezérlőben keletkezett, és a `fault_intr()` függvényt, ha a CPU generálta (nullával osztás, szegmenshiba, laphiba stb.)

### Platform funkciók

Ezek vagy az __src/core/(arch)/plaform.S__ vagy az __src/core/(arch)/(platform)/platform.c__ alatt találhatók, attól függően, hogy
az adott megvalósítás milyen természetű (például a platform_srand() az RDRAND utasítást használja x86-on és ezért az arch alatt a
helye, míg AArch64 alatt egy MMIO eszközt, ezért a platform alá került. A platform_awakecpu()-nál pont fordítva van, ott az AArch64
rendelkezik utasítással, míg az x86_64 esetében kell MMIO). Hogy C-ben vagy Assemblyben vannak-e megírva ezek a fájlok,
az csakis rajtad múlik, a fordítókörnyezet mindkettőt detektálja és támogatja.

 * platform_cpu() - ellenőrzés, detektálás, és a CPU specifikus funkciók inicializálása
 * platform_srand() - a véletlenszám generátor beállítása egy kellően kiismerhetetlen kezdő értékre
 * platform_env() - platform függő környezeti változó alapértékek beállítása
 * platform_parse() - az indító környezet (environment)-ben platform függő kulcsok értelmezése
 * platform_poweroff() - a számítógép kikapcsolása
 * platform_reset() - a számítógép újraindítása
 * platform_halt() - futtatás felfüggesztése, a számítógép lefagyasztása
 * platform_idle() - alacsony fogyasztású üzemmódba kapcsolja a CPU-t, amíg egy megszakítás nem érkezik
 * platform_awakecpu(id) - felébreszti a CPU-t az idle ciklusból
 * platform_dbginit() - a soros vonali debug konzol inicializálása, ha debug támogatással lett fordítva
 * platform_dbgputc() - egy 8 bites ASCII karakter küldése a debug konzolra, ha debug támogatással lett fordítva
 * platform_waitkey() - várakozás leütésre és karakter (nem gombkód) visszaadása billentyűzetről vagy debug konzolról (ha debug
    támogatással lett fordítva. Ezt az induló konzol képernyő szkrollozója hívja, valamint a beépített debugger használja)

### Beépített debugger (opcionális)

Az __src/core/(arch)/dbg.c__ és az __src/core/(arch)/disasm.h__ fájlokat kell megírni. Az általános dbg.c elején található
az implementálandó függvények listája (nem sok). A diszasszemblerhez pedig egy
[szövegfájlba kell felvinni](https://gitlab.com/bztsrc/udisasm) az adott processzor utasításkészletét.

Eszközmeghajtók átültetése
--------------------------

Azok a meghajtók, amiket nem éri meg külön taszkba rakni (csak megszakításvezérlők, időzítők, CPU és memória kezelés) a
`core`-ban vannak beépítve, minden más önálló eszközmeghajtó taszkkal rendelkezik.

Az eszközmeghajtókat meg kell írni minden platformra. Néhány [meghajtó C-ben van](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-develop.md)
és használható több platformon is, de valószínűbb, hogy Assembly betétekkel van tele vagy platform specifikus C implementáció. Hogy
ezen segítsen, a fordítási környezet egy [platforms](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.md) nevű fájlt keres,
ami tartalmazza az összes platformot, amin a meghajtó működik. Hasonlóan ehhez, van egy `devices` fájl, a meghajtott eszköz(ök)
azonosítóival és rendszerbusz címeivel, amit pedig a `core` használ arra, hogy az eszközmeghajtót beazonosítsa, mikor induláskor
a rendszerbuszokat pásztázza.

Az eszközmeghajtók [magasabb prioritási szint](https://gitlab.com/bztsrc/osz/blob/master/docs/scheduler.md)en futnak, mint a többi
felhasználói taszk, és elérhetnek bizonyos speciális `libc` funkciókat, mint például a
[core_regirq()](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/include/driver.h). Ezen kívül hozzáférnek a B/K címtérhez
(ami vagy speciális utasításokat, mint az inb/outb jelent, vagy MMIO elérést, attól függően melyik támogatott az architektúrán).

Az eszközmeghajtók esetében megengedett a bináris terjesztés, nem kötelező a nyílt forráskód. Ez esetben kell lennie egy
előre lefordított "(ARCH).so" fájlnak.

A libc átültetése
-----------------

Ezt a megosztott függvénykönyvtárat történelmi okokból hívják `libc`-nek, igazából ez az igazi platform független interfész
minden egyes OS/Z funkcióhoz, ezért `libosz`-nek kéne hívni. Függvényeket biztosít a felhasználói szintű szolgáltatások,
függvénykönyvtárak és alkalmazások számára éppúgy, mint az eszközmeghajtók számára. Leginkább C, de bizonyos részeit muszáj volt
Assemblyben írni (mint például a memcpy()-t a gyorsaság miatt, vagy a felügyeleti szinten futó funkciók elérése a `core`-ban).
Az üzenetsor minden részletét elrejti, de a teljesség kedvéért biztosít platform független [felhasználi szintű üzenetkezelési](https://gitlab.com/bztsrc/osz/blob/master/docs/messages.md#low-level-user-library-aka-syscalls) lehetőséget is.

Minden OS/Z-re fordított programot dinamikusan kell linkelni ezzel a `libc`-vel, és nem szabad más, alacsonyabb szintű
megvalósításokat (pl syscall/svc interfész) közvetlenül használni. A `libc` szintjén az OS/Z teljesen platform független.

Renszer szolgáltatások átültetése
---------------------------------

Ezek platform függetlenek, kizárólag C-ben íródtak, így a fordításuk új plaforma nem szabadna, hogy bármi gondot okozzon.

Megosztott függvénykönyvtárak és alkalmazások átültetése
--------------------------------------------------------

Mint már említésre került, a `libc` nemcsak a szokásos C függvényeket nyújtja, de egységes felület is biztosít az OS/Z minden
rendszer szolgáltatásának elérésére. Ez megkönnyíti az új programok írását, azonban megnehezíti a már meglévő, más operációs
rendszerre íródott programok átültetését. Ezekben feltételes `#ifdef _OS_Z_` előfordító blokkokat kell alkalmazni. Ez ellen nincs
gyógyszer, mivel az [OS/Z szándékosan nem POSIX](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.md). Habár mindent megtettem,
hogy minnél jobban hasonlítson a POSIX-ra, ezáltal megkönnyívte az átültetést, de attól ez még az OS/Z saját interfésze. Ha teljes
POSIX kompatibilitás a cél, akkor az átültetett `musl` csomagot kell használni, de ezzel elesel nagyon sok OS/Z bővítménytől.

Az OS/Z úgy lett megtervezve, hogy megosztott függvénykönyvtár szintjén minden alkalmazás binárisan kompatíbilis egy adott
architektúrán, és forrás kompatíbilis minden architektúrán és minden platformon. Emiatt a függvénykönyvtárakba nem szabad,
az alkalmazásokba pedig tilos Assembly vagy plaform specifikus C kódot rakni. Ha ez mindenképp szükséges és eszközmeghajtóval
nem megoldható, akkor a kérdéses kódot egy külön függvénykönyvtárba kell rakni, amiben ugyanazt az interfészt kell használni
minden platformhoz, és minden platformon elérhetővé kell tenni. A `libui` pixbuf függvénykönyvtára jó példa erre, ami nagyon
gyors, architektúrára kihegyezett blittert igényel.

Mivel az OS/Z a `libc` szintjén válik platform függetlenné, ezért a modul és funkcionális [tesztek](https://gitlab.com/bztsrc/osz/blob/master/src/test)
kizárólag erre és az efölötti szintekre íródtak, alacsonyabbakra nem. Ez azt jelenti, hogy a tesztek egy új architektúrára
egy az egyben lefordíthatók, és ha hiba nélkül lefutnak, akkor a core és libc átültetése sikeres volt.

Bájtkód átültetése
------------------

Az OS/Z támogatja platformfüggetlen [bájtkód](https://gitlab.com/bztsrc/osz/blob/master/docs/bytecode.md) futtatását. Ehhez
mindössze a [wasmvm](https://gitlab.com/bztsrc/osz/blob/master/usr/wasmvm) alkalmazást kell portolni.
