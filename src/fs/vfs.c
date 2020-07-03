/*
 * fs/vfs.c
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
#include "vfs.h"
#include "devfs.h"
#include "mtab.h"

extern uint32_t pathmax;        /* elérési út maximális hossza */

uint8_t *zeroblk = NULL;        /* üres blokk */
uint8_t *rndblk = NULL;         /* véletlen adatokat tartalmazó blokk */

stat_t st;                      /* lstat() és fstat() buffere */
dirent_t dirent;                /* readdir() buffere */
public fsck_t fsck;             /* fsck állapota a dofsck() és az fsdrv checkfs()-e számára */

int pathstackidx = 0;           /* elérési út verem */
pathstack_t pathstack[PATHSTACKSIZE];

public char *lastlink;          /* a legutóbbi szimbolikus hivatkozás célja, az fsdrv stat()-a tölti */


/**
 * inode hivatkozás berakása az elérési út verembe. forgatjuk, ha túl nagyra nőne
 */
public void pathpush(ino_t lsn, char *path)
{
    int i;

    if(pathstackidx == PATHSTACKSIZE) {
        /* memcpy(&pathstack[0], &pathstack[1], (PATHSTACKSIZE-1)*sizeof(pathstack_t)); */
        for(i = 0; i < PATHSTACKSIZE-1; i++) pathstack[i] = pathstack[i+1];
        pathstack[PATHSTACKSIZE-1].inode = lsn;
        pathstack[PATHSTACKSIZE-1].path = path;
        return;
    }
    pathstack[pathstackidx].inode = lsn;
    pathstack[pathstackidx++].path = path;
}

/**
 * legutóbbi inode kivétele az elérési út veremből
 */
public pathstack_t *pathpop()
{
    if(!pathstackidx) return NULL;
    pathstackidx--;
    return &pathstack[pathstackidx];
}

/**
 * fájlnév hozzáfűzése az elérési úthoz. A path buffernek elég nagynak kell lennie
 */
char *pathcat(char *path, char *filename)
{
    int i;

    /* paraméterek ellenőrzése */
    if(!path || !path[0]) return NULL;
    if(!filename || !filename[0]) return path;

    i = strlen(path);
    if(i + strlen(filename) >= pathmax) return NULL;
    if(path[i-1] != '/') {
        path[i++] = '/'; path[i] = 0;
    }
    strcpy(path + i, filename + (filename[0] == '/'? 1 : 0));
    return path;
}

/**
 * hasonló a realpath()-hoz, de csak memóriából dolgozik, és nem oldja fel a szimbólikus hivatkozásokat és a fel könyvtárat
 */
