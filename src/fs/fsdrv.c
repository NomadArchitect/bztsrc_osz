/*
 * fs/fsdrv.c
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
 * @brief Fájl Rendszer Meghajtók kezelése
 */

#include <osZ.h>
#include "fsdrv.h"
#include "vfs.h"

/* devfs indexe az fsdrv listában */
uint16_t devfsidx = (uint16_t)-1;

/* fájlrendszerek */
uint16_t nfsdrv = 0;
fsdrv_t *fsdrv = NULL;

/**
 * fájlrendszer értelmező regisztrálása
 */
public uint16_t fsdrv_reg(const fsdrv_t *fs)
{
    bool_t isdevfs;
    /* paraméterek ellenőrzése */
    if(!fs->name || !fs->name[0]) return (uint16_t)-1;
    /* csak a devfs-nek engedjük meg, hogy ne legyen detect és locate funkciója */
    isdevfs = !memcmp(fs->name, "devfs", 6);
    if(!isdevfs && (!fs->detect || !fs->locate)) return (uint16_t)-1;

    fsdrv = (fsdrv_t*)realloc(fsdrv, ++nfsdrv * sizeof(fsdrv_t));
    /* kilépünk, ha nem sikerült lefoglalni */
    if(!fsdrv || errno()) exit(2);
    memcpy((void*)&fsdrv[nfsdrv-1], (void*)fs, sizeof(fsdrv_t));
    /* az új devfs index megállapítása */
    if(devfsidx == (uint16_t)-1 && isdevfs)
        devfsidx = nfsdrv-1;
    return nfsdrv-1;
}

/**
 * fájlrendszer értelmező kikeresése név alapján
 */
int16_t fsdrv_get(char *name)
{
    uint16_t i;

    for(i = 0; i < nfsdrv; i++)
        if(!strcmp(fsdrv[i].name, name))
            return i;
    return (uint16_t)-1;
}

/**
 * fájlrendszer detektálása eszközön vagy fájlban
 */
int16_t fsdrv_detect(fid_t dev)
{
    uint16_t i;
    uint8_t *blk;

    /* kis csalás, mivel a devfs-nek nincs szuperblokkja */
    if(dev == DEVFCB)
        return devfsidx;

    /* szuperblokk beolvasása */
    blk = readblock(dev, 0);
    if(blk) {
        /* sorra meghívjuk a fájlrendszer meghajtók detektáló funkcióját */
        for(i = 0; i < nfsdrv; i++) {
            if(fsdrv[i].detect && (*fsdrv[i].detect)(dev, blk))
                return i;
        }
    }
    return (uint16_t)-1;
}

#if DEBUG
/**
 * fájlrendszer meghajtók dumpolása debuggoláshoz
 */
void fsdrv_dump()
{
    uint16_t i;

    dbg_printf("File system drivers %d:\n", nfsdrv);
    for(i = 0; i < nfsdrv; i++) {
        dbg_printf("%3d. '%s' %s %x:", i, fsdrv[i].name, fsdrv[i].desc, fsdrv[i].detect);
        if(i == devfsidx) {
            dbg_printf(" (built-in)\n");
        } else {
            if(fsdrv[i].detect) dbg_printf(" detect");
            if(fsdrv[i].locate) dbg_printf(" locate");
            if(fsdrv[i].resizefs) dbg_printf(" resizefs");
            if(fsdrv[i].checkfs) dbg_printf(" checkfs");
            if(fsdrv[i].stat) dbg_printf(" stat");
            if(fsdrv[i].read) dbg_printf(" read");
            if(fsdrv[i].write) dbg_printf(" write");
            if(fsdrv[i].getdirent) dbg_printf(" getdirent");
            dbg_printf("\n");
        }
    }
}
#endif
