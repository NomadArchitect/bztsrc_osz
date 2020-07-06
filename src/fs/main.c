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

public int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    msg_t *msg;
    taskctx_t *tc;
    meminfo_t m;
    uint64_t ret = 0, j, k;

dbg_printf("fs1\n");
    mtab_init();
dbg_printf("fs2\n");
    devfs_init();
dbg_printf("fs3\n");
/*
    cache_init();
    nomem = false;
*/
    while(1) {
        msg = mq_recv();
dbg_printf("fs4\n");
#if 0
        seterr(SUCCESS);
        ackdelayed = nomem;
        ret=0;
        if(EVT_FUNC(msg->evt) == SYS_nomem) {
            fcb_free();
            cache_flush();
            nomem=true;
            continue;
        } else
        if(EVT_FUNC(msg->evt) == SYS_setblock) {
            cache_setblock(
                msg->data.buffer.attr0/*fcbdev*/,
                msg->data.buffer.attr1/*offs*/,
                msg->data.buffer.ptr/*blk*/, BLKPRIO_NOTDIRTY);
            ctx=taskctx[msg->data.buffer.attr2 & 0xFF]; if(ctx==NULL) continue;
            while(ctx!=NULL) { if(ctx->pid==msg->data.buffer.attr2) break; ctx=ctx->next; }
            if(ctx==NULL || EVT_FUNC(ctx->msg.evt)==0) continue;
            msg = &ctx->msg;
        } else
        if(EVT_FUNC(msg->evt) == SYS_ackblock) {
            if(msg->data.scalar.arg0>=ndev || fcb[dev[msg->data.scalar.arg0].fid].data.device.blksize<=1)
                continue;
            if(cache_cleardirty(msg->data.scalar.arg0, msg->data.scalar.arg1) && nomem) {
                cache_free();
                nomem=false;
            }
            continue;
        } else {
            ctx = taskctx_get(EVT_SENDER(msg->evt));
            ctx->workleft = -1;
        }

        switch(EVT_FUNC(msg->evt)) {
            case SYS_mountfs:
#if DEBUG
                if(_debug&DBG_FS)
                    dbg_printf("FS: mountfs()\n");
#endif
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
                continue;

            default:
#if DEBUG
                dbg_printf("FS: unknown event: %x from pid %x\n",EVT_FUNC(msg->evt), EVT_SENDER(msg->evt));
#endif
                seterr(EPERM);
                ret=-1;
                break;
        }

        m = meminfo();
        j=m.free*100/m.total;
        if(j<cachelimit || errno()==ENOMEM) {
            fcb_free();
            cache_flush();
            nomem=true;
        }
        if(!ackdelayed) {
            j = errno();
#if 0
            mq_send(EVT_SENDER(msg->evt), (j?SYS_nack:SYS_ack) | MSG_RESPONSE, j?j/*errno*/:ret/*return value*/,
                EVT_FUNC(msg->evt), msg->serial, false);
#endif
            ctx->msg.evt = 0;
        } else {
            memcpy(&ctx->msg, msg, sizeof(msg_t));
        }
#endif
    }
    return EX_SOFTWARE;
}

