/*
 * core/aarch64/vmm.c
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
 * @subsystem memória
 * @brief Virtuális Memória Kezelő, architektúra függő rész
 */

#include <arch.h>

/* itt megtehetjük, hogy elvárjuk a 4k-s lapkeretet */
c_assert(__PAGEBITS==12);

extern void *pmm_entries;
extern uint8_t __data;
extern uint8_t pmm_entries_buf;
extern char syslog_buffer[];
extern phy_t environment_phy, syslog_phy;

/**
 * Virtuális Memória Kezelő inicializálása
 */
void vmm_init()
{
    phy_t *l1, *l2, *l3, *ptr1, *ptr2, *ptr3, *stackptr, *cores;
    ccb_t *ccb;
    uint64_t i,r;

    /* ez nagyon korán hívódik, egy-az-egy leképezést feltételez */
    __asm__ __volatile__ ("mrs %0, ttbr0_el1" : "=r" (l1));
    /* nincs még futó alkalmazás, ezért a foglalásokat az idle-nek könyveljük */
    idle_tcb.memroot = vmm_phyaddr((phy_t)l1);
    __asm__ __volatile__ ("mrs %0, ttbr1_el1" : "=r" (l1));
    l1=(phy_t*)vmm_phyaddr(l1);         /* 512G */
    l2=(phy_t*)vmm_phyaddr(l1[511]);    /* 1G */
    l3=(phy_t*)vmm_phyaddr(l2[511]);    /* 2M */
    /* leképezések biztonságosabbá tétele */
    l1[511] = vmm_phyaddr(l1[511]) | PG_CORE_RW | PG_PAGE;
    l2[511] = vmm_phyaddr(l2[511]) | PG_CORE_RW | PG_PAGE;
    l3[0] = vmm_phyaddr(l3[0]) | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;              /* bootboot struct */
    l3[1] = vmm_phyaddr(l3[1]) | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;              /* environment környezet */
    for(i=2;i<(((phy_t)&__data & 0xfffff) >> __PAGEBITS);i++)
        l3[i] = vmm_phyaddr(l3[i]) | PG_CORE_RO | PG_PAGE;                            /* text kódszegmens */
    for(;i<(uint64_t)(512-(numcores+3)/4);i++) if(l3[i])
        l3[i] = vmm_phyaddr(l3[i]) | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;   /* data adatszegmens */
    for(;i<512;i++)
        l3[i] = vmm_phyaddr(l3[i]) | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;          /* cpu-nkénti verem */
    l3[((phy_t)&pmm_entries_buf >>__PAGEBITS) & 511] = (phy_t)pmm_entries | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;
    idle_tcb.pid = vmm_phyaddr(l3[((phy_t)&idle_tcb >> __PAGEBITS) & 511]) >> __PAGEBITS;
    environment_phy = vmm_phyaddr(l3[1]);
    syslog_phy = vmm_phyaddr(l3[((phy_t)&syslog_buffer >> __PAGEBITS) & 511]);
    /* globális megosztott memória leképezése, a fizikai 1G címeknek egyezniük kell minden CPU-n */
    for(i=0;i<((LDYN_ADDRESS>>30) & 511);i++)
        l1[i] = (phy_t)pmm_alloc(&idle_tcb, 1) | (1L<<PG_NX_BIT) | PG_USER_RWNOCACHE | PG_PAGE;
    /* globális megosztott memória első lapjának leképezése */
    ptr1 = (phy_t*)vmm_phyaddr(l1[0]);
    ptr2 = pmm_alloc(&idle_tcb, 1);
    ptr3 = pmm_alloc(&idle_tcb, 1);
    ptr1[0] = (phy_t)ptr2 | (1L<<PG_NX_BIT) | PG_USER_RWNOCACHE | PG_PAGE;
    ptr2[0] = (phy_t)ptr3 | (1L<<PG_NX_BIT) | PG_USER_RWNOCACHE | PG_PAGE;
    /* globális core memória első lapjának leképezése */
    cores = pmm_alloc(&idle_tcb, MAXCPUS/(__PAGESIZE/8));
    for(i=0;i<MAXCPUS/(__PAGESIZE/8);i++)
        l2[((CCBS_ADDRESS>>21) & 511)+i] = (((phy_t)cores)+i*__PAGESIZE) | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;
    ptr1 = pmm_alloc(&idle_tcb, 1);
    l2[(CDYN_ADDRESS>>21) & 511]       = (phy_t)ptr1 | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;
    ptr2 = pmm_alloc(&idle_tcb, 1);
    ptr1[0] = (phy_t)ptr2 | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;
    /* külön lapfordítótár minden CPU-nak a core memóriához */
    stackptr = (phy_t*)-16;
    /* vigyázni kell, hogy a verem ne nőjjön 1k fölé, mert akkor belelógna az első AP vermébe */
    for(i=0;i<numcores;i++) {
        ptr1 = i ? pmm_alloc(&idle_tcb, 1) : l1;
        *stackptr = (phy_t)ptr1 | 1;        /* ttbr1_el1 letárolása a CPU vermek tetején */
        stackptr -= 1024/8;
        /* L1 másolása az AP-nak */
        if(i)
            memcpy(ptr1, l1, __PAGESIZE);
        /* 1G */
        ptr2 = pmm_alloc(&idle_tcb, 1);
        ptr1[(LDYN_ADDRESS>>30) & 511] = (phy_t)ptr2 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;
        /* 2M */
        ptr3 = pmm_alloc(&idle_tcb, 1);
        ptr2[(LDYN_ADDRESS>>21) & 511] = (phy_t)ptr3 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;
        /* 4k */
        ccb = (ccb_t*)pmm_alloc(&idle_tcb, 1);
        ccb->magic = OSZ_CCB_MAGICH;
        ccb->id = i;
        ccb->sharedroot = vmm_phyaddr(ptr1);
        ptr3[0] = (phy_t)ccb | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;          /* CPU Kontrol Blokk */
        cores[i] = ptr3[0];
        ptr3[2] = (phy_t)ptr1 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;         /* 1G leképezési tábla */
        ptr3[3] = (phy_t)ptr2 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;         /* 2M leképezési tábla */
        ptr3[4] = (phy_t)ptr3 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;         /* 4K leképezési tábla */
        ptr3[5] = (phy_t)ccb | (1L<<PG_NX_BIT) | PG_USER_RO | PG_PAGE;          /* CPU Kontrol Blokk, csak olvasható */
    }
    /* új leképezés hattatása */
    r=  (0xFF << 0) |    /* Attr=0: normál, IWBWA, OWBWA, NTR */
        (0x04 << 8) |    /* Attr=1: eszköz, nGnRE (OSH is) */
        (0x44 <<16);     /* Attr=2: nem gyorsítótárazható */
    __asm__ __volatile__ ("msr mair_el1, %0" : : "r" (r));
    __asm__ __volatile__ ("dsb sy; tlbi vmalle1");
    __asm__ __volatile__ ("mrs %0, sctlr_el1" : "=r" (r));
    r|=(1<<19)|(1<<12)|(1<<2); /* WXN, I utasítás és C adat gyorsítótár engedélyezése */
    __asm__ __volatile__ ("msr sctlr_el1, %0; isb" : : "r" (r));
}

