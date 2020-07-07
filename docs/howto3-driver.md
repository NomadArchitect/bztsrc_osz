OS/Z - Hogyan Sorozat #3 - Eszközmeghajtó fejlesztése
=====================================================

Előszó
------

Legutóbb tisztáztuk, hogyan kell [debuggolni OS/Z alatt](https://gitlab.com/bztsrc/osz/blob/master/docs/howto2-debug.md) virtuális
gépen. Ennek ismeretében már nyugodtan nekiállhatunk fejleszteni.

[Eszközmeghajtó](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.md)ra azért van szükség, hogy alacsonyszintű hozzáférést
biztosítson a hardveres eszközökhöz, amit aztán egy egységes interfésszel elfed, vagy hogy a rendszerszolgáltatás kiegészítse
egy új formátum támogatásával (például fájlrendszermeghajtó). Ez csak kevesek számára érdekes, nyugodtan ugord át és lépj tovább
a [szolgáltatások fejlesztése](https://gitlab.com/bztsrc/osz/blob/master/docs/howto4-service.md) anyagra.

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

Bár a hardveres eszközmeghajtók is felhasználói szinten futnak, ezek nem hagyományos alkalmazások. A fő programjuk egy
[eseménykezelő](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c) végtelen ciklus. A meghajtóprogramok ehhez
nyújtanak függvényeket, és ezzel kerülnek összeszerkesztésre. A meghajtóprogramok
[kategorizálva](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/README.md) vannak, mindnek megvan a megfelelő alkönyvtára.
Hívhatnak bizonyos privilegizált `libc` funkciókat, és speciális eszközfájlokat hozhatnak létre a /dev alatt.
Ezen kívül a rendszermemóriák le vannak képezve a címterükbe, amiknek a fizikai címeit a `drv_phymem` struktúrából kiolvashatják,
a `drv_virtmem()` pedig visszaadja, milyen virtuális címen érhetik el azokat.
Üzeneteket a syslog-nak küldhetnek, egyébként a sztandard kimenetre vagy hibakimenetre nem írhatnak (egész egyszerűen azért,
mert amikor inicializálnak, még nincs sztandard kiemenet, az majd csak az FS taszk inicializálása után lehet, ami viszont
függ a blokkeszközmeghajtóktól).

```c
#include <osZ.h>
#include <driver.h>

public void drv_init()
{
    /* Inicializálás. Ha probléma adódik (eszköz nem található, vagy nem válaszol),
     * akkor exit(EX_UNAVAILABLE); hívással ki kell lépni */

    /* a rendszermemória elérhető a címtérben, a következő függvény visszaadja,
     * hogy hol, vagy NULL-t ha nincs leképezve az adott fizikai cím */
    void *acpi_tables = drv_virtmem(drv_phymem.acpi_ptr);
    void *pcie_memory = drv_virtmem(drv_phymem.pcie_ptr);

    /* megszakításlekezelő regisztrálása drv_regirq(irq) hívással. */
    /* periodikus hívás kérése drv_regtmr() hívással (eszköz elérhető-e még hívás). */
    /* ha van értelme, eszközfájl létrehozása mknod() hívással/hívásokkal. */

    /* üzenet kiírása */
    syslog(LOG_INFO, "találtam egy eszközt: %x %U", devaddr, devuuid);
}

public void drv_irq(uint16_t irq, uint64_t ticks)
{
    /* megszakítás lekezelése. Ennek kicsinek és kompaktnak kell lennie, nem küldhet üzeneteket. */
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

Ha konfigurációs beállításokra van szüksége egy eszközmeghajtónak, akkor nem használhat fájlokat. Méghozzá
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

Fájlrendszer meghajtók
----------------------

Ezek, akárcsak a többi szoftveres eszközmeghajtó, nem az általános driver.c-vel kerülnek linkelésre, hanem az FS taszkkal
szerkesztődnek össze. A drv_init() hívásukban nem fogadnak eseményeket, hanem helyette értesíteniük kell az FS taszkot,
hogy milyen fájlrendszert támogatnak, és milyen függvényekkel. Ennek a definíciója az
[fsdrv.h](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.h)-ban található, de mivel úgyis szükség lesz egyéb
fájlrendszer funkciókra is, ezért érdemesebb a vfs.h-t behúzniuk.

```c
#include <osZ.h>
#include <vfs.h>

/* az alább hivatkozott függvények implementációi */

fsdrv_t drv = {
    "fsz",      /* mountnál és fstab-ban használt név */
    "FS/Z",     /* megjelenített név */
    detect,     /* detektáló funkció */
    locate,     /* elérési út feloldás */
    resizefs,   /* átméretezés */
    checkfs,    /* ellenőrzés, fsck hurok */
    stat,       /* stat struktúra visszadása */
    getdirent,  /* könyvtárbejegyzés visszaadása */
    read,       /* olvasás fájlból */
    writestart, /* írás tranzakció kezdete */
    write,      /* írás */
    writeend    /* írás tranzakció vége */
};

/* fájlrendszermeghajtó regisztrálása */
public int drv_init()
{
    fsdrv_reg(&drv);
}
```

Az eseményeket az FS taszk fogadja és kezeli, és a kéréseknek megfelelő fájlrendszermeghajtónak továbbítja.

Csomagok
--------

Az eszközmeghajtók magasabb szonten futnak, és több mindent elérnek, mint a többi processz, ezért bitzonsági okokból nem
telepíthetők úgy, mint a többi szolgáltatás vagy alkalmazás. Ezeknek mindig az inird-ben kell lenniük, a /sys/drv alatt.

Legközelebb a [szolgáltatás](https://gitlab.com/bztsrc/osz/blob/master/docs/howto4-service.md)ok fejlesztéséről lesz szó.
