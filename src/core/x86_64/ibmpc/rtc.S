/*
 * core/x86_64/ibmpc/rtc.S
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
 * @brief Motorola MC146818 Real Time Clock eszközmeghajtó, periodikus falióra
 *
 * https://pdf.datasheetcatalog.com/datasheet/motorola/MC146818AS.pdf
 */

#include <arch.h>

.section .text
.global rtc_init
.type rtc_init, STT_FUNC
.global clock_ack
.type clock_ack, STT_FUNC

/**
 * valós idejű óra inicializálása
 *
 * void rtc_init()
 */
func rtc_init
    /* átprogramozzuk 1024Hz-re */
    movb    $0x8A, %ah
    movb    %ah, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    andb    $0xF0, %al
    orb     $6, %al
    xchg    %ah, %al
    outb    %al, $0x70
    jmp     1f
1:  movb    %ah, %al
    outb    %al, $0x71

    /* periodikus IRQ8 bekapcsolása az RTC-ben */
    movb    $0x8B, %ah
    movb    %ah, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    orb     $0x40, %al
    xchgb   %ah, %al
    outb    %al, $0x70
    jmp     1f
1:  movb   %ah, %al
    outb    %al, $0x71
    jmp     1f
1:
    movw    $1024, clock_freq
    movw    $8, clock_irq
    ret
endf

/**
 * falióra megszakítás visszaigazolása
 *
 * void clock_ack();
 */
func clock_ack
    movb    $0x0C, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    /* NMI engedélyezése */
    xorb    %al, %al
    outb    %al, $0x70
    ret
endf