char *canonize(const char *path)
{
    int i = 0, j = 0, k, m;
    char *result;

    if(!path || !path[0]) return NULL;
    result = (char*)malloc(pathmax);
    if(!result) return NULL;
    k = strlen(path);
    /* "eszköz:" kezdetű elérési út átalakítása */
    while(i < k && path[i] != ':' && path[i] != '/' && !PATHEND(path[i])) i++;
    if(path[i] == ':') {
        /* "root:" átugrása */
        if(i == 4 && !memcmp(path, "root", 4)) {
            i = 4; j = 0;
        } else {
            strcpy(result, DEVPATH); j = sizeof(DEVPATH);
            strncpy(result + j, path, i); j += i;
        }
        result[j++] = '/'; m = j;
        i++;
        if(i < k && path[i] == '/') i++;
    } else {
        i = 0;
        m = strlen(fcb[ctx->rootdir].abspath);
        if(path[0] == '/') {
            /* abszolút elérési út */
            strcpy(result, fcb[ctx->rootdir].abspath);
            j = m = strlen(result);
            if(result[j-1] == '/') i = 1;
        } else {
            /* aktuális munkakönyvtárat használjuk */
            strcpy(result, fcb[ctx->workdir].abspath);
            j = strlen(result);
        }
    }

    /* elérési út fennmaradó részének értelmezése */
    while(i < k && !PATHEND(path[i])) {
        if(result[j-1] != '/') result[j++] = '/';
        while(i < k && path[i] == '/') i++;
        /* akruális könyvtár hivatkozás kihagyása */
        if(path[i] == '.' && i+1 < k && (path[i+1] == '/' || PATHEND(path[i+1]))) {
            i += 2; continue;
        } else
        /* külön lekezeljük a .../../ esetet: kihagyjuk a ../-t és eltávolítjuk a .../-t */
        if(j > 4 && path[i] == '.' && i+2 < k && path[i+1] == '.' && (path[i+2] == '/' || PATHEND(path[i+2])) &&
            !memcmp(result + j-4, ".../", 4)) {
                j -= 4; i += 3; continue;
        /* egyébként nem kezeljük a fel könyvtárat, mert lehet szimbólikus hivatkozás után áll */
/*
        if(path[i] == '.') {
            i++;
            if(path[i] == '.') {
                i++;
                for(j--; j > m && result[j-1] != '/'; j--);
                result[j] = 0;
            }
*/
        } else {
            /* könyvtárnév másolása */
            while(i < k && path[i] != '/' && !PATHEND(path[i])) result[j++]=path[i++];
            /* verzió kanonizálása (könyvtárakon nincs, mert túl költséges lenne, ezért csak a legutolsó tagon lehet) */
            if(path[i] == ';') {
                /* könyvtáraknál nincs */
                if(result[j-1] == '/') break;
                /* előjel átugrása */
                i++; if(i < k && path[i] == '-') i++;
                /* csak érvényes számot fogadunk el */
                if(i+1 < k && path[i] >= '1' && path[i] <= '9' && (path[i+1] == '#' || path[i+1] == 0)) {
                    result[j++] = ';';
                    result[j++] = path[i];
                    i++;
                }
                /* nincs break, mert folytatódhat offszettel */
            }
            /* offszet kanonizálása (ez is csak az utolsó tagnál fordulhat elő) */
            if(path[i] == '#') {
                /* könyvtáraknál nincs */
                if(result[j-1] == '/') break;
                /* előjel átugrása */
                i++; if(i < k && path[i] == '-') i++;
                /* csak érvényes számot fogadunk el */
                if(i < k && path[i] >= '1' && path[i] <= '9') {
                    result[j++] = '#';
                    if(path[i-1] == '-') result[j++] = '-';
                    while(i < k && path[i] && path[i] >= '0' && path[i] <= '9') result[j++] = path[i++];
                }
                /* elérési út végére értünk */
                break;
            }
        }
        i++;
    }
    if(j > 0 && result[j-1] == '/') j--;
    /*while(j > 0 && result[j-1] == '.') j--;*/
    /* lezáró könyvtár határoló */
    if(i > 0 && path[i-1] == '/') result[j++] = '/';
    result[j] = 0;
    return result;
}

/**
 * verzió lekérése elérési útból
 */
public uint8_t getver(char *abspath)
{
    int i = 0;

    if(!abspath || !abspath[0]) return 0;
    while(abspath[i] != ';' && abspath[i]) i++;
    return abspath[i] == ';' ? atoi(&abspath[i+1]) : 0;
}

/**
 * offszet lekérése elérési útból
 */
public fpos_t getoffs(char *abspath)
{
    int i = 0;

    if(!abspath || !abspath[0]) return 0;
    while(abspath[i] != '#' && abspath[i]) i++;
    return abspath[i] == '#' ? atol(&abspath[i+1]) : 0;
}

/**
 * blokk beolvasása tárolóból (zárolt eszköz esetén is működnie kell)
 */
