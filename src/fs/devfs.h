/*
 * fs/devfs.h
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
 * @brief beépített devfs fájlrendszer
 */

#include <osZ.h>

#define MEMFS_MAJOR     0
#define MEMFS_ZERO      0
#define MEMFS_RAMDISK   1
#define MEMFS_RANDOM    2
#define MEMFS_NULL      3

/* eszköz típus */
typedef struct {
    fid_t fid;          /* neve, fcb hivatkozás */
    pid_t drivertask;   /* major */
    dev_t device;       /* minor */
    uint64_t recent;    /* következő eszköz a leggyakrabban használt listában */
    blkcnt_t total;     /* kiirandó blokkok száma */
    blkcnt_t out;       /* kiírt blokkok száma */
} devfs_t;

extern uint32_t ndev;
extern devfs_t *dev;

extern void devfs_init();
extern uint32_t devfs_add(char *name, pid_t drivertask, dev_t device, mode_t mode, blksize_t blksize, blkcnt_t blkcnt);
extern void devfs_del(uint32_t idx);
extern void devfs_used(uint32_t idx);
extern uint32_t devfs_lastused(bool_t all);
extern size_t devfs_getdirent(fpos_t offs);

#if DEBUG
extern void devfs_dump();
#endif
