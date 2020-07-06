OS/Z - Hogyan Sorozat #7 - Telepítés
====================================

Előszó
------

Az ezelőtti oktatóanyagban megnéztük, hogyan kell [vészhelyzeti parancsértelmező](https://gitlab.com/bztsrc/osz/blob/master/docs/howto6-rescueshell.md)t
indítani. Most is üzemeltetői szemszögből közelítünk, és a telepítést vesszük górcső alá.

Rendszer telepítése
-------------------

### Fejlesztői környezetből

Elég egyszerű merevlemezre vagy hordozható pendrájvra illetve SD kártyára kiírni. csak le kell tölteni a
[lemezképek](https://gitlab.com/bztsrc/osz/tree/master/bin/)et, és Linux alatt kiadni a

```sh
dd if=bin/osZ-latest-x86_64-ibmpc.img of=/dev/sdc
```

parancsot, ahol a `/dev/sdc` az a meghajtó, ahová telepíteni akarod (lásd `dmesg` kimenete). MacOSX alatt a `diskutil list`
használható.

Ha nem szereted a parancssort, akkor ajánlott az [USBImager](https://gitlab.com/bztsrc/usbimager) használata, ami egy minimális
ablakos kis program, Windows, MacOSX és Linux alá (szállítható futtatható, nem kell telepíteni se).

### OS/Z-n belül

Ha már bebootolt az OS/Z, akkot a futó rendszer leklónozható a következő paranccsal:

```sh
sys clone /dev/disk2
```

Ahol a `/dev/disk2` annak a meghajtónak a neve, ahová telepíteni szeretnéd a rendszert.

Csomagok telepítése
-------------------

Lehetőség van a csomagok külön, egyesével történő telepítésére és eltávolítására is.

### Csomaglista frissítése

A repók listája, ahonnan csomagok telepíthetők, az `/etc/repos` alatt található. A csomagok metaadatait minden elérhető
csomaghoz le kell tölteni, ezeket a `/sys/var/packages` alatt tárolja a rendszer. A lista frissítéséhez ezt kell kiadni:

```sh
sys update
```

### Rendszerfrissítés

Második paraméter nélkül minden egyes csomagot felfrissít (szóval az egész rendszert), amihez van őjabb verzió.
Ha meg van adva egy csomagnév, akkor csak azt az egyet és a függőségeit frissíti.

```sh
sys upgrade
sys upgrade (csomagnév)
```

### Csomagok keresése

A letöltött metaadatok között lehet keresni a következő paranccsal:

```sh
sys search (sztring)
```

### Csomag felrakása

Ahhoz, hogy egy új csomagot telepítsünk, ami korábban még nem volt a gépen, a parancs:

```sh
sys install (csomagnév)
```

### Csomag eltávolítása

Ha őgy gondolod, hogy már nincs egy programra a továbbiakban szükséged, akkor a következővel törölheted:

```sh
sys remove (csomagnév)
```

Legközelebb arról lesz szó, miként lehet a [szolgáltatások](https://gitlab.com/bztsrc/osz/blob/master/docs/howto8-services.md)at
kezelni.
