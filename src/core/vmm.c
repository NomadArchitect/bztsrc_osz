/*
 * core/vmm.c
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
 * @brief Virtuális Memória Kezelő, architektúra független rész
 */

#include <arch.h>                   /* szükségünk van az architektúra specifikus PG_* jelzőkre */

/* ennek a kódnak lapkeret méret függetlennek kellene lennie, de egyelőre felételezzük, hogy 4k */
c_assert(__PAGESIZE==4096 && __PAGEBITS==12);

extern char kpanictlb[];            /* lapfordítás pánik üzenet */

/**
 * egy fizikai lap leképezése a virtuális címtérbe, nem igényel tcb-t, optimalizált, csak a core használja
 */
void vmm_page(virt_t memroot, virt_t virt, phy_t phys, uint64_t access)
{
    virt_t page = 0, *ptr = (uint64_t*)LDYN_tmpctrl, *ctrl = (uint64_t*)LDYN_tmpctrlptr;
    int step = 0;
    switch(virt) {
        /* gyors elérés a gyakran használt core lapokra */
        case LDYN_tcbalarm: ptr=(virt_t*)LDYN_tcbalrmptr; break;
        case LDYN_tmpmap1:  ptr=(virt_t*)LDYN_tmpmap1ptr; break;
        case LDYN_tmpmap2:  ptr=(virt_t*)LDYN_tmpmap2ptr; break;
        case LDYN_tmpmap3:  ptr=(virt_t*)LDYN_tmpmap3ptr; break;
        case LDYN_tmpmap4:  ptr=(virt_t*)LDYN_tmpmap4ptr; break;
        case LDYN_tmpmap5:  ptr=(virt_t*)LDYN_tmpmap5ptr; break;
        case LDYN_tmpmap6:  ptr=(virt_t*)LDYN_tmpmap6ptr; break;
        case LDYN_tmpmap7:  ptr=(virt_t*)LDYN_tmpmap7ptr; break;
        case LDYN_tmpslot:  ptr=(virt_t*)LDYN_tmpslotptr; break;
        /* egyébként végigjárjuk a címfordítási laptáblákat */
        default:
            /* kanonikus cím? */
            if((virt>>48)!=0 && (virt>>48)!=0xffff) goto err;
            /* 512G */
            if((virt>>39)!=0 && ((virt>>39)&511)!=511) goto err;
            /* memroot felülbírálása core-nál és az aktuális taszknál */
            vmm_pageprologue;
            step++;
            /* 1G */
            *ctrl = vmm_phyaddr(memroot)|PG_CORE_RWNOCACHE|PG_PAGE;
            vmm_invalidate(LDYN_tmpctrl);
            page = ptr[(virt>>30)&511];
            if(!vmm_ispresent(page)) goto err;
            step++;
            /* 2M */
            *ctrl = vmm_phyaddr(page)|PG_CORE_RWNOCACHE|PG_PAGE;
            vmm_invalidate(LDYN_tmpctrl);
            if(vmm_isslot(access)) ptr = &ptr[(virt>>21)&511];
            else {
                page = ptr[(virt>>21)&511];
                if(!vmm_ispresent(page)) {
err:
                    kpanic(kpanictlb, virt, step);
                }
                /* 4K */
                *ctrl = vmm_phyaddr(page)|PG_CORE_RWNOCACHE|PG_PAGE;
                vmm_invalidate(LDYN_tmpctrl);
                ptr = &ptr[(virt>>__PAGEBITS)&511];
            }
    }
    /* ha írható, akkor nem futtatható. -1 cím esetén csak a hozzáférést állítjuk */
    *ptr = vmm_phyaddr(phys!=(phy_t)-1?phys:*ptr) |access|vmm_wnx(access);
    /* MMU fordítási gyorsítótár érvénytelenítése */
    vmm_invalidate(virt);
}

/**
 * általános memória leképezése a virtuális címtérbe. Ha phys 0, új memóriát foglal. Többek között a bztalloc is hívja
 */
