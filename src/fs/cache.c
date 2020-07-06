/*
 * fs/cache.c
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
#include <driver.h>
#include "taskctx.h"
#include "cache.h"
#include "fcb.h"
#include "devfs.h"

extern uint8_t ackdelayed;      /* aszinkron olvasás jelzőbit */
uint32_t cachelines;            /* gyorsítótár vonalak száma */
uint32_t cachelimit;            /* cache max mérete RAM százalékban */
uint64_t cache_inflush=0;       /* kiirandó blokkok száma */

#define CACHE_FD(x) (x&0xFFFFFFFFFFFFFFFUL)
#define CACHE_PRIO(x) (x>>60)

/* sizeof=32 */
typedef struct {
    fid_t fd;
    blkcnt_t lsn;
    void *blk;
    void *next;
} cache_t;

/* sizeof=cachelines*8 */
cache_t **cache;

/**
 * blokkgyorsítótár inicializálása
 */
void cache_init()
{
    cachelimit = env_num("cachelimit", 5, 1, 50);
    cachelines = env_num("cachelines", 16, 16, 65535);
    cache = (cache_t**)malloc(cachelines * sizeof(cache_t*));
    /* ha nem tudtuk lefoglalni, akkor kilépünk */
    if(!cache || errno()) exit(2);
    cache_inflush = 0;
}

/**
 * blokk beolvasása a gyorsítótárból, beállítja az ackdelayed-et ha nincs a blokk a tárban
 */
public void* cache_getblock(fid_t fd, blkcnt_t lsn)
{
    uint64_t i,j=lsn%cachelines;
    cache_t *c,*l=NULL;
    blksize_t bs;

#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_getblock(fd %d, sector %d)\n",fd,lsn);
#endif
    /* paraméterek ellenőrzése */
    if(fd >= nfcb || lsn == -1U || fcb[fd].type != FCB_TYPE_DEVICE || !cache[j]) return NULL;
    if((lsn+1) * fcb[fd].data.device.blksize > fcb[fd].data.device.filesize) { seterr(EFAULT); return NULL; }
    /* gyorsítótárvonal végigjárása */
    for(c = cache[j]; c; c = c->next) {
        if(CACHE_FD(c->fd) == fd && c->lsn == lsn) {
            /* lánc elejére mozgatjuk a blokkot */
            if(l) l->next = c->next;
            c->next = cache[j];
            cache[j] = c;
            return c->blk;
        }
        l = c;
    }
    /* nincs a blokk a gyorsítótárban, szólunk az eszközmeghajtónak, hogy töltse be */
    i = fcb[fd].data.device.inode;
    /* az első szektor esetében többet olvasunk be, hogy tudjuk detektálni a fájlrendszer típusát */
    bs = fcb[fd].data.device.blksize;
    if(!lsn && bs < BUFSIZ) bs = BUFSIZ;
    mq_send5(dev[i].drivertask, DRV_read, dev[i].device, lsn, bs, ctx->pid, i);
    /* válasz késleltetése és a kérő blokkolása. Majd felébresztésre kerül, ha megjött az adat */
    ackdelayed = true;
    return NULL;
}

/**
 * blokk letárolása a gyorsítótárba, eszközmeghajtók üzenetére
 */
