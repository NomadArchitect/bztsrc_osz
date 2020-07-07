OS/Z - Hogyan Sorozat #4 - Szolgáltatás fejlesztése
===================================================

Előszó
------

Ezelőtt megnéztük, hogyan kell [eszközmeghajtót](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-driver.md) készíteni OS/Z
alá, azelőtt pedig hogy hogyan kell [debuggolni](https://gitlab.com/bztsrc/osz/blob/master/docs/howto2-debug.md) virtuális gépen.

Most az van soron, hogy hogyan fejlesszünk szolgáltatást (service, daemon), amik a háttérben futnak, és alkalmazásokat szolgálnak
ki. Ha végfelhasználók számára szeretnél programot írni, akkor nyugodtan [ugord át](https://gitlab.com/bztsrc/osz/blob/master/docs/howto5-app.md).

Fejlécek
--------

Minden OS/Z alá írt alkalmazást a következő include-al kell kezdeni:

```c
#include <osZ.h>
```

Ez behúz mindent, amire szükség lehet, beleértve a sztandard C függvénykönyvtárat is. Habár mindent elkövettem azért, hogy
POSIX-szerű legyen, és így egyszerűbb legyen a meglévő alkalmazások átültetése, az OS/Z szándékosan
[nem teljesen POSIX kompatíbilis](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.md).
Az [OS/Z saját interfész](https://gitlab.com/bztsrc/osz/blob/master/docs/refusr.md)-el rendelkezik.

Szolgáltatások
--------------

A szolgáltatások nagyon hasonlítanak a felhasználói programokra, azonban egy eseményértelmező ciklust tartalmaz a magjuk, mivel
tipikusan eseményvezéreltek. A `main()` függvényük inicializál, majd meghívja az `mq_dispatch()` függvényt. Hiba esetén az exit()-nek
egy jól definiált kóddal kell kilépnie, amik a [sysexits.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/sysexits.h)
fejlécben vannak definiálva.

```c
#include <osZ.h>

public int main(int argc, char **argv)
{
    /* Inicializálás. Hiba esetén exit()-et kell hívni */

    mq_dispatch();
}
```
Ahhoz, hogy ez működjön, minden eseménytípushoz egy lekezelő függvényt kell implementálni. A mellékelt
[tools/elftool.c](https://gitlab.com/bztsrc/osz/blob/master/tools/elftool.c) képes egy szolgáltatás futtathatóját analizálni,
majd a benne definiált függvények alapján létrehozni egy fejlécet a neki küldhető üzenettípusokról. A hívók (vagy a szolgáltatás
függvénykönyvtára) ez behúzhatja és használhatja.

Ha valaki nagyobb rugalmasságot szeretne, akkor használható egy végtelen eseménylekérő ciklus is:

```c
#include <osZ.h>

public int main(int argc, char **argv)
{
    /* Inicializálás. Hiba esetén exit()-et kell hívni */

    while(true) {
        msg_t *msg = mq_recv();
        /* esemény lekezelése */
    }
}
```

### Konfiguráció

A konfigurációt az /usr/(csomagnév)/etc/ alatt fájlokban kell tárolni, és onnan kell beolvasni.

Függvénykönyvtárak
------------------

Egy szolgáltatás csomag több, mint valószínű, hogy függvénykönyvtárakat is biztosít a felhasználói alkalmazások számára,
melyekkel üzeneteket lehet küldeni a háttérben futó szolgáltatásnak. Az is lehetséges, hogy egy szolgáltatás csomag egyáltalán
nem tartalmaz háttérben futó alkalmazást, csakis függvénykönyvtárakat. Ezek helye a csomagban a 'lib/' mappa.

Csomagok
--------

Amint sikeresen lefordítottad és letesztelted a szolgáltatásod, minden bizonnyal el akarod juttatni a felhasználóidhoz.
Ehhez létre kell hozni egy csomagot, ami egy ZIP fájl. Csakis [relatív elérési utak](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.md#usr)
lehetnek benne, pl. 'bin/', 'etc/', 'lib/', stb. Másold fel az archívumot egy webszerverre, add hozzá a metaadatokat
a távoli 'packages' fájlhoz és kész! Minden gép, amin fel van konfigurálva a repód, egyből [telepít](https://gitlab.com/bztsrc/osz/blob/master/docs/howto7-install.md)eni
tudja a szolgáltatásodat, vagy frissíteni a legújabb verzióra.

Legközelebb a [felhasználói alkalmazás](https://gitlab.com/bztsrc/osz/blob/master/docs/howto5-app.md)ok fejlesztéséről lesz szó.