void *vmm_map(tcb_t *tcb, virt_t bss, phy_t phys, size_t size, uint64_t access)
{
    virt_t page = 0, memroot = 0, *ptr = (uint64_t*)LDYN_tmpctrl, *ctrl = (uint64_t*)LDYN_tmpctrlptr;
    uint64_t a = access, i;
    void *ret = (void*)bss;

    /* paraméterek ellenőrzése */
    phys = vmm_phyaddr(phys);
    size = ((size+__PAGESIZE-1)>>__PAGEBITS)<<__PAGEBITS;
    if(!size || (tcb != &idle_tcb && !bss) || ((bss>>39)!=0 && ((bss>>39)&511)!=511)) {
        seterr(EINVAL);
        return (void *)-1;
    }
    if(bss<SDYN_ADDRESS) {
        /* ha még nincs taszk belapozva */
        if(tcb->magic != OSZ_TCB_MAGICH) return (void *)-1;
        /* ha dynbss szegmens, akkor felhasználói, írható, de nem futtatható */
        if(bss>=DYN_ADDRESS && bss<BUF_ADDRESS) a = access = (1UL<<PG_NX_BIT)|PG_USER_RW; else
        /* egyébként vagy gyorsítótárazható, vagy nem, de futtatható semmiképp sem lehet */
            a |= (1UL<<PG_NX_BIT);
    } else {
        /* megosztott memória, felhasználói, írható, nem futtatható */
        if(bss<LDYN_ADDRESS) { a |= (1UL<<PG_NX_BIT); access = (1UL<<PG_NX_BIT)|PG_USER_RWNOCACHE; }
        /* kernel bss memória egyébként, felügyeleti szint, írható, nem futtatható */
        else a = access = (1UL<<PG_NX_BIT)|PG_CORE_RWNOCACHE;
        /* plusz ellenőrzés, hogy ne allokáljunk többet, mint amennyi helyünk van */
        if(bss <= CDYN_TOP && bss+size >= CDYN_TOP) {
            seterr(ENOMEM);
            return (void *)-1;
        }
    }
    /* 512G, ez trükkös, mert ha core memóriát képezünk le, átkapcsol idle_tcb-re, hogy annak könyveljük */
    vmm_getmemroot(bss);
    memroot = vmm_phyaddr(memroot);

#if DEBUG
    if(debug&DBG_VMM)
        kprintf("  vmm_map(m=%x,b=%x,p=%x,s=%d,a=%x)\n", memroot, bss, phys, size, a);
#endif
again:
    /* 1G */
    *ctrl = memroot|PG_CORE_RWNOCACHE|PG_PAGE;
    vmm_invalidate(LDYN_tmpctrl);
    i = (bss>>30)&511;
    if(!vmm_ispresent(ptr[i])) {
        page = (phy_t)pmm_alloc(tcb, 1);
        if(!page) return (void*)-1;
        ptr[i] = page |access|PG_PAGE;
    }
    /* 2M */
    *ctrl = vmm_phyaddr(ptr[i])|PG_CORE_RWNOCACHE|PG_PAGE;
    vmm_invalidate(LDYN_tmpctrl);
    i = (bss>>21)&511;
    if(!vmm_ispresent(ptr[i])) {
        /* 2M leképezése ha címigazított és elég nagy */
        if(!(phys&(__SLOTSIZE-1)) && !(bss&(__SLOTSIZE-1)) && size>=__SLOTSIZE) {
            while(size>=__SLOTSIZE && i<__PAGESIZE/8) {
                if(vmm_ispresent(ptr[i])) { seterr(EFAULT); return (void*)-1; }
                if(phys) {
                    ptr[i] = phys |a|PG_SLOT|PG_SHARED;
                    phys += __SLOTSIZE;
                    tcb->linkmem += __SLOTSIZE/__PAGESIZE;
                } else {
                    page = (phy_t)pmm_allocslot(tcb);
                    if(!page) return (void*)-1;
                    ptr[i] = page |a|PG_SLOT;
                }
                vmm_invalidate(bss);
                bss  += __SLOTSIZE;
                size -= __SLOTSIZE;
                i++;
            }
            goto again;
        }
        page = (phy_t)pmm_alloc(tcb, 1);
        if(!page) return (void*)-1;
        ptr[i] = page |access|PG_PAGE;
    }
    /* 4K */
    *ctrl = vmm_phyaddr(ptr[i])|PG_CORE_RWNOCACHE|PG_PAGE;
    vmm_invalidate(LDYN_tmpctrl);
    i = (bss>>__PAGEBITS)&511;
    while(size>0) {
        if(vmm_ispresent(ptr[i])) { seterr(EFAULT); return (void*)-1; }
        if(phys) {
            ptr[i] = phys |a|PG_PAGE|PG_SHARED;
            phys += __PAGESIZE;
            tcb->linkmem++;
        } else {
            page = (phy_t)pmm_alloc(tcb, 1);
            if(!page) return (void*)-1;
            ptr[i] = page |a|PG_PAGE;
        }
        vmm_invalidate(bss);
        bss  += __PAGESIZE;
        size -= __PAGESIZE;
        i++;
        if(i>511) goto again;
    }
    return ret;
}

