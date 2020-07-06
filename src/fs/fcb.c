/*
 * fs/fcb.c
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
 * @brief Fájl Kontrol Blokkok kezelése
 */

#include <osZ.h>
#include "vfs.h"
#include "pipe.h"

extern uint8_t ackdelayed;
extern pid_t mq_caller;

public uint64_t nfcb = 0;
public fcb_t *fcb = NULL;

/**
 * elérési út keresése az fcb listában
 */
fid_t fcb_get(char *abspath)
{
    fid_t i;
    char *ap = NULL;

    if(!abspath || !abspath[0]) return -1U;
    i = strlen(abspath);
    if(abspath[i-1] != '/') {
        ap = (char*)malloc(i+2);
        if(ap) {
            memcpy(ap, abspath, i);
            ap[i++] = '/'; ap[i] = 0;
        }
    }
    for(i = 0; i < nfcb; i++) {
        if(fcb[i].abspath && (!strcmp(fcb[i].abspath, abspath) || (ap && !strcmp(fcb[i].abspath, ap)))) {
            if(ap) free(ap);
            return i;
        }
    }
    if(ap) free(ap);
    return -1U;
}

/**
 * elérési út keresése az fcb listában, és hozzáadás, ha nem találta
 */
fid_t fcb_add(char *abspath, uint8_t type)
{
    fid_t i, j = -1U;

    for(i = 0; i < nfcb; i++) {
        if(fcb[i].abspath) {
            if(!strcmp(fcb[i].abspath, abspath)) return i;
        } else {
            if(j == -1U) j = i;
        }
    }
    if(j == -1U) {
        nfcb++;
        fcb = (fcb_t*)realloc(fcb, nfcb*sizeof(fcb_t));
        if(!fcb) return -1U;
    } else
        i = j;
    abspath = strdup(abspath);
    fcb[i].abspath = abspath;
    fcb[i].type = type;
    if(type < FCB_TYPE_PIPE)
        fcb[i].fs = (uint16_t)-1;
    return i;
}

/**
 * fcb bejegyzés eltávolítása
 */
void fcb_del(fid_t idx)
{
    uint64_t i;
    fcb_t *f;

    /* paraméterek ellenőrzése */
    if(idx >= nfcb || !fcb[idx].abspath) return;
    f = &fcb[idx];
    /* hivatkozásszámláló csökkentése */
    if(f->nopen > 0) f->nopen--;
    /* ha nulla, kitöröljük a bejegyzést */
    if(!f->nopen) {
        if(f->type == FCB_TYPE_UNION && f->data.uni.unionlist) {
            for(i = 0; f->data.uni.unionlist[i]; i++)
                if(idx != f->data.uni.unionlist[i])
                    fcb_del(f->data.uni.unionlist[i]);
            free(f->data.uni.unionlist);
            f->data.uni.unionlist = NULL;
        }
        /* f->type==FCB_TYPE_PIPE esetet már lekezelte a taskctx_close,
         * mivel névtelen csővezetékekre is működnie kell */
        if(f->abspath) free(f->abspath);
        f->abspath = NULL;
        f->data.reg.filesize = 0;
        idx = nfcb;
        while(idx > 0 && !fcb[idx-1].abspath) idx--;
        if(idx != nfcb) {
            nfcb = idx;
            fcb = (fcb_t*)realloc(fcb, nfcb * sizeof(fcb_t));
        }
    }
}

/**
 * összes nemhasznált bejegyzés törlése az fcb listából
 */
void fcb_free()
{
    uint64_t i;
    bool_t was = true;

    while(was) {
        was = false;
        for(i = 0; i < nfcb; i++)
            if(!fcb[i].nopen) {
                was = true;
                fcb_del(i);
            }
    }
}

/**
 * fid_t hozzáadása az unió fl listához, ha még nincs benne
 */
