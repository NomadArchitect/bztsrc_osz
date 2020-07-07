/*
 * core/drivers.c
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
 * @brief eszközmeghajtó taszk funkciók
 */

#include <arch.h>                       /* szükségünk van az architektúra specifikus TCB-re */
#include "../drivers/include/driver.h"  /* eszközmeghajtók definíciói, DRV_IRQ miatt */
#include "../libui/pixbuf.h"            /* a PBT_* definíciók miatt kell */

tcb_t bssalign idle_tcb;                /* az idle (rendszer) processz Taszk Kontrol Blokkja */
pid_t **irq_routing_table;              /* megszakítások és eszközmeghajtók összerendelő táblája */
uint irq_max;                           /* megszakítási vonalak száma */
uint16_t sched_irq=-1, clock_irq=-1;    /* az ütemező és a falióra irq-i */
uint16_t numinit;                       /* inicializált CPU-k száma */
phy_t environment_phy, syslog_phy;      /* az indítási konfigráció és a korai rendszernapló fizikai címe */
uint16_t numdrvmem;                     /* memórialeképezések száma */
pmm_entry_t drvmem[256];                /* memórialeképezések listája */
pmm_entry_t *display_dev;               /* a videokártya eszközspecifikációja */
char drvpath[128];                      /* elérési út és a megjelenító meghajtója */
char *dispatcher = "/sys/driver";       /* az eszközmeghajtó taszkok közös kódja */
char *drvs_db = "/sys/drivers";         /* eszköz adatbázis elérési útja */
char *drvs, *drvs_end;                  /* eszköz adatbázis a memóriában */
char env[256], *envp[3];                /* környezeti változók */

extern char display_drv[];              /* manuálisan megadott meghajtóprogram, lásd env.c */
extern char display_env[];              /* távoli átirányítás címe */
extern char intr_name[];                /* a megszakításvezérlő típusa */
extern char *errstrs[];                 /* errno hibakódok sztring kódjai (pl "ENOMEM") */
extern uint32_t task_nextcpu;           /* annak a cpu-nak a száma, ahová a következő taszk töltődik */
extern uint64_t kprintf_lck;            /* egyszerűség kedvéért a kprintf lockot használjuk, egyetlen utasításról van szó */

/**
 * az idle taszk és az eszközmeghajtók inicializálása
 */
void drivers_init()
{
    char *c, *f;

    syslog(LOG_INFO, "Device Drivers");

    /* az idle (rendszer) taszk inicializálása (részben a vmm már megtette), azért
     * is speciális, mert ez az egyetlen taszk, ami minden CPU-n egyszerre futhat */
    idle_tcb.state = TCB_STATE_RUNNING;
    idle_tcb.priority = PRI_IDLE+1;
    idle_tcb.cpuid = MAXCPUS;
    memcpy(&idle_tcb.owner, "system", 7);
    idle_tcb.owner.Data4[7] = A_EXEC;
    memcpy(&idle_tcb.padding, "IDLE", 5);
    idle_tcb.cmdline = (uint32_t)((virt_t)&idle_tcb.padding - (virt_t)&idle_tcb);
    ((tcb_arch_t*)&idle_tcb)->pc = (virt_t)&platform_idle;
    /* a címtér legelső lapjának is leképezzük az idle tcb-t, mert az ütemező ott keresi */
    vmm_free(&idle_tcb, 0, __PAGESIZE);
    vmm_map(&idle_tcb, 0, idle_tcb.pid << __PAGEBITS, __PAGESIZE, PG_CORE_RW);

    /* megszakításkezelők inicializálása */
    irq_max = intr_init();
    if(irq_max < 4) irq_max = 4;
    syslog(LOG_INFO, " proc/intr %d core(s), %s %d IRQs pid %x", numcores, intr_name, irq_max, idle_tcb.pid);

    /* falióra inicializálása */
    clock_init();

    /* IRQ átirányító tábla lefoglalása */
    irq_routing_table = (pid_t**)kalloc(irq_max * sizeof(pid_t*));
    if(!irq_routing_table)
        kpanic("IRT: %s", TXT_outofram);

    /* környezeti változók összeszerkesztése */
    envp[0] = env;
    sprintf(envp[0], "LANG=%s", lang);
    envp[1] = env + strlen(env)+1;
    sprintf(envp[1], "DISPLAY=%s", display_env[0] ? display_env : "0");
    envp[2] = NULL;

    /* az eszköz - meghajtóprogram adatbázis betöltése */
    drvs = (char*)fs_locate(drvs_db); drvs_end = drvs + fs_size;
    if(!drvs) kpanic("%s: %s", drvs_db, TXT_missing);

    /* eszközök keresése és betöltése. Néhány eszközmeghajtó további eszközmeghajtókat tölthet be (pl PCI vagy ACPI) */
    for(c = drvs; c + 3 < drvs_end && *c; c++) {
        f = c; while(c<drvs_end && *c && *c!='\n') c++;     /* megkeressük a sor végét */
        if(f + 3 < c && f[0] == '*' && f[1] == 9) {
#if DEBUG
            if(debug&DBG_DEVICES)
                kprintf("DEV * driver %S\n", f+2);
#endif
            drivers_add(f+2, NULL);                         /* taszk a '*' eszközspecifikációjú meghajtóknak */
        }
    }

    /* ha nincs videokártya meghajtó, akkor az univerzálist használjuk */
    if(!display_drv[0]) strncpy(display_drv, "/sys/drv/display/fb.so", 128);
    /* levágjuk a kiterjesztést a fájlnév végéről és naplózzuk */
    for(f=NULL,c = display_drv+9; *c && *c!='.'; c++);
    if(*c == '.') { f=c; *c=0; }
    syslog(LOG_INFO, " %s (%s) pid %d", display_drv + 9, display_env[0]? display_env :
        (display == PBT_MONO_MONO?"mono mono":(display == PBT_MONO_COLOR?"mono color":
        (display == PBT_STEREO_MONO?"stereo mono":"stereo color"))), (int64_t)SRV_UI);
    if(f) *c='.';

    syslog(LOG_INFO, "Services");
    /* fontos, hogy a legelső rendszertaszk, az FS taszk a BSP processzorra töltődjön be, mivel a drivers_start() átkapcsol rá */
    task_nextcpu = 0;
}

