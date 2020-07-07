/*
 * core/pmm.c
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
 * @brief Fizikai Memória Kezelő
 */

#include <arch.h>                   /* szükségünk van az architektúra specifikus PG_* jelzőkre */

/* alap adatok és struktúrák */
uint64_t pmm_size;
uint64_t pmm_totalpages;
uint64_t pmm_freepages;
extern pmm_entry_t pmm_entries_buf;
pmm_entry_t *pmm_entries = NULL;

#if DEBUG
void pmm_dump()
{
    uint i;
    pmm_entry_t *fmem = pmm_entries;

    kprintf("pmm size: %d, free: %d, total: %d\n", pmm_size, pmm_freepages, pmm_totalpages);
    for(i=0;i<pmm_size;i++,fmem++) {
        kprintf("  %8x %11d\n", fmem->base, fmem->size);
    }
}
#endif

/**
 * fizikai Memória Kezelő inicializálása
 */
void pmm_init()
{
    char unit, *types[] = { "used", "free", "ACPI", "ANVS", "MMIO" };
    /* a betöltő által szolgáltatott memória térkép */
    MMapEnt *entry = (MMapEnt*)&bootboot.mmap;
    uint64_t i, m=0, e=0, num = (bootboot.size-128)/16, ptr, siz;
    pmm_entry_t *fmem = (pmm_entry_t*)-1;

    pmm_size = pmm_totalpages = 0;
    systables[systable_dma_idx] = 0;
    memset(drvmem, 0, sizeof(drvmem));
    /* a rendszer által lefoglalt memóriát az idle taszknak számoljuk, ezért minnél előbb, már itt inicializáljuk */
#if DEBUG
    if(debug&DBG_MEMMAP)
        kprintf("\nMemory Map (%d entries)\n", num);
#endif
    for(i=0, numdrvmem = 0;i<num;i++,entry++) {
        /* entrópia növelés */
        srand[i&3]++; srand[(i+2)&3] ^= entry->size;
        srand[((entry->ptr >> __PAGEBITS) + entry->size +
            1000*bootboot.datetime[7] + 100*bootboot.datetime[6] + bootboot.datetime[5])&3] *= 16807;
        kentropy();
        e = MMapEnt_Ptr(entry)+MMapEnt_Size(entry);
/*
        if(systables[systable_acpi_idx] >= MMapEnt_Ptr(entry) && systables[systable_acpi_idx] < e)
            entry->size = MMapEnt_Size(entry) | MMAP_ACPI;
*/
#if DEBUG
        if(debug&DBG_MEMMAP)
            kprintf("  %s %8x %8x %11d\n",
                MMapEnt_Type(entry)<5?types[MMapEnt_Type(entry)]:types[0],
                MMapEnt_Ptr(entry), e, MMapEnt_Size(entry)
            );
#endif
        ptr = MMapEnt_Ptr(entry);
        siz = MMapEnt_Size(entry);
        if(MMapEnt_IsFree(entry)) {
            /* szabad memória felső határa */
            if(e > m) m = e;
            /* biztonságból, laphatárra igazítjuk */
            if(ptr & (__PAGESIZE-1)) {
                siz -= (__PAGESIZE-(ptr & (__PAGESIZE-1))) & 0xFFFFFFFFFFFFF0UL;
                ptr = ((ptr+__PAGESIZE-1) & ~(__PAGESIZE-1));
            }
            /* hozzáadás a szabad memória listához */
            if(siz >= __PAGESIZE) {
                /* memóriafoglalás magához a memória listához */
                if(fmem == (pmm_entry_t*)-1) {
                    fmem = pmm_entries = (pmm_entry_t*)ptr;
                    ptr += __PAGESIZE;
                    siz -= __PAGESIZE;
                    if(siz < __PAGESIZE) continue;
                }
                /* DMA buffer lefoglalása a lehető legalacsonyabb címterületen */
                if(dmabuf && !systables[systable_dma_idx] && siz >= (dmabuf << __PAGEBITS)) {
                    systables[systable_dma_idx] = (phy_t)ptr;
                    drvmem[numdrvmem].base = (phy_t)ptr;
                    drvmem[numdrvmem].size = dmabuf << __PAGEBITS;
                    numdrvmem++;
                    ptr += dmabuf << __PAGEBITS;
                    siz -= dmabuf << __PAGEBITS;
                    if(siz < __PAGESIZE) continue;
                }
                /* hozzáadás a listához */
                fmem->base = ptr;
                fmem->size = siz >> __PAGEBITS;
                pmm_size++;
                pmm_totalpages += fmem->size;
                fmem++;
            }
        } else {
            /* a nem szabad rekordokat hozzáadjuk a meghajtók leképezéseinek listájához */
            drvmem[numdrvmem].base = ptr;
            drvmem[numdrvmem].size = siz;
            numdrvmem++;
        }
    }
    pmm_freepages = pmm_totalpages;

    /* memória ellenőrzés, -1 a kerekítési hibák miatt */
    if(m/1024/1024 < PHYMEM_MIN-1)
        kpanic(TXT_notenoughmem, PHYMEM_MIN, m/1024/1024);

    /* memóriatérkép naplózása */
    entry = (MMapEnt*)&bootboot.mmap;
    syslog(LOG_INFO,"Memory Map (%d entries, %d Mbytes free)\n", num, (pmm_totalpages << __PAGEBITS)/1024/1024);
    i = 0;
    while(num>0) {
        ptr = MMapEnt_Ptr(entry);
        siz = MMapEnt_Size(entry);
        if(MMapEnt_IsFree(entry)) {
            if(ptr & (__PAGESIZE-1)) {
                siz -= (__PAGESIZE-(ptr & (__PAGESIZE-1))) & 0xFFFFFFFFFFFFF0UL;
                ptr = ((ptr+__PAGESIZE-1) & ~(__PAGESIZE-1));
            }
            if((pmm_entry_t*)ptr == pmm_entries) { ptr += __PAGESIZE; siz -= __PAGESIZE; }
            if(ptr == systables[systable_dma_idx]) { ptr += dmabuf << __PAGEBITS; siz -= dmabuf << __PAGEBITS; }
        }
        unit = ' ';
        if(siz >= GBYTE) { unit='G'; siz /= GBYTE; } else
        if(siz >= MBYTE) { unit='M'; siz /= MBYTE; } else
        if(siz >= KBYTE) { unit='k'; siz /= KBYTE; }
        syslog(LOG_INFO," %s %8x %8d %c\n",MMapEnt_Type(entry)<5?types[MMapEnt_Type(entry)]:types[0],ptr, siz, unit);
        num--;
        entry++;
    }
    /* DMA buffer ellenőrzése. A fizikai RAM alsó 16M-jában kell lennie */
    if(dmabuf) {
        if(!systables[systable_dma_idx] || systables[systable_dma_idx] + (dmabuf << __PAGEBITS) >= 16*1024*1024)
            kpanic(TXT_nodmamem);
        else
            syslog(LOG_INFO,"DMA buffer @%x, %d pages", systables[systable_dma_idx], dmabuf);
    }

    /* amíg nincsenek taszkjaink, addig az idle-nek számoljuk a memóriafoglalást */
    idle_tcb.magic = OSZ_TCB_MAGICH;
    idle_tcb.allocmem = idle_tcb.linkmem = 0;
}