size_t fcb_unionlist_add(fid_t **fl, fid_t f, size_t n)
{
    uint64_t i;

    if(f != -1U) {
        for(i = 0; i < n; i++)
            if((*fl)[i] == f) return n;
        *fl = (fid_t*)realloc(*fl, ++n * sizeof(fid_t));
        if(!*fl) return 0;
        (*fl)[n-2] = f;
    }
    return n;
}

/**
 * unió listájának vagy szimbolikus hivatkozás céljának visszaadása
 */
char *fcb_readlink(fid_t idx)
{
    uint64_t bs;

    /* paraméterek ellenőrzése */
    if(idx >= nfcb || !fcb[idx].abspath || (fcb[idx].type != FCB_TYPE_UNION && fcb[idx].type != FCB_TYPE_REG_FILE) ||
      fcb[idx].fs >= nfsdrv || !fsdrv[fcb[idx].fs].read)
        return NULL;
    bs = fcb[fcb[idx].data.reg.storage].data.device.blksize;
    /* fájlrendszermeghajtó hívása */
#if DEBUG
    if(_debug&DBG_FILEIO)
        dbg_printf("FS: file read(fd %d, ino %d, offs %d, size %d)\n",fcb[idx].data.reg.storage, fcb[idx].data.reg.inode, 0, bs);
#endif
    return (*fsdrv[fcb[idx].fs].read)(fcb[idx].data.reg.storage, fcb[idx].data.reg.ver, fcb[idx].data.reg.inode, 0, &bs);
}

/**
 * unió fid listájának összeállítása
 */
fid_t fcb_unionlist_build(fid_t idx, void *buf, size_t s)
{
    uint64_t i, j, k, l, n, bs;
    char *ptr, *fn;
    fid_t f, *fl = NULL;

    /* paraméterek ellenőrzése */
    if(idx >= nfcb || !fcb[idx].abspath || fcb[idx].type != FCB_TYPE_UNION || fcb[idx].fs >= nfsdrv || !fsdrv[fcb[idx].fs].read)
        return -1U;
/*dbg_printf("fcb_unionlist_build(%d,%x,%d)\n",idx,buf,s);*/
    bs = fcb[fcb[idx].data.uni.storage].data.device.blksize;
    ptr = fcb_readlink(idx);
/*dbg_printf("ptr%D",ptr);*/
    /* ptr-nek nem szabad üresnek lennie, mivel az unió inode-ba van ágyazva, és az inode már be van töltve */
    if(!ptr || !ptr[0] || ackdelayed) return -1U;
    n = 1;
    for(i = 0; i < bs && !(ptr[i] == 0 && ptr[i-1] == 0); i++){
        l = strlen(ptr+i);
        k = -1U;
        if(l > 3) {
            for(j = 0; j < l-3; j++) {
                if(ptr[i+j] == '.' && ptr[i+j+1] == '.' && ptr[i+j+2] == '.') {
                    k = j;
                    break;
                }
            }
        }
        /* ha joker van az unió egyik tagjában */
        if(k != -1U) {
            fn = strndup(ptr+i,k+1);
            if(!fn) return -1U;
            f = lookup(fn, false);
/*dbg_printf("  unionlist '%s'...'%s' %d buf=%x s=%d\n",fn,ptr+i+k+4,f,buf,s);*/
            if(ackdelayed) { free(fn); return -1U; }
            if(f != -1U && fcb[f].fs < nfsdrv && fsdrv[fcb[f].fs].getdirent) {
                if(!buf) return f;
                j = 0; l = 1;
                fn = realloc(fn,pathmax);
                if(!fn) continue;
                while(j < s && l != 0) {
                    fn[k] = 0;
                    memzero(&dirent.d_name, FILENAME_MAX);
                    l = (*fsdrv[fcb[f].fs].getdirent)((uint8_t*)buf + j, j, &dirent);
                    if(dirent.d_name[0] && dirent.d_name[strlen(dirent.d_name)-1] == '/') {
                        pathcat(fn, dirent.d_name);
                        pathcat(fn, ptr + i + k+4);
                        f = lookup(fn, false);
/*dbg_printf("  unionlist2 '%s' %d\n",fn,f);*/
                        n = fcb_unionlist_add(&fl, f, n);
                        if(!n) return -1U;
                    }
                    j += l;
                }
            }
            free(fn);
        } else {
            f = lookup(ptr+i,false);
            if(f != -1U) {
/*dbg_printf("  unionlist '%s' %d\n",ptr+i,f);*/
                if(ackdelayed) return -1U;
                n = fcb_unionlist_add(&fl, f, n);
                if(!n) return -1U;
            }
        }
        i += l;
    }
/*dbg_printf(" unionlist done fl %x n %d errno %d\n",fl,n,errno());*/
    /* kész vagyunk, és többször nem hívjuk az fcb_unionlist_build-et, most megnövelhetjük a hivatkozások számát a tagokon */
    if(fl && n > 1) {
        for(i = 0; i < n; i++)
            fcb[fl[i]].nopen++;
        seterr(SUCCESS);
    }
    fcb[idx].data.uni.unionlist = fl;
/*fcb_dump();*/
    return -1U;
}

