OS/Z - Hogyan Sorozat #3 - Eszközmeghajtó fejlesztése
=====================================================

Előszó
------

Legutóbb tisztáztuk, hogyan kell [debuggolni OS/Z alatt](https://gitlab.com/bztsrc/osz/blob/master/docs/howto2-debug.md) virtuális
gépen. Ennek ismeretében már nyugodtan nekiállhatunk fejleszteni.

Eszközmeghajtóra azért van szükség, hogy alacsonyszintű hozzáférést biztosítson a hardveres eszközökhöz, amit aztán egy egységes
interfésszel elfed. Ez csak kevesek számára érdekes, nyugodtan ugord át és lépj tovább a [szolgáltatások fejlesztése](https://gitlab.com/bztsrc/osz/blob/master/docs/howto4-service.md)
anyagra.

Fejlécek
--------

Minden OS/Z alá írt meghajtóprogramot a következő include-okkal kell kezdeni:

```c
#include <osZ.h>
#include <driver.h>
```

Az első behúz mindent, amire szükség lehet, beleértve a sztandard C függvénykönyvtárat is. Habár mindent elkövettem azért, hogy
POSIX-szerű legyen, és így egyszerűbb legyen a meglévő alkalmazások átültetése, az OS/Z szándékosan
[nem teljesen POSIX kompatíbilis](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.md).
Az [OS/Z saját interfész](https://gitlab.com/bztsrc/osz/blob/master/docs/refusr.md)-el rendelkezik.

Ezen túlmenően az eszközmeghajtóknak szüksége van az src/drivers/include alatti fejlécekre. Ezeket a második include húzza be.

Eszközmeghajtók
---------------

Bár ezek is felhasználói szinten futnak, ezek nem hagyományos alkalmazások. A fő programjuk egy [eseménykezelő](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c)
végtelen ciklus. A meghajtóprogramok ehhez nyújtanak függvényeket, és ezzel kerülnek összeszerkesztésre.
A meghajtóprogramok [kategorizálva](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/README.md) vannak, mindnek megvan a
megfelelő alkönyvtára. Hívhatnak bizonyos privilegizált `libc` funkciókat, és speciális eszközfájlokat hozhatnak létre a /dev alatt.

```c
#include <osZ.h>
#include <driver.h>

public void drv_init()
{
    /* Inicializálás. Ha probléma adódik, akkor exit(EX_UNAVAILABLE) hívással ki kell lépni. */
    /* megszakításlekezelő rgisztrálása drv_regirq(irq) hívással. */
    /* periodikus hívás kérése drv_regtmr() hívással. */
    /* ha van értelme, eszközfájlok létrehozása mknod() hívás(ok)al. */
}

public void drv_irq(uint16_t irq, uint64_t ticks)
{
    /* megszakítás lekezelése */
}

public void drv_ioctl(dev_t device, uint64_t fmt,...)
{
    /* eszközkonfiguráló kérés lekezelése */
}

public void drv_read(dev_t device)
{
    /* olvasás az eszközről */
}

/* ... stb. ... */
```

### Konfiguráció

Ha konfigurációs beállításokra van szüksége egy eszközmeghajtónak, akkor nem használhat fájlokat. Egész egyszerűen
azért, mert amikor a drv_init() hívódik, még nincsenek fájlrendszerek felcsatolva. Ezért a beállításaikat az
[induló környezeti konfigurációs fájl](https://gitlab.com/bztsrc/osz/blob/master/etc/config) biztosítja, amihez a
következő `libc` függvényekkel férhetnek hozzá:

```c
uint64_t env_num(char *key, uint64_t default, uint64_t min, uint64_t max);
bool_t env_bool(char *key, bool_t default);
char *env_str(char *key, char *default);
```

Ezek függvények a megfelelő értékkel térnek vissza egy adott kulcshoz, vagy a *default* értékkel, ha a kulcs nincs definiálva
a környezeti fájlban. Az első, a szám decimális és (0x prefixű) hexadecimális számokat értelmez, a második logikai értékeket,
amik a következők lehetnek: 0,1,true,false,enabled,disabled,on,off. Az `env_str()` egy frissen allokált bufferben feldolgozás
nélkül adja vissza a kulcs értékét sztringben. Ezt a buffert a hívónak kell később felszabadítania.

A fordítási környezet két [speciális fájl](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.md)t kezel az eszközmeghajtók
forrásában:

 * `platforms` - ha ez létezik, akkor csak akkor fordítja le az eszközmeghajtót, ha az aktuális platform fel van sorolva benne
 * `devices` - ez pedig azon eszközöknek az azonosítóit tartalmazza, melyekhez a meghajtó támogatást biztosít.

Legközelebb a [szolgáltatás](https://gitlab.com/bztsrc/osz/blob/master/docs/howto4-service.md)ok fejlesztéséről lesz szó.
