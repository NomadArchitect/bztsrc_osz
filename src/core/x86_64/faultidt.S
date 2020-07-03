/*
 * core/x86_64/faultidt.S
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
 * @subsystem platform
 * @brief alacsony szintű kivétel kezelők
 */

#include <arch.h>

.section .text
.global platform_exc00divzero
.type platform_exc00divzero, STT_FUNC
.global platform_exc31
.type platform_exc31, STT_FUNC

.macro exception num, errcode, name, func
    .balign 128
.global platform_\name
platform_\name:
xchg %bx, %bx
    cli
    /* ha a kivétel a debuggerben vagy a kernelben történt, akkor csak a sys_fault-ot állítjuk */
#if DEBUG
    cmpb    $0, dbg_indump
    jnz     2f
#endif
.if \errcode
    cmpq    $CORE_ADDRESS, 8(%rsp)
.else
    cmpq    $CORE_ADDRESS, (%rsp)
.endif
    jb      1f
2:  movb    $\num, sys_fault
.if \errcode
    addq    $8, %rsp
.endif
    xorq    %rax, %rax
    incq    TCBFLD_PC
    iretq
1:  call    platform_save
.if \errcode
    movq    (%rsp), %rsi
.else
    xorq    %rsi, %rsi
.endif
.if \num == 14
    movq    %rdx, %cr2
.endif
    /* átkapcsolunk taszk független, CPU-nkénti core veremre */
    movq    $LDYN_ccb + __PAGESIZE, %rsp
    xorq    %rdi, %rdi
    movb    $\num, %dil
    callq   \func
    /* visszakapcsolunk taszkonkénti core veremre */
    movq    $TCBFLD_PC, %rsp
    callq   platform_load
    iretq
.endm

/*** kivétel kezelők ***/
    exception  0, 0, exc00divzero,   fault_intr
    exception  1, 0, exc01debug,     fault_dbg
    exception  2, 0, exc02nmi,       fault_intr
    exception  3, 0, exc03chkpoint,  fault_dbg
    exception  4, 0, exc04overflow,  fault_intr
    exception  5, 0, exc05bound,     fault_intr
    exception  6, 0, exc06invopcode, fault_intr
    exception  7, 0, exc07devunavail,fault_intr
    exception  8, 1, exc08dblfault,  fault_intr
    exception  9, 0, exc09coproc,    fault_intr
    exception 10, 1, exc10invtss,    fault_intr
    exception 11, 1, exc11segfault,  fault_intr
    exception 12, 1, exc12stackfault,fault_intr
    exception 13, 1, exc13genprot,   fault_intr
    exception 14, 1, exc14pagefault, fault_pagefault
    exception 15, 0, exc15unknown,   fault_intr
    exception 16, 0, exc16float,     fault_intr
    exception 17, 1, exc17alignment, fault_intr
    exception 18, 0, exc18machinechk,fault_intr
    exception 19, 0, exc19double,    fault_intr
    exception 20, 0, exc20virt,      fault_intr
    exception 21, 0, exc21,          fault_intr
    exception 22, 0, exc22,          fault_intr
    exception 23, 0, exc23,          fault_intr
    exception 24, 0, exc24,          fault_intr
    exception 25, 0, exc25,          fault_intr
    exception 26, 0, exc26,          fault_intr
    exception 27, 0, exc27,          fault_intr
    exception 28, 0, exc28,          fault_intr
    exception 29, 0, exc29,          fault_intr
    exception 30, 1, exc30security,  fault_intr
    exception 31, 0, exc31,          fault_intr