/**
 * adatok írása egy fcb-be, a módosítás bufferbe kerül
 */
public bool_t fcb_write(fid_t idx, off_t offs, void *buf, size_t size)
{
    fcb_t *f;
    writebuf_t *w, *nw, *l = NULL;
    size_t s;
    fpos_t e, we;
    bool_t chk;

    /* csak fájlokat kezelünk itt. A többit a taskctx_write kezeli, eszközöknél meg nincs írási buffer,
     * hanem blokk gyorsítótárat használnak writeblobk()-al, végezetül a dirent-et máshol kezeljük le. */
    if(idx >= nfcb) { seterr(EFAULT); return false; }
    f = &fcb[idx];
    if(f->type != FCB_TYPE_REG_FILE) { seterr(EBADF); return false; }
    /* paraméterek ellenőrzése */
    if(f->data.reg.storage != -1U && fsck.dev == f->data.reg.storage) { seterr(EBUSY); return false; }
    if(!(f->mode & O_WRITE)) { seterr(EACCES); return false; }
    if(f->fs >= nfsdrv || !fsdrv[f->fs].write) { seterr(EROFS); return 0; }
    /* új írási buffer lefoglalása */
    nw = (writebuf_t*)malloc(sizeof(writebuf_t));
    if(!nw) return false;
    nw->data = (void*)malloc(size);
    nw->offs = offs;
    nw->size = size;
    e = offs + size;
    if(!nw->data) return false;
    /* kitöröljük azokat a régi buffereket, melyeket az új teljesen kitakar */
    l = NULL;
    for(w = f->data.reg.buf; w; w = w->next) {
        we = w->offs + w->size;
        /* ##############       offs,e
         *  +--+ +----+         w->offs,we */
        if(((fpos_t)offs <= w->offs && e >= we) || !w->data) {
            if(l) l->next = w->next; else f->data.reg.buf = w->next;
            if(w->data) { free(w->data); } free(w); w = l;
        }
        l = w;
    }
    l = NULL; chk = false;
    for(w = f->data.reg.buf; w; w = w->next) {
        we = w->offs + w->size;
        /* #######              offs,e
         *          +--------+  w->offs,we */
        if(e < w->offs) {
            if(l) l->next = nw; else f->data.reg.buf = nw;
            nw->next = w; memcpy(nw->data, buf, size); chk = true;
            break;
        } else
        /* ########
         *    +-------+ */
        if((fpos_t)offs < w->offs && e >= w->offs && e < we) {
            s = we-offs;
            nw->data = (void*)realloc(nw->data, s); if(!nw->data) return false;
            memcpy(nw->data, buf, size); memcpy(nw->data + size, w->data, we - e);
            w->offs = offs; w->size = s; free(w->data); w->data = nw->data; free(nw); chk = true;
            break;
        } else
        /*     ####
         *  +--------+ */
        if((fpos_t)offs >= w->offs && e <= we) {
            memcpy(w->data + offs - w->offs, buf, size); free(nw->data); free(nw); chk = true;
            break;
        } else
        /*     ##########
         * +-------+        */
        if((fpos_t)offs > w->offs && (fpos_t)offs <= we && e > we) {
            w->data = (void*)realloc(nw->data, we - offs); if(!w->data) return false;
            memcpy(w->data + offs - w->offs, buf, size); free(nw->data); free(nw); chk = true;
            break;
        } else
        /*           ##########
         * +-------+             +----+?  */
        if((fpos_t)offs > we && (fpos_t)offs <= we && e > we && w->next) {
            nw->next = w->next; w->next = nw; memcpy(nw->data, buf, size); chk = true;
            break;
        }
    }
    if(!chk) {
#if DEBUG
        dbg_printf("fcb_write new buffer NOT ADDED! Should never happen!!!\n");
#endif
        if(l) { nw->next = l->next; l->next = nw; memcpy(nw->data, buf, size); chk = true; }
    }
    return chk;
}

