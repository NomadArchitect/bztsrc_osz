/*
 * fs/fsdrv.h
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
 * @brief fájlrendszer meghajtó nyilvántartás
 */

#include <osZ.h>

#ifndef _FSDRV_H_
#define _FSDRV_H_ 1

/* devfs indexe az fsdrv listában */
extern uint16_t devfsidx;

/* elérési út keresés állapota */
typedef struct {
    fid_t inode;
    fpos_t filesize;
    void *fileblk;
    char *path;
    uint8_t type;
    bool_t creat;
} locate_t;

/* fájlrendszerstruktúra */
typedef struct {
    const char *name;                                                               /* fájlrendszer unix neve */
    const char *desc;                                                               /* teljes név */
    bool_t  (*detect)   (fid_t fd, void *blk);                                      /* azonosító bájtok ellenőrzése */
    uint8_t (*locate)   (fid_t fd, ino_t parent, locate_t *loc);                    /* elérési út keresése a fájlrendszeren */
    void    (*resizefs) (fid_t fd);                                                 /* fájlrendszer átméretezése */
    bool_t  (*checkfs)  (fid_t fd);                                                 /* fájlrendszer konzisztencia ellenőrzése */
    bool_t  (*stat)     (fid_t fd, ino_t file, stat_t *st);                         /* stat struktúra visszaadása */
    size_t  (*getdirent)(void *buf, fpos_t offs, dirent_t *dirent);                 /* dirent struktúra visszaadása*/
    void*   (*read)     (fid_t fd, ino_t file, uint ver, fpos_t offs, size_t *s);   /* olvasás fájlból */
    bool_t  (*writetrb) (fid_t fd, ino_t file);                                     /* írás tranzakció kezdete */
    bool_t  (*write)    (fid_t fd, ino_t file, fpos_t offs, void *blk, size_t s);   /* írás fájlba */
    bool_t  (*writetre) (fid_t fd, ino_t file);                                     /* írás tranzakció vége */
} fsdrv_t;

/* fájlrendszer értelmezők */
extern uint16_t nfsdrv;
extern fsdrv_t *fsdrv;

extern public uint16_t fsdrv_reg(const fsdrv_t *fs);
extern int16_t fsdrv_get(char *name);
extern int16_t fsdrv_detect(fid_t dev);

#if DEBUG
extern void fsdrv_dump();
#endif

#endif
