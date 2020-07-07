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
/*
    taskctx_t *tc;
*/
    meminfo_t m;
    uint64_t ret, j, k;
    char *ptr;

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

            /*** fájl meta adatok kezelése ***/
            case SYS_realpath:
                ret = lookup(msg->data.buf.ptr, false);
                if(ret != -1UL && !errno())
                    ptr = fcb[ret].abspath;
                ret = 0;
                break;

#if 0
            case SYS_dstat:
                if(msg->data.scalar.arg0 >= 0 && msg->data.scalar.arg0 < nfcb &&
                    fcb[msg->data.scalar.arg0].type == FCB_TYPE_DEVICE) {
                    ret = msg->arg0;
                    goto dostat;
                }
                seterr(EINVAL);
                ret = 0;
                break;

            case SYS_fstat:
                if(taskctx_validfid(ctx,msg->arg0)) {
                    ret=ctx->openfiles[msg->arg0-1].fid;
                    if(ctx->openfiles[msg->arg0-1].mode & O_FIFO) {
                        // unnamed pipe
                        j = ctx->openfiles[msg->arg0-1].mode;
                        goto dopstat;
                    } else
                    if(ctx->openfiles[msg->arg0-1].mode & OF_MODE_TTY) {
                        // tty stdout (no fcb or pipe equivalent, so let's fake it's a pipe)
                        memzero(&st,sizeof(stat_t));
                        st.st_dev=-1;   // named pipes return fid here, but ttys are unnamed
                        st.st_ino=ret;  // without better place, return windows id in inode field
                        st.st_mode=O_WRONLY|O_APPEND|S_IFTTY;
                        memcpy(st.st_type,"appl",4);
                        memcpy(st.st_mime,"pipe",4);
                        ptr=&st;
                        goto statok;
                    }
                    goto dostat;
                }
                seterr(EBADF);
                ret=0;
                break;

            case SYS_lstat:
                ret=lookup(msg->ptr, false);
dostat:         if(ret!=-1 && !errno()) {
                    if(fcb[ret].type!=FCB_TYPE_PIPE)
                        ptr=statfs(ret);
                    else {
                        // named pipe
                        ret=fcb[ret].pipe.idx;
                        j = O_RDWR|O_APPEND;
dopstat:                ptr=pipe_stat(ret, j);
                        // for FIFO TTY (stdin) we store window id in the otherwise unused offs field
                        if(j & OF_MODE_TTY)
                            ((stat_t*)ptr)->st_ino = ctx->openfiles[msg->arg0-1].offs;
                    }
statok:             if(ptr!=NULL)
                        p2pcpy(EVT_SENDER(msg->evt), (void*)msg->arg2, ptr, sizeof(stat_t));
                    else
                        seterr(ENOENT);
                }
                ret=0;
                break;

            /*** könyvtárbejegyzések kezelése ***/
#endif
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
        j=m.free*100/m.total;
        if(j<cachelimit || errno()==ENOMEM) {
            fcb_free();
            cache_flush();
            nomem=true;
        }
        /* válasz küldése, vagy az erdeti üzenet elrakása későbbre */
        if(!ackdelayed) {
            if(!ret && ptr)
                mq_send3(EVT_SENDER(msg->evt), EVT_ack | MSG_PTRDATA, ptr, strlen(ptr) + 1, msg->serial);
            else
                mq_send4(EVT_SENDER(msg->evt), EVT_ack, ret, errno(), EVT_FUNC(msg->evt), msg->serial);
            ctx->msg.evt = 0;
        } else {
            memcpy(&ctx->msg, msg, sizeof(msg_t));
        }
    }
    return EX_SOFTWARE;
}

