/*
 * core/msgcore.c
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
 * @brief a core-nak küldött üzenetek feldolgozása, az általános üzenetküldés az msg.c-ben van
 */

#include <arch.h>                                   /* cpu_relax miatt */

/* külső erőforrások */
extern uint64_t env_getts(char *p, int16_t timezone);
extern int16_t clock_tz;                            /* rendszeróra időzóna percekben */
extern uint16_t irq_max;                            /* IRQ vonalak száma */

/* a nyomkövetés bufferét a taszk memóriájában tároljuk */
#define realloc(p,s) bzt_alloc((void*)DYN_ADDRESS,8,p,s,MAP_PRIVATE)
#define free(p) bzt_free((void*)DYN_ADDRESS,p)

typedef struct {
    uint64_t ret0;
    uint64_t ret1;
} msgret_t;

/**
 * az SRV_CORE taszknak küldött üzenetek feldolgozása
 */
msgret_t msg_core(uint64_t a, uint64_t b, uint64_t c, uint64_t d,
    uint64_t e __attribute__((unused)),  uint64_t f __attribute__((unused)), evt_t event)
{
    tcb_t *tcb = (tcb_t*)0;
    msgret_t ret = {0,0};
    uint i, evt = EVT_FUNC(event);
    char *s, *n;
    uint8_t t;

    if(!msg_allowed(tcb, SRV_CORE, evt)) return ret;

    /* ha nyomkövetést kértünk */
    if(tcb->tracebuf && tcb->pid != services[-SRV_FS]) {
        memset(tcb->tracebuf, 0, TRACEBUFSIZ);
        s = sprintf(tcb->tracebuf, "[%d] %3x.%2x(", clock_ticks, 0, evt);
        s = sprintf(s, event & MSG_PTRDATA? "*%x[%d]" : "%x,%x", a, b);
        s = sprintf(s, ",%x,%x,%x,%x)\n", c, d, e, f);
        msg_send((virt_t)tcb->tracebuf, s-tcb->tracebuf+1, 3, 0, 0, 0, EVT_DEST(SRV_FS) | SYS_fwrite | MSG_PTRDATA, 0);
    }
#if DEBUG
    if(debug&DBG_MSG) {
        kprintf(" msg %3x -> %3x, #%2x(", tcb->pid, 0, evt);
        kprintf(event & MSG_PTRDATA? "*%x[%d]" : "%x,%x", a, b);
        kprintf(",%x,%x,%x,%x)\n",c,d,e,f);
    }
#endif

    seterr(SUCCESS);
    switch(evt) {
/*      case SYS_yield:  assemblyben van lekezelve  */

        case SYS_exit:
            /* ha kritikus rendszer szolgáltatás lépett ki */
            if(tcb->pid == services[-SRV_FS]) kpanic(a==1? TXT_mounterr : TXT_structallocerr);
            if(tcb->pid == services[-SRV_UI]) kpanic(TXT_uiexit);
            /* ha az init rendszer szolgáltatás fejeződött be, akkor újraindítjuk vagy kikapcsoljuk a gépet */
            if(tcb->pid == services[-SRV_init]) {
                if(a == EAGAIN)
                    kprintf_reboot();
                else
                    kprintf_poweroff();
            }
            /* rendszer szolgáltatások értesítése, hogy befejeződött a taszk */
            for(i = -SRV_FS; i < NUMSRV; i++) {
                if(services[i] == tcb->pid)
                    services[i] = 0;
                else
                    msg_notify(services[i], EVT_exit, tcb->pid);
            }
            /* core listáiból kivesszük */
            sched_remove(tcb);
            drivers_unregintr(tcb->pid);
            /* megszüntetjük a taszkot. Ez át is kapcsol egy új címtérre, mivel annélkül nem tudná törölni */
            task_del(tcb);
            break;

        case SYS_fork:
            break;

        case SYS_execve:
            break;

        case SYS_usleep:
            if(a && a < 365*24*60*60*1000000UL) {
                if(runlevel == RUNLVL_NORM) sched_delay(tcb, a);
                else {
                    /* ha még nincs időzítő megszakításunk, akkor sem térhetünk vissza egyből */
                    for(a *= 1000; a; --a) cpu_relax;
                }
            } else
                seterr(EINVAL);
            break;

        case SYS_regirq:
            if(a < irq_max && a != sched_irq && a != clock_irq)
                drivers_regintr(a, tcb->pid);
            else
                seterr(EINVAL);
            break;

        case SYS_regtmr:
            drivers_regintr(clock_irq, tcb->pid);
            break;

        case SYS_drvfind:
            if(a && b && c > 1) {
                n = (char*)b;
                *n = 0; c--;
                s = drivers_find((char*)a);
                if(s) { while(c-- && *s && *s!='\n') { *n++ = *s++; } *n = 0; }
            }
            break;

        case SYS_drvadd:
            if(a)
                drivers_add((char*)a, (pmm_entry_t*)b);
            break;

        case SYS_stimezone:
            if((int16_t)a >= -1440 && (int16_t)a <= 1440) {
                clock_ts -= clock_tz*60000000;
                clock_tz = a;
                clock_ts += clock_tz*60000000;
            } else
                seterr(EINVAL);
            break;

        case SYS_stimebcd:
            clock_ts = env_getts((char *)a, clock_tz);
            clock_ns = 0;
            break;

        case SYS_stime:
            clock_ts = a/1000000;
            clock_ns = a%1000000;
            break;

        case SYS_time:
            ret.ret0 = (clock_ts*1000000) + (clock_ns/1000000);
            ret.ret1 = clock_ticks;
            break;

        case SYS_srand:
            srand[(a>>2)&3] += a;
            srand[(a>>4)&3] ^= a;
            srand[(a>>6)&3] += a;
            srand[(a>>8)&3] ^= a;
            srand[(a>>1)&3] *= 16807;
            srand[(a>>3)&3] ^= clock_ts + clock_ns;
            kentropy();
            break;

        case SYS_rand:
            ret.ret0 = srand[clock_ts&3];
            ret.ret1 = srand[(clock_ts-1)&3];
            t = *((uint8_t*)&srand); memcpy(srand, (uint8_t*)&srand + 1, sizeof(srand) - 1); srand[3] |= (uint64_t)t << 56;
            kentropy();
            break;

        case SYS_getentropy:
            t = *((uint8_t*)&srand); memcpy(srand, (uint8_t*)&srand + 1, sizeof(srand) - 1); srand[3] |= (uint64_t)t << 56;
            kentropy();
            while(b > 0) {
                c = b > sizeof(srand) ? sizeof(srand) : b;
                memcpy((void*)a, srand, c);
                srand[(clock_ts+2)&3] ^= clock_ns;
                srand[clock_ts%4] *= 16807;
                kentropy();
                a += c;
                b -= c;
            }
            break;

        case SYS_meminfo:
            ret.ret0 = pmm_freepages;
            ret.ret1 = pmm_totalpages;
            break;

        case SYS_memmap:
            ret.ret0 = (virt_t)vmm_map(tcb, a, 0, b, PG_USER_RW);
            break;

        case SYS_memunmap:
            vmm_free(tcb, a, b);
            break;

        case SYS_trace:
            if(a) tcb->tracebuf = (char*)realloc(tcb->tracebuf, TRACEBUFSIZ);
            else { free(tcb->tracebuf); tcb->tracebuf = NULL; }
            break;

        case SYS_getpid:
            ret.ret0 = tcb->pid;
            break;

        case SYS_getppid:
            ret.ret0 = tcb->parent;
            break;

        case SYS_getuid:
            if(!a) memcpy((void*)&ret, (void*)&tcb->owner, sizeof(uid_t));
            else {
                vmm_page(0, LDYN_tmpmap1, a << __PAGEBITS, PG_CORE_RONOCACHE|PG_PAGE);
                if(((tcb_t*)LDYN_tmpmap1)->magic != OSZ_TCB_MAGICH) seterr(ESRCH);
                else memcpy((void*)&ret, (void*)&((tcb_t*)LDYN_tmpmap1)->owner, sizeof(uid_t));
            }
            break;

        case SYS_getgid:
            if(!a) seterr(ENOMEM);
            else if(!b) {
                memcpy((void*)a, (void*)&tcb->acl, TCB_MAXACE * sizeof(gid_t));
                ret.ret0 = a;
            } else {
                vmm_page(0, LDYN_tmpmap1, b << __PAGEBITS, PG_CORE_RONOCACHE|PG_PAGE);
                if(((tcb_t*)LDYN_tmpmap1)->magic != OSZ_TCB_MAGICH) seterr(ESRCH);
                else {
                    memcpy((void*)a, (void*)&((tcb_t*)LDYN_tmpmap1)->acl, TCB_MAXACE * sizeof(gid_t));
                    ret.ret0 = a;
                }
            }
            break;

        case SYS_tskcpy:
            break;

        case EVT_ack:
            if(tcb->priority == PRI_DRV && a < irq_max) intr_enable(a);
            break;
#if DEBUG
        case SYS_dbgprintf: kprintf((char*)a,b,c,d,e,f); break;
#endif
        default:
            seterr(EPERM);
            kpanic("unknown core event %x(%x,%x,%x,%x,%x,%x)\n",event,a,b,c,d,e,f);
    }
    return ret;
}
