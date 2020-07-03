/*
 * libc/aarch64/stdlib.S
 *
 * Copyright (c) 2019 bzt (bztsrc@gitlab)
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * A művet szabadon:
 *
 * - Megoszthatod — másolhatod és terjesztheted a művet bármilyen módon
 *     vagy formában
 * - Átdolgozhatod — származékos műveket hozhatsz létre, átalakíthatod
 *     és új művekbe építheted be. A jogtulajdonos nem vonhatja vissza
 *     ezen engedélyeket míg betartod a licensz feltételeit.
 *
 * Az alábbi feltételekkel:
 *
 * - Nevezd meg! — A szerzőt megfelelően fel kell tüntetned, hivatkozást
 *     kell létrehoznod a licenszre és jelezned, ha a művön változtatást
 *     hajtottál végre. Ezt bármilyen ésszerű módon megteheted, kivéve
 *     oly módon ami azt sugallná, hogy a jogosult támogat téged vagy a
 *     felhasználásod körülményeit.
 * - Ne add el! — Nem használhatod a művet üzleti célokra.
 * - Így add tovább! — Ha feldolgozod, átalakítod vagy gyűjteményes művet
 *     hozol létre a műből, akkor a létrejött művet ugyanazon licensz-
 *     feltételek mellett kell terjesztened, mint az eredetit.
 *
 * @subsystem libc
 * @brief alacsony szintű standard libc funkciók, lásd stdlib.h
 */

#include <osZ/types.h>
#include <osZ/limits.h>
#include <osZ/syscall.h>

.section .crt0, "ax"
func init
    ret
endf

.section .text, "ax"
/*****************************************************************************
 *                           OS/Z rendszerhívások                            *
 *****************************************************************************/

/**
 * nem-blokkoló, üzenet küldése
 *
 * uint64_t mq_send(arg0, arg1, arg2, arg3, arg4, arg5, event, serial)
 */
func mq_send
    svc     0
    stp     x1, x2, [sp, #-16]!
    stp     x29, x30, [sp, #-16]!
    bl      seterr
    ldp     x29, x30, [sp], #16
    ldp     x0, x1, [sp], #16
    ret
endf

/**
 * blokkoló, üzenet küldése és várakozás válaszra
 *
 * msg_t *mq_call(arg0, arg1, arg2, arg3, arg4, arg5, event)
 */
func mq_call
    mov     x7, xzr
    /* küldés */
    svc     0
    /* sikerült? */
    cbnz    x0, 1f
    /* igen, akkor erre a sorszámra várjuk a választ */
    mov     x0, x1
    b       mq_recv_local
1:  stp     x29, x30, [sp, #-16]!
    bl      seterr
    ldp     x29, x30, [sp], #16
    mov     x0, xzr
    mov     x1, xzr
    mov     x2, xzr
    ret
endf

/**
 * blokkoló, addig várakozik, míg üzenet nem érkezik
 *
 * msg_t *mq_recv()
 */
func mq_recv
    mov     x0, xzr
mq_recv_local:
    mov     x6, xzr
    svc     0
    ret
endf

/**
 * visszaadja a sorszámot vagy 0-át, ha nincs várakozó üzenet
 *
 * uint64_t mq_ismsg()
 */
func mq_ismsg
    mov     x0, xzr
    ldr     x1, =__PAGESIZE
    ldp     x2, x3, [x1]
    sub     x3, x3, x2
    cbz     x3, 1f
    ldr     x0, [x2, #56]
1:  ret
endf

/*****************************************************************************
 *                   OS/Z sztandard megosztott függvények                    *
 *****************************************************************************/

/* ne használd, mq_dispatch() segédfüggvénye */
func mq_dispatchcall
    /* x0=*symtable[MSG_FUNC(msg->evt)](msg->arg0, msg->arg1, msg->arg2, msg->arg3, msg->arg4, msg->arg5) */
    stp     x29, x30, [sp, #-16]!
    blr     x6
    ldp     x29, x30, [sp], #16
    ret
endf

#if DEBUG
func dbg_printf
    mov     x6, #SYS_dbgprintf
    svc     0
    ret
endf
#endif

/**
 * felfüggeszti a futást (yield), míg nem sikerül a jelzőbitet lefoglalni
 *
 * void lockacquire(int bit, uint64_t *mem)
 */
func lockacquire
    /* x5 = 1 << x0 */
    mov     x2, #1
    lsl     x5, x2, x0
#if 0
    /* elemi x3 = [x1], [x1] |= x5 */
1:  ldsetal x3, x5, [x1]
    and     x3, x3, x5
    cbz     x3, 2f
    mov     x6, #SYS_sched_yield
    svc
    b       1b
2:
#else
1:  prfm    pstl1keep, [x1]         /* prefetch */
    ldaxr   x3, [x1]                /* x3 = [x1] */
    and     x4, x3, x5              /* ha (x3 & x5) akkor újra */
    cbz     x4, 2f
    mov     x6, #SYS_yield
    svc     0
    b       1b
2:  orr     x3, x3, x5              /* x3 |= x5 */
    stxr    w4, x3, [x1]            /* [x1] = x3 */
    cbnz    w4, 1b
#endif
    ret
endf

/**
 * jelzőbit felszabadítása
 *
 * void lockrelease(int bit, uint64_t *mem)
 */
func lockrelease
    /* x0 = 1 << x0 */
    mov     x2, #1
    lsl     x0, x2, x0
#if 0
    /* elemi x3 = [x1], [x1] &= ~x0 */
    ldclral x3, x0, [x1]
#else
    prfm    pstl1keep, [x1]         /* prefetch */
    ldaxr   x3, [x1]                /* x3 = [x1] */
    bic     x3, x3, x0              /* x3 &= ~x0 */
    stxr    w4, x3, [x1]            /* [x1] = x3 */
#endif
    ret
endf

/**
 * megadott azonosítóval foglalja le a jelzőt. Nem blokkol, 0-át ad vissza, ha sikerült
 *
 * int lockid(uint64_t id, uint64_t *mem)
 */
func lockid
    mov     x2, x0
    stxr    w0, x2, [x1]
    ret
endf

/**
 * felfüggeszti a futást
 *
 * void yield()
 */
func yield
    mov     x6, #SYS_yield
    svc     0
    ret
endf

/**
 * visszaadja a szabad és összes memória lapok számát
 *
 * meminfo_t meminfo()
 */
func meminfo
    mov     x6, #SYS_meminfo
    svc     0
    ret
endf

/**
 * visszaadja a hívó taszk felhasználójának azonosítóját
 *
 * uid_t getuid(void)
 */
func getuid
    mov     x0, xzr
    mov     x6, #SYS_getuid
    svc     0
    ret
endf

/**
 * visszaadja egy taszk felhasználójának azonosítóját
 *
 * uid_t getuidp(pid_t pid)
 */
func getuidp
    mov     x6, #SYS_getuid
    svc     0
    ret
endf
