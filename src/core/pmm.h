/*
 * core/pmm.h
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
 * @subsystem memória
 * @brief Fizikai Memória Kezelő
 */

#ifndef _AS

typedef struct {
    phy_t base;
    uint64_t size;
} pmm_entry_t;

/*** RAM foglaltság állapota ***/
extern uint64_t pmm_totalpages;
extern uint64_t pmm_freepages;

/*** Funkciók ***/
void pmm_init();
void pmm_vmem();
void *pmm_alloc(tcb_t *tcb, uint64_t pages);
void *pmm_allocslot(tcb_t *tcb);
void pmm_free(tcb_t *tcb, phy_t base, size_t numpages);
#if DEBUG
void pmm_dump();
#endif

#endif