public bool_t cache_setblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)
{
    uint64_t j = lsn%cachelines, bs;
    cache_t *c, *l = NULL, *nc = NULL;

#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_setblock(fd %d, sector %d, buf %x, prio %d)\n",fd,lsn,blk,prio);
#endif
    /* paraméterek ellenőrzése */
    if(fd >= nfcb || lsn == -1U || fcb[fd].type != FCB_TYPE_DEVICE) return false;
    /* blokk méret */
    bs = fcb[fd].data.device.blksize;
    if(bs <= 1) return false;
    if((lsn+1) * bs > fcb[fd].data.device.filesize) { seterr(EFAULT); return false; }
    /* már benne van a gyorsítótárban? */
    for(c = cache[j]; c; c = c->next) {
        if(CACHE_FD(c->fd) == fd && c->lsn == lsn) {
            if(l) l->next = c->next;
            nc = c;
            break;
        }
        l = c;
    }
    /* új gyorsítótár blokk allokálása, ha még nincs */
    if(!nc) {
        nc = (cache_t*)malloc(sizeof(cache_t));
        if(!nc) return false;
        nc->blk = (void*)malloc(bs);
        if(!nc->blk) return false;
        nc->lsn = lsn;
    }
    /* lánc elejére fűzzük */
    nc->next = cache[j];
    cache[j] = nc;
    /* gyorsítótárblokk frissítése */
    nc->fd = CACHE_FD(fd) | ((uint64_t)(prio & 0xFUL)<<60);
    memcpy(nc->blk, blk, bs);
    return true;
}

/**
 * egy adott eszköz összes blokkjának felszabadítása
 */
public blkcnt_t cache_freeblocks(fid_t fd, blkcnt_t needed)
{
    uint64_t i, j, k;
    blkcnt_t cnt = 0;
    cache_t *c, *l, **r = NULL;

#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_freeblocks(fd %d, needed %d)\n",fd,needed);
#endif
    /* paraméterek ellenőrzése */
    if(fd >= nfcb || fcb[fd].type != FCB_TYPE_DEVICE || fcb[fd].data.device.blksize <= 1) return 0;
    /* indulásnak feltesszük 512 blokk van egy vonalon, úgyhogy egy lapot foglalunk */
    k = 512; r = (cache_t**)malloc(k*sizeof(cache_t*));
    if(!r) goto failsafe;
    /* fordított sorrend, hogy a szuperblokkot a legvégén szabadítsuk fel */
    for(j = cachelines-1; (int)j >= 0 && (!needed || cnt < needed); j--) {
        if(!cache[j]) continue;
        /* úgy döntöttem, csak egyirányú láncolt listát használok, azért most fel kell építeni az indexet */
        for(i = 0, c = cache[j]; c; c = c->next) {
            r[i++] = c;
            /* átméretezzük, ha kell */
            if(i + 1 >= k) {
                k <<= 1; r = (cache_t**)realloc(r, k*sizeof(cache_t*));
                if(!r) goto failsafe;
            }
        }
        /* blokkok felszabadítása, fordított sorrendben, hogy a legrégebben használt szabaduljon fel először */
        for(i--; (int)i >= 0; i--) {
            c = r[i];
            if(CACHE_FD(c->fd) == fd) {
                if(i > 0)
                    r[i-1]->next = c->next;
                else
                    cache[j] = c->next;
                free(c->blk);
                free(c);
                cnt++;
            }
        }
    }
    free(r);
    return cnt;
    /* biztonsági megoldás. Nem igényel plusz memóriát, de nem is garantálja, hogy helyes
     * sorrendben szabadítja fel a blokkokat. Jobb, mint a semmi, ha nincs elég memória. */
failsafe:
    for(j = cachelines-1; (int)j >= 0 && (!needed || cnt<needed); j--) {
        if(!cache[j]) continue;
        l = cache[j];
        for(c = cache[j]; c ; c = c->next) {
            if(CACHE_FD(c->fd) == fd) {
                l->next = c->next;
                l = c->next;
                free(c->blk);
                free(c);
                c = l;
                cnt++;
            }
            l = c;
        }
    }
    return cnt;
}

/**
 * blokk gyorsítótár kiírása az eszközökre
 */