public void *readblock(fid_t fd, blkcnt_t lsn)
{
    devfs_t *device;
    uint8_t *blk=NULL, *ptr;
    writebuf_t *w;
    fcb_t *f;
    size_t bs;
    fpos_t o, e, we;

    /* paraméterek ellenőrzése */
    if(fd >= nfcb || lsn == -1U) { seterr(EFAULT); return NULL; }
#if DEBUG
    if(_debug&DBG_BLKIO)
        dbg_printf("FS: readblock(fd %d, sector %d)\n",fd,lsn);
#endif
    f = &fcb[fd];
    if(!(f->mode & O_READ)) { seterr(EACCES); return NULL; }
    switch(f->type) {
        case FCB_TYPE_REG_FILE:
            /* blokk olvasása képfájlból */
            if(f->data.reg.storage == -1U || fcb[f->data.reg.storage].type != FCB_TYPE_DEVICE) { seterr(ENODEV); return NULL; }
            devfs_used(fcb[f->data.reg.storage].data.device.inode);
            /* első szektor esetén többet olvasunk a fájlrendszerazonosítás miatt */
            bs = fcb[f->data.reg.storage].data.device.blksize;
            if(!lsn && bs < BUFSIZ) bs = BUFSIZ;
            if((int64_t)bs < 1) { seterr(EINVAL); return NULL; }
            /* megnézzük, hogy a blokk a write buf-ban benne van-e */
            o = lsn*bs; e = o + bs;
            for(w = f->data.reg.buf; w; w = w->next) {
                /*     ####
                 *  +--------+   */
                if(o >= w->offs && e <= w->offs + w->size)
                    return (w->data + o - w->offs);
            }
            /* érvényes cím? */
            if((lsn + 1) * bs > f->data.reg.filesize) { seterr(EFAULT); return NULL; }
            if(f->fs < nfsdrv && fsdrv[f->fs].read) {
                /* fájlrendszer meghajtó hívása */
 #if DEBUG
                if(_debug&DBG_FILEIO)
                    dbg_printf("FS: file read(fd %d, ino %d, offs %d, size %d)\n",
                        f->data.reg.storage, f->data.reg.inode, lsn*bs, bs);
#endif
                blk = (*fsdrv[f->fs].read)(f->data.reg.storage, f->data.reg.ver, f->data.reg.inode, lsn*bs, &bs);
                /* ha nincs a blokkgyorsítótárban van fájl végére értünk */
                if(ackdelayed || !bs) return NULL;
            }
            if(!blk) {
                /* üres blokk visszaadása ha nincs read vagy hézag van a fájlban */
                zeroblk = (void*)realloc(zeroblk, bs);
                blk = zeroblk;
            }
            /* összevetjük a write buf-al */
            for(w = f->data.reg.buf; w; w = w->next) {
                we = w->offs + w->size;
                /* ####
                 *   +----+  */
                if(o < w->offs && e > w->offs && e <= we) {
                    ptr = malloc(we - o);
                    if(!ptr) return blk;
                    memcpy(ptr, blk, w->offs - o);
                    memcpy(ptr + w->offs - o, w->data, w->size);
                    free(w->data);
                    w->size = we - o;
                    w->offs = o;
                    w->data = ptr;
                    return ptr;
                }
                /*     ####
                 * +----+    */
                if(o > w->offs && o < we && e > we) {
                    w->data = realloc(w->data, e - w->offs);
                    if(!w->data) return blk;
                    memcpy(w->data + w->size, blk + we - o, e - we);
                    w->size = e-w->offs;
                    return w->data + o - w->offs;
                }
            }
            /* nincs közös metszet, úgy adjuk vissza a blokkot, ahogy van */
            return blk;

        case FCB_TYPE_DEVICE:
            /* blokk olvasása eszközről */
            if(f->data.device.inode < ndev) {
                device = &dev[f->data.device.inode];
                devfs_used(f->data.device.inode);
                bs = f->data.device.blksize;
                if((int)bs < 1) { seterr(EINVAL); return NULL; }
                /* memória eszköz? */
                if(device->drivertask == MEMFS_MAJOR) {
                    switch(device->device) {
                        /* lsn nem érdekes ezeknél */
                        case MEMFS_NULL:
                            return NULL;

                        case MEMFS_ZERO:
                            zeroblk = (uint8_t*)realloc(zeroblk, bs);
                            return zeroblk;

                        case MEMFS_RANDOM:
                            rndblk = (uint8_t*)realloc(rndblk, bs);
                            if(rndblk) getentropy(rndblk, bs);
                            return rndblk;

                        case MEMFS_RAMDISK:
                            /* érvényes cím? */
                            if((lsn + 1) * bs > f->data.device.filesize) {
                                seterr(EFAULT);
                                return NULL;
                            }
                            return (void *)(BUF_ADDRESS + lsn * bs);
                        default:
                            /* ide nem szabadna eljutni, de azért állítunk hibaüzenetet */
                            seterr(ENODEV);
                            return NULL;
                    }
                }
                /* egyébként igazi eszköz, a blokkgyorsítótárat használjuk */
                return cache_getblock(fd, lsn);
            }
            seterr(ENODEV);
            return NULL;

        default:
            break;
    }
    /* hibás hívás, csak képfájlok és blokk eszközök lehetnek tárolók */
    seterr(EPERM);
    return NULL;
}

