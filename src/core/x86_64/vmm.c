/*
 * core/x86_64/vmm.c
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
extern void platform_exc00divzero();
extern void platform_exc31();
extern phy_t environment_phy, syslog_phy;

uint64_t bssalign idt[512];     /* Megszakítás Leíró Tábla */

/**
 * Virtuális Memória Kezelő inicializálása
 */
void vmm_init()
{
    phy_t *cr3, *pml4, *pdpe, *pde, *ptr1, *ptr2, *ptr3, *stackptr, *cores;
    ccb_t *ccb;
    tcb_arch_t *tcb = (tcb_arch_t*)&idle_tcb;
    void (*fnc)() = &platform_exc00divzero;
    uint64_t i;

    /* ez nagyon korán hívódik, egy-az-egy leképezést feltételez */
    __asm__ __volatile__("mov %%cr3, %0" : "=a"(cr3));
    cr3= (phy_t*)vmm_phyaddr((phy_t)cr3);
    pml4=(phy_t*)vmm_phyaddr(cr3[511]);     /* 512G */
    pdpe=(phy_t*)vmm_phyaddr(pml4[511]);    /* 1G */
    pde= (phy_t*)vmm_phyaddr(pdpe[511]);    /* 2M */
    /* nincs még futó alkalmazás, ezért a foglalásokat az idle-nek könyveljük */
    tcb->common.memroot = (phy_t)cr3;
    tcb->cs = 0x8;          /* ring 0-ás memória szegmens szelektorok */
    tcb->ss = 0x10;
    tcb->rflags = 0x202;    /* megszakítások engedélyezése és a kötelező 1-es bit */
    /* leképezések biztonságosabbá tétele */
    cr3[511]  = vmm_phyaddr(cr3[511])  | PG_USER_RW | PG_PAGE;
    pml4[511] = vmm_phyaddr(pml4[511]) | PG_CORE_RW | PG_PAGE;
    pdpe[511] = vmm_phyaddr(pdpe[511]) | PG_CORE_RW | PG_PAGE;
    pde[0] = vmm_phyaddr(pde[0]) | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;              /* bootboot struct */
    pde[1] = vmm_phyaddr(pde[1]) | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;              /* environment környezet */
    for(i=2;i<(((phy_t)&__data & 0xfffff) >> __PAGEBITS);i++)
        pde[i] = vmm_phyaddr(pde[i]) | PG_CORE_RO | PG_PAGE;                            /* text kódszegmens */
    for(;i<(uint64_t)(512-(numcores+3)/4);i++) if(pde[i])
        pde[i] = vmm_phyaddr(pde[i]) | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;   /* data adatszegmens */
    for(;i<512;i++)
        pde[i] = vmm_phyaddr(pde[i]) | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;          /* cpu-nkénti verem */
    pde[((phy_t)&pmm_entries_buf >> __PAGEBITS) & 511] = (phy_t)pmm_entries | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;
    tcb->common.pid = vmm_phyaddr(pde[((phy_t)&idle_tcb >> __PAGEBITS) & 511]) >> __PAGEBITS;
    environment_phy = vmm_phyaddr(pde[1]);
    syslog_phy = vmm_phyaddr(pde[((phy_t)&syslog_buffer >> __PAGEBITS) & 511]);
    /* globális megosztott memória leképezése, a fizikai 1G címeknek egyezniük kell minden CPU-n */
    for(i=0;i<((LDYN_ADDRESS>>30) & 511);i++)
        pml4[i] = (phy_t)pmm_alloc(&idle_tcb, 1) | (1L<<PG_NX_BIT) | PG_USER_RWNOCACHE | PG_PAGE;
    /* globális megosztott memória első lapjának leképezése */
    ptr1 = (phy_t*)vmm_phyaddr(pml4[0]);
    ptr2 = pmm_alloc(&idle_tcb, 1);
    ptr3 = pmm_alloc(&idle_tcb, 1);
    ptr1[0] = (phy_t)ptr2 | (1L<<PG_NX_BIT) | PG_USER_RWNOCACHE | PG_PAGE;
    ptr2[0] = (phy_t)ptr3 | (1L<<PG_NX_BIT) | PG_USER_RWNOCACHE | PG_PAGE;
    /* globális core memória első lapjának leképezése */
    cores = pmm_alloc(&idle_tcb, MAXCPUS/(__PAGESIZE/8));
    for(i=0;i<MAXCPUS/(__PAGESIZE/8);i++)
        pdpe[((CCBS_ADDRESS>>21) & 511)+i] = (((phy_t)cores)+i*__PAGESIZE) | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;
    ptr1 = pmm_alloc(&idle_tcb, 1);
    pdpe[(CDYN_ADDRESS>>21) & 511]     = (phy_t)ptr1 | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;
    ptr2 = pmm_alloc(&idle_tcb, 1);
    ptr1[0] = (phy_t)ptr2 | (1L<<PG_NX_BIT) | PG_CORE_RWNOCACHE | PG_PAGE;
    /* külön lapfordítótár minden CPU-nak a core memóriához */
    stackptr = (phy_t*)-8;
    /* vigyázni kell, hogy a verem ne nőjjön 1k fölé, mert akkor belelógna az első AP vermébe */
    for(i=0;i<numcores;i++) {
        ptr1 = i ? pmm_alloc(&idle_tcb, 1) : cr3;
        *stackptr = (phy_t)ptr1;        /* cr3 letárolása a CPU vermek tetején */
        stackptr -= 1024/8;
        /* PML4 másolása az AP-nak */
        if(i) {
            ptr2 = pmm_alloc(&idle_tcb, 1);
            memcpy(ptr2, pml4, __PAGESIZE);
            ptr1[511] = (phy_t)ptr2 | PG_USER_RW;
            ptr1 = ptr2;
        } else {
            ptr1 = pml4;
        }
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
        /* megszakításkezelők vermének beállítása. A cím ugyanaz,
         * de taszkonként (és CPU-nként) más fizikai lap van ide leképezve */
        ccb->ist1 = __PAGESIZE;                     /* megszakítás verem, minden taszknak saját */
        ccb->ist2 = (phy_t)ccb + __PAGESIZE;        /* biztonságos NMI verem minden CPU-nak egy */
        ptr3[0] = (phy_t)ccb | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;          /* CPU Kontrol Blokk */
        cores[i] = ptr3[0];
        ptr3[2] = (phy_t)ptr1 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;         /* 1G leképezési tábla */
        ptr3[3] = (phy_t)ptr2 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;         /* 2M leképezési tábla */
        ptr3[4] = (phy_t)ptr3 | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;         /* 4K leképezési tábla */
        ptr3[5] = (phy_t)ccb | (1L<<PG_NX_BIT) | PG_USER_RO | PG_PAGE;          /* CPU Kontrol Blokk, csak olvasható */
    }
    /* új leképezés hattatása */
    __asm__ __volatile__("mov %0, %%cr3" : : "a"(cr3));

    /* Sajnos sem a GNU ld, sem az LLVM lld nem elég okos ahhoz, hogy fordítási időben
     * feltöltse a táblát statikus címekkel, ezért futásidőben nekünk kell megtennünk.
     * minden kód 128 bájt, ennek egyeznie kell a platform.S-beli .balign-okkal. */
#if DEBUG
    if((&platform_exc31-&platform_exc00divzero)!=31*128) kpanic("Misaligned ISRs");
#endif
    for(i=0; i<32; i++, fnc+=128)
        IDT_GATE(i, i==2||i==8||i==18?IDT_NMI:IDT_EXC, fnc);
}

