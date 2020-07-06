OS/Z - Hogyan Sorozat #6 - Vészhelyzeti parancssor
==================================================

Előszó
------

Az ezelőtti anyagban azt néztük meg, hogy kell [alkalmazást fejleszteni](https://gitlab.com/bztsrc/osz/blob/master/docs/howto5-app.md)
OS/Z alá, ami magától értetődően fejlesztők számára érdekes. Ebben a részben rendszerüzemeltetői megközelítést alkalmazunk,
megnézzük, mit lehet tenni, ha lehalt a rendszer.

Vészhelyzeti parancssor aktiválása
----------------------------------

<img align="left" style="margin-right:10px;" height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszrsh.png" alt="OS/Z Vészhelyzeti parancssor">

Az induló környezeti [konfiguráció](https://gitlab.com/bztsrc/osz/blob/master/etc/config)ban kell engedélyezni a `rescueshell=true`
beállítással. A következő boot alkalmával a szokásos `init` helyett egy rendszergazdai jogosultsággal futó `sh` fog elindulni. Ez
azt jelenti, hogy az OS/Z nem fogja a felhasználói szolgáltatásokat elindítani.

A vészhelyzeti parancssor pont olyan, mint bármelyik másik parancssor, kivéve, hogy rendszergazdai jogosultságokkal fut,
ezért légy vele óvatos. Az elsődleges célja nem a mindennapi használat, hanem egy hibás rendszer lementése, helyreállítása.
A Linux-al ellentétben itt minden fájlrendszer a szoksásos módon fel van csatolva, és a hálózat is elérhető, így lehet
távolra menteni.

Elérhető parancsok
------------------

### ls
Könyvtár tartalmának listázása. A -l opció bőbeszédűvé teszi.

### cd
Aktuális munkakönyvtár beállítása.

### pwd
Akruális munkakönyvtár kiírása.

### cat
Fájlok tartalmának kiírása a kimenetre.

### echo
Syöveg kimenetre írása.

### exit, quit
Kilépés. Amikor a vészhelyzeti parancsértelmező befejezi a futást, a vezérlés visszakerül a core-ra, ami leállítja a gépet.

Legközelebb arról beszélünk, miképp lehet [telepíteni](https://gitlab.com/bztsrc/osz/blob/master/docs/howto7-install.md) OS/Z alatt.