/**
 * fcb írási buffer kiírása az eszközre (blokk gyorsítótárba)
 */
public bool_t fcb_flush(fid_t idx)
{
    fcb_t *f;
    writebuf_t *w;
    uint8_t *blk = NULL, *ptr;
    off_t o;
    size_t bs, s, a;

    /* paraméterek ellenőrzése */
    if(idx >= nfcb) { seterr(EFAULT); return false; }
    f = &fcb[idx];
    bs = fcb[f->data.reg.storage].data.device.blksize;
    /* plusz ellenrőrzések az első hívásnál */
    if(ctx->workleft == (size_t)-1) {
        if(f->type != FCB_TYPE_REG_FILE) { seterr(EBADF); return false; }
        if(f->data.reg.storage != -1U && fsck.dev == f->data.reg.storage) { seterr(EBUSY); return false; }
        if(!(f->mode & O_WRITE)) { seterr(EACCES); return false; }
        if(f->fs >= nfsdrv || !fsdrv[f->fs].write) { seterr(EROFS); return false; }
        if((int64_t)bs < 1) { seterr(EINVAL); return false; }
        ctx->workleft = 0;
        /* írás tranzakció kezdete üzenet küldése a fájlrendszer meghajtónak */
        if(fsdrv[f->fs].writetrb)
            (*fsdrv[f->fs].writetrb)(f->data.reg.storage, f->data.reg.inode);
    }
    /* minden írási buffert blokkcímre és méretre kell igazítani */
    for(w = f->data.reg.buf; f->data.reg.buf;) {
        /* ####
         *   +----+  */
        a = w->offs % bs;
        if(a) {
            if(fsdrv[f->fs].read) {
                /* blokk beolvasása, ennek a gyorsítótárban kell már lennie */
#if DEBUG
                if(_debug&DBG_FILEIO)
                    dbg_printf("FS: file read(fd %d, ino %d, offs %d, size %d)\n",
                        f->data.reg.storage, f->data.reg.inode, w->offs, bs);
#endif
                blk = (*fsdrv[f->fs].read)(f->data.reg.storage, f->data.reg.ver, f->data.reg.inode, w->offs, &bs);
                /* egyelőre végeztünk, ha nincs a gyorsítótárban vagy EOF */
                if(ackdelayed || !bs) return false;
            }
            /* biztonság kedvéért, a beolvasás kissebbre vehette */
            bs = fcb[f->data.reg.storage].data.device.blksize;
            ptr = malloc(w->size+a);
            if(!ptr) return false;
            if(blk)
                memcpy(ptr, blk, bs);
            memcpy(ptr + a, w->data, w->size);
            free(w->data);
            w->size += a;
            w->offs -= a;
            w->data = ptr;
            ptr = NULL;
        }
        /*     ####
         * +----+     */
        if(w->size % bs) {
            a = (w->size % bs);
            o = w->offs + w->size - a;
            s = w->size - a + bs;
            if(fsdrv[f->fs].read) {
                /* blokk beolvasása, ennek a gyorsítótárban kell már lennie */
#if DEBUG
                if(_debug&DBG_FILEIO)
                    dbg_printf("FS: file read(fd %d, ino %d, offs %d, size %d)\n",
                        f->data.reg.storage, f->data.reg.inode, o, bs);
#endif
                blk = (*fsdrv[f->fs].read)(f->data.reg.storage, f->data.reg.ver, f->data.reg.inode, o, &bs);
                /* egyelőre végeztünk, ha nincs a gyorsítótárban vagy EOF */
                if(ackdelayed || !bs) return false;
            }
            bs = fcb[f->data.reg.storage].data.device.blksize;
            w->data = realloc(w->data,s);
            if(!w->data) return false;
            if(blk)
                memcpy(w->data + w->size, blk + a, bs - a);
            w->size = s;
        }
        /* módosított blokk kiírása, ennek nem szabad false-al visszatérnie */
#if DEBUG
        if(_debug&DBG_FILEIO)
            dbg_printf("FS: file write(fd %d, ino %d, offs %d, buf %x, size %d)\n",
                f->data.reg.storage, f->data.reg.inode, w->offs, w->data, w->size);
#endif
        if(!(*fsdrv[f->fs].write)(f->data.reg.storage, f->data.reg.inode, w->offs, w->data, w->size))
            return false;
        /* kiszedjük a listából és felszabadítjuk az írási buffert */
        f->data.reg.buf = w->next;
        free(w->data);
        free(w);
    }
    /* írási tranzakció vége üzenet küldése a fájlrendszer meghajtónak */
    if(fsdrv[f->fs].writetre)
        (*fsdrv[f->fs].writetre)(f->data.reg.storage, f->data.reg.inode);
    ctx->workleft = -1U;
    return true;
}