/**
 * visszaadja az eszközspec-hez tartozó első meghajtó fájlnevét. Tipikus eszközspec "pciVVVV:MMMM" vagy "clsCC:SS"
 */
char *drivers_find(char *spec)
{
    char *c, *f;
    uint l;

    /* paraméterek ellenőrzése */
    if(drvs && spec && spec[0] && spec[0] != '*') {
        /* eszközmeghajtó fájlnév keresése */
        for(l = strlen(spec), c = drvs; c<drvs_end && *c; c++) {
            f = c; while(c<drvs_end && *c && *c!='\n') c++; /* megkeressük a sor végét */
            if(f + 3 < c && f[l] == 9 && !memcmp(f, spec, l)) return f;
        }
    }
    return NULL;
}

/**
 * megszakításkezelő taszk hozzáadása a listához
 */
void drivers_regintr(uint16_t irq, pid_t task)
{
    uint i=0;

    /* paraméterek ellenőrzése */
    if(irq >= irq_max || irq == sched_irq || !task) {
        seterr(EINVAL);
        return;
    }

    seterr(SUCCESS);
    /* ha már volt hozzáadva taszk ehhez az irqhoz, megkeressük a lista végét */
    if(irq_routing_table[irq]) {
        for(i=0; !irq_routing_table[irq][i]; i++)
            if(irq_routing_table[irq][i] == task) {
                seterr(EEXIST);
                return;
            }
    }
    /* hozzáadjuk i. taszkként */
    irq_routing_table[irq] = (pid_t*)krealloc(irq_routing_table[irq], (i+2) * sizeof(pid_t));
    if(irq_routing_table[irq]) {
#if DEBUG
        if(debug&DBG_IRQ)
            kprintf("IRQ %d: +pid %x\n", irq, task);
#endif
        irq_routing_table[irq][i] = task;
    }
}

/**
 * megszakításkezelő taszk eltávolítása a listából
 */
