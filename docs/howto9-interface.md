OS/Z - Hogyan Sorozat #9 - Felhasználói interfész
=================================================

Előszó
------

Legutóbb a [szolgáltatások](https://gitlab.com/bztsrc/osz/blob/master/docs/howto8-services.md)ról volt szó, rendszerüzemeltetői
megközelítésből. Most mint végfelhasználó vesszük szemügyre a rendszert.

Bejelentkezés
-------------

Mivel az OS/Z egy többfelhasználós rendszer, ezért amikor elindul, egy bejelentkező képernyővel fogad. Itt bekéri a
felhasználónevet és a hozzá tartozó jelszót. Sikeres azonosítás esetén a `logind` leforkol, és munkamenetmenedzserré
változik. Amikor ez a munkamenetmenedzser kilép, akkor a vezérlés visszakerül a szülő logind-hez, ami újból jelszót
fog kérni.
