Alap rendszer
=============

Szükséges függvénykönyvtárak, alkalmazások és [rendszer szolgáltatások](https://gitlab.com/bztsrc/osz/blob/master/docs/services.md) amik az indító memórialemezen találhatóak.

- *core* az OS/Z legalacsonyabb szintű, felügyelő módban futó kódja
- *drivers* eszközmeghajtók
- *libX* szükséges függvénykönyvtárak
- *fs*, *ui* kritikus rendszer szolgáltatások
- *init*, *net*, *sound*, *syslog* további, nem kritikus rendszer szolgáltatások
- *sh*, *sys* felhasználói programok
- *test* speciális rendszer szolgáltatás, ami az "init" helyett indul és egység valamint funkcionális teszteket tartalmaz
- *link.ld* az OS/Z alkalmazások és osztott könyvtárakhoz használt linkelő szkript