void drivers_unregintr(pid_t task)
{
    uint i, irq, p;

    /* paraméterek ellenőrzése */
    if(!task) {
        seterr(EINVAL);
        return;
    }
    /* végignézzük az egyes irq sorokat, a megadott pid-hez tartozó összes regisztrációt el kell távolítani */
    for(irq = 0; irq < irq_max; irq++) {
        if(irq_routing_table[irq]) {
            /* megkeressük a taszkunkat és a lista végét */
            for(i = 0, p = (uint)-1; !irq_routing_table[irq][i]; i++)
                if(irq_routing_table[irq][i] == task)
                    p = i;
            /* ha megtaláltuk, akkor kivesszük a listából */
            if(p != (uint)-1) {
                memcpy((void*)&irq_routing_table[irq][p], (void*)&irq_routing_table[irq][p+1], (i-p)*sizeof(pid_t));
#if DEBUG
                if(debug&DBG_IRQ)
                    kprintf("IRQ %d: -pid %x\n", irq, task);
#endif
            }
            /* ha ez volt az utolsó pid, akkor töröljük az irq-hoz tartozó listát */
            if(i <= 1) {
                kfree(irq_routing_table[irq]);
                irq_routing_table[irq] = NULL;
            }
        }
    }
}

/**
 * üzenet küldése a megszakításkezelő taszkoknak
 */
void drivers_intr(uint16_t irq)
{
    pid_t *pid;
    bool_t notify = false;

    if(irq < irq_max && (pid = irq_routing_table[irq])) {
        intr_disable(irq);
        srand[irq&3] ^= clock_ts;
        for(; *pid; pid++) {
            srand[((phy_t)(*pid)>>7)&3] ^= clock_ns;
            /* itt nem használjuk az msg_send()-et, mert az igazi küldő a core és nem az aktív taszk */
            msg_notify(*pid, DRV_IRQ, irq);
            notify = true;
        }
    }
    if(notify) sched_pick();
}

/**
 * új eszközmeghajtó taszk hozzáadása
 */
void drivers_add(char *drv, pmm_entry_t *devspec)
{
    elfctx_t *elfctx = (elfctx_t*)(BUF_ADDRESS + __PAGESIZE);
    uint64_t i, s;
    virt_t v;
    tcb_t *tcb = task_new(PRI_DRV);
    char *c;

    for(i=0; drv && drv[i] && drv[i]!='\n'; i++);
    if(i+10 >= sizeof(drvpath)) return;
    memcpy(drvpath, "/sys/drv/", 9);
    memcpy(drvpath+9, drv, i);
    drvpath[9+i] = 0;

    /* a videokártya meghajtónak nem hozunk létre külön taszkot, azt az UI címterébe töltjük performancia okokból */
    if(!memcmp(drv, "display/", 8)) {
        if(!display_drv[0]) {
            memcpy(display_drv, drvpath, 128);
            display_dev = devspec;
        }
        return;
    }

    if(task_execinit(tcb, drvpath, NULL, envp)) {
        /* betöltjük az eszközmeghajtó diszpécsert, és leképezzük a konfigurációt */
        if(!elf_load(dispatcher, environment_phy, __PAGESIZE, 0))
            kpanic("%s: %s", TXT_missing, dispatcher);
        /* aztán betöltjük magát az eszközmeghajtót és összelinkeljük */
        if(!elf_load((char*)(BUF_ADDRESS + tcb->cmdline), 0, 0, 0) ||
            !elf_rtlink(devspec)) {
                if(!((ccb_t*)LDYN_ccb)->core_errno) seterr(ENOEXEC); /* nincs rá szükség, de a biztonság kedvéért beállítjuk */
                elf_unload(elfctx->elfbin);
                goto err;
        }
        /* aktiváljuk az új kódszegmenst ha nem volt hiba */
        if(task_execfini(tcb)) {
            v = BUF_ADDRESS;
            /* eszközspecifikáció leképezése */
            if(devspec)
                for(s = 0; devspec->base && devspec->size; devspec++, v += s) {
                    s = (devspec->size+__PAGESIZE-1) & ~(__PAGESIZE-1);
                    vmm_map(tcb, v, vmm_phyaddr(devspec->base), s, PG_USER_DRVMEM|PG_SHARED);
                }
            /* leképezzük az összes MMIO-t és az ACPI memóriát is a címtérbe */
            for(i = s = 0; i<numdrvmem; i++, v += drvmem[i].size) {
                s = (drvmem[i].size+__PAGESIZE-1) & ~(__PAGESIZE-1);
                vmm_map(tcb, v, vmm_phyaddr(drvmem[i].base), s, PG_USER_DRVMEM|PG_SHARED);
            }
            /* naplózás és ütemezőhöz hozzáadás */
            for(c = drvpath+9; *c && *c!='.'; c++);
            if(*c == '.') *c=0;
            syslog(LOG_INFO, " %s pid %x", drvpath+9, tcb->pid);
            sched_add(tcb);
        } else
            task_del(tcb);
    } else {
        /* ha nem adtuk hozzá, de nincs hibakód, az azt jelenti, hogy ezt az eszközmeghajtót már betöltöttük egyszer */
err:    if(((ccb_t*)LDYN_ccb)->core_errno) syslog(LOG_ERR, "%s: %s", errstrs[((ccb_t*)LDYN_ccb)->core_errno], drvpath);
        task_del(tcb);
    }
}