/**
 * inicializálás befejezése, miután lett virtuális memóriánk
 */
void pmm_vmem()
{
    uint i;

    syslog(LOG_INFO,"VMM mappings %d pages",pmm_totalpages-pmm_freepages);
    /* helyet foglalunk az initrd-nek és leképezzük a kernel memóriába */
    fs_initrd = (virt_t)kalloc_gap(((bootboot.initrd_size+__SLOTSIZE-1)/__SLOTSIZE)*__SLOTSIZE);
    vmm_map(&idle_tcb, fs_initrd, bootboot.initrd_ptr, bootboot.initrd_size, PG_CORE_RO|PG_PAGE|PG_SHARED);
    /* fordítások initrd-n belüli fizikai címeit is át kell alakítani kernel memória címekké */
    for(i=0; i<TXT_LAST; i++) txt[i] += fs_initrd-bootboot.initrd_ptr;
    /* átkapcsolás fizikai címről magas értékű virtuális címre */
    pmm_entries = &pmm_entries_buf;
    runlevel = RUNLVL_VMEM;
}

/**
 * folyamatos fizikai memória lefoglalása. Ficikai címmel tér vissza
 */
void* pmm_alloc(tcb_t *tcb, uint64_t pages)
{
    pmm_entry_t *fmem = pmm_entries;
    uint64_t i=0, b=0, p=0;

    /* futás idejű ellenőrzés */
    if(pmm_freepages>pmm_totalpages)
        kpanic("pmm_alloc: %s freepages %d > totalpages %d",TXT_shouldnothappen,pmm_freepages,pmm_totalpages);
    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH || !pages)
        return NULL;

    lockacquire(63, &pmm_totalpages);
    while(i < pmm_size) {
        if(fmem->size >= (uint64_t)pages) {
            /* entrópia növelése */
            srand[((uint64_t)fmem->base >> __PAGEBITS)&3] ^= (uint64_t)fmem->base >> __PAGEBITS;
            srand[((uint64_t)fmem->base >> __PAGEBITS)&3] *= 16807;
            kentropy();
            /* lap lefoglalása */
            b=fmem->base;
            fmem->base += pages << __PAGEBITS;
            fmem->size -= pages;
            if(!fmem->size) {
                for(;i < pmm_size-1; i++) {
                    pmm_entries[i].base = pmm_entries[i+1].base;
                    pmm_entries[i].size = pmm_entries[i+1].size;
                }
                pmm_size--;
            }
            break;
        }
        fmem++; i++;
    }
    lockrelease(63, &pmm_totalpages);
    if(b) {
        pmm_freepages -= pages;
        tcb->allocmem += pages;
        if(runlevel < RUNLVL_VMEM) {
            memset((void*)b, 0, pages << __PAGEBITS);
        } else {
            for(i=0, p=b; i < pages; i++, p += __PAGESIZE) {
                vmm_page(0, LDYN_tmpmap1, p, PG_CORE_RWNOCACHE|PG_PAGE);
                memset((void*)LDYN_tmpmap1, 0, __PAGESIZE);
            }
        }
#if DEBUG
        if(debug&DBG_PMM)
            kprintf("    pmm_alloc(%d) =>%x pid %x\n",pages,b,tcb->pid);
#endif
    } else {
        if(runlevel < RUNLVL_NORM) {
            /* nincs elég memória. Rendszerindulás közben nem szabad előfordulnia */
#if DEBUG
            pmm_dump();
#endif
            kpanic("pmm_alloc: %s", TXT_outofram);
        }
        seterr(ENOMEM);
    }
    return (void*)b;
}