/**
 * blokk kiírása tárolóra megfelelő prioritással (zárolt eszköz esetén is működnie kell)
 */
public bool_t writeblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)
{
    fcb_t *f;
    devfs_t *device;
    size_t bs;

#if DEBUG
    if(_debug&DBG_BLKIO)
        dbg_printf("FS: writeblock(fd %d, sector %d, buf %x, prio %d)\n",fd,lsn,blk,prio);
#endif

    /* paraméterek ellenőrzése */
    if(fd >= nfcb || lsn == -1U) { seterr(EFAULT); return false; }
    f = &fcb[fd];
    if(!(f->mode & O_WRITE)) { seterr(EACCES); return false; }
    switch(f->type) {
        case FCB_TYPE_REG_FILE:
            /* blokk írása képfájlba */
            if(f->data.reg.storage == -1U || fcb[f->data.reg.storage].type != FCB_TYPE_DEVICE) { seterr(ENODEV); return false; }
            if(fsck.dev == f->data.reg.storage) { seterr(EBUSY); return false; }
            if(!(fcb[f->data.reg.storage].mode & O_WRITE) || f->fs >= nfsdrv || !fsdrv[f->fs].write) { seterr(EROFS); return false; }
            devfs_used(fcb[f->data.reg.storage].data.device.inode);
            bs = fcb[f->data.reg.storage].data.device.blksize;
            /* write buf-ba írás */
            return fcb_write(fd, lsn * bs, blk, bs);

        case FCB_TYPE_DEVICE:
            /* blokk írása eszközre */
            if(f->data.device.inode < ndev) {
                device = &dev[f->data.device.inode];
                devfs_used(f->data.device.inode);
                bs = f->data.device.blksize;
                /* memória eszköz? */
                if(device->drivertask==MEMFS_MAJOR) {
                    switch(device->device) {
                        case MEMFS_NULL:
                            return true;

                        case MEMFS_RAMDISK:
                            if((lsn + 1) * bs > f->data.device.filesize) {
                                seterr(EFAULT);
                                return false;
                            }
                            /* ha helyben van, akkor csak visszatérünk */
                            if(blk == (uint8_t *)(BUF_ADDRESS + lsn * bs)) return true;
                            /* blokk másolása */
                            memcpy((void *)(BUF_ADDRESS + lsn * bs), blk, bs);
                            return true;

                        default:
                            seterr(EACCES);
                            return false;
                    }
                }
                /* egyébként igazi eszköz, a blokkgyorsítótárat használjuk */
                return cache_setblock(fd, lsn, blk, prio);
            }
            seterr(ENODEV);
            break;

        default:
            break;
    }
    /* hibás hívás, csak képfájlok és blokk eszközök lehetnek tárolók */
    seterr(EPERM);
    return false;
}

