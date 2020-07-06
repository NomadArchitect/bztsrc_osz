OS/Z Eszközmeghajtók
====================

Az itt található driver.c fordul le `sys/driver`-nek. Ez a közös kódja minden egyes eszközmeghajtó taszknak. A benne
hivatkozott függvényeket az alkönyvtárakban található megosztott függvénykönyvtárak valósítják meg.

- *include* eszközmeghajtó specifikus definíciós fájlok
- *driver.c* a közös eszközmeghajtó diszpécser
- a többi könyvtár eszközosztály

Licensz
-------

Az eszközmeghajtók külön kivételt képeznek licenszelési szempontból, a forrásukat nem kötelező megosztani, a bináris
terjesztés is megengedett. Ehhez egy megfelelő architektúrára fordított ELF binárist kell a könyvtárba helyezni, "(ARCH).so"
névvel, ami megvalósítja a drv függvényeket, és ami a driver-el kerül összeszerkesztésre futási időben.

Az eszközmeghajtók ezért kettős licenszűek. Vagy nyílt forráskódú **CC-by-nc-sa**, mint a rendszer többi része, vagy zárt
forráskódú, bináris terjesztésűek, ún. **propietary**.

Eszközmeghajtó osztályok
------------------------

| Könyvtár    | PCI osztály | Leírás |
| ----------- | ----------- | ----------- |
| comctl      | 07          | Kommunikációs vezérlők |
| bridge      | 06          | Rendszerbusz vezérlők |
| display     | 03          | Képernyő megjelenítők, `UI`-ba töltődik be |
| docking     | 0A          | Dokkoló állomások |
| encrypt     | 10          | Titkosítás vezérlők |
| fs          | xx          | Fájlrendszer meghajtók, `FS`-be töltődnek be |
| generic     | 08          | Általános, máshova nem sorolható eszközmeghajtók |
| inet        | xx          | Hálózati protokoll kezelők, `inet`-be töltődnek be |
| input       | 09          | Bemeneti eszközmeghajtók |
| intelligent | 0E          | Intelligens vezérlők |
| mmedia      | 04          | Multimédia vezérlők |
| network     | 02          | Hálózati eszközmeghajtók (vezetékes) |
| print       | xx          | Nyomtatási kép formázók, `print`-be töltődnek be |
| satellite   | 0F          | Szatelit kommunikációs vezérlők |
| serial      | 0C          | Soros busz meghajtók |
| sound       | xx          | Hangfeldolgozó és keverő meghajtók, `sound`-ba töltődnek be |
| signal      | 11          | Jelfeldolgozó vezérlők |
| storage     | 01          | Háttértároló eszközmeghajtók (blokk eszközök) |
| ui          | xx          | Felhasználói felület meghajtók, `UI`-ba töltődnek be |
| wireless    | 0D          | Hálózati eszközmeghajtók (vezeték nélküli) |

A hasonlóság a [PCI eszköz osztály](http://pci-ids.ucw.cz/read/PD)okkal nem a véletlen műve. A specifikáció definiál még
`proc` és `memory` osztályokat, ezek azonban itt hiányoznak, mivel performancia okokból a core-ban lettek megvalósítva, és az
[src/core/(arch)/(platform)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64) alatt találhatók, lásd
[portolás](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.md).

Az osztálykönyvtár alatt pontosan egy újabb könyvtárszint helyezkedik el, meghajtónként egy könyvtárral. Minden egyes lefordított
eszközmeghajtó az initrd-n a `sys/drv/(osztály)/(meghajtó).so` alá kerül.

További információért lásd a [dokumentáció](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.md)t és a [hogyan fejlesszünk](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-driver.md) leírást.
