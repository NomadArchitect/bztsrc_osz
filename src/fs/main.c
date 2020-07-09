/*
 * fs/main.c
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
 * @brief FS taszk
 */

#include <osZ.h>
#include <driver.h>
#include "../libc/libc.h"
#include "mtab.h"
#include "devfs.h"
#include "vfs.h"
#include "pipe.h"

public char *_fstab_ptr = NULL;     /* mutató az /etc/fstab adataira (valós idejű linkelő tölti ki) */
public size_t _fstab_size = 0;      /* fstab mérete (valós idejű linkelő tölti ki) */
public uint32_t pathmax;            /* elérési út maximális hossza */
extern uint32_t cachelines;         /* blokkgyorsítórár vonalainak száma */
extern uint32_t cachelimit;         /* ha ennyi százaléknál kevesebb a szabad RAM, fel kell szabadítani a blokkgyorsítótárat */
bool_t ackdelayed;                  /* asszinkron blokk olvasás jelzőbitje */
bool_t nomem = false;               /* gyorsítótárat fell kell szabadítani jelzés */

extern stat_t st;                   /* tty csővezetétekhez kell */
extern bool_t rootmounted;          /* azt jelzi, hogy csatoltunk-e már fel root fájlrendszert */

public int main(unused int argc, unused char **argv)
{
    msg_t *msg;
    taskctx_t *tc;
    meminfo_t m;
    uint64_t ret, j, k;
    char *ptr;
    stat_t *pst;
    off_t o;

    /* inicializálás, hogy tudjuk mknod() hívásokat fogadni */
    mtab_init();
    devfs_init();
    cache_init();
    nomem = false;

    /* üzenetek feldolgozása */
    while(1) {
        msg = mq_recv();

        seterr(SUCCESS);
        ackdelayed = nomem;
        ret = 0; ptr = NULL;

        /* taskctx betöltése, illetve olyan üzenetek feldolgozása, melyekhez nem kell taskctx */
        switch(EVT_FUNC(msg->evt)) {
#if DEBUG
            case SYS_fsdump:
                taskctx_dump();
                fsdrv_dump();
                devfs_dump();
                mtab_dump();
                fcb_dump();
                cache_dump();
                continue;
#endif
            case SYS_mountfs:
                /* a core küldi induláskor, mikor az eszközmeghajtók inicializálódtak, és a lemezek elérhetők már */
#if DEBUG
                if(_debug&DBG_FS)
                    dbg_printf("FS: mountfs()\n");
#endif
                /* csak egyszer kaphatunk ilyen üzenetet */
                if(!rootmounted) {
                    mtab_fstab(_fstab_ptr, _fstab_size);
#if DEBUG
                    if(_debug&DBG_FS)
                        mtab_dump();
#endif
                    mq_send0(SRV_UI, EVT_ack);
                    mq_send0(SRV_syslog, EVT_ack);
                    mq_send0(SRV_inet, EVT_ack);
                    mq_send0(SRV_sound, EVT_ack);
                    mq_send0(SRV_print, EVT_ack);
                    mq_send0(SRV_init, EVT_ack);
                }
                continue;

            case SYS_nomem:
                /* ha elfogyott a memória, akkor kiírunk mindent a cache-ből, és felszabadítjuk a gyorsítótárat */
                fcb_free();
                cache_flush();
                nomem=true;
                continue;

            case SYS_ackblock:
                /* blokk kiírás visszaigazolás, a piszkos flaget töröljük a cache-ben, más teendő nincs vele */
                if(msg->data.scalar.arg0 >= ndev || fcb[dev[msg->data.scalar.arg0].fid].data.device.blksize <= 1)
                    continue;
                if(cache_cleardirty(msg->data.scalar.arg0, msg->data.scalar.arg1) && nomem) {
                    cache_free();
                    nomem=false;
                }
                continue;

            case SYS_setblock:
                /* ha megérkezett a blokk, akkor folytatjuk az eredeti üzenet feldolgozását */
                cache_setblock(
                    msg->data.buf.attr0/*fcbdev*/,
                    msg->data.buf.attr1/*offs*/,
                    msg->data.buf.ptr/*blk*/, BLKPRIO_NOTDIRTY);
                ctx = taskctx[msg->data.buf.attr2 & 0xFF]; if(!ctx) continue;
                while(ctx) { if(ctx->pid == msg->data.buf.attr2) break; ctx = ctx->next; }
                if(!ctx || !EVT_FUNC(ctx->msg.evt)) continue;
                msg = &ctx->msg;
            break;

            case EVT_exit:
                /* exit üzenet a core-tól jön, az igazi küldő pid az első paraméterben van */
                ctx = taskctx_get(msg->data.scalar.arg0);
                if(ctx) {
                    for(j = 0; j < ctx->nopenfiles; j++)
                        taskctx_close(ctx, j + 1, false);
                    for(j = 0; j < ndev; j++)
                        if(dev[j].drivertask == ctx->pid)
                            devfs_del(j);
                    fcb_free();
                }
                continue;

            default:
                ctx = taskctx_get(EVT_SENDER(msg->evt));
                if(ctx) ctx->workleft = -1;
        }

        /* most hogy már van taskctx (akár új kérés, akár ackdelayed-es) */
        switch(EVT_FUNC(msg->evt)) {
            /*** eszközmeghajtókhoz köthető üzenetek */
            case SYS_mknod:
                /* eszközfájl létrehozása */
#if DEBUG
                if(_debug&DBG_DEVICES)
                    dbg_printf("FS: mknod(%s,%d,%02x,%d,%d)\n",msg->data.buf.ptr,msg->data.buf.type,msg->data.buf.attr0,
                        msg->data.buf.attr1,msg->data.buf.attr2);
#endif
                if(!msg->data.buf.ptr || !msg->data.buf.size) {
                    seterr(EINVAL);
                    ret = -1UL;
                } else {
                    devfs_add(msg->data.buf.ptr, ctx->pid, msg->data.buf.type, msg->data.buf.attr0, msg->data.buf.attr1,
                        msg->data.buf.attr2);
                    ret = 0;
                }
                break;

            case SYS_ioctl:
#if DEBUG
                if(_debug&DBG_DEVICES)
                    dbg_printf("FS: ioctl(%d,%d,%s[%d])\n",msg->data.buf.attr1,msg->data.buf.attr0,msg->data.buf.ptr,
                        msg->data.buf.size);
#endif
                if(!taskctx_validfid(ctx,msg->data.buf.type) ||
                    fcb[ctx->openfiles[msg->data.buf.type-1].fid].type != FCB_TYPE_DEVICE) {
                        seterr(EBADF);
                        ret = -1UL;
                } else {
                    devfs_t *df = &dev[fcb[ctx->openfiles[msg->data.buf.type-1].fid].data.device.inode];
#if DEBUG
                    if(_debug&DBG_DEVICES)
                        dbg_printf("FS: ioctl from %x to pid %x dev %d\n",ctx->pid, df->drivertask, df->device);
#endif
                    mq_send5(df->drivertask, DRV_ioctl|(msg->data.buf.ptr && msg->data.buf.size>0 ? MSG_PTRDATA : 0),
                        msg->data.buf.ptr, msg->data.buf.size, df->device, msg->data.buf.attr0, ctx->pid);
                    ctx->msg.evt = 0;
                    continue;
                }
                break;

            /*** fájlrendszerkezelő üzenetek ***/
            case SYS_mount:
                if(!msg->data.buf.ptr || !msg->data.buf.size) {
                    seterr(EINVAL);
                    ret = -1UL;
                } else {
                    j = strlen((char*)msg->data.buf.ptr);
                    k = strlen((char*)msg->data.buf.ptr + j+1);
#if DEBUG
                if(_debug&DBG_FS)
                    dbg_printf("FS: mount(%s, %s, %s)\n",msg->data.buf.ptr, (char*)msg->data.buf.ptr+j+1,
                        (char*)msg->data.buf.ptr+j+1+k+1);
#endif
                    ret = mtab_add(msg->data.buf.ptr, (char*)msg->data.buf.ptr+j+1,
                        (char*)msg->data.buf.ptr+j+1+k+1) != (uint16_t)-1 ? 0 : -1UL;
#if DEBUG
                    if(_debug&DBG_FS)
                        mtab_dump();
#endif
                }
                break;

            case SYS_umount:
#if DEBUG
                if(_debug&DBG_FS)
                    dbg_printf("FS: umount(%s)\n", (char*)msg->data.buf.ptr);
#endif
                if(!msg->data.buf.ptr || !msg->data.buf.size) {
                    seterr(EINVAL);
                    ret = -1UL;
                } else {
                    ret = mtab_del((char*)msg->data.buf.ptr, (char*)msg->data.buf.ptr) ? 0 : -1UL;
                    fcb_free();
#if DEBUG
                    if(_debug&DBG_FS)
                        mtab_dump();
#endif
                }
                break;

            case SYS_fsck:
#if DEBUG
                if(_debug&DBG_DEVICES)
                    dbg_printf("FS: fsck(%s)\n",msg->data.buf.ptr);
#endif
                ret = lookup(msg->data.buf.ptr, false);
                if(fsck.dev != -1UL && fsck.dev != ret) {
                    seterr(EBUSY);
                    ret = -1UL;
                } else {
                    if(ret != -1UL && !errno()) {
                        ret = dofsck(ret, msg->data.buf.type) ? 0 : -1;
                        if(!ret)
                            break;
                    }
                    seterr(ENODEV);
                    ret = -1UL;
                }
                break;

            /*** elérési út kezelése ***/
            case SYS_chroot:
                ret = lookup(msg->data.buf.ptr, false);
                if(ret != -1UL && !errno()) {
                    if(fcb[ret].type != FCB_TYPE_REG_DIR) {
                        seterr(ENOTDIR);
                        ret = -1UL;
                    } else {
                        fcb_del(ctx->rootdir);
                        fcb[ret].nopen++;
                        ctx->rootdir = ret;
                        if(memcmp(fcb[ctx->workdir].abspath, fcb[ctx->rootdir].abspath, strlen(fcb[ctx->rootdir].abspath))) {
                            fcb_del(ctx->workdir);
                            fcb[ret].nopen++;
                            ctx->workdir = ret;
                        }
                        ret = 0;
                    }
                }
                break;

            case SYS_chdir:
                ret = lookup(msg->data.buf.ptr, false);
                if(ret!= -1UL && !errno()) {
                    if(fcb[ret].type != FCB_TYPE_REG_DIR && fcb[ret].type != FCB_TYPE_UNION) {
                        seterr(ENOTDIR);
                        ret = -1UL;
                    } else {
                        fcb_del(ctx->workdir);
                        if(memcmp(fcb[ret].abspath, fcb[ctx->rootdir].abspath, strlen(fcb[ctx->rootdir].abspath)))
                            ret = ctx->rootdir;
                        fcb[ret].nopen++;
                        ctx->workdir = ret;
                        ret = 0;
                    }
                }
                break;

            case SYS_getcwd:
                ptr = fcb[ctx->workdir].abspath;
                ret = strlen(fcb[ctx->rootdir].abspath);
                if(!memcmp(ptr, fcb[ctx->rootdir].abspath, ret))
                    ptr += ret - 1;
                else
                    ptr = "/";
                break;

            /*** könyvtárbejegyzések kezelése ***/
            case SYS_opendir:
                if(msg->data.buf.type == -1UL) {
/*dbg_printf("fs opendir(%s,%d)\n", msg->data.buf.ptr,msg->data.buf.type);*/
                    ret = lookup(msg->data.buf.ptr, false);
                } else {
/*dbg_printf("fs opendir(%x[%d],%d)\n", msg->data.buf.ptr,msg->data.buf.size,msg->data.buf.type);*/
                    if(!taskctx_validfid(ctx, msg->data.buf.type)) {
                        seterr(EBADF);
                        ret = -1UL;
                        break;
                    } else
                    if(!(ctx->openfiles[msg->data.buf.type-1].mode & O_READ) ||
                       !(fcb[ctx->openfiles[msg->data.buf.type-1].fid].mode & O_READ)) {
                            seterr(EACCES);
                            ret = -1UL;
                            break;
                    }
                    ret = ctx->openfiles[msg->data.buf.type-1].unionidx;
                    ctx->openfiles[msg->data.buf.type-1].unionidx = 0;
                }
                if(ret != -1UL && !errno()) {
                    if(fcb[ret].fs != devfsidx && fcb[ret].type != FCB_TYPE_REG_DIR && fcb[ret].type != FCB_TYPE_UNION) {
                        seterr(ENOTDIR);
                        ret = -1UL;
                    } else {
                        j = -1;
                        if(fcb[ret].type == FCB_TYPE_UNION) {
                            j = fcb_unionlist_build(ret, (void*)(msg->data.buf.type == -1UL ? NULL : msg->data.buf.ptr),
                                msg->data.buf.size);
                            if(ackdelayed) break;
                            if(j != -1UL) {
                                k = ret;
                                ret = taskctx_open(ctx, j, O_RDONLY, 0, msg->data.buf.type);
                                if(!ret)
                                    break;
                                ctx->openfiles[ret-1].unionidx = k;
                                pst = statfs(j);
                                if(!pst || !pst->st_size)
                                    taskctx_close(ctx, ret, true);
                                else {
                                    mq_send5(EVT_SENDER(msg->evt), EVT_ack, ret, SUCCESS, EVT_FUNC(msg->evt),
                                        msg->serial, pst->st_size);
                                    ctx->msg.evt = 0;
                                    continue;
                                }
                            }
                        }
                        if(msg->data.buf.type != -1UL) {
                            taskctx_close(ctx, msg->data.buf.type, true);
/*dbg_printf("opening an union at %d\n",ret,errno());fcb_dump();*/
                        }
                        ret = taskctx_open(ctx, ret, O_RDONLY, 0, msg->data.buf.type);
                        if(ret)
                            ctx->openfiles[ret-1].mode |= OF_MODE_READDIR;
                    }
                }
/*taskctx_dump();*/
                break;

            case SYS_readdir:
/*dbg_printf("fs readdir(%x,%d,%d,%x)\n", msg->data.buf.ptr, msg->data.buf.size, msg->data.buf.type,msg->data.buf.attr0);*/
                if(!taskctx_validfid(ctx,msg->data.buf.type) ||
                    (fcb[ctx->openfiles[msg->data.buf.type-1].fid].type != FCB_TYPE_REG_DIR &&
                    fcb[ctx->openfiles[msg->data.buf.type-1].fid].type != FCB_TYPE_UNION)) {
                    seterr(EBADF);
                    ret = 0;
                } else {
                    ptr = (char*)taskctx_readdir(ctx, msg->data.buf.type, msg->data.buf.ptr, msg->data.buf.size);
                    if(ptr && !errno()) {
                        tskcpy(EVT_SENDER(msg->evt), (void*)msg->data.buf.attr0, ptr, sizeof(dirent_t));
                        ret = 0;
                    }
                }
                break;

            /*** fájl meta adatok kezelése ***/
            case SYS_readlink:
/*dbg_printf("fs readlink(%s)\n",msg->ptr);*/
                ret = lookup(msg->data.buf.ptr, false);
                if(ret != -1UL && !errno()) {
                    ptr = fcb_readlink(ret);
                    if(ptr) {
                        mq_send4(EVT_SENDER(msg->evt), EVT_ack | MSG_PTRDATA, ptr, fcb[ret].data.reg.filesize,
                            EVT_FUNC(msg->evt), msg->serial);
                        ctx->msg.evt = 0;
                        continue;
                    }
                    seterr(EINVAL);
                }
                ret = 0;
                break;

            case SYS_realpath:
                ret = lookup(msg->data.buf.ptr, false);
                if(ret != -1UL && !errno())
                    ptr = fcb[ret].abspath;
                ret = 0;
                break;

            case SYS_dstat:
                if(msg->data.scalar.arg0 < nfcb && fcb[msg->data.scalar.arg0].type == FCB_TYPE_DEVICE) {
                    ret = msg->data.scalar.arg0;
                    goto dostat;
                }
                seterr(EINVAL);
                ret = 0;
                break;

            case SYS_fstat:
                if(taskctx_validfid(ctx,msg->data.scalar.arg0)) {
                    ret=ctx->openfiles[msg->data.scalar.arg0-1].fid;
                    if(ctx->openfiles[msg->data.scalar.arg0-1].mode & O_FIFO) {
                        /* névtelen csővezeték */
                        j = ctx->openfiles[msg->data.scalar.arg0-1].mode;
                        goto dopstat;
                    } else
                    if(ctx->openfiles[msg->data.scalar.arg0-1].mode & OF_MODE_TTY) {
                        /* tty stdout (nincs fcb se igazi csővezeték, szóval lehazudjuk csővezetéknek) */
                        memzero(&st,sizeof(stat_t));
                        st.st_dev = -1UL; /* névvel rendelkező csővezetékek itt a fid-et adnák vissza */
                        st.st_ino = ret;  /* jobb híjján a window id-t az inode-ban adjuk vissza */
                        st.st_mode = O_WRONLY|O_APPEND|S_IFTTY;
                        memcpy(st.st_type,"appl",4);
                        memcpy(st.st_mime,"pipe",4);
                        pst = &st;
                        goto statok;
                    }
                    goto dostat;
                }
                seterr(EBADF);
                ret = 0;
                break;

            case SYS_lstat:
                ret = lookup(msg->data.buf.ptr, false);
dostat:         if(ret != -1UL && !errno()) {
                    if(fcb[ret].type != FCB_TYPE_PIPE)
                        pst = statfs(ret);
                    else {
                        /* neves csővezeték */
                        ret = fcb[ret].data.pipe.idx;
                        j = O_RDWR|O_APPEND;
dopstat:                pst = pipe_stat(ret, j);
                        /* FIFO TTY (stdin) esetén a window id az egyébként használaton kívüli offs-ba kerül */
                        if(j & OF_MODE_TTY)
                            (pst)->st_ino = ctx->openfiles[msg->data.scalar.arg0-1].offs;
                    }
statok:             if(pst)
                        tskcpy(EVT_SENDER(msg->evt), (void*)msg->data.scalar.arg2, pst, sizeof(stat_t));
                    else
                        seterr(ENOENT);
                }
                ret = 0;
                break;

            /*** csővezetékek ***/
            case SYS_mkfifo:
/*dbg_printf("fs mkfifo(%s, %x)\n", msg->data.buf.ptr, msg->data.buf.type);*/
                ret = fcb_add(msg->data.buf.ptr, FCB_TYPE_PIPE);
                if(ret != -1UL)
                    fcb[ret].data.pipe.idx = pipe_add(ret, EVT_SENDER(msg->evt));
                break;

            case SYS_pipe:
/*dbg_printf("fs pipe()\n");*/
                ret = pipe_add(-1, EVT_SENDER(msg->evt));
                j = taskctx_open(ctx, ret, O_RDONLY|O_FIFO, 0, 0);
                k = taskctx_open(ctx, ret, O_WRONLY|O_APPEND|O_FIFO, 0, 0);
                if(errno() == SUCCESS && j && k)
                    mq_send4(EVT_SENDER(msg->evt), EVT_ack, j, k, EVT_FUNC(msg->evt), msg->serial);
                else
                    mq_send4(EVT_SENDER(msg->evt), EVT_ack, -1UL, errno(), EVT_FUNC(msg->evt), msg->serial);
                ctx->msg.evt = 0;
                continue;

            case SYS_getty:
/*dbg_printf("fs getty(%x,%x)\n", msg->arg0, msg->arg1);*/
                tc = taskctx_get(msg->data.scalar.arg0);
                if(tc) {
                    /* stdin, stdout, stderr bezárása */
                    taskctx_close(tc, STDIN_FILENO, false);
                    taskctx_close(tc, STDOUT_FILENO, false);
                    taskctx_close(tc, STDERR_FILENO, false);
                    ret = pipe_add(-1, msg->data.scalar.arg0);
                    if(ret != -1UL && !errno()) {
                        /* taszk sztandard csővezetékeit az UI ablakhoz irányítjuk */
                        j = taskctx_open(tc, ret, O_RDONLY|O_FIFO|OF_MODE_TTY, 0, STDIN_FILENO);
                        /* a window id lementése (fid most már egy csővezeték index) */
                        if(j) tc->openfiles[j-1].offs = msg->data.scalar.arg1;
                        /* nem igazi csővezeték, a fid egy window id */
                        taskctx_open(tc, msg->data.scalar.arg1, O_WRONLY|O_APPEND|OF_MODE_TTY, 0, STDOUT_FILENO);
                        taskctx_open(tc, msg->data.scalar.arg1, O_WRONLY|O_APPEND|OF_MODE_TTY, 0, STDERR_FILENO);
                    }
                }
                ret = 0;
                break;

            case SYS_evtty:
/*dbg_printf("fs evtty(%x%x, pid %x) %d\n", msg->arg1, msg->arg0, msg->arg2, errno());*/
                tc = taskctx_get(msg->data.scalar.arg2);
                /* írás a sztandard csővezetékébe */
                if(!taskctx_validfid(tc, STDIN_FILENO)) {
                    seterr(EBADF);
                    ret = 0;
                } else {
                    j = strlen((char*)&msg->data.scalar.arg0);
                    if(j)
                        ret = pipe_write(tc->openfiles[STDIN_FILENO].fid, (virt_t)&msg->data.scalar.arg0, j) ? j : 0;
                    else
                        ret = 0;
                }
                break;

            /*** fájlműveletek ***/
            case SYS_tmpfile:
/*dbg_printf("fs tmpfile()\n");*/
                ptr = malloc(128);
                if(!ptr) {
                    ret = -1UL;
                    break;
                }
                sprintf(ptr, "%s%lx%lx", P_tmpdir, ctx->pid, rand());
                ret = lookup(ptr, true);
                if(ret != -1UL && !errno())
                    ret = taskctx_open(ctx, ret, O_RDWR|O_TMPFILE, 0, -1);
                free(ptr);
/*taskctx_dump();*/
                break;

            case SYS_fopen:
/*dbg_printf("fs fopen(%s, %x, %d)\n", msg->data.buf.ptr, msg->data.buf.type, msg->data.buf.attr0);*/
                ret = lookup(msg->data.buf.ptr, msg->data.buf.type & O_CREAT ? true : false);
                if(ret != -1UL && !errno()) {
                    if(fcb[ret].type == FCB_TYPE_REG_DIR || fcb[ret].type == FCB_TYPE_UNION) {
                        seterr(EISDIR);
                        ret = 0;
                    } else {
                        if(msg->data.buf.attr0 != -1UL)
                            taskctx_close(ctx, msg->data.buf.attr0, true);
                        /* az elérési út offszet része */
                        o = getoffs(msg->data.buf.ptr);
                        if(o < 0) o += fcb[ret].data.reg.filesize;
                        if(msg->data.buf.type & O_APPEND) o = fcb[ret].data.reg.filesize;
                        ret = taskctx_open(ctx, ret, msg->data.buf.type, o, msg->data.buf.attr0);
                    }
                }
                if(ret == -1UL) ret = 0;
/*fcb_dump(); taskctx_dump();*/
                break;

            /*** műveletek fájlleírókon (fid) ***/
          /*case SYS_closedir:*/
            case SYS_fclose:
/*dbg_printf("fs %s %d\n",ctx->openfiles[msg->data.scalar.arg0-1].mode&OF_MODE_READDIR?"closedir":"fclose",msg->data.scalar.arg0);*/
                if(!taskctx_validfid(ctx, msg->data.scalar.arg0)) {
                    seterr(EBADF);
                    ret = -1UL;
                } else {
                    ret = taskctx_close(ctx, msg->data.scalar.arg0, false) ? 0 : -1;
                }
/*taskctx_dump();*/
                break;

            case SYS_fcloseall:
/*dbg_printf("fs fcloseall\n");*/
                for(j = 0; j < ctx->nopenfiles; j++)
                    taskctx_close(ctx, j+1, false);
                cache_flush(0);
                fcb_free();
                ret = 0;
/*taskctx_dump();*/
                break;

            case SYS_fseek:
                ret = taskctx_seek(ctx, msg->data.scalar.arg0, msg->data.scalar.arg1, msg->data.scalar.arg2) ? 0 : -1;
                if(ret && !errno())
                    seterr(EINVAL);
                break;

            case SYS_rewind:
                /* a taskctx_seek nem jó ide, mert ennek könyvtáraknál is működnie kell */
                if(!taskctx_validfid(ctx, msg->data.scalar.arg0)) {
                    seterr(EBADF);
                    ret = -1UL;
                } else {
                    ctx->openfiles[msg->data.scalar.arg0-1].offs = 0;
                    ctx->openfiles[msg->data.scalar.arg0-1].unionidx = 0;
                    ret = 0;
                }
                break;

            case SYS_ftell:
                if(!taskctx_validfid(ctx, msg->data.scalar.arg0)) {
                    seterr(EBADF);
                    ret = 0;
                } else
                    ret = ctx->openfiles[msg->data.scalar.arg0-1].offs;
                break;

            case SYS_feof:
                if(!taskctx_validfid(ctx, msg->data.scalar.arg0)) {
                    seterr(EBADF);
                    ret = -1UL;
                } else {
                    if(ctx->openfiles[msg->data.scalar.arg0-1].mode & O_FIFO)
                        ret = 0;
                    else
                        ret = !fcb[ctx->openfiles[msg->data.scalar.arg0-1].fid].abspath ||
                            (ctx->openfiles[msg->data.scalar.arg0-1].mode & OF_MODE_EOF) ||
                            ctx->openfiles[msg->data.scalar.arg0-1].offs >=
                                fcb[ctx->openfiles[msg->data.scalar.arg0-1].fid].data.reg.filesize;
                }
                break;

            case SYS_ferror:
                if(!msg->data.scalar.arg0 || !ctx || !ctx->openfiles || ctx->nopenfiles <= (uint64_t)msg->data.scalar.arg0) {
                    seterr(EBADF);
                    ret = -1UL;
                } else {
                    j = msg->data.scalar.arg0-1;
                    ret = (ctx->openfiles[j].fid == -1UL || (ctx->openfiles[j].mode & OF_MODE_EOF) ||
                        ((ctx->openfiles[j].mode & O_FIFO) && ctx->openfiles[j].fid >= npipe) ||
                        (ctx->openfiles[j].fid >= nfcb || !fcb[ctx->openfiles[j].fid].abspath) ? 1 : 0);
                }
                break;

            case SYS_fclrerr:
                if(!taskctx_validfid(ctx, msg->data.scalar.arg0)) {
                    seterr(EBADF);
                    ret = -1UL;
                } else {
                    ctx->openfiles[msg->data.scalar.arg0-1].mode &= 0xffff;
                    ret = 0;
                }
                break;

            case SYS_dup:
/*dbg_printf("fs dup(%d)\n",msg->data.scalar.arg0);*/
                if(!taskctx_validfid(ctx, msg->data.scalar.arg0)) {
                    seterr(EBADF);
                    ret = -1UL;
                } else {
                    ret = taskctx_open(ctx, ctx->openfiles[msg->data.scalar.arg0-1].fid,
                        ctx->openfiles[msg->data.scalar.arg0-1].mode,
                        ctx->openfiles[msg->data.scalar.arg0-1].offs, -1);
/*taskctx_dump();*/
                }
                break;

            case SYS_dup2:
/*dbg_printf("fs dup2(%d,%d)\n",msg->arg0,msg->arg1);*/
                if(!taskctx_validfid(ctx, msg->data.scalar.arg0)) {
                    seterr(EBADF);
                    ret = -1UL;
                } else {
                    /* ne használj itt task_validfid-et, csak az a lényeg, hogy a leíró ne legyen használatban */
                    if(ctx->nopenfiles >= msg->data.scalar.arg1 && ctx->openfiles[msg->data.scalar.arg1-1].fid != -1UL) {
                        taskctx_close(ctx, msg->data.scalar.arg1, true);
                    }
                    ret = taskctx_open(ctx, ctx->openfiles[msg->data.scalar.arg0-1].fid,
                        ctx->openfiles[msg->data.scalar.arg0-1].mode,
                        ctx->openfiles[msg->data.scalar.arg0-1].offs, msg->data.scalar.arg1);
/*taskctx_dump();*/
                }
                break;

            /*** írás / olvasás ***/
            case SYS_fread:
/*dbg_printf("fs fread(%d,%x,%d) offs %d mode %x\n",msg->data.buf.type,msg->ptr,msg->data.buf.size,
    ctx->openfiles[msg->data.buf.type-1].offs,ctx->openfiles[msg->data.buf.type-1].mode);*/
                if(MSG_ISPTR(msg->evt)) {
                    seterr(EINVAL);
                    ret = 0;
                } else
                /* az 1M-nél nagyobb buffereket megosztott memóriában kell átadni */
                if(msg->data.buf.size >= __BUFFSIZE && (int64_t)msg->data.buf.ptr > 0) {
                    seterr(ENOTSHM);
                    ret = 0;
                } else
                if(!msg->data.buf.ptr || !msg->data.buf.size) {
                    if(!msg->data.buf.ptr && msg->data.buf.size > 0)
                        seterr(EINVAL);
                    ret = 0;
                } else
                    ret = taskctx_read(ctx, msg->data.buf.type, (virt_t)msg->data.buf.ptr, msg->data.buf.size);
                break;

            case SYS_fwrite:
/*dbg_printf("fs fwrite(%d,%x,%d)\n",msg->data.buf.type,msg->data.buf.ptr,msg->data.buf.size);*/
                if(!taskctx_validfid(ctx, msg->data.buf.type) || (!(ctx->openfiles[msg->data.buf.type-1].mode & OF_MODE_TTY) && (
                   fcb[ctx->openfiles[msg->data.buf.type-1].fid].type == FCB_TYPE_REG_DIR ||
                   fcb[ctx->openfiles[msg->data.buf.type-1].fid].type == FCB_TYPE_UNION))) {
                    seterr(EBADF);
                    ret = 0;
                } else
                if(!msg->data.buf.ptr || !msg->data.buf.size) {
                    if(!msg->data.buf.ptr && msg->data.buf.size > 0)
                        seterr(EINVAL);
                    ret = 0;
                } else
                    ret = taskctx_write(ctx, msg->data.buf.type, msg->data.buf.ptr, msg->data.buf.size);
                break;

            case SYS_fflush:
/*dbg_printf("fs fflush(%d)\n",msg->data.scalar.arg0);*/
                if(!taskctx_validfid(ctx, msg->data.scalar.arg0)) {
                    seterr(EBADF);
                    ret = -1UL;
                } else
                if(!(ctx->openfiles[msg->data.scalar.arg0-1].mode & O_WRITE)) {
                    seterr(EACCES);
                    ret = -1UL;
                } else {
                    /* tty csővezeték lekezelése, a fid egy window id */
                    if(ctx->openfiles[msg->data.scalar.arg0-1].mode & OF_MODE_TTY) {
                        mq_send1(SRV_UI, SYS_flush, ctx->openfiles[msg->data.scalar.arg0-1].fid);
                        ret = 0;
                    } else
                        /* egyébként a fid egy fcb id */
                        ret = fcb_flush(ctx->openfiles[msg->data.scalar.arg0-1].fid) ? 0 : -1;
                }
                break;

            /*** elvileg ennek sosem szabad bekövetkeznie ***/
            default:
#if DEBUG
                dbg_printf("FS: unknown event: %x from pid %x\n",EVT_FUNC(msg->evt), EVT_SENDER(msg->evt));
#endif
                seterr(EPERM);
                ret = -1UL;
                break;
        }

        /* elébe megyünk az elfogyott a memória helyzeteknek */
        m = meminfo();
        j = m.free*100/m.total;
        if(j < cachelimit || errno() == ENOMEM) {
            fcb_free();
            cache_flush();
            nomem = true;
        }
        /* válasz küldése, vagy az erdeti üzenet elrakása későbbre */
        if(!ackdelayed) {
            if(!ret && ptr)
                mq_send4(EVT_SENDER(msg->evt), EVT_ack | MSG_PTRDATA, ptr, strlen(ptr) + 1, EVT_FUNC(msg->evt), msg->serial);
            else
                mq_send4(EVT_SENDER(msg->evt), EVT_ack, ret, errno(), EVT_FUNC(msg->evt), msg->serial);
            ctx->msg.evt = 0;
        } else {
            memcpy(&ctx->msg, msg, sizeof(msg_t));
        }
    }
    return EX_SOFTWARE;
}

