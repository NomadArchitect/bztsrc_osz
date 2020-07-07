/*
 * fs/taskctx.c
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
 * @brief taszk kontextus fájlrendszer szolgáltatásokhoz
 */

#include <osZ.h>
#include "vfs.h"
#include "pipe.h"
#include "devfs.h"

/* taszk kontextusok */
uint64_t ntaskctx = 0;
taskctx_t *taskctx[256];

/* aktuális kontextus */
public taskctx_t *ctx;

/**
 * taszk kontextus visszaadása (ha nincs, akkor létrehoz egy újat)
 */
taskctx_t *taskctx_get(pid_t pid)
{
    taskctx_t *tc=taskctx[pid & 0xFF], *ntc;

    if(!pid) return NULL;
    /* megkeressük */
    while(tc) {
        if(tc->pid == pid)
            return tc;
        tc = tc->next;
    }
    /* ha nem találtuk, akkor hozzáadunk egyet */
    ntc = (taskctx_t*)malloc(sizeof(taskctx_t));
    if(!ntc) return NULL;
    ntc->pid = pid;
    /* belinkeljük */
    if(tc)
        tc->next = ntc;
    else
        taskctx[pid & 0xFF] = ntc;
    fcb[0].nopen += 2;
    ntaskctx++;
    return ntc;
}

/**
 * taszk kontextus eltávolítása
 */
void taskctx_del(pid_t pid)
{
    taskctx_t *tc = taskctx[pid & 0xFF], *ltc = NULL;
    uint64_t i;

    if(!tc) return;
    /* megkeressük */
    while(tc->next) {
        if(tc->pid == pid)
            break;
        ltc = tc;
        tc = tc->next;
    }
    /* kivesszük a listából */
    if(!ltc)
        taskctx[pid & 0xFF] = tc->next;
    else
        ltc->next = tc->next;
    ntaskctx--;
    /* memória felszabadítása */
    fcb_del(tc->workdir);
    fcb_del(tc->rootdir);
    if(tc->openfiles) {
        for(i = 0; i < tc->nopenfiles; i++)
            fcb_del(tc->openfiles[i].fid);
        free(tc->openfiles);
    }
    free(tc);
}

/**
 * gyökérkönyvtár beállítása
 */
void taskctx_rootdir(taskctx_t *tc, fid_t fid)
{
    if(!tc || fid >= nfcb || !fcb[fid].abspath)
        return;
    fcb_del(tc->rootdir);
    tc->rootdir = fid;
    fcb[fid].nopen++;
}

/**
 * munkakönyvtár beállítása
 */
void taskctx_workdir(taskctx_t *tc, fid_t fid)
{
    if(!tc || fid >= nfcb || !fcb[fid].abspath)
        return;
    fcb_del(tc->workdir);
    tc->workdir = fid;
    fcb[fid].nopen++;
}

/**
 * megnyitott fájlok listájához hozzáadás
 */
uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs, uint64_t idx)
{
    uint64_t i;
    fcb_t *f = NULL;
    openfiles_t *of = NULL;
    pipe_t *p = NULL;

    if(!tc) {
        seterr(ENOENT);
        return 0;
    }
    /* ellenőrizzük, hogy a kért hozzáférés elérhető-e az eszközön */
    if(!(mode & O_READ) && !(mode & O_WRITE)) mode |= O_READ;
    if((mode & O_CREAT) || (mode & O_APPEND)) mode |= O_WRITE;
    if(!(mode & OF_MODE_TTY)) {
        if(fid >= nfcb || !fcb[fid].abspath) {
            seterr(ENOENT);
            return 0;
        }
        f = &fcb[fid];
        /* megnézzük, kizárólagos hozzáférésre meg lett-e már nyitva */
        if(f->flags & FCB_FLAG_EXCL) {
            seterr(EBUSY);
            return 0;
        }
        /* hozzáférési jelzők lefedik, vagy szükség van creat vagy append és írás hozzáférésre */
        if(!((mode & O_AMSK) & (f->mode & O_AMSK)) && !((mode & (O_APPEND|O_CREAT)) && (f->mode & O_WRITE))) {
            seterr(EACCES);
            return 0;
        }
        /* nevesített csővezetékek megnyitása */
        if(f->type == FCB_TYPE_PIPE) {
            if(f->data.pipe.idx >= npipe || pipe[f->data.pipe.idx].fid != fid) {
#if DEBUG
                if(_debug&DBG_FILEIO)
                    dbg_printf("FS-ERR: named pipe mismatch fid=%d pipe=%d pipe.fid=%d\n",
                        fid,f->data.pipe.idx, pipe[f->data.pipe.idx].fid);
#endif
                seterr(EPIPE);
                return 0;
            }
            mode |= O_FIFO;
            /* fcb id lecserélése pipe id-ra */
            fid = f->data.pipe.idx;
        }
    }
    /* csővezeték megnyitása */
    if(mode & O_FIFO) {
        if(fid == -1U || fid >= npipe) {
            seterr(EPIPE);
            return 0;
        }
        p = &pipe[fid];
    }
    /* fájl leíró index kényszerítése */
    if(idx != 0 && idx <= tc->nopenfiles && tc->openfiles[idx-1].fid == -1U) {
        i = idx-1;
        goto found;
    }
    /* első üres hely keresése */
    if(tc->openfiles) {
        for(i = 0; i < tc->nopenfiles; i++)
            if(tc->openfiles[i].fid == -1U)
                goto found;
    } else
        tc->nopenfiles = 0;
    /* ha nem találtunk, akkor hozzáadunk egyet */
    i = tc->nopenfiles;
    tc->nopenfiles++;
    tc->openfiles = (openfiles_t*)realloc(tc->openfiles, tc->nopenfiles * sizeof(openfiles_t));
    if(!tc->openfiles) return 0;
    /* nyitott fájl leíró hozzáadása */
found:
    of = &tc->openfiles[i];
    /* TTY fájlok esetén a fid az ablak id, csővezetékeknél a pipe id, egyébként fcb id */
    of->fid = fid;
    of->mode = mode;
    of->offs = offs;
    of->unionidx = 0;
    tc->nfiles++;
    if(f) {
        f->nopen++;
        /* kizárólagos hozzáférés lekezelése. Vagy jelzőbittel kérték, vagy speciális eszköz */
        if((mode & O_EXCL) || (f->mode & O_EXCL)) f->flags |= FCB_FLAG_EXCL;
    }
    if(p) p->nopen++;
    return i+1;
}

/**
 * eltávolítás a megnyitott fájlok listájából
 */
bool_t taskctx_close(taskctx_t *tc, uint64_t idx, bool_t dontfree)
{
    fcb_t *f;
    openfiles_t *of;
    fid_t fid = -1U;

    if(!taskctx_validfid(tc,idx)) return false;
    idx--;
    of = &tc->openfiles[idx];
    if(of->mode & O_FIFO) {
        /* elmentjük az fcb id-t, mivel a pipe_del() felszabadíthatja a rekordot */
        if(pipe[of->fid].fid) fid = pipe[of->fid].fid;
        /* olvasó pid felszabadítása */
        if(pipe[of->fid].reader == tc->pid) pipe[of->fid].reader = 0;
        pipe_del(of->fid);
        /* nevesített csővezeték esetén az fcb-t is lezárjuk */
        if(pipe[of->fid].fid && fid != -1U) {
            of->fid = fid;
            if(!of->fid) of->fid = -1U;
        }
    }
    /* fcb lezárása */
    if(!(of->mode & OF_MODE_TTY) && of->fid != -1U && of->fid < nfcb) {
        f = &fcb[of->fid];
        if(of->mode & O_WRITE && !(of->mode & O_FIFO)) fcb_flush(of->fid);
        if(of->mode & O_TMPFILE && f->mode!=FCB_TYPE_REG_DIR) {
            /*unlink(f->abspath);*/
        }
        f->flags &= ~FCB_FLAG_EXCL;
        fcb_del(of->fid);
    }
    of->fid = -1U;
    tc->nfiles--;
    if(dontfree) return true;
    idx = tc->nopenfiles;
    while(idx > 0 && tc->openfiles[idx-1].fid == -1U) idx--;
    if(idx != tc->nopenfiles) {
        tc->nopenfiles = idx;
        if(!idx) {
            free(tc->openfiles);
            tc->openfiles = NULL;
        } else {
            tc->openfiles = (openfiles_t*)realloc(tc->openfiles, tc->nopenfiles * sizeof(openfiles_t));
            if(!tc->openfiles) return false;
        }
    }
    return true;
}

