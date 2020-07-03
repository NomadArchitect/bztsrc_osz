/*
 * fs/devfs.c
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
 * @brief beépített eszköz fájlrendszer (devfs)
 */

#include <osZ.h>
#include <osZ/fsZ.h>
#include "mtab.h"
#include "devfs.h"
#include "fsdrv.h"
#include "fcb.h"
#include "vfs.h"

/* eszközlista */
uint32_t ndev = 0;
devfs_t *dev = NULL;

uint64_t dev_recent = 0;
uint32_t *devidx = NULL;

/* segédfüggvény komparáláshoz */
int devfs_cmp(void *a, void *b)
{
    if(*(uint32_t*)a == -1U) return 1;
    if(*(uint32_t*)b == -1U) return -1;
    return strcmp(fcb[dev[*(uint32_t*)b].fid].abspath, fcb[dev[*(uint32_t*)a].fid].abspath);
}

/**
 * eszköz hozzáadása
 */
uint32_t devfs_add(char *name, pid_t drivertask, dev_t device, mode_t mode, blksize_t blksize, blkcnt_t blkcnt)
{
    uint i, s;
    fid_t f;
    char tmp[64 + sizeof(DEVPATH)] = DEVPATH;

    strncpy(&tmp[5], name, 64);
    for(i = 0; i < ndev; i++)
        if(!strcmp(fcb[dev[i].fid].abspath, tmp))
            return i;
    ndev++;
    fcb[1].data.reg.filesize = s = ndev * sizeof(devfs_t);
    dev = (devfs_t*)realloc(dev, s);
    devidx = realloc(devidx, ndev * sizeof(uint32_t));
    /* kilépés, ha nem sikerült a devfs-t allokálni */
    if(!dev || !devidx) exit(2);
    f = fcb_add(tmp, FCB_TYPE_DEVICE);
    fcb[f].nopen++;
    fcb[f].mode = mode & O_AMSK;
    fcb[f].data.device.inode = i;
    fcb[f].data.device.blksize = blksize;
    fcb[f].data.device.filesize = blksize * blkcnt;
    fcb[f].data.device.storage = DEVFCB;
    dev[i].fid = f;
    dev[i].drivertask = drivertask;
    dev[i].device = device;
    dev[i].recent = ++dev_recent;
    devidx[i] = i;
    qsort(devidx, ndev, sizeof(uint32_t), devfs_cmp);
    return i;
}

/**
 * eszköz eltávolítása
 */
void devfs_del(uint32_t idx)
{
    uint i;

    if(idx >= ndev) return;

    /* kivesszük az mtab-ból */
    for(i = 0; i < nmtab; i++)
        if(mtab[i].storage == dev[idx].fid) {
            fcb_del(mtab[i].mountpoint);
            nmtab--;
            memcpy(&mtab[i], &mtab[i+1], (nmtab-i)*sizeof(mtab_t));
            mtab = (mtab_t*)realloc(mtab, nmtab*sizeof(mtab_t));
        }
    /* kiveszzük az összes fájlt a listából, amik ezen az eszközön találhatók */
    for(i = 0; i < nfcb; i++)
        if(fcb[i].type < FCB_TYPE_PIPE && fcb[i].data.reg.storage == dev[idx].fid) {
            fcb[i].nopen = 0;
            fcb_del(i);
        }
    /* kivesszük a gyorsítótárból */
    cache_freeblocks(dev[idx].fid, 0);
    /* kivesszük az eszközt a fájl listából */
    fcb[dev[idx].fid].nopen = 0;
    fcb_del(dev[idx].fid);
    /* eltávolítjuk a devfs-ből */
    dev[idx].fid = -1U;
    for(i = ndev; i > 0 && dev[i-1].fid == -1U; i--);
    if(i != ndev) {
        ndev = i;
        dev = (devfs_t*)realloc(dev, ndev * sizeof(devfs_t));
    }
    for(i = 0; i < ndev; i++)
        if(devidx[i] == idx) devidx[i] = -1U;
    qsort(devidx, ndev, sizeof(uint32_t), devfs_cmp);
    devidx = (uint32_t*)realloc(devidx, ndev * sizeof(uint32_t));
    /* a taskctx-eket nem vesszük ki, mivel a következő műveletnél úgyis feof()-ot vagy ferror()-t adnak */
}