/**
 * fcb index visszaadása abszolút elérési úthoz. Errno-t EAGAIN-re állíthatja gyorsítótárhiány esetén
 */
fid_t lookup(char *path, bool_t creat)
{
    locate_t loc;
    char *abspath, *tmp = NULL, *c;
    fid_t f, fd, ff, re = 0;
    int16_t fs;
    int i, j, l, k;
    uint8_t status;

again:
/*dbg_printf("lookup '%s' creat %d\n",path,creat); */
    if(tmp) { free(tmp); tmp = NULL; }
    if(!path || !path[0]) { seterr(EINVAL); return -1U; }
    abspath = canonize(path);
    if(!abspath) { seterr(ENOENT); return -1U; }
    /* fájl létrehozás esetén a joker nem szerepelhet az elérési útban */
    j = strlen(abspath);
    if(creat && j > 4) {
        j -= 3;
        for(i = 0; i < j; i++)
            if(abspath[i] == '.' && abspath[i+1] == '.' && abspath[i+2] == '.') { free(abspath); seterr(ENOENT); return -1U; }
    }
    f = fcb_get(abspath);
    fd = ff = fs = -1;
    i = j = l = k = 0;
    /* ha nincs a gyorsítótárban */
    if(f == -1U || (f == ROOTFCB && !fcb[f].data.reg.inode)) {
        /* először a leghosszabb egyezést keressük a statikus felcsatolásokban, a gyökérkönyvtár mindig találatot ad */
        for(i = 0; i < nmtab; i++) {
            j = strlen(fcb[mtab[i].mountpoint].abspath);
            if(j > l && !memcmp(fcb[mtab[i].mountpoint].abspath, abspath, j)) { l = j; k = i; }
        }
        /* felcsatolt eszköz */
        fd = mtab[k].storage;
        /* felcsatolási pont */
        ff = mtab[k].mountpoint;
        /* fájlrendszer meghajtó */
        fs = mtab[k].fstype;
        /* ha a leghosszabb egyezés a "/" volt, akkor automount-ot is nézünk */
        if(k == ROOTMTAB && !memcmp(abspath, DEVPATH, sizeof(DEVPATH))) {
            i = sizeof(DEVPATH); while(abspath[i] != '/' && !PATHEND(abspath[i])) i++;
            if(abspath[i] == '/') {
                /* az elérési út "/dev/x/"-al kezdődik, kell az eszköz fcb-je */
                abspath[i] = 0;
                fd = ff = fcb_get(abspath);
                abspath[i++] = '/'; l = i;
                fs = -1;
            }
        }
        /* csak képfájlok és blokk eszközök lehetnek tárolók */
        if(fd >= nfcb || (fcb[fd].type != FCB_TYPE_DEVICE && fcb[fd].type != FCB_TYPE_REG_FILE)) {
            free(abspath);
            seterr(ENODEV);
            return -1U;
        }
        /* fájlrendszer detektálása menet közben */
        if(fs == -1) {
            fs = fsdrv_detect(fd);
            if(k != ROOTMTAB && fs != -1)
                mtab[k].fstype = fs;
        }
        /* ha a fájlrendszer nem ismert, nem sok mindent tehetünk */
        if(fs == -1 || !fsdrv[fs].locate) {
            free(abspath);
            seterr(ENOFS);
            return -1U;
        }
        /* ellenőrizzük, hogy zárolt-e */
        if(fsck.dev == fd) {
            free(abspath);
            seterr(EAGAIN);
            return -1U;
        }
        /* fennmaradó részt átadjuk a fájlrendszer meghajtónak */
        /* fd=eszköz fcb, fs=fsdrv idx, ff=felcsatolási pont, l=leghosszabb egyezés hossza */
        loc.path = abspath + l;
        loc.inode = -1U;
        loc.fileblk = NULL;
        loc.creat = creat;
        pathstackidx = 0;
        status = (*fsdrv[fs].locate)(fd, 0, &loc);
        if(ackdelayed) return -1U;
        switch(status) {
            case SUCCESS:
                i = strlen(abspath);
                if((loc.type == FCB_TYPE_REG_DIR || loc.type == FCB_TYPE_UNION) && i > 0 && abspath[i-1] != '/') {
                    abspath[i++] = '/'; abspath[i] = 0;
                }
                if(f == -1U) f = fcb_add(abspath, loc.type);
                fcb[f].fs = fs;
                if(loc.type == FCB_TYPE_REG_FILE || loc.type == FCB_TYPE_REG_DIR || loc.type == FCB_TYPE_UNION)
                    fcb[f].mode = fcb[fd].mode;   /* eszköz hozzáférési módját örökítjük */
                fcb[f].data.reg.storage = fd;
                fcb[f].data.reg.inode = loc.inode;
                fcb[f].data.reg.filesize = loc.filesize;
                break;

            case NOBLOCK:
                /* az errno EAGAIN blokkgyorsítótárhiány esetén és ENOMEM memóriahiány esetén */
                break;

            case NOTFOUND:
                seterr(ENOENT);
                break;

            case FSERROR:
                seterr(EBADFS);
                break;

            case UPDIR:
                /* TEENDŐ: fel könyvtár lekezelése */
                break;

            case FILEINPATH:
                /* TEENDŐ: fájlrendszer detektálása a képfájlban */
                break;

            case SYMINPATH:
/*dbg_printf("SYMLINK %s %s\n",loc.fileblk,loc.path);*/
                if(((char*)loc.fileblk)[0] != '/') {
                    /* relatív szimbólikus hivatkozás */
                    tmp = (char*)malloc(pathmax);
                    if(!tmp) break;
                    c = loc.path - 1; while(c > abspath && c[-1] != '/') c--;
                    memcpy(tmp, abspath, c-abspath);
                    tmp[c-abspath] = 0;
                    tmp = pathcat(tmp,(char*)loc.fileblk);
                } else {
                    /* abszolút szimbólikus hivatkozás */
                    tmp = canonize(loc.fileblk);
                }
                if(!tmp) break;
                tmp = pathcat(tmp, loc.path);
                if(!tmp) break;
                path = tmp;
                free(abspath);
                goto again;

            case UNIONINPATH:
                /* végigmegyünk az unió tagjain */
                c = (char*)loc.fileblk;
                while(f == -1U && *c && (virt_t)(c - (char*)loc.fileblk) < (virt_t)(__PAGESIZE - 2048)) {
                    /* fájl létrehozásnál nem megengedett az unió */
                    if(creat) { seterr(EISUNI); break; }
/*dbg_printf("UNION %s %s\n",c,loc.path);*/
                    /* az unió tagjai abszolút eléréi utak lehetnek csak */
                    if(*c!='/') { seterr(EBADFS); break; }
                    tmp = canonize(c);
                    if(!tmp) break;
                    tmp = pathcat(tmp, loc.path);
                    if(!tmp) break;
                    f = lookup(tmp, creat);
                    free(tmp);
                    c += strlen(c) + 1;
                }
                break;

            case NOSPACE:
#if DEBUG
                if(_debug&DBG_FS)
                    dbg_printf("No space left on device %s (fcb %d)\n", fcb[fd].abspath,fd);
#endif
                /* az initrd-t kicsit másképp kezeljük: plusz memóriát foglalunk és újrapróbáljuk, de csak akkor,
                 * ha a resizefs() korábban nem adott valamiért hibát */
                if(fcb[fd].type == FCB_TYPE_DEVICE && dev[fcb[fd].data.device.inode].drivertask == MEMFS_MAJOR &&
                    dev[fcb[fd].data.device.inode].device == MEMFS_RAMDISK && fsdrv[fs].resizefs && re != loc.inode &&
                    mmap((void*)(BUF_ADDRESS + fcb[fd].data.device.filesize), __PAGESIZE, PROT_READ|PROT_WRITE,
                        MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) != MAP_FAILED) {
                            fcb[fd].data.device.filesize += __PAGESIZE;
                            free(abspath);
                            (*fsdrv[fs].resizefs)(fd);
                            re = loc.inode;
                            goto again;
                }
                seterr(ENOSPC);
                break;
        }
    }
/*dbg_printf("lookup result %s = %d (err %d)\n",abspath,f,errno());*/
    free(abspath);
    /* biztonság kedvéért beállítunk hibakódot */
    if(f == -1U && !errno()) seterr(ENOENT);
    return f;
}

