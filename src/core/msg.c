/*
 * core/msg.c
 *
 * Copyright (c) 2019 bzt (bztsrc@gitlab)
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
 * @subsystem taszk
 * @brief üzenetküldés, a core-nak küldött üzenetek feldolgozása az msgcore.c-ben van
 */

#include <platform.h>               /* PG_* lapozási jelzőbitek miatt */

bool_t in_trace = false;

/**
 * üzenetek fogadása
 */
msg_t *msg_recv(uint64_t serial)
{
    tcb_t *tcb = (tcb_t*)0, *sndtcb = (tcb_t*)LDYN_tmpmap3;
    msghdr_t *msghdr = (msghdr_t*)__PAGESIZE;
    msg_t *msg = NULL;
    phy_t *ptr = (phy_t*)LDYN_tmpmap2;
    uint i, j;
    char *s;

    /* az idle taszknak nincs üzenetsora */
    if(tcb->pid == idle_tcb.pid) return NULL;

    /* ha nyomkövetést kértünk */
    if(tcb->tracebuf && tcb->pid != services[-SRV_FS]) {
        memset(tcb->tracebuf, 0, TRACEBUFSIZ);
        s = sprintf(tcb->tracebuf, "[%d] 000000.0000(%x)\n", clock_ticks, serial);
        msg_send((virt_t)tcb->tracebuf, s-tcb->tracebuf+1, 3, 0, 0, 0, EVT_DEST(SRV_FS) | SYS_fwrite | MSG_PTRDATA, 0);
    }
#if DEBUG
    if(debug&DBG_MSG)
        kprintf(" msg %3x -> 000000, #0000(%x) %d\n", tcb->pid, serial, msghdr->mq_total);
#endif

    if(msghdr->mq_start != msghdr->mq_end) {
        /* nem minden architektúrán tudja a felügyeleti szint írni a csak olvasható felhasználói lapokat, ezért leképezzük */
        vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(tcb->taskmemroot), PG_CORE_RWNOCACHE|PG_PAGE);
        vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(ptr[0]), PG_CORE_RWNOCACHE|PG_PAGE);
        vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(ptr[0]), PG_CORE_RWNOCACHE|PG_PAGE);
        vmm_page(0, LDYN_tmpmap3, vmm_phyaddr(ptr[1]), PG_CORE_RWNOCACHE|PG_PAGE);
        msghdr = (msghdr_t*)LDYN_tmpmap3;
        if(!serial) {
            /* ha bárkitől fogadunk, akkor kivesszük a soronkövetkező üzenetet a listából FIFO módon */
            do {
                msg = (msg_t*)(__PAGESIZE + msghdr->mq_start * sizeof(msg_t));
                msghdr->mq_start++;
                if(msghdr->mq_start >= MQ_MAX) msghdr->mq_start = 1;
            } while(MSG_ISRESP(msg->evt) && msghdr->mq_start != msghdr->mq_end);
            if(MSG_ISRESP(msg->evt)) msg = NULL;
            else {
                msghdr->mq_total++;
                if(MSG_ISPTR(msg->evt)) msghdr->mq_bufstart =
                    ((((virt_t)msg->data.buffer.ptr + msg->data.buffer.size + __PAGESIZE-1) >> __PAGEBITS) << __PAGEBITS);
            }
        } else {
            /* ha egy adott taszktól várunk válaszüzenetet, akkor a lista végét nézzük, FILO módon */
            i = msghdr->mq_end;
            j = i-1; if(!j) j = MQ_MAX-1;
            do {
                i--; if(!i) i = MQ_MAX-1;
                msg = (msg_t*)(__PAGESIZE + i * sizeof(msg_t));
            } while((MSG_ISRESP(msg->evt) || msg->serial != serial) && i != msghdr->mq_start);
            if(MSG_ISRESP(msg->evt) || msg->serial != serial) msg = NULL;
            else {
                msghdr->mq_total++;
                if(i == j) {
                    msghdr->mq_end = j;
                    if(MSG_ISPTR(msg->evt)) msghdr->mq_bufend = (virt_t)msg->data.buffer.ptr & ~(__PAGESIZE-1UL);
                } else {
                    vmm_page(0, LDYN_tmpmap4, vmm_phyaddr(ptr[((virt_t)msg>>__PAGEBITS)&511]), PG_CORE_RWNOCACHE|PG_PAGE);
                    ((msg_t*)(LDYN_tmpmap4 + (((virt_t)msg)&(__PAGESIZE-1))))->evt |= MSG_RESPONSED;
                }
            }
        }
    }
    /* ha válaszüzenet, akkor állítjuk a hibakódot is hozzá */
    if(msg && EVT_FUNC(msg->evt) == EVT_ack) seterr(msg->data.scalar.arg2);

    /* ha van, aki azért blokkolódott, mert nem tudott küldeni nekünk, azt felébresztjük most */
    if(tcb->blksend) {
        vmm_page(0, LDYN_tmpmap3, tcb->blksend << __PAGEBITS, PG_CORE_RWNOCACHE|PG_PAGE);
        if(sndtcb->magic == OSZ_TCB_MAGICH && tcb->pid != sndtcb->pid) {
            tcb->blksend = sndtcb->blksend;
            sndtcb->blksend = 0;
            sched_awake(sndtcb);
        } else
            tcb->blksend = 0;
    }

    return msg;
}