/**
 * egy egybefüggő, címigazított blokk (2M) lefoglalása. Fizikai címmel tér vissza
 */
void* pmm_allocslot(tcb_t *tcb)
{
    pmm_entry_t *fmem = pmm_entries;
    uint64_t i, j, b, s;

    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH)
        return NULL;

    lockacquire(63, &pmm_totalpages);
    for(i=0; i < pmm_size; i++) {
        b = ((fmem->base + __SLOTSIZE-1) / __SLOTSIZE) * __SLOTSIZE;
        if(fmem->base + (fmem->size << __PAGEBITS) >= b + __SLOTSIZE) {
            /* entrópia növelése */
            srand[((uint64_t)fmem->base >> __PAGEBITS)&3] ^= (uint64_t)fmem->base >> __PAGEBITS;
            srand[((uint64_t)fmem->base >> __PAGEBITS)&3] *= 16807;
            kentropy();
            /* szabad memória bejegyzés kettévágása */
            if(b != fmem->base) {
                for(j=pmm_size; j > i; j--) {
                    pmm_entries[j].base = pmm_entries[j-1].base;
                    pmm_entries[j].size = pmm_entries[j-1].size;
                }
                pmm_size++;
                s = (b-fmem->base) >> __PAGEBITS;
                fmem->size = s;
                fmem++;
            } else {
                s=0;
            }
            /* lefoglalás */
            fmem->base = b+__SLOTSIZE;
            fmem->size -= s+(__SLOTSIZE/__PAGESIZE);
            if(!fmem->size) {
                for(j=i; j < pmm_size-1; j++) {
                    pmm_entries[j].base = pmm_entries[j+1].base;
                    pmm_entries[j].size = pmm_entries[j+1].size;
                }
                pmm_size--;
            }
            pmm_freepages -= (__SLOTSIZE/__PAGESIZE);
            tcb->allocmem += (__SLOTSIZE/__PAGESIZE);
            if(runlevel < RUNLVL_VMEM) {
                memset((void*)b, 0, __SLOTSIZE);
            } else {
                vmm_page(0, LDYN_tmpslot, b, PG_CORE_RWNOCACHE|PG_SLOT);
                memset((void*)LDYN_tmpslot, 0, __SLOTSIZE);
            }
#if DEBUG
            if(debug&DBG_PMM)
                kprintf("    pmm_allocslot() => %x pid %x\n",b,tcb->pid);
#endif
            lockrelease(63, &pmm_totalpages);
            return (void*)b;
        }
        fmem++;
    }
    lockrelease(63, &pmm_totalpages);
    /* nincs elég memória. Rendszerindulás közben nem szabad előfordulnia */
    if(runlevel < RUNLVL_NORM) {
#if DEBUG
        pmm_dump();
#endif
        kpanic("pmm_allocslot: %s", TXT_outofram);
    }
    seterr(ENOMEM);
    return NULL;
}