/**
 * pozíció beállítása megnyitott fájl leírón
 */
bool_t taskctx_seek(taskctx_t *tc, uint64_t idx, off_t offs, uint8_t whence)
{
    openfiles_t *of;

    if(!taskctx_validfid(tc, idx)){
        seterr(EBADF);
        return false;
    }
    idx--;
    of = &tc->openfiles[idx];
    if(of->mode & OF_MODE_TTY || of->mode & O_FIFO || fcb[of->fid].type != FCB_TYPE_REG_FILE) {
        seterr(ESPIPE);
        return false;
    }
    if(of->mode & O_APPEND) {
        seterr(EACCES);
        return false;
    }
    switch(whence) {
        case SEEK_CUR:
            of->offs += offs;
            break;
        case SEEK_END:
            of->offs = fcb[of->fid].data.reg.filesize + offs;
            break;
        default:
            of->offs = offs;
            break;
    }
    if((off_t)of->offs < 0) {
        of->offs = 0;
        seterr(EINVAL);
        return false;
    }
    return true;
}

/**
 * annak ellenőrzése, hogy a fájl leíró érvényes-e az adott kontextusban
 */
bool_t taskctx_validfid(taskctx_t *tc, uint64_t idx)
{
    return (idx > 0 && tc && tc->openfiles && tc->nopenfiles >= idx && tc->openfiles[idx-1].fid != -1U &&
        /* érvényes csővezetékre mutat */
        (((tc->openfiles[idx-1].mode & O_FIFO) && tc->openfiles[idx-1].fid < npipe && pipe[tc->openfiles[idx-1].fid].nopen > 0) ||
        tc->openfiles[idx-1].mode & OF_MODE_TTY ||
        /* érvényes fcb rekordra mutat */
        (tc->openfiles[idx-1].fid < nfcb && fcb[tc->openfiles[idx-1].fid].abspath)));
}

/**
 * a soronkövetkező könyvtárbejegyzés (dirent) beolvasása
 */
