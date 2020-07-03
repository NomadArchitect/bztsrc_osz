/*
 * core/x86_64/ibmpc/pic.S
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
 * @subsystem megszakítás
 * @brief i8259A Programmable Interrupt Controller megszakításvezérlő eszközmeghajtó
 *
 * https://en.wikipedia.org/wiki/Intel_8259
 */

#include <platform.h>

.section .text
.global intr_0
.type intr_0, STT_FUNC
.global intr_15
.type intr_15, STT_FUNC

/**
 * csip inicializálás
 *
 * void pic_init();
 */
func pic_init
    /* minden IRQ letiltása */
    movb    $0xFF, %al
    outb    %al, $PIC_SLAVE_DATA
    btrw    $2, %ax     /* kaszkád irq 2 engedélyezése */
    outb    %al, $PIC_MASTER_DATA
    /* NMI engedélyezése */
    inb     $0x70, %al
    btrw    $8, %ax
    outb    %al, $0x70
    ret
endf

/**
 * egy adott IRQ vonal engedélyezése
 *
 * void intr_enable(uint16_t irq);
 */
func intr_enable
    movl    %edi, %ecx
    andw    $0xF, %cx
    cmpb    $8, %cl
    jae     1f
    inb     $PIC_MASTER_DATA, %al
    btrw    %cx, %ax
    outb    %al, $PIC_MASTER_DATA
    ret
1:  subb    $8, %cl
    inb     $PIC_SLAVE_DATA, %al
    btrw    %cx, %ax
    outb    %al, $PIC_SLAVE_DATA
    ret
endf

/**
 * egy adott IRQ vonal letiltása
 *
 * void intr_disable(uint16_t irq);
 */
func intr_disable
    cmpl    $2, %edi
    je      1f
    movl    %edi, %ecx
    andw    $0xF, %cx
    cmpl    $8, %edi
    jae     2f
    inb     $PIC_MASTER_DATA, %al
    btsw    %cx, %ax
    outb    %al, $PIC_MASTER_DATA
1:  ret
2:  subb    $8, %cl
    inb     $PIC_SLAVE_DATA, %al
    btsw    %cx, %ax
    outb    %al, $PIC_SLAVE_DATA
    ret
endf

.macro interrupt num, func
    .balign 64
intr_\num:
    call    platform_save
    /* átkapcsolunk taszk független, CPU-nkénti core veremre */
    movq    $LDYN_ccb + __PAGESIZE, %rsp
    xorq    %rdi, %rdi
    movb    $\num, %dil
    /* srand[0] += irq; */
    addq    %rdi, srand
    /* drivers_intr(irq); */
    callq   \func
    callq   kentropy
    /* EOI, megszakításkezelő vége jelzés */
    movb    $0x20, %al
.if \num > 7
    outb    %al, $PIC_SLAVE
.endif
    outb    %al, $PIC_MASTER
    /* visszakapcsolunk taszkonkénti core veremre */
    movq    $TCBFLD_PC, %rsp
    callq   platform_load
    iretq
.endm

/*** megszakítás kezelők ***/
    interrupt  0, sched_intr        /* ütemező megszakítása */
    interrupt  1, drivers_intr
    interrupt  2, drivers_intr
    interrupt  3, drivers_intr
    interrupt  4, drivers_intr
    interrupt  5, drivers_intr
    interrupt  6, drivers_intr
    interrupt  7, drivers_intr
    interrupt  8, clock_intr        /* falióra megszakítása */
    interrupt  9, drivers_intr
    interrupt 10, drivers_intr
    interrupt 11, drivers_intr
    interrupt 12, drivers_intr
    interrupt 13, drivers_intr
    interrupt 14, drivers_intr
    interrupt 15, drivers_intr
