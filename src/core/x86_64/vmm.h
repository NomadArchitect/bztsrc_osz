/*
 * core/x86_64/vmm.h
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

/* VMM hozzáférési bitek */
#define PG_CORE_RW          0x0003
#define PG_CORE_RWNOCACHE   0x001b
#define PG_CORE_RO          0x0001
#define PG_CORE_RONOCACHE   0x0019
#define PG_USER_RW          0x0007
#define PG_USER_RWNOCACHE   0x0017
#define PG_USER_RO          0x0005
#define PG_USER_RONOCACHE   0x001d
#define PG_USER_DRVMEM      0x001f
#define PG_PAGE             0x0001  /* 4k felbontás */
#define PG_SLOT             0x0081  /* 2M felbontás */
#define PG_SHARED           (1<<11) /* megosztott lap */
#define PG_NX_BIT           63

/* lapozási táblákat kezelő makrók */
#define vmm_phyaddr(x) ((phy_t)(x)&~((0xFFUL<<56)|0xFFF))
#define vmm_switch(m) __asm__ __volatile__("movq %0, %%cr3" : : "a"(m) : "memory")
#define vmm_invalidate(p) __asm__ __volatile__ ("invlpg (%0)" : : "r" (p) : "memory");
#define vmm_ispresent(p) (p&1)
#define vmm_isslot(p) (p&0x80)
#define vmm_wnx(a) (access&2?(1UL<<PG_NX_BIT):0)
#define vmm_notpresent(x) (!((x)&1))
#define vmm_stack __asm__ __volatile__ ("movq %0, %%rsp" : : "r"(LDYN_ccb + __PAGESIZE) : "rsp", "memory");
/* 512G-s tábla betöltése, vmm_page() használja */
#define vmm_pageprologue if(!memroot) { __asm__ __volatile__("mov %%cr3, %0" : "=a"(memroot)); } \
    *ctrl = vmm_phyaddr(memroot)|PG_CORE_RWNOCACHE|PG_PAGE; vmm_invalidate(LDYN_tmpctrl); \
    memroot = ((uint64_t*)LDYN_tmpctrl)[((virt)>>39)&511]; if(!(memroot&1)) goto err
/* 512G-s tábla betöltése és a megfelelő tcb kiválasztása, vmm_*() függvények használják */
#define vmm_getmemroot(p) if((virt_t)(p)>=SDYN_ADDRESS) tcb = &idle_tcb; \
    *((uint64_t*)LDYN_tmpctrlptr) = vmm_phyaddr(tcb->memroot)|PG_CORE_RWNOCACHE|PG_PAGE; \
    vmm_invalidate(LDYN_tmpctrl); memroot = ((uint64_t*)LDYN_tmpctrl)[((virt_t)(p)>>39)&511]
/* vezérlés átadása az adott címtérre */
#define vmm_enable(t) __asm__ __volatile__( \
        "movq %0, %%rsp; xor %%rbp, %%rbp; iretq" : : \
        "a"(&(t)->pc) : "rsp","rbp", "memory")
