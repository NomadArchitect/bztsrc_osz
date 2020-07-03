Virtuális Fájlrendszer (VFS)
============================

Natív fájlrendszernek az OS/Z mindig [FS/Z](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.md)-t használ. Ezért a gyökér
fájlrendszer az initrd-n mindig FS/Z, de a felcsatolt tárolók már bármilyen más fájl rendszert használhatnak, például
FAT-et vagy ISO9660-t.

Teljesen Kiírt Elérési Út (FQFP)
--------------------------------

Egy komplett elérési út a következőképp néz ki:

```
 (meghajtó):(könyvtár)/(fájlnév);(verzió)#(pozíció)
```

A meghajtó rész elhagyható, ez azonosítja a tárolót, amin a fájlrendszer található. Opcionális, és a gyökérkönyvtárral
helyettesítődik. A meghajtók `/dev/(meghajtó)/`-ra cserélődnek, ahol az automata felcsatolás gondoskodik a többiről. Ez
főként a cserélhető tárolók esetén hasznos, mint például a `dvd0:`. Ezek a meghajtónevek nem ütközhetnek a hálózati protokoll
nevekkel (http, ftp, ssh stb.), de ez nem is valószínű, és egyáltalán nem jellemző. Ha mégis egyezés lenne, a meghajtónevek mindig
számra végződnek, míg a protokollok soha.

A könyvtár rész tartalmazza a '/'-el tagolt elérési utat (lásd hierarchia alább). A speciális '...' joker név használható
egy adott mélység összes könyvtárára, ami minden alkönyvtárat jelent egyszerre. Ebben az esetben az első egyező elérési utat
fogja használni a rendszer. Könyvtárak lehetnek [a UNIX számára ismeretlen](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.md)
uniók is. Ezek úgy móködiknek, mint a joker, de a találatokat egy adott listára limitálják.

A fájlnév maximum 111 bájt lehet (egészen 27-111 karakterig), és bármit tartalmazhat, kivéve a vezérlőkaraktereket (<32), törlés
karaktert (127), határoló karaktereket (meghajtó ':', könyvtár '/', verzió ';', pozíció '#'), valamint az érvénytelen UTF-8
szekvenciákat. Minden más UNICODE karakter, egészen 4 bájtos UTF-8 kódolásig engedélyezett. (Tudom, sok fájlrendszer megengedi,
hogy bármi legyen a névben, de ez szimplán rossz, és sok boldogtalan (sőt dühös) felhasználót eredményez, amikor elfelejtenek
megfelelően eszképelni. Képzelj csak el egy "/bin" nevű fájlt a gyökérkönyvtárban. Akkor inkább hagyjunk ki pár szimbólumot.)

A verzió rész elhagyható, alapértelmezetten ';0', ami a fájl legfrissebb verziójára mutat. A ';-1' verzió jelenti az eggyel
korábbit, a ';-2' az azelőttit és így tovább, egészen 5 megelőző verzióig.

A pozíció rész szintén elhagyható, alapértelmezésben '#0', ami a fájl elejét jelenti. A pozitív számok a fájl elejétől számított
pozíciót jelentik, míg a negatívak a fájl végétől számítódnak.

Fájl Hierarchia
---------------