/**
 * üzenetek küldése
 */
uint64_t msg_send(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, evt_t event, uint64_t serial)
{
    msghdr_t *msghdr = (msghdr_t*)LDYN_tmpmap6;
    msg_t *msg;
    tcb_t *srctcb = (tcb_t*)0, *dsttcb = (tcb_t*)LDYN_tmpmap5;
    phy_t *ptr = (phy_t*)LDYN_tmpmap2, *ptr2 = (phy_t*)LDYN_tmpmap3, *ptr3 = (phy_t*)LDYN_tmpmap4, p;
    int64_t task = ((int64_t)(event&~0xFFFF)/(int64_t)65536); /* az EVT_SENDER() elhagyná az előjelet */
    pid_t dst;
    evt_t evt = EVT_FUNC(event);
    uint64_t i, j, k;
    char *s;

    if(!task || !msg_allowed(srctcb, task, evt)) return 0;

    /* negatív taszkazonosítók átfordítása rendszer szolgáltatás pidekké */
    if(task < 0) {
        if(-task < NUMSRV && services[-task]) dst = services[-task];
        else { seterr(ESRCH); return 0; }
    } else {
        dst = (pid_t)task;
    }
    srand[dst&3] ^= clock_ts;

    /* céltaszk üzenetsorának leképezése */
    vmm_page(0, LDYN_tmpmap5, dst << __PAGEBITS, PG_CORE_RWNOCACHE|PG_PAGE);
    if(dsttcb->magic != OSZ_TCB_MAGICH) { seterr(ESRCH); return 0; }
    /* plusz ellenőrzés, eszközmeghajtóknak csak az FS küldhet üzenetet vagy a core DRV_IRQ-t az msg_notify()-al */
    if(dsttcb->priority == PRI_DRV && (srctcb->pid != services[-SRV_FS] || !evt))
        { seterr(EPERM); return 0; }

    /* ha nyomkövetést kértünk */
    if(!in_trace && srctcb->tracebuf && srctcb->pid != services[-SRV_FS]) {
        memset(srctcb->tracebuf, 0, TRACEBUFSIZ);
        s = sprintf(srctcb->tracebuf, "[%d] %3x.%2x(", clock_ticks, dst, evt);
        s = sprintf(s, event & MSG_PTRDATA? "*%x[%d]" : "%x,%x", a, b);
        s = sprintf(s, ",%x,%x,%x,%x)\n", c, d, e, f);
        in_trace = true;
        msg_send((virt_t)srctcb->tracebuf, s-srctcb->tracebuf+1, 3, 0, 0, 0, EVT_DEST(SRV_FS) | SYS_fwrite | MSG_PTRDATA, 0);
        in_trace = false;
        /* vissza kell állítanunk azt eredeti tcb leképezést */
        vmm_page(0, LDYN_tmpmap5, dst << __PAGEBITS, PG_CORE_RWNOCACHE|PG_PAGE);
    }
#if DEBUG
    if(debug&DBG_MSG) {
        kprintf(" msg %3x -> %3x, #%2x(", srctcb->pid, dst, evt);
        kprintf(event & MSG_PTRDATA? "*%x[%d]" : "%x,%x", a, b);
        kprintf(",%x,%x,%x,%x)\n",c,d,e,f);
    }
#endif

    seterr(SUCCESS);

    vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(dsttcb->taskmemroot), PG_CORE_RWNOCACHE|PG_PAGE);     /* 512G */
    vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(ptr[0]), PG_CORE_RWNOCACHE|PG_PAGE);                  /* 1G */
    vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(ptr[0]), PG_CORE_RWNOCACHE|PG_PAGE);                  /* 2M, 4K-s címek */
    vmm_page(0, LDYN_tmpmap6, vmm_phyaddr(ptr[1]), PG_CORE_RWNOCACHE|PG_PAGE);                  /* msghdr */

    /* ne a fejlécre mutassanak az indexek */
    if(!msghdr->mq_start)   msghdr->mq_start++;
    if(!msghdr->mq_end)     msghdr->mq_end++;

    /* ha betelt az üzenetsor, akkor blokkoljuk a küldőt */
    j = sizeof(msg_t) * msghdr->mq_end++;
    if(msghdr->mq_end >= MQ_MAX) msghdr->mq_end = 1;
    if(msghdr->mq_end == msghdr->mq_start) {
        if(!in_trace && srctcb->pid != dsttcb->pid) {
            srctcb->blksend = dsttcb->blksend;
            dsttcb->blksend = srctcb->pid;
            sched_block(srctcb);
            sched_pick();
        } else seterr(EBUSY);
        return 0;
    }
    /* ha még nincs lap lefoglalva, és nem üres a sor, akkor most foglalunk egyet és leképezzük */
    i = ((j>>__PAGEBITS)+1)&511;
    if(!vmm_ispresent(ptr[i])) {
        if(msghdr->mq_start+1 == msghdr->mq_end) {
            j = sizeof(msghdr_t); i = 1;
            msghdr->mq_end = 1;
        } else {
            ptr[i] = (phy_t)pmm_alloc(dsttcb, 1) |PG_USER_RO|PG_PAGE|(1UL<<PG_NX_BIT);
            if(srctcb->pid == dsttcb->pid)
                vmm_invalidate(__PAGESIZE+j);
        }
    }
    vmm_page(0, LDYN_tmpmap7, vmm_phyaddr(ptr[i]), PG_CORE_RWNOCACHE|PG_PAGE);
    msg = (msg_t*)(LDYN_tmpmap7 + (j&(__PAGESIZE-1)));

    /* ha buffer is tartozik az üzenethez, akkor azt is le kell képeznünk, kivéve, ha megosztott memóriacím */
    if(event & MSG_PTRDATA){
        if(a < TEXT_ADDRESS || (a >= BUF_ADDRESS && a < SDYN_ADDRESS) || a >= LDYN_ADDRESS) { seterr(EFAULT); return 0; }
        if(a < BUF_ADDRESS) {
            if(msghdr->mq_bufstart < MQ_BUF) msghdr->mq_bufstart = MQ_BUF;
            if(msghdr->mq_bufend < MQ_BUF) msghdr->mq_bufend = MQ_BUF;
            /* ha nem fér bele az üzenetsor bufferébe, akkor megosztott memóriában kell átadni */
            if(b > __BUFFSIZE || (msghdr->mq_bufend + b > __SLOTSIZE && MQ_BUF + b > msghdr->mq_bufstart)) {
                seterr(ENOTSHM); return 0;
            }
            /* küldő taszk bufferéhez tartozó lapcímtáblák leképezése */
            p = a + __SLOTSIZE-1;
            vmm_page(0, LDYN_tmpmap3, vmm_phyaddr(srctcb->taskmemroot), PG_CORE_RWNOCACHE|PG_PAGE); /* 512G */
            vmm_page(0, LDYN_tmpmap4, vmm_phyaddr(ptr2[(p>>30)&511]), PG_CORE_RWNOCACHE|PG_PAGE);   /* 1G vége */
            vmm_page(0, LDYN_tmpmap3, vmm_phyaddr(ptr2[(a>>30)&511]), PG_CORE_RWNOCACHE|PG_PAGE);   /* 1G eleje */
            vmm_page(0, LDYN_tmpmap4, vmm_phyaddr(ptr3[(p>>21)&511]), PG_CORE_RWNOCACHE|PG_PAGE);   /* 2M vége */
            vmm_page(0, LDYN_tmpmap3, vmm_phyaddr(ptr2[(a>>21)&511]), PG_CORE_RWNOCACHE|PG_PAGE);   /* 2M eleje */
            /* tmpmap2: az ideiglenesen leképzett mq lapcímtára (i)
             * tmpmap3: forrás buffer lapcímtára (j)
             * tmpmap4: forrás buffer következő lapcímtára, ha esetleg j túllépné az előzőt */
            i = (msghdr->mq_bufend + b < __SLOTSIZE? msghdr->mq_bufend : MQ_BUF) >> __PAGEBITS;
            if(i < 2) i = 2; /* paranoia, nem szabad véletlenül sem felülírni a TCB-t és az msghdr-t */
            j = (a >> __PAGEBITS) & 511;
            k = ((a+b+__PAGESIZE-1) >> __PAGEBITS) & 511;
            a = (i << __PAGEBITS) + (a & (__PAGESIZE-1));
            msghdr->mq_bufend = ((a+b+__PAGESIZE-1) >> __PAGEBITS) << __PAGEBITS;
            dsttcb->linkmem += k-j;
            for(; j < k; j++, i++) {
                if(ptr[i]) dsttcb->linkmem--;
                ptr[i] = vmm_phyaddr(ptr2[j]) | PG_USER_RONOCACHE | PG_PAGE | PG_SHARED;
            }
        }
    }
    if(msghdr->mq_serial >= USHRT_MAX) msghdr->mq_serial = 1; else msghdr->mq_serial++;
    k = EVT_DEST(srctcb->pid) | msghdr->mq_serial;

    /* üzenet bemásolása. EVT_FUNC()-al elvesztenénk a MSG_PTRDATA jelzést  */
    msg->evt = EVT_DEST(srctcb->pid) | (event&0x7FFF);
    msg->data.scalar.arg0 = a;
    msg->data.scalar.arg1 = b;
    msg->data.scalar.arg2 = c;
    msg->data.scalar.arg3 = d;
    msg->data.scalar.arg4 = e;
    msg->data.scalar.arg5 = f;
    msg->serial = serial ? serial : k;

    /* címzett felébresztése */
    sched_awake(dsttcb);
    return k;
}