/**
 * új rendszerszolgáltatás taszk hozzáadása. Kicsit kilóg a sorból, de sok a közös elem az eszközmeghajtókkal, itt a helye
 */
void service_add(int srv, char *cmd)
{
    elfctx_t *elfctx = (elfctx_t*)(BUF_ADDRESS + __PAGESIZE);
    tcb_t *tcb = task_new(PRI_SRV);
    virt_t v;
    char *c, *f, *name = (srv == SRV_FS? "FS" : (srv == SRV_UI? "UI" : cmd+4));
    uint l = strlen(name);
    uint64_t s;

    if(task_execinit(tcb, cmd, NULL, envp)) {
        /* betöltjük a szolgáltatás kódját, és leképezzük a konfigurációt vagy a rendszernaplót */
        if(!elf_load((char*)(BUF_ADDRESS + tcb->cmdline), srv == SRV_syslog ? syslog_phy : environment_phy, __PAGESIZE, 0))
            kpanic("%s: %s", TXT_missing, cmd);

        /* betöltjük az ehhez a rendszerszolgáltatáshoz tartozó meghajtókat */
        memcpy(drvpath, "/sys/drv/", 9);
        for(c = drvs; c<drvs_end && *c; c++) {
            f = c; while(c<drvs_end && *c && *c!='\n') c++; /* megkeressük a sor végét */
            if(f + 3 < c && f[l] == 9 && !memcmp(f, name, l)) {              /* ehhez a szolgáltatáshoz tartozó meghajtók */
                f += l+1;
                memcpy(drvpath+9, f, c-f); drvpath[9+c-f] = 0;
                if(!elf_load(drvpath, 0, 0, 0)) {
                    if(!((ccb_t*)LDYN_ccb)->core_errno) seterr(ENOEXEC);
                    syslog(LOG_ERR, "%s: %s", errstrs[((ccb_t*)LDYN_ccb)->core_errno], drvpath);
                    seterr(SUCCESS);
                }
            }
        }
        /* a felhasználói felülethez betöltjük még a videokártya meghajtóját is, ez kritikus elem */
        if(srv == SRV_UI && !elf_load(display_drv, 0, 0, 0))
            kpanic("%s: %s", TXT_missing, display_drv);

        /* aztán összelinkeljük, ez megint kritikus a nagybetűs taszkok esetén */
        if(!elf_rtlink(NULL)) {
            elf_unload(elfctx->elfbin);
            goto err;
        }
        /* aktiváljuk az új kódszegmenst ha nem volt hiba */
        if(task_execfini(tcb)) {
            /* bufferterület feltöltése szolgáltatásspecifikus adatokkal */
            if(srv == SRV_FS)
                vmm_map(tcb, BUF_ADDRESS, bootboot.initrd_ptr, bootboot.initrd_size, PG_USER_RW|PG_SHARED);
            if(srv == SRV_UI) {
                v = BUF_ADDRESS;
                /* framebuffer az UI-nak */
                s = (bootboot.fb_scanline*bootboot.fb_height+__PAGESIZE-1) & ~(__PAGESIZE-1);
                vmm_map(tcb, v, (phy_t)bootboot.fb_ptr, s, PG_USER_DRVMEM|PG_SHARED);
                v += s;
                /* eszközspecifikáció leképezése */
                if(display_dev)
                    for(s = 0; display_dev->base && display_dev->size; display_dev++, v += s) {
                        s = (display_dev->size+__PAGESIZE-1) & ~(__PAGESIZE-1);
                        vmm_map(tcb, v, vmm_phyaddr(display_dev->base), s, PG_USER_DRVMEM|PG_SHARED);
                    }
            }
            /* naplózás és ütemezőhöz hozzáadás */
            syslog(LOG_INFO, " %s -%d pid %x", name, -srv, tcb->pid);
            services[-srv] = tcb->pid;
            sched_add(tcb);
        } else goto err;
    } else {
err:    if(!((ccb_t*)LDYN_ccb)->core_errno) seterr(ENOEXEC);
        if(srv == SRV_FS || srv == SRV_UI)
            kpanic("%s: %s", ((ccb_t*)LDYN_ccb)->core_errno==ENOENT? TXT_missing : errstrs[((ccb_t*)LDYN_ccb)->core_errno], cmd);
        else
            syslog(LOG_ERR, "%s: %s", errstrs[((ccb_t*)LDYN_ccb)->core_errno], cmd);
        task_del(tcb);
    }
}

