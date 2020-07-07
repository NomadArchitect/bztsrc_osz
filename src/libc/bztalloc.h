/*
 * libc/bztalloc.h
 *
 * Copyright (C) 2016 bzt (bztsrc@gitlab)
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
 * @brief Memória allokátor és deallokátor header
 */

#ifndef _BZTALLOC_H_
#define _BZTALLOC_H_ 1

#if defined(_OS_Z_) || defined(_OSZ_CORE_)
#include <osZ/mman.h>
#else
#include <sys/mman.h>
#endif

#define ALLOCSIZE     __SLOTSIZE                                /* allokációs egység 2M */
#define ARENASIZE     (ALLOCSIZE)                               /* hely 256k mutatónak */
#define MAXARENA      ((ARENASIZE/sizeof(void*))-1)             /* aréna maximális mérete */
#define BITMAPSIZE    (ALLOCSIZE/__PAGESIZE/sizeof(void*)/8)    /* allokációs blokkok száma egy csomagban */
#define numallocmaps(a) (a[0]&0x7ffffffff)                      /* allokációs térképek száma egy arénában */
#define chunksize(q)  ((q*BITMAPSIZE*sizeof(void*)*8/__PAGESIZE)*__PAGESIZE)  /* csomag mérete a quantum függvényében */

#ifndef _AS

typedef struct {
    uint64_t quantum;               /* foglalási egység ebben a csomagban */
    void *ptr;                      /* memória mutató (lap vagy blokk címhelyes) */
    uint64_t map[BITMAPSIZE];       /* szabad egységek ebben a csomagban (512 bit), vagy az első bejegyzés mérete */
} chunkmap_t;

/* ennek működnie kell üres lappal, ezért nincs azonosító és a 0 bite jelenti a szabadot */
typedef struct {
    uint64_t numchunks;             /* csomagok azaz chunkmap_t elemszáma */
    chunkmap_t chunk[1];            /* csomagok, dinamikusan foglalt */
} allocmap_t;

/*** Prototípusok ***/
extern __attribute__((malloc)) void *bzt_alloc(uint64_t *arena, size_t a, void *ptr, size_t s, int flag);
extern void bzt_free(uint64_t *arena, void *ptr);
#if DEBUG
void dbg_bztdump(uint64_t *arena);
#endif

#endif

#endif
