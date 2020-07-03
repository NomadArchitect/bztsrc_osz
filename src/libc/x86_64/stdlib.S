/*
 * libc/x86_64/stdlib.S
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
 * @brief alacsony szintű sztandard libc funkciók, lásd stdlib.h
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
    pushq   %r12
    movq    16(%rsp), %rax
    movq    24(%rsp), %r12
    movq    %rcx, %r10
    syscall
    pushq   %rax
    pushq   %rdx
    call    seterr@PLT
    popq    %rdx
    popq    %rax
    popq    %r12
    ret
endf

/**
 * blokkoló, üzenet küldése és várakozás válaszra
 *
 * msg_t *mq_call(arg0, arg1, arg2, arg3, arg4, arg5, event)
 */
func mq_call
    /* küldés */
    pushq   %r12
    mov     8(%rsp), %rax
    movq    %rcx, %r10
    xorq    %r12, %r12
    syscall
    /* sikerült? */
    orq     %rdi, %rdi
    jnz     1f
    /* igen, akkor erre a sorszámra várjuk a választ */
    movq    %rax, %rdi
    jmp     mq_recv_local
1:  call    seterr@PLT
    xor     %rax, %rax
    xor     %rdx, %rdx
    popq    %r12
    ret
endf

/**
 * blokkoló, addig várakozik, míg üzenet nem érkezik
 *
 * msg_t *mq_recv()
 */
func mq_recv
    pushq   %r12
    xorq    %rdi, %rdi
mq_recv_local:
    xorq    %rax, %rax
    syscall
    popq    %r12
    ret
endf

/**
 * visszaadja a sorszámot vagy 0-át, ha nincs várakozó üzenet
 *
 * uint64_t mq_ismsg()
 */
func mq_ismsg
    movq    __PAGESIZE, %rax
    cmpq    __PAGESIZE+8, %rax
    je      1f
    movq    56(%rax), %rax
1:  ret
endf

/*****************************************************************************
 *                   OS/Z sztandard megosztott függvények                    *
 *****************************************************************************/

/* ne használd, mq_dispatch() segédfüggvénye */
func mq_dispatchcall
    /* rax=*symtable[MSG_FUNC(msg->evt)](msg->arg0, msg->arg1, msg->arg2, msg->arg3, msg->arg4, msg->arg5) */
    movq    8(%rsp), %rax
    subq    $8, %rsp
    call    *%rax
    addq    $8, %rsp
    ret
endf

#if DEBUG
func dbg_printf
    pushq   %rbp
    pushq   %r12
    movq    $SYS_dbgprintf, %rax
    movq    %rcx, %r10
    syscall
    popq    %r12
    popq    %rbp
    ret
endf
#endif

/**
 * felfüggeszti a futást (yield), míg nem sikerül a jelzőbitet lefoglalni
 *
 * void lockacquire(int bit, uint64_t *mem)
 */
func lockacquire
1:  lock
    btsq    %rdi, (%rsi)
    jnc     1f
    xorq    %rax, %rax
    movb    $SYS_yield, %al
    syscall
    jmp     1b
1:  ret
endf

/**
 * jelzőbit felszabadítása
 *
 * void lockrelease(int bit, uint64_t *mem)
 */
func lockrelease
    lock
    btrq    %rdi, (%rsi)
    ret
endf

/**
 * megadott azonosítóval foglalja le a jelzőt. Nem blokkol, 0-át ad vissza, ha sikerült
 *
 * int lockid(uint64_t id, uint64_t *mem)
 */
func lockid
    xorq    %rax, %rax
    lock
    cmpxchg %rdi, (%rsi)
    ret
endf

/**
 * felfüggeszti a futást
 *
 * void yield()
 */
func yield
    xorq    %rax, %rax
    movb    $SYS_yield, %al
    syscall
    ret
endf

/**
 * visszaadja a szabad és összes memória lapok számát
 *
 * meminfo_t meminfo()
 */
func meminfo
    xorq    %rax, %rax
    movb    $SYS_meminfo, %al
    syscall
    ret
endf

/**
 * visszaadja a hívó taszk felhasználójának azonosítóját
 *
 * uid_t getuid(void)
 */
func getuid
    xorq    %rdi, %rdi
    xorq    %rax, %rax
    movb    $SYS_getuid, %al
    syscall
    ret
endf

/**
 * visszaadja egy taszk felhasználójának azonosítóját
 *
 * uid_t getuidp(pid_t pid)
 */
func getuidp
    xorq    %rax, %rax
    movb    $SYS_getuid, %al
    syscall
    ret
endf