/**
 * kooperatív ütemezés, eszközmeghajtó és rendszerszolgáltatás taszkok futtatása
 */
void drivers_start()
{
    /* betöltjük az első taszk, az FS szolgáltatás TCB-jét */
    vmm_page(0, LDYN_tmpmap1, services[-SRV_FS] << __PAGEBITS, PG_CORE_RONOCACHE|PG_PAGE);

    syslog(LOG_INFO, "Initializing");

    /* lehazudunk egy megszakításvisszatérést, és ezzel átkapcsolunk a legelső taszkra */
    vmm_switch(((tcb_t*)LDYN_tmpmap1)->memroot);
#if DEBUG
    if(debug&DBG_PROMPT) {
        dbg_start("prompt at boot", false);
        vmm_page(0, LDYN_tmpmap1, services[-SRV_FS] << __PAGEBITS, PG_CORE_RONOCACHE|PG_PAGE);
        vmm_switch(((tcb_t*)LDYN_tmpmap1)->memroot);
    }
#endif
    numinit = 0;
    runlevel = RUNLVL_COOP;

    vmm_enable((tcb_arch_t*)LDYN_tmpmap1);
    /* ezt követően a SYS_recv és a sched_pick() addig választ taszkot, míg az összes nem blokkolódik */
}

/**
 * akkor hívódik, amikor az összes eszközmeghajtó taszk incializálódott és blokkolódott teendőre várva
 */
void drivers_ready()
{
    uint16_t i, j, k;

    /* nem hozunk be új mutexet meg szinkronizációs primitíveket egyetlen kritikus utasítás miatt, K.I.S.S. */
    lockacquire(1, &kprintf_lck);
    numinit++;
    lockrelease(1, &kprintf_lck);

    if(((ccb_t*)LDYN_ccb)->id) {
        do{ cpu_relax; } while(runlevel != RUNLVL_NORM);            /* ha AP-n futunk, bevárjuk a BSP-t */
    } else {
        do{ cpu_relax; } while(numinit < numcores);                 /* ha a BSP-n futunk, bevárjuk a többi CPU-t */

        /* az IRQ útirányó tábla naplózása */
        syslog(LOG_INFO, "IRQ Routing Table (%d IRQs)", irq_max);
        k = (sched_irq>clock_irq? sched_irq : clock_irq) + 1;
        k = irq_max>k? irq_max : k;
        for(i=0; i<k; i++) {
            if(i == sched_irq)
                syslog(LOG_INFO, " %3d: core (scheduler)", i);
            else if(i == clock_irq)
                syslog(LOG_INFO, " %3d: core (wallclock %d Hz)", i, clock_freq);
            else if(i<irq_max && irq_routing_table[i]) {
                for(j=0;irq_routing_table[i][j];j++)
                    syslog(LOG_INFO, " %3d: pid %x", i, irq_routing_table[i][j]);
            }
            /* felkonfigurált IRQ-k engedélyezése */
            if(i == sched_irq || i == clock_irq || (i<irq_max && irq_routing_table[i]))
                intr_enable(i);
        }
        /* naplózzuk, hogy kész vagyunk */
        syslog(LOG_INFO, "Ready. Memory %d of %d pages free.", pmm_freepages, pmm_totalpages);
        /* kikapcsljuk az indító konzol szkrollozását, ezután nincs várakozás, ha betelne a képernyő */
        kprintf_scrolloff();

        /* küldünk egy falióra visszaigazolást, hogy biztosan nullázuk a interrupt jelzőjét */
        clock_ack();

        /* üzenetet küldünk az FS tasznak, hogy most már felcsatolhatja a fájlrendszereket */
        msg_notify(services[-SRV_FS], SYS_mountfs, 0);

        /* átváltunk normál preemptív ütemezésre, ezzel a bootolásnak vége */
        runlevel = RUNLVL_NORM;
    }
}
