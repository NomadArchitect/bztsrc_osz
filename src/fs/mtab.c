/*
 * fs/mtab.c
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
 * @brief felcsatolási pontok kezelése
 */

#include <osZ.h>
#include "mtab.h"
#include "fcb.h"
#include "vfs.h"
#include "fsdrv.h"

uint8_t rootmounted = false;
uint16_t nmtab = 0;
mtab_t *mtab = NULL;

/**
 * felcsatolási pontok inicializálása
 */
void mtab_init()
{
    fcb_add("/",FCB_TYPE_REG_DIR);
}

/**
 * új felcsatolási pont hozzáadása
 */
uint16_t mtab_add(char *dev, char *file, char __attribute__((unused)) *opts)
{
    fid_t fd, ff;
    uint16_t fs, i;

    /* paraméterek ellenőrzése */
    if(!dev || !dev[0] || !file || !file[0]) {
        seterr(EINVAL);
        return (uint16_t)-1;
    }

    /* eszköz fcb */
    fd = fcb_get(dev);
    if(fd == -1U || (fd != DEVFCB && fcb[fd].type != FCB_TYPE_DEVICE && fcb[fd].type != FCB_TYPE_REG_FILE)) {
        if(!errno()) seterr(ENODEV);
        return (uint16_t)-1;
    }
    /* felcsatolási pont fcb */
    ff = fcb_get(file);
    if(ff == -1U || fcb[ff].type != FCB_TYPE_REG_DIR) {
        if(!errno()) seterr(ENOTDIR);
        return (uint16_t)-1;
    }
    /* tyúk és tojás probléma: a /dev-re már azelőtt szükségünk van, hogy lenne / felcsatolva */
    if((fd == ff && rootmounted) || (ff != ROOTFCB && !nmtab)) {
        seterr(EINVAL);
        return (uint16_t)-1;
    }
    /* fájlrendszer detektálása */
    fs = fsdrv_detect(fd);
    if(fs == (uint16_t)-1) {
        /* plusz ellenőrzés, a gyökérfájlrendszernek kell lennie az első felcsatolásnak */
        if(!rootmounted || !nmtab) exit(1);
        seterr(ENOFS);
        return (uint16_t)-1;
    }
    /* megnézzük, van-e már felcsatolva valami ide */
    for(i = 0; i < nmtab; i++) {
        if(mtab[i].mountpoint == ff) {
            fcb[mtab[i].storage].nopen--;
            break;
        }
    }
    if(i == nmtab) {
        mtab = (mtab_t*)realloc(mtab, ++nmtab * sizeof(mtab_t));
        if(!mtab) return (uint16_t)-1;
        fcb[ff].nopen++;
    }
    mtab[i].storage = fd;
    mtab[i].mountpoint = ff;
    mtab[i].fstype = fs;
    fcb[ff].fs = fs;
    fcb[ff].mode = fcb[fd].mode;
    if(ff != DEVFCB)
        fcb[ff].data.reg.storage = fd;
    else
        fcb[ff].mode = O_RDONLY;
    fcb[fd].nopen++;

/*dbg_printf("dev '%s' file '%s' opts '%s' fs %d\n",dev,file,opts,fs);*/
    if(ff == ROOTFCB && !rootmounted) {
        /* ha a gyökérfájlrendszert csatoljuk fel, akkor a /dev-et is felcsatoljuk. Az elérési útja nem megváltoztatható */
        mtab_add(DEVPATH, DEVPATH, "");
        rootmounted = true;
    }
    return (uint16_t)-1;
}

/**
 * felcsatolási pont megszüntetése
 */
bool_t mtab_del(char *dev, char *file)
{
    uint16_t i;
    char *absdev = canonize(dev), *absmnt = canonize(file);

    if(!absdev && !absmnt) {
        seterr(EINVAL);
        if(absdev) free(absdev);
        if(absmnt) free(absmnt);
        return false;
    }
    for(i = 0; i < nmtab; i++) {
        if( (absdev && !strcmp(absdev, fcb[mtab[i].storage].abspath)) ||
            (absmnt && !strcmp(absmnt, fcb[mtab[i].mountpoint].abspath))) {
                if(absdev) free(absdev);
                if(absmnt) free(absmnt);
                nmtab--;
                memcpy(&mtab[i], &mtab[i+1], (nmtab-i)*sizeof(mtab_t));
                mtab = (mtab_t*)realloc(mtab, nmtab*sizeof(mtab_t));
                return !mtab? false : true;
        }
    }
    if(absdev) free(absdev);
    if(absmnt) free(absmnt);
    return false;
}

/**
 * az /etc/fstab értemezése és fájlrendszerek felcsatolása
 */
void mtab_fstab(char *ptr, size_t size)
{
    char *dev, *file, *opts, *end, pass;
    char *fstab = (char*)malloc(size);

    /* le kell másolnunk, mivel nullával zárjuk le benne a sztringeket */
    if(!fstab || errno()) exit(2);
    memcpy(fstab, ptr, size);
    end = fstab + size;
    /* az mtab_add-ot megfelelő sorrendben kell meghívni */
    for(pass = '1'; pass <= '9'; pass++) {
        ptr = fstab;
        while(ptr < end) {
            while(*ptr == '\n' || *ptr == '\t' || *ptr == ' ') ptr++;
            /* átlépjük a kommenteket */
            if(*ptr == '#') {
                for(ptr++; ptr < end && *(ptr-1) != '\n'; ptr++);
                continue;
            }
            /* blokk eszköz */
            dev = ptr;
            while(ptr < end && *ptr && *ptr != '\t' && *ptr != ' ') ptr++;
            if(*ptr == '\n') continue;
            *ptr = 0; ptr++;
            while(ptr < end && (*ptr == '\t' || *ptr == ' ')) ptr++;
            if(*ptr == '\n') continue;
            /* felcsatolási pont */
            file = ptr;
            while(ptr < end && *ptr && *ptr != '\t' && *ptr != ' ') ptr++;
            if(*ptr == '\n') continue;
            *ptr = 0; ptr++;
            while(ptr < end && (*ptr == '\t' || *ptr == ' ')) ptr++;
            if(*ptr == '\n') continue;
            /* opciók és hozzáférések */
            opts = ptr;
            while(ptr < end && *ptr && *ptr != '\t' && *ptr != ' ') ptr++;
            if(*ptr == '\n') continue;
            *ptr = 0; ptr++;
            while(ptr < end && (*ptr == '\t' || *ptr == ' ')) ptr++;
            if(*ptr == '\n') continue;
            /* hányadik körben kell felcsatolni */
            if(*ptr == pass)
                mtab_add(dev, file, opts);
            ptr++;
        }
        /* ha az első körben nem csatoltuk fel a gyökérkönyvtárat, akkor most megtesszük manuálisan */
        if(pass == '1' && !rootmounted)
            mtab_add("/dev/root", "/", "");
    }
    free(fstab);
}

#if DEBUG
/**
 * felcsatolási pontok dumpolása debuggoláshoz
 */
void mtab_dump()
{
    uint16_t i;

    dbg_printf("Mounts %d:\n", nmtab);
    for(i = 0; i < nmtab; i++) {
        dbg_printf("%3d. %s %s %s\n", i, fcb[mtab[i].storage].abspath,
            mtab[i].fstype>=nfsdrv? "???" : fsdrv[mtab[i].fstype].name,
            fcb[mtab[i].mountpoint].abspath);
    }
}
#endif
