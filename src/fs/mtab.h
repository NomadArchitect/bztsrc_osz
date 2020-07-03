/*
 * fs/mtab.h
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
 * @brief felcsatolt fájlrendszerek kezelése
 */

#include <osZ.h>

#define MTAB_FLAG_ASYNC        (1<<0)

/* mtab struktúra */
typedef struct {
    fid_t mountpoint;   /* felcsatolási pont fcb indexe */
    fid_t storage;      /* eszköz fcb indexe */
    uint64_t fstype;    /* fsdrv index, fájlrendszer meghajtó */
    uint16_t len;       /* hozzáférési lista hossza */
    gid_t *grp;         /* hozzáférési lista */
} mtab_t;

/* felcsatolási pontok */
extern uint16_t nmtab;
extern mtab_t *mtab;

extern void mtab_fstab(char *ptr, size_t size);
extern void mtab_init();
extern uint16_t mtab_add(char *dev, char *file, char *opts);
extern bool_t mtab_del(char *dev, char *file);

#if DEBUG
extern void mtab_dump();
#endif