dirent_t *taskctx_readdir(taskctx_t *tc, fid_t idx, void *ptr, unused size_t size)
{
    fcb_t *fc;
    size_t s;
    fid_t f;
    openfiles_t *of;
    char *abspath;

    if(!taskctx_validfid(tc, idx)) {
        seterr(EBADF);
        return NULL;
    }
    idx--;
    of = &tc->openfiles[idx];
    if(of->mode & O_FIFO || of->mode & OF_MODE_TTY) {
        seterr(EPIPE);
        return NULL;
    }
    memzero(&dirent,sizeof(dirent_t));
    fc = &fcb[of->fid];
    /* a beépített devfs bejegyzései */
    if(fc->fs == devfsidx) {
        s = devfs_getdirent(of->offs);
        of->offs += s;
        if(!s || !dirent.d_name[0]) {
            of->offs = ndev;
            seterr(ENOENT);
            return NULL;
        }
        return &dirent;
    }
    /* többi fájlrendszer esetén */
    if(fc->fs >= nfsdrv || !fsdrv[fc->fs].getdirent) {
        seterr(ENOTSUP);
        return 0;
    }
    /* ellenőrizzük, hogy zárolt-e az eszköz */
    if(fsck.dev == fc->data.reg.storage) {
        seterr(EBUSY);
        return 0;
    }
    /* fájlrendszermeghajtó hívása dirent beolvasáshoz */
    s = (*fsdrv[fc->fs].getdirent)(ptr, of->offs, &dirent);
    if(!s || !dirent.d_name[0]) return NULL;
    if(!dirent.d_len) dirent.d_len = strlen(dirent.d_name);
    dirent.d_dev = fc->data.reg.storage;
    /* sima fájlt feltételezünk alapból */
    dirent.d_type = FCB_TYPE_REG_FILE;
    /* fájlrendszermeghajtó hívása a stat_t kitöltéséhez */
    memzero(&st,sizeof(stat_t));
    if(fsdrv[fc->fs].stat && (*fsdrv[fc->fs].stat)(fc->data.reg.storage, dirent.d_ino, &st)) {
        dirent.d_type = st.st_mode >> 16;
        dirent.d_filesize = st.st_size;
        /* ha szimbólikus hivatkozás, akkor lekérjük a cél típusát és méretét */
        if(lastlink) {
            f = -1U;
            if(lastlink[0] == '/') {
                f = lookup(lastlink, false);
            } else {
                abspath = (char*)malloc(pathmax);
                if(abspath) {
                    strcpy(abspath, fc->type == FCB_TYPE_UNION ? fcb[fc->data.uni.unionlist[of->unionidx]].abspath : fc->abspath);
                    pathcat(abspath, lastlink);
                    f = lookup(abspath, false);
                    free(abspath);
                }
            }
            if(ackdelayed) return NULL;
            if(f != -1U && fcb[f].fs < nfsdrv && (*fsdrv[fcb[f].fs].stat)(fcb[f].data.reg.storage, fcb[f].data.reg.inode, &st)) {
                    dirent.d_type &= ~(S_IFMT >> 16);
                    dirent.d_type |= st.st_mode >> 16;
                    dirent.d_filesize = st.st_size;
                    /* az ikont a módosított stat bufferből másoljuk majd */
            }
        }
        memcpy(&dirent.d_icon, &st.st_type, 8);
        if(dirent.d_icon[3] == ':') dirent.d_icon[3] = 0;
    }
    /* pozíció mozgatása */
    of->offs += s;
    if(fc->type == FCB_TYPE_UNION && of->offs >= fcb[fc->data.uni.unionlist[of->unionidx]].data.uni.filesize) {
        of->offs = 0;
        of->unionidx++;
    }
    return &dirent;
}

/**
 * olvasás fájlból, ptr vagy egy virtuális cím ctx->pid címterében (nem írható direktben) vagy megosztott memória (írható)
 */
