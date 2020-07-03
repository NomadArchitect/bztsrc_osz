OS/Z Szolgáltatások
===================

Kétfajta szolgáltatás létezik: rendszer szolgáltatás és felhasználói szolgáltatás. A rendszer szolgáltatásoknak
az initrd-n kell lenniük, a `libc` hívásokra válaszolnak, és nem befolyásolhatók felhasználói szintről. A
felhasználói szolgáltatásokat (a tipikus UNIX démonokat) másrészről az init rendszer szolgáltatás kezeli, és
külső tárolóról is betölthetők.

### Szolgáltatás hierarchia

```
              +------------------------------------------------------------+
EL1 / ring 0  |                       CORE (+IDLE)                         | (felügyelő)
--------------+-----------+----+----+--------------------------------------+---------------------
EL0 / ring 3  | Meghajtók | FS | UI | syslog | inet | sound | print | init | (rendszer szolgáltatások)
              +-----------------------------------------------------+      |
              | logind | httpd | ...                                       | (init által vezérelt szolgáltatások)
              +------------------------------------------------------------+

              +------------------------------------------------------------+
              | identity | sh | sys | fsck | test | ...                    | (sima felhasználói programok)
              +------------------------------------------------------------+
```

CORE
----

A rendszermag egy speciális szolgáltatás, ami felügyeleti jogosultságokkal fut, és minden címtérbe állandóan le van képezve.
Az OS/Z legalacsonyabb szintje, kivételkezelők, fizikai memória foglalás, taszkkapcsolás, időzítők és hasonlók vannak itt
megvalósítva. Habár önálló taszknak látszik, az `IDLE` taszk része a core-nak, mivel le kell tudnia állítani a CPU-t.
Minden más szolgáltatás (beleértve az eszközmeghajtókat is) nem privilegizált felhasználói jogosultsággal fut (EL0 / ring 3).

Tipikus függvények: alarm(), setuid(), setsighandler(), yield(), fork(), execve().

Eszközmeghajtók
---------------

Minden eszközmeghajtó saját taszkkal rendelkezik. Az MMIO le van képezve a memóriájukba azokon az architektúrákon, ahol ez
támogatott, és elérhetik az B/K címteret. Kizárólag az `FS` taszk és a `CORE` küldhet nekik üzenetet. Egy kivétel van,
a megjelenítő eszközmeghajtója, aminek nincs saját taszkja, hanem az `UI` címterébe kerül betöltésre a hatékonyság miatt.

Tipikus függvények: IRQ(), ioctl(), reset(), read(), write().

FS
--

A fájlrendszer szolgáltatás (File System). Egy nagy adatbázis szerver, ami mindent igyekszik fájlként prezentálni. Az OS/Z osztja
a UNIX és a [Plan 9](https://en.wikipedia.org/wiki/Plan_9_from_Bell_Labs) "minden fájl" megközelítését. A teljes initrd le van
képezve ennek a taszknak a címterébe, és minden szabad fizikai memóriát felhasználhat lemez gyorsítótárnak. Rendszerinduláskor a
fájlrendszer meghajtók betöltésre kerülnek a címterébe, mint megosztott függvénykönyvtárak. Egy beépített fájl rendszerrel
rendelkezik, amit "devfs"-nek hívnak, és ami a detektált eszközöket rendeli az eszközmeghajtó taszkokhoz.

Tipikus függvények: printf(), fopen(), fread(), fclose(), mount().

UI
--

Felhasználói felület szolgáltatás (User Interface). Feladata a tty konzolok, ablakok és GL kontextusok kezelése. Amikor
szükséges, ezt mind összeszerkeszti egy képpé és frissíti a videokártya memóriáját (compositor). Gombkódokat (scancode)
fogad az eszközmeghajtóktól, és cserébe billentyűkód (keycode) üzeneteket küld a fókuszált alkalmazásnak. Egy duplabuffert
tart fenn minden alkalmazással, ami egy sztereografikus képet tartalmazhat. A video frame buffer szintén le van képezve
a címterébe. A helyi képernyő "0" eszközmeghajtójának nincs külön taszkja, hanem ide van betöltve, hogy hatékonyan elérhesse
a képernyő buffert.

Tipikus függvények: ui_opendisplay(), ui_createwindow(), ui_destroywindow().

Syslog
------

A rendszernaplózó szolgáltatás. A címterében egy körkörös buffer van, ami időnként ürítésre kerül a lemezre. Induláskor
a CORE által gyűjtött adatok egy átmeneti bufferbe kerülnek (lásd [syslog_early()](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c) függvény).
Ez a buffer leképezésre kerül a "syslog" taszk címterébe. A naplózás kikapcsolható a `syslog=false`
[induló konfiguráció](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md)s opcióval.

Tipikus függvények: syslog(), setlogmask().

Inet
----

A hálózati interfészek és IP útvonal kezeléséért felelős szolgáltatás (Internet). A hálózati támogatás kikapcsolható a
`networking=false` induló konfigurációs opcióval.

Tipikus függvények: connect(), accept(), bind(), listen().

Sound
-----

Hangkeverő szolgáltatás, ami több audió csatornát egy folyammá alakít. Kizárólagos hozzáférése van a hangkártya eszközhöz, és
helyette mixelő eszköz hozzáférést biztosít az alkalmazások számára. A hangkeverő szolgáltatás és a hang teljes egészében
kikapcsolható a `sound=false` induló konfigurációs opcióval.

Tipikus függvények: speak(), setvolume().

Print
-----

Nyomtatási sor szolgáltatás, ami formázza a beérkező dokumentumokat és továbbítja a nyomtató meghajtónak. Kikapcsolható a
`print=false` induló konfigurációs opcióval.

Tipikus függvények: print_job_add(), print_job_cancel().

Init
----

Ez a szolgáltatás kezeli az összes többi, nem rendszer szolgáltatást. Ilyen például a felhasználói munkamenet, a levelező démon,
nyomtató démon, webszerver stb. Helyette egy vészhelyzeti parancsértelmező indítható a `rescueshell=true` induló konfigurációs
opcióval. Továbbá, ha a rendszer [DEBUG = 1](https://gitlab.com/bztsrc/osz/blob/master/Config) támogatással lett fordítva, és
induló konfigurációban meg van adva a `debug=test`, akkor egy speciális, [tesztelő](https://gitlab.com/bztsrc/osz/blob/master/src/test)
taszk indul az `init` helyett, ami különböző rendszerteszteket futtat le.

Tipikus függvények: start(), stop(), restart(), status().