/**
 * stat_t struktúra lekérése fcb indexhez
 */
stat_t *statfs(fid_t idx)
{
    fcb_t *f;

    if(idx >= nfcb || !fcb[idx].abspath) return NULL;
    f = &fcb[idx];
    memzero(&st, sizeof(stat_t));
/*dbg_printf("statfs(%d)\n",idx);*/
    /* általános fcb mezők */
    st.st_dev = f->data.reg.storage;
    st.st_ino = f->data.reg.inode;
    st.st_nlink = f->nopen;
    st.st_mode = (f->mode & S_IMODE);
    if(f->type == FCB_TYPE_UNION)
        st.st_mode |= S_IFDIR | S_IFUNI;
    else
        st.st_mode |= (f->type << 16);
    st.st_size = f->data.reg.filesize;
    /* típus fuggő mezők */
    switch(f->type) {
        case FCB_TYPE_PIPE:
            /* a csővezetékeket már lekezeltük */
            break;

        case FCB_TYPE_SOCKET:
            /* nincs extra infó */
            break;

        case FCB_TYPE_DEVICE:
            /* eszköz blokkmérete */
            st.st_blksize = f->data.device.blksize;
            if(f->data.device.blksize == 1) st.st_mode |= S_IFCHR;
            break;

        default:
            /* fájlrendszer meghajtó hívása a stat_t mezők kitöltésére */
            if(f->fs < nfsdrv && fsdrv[f->fs].stat) (*fsdrv[f->fs].stat)(f->data.reg.storage, f->data.reg.inode, &st);
            /* a tároló eszköz blokkmérete */
            st.st_blksize = fcb[f->data.reg.storage].data.device.blksize;
            break;
    }
    return &st;
}