void cache_flush()
{
    uint64_t i, k, p;
    fid_t f;
    cache_t *c;

    cache_inflush = 0;
    /* fontos, hogy a megfelelő prioritási sorrendben írjuk ki a blokkokat */
    for(p = BLKPRIO_CRIT; p <= BLKPRIO_META; p++)
        for(i = 0; i < cachelines; i++) {
            if(!cache[i]) continue;
            for(c = cache[i]; c; c = c->next)
                if(CACHE_PRIO(c->fd) == p) {
                    f = CACHE_FD(c->fd);
                    k = fcb[f].data.device.inode;
                    /* módosított blokk a gyorsítótárban, kiküldjük az eszközmeghajtónak */
                    mq_send6(dev[k].drivertask, DRV_write|MSG_PTRDATA, c->blk, fcb[f].data.device.blksize,
                        dev[k].device, f, c->lsn, k);
                    dev[k].total++;
                    cache_inflush++;
                }
        }
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_flush() flushing %d blks\n",cache_inflush);
#endif
}

/**
 * módosított jelző törlése és a kiírt blokk számláló növelése az eszközön
 */
bool_t cache_cleardirty(fid_t fd, blkcnt_t lsn)
{
    uint64_t i, j = lsn%cachelines;
    cache_t *c;

#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_cleardirty(fd %d, sector %d)\n",fd,lsn);
#endif
    /* paraméterek ellenőrzése */
    if(fd >= nfcb || lsn == -1U || fcb[fd].type != FCB_TYPE_DEVICE || !cache[j]) return false;
    /* blokk keresése a láncban */
    for(c = cache[j]; c; c = c->next)
        if(CACHE_FD(c->fd) == fd && c->lsn == lsn) {
            c->fd = CACHE_FD(fd);
            break;
        }
    /* kiírt blokkok számának növelése */
    i = fcb[fd].data.device.inode;
    dev[i].out++;
    if(dev[i].out >= dev[i].total)
        dev[i].out = dev[i].total = 0;
    /* UI taszk értesítése, hogy frissítse a dokkot */
    mq_send3(SRV_UI, SYS_devprogress, fd, dev[i].out, dev[i].total);
    cache_inflush--;
#if DEBUG
    if(_debug&DBG_CACHE && !cache_inflush)
        dbg_printf("FS: cache_cleardirty() all blks written out\n");
#endif
    return cache_inflush == 0;
}

/**
 * a gyorsítótár legalább felének felszabadítása a legrégebben használt blokkok kiírásával
 */
void cache_free()
{
    uint64_t i, k;
    blkcnt_t total = 0, freed = 0;
    cache_t *c;
    /* blokkok összeszámolása. erre csak ritkán van szükség */
    for(i = 0; i < cachelines; i++) {
        if(!cache[i]) continue;
        for(c = cache[i]; c; c = c->next)
            total++;
    }
    /* legalább a felét fel kell szabadítani */
    total >>= 1; i = 0; k = -1;
    /* biztonság kedvéért, ugyanaz az eszköz, mint legutóbb? */
    while(freed < total && i != k) {
        i = k;
        /* legrégebben használt eszköz lekérése */
        k = devfs_lastused(false);
        /* felszabadítjuk a blokkjait a gyorsítótárból */
        freed += cache_freeblocks(dev[k].fid, total - freed);
    }
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_free() freed %d out of %d blks\n", freed, total<<1);
#endif
}

#if DEBUG
/**
 * blokk gyorsítótár dumpolása, debuggoláshoz
 */
void cache_dump()
{
    uint64_t i;
    cache_t *c;
    blkcnt_t total = 0;

    for(i = 0; i < cachelines; i++) {
        if(!cache[i]) continue;
        for(c = cache[i]; c; c = c->next)
            total++;
    }
    dbg_printf("Block cache %d:\n", total);
    for(i = 0; i < cachelines; i++) {
        if(!cache[i]) continue;
        dbg_printf("%3d:", i);
        for(c = cache[i]; c; c = c->next)
            dbg_printf(" (prio %d, fd %d, lsn %d)", CACHE_PRIO(c->fd), CACHE_FD(c->fd), c->lsn);
        dbg_printf("\n");
    }
}
#endif