/**
 * kódszegmmens leképezése a virtuális címtérbe. Ez mindig a taszk kódbufferébe dolgozik, elf_load() hívja
 */
void *vmm_maptext(tcb_t *tcb, virt_t bss, phy_t *phys, size_t size)
{
    virt_t page = 0, *ptr = (uint64_t*)LDYN_tmpctrl, *ctrl = (uint64_t*)LDYN_tmpctrlptr;
    uint64_t i;
    void *ret = (void*)bss;

    /* paraméterek ellenőrzése */
    size = ((size+__PAGESIZE-1)>>__PAGEBITS)<<__PAGEBITS;
    if(!size || bss<BUF_ADDRESS+TEXT_ADDRESS || bss>BUF_ADDRESS+DYN_ADDRESS || tcb->magic != OSZ_TCB_MAGICH) {
        seterr(EINVAL);
        return NULL;
    }

#if DEBUG
    if(debug&DBG_VMM)
        kprintf("  vmm_maptext(m=%x,b=%x,*p=%x,s=%d)\n", tcb->taskmemroot, bss, phys, size);
#endif
again:
    /* 1G */
    *ctrl = tcb->taskmemroot|PG_CORE_RWNOCACHE|PG_PAGE;
    vmm_invalidate(LDYN_tmpctrl);
    i = (bss>>30)&511;
    if(!vmm_ispresent(ptr[i])) {
        page = (phy_t)pmm_alloc(tcb, 1);
        if(!page) return NULL;
        ptr[i] = page |PG_USER_RW|PG_PAGE;
    }
    /* 2M */
    *ctrl = vmm_phyaddr(ptr[i])|PG_CORE_RWNOCACHE|PG_PAGE;
    vmm_invalidate(LDYN_tmpctrl);
    i = (bss>>21)&511;
    if(!vmm_ispresent(ptr[i])) {
        page = (phy_t)pmm_alloc(tcb, 1);
        if(!page) return NULL;
        ptr[i] = page |PG_USER_RW|PG_PAGE;
    }
    /* 4K */
    *ctrl = vmm_phyaddr(ptr[i])|PG_CORE_RWNOCACHE|PG_PAGE;
    vmm_invalidate(LDYN_tmpctrl);
    i = (bss>>__PAGEBITS)&511;
    while(size>0) {
        if(vmm_ispresent(ptr[i])) { seterr(EFAULT); return (void*)-1; }
        ptr[i] = *phys++ |PG_USER_RO|PG_PAGE|PG_SHARED;
        tcb->linkmem++;
        vmm_invalidate(bss);
        bss  += __PAGESIZE;
        size -= __PAGESIZE;
        i++;
        if(i>511) goto again;
    }
    return ret;
}

/**
 * aktív kódszegmmens és a taszk kódbufferének cseréje, task_execfini() hívja, ha sikeres volt a betöltés
 */
