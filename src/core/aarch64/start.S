/*
 * core/aarch64/start.S
 *
 * Copyright (c) 2016 bzt (bztsrc@gitlab)
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
 * @brief Belépési pont
 */

#include <arch.h>

.section .text
.global _start
.type _start, STT_FUNC

/**********************************************************************
 *                   OS/Z betöltő belépési pontja                     *
 **********************************************************************/
_start:
    /* verem választó engedélyezése */
    mov     x0, sp
    msr     SPSel, #1
    mov     sp, x0
    mov     x0, #0x3c4
    msr     spsr_el1, x0
    /* megszakításkezelő rutinok betöltése */
    ldr     x0, =platform_idt
    msr     vbar_el1, x0

    /* helyet csinálunk a CPU-nkénti ttbr1_el1-nek és SysV ABI szerint igazítjuk a vermet */
    stp     xzr, xzr, [sp, #-16]!

    ldr     x0, =numcores
1:  ldaxr   w1, [x0]
    add     w1, w1, #1
    stlxr   w2, w1, [x0]
    cbnz    w2, 1b

    /* AP-k leállítása egyelőre */
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    /* ugrás a C nyelvű main() funkcióra a 64 bites kódszegmensben */
    cbz     x0, main
    /* várakozás, míg RUNLVL_COOP szintet el nem érjük, aztán ugrás ap_start-ra */
    ldr     x1, =runlevel
1:  dsb     sy
    ldr     w0, [x1]
    and     w0, w0, #0xff
    cmp     w0, #RUNLVL_COOP
    b.ne    1b
    /* futás folytatódik az ap_start-on, nem kell branch */

/**********************************************************************
 *            Alkalmazás Processzor(ok) belépési pontja               *
 **********************************************************************/
ap_start:
    /* mair_el1 = 0x4404ff; */
    mov     x0, xzr
    mov     w0, #0x04FF
    movk    w0, #0x44, lsl #16
    msr     mair_el1, x0
    /* CPU-nkénti memórialeképezés betöltése */
    ldr     x0, [sp]
    msr     ttbr1_el1, x0
    tlbi    vmalle1
    dsb     ish
    /* sctlr_el1 |= (1<<19)|(1<<12)|(1<<2); WXN, I utasítás és C adat gyorsítótár engedélyezése */
    mrs     x0, sctlr_el1
    mov     x1, #0x1004
    movk    x1, #0x8, lsl #16
    orr     x0, x0, x1
    msr     sctlr_el1, x0
    isb
    /* átkapcsolunk taszk független, CPU-nkénti core veremre */
    ldr     x29, =LDYN_ccb + __PAGESIZE
    mov     sp, x29
    /* engedélyezzük a processzorspecifikus funkciókat */
    bl      platform_cpu
    /* kiválasztunk egy taszkot futásra */
    bl      sched_pick
    /* átadjuk rá a vezérlést */
    ldr     x29, =__PAGESIZE - 16
    mov     sp, x29
    bl      platform_load
    ldp     x29, x30, [sp]
    add     sp, sp, #32
    msr     spsr_el1, xzr
    eret
