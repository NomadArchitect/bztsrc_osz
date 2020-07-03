OS/Z Indulása
=============

Loader
------

Az OS/Z a [BOOTBOOT](https://gitlab.com/bztsrc/bootboot) protokollt használja a rendszer betöltésére.
A kompatíbilis [loader](https://gitlab.com/bztsrc/osz/tree/master/loader) betöltőt a vas tölti be a POST utolsó lépéseként.
Ez a betöltő minden platformon egységesen inicializálja a hardvert (framebuffert beleértve), betölti az initrd-t és megkeresi benne
a `sys/core`-t. Ha megvan, akkor leképezi a magas címtartományba (-2M..0) és átadja a vezérlést rá.

Core
----

Legelőször tisztáznunk kell, hogy a `core` két részből áll: az egyik platform független, a másik platform függő (lásd
[portolás](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.md)). Az indulás alatt a vezérlés az egyik részről a másikra
ugrál oda-vissza.

A belépési pont, amit a betöltő hív a `_start`, ami az [src/core/(arch)/start.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/start.S)
alatt található. Kikapcsolja a megszakításokat, beállítja a szegmens regisztereket, megnézi, melyik CPU magon fut, és az AP-kat egy
ellenörzőhurokba küldi. Végül a BSP-n meghívja a C-ben íródott `main()`-t az [src/core/main.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/main.c)-ben,
ami platform független kód.

Hogy a gépet elindítsa, a `core` a következő lépéseket hajtja végre:

1. `krpintf_init()` inicializálja az [induló konzolt](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c).
2. `platform_dbginit()` inicilizálja a soros vonali debug konzolt az [src/core/(arch)/(platform)/platform.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c)-ben.
3. `platform_cpu()` inicializálja a CPU specifikus funkciókat az [src/core/(arch)/platform.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S)-ben.
4. `platform_srand()` inicializálja a véletlenszám generátort az [src/core/(arch)/platform.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S)-ben.
5. `env_init` értelmezi a betöltő által átadott [induló környezeti változók](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md)at az [src/core/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c)-ben.
6. `pmm_init()` felkonfigurálja a Fizikai Memória Kezelőt az [src/core/pmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c)-ben.
7. `vmm_init()` felkonfigurálja a Virtuális Memória Kezelőt és a lapcímfordítást az [src/core/(arch)/vmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c)-ben.
8. `drivers_init()` az [src/core/drivers.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c)-ben létrehozza az `IDLE` taszkot (CPU "meghajtó"), inicializálja a megszakításvezérlőt, az IRQ átirányító táblát (IRT), majd végigpásztázza a rendszerbuszokat és a szükséges [eszközmeghajtókat](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.md) betölti `drivers_add()` hívással.
9. következőnek a `core` betölt egy [rendszer szolgáltatás](https://gitlab.com/bztsrc/osz/blob/master/docs/services.en.md)t a `service_add(SRV_FS)` hívással, ami egy normál rendszer szolgáltatás, kivéve, hogy az initrd le van képezve a bss memóriájába.
10. betöltésre kerül a felhasználói felület a `service_add(SRV_UI)` hívással. Ez az első három szolgáltatás (úgymint `IDLE`, `FS`, `UI`) alapvető fontosságú, nem úgy, mint a többi (ezért a nagybetűs név).
11. további `service_add()` hívásokkal nem rendszer kritikus taszkokat tölt be, mint pl `syslog`, `inet`, `sound`, `print` és `init`.
12. a `drivers_start()` hívásával ledobja a felügyeleti jogosultságot, és elindítja a kooperatív ütemezést.
13. ez átkapcsol az FS taszkra, ami elrendezi a memóriáját, hogy mknod() hívásokat tudjon fogadni az eszközmeghajtó taszkoktól.
14. ezután az ütemező, a `sched_pick()` az [src/core/sched.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c)-ben egymás után kiválasztja az eszközmeghajtó és rendszer szolgáltatás taszkokat futásra. Ekkor még nincs preemptív ütemezés.
15. az eszközmeghajtók inicializálják a hardvereiket, és feltöltik az IRT-t.
16. miután minden taszk blokkolódott és az `IDLE` taszk először kerül beütemezésre, a `drivers_ready()` az [src/core/drivers.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c)-ben meghívódik, ami
17. engedélyezi azokat az IRQ-kat, amikhez tartozik bejegyzés az IRT-ben. Engedélyezi az ütemező és a falióra IRQ-it is, és ezzel kezdetét veheti a preemptív ütemezés.
18. most, hogy már minden háttértároló eszközmeghajtó inicializálódott, végezetül a `drivers_start()` küld egy SYS_mountfs üzenetet az `FS` taszknak. Ennél a pontnál a `core` teljesen végzett az indulással.
19. a vezérlés átadódik az [FS taszk](https://gitlab.com/bztsrc/osz/blob/master/src/fs/main.c)nak, ami értelmezi az [fstab](https://gitlab.com/bztsrc/osz/blob/master/etc/sys/etc/fstab) fájlt.
20. miután végzett a háttértárolók felcsatolásával, az `FS` értesíti az összes többi rendszer szolgáltatást, hogy inicializálhatnak.
21. amint ütemezésre kerül, az egyik rendszer szolgáltatás, az `init` taszk betölti és elindítja a felhasználói munkamenet szolgáltatást.

Ha az [induló környezet](https://gitlab.com/bztsrc/osz/blob/master/etc/sys/config)ben vészeseti parancsértelmezőt kértünk, akkor a [bin/sh](https://gitlab.com/bztsrc/osz/blob/master/src/sh/main.c) töltődik be az `init` helyett.

(MEGJEGYZÉS: Ha az OS/Z debug támogatással lett fordítva, és `debug=tests` lett megadva az [induló környezet](https://gitlab.com/bztsrc/osz/blob/master/etc/sys/config)ben,
akkor a `core` a [bin/test](https://gitlab.com/bztsrc/osz/blob/master/src/test/main.c) nevű programot tölti be az `init` helyett, ami különféle modul és funkcionális
rendszerteszteket hajt végre.)

Felhasználói oldal
------------------

Az első igazán 100%-ban felhasználói processzt az [init](https://gitlab.com/bztsrc/osz/blob/master/src/init/main.c) rendszer
szolgáltatás indítja. Ha ez a rendszer legeslegelső indulása, akkor a `bin/identity` meghívódik, ami bekéri a felhasználótól a
számítógép azonosságát (mint például a hosztnév vagy egyéb konfigurációk). Végezetül az `init` elindítja a felhasználói
szolgáltatásokat. A felhasználói szolgáltatások klasszikus UNIX démonok, többek között a felhasználói munkamenet szolgáltatása,
ami a bejelentkezési képernyőt adja.

Felhasználói munkamenet
-----------------------

Az OS/Z egy többfelhasználós operációs rendszer, és mint ilyen, azonosítania kell a felhasználót. Ezt a `logind` démon
végzi. Amint a felhasználó azonossága megállapítást nyert, a megfelelő munkafolyamat szkript annak a bizonyos felhasználónak a
nevében indítható. Amint ez a szkript lefutott, a vezérlés visszakerül a `logind`-hez, ami újból megjeleníti a bejelentkezési
képernyőt.

Ez különbözik a többi POSIX operációs rendszertől, amik általában több, (getty által biztosított) tty-al rendelkeznek, melyek közül
az egyik a grafikus felület (tipikusan a tty8, amit az X kezel). OS/Z alatt a grafikus felület az első (amit az `UI` taszk
kezel), és egy parancsértelmező indításával lehet új tty-t kérni (ezt a UNIX pszeudo-tty-nak hívja, és tipikusan a terminál
emulátorok hozzák létre).

Végjáték
--------

Amikor az `init` kilép, a vezérlés [visszaadódik](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c) a `core`-ra. Az OS/Z
ekkor "Viszlát!"-ot kíván vagy a `kprintf_reboot()` vagy a `kprintf_poweroff()` hívásával (a kilépési státusztól függően), majd a
platform specifikus rész újraindítja illetve kikapcsolja a számítógépet.