/**
 * fizikai memória felszabadítása és a szabad listához adása
 */
void pmm_free(tcb_t *tcb, phy_t base, size_t pages)
{
    uint64_t i, j, s = pages << __PAGEBITS;

    /* a legelső fizikai lapot sose szabadítjuk fel, mert a firmver használhatja. x86-on a valós módú IVT
     * van ott (ha esetleg vissza akarnánk menni BIOS-ba), AArch64-on meg a multi-core trampoline code címei. */
    if(!base && pages > 0) { base += __PAGESIZE; pages--; }
    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH || !pages)
        return;

    lockacquire(63, &pmm_totalpages);
    /* entrópia növelése */
    srand[(pages+0)&3] -= (uint64_t)base;
    srand[(pages+2)&3] += (uint64_t)base;
    kentropy();
#if DEBUG
    if(debug&DBG_PMM)
        kprintf("    pmm_free(%x,%d) pid %x\n", base, pages, tcb->pid);
#endif
    /* összefüggő terület keresése */
    for(i=0; i<pmm_size; i++) {
        /* elejére illeszkedik */
        if(pmm_entries[i].base == base+s)
            goto addregion;
        /* végére illeszkedik */
        if(pmm_entries[i].base + (pmm_entries[i].size << __PAGEBITS) == base)
            goto addregion2;
    }
    /* új bejegyzés hozzáadása */
    pmm_size++;
    pmm_entries[i].size = 0;
addregion:
    pmm_entries[i].base = base;
addregion2:
    pmm_entries[i].size += pages;
    pmm_freepages += pages;
    tcb->allocmem -= pages;
    /* megnézzük, összevonható-e egy már létező másik bejegyzéssel. Ez ekkor fordulhat elő, ha a most felszabadított
     * rész pont két másik bejegyzés közötti űrt tölti ki, ekkor három bejegyzés helyett csak egyet hagyunk meg */
    for(j=0; j<pmm_size; j++) {
        if(pmm_entries[i].base + (pmm_entries[i].size << __PAGEBITS) == pmm_entries[j].base) goto merge;
        if(pmm_entries[i].base == pmm_entries[j].base + (pmm_entries[j].size << __PAGEBITS)) {
            pmm_entries[i].base = pmm_entries[j].base;
merge:      pmm_entries[i].size += pmm_entries[j].size;
            memcpy((void*)&pmm_entries[j], (void*)&pmm_entries[j+1], (pmm_size-j-1)*sizeof(pmm_entry_t));
            pmm_size--;
            break;
        }
    }
    lockrelease(63,&pmm_totalpages);
    if(pmm_freepages>pmm_totalpages)
        kpanic("pmm_free: %s freepages %d > totalpages %d",TXT_shouldnothappen,pmm_freepages,pmm_totalpages);
}