void vmm_swaptextbuf(tcb_t *tcb)
{
    virt_t page = 0, *ptr = (uint64_t*)LDYN_tmpctrl, *ctrl = (uint64_t*)LDYN_tmpctrlptr;
    uint64_t i, j = (BUF_ADDRESS>>30)&511;

    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH) {
        seterr(EINVAL);
        return;
    }

#if DEBUG
    if(debug&DBG_VMM)
        kprintf("  vmm_swaptextbuf(m=%x)\n", tcb->memroot);
#endif
    /* 1G */
    *ctrl = vmm_phyaddr(tcb->taskmemroot)|PG_CORE_RWNOCACHE|PG_PAGE;
    vmm_invalidate(LDYN_tmpctrl);
    for(i=0; i<(DYN_ADDRESS/GBYTE); i++) {
        page = ptr[j+i];
        ptr[j+i] = ptr[i];
        ptr[i] = page;
        vmm_invalidate(i*GBYTE);
        vmm_invalidate(BUF_ADDRESS + i*GBYTE);
    }
    /* megcseréljük a megosztott jelzőbitet a kódszegmens és a kódbuffer tcb-éin */
    vmm_page(tcb->memroot, BUF_ADDRESS, -1, PG_CORE_RW | PG_PAGE | PG_SHARED);
    vmm_page(tcb->memroot, 0, -1, PG_CORE_RW | PG_PAGE);
    vmm_page(tcb->memroot, __PAGESIZE, -1, PG_USER_RO | PG_PAGE);  /* az üzenetsor csak olvasható felhasználói szintről */
}

/**
 * memória leképezés megszüntetése a virtuális cimtérben, és a hozzá tartozó fizikai RAM felszabadítása
 */
void vmm_free(tcb_t *tcb, virt_t bss, size_t size)
{
    virt_t *ptr1 = (uint64_t*)LDYN_tmpctrl, *ctrl1 = (uint64_t*)LDYN_tmpctrlptr;
    virt_t *ptr2 = (uint64_t*)LDYN_tmpmap1, *ctrl2 = (uint64_t*)LDYN_tmpmap1ptr;
    virt_t *ptr3 = (uint64_t*)LDYN_tmpmap2, *ctrl3 = (uint64_t*)LDYN_tmpmap2ptr;
    virt_t memroot = 0, start = bss, end;
    uint64_t i,j,je,k,ke,n;

    /* paraméterek ellenőrzése */
    size = ((size+__PAGESIZE-1)>>__PAGEBITS)<<__PAGEBITS;
    if(!size || ((bss>>39)!=0 && ((bss>>39)&511)!=511) || tcb->magic != OSZ_TCB_MAGICH) {
        seterr(EINVAL);
        return;
    }
    /* 512G */
    vmm_getmemroot(bss);
    memroot = vmm_phyaddr(memroot);

    end = bss+size;
#if DEBUG
    if(debug&DBG_VMM)
        kprintf("  vmm_free(m=%x,b=%x,s=%d,e=%x)\n", memroot, bss, size, end);
#endif
    *ctrl1 = memroot|PG_CORE_RWNOCACHE|PG_PAGE;
    vmm_invalidate(LDYN_tmpctrl);
    while(bss < end) {
        /* 1G */
        i = (bss>>30)&511;
        /* nincs lefoglalva */
        if(!vmm_ispresent(ptr1[i])) {
            bss +=1UL<<30;
            continue;
        }
        /* 2M */
        *ctrl2 = vmm_phyaddr(ptr1[i])|PG_CORE_RWNOCACHE|PG_PAGE;
        vmm_invalidate(LDYN_tmpmap1);
        je = (end>>21)==(bss>>21)? (bss>>21&511) : 511;
        for(j = ((start>>21)==(bss>>21)? (bss>>21&511) : 0); j<=je && bss<end; j++) {
            /* nincs lefoglalva */
            if(!vmm_ispresent(ptr2[j])) {
                bss += __SLOTSIZE;
                continue;
            }
            /* 2M-es blokk */
            if(vmm_isslot(ptr2[j])) {
                if(ptr2[j]&PG_SHARED)
                    tcb->linkmem -= __SLOTSIZE/__PAGESIZE;
                else
                    pmm_free(tcb, vmm_phyaddr(ptr2[j]), __SLOTSIZE/__PAGESIZE);
                bss +=__SLOTSIZE;
                ptr2[j] = 0;
                continue;
            }
            /* 4K */
            *ctrl3 = vmm_phyaddr(ptr2[j])|PG_CORE_RWNOCACHE|PG_PAGE;
            vmm_invalidate(LDYN_tmpmap2);
            ke = (end>>21)==(bss>>21)?(end>>__PAGEBITS&511) : 511;
            for(k = ((start>>__PAGEBITS)==(bss>>__PAGEBITS)? (bss>>__PAGEBITS&511) : 0); k<=ke && bss<end; k++) {
                bss += __PAGESIZE;
                /* nincs lefoglalva */
                if(!vmm_ispresent(ptr3[k])) continue;
                if(ptr3[k]&PG_SHARED)
                    tcb->linkmem--;
                else
                    pmm_free(tcb, vmm_phyaddr(ptr3[k]), 1);
                ptr3[k] = 0;
            }
            /* van más 4K-s bejegyzés ezen a 2M-nyi táblán? */
            for(n=0,k=0; k<512; k++) if(vmm_ispresent(ptr3[k])) { n=1; break; }
            if(!n) {
                pmm_free(tcb, vmm_phyaddr(ptr2[j]), 1);
                ptr2[j] = 0;
            }
        }
        /* az 1G-s táblát csak akkor szabadítjuk fel, ha nem megosztott memória */
        if(end<SDYN_ADDRESS) {
            /* van más 2M-s bejegyzés ezen az 1G-nyi táblán? */
            for(n=0,j=0; j<512; j++) if(vmm_ispresent(ptr2[j])) { n=1; break; }
            if(!n) {
                pmm_free(tcb, vmm_phyaddr(ptr1[i]), 1);
                ptr1[i] = 0;
            }
        }
    }
}

