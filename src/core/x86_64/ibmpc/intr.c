/*
 * core/x86_64/ibmpc/intr.c
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
 * @brief Megszakításvezérlő és Intel 8253 Programmable Interval Timer eszközmeghajtó
 *
 * https://en.wikipedia.org/wiki/Intel_8253
 */

#include <arch.h>

extern void pic_init();
extern void rtc_init();
extern void intr_0();
extern void intr_15();

uint64_t nextsched_remain=0;
char intr_name[] = "PIC";

/**
 * megszakításvezérlő inicializálása
 */
uint intr_init()
{
    void (*fnc)() = &intr_0;
    uint64_t i;

    /* csip inicialializálás */
    pic_init();

    /* IDT mágia. Nem minden megszakítás vezérlő egyforma. */
#if DEBUG
    if((&intr_15-&intr_0)!=15*64) kpanic("Misaligned ISRs");
#endif
    for(i=32; i<48; i++, fnc+=64)
        IDT_GATE(i, IDT_INT, fnc);

    /* falióra inicializálása */
    rtc_init();

    /* ütemező megszakítás inicializálása */
    sched_irq = 0;

    /* 16 IRQ-t tudunk kezelni */
    return 16;
}

/**
 * segédfüggvény, ha a számláló nem képes egy megszakítással usec-et várni
 */
bool_t intr_nextschedremainder()
{
    register uint64_t r;
    if(!nextsched_remain) return false;
    r = nextsched_remain>USHRT_MAX? USHRT_MAX : nextsched_remain;
    nextsched_remain -= r;
    __asm__ __volatile__(
        /* PIT 00 = channel 0, 11 = write lo/hi, 000 = one shot, 0 = binary */
        "movb $(3<<4), %%al; outb %%al, $0x43;"
        "movb %%dl, %%al; outb %%al, $0x40;"
        "movb %%dh, %%al; outb %%al, $0x40": : "d"(r) : "rax");
    return true;
}

/**
 * generál egy megszakítást usec mikroszekundum múlva (PIT one-shot)
 */
void intr_nextsched(uint64_t usec)
{
    /* ez az időzítő egyszerre csak 65536-ig tud számolni */
    if(!usec) usec++;
    if(usec>quantum) usec=quantum;
    nextsched_remain=(1193180*usec)/1000000;
    intr_nextschedremainder();
}
