OS/Z - Hogyan Sorozat #8 - Szolgáltatások kezelése
==================================================

Előszó
------

Legutóbb az OS/Z és a csomagjainak [telepítés](https://gitlab.com/bztsrc/osz/blob/master/docs/howto7-install.md)e volt terítéken.

Szolgáltatások Menedzselése
---------------------------

A legtöbb POSIX operációs rendszerben a szolgáltatásokat vagy szkriptek kezelik (SysV init) vagy egy másik szolgáltatás
(systemd vagy launchd). Mindkét esetben start és stop parancsok állnak rendelkezésre a parancsértelmezőben. Az OS/Z sem
kivétel, itt az `init` rendszerszolgáltatás a felelős a felhasználi szolgáltatások felügyeletéért. Fedd figyelembe, hogy
a [rendszer szolgáltatások](https://gitlab.com/bztsrc/osz/blob/master/docs/services.md)at nem lehet parancssorból irányítani,
mivel azok a rendszer szerves részeit képezik.

### Szolgáltatás indítása

Hogy egyszer elinduljon egy szolgáltatás, a következő parancsot kell kiadni

```sh
sys start (szolgáltatás)
```

### Szolgáltatás leállítása

A leállításhoz

```sh
sys stop (szolgáltatás)
```

### Szolgáltatás indítása minden induláskor

Hogy az indítás elinduljon, és hogy ez az indítás tartós maradjon, újrabootolás után is,

```sh
sys enable (szolgáltatás)
```

### Szolgáltatás kikapcsolása induláskor

Az ellenkezőjéhez

```sh
sys disable (szolgáltatás)
```

### Szolgáltatás újraindítása

Ha egy szolgáltazás konfigurációs fájlja megvéltozik, akkor újra kell olvasnia. De ha ezt nem tenné, vagy éppenséggel
a démon rosszul viselkedik, akkor szükség lehet a szolgáltatás újraindítására. Ehhez a következő parancsot kell kiadni

```sh
sys restart (szolgáltatás)
```

### Információ lekérdezése

Az elérhető szolgáltatások listájához a következő parancsot paraméter nélkül kell kiadni. Egy szolgáltatás részletes
adatainak megjelenítéséhez paraméterként a szolgáltatás nevét is meg kell adni.

```sh
sys status
sys status (szolgáltatás)
```

A következő oktatóanyag végfelhasználói szemszögből vizsgálja a [felhasználói felület](https://gitlab.com/bztsrc/osz/blob/master/docs/howto9-interface.md)et.