/**
 * virtuális cimtér megszüntetése, és a hozzá tartozó fizikai RAM felszabadítása
 */
void vmm_del(tcb_t *tcb)
{
    phy_t *ptr  = (phy_t*)LDYN_tmpmap2;
    tcb_t *tcb2 = (tcb_t*)LDYN_tmpmap3;

    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH)
        return;
#if DEBUG
    if(debug&DBG_VMM)
        kprintf("  vmm_del(pid=%d,m=%x)\n", tcb->pid, tcb->memroot);
#endif
    /* leképezzük a TCB-t a core memóriába, mivel azalól is kihúzzuk a leképezést */
    vmm_page(0, LDYN_tmpmap3, tcb->pid << __PAGEBITS, PG_CORE_RWNOCACHE | PG_PAGE);
    /* felszabadítunk mindent 1G-s lapméretig */
    vmm_free(tcb2, 0, BUF_ADDRESS+DYN_ADDRESS);
    /* a legvégén a TCB-hez vezető utat (512G-s lapig, ha van) és a leképező fa gyökerét is felszabadítjuk */
    vmm_page(0, LDYN_tmpmap2, tcb2->memroot, PG_CORE_RWNOCACHE | PG_PAGE);
    while((ptr[0]&PG_PAGE)) {
        pmm_free(tcb2, vmm_phyaddr(ptr[0]), 1);
        vmm_page(0, LDYN_tmpmap2, vmm_phyaddr(ptr[0]), PG_CORE_RWNOCACHE | PG_PAGE);
    }
    pmm_free(tcb2, vmm_phyaddr(tcb2->memroot), 1);
#if DEBUG
    if(tcb2->linkmem || tcb2->allocmem)
        kpanic("vmm_del(pid=%x): memleak, alloc %d link %d", tcb2->pid, tcb2->allocmem, tcb2->linkmem);
#endif
}
