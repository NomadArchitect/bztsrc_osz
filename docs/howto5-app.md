OS/Z - Hogyan Sorozat #3 - Alkalmazás fejlesztése
=================================================

Előszó
------

Legutóbb arról beszéltünk, hogyan kell [szolgáltatás](https://gitlab.com/bztsrc/osz/blob/master/docs/howto4-service.md)t fejleszteni
OS/Z alá.

A [következő oktatóanyag](https://gitlab.com/bztsrc/osz/blob/master/docs/howto6-rescueshell.md) üzemeltetői szemléletű, és a
vészhelyzeti parancsértelmezőt tárgyalja.

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

Konzolos alkalmazások
---------------------

A kötelező Helló Világ példa nagyon hasonlít egy bármelyik UNIX rendszeren lévőre:

```c
#include <osZ.h>

public int main(int argc, char **argv)
{
    printf( "Helló Világ!\n" );
    return 0;
}
```

Ablakos alkalmazások
--------------------

A felhasználói felület definíciói az ui.h-ban vannak, amiket az osZ.h már behúzott. Azonban az ablakos alkalmazások esetében
szükség van a `libui`-val való szerkesztésre, ezért add hozzá a `-Lui` kapcsolót a gcc-hez. Az ablakos alkalmazások main()
függvénye pont olyan, mint a többié. Nincs külön prototípus, de a máküdésben van eltérés. Inicializálnak (például ablakot nyitnak),
majd egy `mq_recv()` diszpécsert tartalmaznak, mivel az ablakos alkalmazások elsősorban esemény-vezérelt természetűek.

```c
#include <osZ.h>

public int main(int argc, char **argv)
{
    /* inicializálás */
    win_t *win;
    win = ui_openwindow( "pixbuf", 10, 10, 640, 480, 0 );

    ui_setcursor(UIC_progress);
    load_a_lot_of_data();
    ui_setcursor(UIC_default);

    /* események fogadása */
    while(true) {
        /* blokkol, ha nincs esemény */
        msg_t *msg = mq_recv();

        /* milyen feladatot kaptunk? */
        switch( EVT_FUNC(msg->evt) ) {

            case UI_keyevent:
                ui_keyevent_t *keyevt = msg;
                ...
                break;

            case UI_ptrevent: ...
        }
    }
}
```

Az alap ablak és eseménykezelést biztosítja a `libui`. Ez egy pixelbuffert jelent, amire rajzolhatsz, de semmi többet. Azok a
függvények, melyek a buffert kezelik (rajzprimitívek, gombok, képek stb.) csak minimálisan vannak benne, az összetettebb
funkciókhoz (például HTML megjelenítés) külön függvénykönyvtárakra van szükség.

Csomagok
--------

Amint sikeresen lefordítottad és letesztelted az alkalmazásod, minden bizonnyal el akarod juttatni a felhasználóidhoz.
Ehhez létre kell hozni egy csomagot, ami egy ZIP fájl. Csakis [relatív elérési utak](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.md#usr)
lehetnek benne, pl. 'bin/', 'etc/', 'share/', stb. Másold fel az archívumot egy webszerverre, add hozzá a metaadatokat
a távoli 'packages' fájlhoz és kész! Minden gép, amin fel van konfigurálva a repód, egyből [telepít](https://gitlab.com/bztsrc/osz/blob/master/docs/howto7-install.md)eni
tudja az alkalmazásodat, vagy frissíteni a legújabb verzióra.

A [következő oktatóanyag](https://gitlab.com/bztsrc/osz/blob/master/docs/howto6-rescueshell.md) a vészhelyzeti parancssorról szól.
