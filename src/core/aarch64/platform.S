/*
 * core/aarch64/platform.S
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
 * @brief Architektúra függő, platform specifikus függvények
 */

#include <platform.h>

.section .data
nocpu:
.asciz "CPU not supported"
    .balign 4
serrstr:
    .asciz  "SError #%d esr=%x @%x"
    .balign 4

.section .text
/* a debuggolás megkönnyítése végett */
#if DEBUG
.global handler_current_el_sp0
.type handler_current_el_sp0, STT_FUNC
.global handler_current_el_spx
.type handler_current_el_spx, STT_FUNC
.global handler_lower_el_spx
.type handler_lower_el_spx, STT_FUNC
#endif

/**
 * CPU üresjárata
 */
func platform_idle
1:  wfe
    mov     x1, xzr                         /* nem használhatunk inst pool-t, mert nincs olvasási jogunk */
    sub     x1, x1, #1
    movk    x1, #((LDYN_usrccb >> 16) & 0xFFFF), lsl #16
    movk    x1, #((LDYN_usrccb) & 0xFFFF)
    add     x1, x1, #ccb_hasatask           /* megnézzük, hogy tényleg van-e futtatható taszk, vagy csak az eret ébresztett fel */
    mov     w0, wzr
    ldrb    w0, [x1]
    cbz     w0, 1b
    mov     x6, #SYS_yield
    svc     0
    wfe                                     /* az eret sajnos állítja a PE.Event jelzőt, úgyhogy törölni kell egy wfe-el */
    b       1b
endf

/**
 * felébresztés üresjáratból
 */
func platform_awakecpu
    sev
    ret
endf

/**
 * CPU ellenőrzése
 */
func platform_cpu
    mrs     x0, id_aa64mmfr0_el1
    /* 4k felbontás */
    tst     x0, #0xff0000000
    b.ne    1f
    /* legalább 36 bites címtér */
    and     x0, x0, #15
    cbz     x0, 1f
    /* ütemező időzítőt regiszterből tiltjuk IMASK=1, ENABLE=1 */
    mov     x0, #3
    msr     cntp_ctl_el0, x0
    /* nem engedjük, hogy felhasználói szintről felülbírálhassák */
    mov     x0, #0
    msr     cntkctl_el1, x0
    ret
    /* funkció nem támogatott */
1:  ldr     x0, =nocpu
    b       kpanic
    .pool
endf

/**
 * eldönti egy címről, elérhető-e, lehet, hogy nincs leképezve
 *
 * bool_t platform_memfault(void *ptr)
 */
func platform_memfault
    ldr     x1, =sys_fault
    strb    wzr, [x1]
    ldrb    w0, [x0]
    ldrb    w0, [x1]
    ret
endf

/**
 * syscall hívás
 * Be:
 *    funkció: x6
 *    paraméterek: x0, x1, x2, x3, x4, x5
 * Ki:
 *    visszatérési érték: x2:x1
 *    hibakód: x0
 */
func platform_syscall
    /* átkapcsolunk taszk független, CPU-nkénti core veremre */
    ldr     x29, =LDYN_ccb + __PAGESIZE
    mov     sp, x29
    /* üzenet fogadás? */
    cbnz    x6, 1f
    bl      msg_recv
    /* ha nem volt üzenet, akkor blokkolunk és yield() */
    cbnz    x0, 6f
    ldr     x4, =TCBFLD_PC
    ldr     x3, [x4]
    sub     x3, x3, #4
    str     x3, [x4]
    bl      sched_block
    b       2f
1:  cmp     x6, #SYS_yield
    b.ne    3f
2:  bl      sched_pick
    b       5f
3:  /* a core-nak szól az üzenet? */
    lsr     x27, x6, #16
    cbnz    x27, 4f
    bl      msg_core
    b       5f
4:  bl      msg_send
    /* csak akkor szabad felülírni az állapotot, ha történt taszkkapcsolás */
5:  ldr     x4, =LDYN_ccb + ccb_flags
    mov     w3, wzr
    ldrb    w3, [x4]
    strb    wzr, [x4]
    cbnz    w3, handler_back
