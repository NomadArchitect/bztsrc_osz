OS/Z Jelentős eltérések a POSIX-tól
===================================

Először is, az OS/Z-nek alapból egy [saját, letisztult POSIX-szerű interfész](https://gitlab.com/bztsrc/osz/blob/master/docs/refusr.md)e
van, ami szándékosan **nem** teljesen POSIX kompatíbilis. Nagyon hasonló, de a sallangoktól és a visszamenő kompatíbilitási
rétegektől mentes, szigorúan platform független; kiegészítve az OS/Z specifikus funkciókkal. Ha a fordítód -I kapcsolóját az
"include/osZ"-re irányítod, akkor bármelyik egyszerűbb ISO C forrást le fogod tudni fordítani gond nélkül. Ha teljes POSIX
támogatásra vágysz, akkor telepítened kell az OS/Z-re átültetett musl csomagot: "sys install musl". De ne feledd, csak POSIX-ra
hagyatkozva elesel az összes király, alább felsorolt OS/Z funkciótól.

Include fájlok
--------------

A legtöbb ugyanaz, mint POSIX alatt, de szigorú szabály, hogy minden rendszer szolgáltatáshoz csak egy hedör tartozhat, ezért nincs
unistd.h például, az ott deklarált funkciók átkerültek vagy az stdlib.h-ba (core szolgáltatások) vagy az stdio.h-ba (amit meg
az FS taszk biztosít). Ez azonban lényegtelen, mivel az összes hedörfájlt egyetlen gyűjti össze, ezért végeredményben csak egyetlen
egyet kell beinclude-olni:
```
#include <osZ.h>
```

Limitációk
----------

A legtöbb [limits.h](https://gitlab.com/bztsrc/osz/blob/master/include/limits.h)-beli definíció értelmezhetetlen az OS/Z-ben.
Vagy azért, mert limitáció nélküli algoritmust használtam, vagy azért, mert a határérték [indításkor konfigurálható](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.md).

Errno
-----

Az OS/Z-beli libc-ben a legutóbbi hibakódot visszaadó `errno()` egy függvény, és nem egy globális változó. Beállítani a
`seterr(errno)` függvénnyel lehet. Mivel a rendszer limitációk nélkül lett tervezve, ezért sok POSIX [errno érték](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/errno.h)
értelmetlen és nincs definiálva OS/Z alatt. Például an ENFILE (Fájl tábla túlcsordulás) vagy az EMFILE (Túl sok megnyitott fájl).
[FS/Z](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.md) használata mellett sosem fogsz találkozni EFBIG (Fájl túl nagy) vagy
EMLINK (Túl sok hivatkozás) hibákkal sem.

Vannak új hibakódok is, mint például az ENOTUNI (Nem unió) vagy az ENOTSHM (Nem megosztott memória buffer), amik ismeretlenek
a POSIX szabványban.

Egyértelműség
-------------

A szokásos POSIX libc rengeteg azonos funkciójú függvényt nyújt. Az OS/Z azonban inkább az ANSI C filozófiáját támogatja, miszerint
egy feladatra pontosan egy jól definiált függényt kell nyújtani. Ezért például nincs open/fopen, write/fwrite, getc/fgetc,
dprintf/fprintf duplázás, egy változat van mindegyik funkcióból. Az a függvény, ami fájl folyamokat kezel, az "f" előtaggal
rendelkezik, és `fid_t` (egy index sorszám) paramétert vár `FILE *` struct mutató helyett. Ugyancsak sztandardizálva lett a
paraméterek sorrendje. Ezért például a fájlba írás úgy néz ki, hogy `size_t fwrite(fid_t f, void *ptr, size_t size)`,
ami határozottan nem POSIX sztandard, de sokkal több értelme van.

Nincs mblen() sem az stdlib.h-ban, ami POSIX elmebaj, helyette mbstrnlen() van a srting.h-ban, ahová igazából való. Továbbá ki
lett bővítve a string.h, például a chr() és ord() függvényekkel, amik UTF-8 és UNICODE között konvertálnak oda-vissza.

Az összes str* függvény UTF-8 kompatíbilis, és az strcasecmp() helyesen kezeli a nem ASCII kis és nagybetűket és számjegyeket is,
például strcasecmp("ÁÄÉÍŐŠÇÑÆΔДЉѤ٤٢", "áäéíőšçñæδдљѥ42") == 0.

Az uid_t és gid_t típusok
-------------------------

A POSIX-al ellentétben nem numerikus sorszámok, hanem mindkettő univerzális azonosító, uuid_t (16 bájt). Mivel az OS/Z
Hozzáférési Listákat használ, ezért a taszkoknak és fájloknak több csoportja is lehet, így a getgid() nem gid_t-t ad vissza,
hanem egy mutatót a csoport azonosítók listájára, `gid_t *getgid()` (vedd észre a csillagot). A hozzáférési jogosultságok a
UUID legutolsó bájtjában vannak tárolva. Emiatt a formázó függvények (úgy mint sprintf, fprintf, printf stb.) támogatják a
%U opciót a UUID-k megfelelően formázott kiírására.

Hozzáférési jogok
-----------------

A tipikus UNIX-os 'rwx'-al ellentétben az OS/Z támogatja a hozzáfűzés és törlési jogosultságot, azaz 'rwxad'-t használ,
ahol az 'a'-ból következik a 'w'. A legtöbb esetben a 'w' és az 'x' kölcsönösen kizárják egymást, és az 'x' nem igényel
'r' jogosultságot. Hasonlóan a UNIX-hoz, az opendir() és readdir() hívásokhoz az 'x' jogosultság szükséges a könyvtárakon.

Idő reprezentáció
-----------------

Mégegy különbség, hogy az OS/Z az időbélyegeket mikroszekundum (a másodperc milliomod része) pontossággal használja, 64 biten.
Ezért a `time_t` a UTC (Greenwhichi középidő) UNIX EPOCH-tól, 1970-01-01 00:00:00 óta eltelt mikroszekundumokat tárolja (és nem
másodpercet, mint ahogy a POSIX elvárná). Emiatt nem érinti a 2028-as év bug, a dátumokat Kr.e 290501-01-01-től egészen I.sz
294441-12-31-ig (1970 +/- 2^63 /1000000/60/60/24/365) képes kezelni.

Elérési utak
------------

Az OS/Z nem tárolja a `.` és `..` könyvtárakat a lemezen. Helyette az útvonalfeloldást sztringmanipulációval éri el, kihagyva
egy csomó lemezbeolvasást és felgyorsítva ezzel a folyamatot. Emiatt kötelező a könyvtárneveket '/' karakterrel lezárni. Ez nagyon
hasonló ahhoz, ahogy a VMS kezelte az elérési utakat. OS/Z ugyancsak implementálja a FILES-11 (az egyetemi évei alatt a szerző
által sokat használt és szeretett fájlrendszer) azon tulajdonságát, hogy képes a fájlokat verziózni.

Újdonságképp az OS/Z kezeli a speciális `...` joker könyvtárat. Minden könyvtárra illeszkedik az adott mélységben, és a legelső
egyezésre fog feloldódni. Például, legyen '/a/b/c', '/a/d/a' és '/a/e/a' könyvtár, akkor az '/a/.../a' ebből az '/a/d/a'-t
jelenti. A VMS-el ellentétben a három pont csak egy szintet jelöl, és nem megy le rekurzívan az alkönyvtárakba (egyelőre).

Ugyancsak újjítás a könyvtár unió. Ezek szimbólikus hivatkozás szerű könyvtárak (és nem Plan9 uniók, amik túlterhelt felcsatolási
pontok ugyanazon a könyvtáron). Egy vagy több abszolút elérési utat tartalmazhatnak (amikben a joker könyvtár megengedett). Az
unióban felsorolt összes elérési út ellenőrzésre kerül a végleges elérési út megtalálásakor. Példa: a '/bin/' mappa az OS/Z alatt
egy unió, ami a '/sys/bin/' és a '/usr/.../bin/' bejegyzéseket tartalmazza. Minden ezen könyvtárak alatti fájl meg fog jelenni
a '/bin/' alatt. Ezért az OS/Z nem használja a klasszikus PATH környezeti változót, nincs is rá szükség, mivel minden futtatható
parancs megtalálható a '/bin/' alatt.

Ezen túlmenően az OS/Z támogatja a verziókat (';'-al) és fájl pozíciókat ('#'-al) a fájlnevekben.

Fájl hierarchia
---------------

Az OS/Z szándékosan felrúgra a [Fájl Hierarchia Szabvány](http://www.pathname.com/fhs/)t. Habár a fő könyvtárak nagyon
hasonlóak, nincs '/sbin/' se '/proc/', és a '/sys/' egy szokványos könyvtár, ami az operációs rendszer fájlait tartalmazza.
Az '[/usr/](https://gitlab.com/bztsrc/osz/blob/master/usr/)' könyvtár alkönyvtárainak kötött neve van, meg kell egyezniük a
csomag nevével, ami telepítette őket. Ez alatt a fő hierarchia megismétlődik (pl /usr/(csomag)/etc/, /usr/(csomag)/bin/,
/usr/(csomag)/lib/ stb.) Az /mnt/ szintén kiemelt figyelmet kap, a cserélhető tárolók partícióinak nevei automatikusan
létrehozásra kerülnek benne. A [fájl hierachiáról bővebben](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.md).

