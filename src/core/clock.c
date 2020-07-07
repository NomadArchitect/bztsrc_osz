/*
 * core/clock.c
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
 * @brief falióra funkciók. Nanoszekundumban számol, de csak 1/clock_freq másodpercenként frissül
 */

#include <core.h>
#include "../drivers/include/driver.h"  /* eszközmeghajtók definíciói, DRV_IRQ miatt */

extern uint64_t env_getts(char *p, int16_t timezone);
extern pid_t **irq_routing_table;
#if DEBUG
extern void kprintf_clock();
#endif

uint64_t clock_freq=0, clock_step;      /* a falióra frekvenciája */
uint64_t clock_cnt, clock_ts, clock_ns; /* számlálók */
uint64_t clock_ticks;                   /* indítás óta eltelt mikroszekundum */
int16_t  clock_tz;                      /* időzóna percekben */
uint64_t clock_pids;                    /* feliratkozott taszkok */

/**
 * falióra inicializálása
 */
void clock_init()
{
    /* aktuális másodpercek beállítása */
    clock_ts = env_getts((char *)&bootboot.datetime, clock_tz) + 1;
    clock_ns = clock_ticks = 0;
    /* falióra léptetésének kiszámítása nanoszekben */
    clock_step = 1000000000000UL/clock_freq;
    clock_cnt = clock_freq;
    clock_pids = 0;
}

/**
 * falióra megszakítás kezelője
 */
void clock_intr(unused uint16_t irq)
{
    pid_t *pid = irq_routing_table[clock_irq];

    /* megszakítás lekezeltségének visszaigazolása */
    clock_ack();
    clock_ticks += clock_step;
    /* még nem telt le egy egész másodperc? */
    if(--clock_cnt) {
        clock_ns += clock_step;
        /* feliratkozott taszkok értesítése, nem egyszerre, szétszórjuk clock Hz-re */
        if(pid && pid[clock_pids]) {
            msg_notify(pid[clock_pids++], DRV_IRQ, USHRT_MAX);
            sched_pick();
        }
    } else {
        clock_ns = 0;
        clock_ts++;
        clock_cnt = clock_freq;
        /* egész másodperckor újraindítjuk a feliratkozott listát, ha végig értünk */
        if(pid && !pid[clock_pids]) clock_pids = 0;
#if DEBUG
        kprintf_clock();
#endif
    }
}