size_t taskctx_read(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size)
{
    size_t bs, ret=0;
    uint8_t *blk, *p;
    fcb_t *f;
    openfiles_t *of;
    writebuf_t *w;
    fpos_t o, e, we;

    if(!taskctx_validfid(tc, idx)) {
        seterr(EBADF);
        return 0;
    }
    idx--;
    of = &tc->openfiles[idx];
    /* ha if->fid egy pipe id */
    if(of->mode & O_FIFO) return pipe_read(of->fid, ptr, size, tc->pid);
    /* egyébként of->fid egy fcb id */
    f = &fcb[of->fid];
    /* beépített devfs bejegyzés? */
    if(f->fs == devfsidx && (of->mode & OF_MODE_READDIR)) return of->offs < ndev;
    if(!(f->mode & O_READ)) { seterr(EACCES); return 0; }
    switch(f->type) {
        case FCB_TYPE_PIPE:
            break;

        case FCB_TYPE_SOCKET:
            break;

        default:
            /* ellenőrizzük, hogy zárolt-e az eszköz */
            if(f->data.reg.storage != -1U && fsck.dev == f->data.reg.storage) { seterr(EBUSY); return 0; }
            if(f->type == FCB_TYPE_UNION && (of->mode & OF_MODE_READDIR)) {
                if(!f->data.uni.unionlist[of->unionidx]) return 0;
                f = &fcb[f->data.uni.unionlist[of->unionidx]];
            }
            if(f->fs >= nfsdrv || !fsdrv[f->fs].read) { seterr(EACCES); return 0; }
            /* megnézzük, hogy az olvasás teljes egészében write buf-ban van-e */
            if(tc->workleft == (size_t)-1) {
                e = of->offs + size;
                for(w = f->data.reg.buf; w; w = w->next) {
                    /*     ####
                     *  +--------+   */
                    if(of->offs >= w->offs && e <= w->offs + w->size) {
                        if((int64_t)ptr < 0)
                            memcpy((void*)ptr, w->data + of->offs - w->offs, size);
                        else
                            tskcpy(tc->pid, (void*)ptr, w->data + of->offs - w->offs, size);
                        return size;
                    }
                }
                /* nem, fel kell bontani darabokra és külön-külön visszaadni */
                /* induló értékek */
                tc->workleft = size; tc->workoffs = 0;
            }
/*dbg_printf("readfs storage %d file %d offs %d\n",fc->data.reg.storage, tc->openfiles[idx].fid, tc->openfiles[idx].offs);*/
            /* minden blokkot beolvasunk */
            while(tc->workleft != (size_t)-1 && tc->workleft > 0) {
                /* egy blokk olvasása */
                bs = fcb[f->data.reg.storage].data.device.blksize;
/*dbg_printf("readfs %d %d left %d\n",tc->openfiles[idx].offs + tc->workoffs,bs,tc->workleft);*/
                blk = NULL;
                /* megnézzük, hogy a blokkot ki tudjuk-e szolgálni write buf-ból */
                o = of->offs + tc->workoffs; e = o + bs;
                for(w = f->data.reg.buf; w; w = w->next) {
                    /*     ####
                     *  +--------+   */
                    if(o >= w->offs && e <= w->offs + w->size) {
                        blk = w->data + o - w->offs;
                        break;
                    }
                }
                if(!blk) {
                    /* fájlrendszer meghajtó hívása */
#if DEBUG
                    if(_debug&DBG_FILEIO)
                        dbg_printf("FS: file read(fd %d, ino %d, offs %d, size %d)\n",
                            f->data.reg.storage, f->data.reg.inode, o, bs);
#endif
                    blk = (*fsdrv[f->fs].read)(f->data.reg.storage, f->data.reg.ver, f->data.reg.inode, o, &bs);
/*dbg_printf("readfs ret %x bs %d, delayed %d\n",blk,bs,ackdelayed);*/
                    /* ha nincs a blokkgyorsítótárban */
                    if(ackdelayed) break;
                    /* ha vége a fájlnak */
                    if(!bs) {
                        ret = tc->workoffs; tc->workleft = -1; break;
                    } else if(!blk) {
                        /* hézag a fájlban? csupa nullákat adunk vissza */
                        zeroblk = (void*)realloc(zeroblk, fcb[f->data.reg.storage].data.device.blksize);
                        blk = zeroblk;
                    }
                    /* összevetjük a write buf-al */
                    for(w = f->data.reg.buf; w; w = w->next) {
                        we = w->offs + w->size;
                        /* ####
                         *   +----+  */
                        if(o < w->offs && e > w->offs && e <= we) {
                            p = malloc(we - o);
                            if(!p) break;
                            memcpy(p, blk, w->offs - o); memcpy(p + w->offs - o, w->data, w->size);
                            free(w->data); w->size = we - o; w->offs = o; w->data = p; blk = p;
                            break;
                        }
                        /*     ####
                         * +----+    */
                        if(o > w->offs && o < we && e > we) {
                            w->data = realloc(w->data, e - w->offs);
                            if(!w->data) break;
                            memcpy(w->data + w->size, blk + we - o, e - we);
                            w->size = e - w->offs; blk = w->data + o - w->offs;
                            break;
                        }
                    }
                }
                /* biztonság kedvéért ellenőrizzük az olvasandó bájtok számát */
                if(bs >= tc->workleft) { bs = tc->workleft; tc->workleft = -1; ret = tc->workoffs + bs; } else tc->workleft -= bs;
                /* eredmény másolása a hívónak. Ha megosztott, akkor direktben, egyébként syscall-al */
                if((int64_t)ptr < 0)
                    memcpy((void*)(ptr + tc->workoffs), blk, bs);
                else
                    tskcpy(tc->pid, (void*)(ptr + tc->workoffs), blk, bs);
                tc->workoffs += bs;
            }
            /* ha végeztünk az olvasással, és nem readdir, akkor pozíciót is állítjuk */
            if(!(of->mode & OF_MODE_READDIR)) of->offs += ret;
            break;
    }
    return ret;
}