6:  ldr     x29, =TCBFLD_GPR
    stp     x0, x1, [x29, #8]
    ldr     x0, =LDYN_ccb + ccb_core_errno
    ldr     x0, [x0]
    str     x0, [x29]
    bl      platform_load
    /* visszakapcsolunk taszkonkénti core veremre */
    b       handler_back
endf

/**
 * CPU állapotának lementése
 */
func platform_save
    /* híváskor a veremben kell lennie az x29 és x30 regisztereknek */
    ldr     x29, =TCBFLD_GPR
    stp     x0, x1,   [x29, #16 * 0]
    stp     x2, x3,   [x29, #16 * 1]
    stp     x4, x5,   [x29, #16 * 2]
    stp     x6, x7,   [x29, #16 * 3]
    stp     x8, x9,   [x29, #16 * 4]
    stp     x10, x11, [x29, #16 * 5]
    stp     x12, x13, [x29, #16 * 6]
    stp     x14, x15, [x29, #16 * 7]
    stp     x16, x17, [x29, #16 * 8]
    stp     x18, x19, [x29, #16 * 9]
    stp     x20, x21, [x29, #16 *10]
    stp     x22, x23, [x29, #16 *11]
    stp     x24, x25, [x29, #16 *12]
    stp     x26, x27, [x29, #16 *13]
    /* kivesszük a veremből az x29 és x30 eredeti értékeit */
    ldp     x26, x27, [sp], #16
    stp     x28, x26, [x29, #16 *14]
    str     x27, [x29, #16 *15]
    /* NEON regiszterek */
    ldr     x29, =TCBFLD_FP
    stp     q0, q1,   [x29, #32 * 0]
    stp     q2, q3,   [x29, #32 * 1]
    stp     q4, q5,   [x29, #32 * 2]
    stp     q6, q7,   [x29, #32 * 3]
    stp     q8, q9,   [x29, #32 * 4]
    stp     q10, q11, [x29, #32 * 5]
    stp     q12, q13, [x29, #32 * 6]
    stp     q14, q15, [x29, #32 * 7]
    stp     q16, q17, [x29, #32 * 8]
    stp     q18, q19, [x29, #32 * 9]
    stp     q20, q21, [x29, #32 *10]
    stp     q22, q23, [x29, #32 *11]
    stp     q24, q25, [x29, #32 *12]
    stp     q26, q27, [x29, #32 *13]
    stp     q28, q29, [x29, #32 *14]
    stp     q30, q31, [x29, #32 *15]!
    mrs     x27, fpsr
    mrs     x28, fpcr
    stp     x27, x28, [x29, #32 *0]
    /* eredeti programszámláló és verem lementése */
    ldr     x29, =TCBFLD_PC
    mrs     x27, elr_el1
    mrs     x28, sp_el0
    stp     x27, x28, [x29]
    ret
endf

/**
 * CPU állapotának visszatöltése
 */
func platform_load
    /* eredeti programszámláló és verem visszatöltése */
    ldr     x29, =TCBFLD_PC
    ldp     x0, x1, [x29]
    msr     sp_el0, x1
    msr     elr_el1, x0
    /* NEON regiszterek */
    ldr     x29, =TCBFLD_FP
    ldp     q0, q1,   [x29, #32 * 0]
    ldp     q2, q3,   [x29, #32 * 1]
    ldp     q4, q5,   [x29, #32 * 2]
    ldp     q6, q7,   [x29, #32 * 3]
    ldp     q8, q9,   [x29, #32 * 4]
    ldp     q10, q11, [x29, #32 * 5]
    ldp     q12, q13, [x29, #32 * 6]
    ldp     q14, q15, [x29, #32 * 7]
    ldp     q16, q17, [x29, #32 * 8]
    ldp     q18, q19, [x29, #32 * 9]
    ldp     q20, q21, [x29, #32 *10]
    ldp     q22, q23, [x29, #32 *11]
    ldp     q24, q25, [x29, #32 *12]
    ldp     q26, q27, [x29, #32 *13]
    ldp     q28, q29, [x29, #32 *14]
    ldp     q30, q31, [x29, #32 *15]!
    ldp     x27, x28, [x29, #32 *0]
    msr     fpsr, x27
    msr     fpcr, x28
    /* x29 és x30 verembe helyezése */
    ldr     x29, =TCBFLD_GPR
    ldp     x0, x1, [x29, #16 *14+8]
    stp     x0, x1, [sp, #-16]!
    /* regiszterek betöltése */
    ldp     x0, x1,   [x29, #16 * 0]
    ldp     x2, x3,   [x29, #16 * 1]
    ldp     x4, x5,   [x29, #16 * 2]
    ldp     x6, x7,   [x29, #16 * 3]
    ldp     x8, x9,   [x29, #16 * 4]
    ldp     x10, x11, [x29, #16 * 5]
    ldp     x12, x13, [x29, #16 * 6]
    ldp     x14, x15, [x29, #16 * 7]
    ldp     x16, x17, [x29, #16 * 8]
    ldp     x18, x19, [x29, #16 * 9]
    ldp     x20, x21, [x29, #16 *10]
    ldp     x22, x23, [x29, #16 *11]
    ldp     x24, x25, [x29, #16 *12]
    ldp     x26, x27, [x29, #16 *13]
    ldr     x28, [x29, #16 *14]
    ret
endf

.macro interrupt
    .balign 128
    sub     sp, sp, #16
    stp     x29, x30, [sp, #-16]!
    bl      platform_save
    /* átkapcsolunk taszk független, CPU-nkénti core veremre */
    ldr     x29, =LDYN_ccb + __PAGESIZE
    mov     sp, x29
    bl      intr_getirq
    /* srand[0] += irq; */
    ldr     x1, =srand
    ldr     x2, [x1]
    add     x2, x2, x0
    str     x2, [x1]
    /* sched_intr(irq); */
    cmp     x0, #SCHED_IRQ
    b.ne    1f
    bl      sched_intr
    b       3f
    /* clock_intr(irq); */
1:  cmp     x0, #CLOCK_IRQ
    b.ne    1f
    bl      clock_intr
    b       3f
    /* drivers_intr(irq); */
1:  bl      drivers_intr
3:  bl      kentropy
    /* visszakapcsolunk taszkonkénti core veremre */
    ldr     x29, =__PAGESIZE - 16 /* platform_load berakja a verembe az új taszk x29 és x30 regisztereit */
    mov     sp, x29
    bl      platform_load
    ldp     x29, x30, [sp]
    add     sp, sp, #32
    eret
.endm

.macro serror num
    .balign 128
    adr     x0, serrstr
    mov     x1, #\num
    mrs     x2, esr_el1
    mrs     x3, elr_el1
    b       kpanic
    .pool
.endm

.section .text.aligned
    /* ezt linker szkriptből igazítjuk, mert sajnos a clang elrontja a
     * text szegmens igazítását, ha .align 11-et használnánk */

    /* kivételek */
    .balign 128
func platform_idt
/*** EL1 -> EL1, SP EL0 == SP EL1, SPSel == 0 ***/
handler_current_el_sp0:
    sub     sp, sp, #16
    stp     x29, x30, [sp, #-16]!
    bl      platform_save
    mrs     x0, esr_el1
    /* ha a kivétel a debuggerben vagy a kernelben történt, akkor csak a sys_fault-ot állítjuk */
/*
#if DEBUG
    ldr     w26, dbg_indump
    tbnz    w26, #0, 1f
#endif
    ldr     x26, =CORE_ADDRESS
    cmp     x27, x26
    b.lt    2f
1:  ldr     x26, =sys_fault
    lsr     w0, w0, #26
    strb    w0, [x26]
    b       3f
*/
2:  mrs     x1, far_el1
    mrs     x2, spsr_el1
    bl      fault_intr
3:  bl      platform_load
    ldp     x29, x30, [sp]
    add     sp, sp, #32
    eret
    .pool

    /* IRQ-k */
    interrupt

    /* FIQ-k (nem használt) */
    interrupt

    /* SError, olyan, mint az NMI, nem tehetünk sok mindent, ha bekövetkezik */
    serror  0

/*** EL1 -> EL1, SP EL0 != SP EL1, SPSel == 1 ***/
    .balign 128
handler_current_el_spx:
    /* a kivétel a debuggerben vagy a kernelben történt, akkor csak a sys_fault-ot állítjuk */
    sub     sp, sp, #16
    stp     x27, x28, [sp, #-16]!
    mrs     x27, esr_el1
    ldr     x28, =sys_fault
    lsr     w27, w27, #26
    strb    w27, [x28]
    mrs     x27, elr_el1
    add     x27, x27, #4
    msr     elr_el1, x27
    mov     x0, xzr
    ldp     x27, x28, [sp]
    add     sp, sp, #32
    eret
    .pool

    /* IRQ-k */
    interrupt

    /* FIQ-k (nem használt) */
    interrupt

    /* SError, olyan, mint az NMI, nem tehetünk sok mindent, ha bekövetkezik */
    serror  1

/*** EL0 -> EL1, SP EL0 != SP EL1, SPSel == 1, 64 bit ***/
    .balign 128
handler_lower_el_spx:
    /* a kivétel felhasználói kód futtatása közben történt */
    sub     sp, sp, #16
    stp     x29, x30, [sp, #-16]!
    bl      platform_save
    /* SVC utasításból jöttünk? */
    mrs     x29, esr_el1
    lsr     w29, w29, #26
    cmp     w29, #0x15
    b.ne    1f
    b       platform_syscall
    /* sima kivétel kezelő */
1:  mrs     x0, esr_el1
    mrs     x1, far_el1
    mrs     x2, spsr_el1
    /* átkapcsolunk taszk független, CPU-nkénti core veremre */
    ldr     x29, =LDYN_ccb + __PAGESIZE
    mov     sp, x29
    bl      fault_intr
    /* átléptetjük az utasítást */
/*
    ldr     x29, =TCBFLD_PC
    ldr     x28, [x29]
    add     x28, x28, #4
    str     x28, [x29]
*/
    /* visszakapcsolunk taszkonkénti core veremre */
handler_back:
    ldr     x29, =__PAGESIZE - 16 /* platform_load berakja a verembe az új taszk x29 és x30 regisztereit */
    mov     sp, x29
    bl      platform_load
    ldp     x29, x30, [sp]
    add     sp, sp, #32
    eret
    .pool

    /* IRQ-k */
    interrupt

    /* FIQ-k (nem használt) */
    interrupt

    /* SError, olyan, mint az NMI, nem tehetünk sok mindent, ha bekövetkezik */
    serror  2

/*** EL0 -> EL1, SP EL0 != SP EL1, SPSel == 1, 32 bit ***/
    /* nem támogatott, nem használunk 32 bites módot */
endf