/**
 * létrehoz egy új virtuális címteret
 */
tcb_t *vmm_new(uint16_t cpu, uint8_t priority)
{
    tcb_t *tcb = (tcb_t*)LDYN_tmpmap4;
    phy_t l0, l1, l2, l3, *ptr = (phy_t*)LDYN_tmpmap4;

    /* amíg nincs meg az új TCB-nk, addig az idle-nek számoljuk a foglalást */
    l0 = (phy_t)pmm_alloc(&idle_tcb, 1);
    l1 = (phy_t)pmm_alloc(&idle_tcb, 1);
    l2 = (phy_t)pmm_alloc(&idle_tcb, 1);
    l3 = (phy_t)pmm_alloc(&idle_tcb, 1);
    /* 1G-s bejegyzések */
    vmm_page(0, LDYN_tmpmap4, l0, PG_USER_RWNOCACHE | PG_PAGE);
    ptr[0] = l1 | PG_USER_RW | PG_PAGE;
    /* 2M-s bejegyzések */
    vmm_page(0, LDYN_tmpmap4, l1, PG_CORE_RWNOCACHE | PG_PAGE);
    ptr[0] = l2 | PG_USER_RW | PG_PAGE;
    /* 4K-s bejegyzések */
    vmm_page(0, LDYN_tmpmap4, l2, PG_CORE_RWNOCACHE | PG_PAGE);
    ptr[0] = l3 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;
    /* TCB */
    vmm_page(0, LDYN_tmpmap4, l3, PG_CORE_RWNOCACHE | PG_PAGE);
    tcb->magic = OSZ_TCB_MAGICH;
    tcb->cpuid = cpu<numcores? cpu : 0;
    tcb->priority = priority;
    tcb->pid = l3 >> __PAGEBITS;
    tcb->memroot = tcb->taskmemroot = l0;
    /* korrigáljuk a memória foglalást */
    tcb->allocmem = 4;
    idle_tcb.allocmem -= 4;
#if DEBUG
    if(debug&DBG_VMM)
        kprintf("  vmm_new(cpu=%d) => pid %x\n", tcb->cpuid, tcb->pid);
#endif
    return tcb;
}