/**
 * írás fájlba, ptr akár virtuális cím, akár megosztott memóriacím, mindenképp olvasható
 */
size_t taskctx_write(taskctx_t *tc, fid_t idx, void *ptr, size_t size)
{
    fcb_t *f;
    openfiles_t *of;

    if(!taskctx_validfid(tc,idx)) {
        seterr(EBADF);
        return 0;
    }
    idx--;
/*dbg_printf("taskctx_write %d%2D",size,ptr);*/
    of = &tc->openfiles[idx];
    if(!(of->mode & O_WRITE) && !(of->mode & O_APPEND)) { seterr(EACCES); return 0; }
    /* ha of->fid egy pipe id */
    if(of->mode & O_FIFO) {
        pipe_write(of->fid, (virt_t)ptr, size);
        return size;
    }
    /* ha tty cső, akkor of->fid egy window id */
    if(of->mode & OF_MODE_TTY) {
        mq_send((virt_t)ptr, size, 0, 0, 0, 0, EVT_DEST(SRV_UI) | SYS_datapacket | MSG_PTRDATA, 0);
        ackdelayed = true;
        return size;
    }
    /* egyébként of->fid egy fcb id */
    f = &fcb[of->fid];
    if(!(f->mode & O_WRITE)) { seterr(EACCES); return 0; }
    switch(f->type) {
        case FCB_TYPE_PIPE:
            /* nevesített csővezetékeket már lekezeltük a névtelenekkel együtt */
            break;

        case FCB_TYPE_SOCKET:
            break;

        case FCB_TYPE_DEVICE:
            break;

        case FCB_TYPE_REG_DIR:
        case FCB_TYPE_UNION:
            /* ide sose szabadna elérni, de a biztonság kedvéért állítunk hibakódot */
            seterr(EPERM);
            break;

        default:
            if(fcb_write(of->fid, of->offs, ptr, size)) {
                of->offs += size;
                return size;
            }
            break;
    }
    return 0;
}


#if DEBUG
/**
 * taszk kontextusok dumpolása debuggoláshoz
 */
void taskctx_dump()
{
    uint i,j;
    taskctx_t *tc;

    dbg_printf("Task Contexts %d:\n", ntaskctx);
    for(i = 0; i < 256; i++) {
        if(!taskctx[i]) continue;
        tc = taskctx[i];
        while(tc) {
            dbg_printf(" %x %s%02x ", tc, ctx==tc?"*":" ", tc->pid);
            dbg_printf("root %3d pwd %3d open %2d:", tc->rootdir, tc->workdir, tc->nopenfiles);
            for(j = 0; j < tc->nopenfiles; j++)
                dbg_printf(" (%d,%1x,%d)", tc->openfiles[j].fid, tc->openfiles[j].mode, tc->openfiles[j].offs);
            dbg_printf("\n");
            tc = tc->next;
        }
    }
}
#endif