/**
 * eszköz használtnak jelölése
 */
void devfs_used(uint32_t idx)
{
    uint32_t i;

    if(idx >= ndev) return;
    /* túlcsordulás védelem a biztonság kedvéért, nemmintha egyhamar elérnénk :-) */
    if(dev_recent > 0xFFFFFFFFFFFFFFFUL) {
        dev_recent >>= 48;
        for(i = 0; i < ndev; i++)
            if(dev[i].fid != -1U) {
                dev[i].recent >>= 48;
                dev[i].recent++;
            }
    }
    dev[idx].recent = ++dev_recent;
}

/**
 * legrégebben használt eszközt adja vissza
 */
uint32_t devfs_lastused(bool_t all)
{
    uint32_t i, l = 0, k = dev_recent;

    for(i = 0; i < ndev; i++)
        if(dev[i].fid != -1U && dev[i].recent < k && (all || dev[i].recent)) {
            k = dev[i].recent;
            l = i;
        }
    if(!all)
        dev[l].recent = 0;
    return l;
}

/**
 * dirent_t visszaadása egy devfs bejegyzéshez
 */
public size_t devfs_getdirent(fpos_t offs)
{
    fcb_t *f;
    size_t s = 1;

    /* üres helyek kezelése */
    while(offs != -1U && offs < ndev && devidx[offs] != -1U && dev[devidx[offs]].fid == -1U) { s++; offs++; }
    /* EOF kezelése */
    if(offs >= ndev || devidx[offs] == -1U) return 0;
    /* dirent struktúra mezőinek kitöltése */
    f = &fcb[dev[devidx[offs]].fid];
    dirent.d_dev = DEVFCB;
    dirent.d_ino = offs;
    memcpy(&dirent.d_icon, "dev", 3);
    dirent.d_filesize = f->data.device.filesize;
    dirent.d_type = FCB_TYPE_DEVICE | (f->data.device.blksize <= 1? S_IFCHR>>16 : 0);
    strcpy(dirent.d_name, f->abspath + strlen(DEVPATH));
    dirent.d_len = strlen(dirent.d_name);
    return s;
}


/**
 * devfs inicializálása
 */
void devfs_init()
{
    size_t s;
    FSZ_SuperBlock *sb = (FSZ_SuperBlock*)BUF_ADDRESS;

    fsdrv_t devdrv = {
        "devfs",
        "Device list",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };
    fsdrv_reg(&devdrv);

    s = (sb->numsec * (1 << (sb->logsec+11)) + __PAGESIZE-1) >> __PAGEBITS;
    fcb_add(DEVPATH, FCB_TYPE_REG_DIR);
    if(devfs_add("zero",   MEMFS_MAJOR, MEMFS_ZERO,    O_RDONLY, __PAGESIZE, 1) == -1U) exit(2);
    if(devfs_add("root",   MEMFS_MAJOR, MEMFS_RAMDISK, O_RDWR,   __PAGESIZE, s) == -1U) exit(2);
    if(devfs_add("random", MEMFS_MAJOR, MEMFS_RANDOM,  O_RDONLY, __PAGESIZE, 1) == -1U) exit(2);
    if(devfs_add("null",   MEMFS_MAJOR, MEMFS_NULL,    O_WRONLY, __PAGESIZE, 0) == -1U) exit(2);
}

#if DEBUG
/**
 * eszközlista dumpolása, debuggoláshoz
 */
void devfs_dump()
{
    uint i;

    dbg_printf("Devices %d:\n", ndev);
    for(i = 0; i < ndev; i++) {
        if(dev[i].fid == -1U) continue;
        dbg_printf("%3d.(%3d) pid %02x dev %x mode %2x blk %d",
            i, devidx[i], dev[i].drivertask,dev[i].device, fcb[dev[i].fid].mode, fcb[dev[i].fid].data.device.blksize);
        dbg_printf(" size %d %d %s\n", fcb[dev[i].fid].data.device.filesize, dev[i].fid, fcb[dev[i].fid].abspath);
    }
}
#endif