| Elérés | Tároló  | Leírás |
| ------ | ------- | ----------- |
| /bin/  | initrd  | futtatható fájlokat tartlamazó könyvtárak uniója |
| /etc/  | initrd  | rendszer szintű konfigurációs fájlokat tartlamazó könyvtárak uniója |
| /lib/  | initrd  | megosztott függvénykönyvtárakat tartalmazó könyvtárak uniója |
| /root/ | initrd  | a rendszergazda home könyvtára (további felcsatolás nélkül is léteznie kell) |
| /sys/  | initrd  | rendszer fájlok |
| /tmp/  | mounted | átmeneti fájlok, egy memórialemez kerül ide felcsatolásra |
| /dev/  | mounted | devfs felcsatolási pontja (az elérési út be van égetve, nem megváltoztatható) |
| /home/ | mounted | felhasználók adatainak felcsatolási pontja |
| /usr/  | mounted | [felhasználói programok](https://gitlab.com/bztsrc/osz/blob/master/usr) felcsatolási pontja |
| /var/  | mounted | programadatok felcsatolási pontja |
| /mnt/  | mounted | cserélhető tárolók felcsatolási pontjait tartalmazó könyvtár |

### /bin
**Célja**
Futtatható parancsok. A '/sys/bin/' (rendszerprogramok) és az '/usr/.../bin/' (telepített felhasználói alkalmazások,
mind saját almappával) uniója.

**Különleges fájlok**
Parancsértelmező, a '/sys/bin/sh' és a rendszerelérést biztosító '/sys/bin/sys' parancs mindenképpen itt kell legyen.
Továbbá minden olyan parancs, ami más fájlrendszerek létrehozását, felcsatolását és javítását hivatott elvégezni.

### /etc
**Célja**
Statikus konfigurációs fájlok. A '/sys/etc/' (rendszer konfigurációs fájlok) és az '/usr/.../etc/' (felhasználói
alkalmazások konfigurációs fájlai) mappákat tartalmazó unió.

**Kölönleges fájlok**
Az aktuális OS verzió a '/sys/etc/os-release' alatt található. A gép neve az '/usr/sys/etc/hostname' alatt kell legyen.

### /lib
**Célja**
Elérhető megosztott függvénykönyvtárak. A '/sys/lib/' (elemi rendszer könyvtárak) és az '/usr/.../lib/' (felhasználói
alkalamazások függvénykönyvtárai) mappák uniója.

**Különleges fájlok**
Legalább a '/sys/lib/crt0.o' és a '/sys/lib/libc.so'. A teljes POSIX kompatibilitást biztosító függvénykönyvtár az
'/usr/musl/lib/libmusl.so' alatt található, ha telepítve van.

### /root
**Célja**
A rendszergazda home könyvtára. Akkor is működnie kell, ha a /home nem felcsatolható, és minden rendszerindításkor
nullázni kell. Normál esetben a rendszergazda sosem jelentkezik be, csak vészhelyzet esetén.

### /sys
**Célja**
Rendszer fájlok. Minden olyan fájl, ami az operációs rendszerhez tartozik, és elengedhetetlen a rendszerinduláskor.
Más rendszer fájlok (amik nem szükségesek az induláskor) az '/usr/sys' alatt helyezkednek el, ami egy felcsatolt mappa.

**Különleges fájlok**
A rendszerfelügyelő a '/sys/core' alatt található, valamint itt vannak a rendszerszolgáltatások is (pl '/sys/fs', '/sys/ui' stb.).
Futtatható parancsok (legalább a parancsértelmező és a rendszer parancs) helye '/sys/bin/sh' és '/sys/bin/sys'. Az eszközmeghajtók
helye a '/sys/drv/' mappa. Az eszközazonosítókat és meghajtóprogramokat összerendelő adatbázis a '/sys/drivers'-ben található.
Minden más rendszerkonfigurációs fájl helye a '/sys/etc'.

### /tmp
**Célja**
Egy memórialemez felcsatolási pontja, ahol átmeneti fájlokat hozhatnak létre a felhasználói programok.

### /dev
**Célja**
A beépített devfs felcsatolási pontja, amit az FS szolgáltatás kezel. Az eszközmeghajtók és szolgáltatások speciális fájlokat
hozhatnak létre benne az mknod() rendszerhívással.

### /home
**Célja**
Felhasználók könyvtárai. Minden olyan fájl, ami csak egy adott felhasználót érint, itt tárolandó.

**Különleges fájlok**
Az első szint a felhasználó belépési azonosítója, '/home/(felhasználó)/', pl. '/home/bzt/'. Nagy rendszerek esetén csoportosító
alkönyvtárak hozhatók létre, pl.: '/home/fejlesztők/bzt/'. A programoknak TILOS felhasználói konfigurációs fájlokat létrehozniuk
a home könyvtárban. Ezek helye a '/home/(felhasználó)/.etc/(alkalmazás)/'.

### /usr
[Felhasználói alkalmazások](https://gitlab.com/bztsrc/osz/blob/master/usr), függvénykönyvtárak és szolgáltatások. A rövidítés
eredetileg a UNIX Shared Resources (megosztott UNIX erőforrások) nlvből származik, de illik a user (felhasználói) kifejezésre is.

**Különleges fájlok**
Az első szintnek MUSZÁJ a csomagnévvel egyeznie. Hasonló az FHS /opt és /usr/X11R6 mappáihoz. Az alkönyvtárak a következők lehetnek:

/usr/(csomag)/bin/ - futtathatók és szkriptek (ha van)

/usr/(csomag)/lib/ - megosztott függvénykönyvtárak (ha van)

/usr/(csomag)/include/ - C header fájlok a függvénykönyvtárakhoz (ha van)

/usr/(csomag)/etc/ - konfigurációs fájlok (ha van)

/usr/(csomag)/share/ - statikus megosztott fájlok (ha van)

/usr/(csomag)/src/ - források (ha van)

A 'sys' csomagnév és az '/usr/sys/' könyvtár nem használható, az az indításhoz nem szükséges rendszer fájlok számára van fenntartva.

### /var
**Célja**
Variálható felhasználói alkalmazás adatok.

**Különleges fájlok**
Az első szintnek MUSZÁJ a csomagnévvel egyeznie. Hasonló az FHS /srv és /var/lib mappáihoz. Az alkönyvtárak nincsenek definiálva,
de a javasolt struktúra:

/var/(csomag)/cache - generált fájlok gyorsítótára (ha van)

/var/(package)/log - generált napló fájlok (ha van)

### /mnt
**Célja**
Cserélhető tárolók felcsatolási pontjait tartalmazó könyvtár. Az FS szolgáltatás hozza létre az alkönyvtárait, a csatlakoztatott
tárolók cimkéinek megfelelően. Ha a tárolón van GPT, a partíciós tábla neve használt, ha nincs, akkor vagy a szuperblokkból kerül
kiolvasásra, vagy egy generált név lesz. Hasonló a MacOSX /Volumes mappájához.

Felcsatolások
-------------

A lemezen egy területre könyvtárként vagy fájlként hivatkozhatunk. A könyvtárak minden esetben '/'-re végződnek. Ez akkor is így
van, ha az a könyvtár egy felcsatolási pont egy eszköznéven. Ekkor a sima névhivatkozás (nincs lezáró '/') használható a read()
és write() hívásoknál a tároló blokkszintű elérésére, míg a lezáró '/' hivatkozás lehetővé reszi az opendir() és readdir()
használatát, ami a tárolón található fájlrendszer gyökér könyvtárát adja vissza.

Példák, mind ugyanarra a könyvtárra mutat, ha a boot partíció (az első indítható partíció az első indítható lemezen, FS0:
EFI alatt) fel van csatolva a /boot/ alá:

```
 /boot/EFI/
 root:/boot/EFI/
 boot:/EFI/
 /dev/boot/EFI/
```
```
 /dev/boot       // blokkeszköz, read() és write() használja
 /dev/boot/      // rajta lévő fájlrendszer gyökér könyvtára, opendir() és readdir() hívások esetén
```

Fontos megjegyezni, hogy ez lehetővé teszi az automatikus felcsatolást egy, a fájlrendszeren lévő fájl hivatkozásával.
