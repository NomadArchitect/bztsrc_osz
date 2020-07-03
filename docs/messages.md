OS/Z üzenetsor
==============

Mint minden [mikro-kernel](https://en.wikipedia.org/wiki/Microkernel) architektúrában, így OS/Z-ben is felhasználói szinten futnak a
folyamatok. Nem láthatják egymás memóriáját, és a monolitikus kernellel ellentétben az eszközmeghajtók nem ttudják magukkal rántani
a rendszert. Ezen folyamatok alapvető kommunikációs formája (IPC-je) az üzenetsor, amit a core felügyel. Kényelmi okoból a taszkok
a core-al is az üzenetsor interfészen keresztül kommunikálnak, ehhez csak a címzettet SYS_CORE-ra kell állítani, habár a core-nak
nincs igazi üzenetsora.

Felhasználói szintű függvények
------------------------------

Normális esetben semmit nem fogsz látni az üzenetsorokból. A `libc` függvénykönyvtár minden részletet elrejt, szóval csak meg kell
hívnod a printf() és fopen() stb. függvényeket, ahogy szoktad. [Biztonság](https://gitlab.com/bztsrc/osz/blob/master/docs/security.md)i
okokból az üzenetsor nem érhető el közvetlenül, és nem ajánlott a svc/syscall interfész direkt használata sem. A `libc` függvényei
garantálják az alkalmazások [hordozhatóság](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.md)át.

De ha mégis szükség lenne, hogy elérjük az üzenetsort (például amikor egy új protokollt implementál az ember), erre az esetre
a `libc` biztosít néhány függvényt.

### Alacsony szintű felhasználói függvények (rendszerhívások)

Mindössze öt funkció az üzenetsor kezelése, mind variáció a szinkronizálásra. A `libc` biztosítja őket, és a [syscall.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/syscall.h)
alatt találod a definícióikat. Ezeket nem szükséges közvetlenül hívni, (hacsak nem implementálsz saját protokollt), inkább valamelyik
magasabb szintű függvény használata preferált.

```c
/* aszinkron, üzenetet küld, és visszaadja a sorszámát (nem-blokkol) */
mq_send0(dst, func);
mq_send1(dst, func, arg0);
mq_send2(dst, func, arg0, arg1);
 ...
uint64_t mq_send(
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    evt_t event);

/* szinkron, üzenetet küld és vár a válaszra (blokkol) */
mq_call0(dst, func);
mq_call1(dst, func, arg0);
mq_call2(dst, func, arg0, arg1);
 ...
msg_t *mq_call(
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    pid_t dst,
    uint64_t func);

/* aszinkron, van üzenetet? visszaadja az üzenet sorszámát vagy 0-át (nem-blokkol) */
uint64_t mq_ismsg();

/* szinkron, addig vár, míg üzenet nem érkezik (blokkol) */
msg_t *mq_recv(pid_t from);

/* szinkron, események lekezelése (blokkol, nem tér vissza) */
void mq_dispatch();
```
Amíg az [mq_call()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S) egy mq_send() és mq_recv() hívás párosa,
addig az [mq_dispatch()](https://gitlab.com/bztsrc/osz/blob/master/src/lib/libc/dispatch.c) pont fordított: először üzenetet fogad,
mq_recv()-el, meghívja az üzenetet lekezelő funkciót, majd az mq_send() segítségével választ küld vissza. Az msg_t struct
definícióját lásd alább.

A célnak megadható egy rendszer szolgáltatás száma is a `dst`-ben, ezeket az [syscall.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/syscall.h)
fájlban találod. A hozzájuk tartozó funkciókódokat az [include/osZ/bits](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/bits) alatt találod,
ezeket mind betölti ez a headör. A kényelem kedvéért mq_sendX() és mq_callX() makrók állnak rendelkezésre.

Felügyeleti szint (EL1 / Ring 0)
------------------------------

Core nem használhatja a libc-t, ezért saját üzenetsor implementációja van. Három függvény az [src/core/msg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c)-ben:

```c
msg_t *msg_recv(pid_t pid);

bool_t msg_send(
    uint64_t arg0, /* mutató */
    uint64_t arg1, /* méret */
    uint64_t arg2, /* típus */
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    evt_t event,
    uint64_t serial);

bool_t msg_core(
    uint64_t arg0, /* mutató */
    uint64_t arg1, /* méret */
    uint64_t arg2, /* típus */
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    evt_t event);
```

Mivel a `core` sosem blokkolódik, ezért mind aszinkron. Továbbá összevont (dst<<16 | func) értéket használnak üzenet eseményként.
Ha az EVT_PTRDATA jelző be van állítva, akkor az arg0 egy virtuális cím, arg1 a buffer méret és arg2 a buffer típusát jelöli.

A harmadik verzió kezeli a konkrétan `core`-nak címzett üzeneteket (mint pl SYS_exit vagy SYS_alarm események).

### Alacsony szint

Felhasználói szintről az üzenetsor x86_64-en a `syscall` utasítással, míg AArch64 alatt az `svc` utasítással érhető el.
Az alacsony szintű függvénykönyvtár épít ezekre (lényegében a mq_send() és az mq_recv() és a többi csak beburkolják).
A cél és a funkció az %rax/x6 regiszterben adódik át. Amikor a cél a SRV_CORE (0) és a funkció SYS_recv (0),
akkor üzenetet fogad, minden más esetben küld.

A paraméterek a System V ABI szerint kerülnek átadásra, kivéve x86_64 esetében, ahol az %rcx-et használja a syscall utasítás,
ezért a 4. paramétert az %r10-ben kell átadni.
```
x6 / %rax= (pid_t taszk) << 16 | funkció,
x0 / %rdi= arg0/mutató
x1 / %rsi= arg1/méret
x2 / %rdx= arg2/magic
x3 / %r10= arg3
x4 / %r8 = arg4
x5 / %r9 = arg5
svc / syscall
```
Fontos, hogy ezeket az alacsonyszintű syscall (vagy svc) utasításokat sose használd, helyette ott van az `mq_send()`, `mq_call()`
és a többiek a hordozhatóság kedvéért. Vagy még jobb, ha teheted, használj egy magasabb szintű `libc` funkciót, mint például a
printf() vagy fopen().

Struktúrák
----------

Annyira alap típus, hogy a [types.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/types.h) fájlba került.

```c
msg_t *mq;
```

A tömb első eleme az üzenetsor fejléc. Mivel a konkrét msghdr_t implementációja az üzenetsor felhasználója számára nem fontos,
ezért a fejlécet az [src/core/msg.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.h) definiálja.

```c
msghdr = (msghdr_t *)mq[0];
```

A többi elem (1-től kezdődően) egy körkörös FIFO buffert alkotnak.