#if DEBUG
/**
 * Fájl Kontrol Blokkok listájának dumpolása, debuggoláshoz
 */
void fcb_dump()
{
    uint64_t i, j;
    char *types[] = { "file", "dir ", "dev ", "pipe", "sock", "uni " };

    dbg_printf("File Control Blocks %d:\n", nfcb);
    for(i = 0; i < nfcb; i++) {
        dbg_printf("%3d. %3d ", i, fcb[i].nopen);
        if(!fcb[i].abspath) {
            dbg_printf("(free slot)\n");
            continue;
        }
        dbg_printf("%s %4d %8s ", types[fcb[i].type],
            fcb[i].type != FCB_TYPE_UNION? fcb[i].data.reg.blksize : 0, fcb[i].fs < nfsdrv? fsdrv[fcb[i].fs].name : "nofs");
        dbg_printf("%02x %c%c%c%c ", fcb[i].mode,
            fcb[i].mode&O_READ? 'r' : '-', fcb[i].mode&O_WRITE? 'w' : '-', fcb[i].mode&O_APPEND? 'a' : '-',
            fcb[i].flags&FCB_FLAG_EXCL? 'L' : (fcb[i].mode&O_EXCL? 'l' : '-'));
        dbg_printf("%3d:%3d %8d %s", fcb[i].data.reg.storage, fcb[i].data.reg.inode, fcb[i].data.reg.filesize, fcb[i].abspath);
        if(fcb[i].type == FCB_TYPE_UNION && fcb[i].data.uni.unionlist) {
            dbg_printf("\t[");
            for(j = 0; fcb[i].data.uni.unionlist[j]; j++)
                dbg_printf(" %d", fcb[i].data.uni.unionlist[j]);
            dbg_printf(" ]");
        }
        dbg_printf("\n");
    }
}
#endif
