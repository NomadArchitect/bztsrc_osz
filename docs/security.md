OS/Z Biztonság
==============

OS/Z alatt a fő processzközti kommunikáció (IPC) formája az üzenetküldés. Az üzenetsorok csak olvashatóként vannak leképezve
a címterekbe, ami azt jelenti, hogy csak `libc` [hívásokon](https://gitlab.com/bztsrc/osz/blob/master/docs/messages.md)
keresztül, felügyeleti szinten lehet módosítani.

Az üzenetek a sorba rakás előtt ellenőrzésre kerülnek az [src/core/msg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c)
fájlban. Ha egy üzenet nem megfelelő, akkor be sem kerülhet a sorba. Emiatt a fogadó taszk biztos lehet benne, hogy a sorból
kiolvasott üzenet mindig érvényes, és nincs szükség további, fogadó oldali ellenőrzésekre.

Ha egy üzenet érvénytelen, akkor a `libc` hibakód `EPERM`-re állítódik, ami azt jelenti, a hozzáférés megtagadva.

Hozzáférési jogosultságok
-------------------------

Bizonyos üzenetek általános kritériumok alapján kerülnek ellenőrzésre. Például csak eszközmeghajtó prioritási szinten futó taszk
küldhet SYS_regirq üzenetet az IRQ átirányító tábla módosítására. Hasonlóan a SYS_mknod üzenet is csak eszközmeghajtó és
szolgáltatás szintről küldhető. Bizonyos esetekben az üzenet egy taszkhoz kapcsolódik, például csak az "FS" taszk küldhet
SYS_mountfs üzenetet az "FS"-nek.

A többi üzenetet egy szofisztikáltabb módon ellenőrződik, ún. Hozzáférési Listák használatával (Access Control List). Jó példa erre
a SYS_stime üzenet, ami a rendszerórát állítja. Csak azok a taszkok küldhetik, melyek írási Hozzáférési Bejegyzéssel (Access Control
Entry) rendelkeznek az "stime" erőforrásra vonatkozóan.

Biztonsági ellenőrzések
-----------------------

Az ellenőrzések egy fájlba lettek összegyűjtve, az [src/core/security.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/security.c)-ba,
így könnyű átnézni a szabályokat és a hozzáférési házirendet javítani.

 * `task_allowed(taszk, erőforrás, mód)` ellenőrzi, hogy a taszknak van-e mód (írás / olvasás) jogosultsága az erőforráshoz.
 * `msg_allowed(taszk, cél, üzenettípus)` ellenőrzi, hogy a megadott taszk küldhet-e adott üzenetet a cél taszknak.

Mint korábban megállapítottuk, csakis a `core` helyezhet el üzenetet a sorba, és ellenőrzi mielőtt betenné. Mivel a sor csak
olvasható, ezért egy kártékony megosztott függvénykönyvtár nem tudja módosítani az üzeneteket a sorban.

Fájl hozzáférési jogosultságok
------------------------------

[POSIX-al ellentétben](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.md) az OS/Z nem használ csoportokat. Helyette
ACL listákat (vagy ha úgy tetszik, csoporttagságok listáját) alkalmazza rendszerszinten. Minden Hozzáférési Bejegyzés (ACE)
tartalmaz hozzáférési jelzőket, úgy mint írás / olvasás / hozzáfűzés stb. Csak az a taszk hajthatja végre az adott műveletet
a fájlon, amelyik legalább egy engedélyező bejegyzéssel rendelkezik.

Például: van egy a/b/c fájlunk `wheel:w,admin:r` hozzáféréssel. Ez azt jelenti, hogy azok a taszkok, melyeknek `admin:r`
bejegyzésük van, olvashatják a fájlt, de nem módosíthatják, hacsak nincs `wheel:w` bejegyzésük is. Az ACE bejegyzéseket átalában
teljes hozzáféréssel szokás adni, mint például `wheel:rwxad`.