/**
 * taszk értesítése, core nevében küld üzenetet. Ezt csakis a core használhatja
 */
void msg_notify(pid_t pid, evt_t event, uint64_t a)
{
    msghdr_t *msghdr = (msghdr_t*)LDYN_tmpmap3;
    phy_t *ptr = (phy_t*)LDYN_tmpmap2;
    msg_t *msg;
    uint i, j;

    if(!pid) return;
    vmm_page(0, LDYN_tmpmap5, pid << __PAGEBITS, PG_CORE_RWNOCACHE|PG_PAGE);
    if(((tcb_t*)LDYN_tmpmap5)->magic == OSZ_TCB_MAGICH) {
        /* csak az üzenetsort képezzük le ideiglenesen, és mindenképp kézbesítünk, nem blokkoljuk a küldőt soha */
        sched_awake((tcb_t*)LDYN_tmpmap5);
        vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(((tcb_t*)LDYN_tmpmap5)->taskmemroot), PG_CORE_RWNOCACHE|PG_PAGE);
        vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(ptr[0]), PG_CORE_RWNOCACHE|PG_PAGE);
        vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(ptr[0]), PG_CORE_RWNOCACHE|PG_PAGE);
        vmm_page(0, LDYN_tmpmap3, vmm_phyaddr(ptr[1]), PG_CORE_RWNOCACHE|PG_PAGE);
        if(!msghdr->mq_start)   msghdr->mq_start++;
        if(!msghdr->mq_end)     msghdr->mq_end++;
        if(msghdr->mq_serial >= USHRT_MAX) msghdr->mq_serial = 1; else msghdr->mq_serial++;
        j = sizeof(msg_t) * msghdr->mq_end++;
        if(msghdr->mq_end >= MQ_MAX) msghdr->mq_end = 1;
        i = ((j>>__PAGEBITS)+1)&511;
        if(!vmm_ispresent(ptr[i])) {
            if(msghdr->mq_start+1 == msghdr->mq_end) {
                j = sizeof(msghdr_t); i = 1;
                msghdr->mq_end = 1;
            } else {
                ptr[i] = (phy_t)pmm_alloc(((tcb_t*)LDYN_tmpmap5), 1) |PG_USER_RO|PG_PAGE|(1UL<<PG_NX_BIT);
                vmm_invalidate(__PAGESIZE+j); /* ha az aktuális taszknak küldünk (esélyes), akkor nem lesz címtér frissítés */
            }
        }
        vmm_page(0, LDYN_tmpmap4, vmm_phyaddr(ptr[i]), PG_CORE_RWNOCACHE|PG_PAGE);
        msg = (msg_t*)(LDYN_tmpmap4 + (j&(__PAGESIZE-1)));
        msg->evt = EVT_FUNC(event);
        msg->data.scalar.arg0 = a;
        msg->data.scalar.arg1 = clock_ticks;
        msg->serial = msghdr->mq_serial;
    }
}
