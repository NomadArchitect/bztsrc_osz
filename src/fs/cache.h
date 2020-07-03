/*
 * fs/cache.h
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
 * @subsystem fájlrendszer
 * @brief blokk gyorsítótár
 */

#include <osZ.h>

/* blokk visszaírási prioritás (soft update) */
#define BLKPRIO_NOTDIRTY 0  /* olvasási művelet */
#define BLKPRIO_CRIT     1  /* szuperblokk és blokk allokálás */
#define BLKPRIO_DATA     2  /* fájl adat */
#define BLKPRIO_MAPPING  3  /* adatleképezés */
#define BLKPRIO_META     4  /* inode információ */

typedef uint8_t blkprio_t;

extern void cache_init();
extern void *cache_getblock(fid_t fd, blkcnt_t lsn);
extern bool_t cache_setblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio);
extern blkcnt_t cache_freeblocks(fid_t fd, blkcnt_t needed);
extern bool_t cache_cleardirty(fid_t fd, blkcnt_t lsn);
extern void cache_flush();
extern void cache_free();

#if DEBUG
extern void cache_dump();
#endif
