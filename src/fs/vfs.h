/*
 * fs/vfs.h
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
 * @brief VFS illesztési réteg
 */

#include <osZ.h>
#include "cache.h"
#include "fcb.h"
#include "fsdrv.h"
#include "taskctx.h"

#define ROOTMTAB 0
#define ROOTFCB 0
#define DEVFCB 1
#define DEVPATH "/dev/"
#define PATHSTACKSIZE 64

/* lookup visszatérési értékei */
#define NOBLOCK     1   /* blokk nincs a gyorsítótárban */
#define NOTFOUND    2   /* nincs ilyen fájl */
#define FSERROR     3   /* inkonzisztens fájlrendszer */
#define UPDIR       4   /* a fel könyvtár kivezet az eszközről */
#define FILEINPATH  5   /* fájlrendszer kép az elérési útban */
#define SYMINPATH   6   /* szimbólikus hivatkozás az elérési útban */
#define UNIONINPATH 7   /* unió az elérési útban */
#define NOSPACE     8   /* nincs több szabad hely az eszközön */

#define PATHEND(a) (a==';' || a=='#' || a==0)

#define FSCKSTEP_SB     0
#define FSCKSTEP_ALLOC  1
#define FSCKSTEP_DIR    2
#define FSCKSTEP_NLINK  3
#define FSCKSTEP_DONE  4
typedef struct {
    fid_t dev;
    uint8_t step;
    bool_t fix;
    uint16_t fs;
} fsck_t;

/* fájlrendszer ellenőrzés státusz */
extern fsck_t fsck;

extern uint8_t ackdelayed;      /* aszinkron blokk olvasás jelző */
extern uint32_t pathmax;        /* elérési út maximális hossza */
extern uint8_t *zeroblk;        /* üres blokk */
extern stat_t st;               /* az lstat() és fstat() buffere */
extern dirent_t dirent;         /* a readdir() buffere */
extern char *lastlink;          /* az utolsó szimbolikus hivatkozás célja, az fsdrv stat()-ja tölti */

typedef struct {
    ino_t inode;
    char *path;
} pathstack_t;

/* alacsony szintű funkciók */
extern void pathpush(ino_t lsn, char *path);
extern pathstack_t *pathpop();
extern char *pathcat(char *path, char *filename);
extern char *canonize(const char *path);
extern uint8_t getver(char *abspath);
extern fpos_t getoffs(char *abspath);
extern void *readblock(fid_t fd, blkcnt_t lsn);
public bool_t writeblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio);
/* fájlrendszer meghajtók becsomagolása */
extern fid_t lookup(char *abspath, bool_t creat);
extern stat_t *statfs(fid_t idx);
extern bool_t dofsck(fid_t fd, bool_t fix);