/**
 * létrehoz egy új virtuális címteret
 */
tcb_t *vmm_new(uint16_t cpu, uint8_t priority)
{
    ccb_t *ccb = (ccb_t*)(cpu<numcores? CCBS_ADDRESS+cpu*__PAGESIZE : LDYN_ccb);
    tcb_arch_t *tcb = (tcb_arch_t*)LDYN_tmpmap4;
    phy_t cr3, pml4, pdpe, pde, pte, *ptr = (phy_t*)LDYN_tmpmap4;

    /* amíg nincs meg az új TCB-nk, addig az idle-nek számoljuk a foglalást */
    cr3  = (phy_t)pmm_alloc(&idle_tcb, 1);
    pml4 = (phy_t)pmm_alloc(&idle_tcb, 1);
    pdpe = (phy_t)pmm_alloc(&idle_tcb, 1);
    pde  = (phy_t)pmm_alloc(&idle_tcb, 1);
    pte  = (phy_t)pmm_alloc(&idle_tcb, 1);
    /* 512G-s bejegyzések */
    vmm_page(0, LDYN_tmpmap4, cr3, PG_CORE_RWNOCACHE | PG_PAGE);
    ptr[0] = pml4 | PG_USER_RW | PG_PAGE;
    ptr[511] = ccb->sharedroot | PG_USER_RW | PG_PAGE;
    /* 1G-s bejegyzések */
    vmm_page(0, LDYN_tmpmap4, pml4, PG_CORE_RWNOCACHE | PG_PAGE);
    ptr[0] = pdpe | PG_USER_RW | PG_PAGE;
    /* 2M-s bejegyzések */
    vmm_page(0, LDYN_tmpmap4, pdpe, PG_CORE_RWNOCACHE | PG_PAGE);
    ptr[0] = pde | PG_USER_RW | PG_PAGE;
    /* 4k-s bejegyzések */
    vmm_page(0, LDYN_tmpmap4, pde, PG_CORE_RWNOCACHE | PG_PAGE);
    ptr[0] = pte | (1L<<PG_NX_BIT) | PG_CORE_RW | PG_PAGE;
    /* TCB */
    vmm_page(0, LDYN_tmpmap4, pte, PG_CORE_RWNOCACHE | PG_PAGE);
    tcb->common.magic = OSZ_TCB_MAGICH;
    tcb->common.cpuid = cpu<numcores? cpu : 0;
    tcb->common.priority = priority;
    tcb->common.pid = pte >> __PAGEBITS;
    tcb->common.memroot = cr3;
    tcb->common.taskmemroot = pml4;
    /* memória szegmensek a megszakítási verem tetején */
    tcb->cs = 0x20+3;       /* ring 3 felhasználói kód */
    tcb->ss = 0x18+3;       /* ring 3 felhasználói adat */
    tcb->rflags = 0x202;    /* megszakítások engedélyezése és a kötelező 1-es bit */
    /* eszközmeghajtóknak IOPL=3, hogy eléhessék a B/K címteret is */
    if(priority == PRI_DRV) tcb->rflags |= (3<<12);
    /* korrigáljuk a memória foglalást */
    tcb->common.allocmem = 5;
    idle_tcb.allocmem -= 5;
#if DEBUG
    if(debug&DBG_VMM)
        kprintf("  vmm_new(cpu=%d) => pid %x\n", tcb->common.cpuid, tcb->common.pid);
#endif
    return (tcb_t*)tcb;
}