/**
 * fájlrendszer ellenőrzése egy eszközön
 */
bool_t dofsck(fid_t fd, bool_t fix)
{
    /* paraméterek ellenőrzése */
    if(fd == -1U || fsck.dev != -1U || fsck.dev != fd || fcb[fd].type != FCB_TYPE_DEVICE || fcb[fd].data.device.storage != DEVFCB) {
        seterr(EPERM);
        return false;
    }
    /* először hívjuk a dofck-t? */
    if(fsck.dev == -1U) {
        fsck.dev = fd;
        fsck.fix = fix;
        fsck.step = FSCKSTEP_SB;
        fsck.fs = fsdrv_detect(fd);
    }
    if(fsck.fs == (uint16_t)-1 || fsck.fs >= nfsdrv || !fsdrv[fsck.fs].checkfs) {
        fsck.dev = -1U;
        fsck.step = 0;
        seterr(EPERM);
        return false;
    }
    /* félbeszakított lépés folytatása */
    while(fsck.step < FSCKSTEP_DONE) {
        if(!(*fsdrv[fsck.fs].checkfs)(fd) || ackdelayed) return false;
        fsck.step++;
    }
    /* zárolás törlése és a lépés nullázása */
    fsck.dev = -1;
    fsck.step = 0;
    return true;
}
