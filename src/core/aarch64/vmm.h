/*
 * core/aarch64/vmm.h
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

#define XLAT_OSH (2<<8)
#define XLAT_ISH (3<<8)
#define XLAT_RW  (0<<7)
#define XLAT_RO  (1<<7)
#define XLAT_CORE (0<<6)
#define XLAT_USER (1<<6)
#define XLAT_NS  (1<<5)
#define XLAT_MEM (0<<2)
#define XLAT_DEV (1<<2)
#define XLAT_NC  (2<<2)

/* VMM hozzáférési bitek */
#define PG_CORE_RW          0x0700  /* nG=0,AF=1,SH=3,AP=0,NS=0,Attr=0 */
#define PG_CORE_RWNOCACHE   0x0600  /* nG=0,AF=1,SH=2,AP=0,NS=0,Attr=0 */
#define PG_CORE_RO          0x0780  /* nG=0,AF=1,SH=3,AP=2,NS=0,Attr=0 */
#define PG_CORE_RONOCACHE   0x0680  /* nG=0,AF=1,SH=2,AP=3,NS=0,Attr=0 */
#define PG_USER_RW          0x0740  /* nG=0,AF=1,SH=3,AP=1,NS=0,Attr=0 */
#define PG_USER_RWNOCACHE   0x0640  /* nG=0,AF=1,SH=2,AP=1,NS=0,Attr=0 */
#define PG_USER_RO          0x07c0  /* nG=0,AF=1,SH=3,AP=3,NS=0,Attr=0 */
#define PG_USER_RONOCACHE   0x06c0  /* nG=0,AF=1,SH=2,AP=3,NS=0,Attr=0 */
#define PG_USER_DRVMEM      0x0644  /* nG=0,AF=1,SH=2,AP=1,NS=0,Attr=1 */
#define PG_PAGE             0x0003  /* 4k felbontás */
#define PG_SLOT             0x0001  /* 2M felbontás */
#define PG_SHARED           (1UL<<58) /* megosztott lap */
#define PG_NX_BIT           54

/* lapozási táblákat kezelő makrók */
#define vmm_phyaddr(x) ((phy_t)(x)&~((0xFFFFUL<<48)|0xFFF))
#define vmm_switch(m) __asm__ __volatile__("msr ttbr0_el1, %0;tlbi vmalle1;dsb ish;isb"::"r"(m|1):"memory")
#define vmm_invalidate(p) __asm__ __volatile__ ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish" : : \
    "r" ((uint64_t)(p)>>__PAGEBITS), "r"(p) : "memory");
#define vmm_ispresent(p) (p&1)
#define vmm_isslot(p) (!(p&2))
#define vmm_wnx(a) (access&0x80?0:(1UL<<PG_NX_BIT))
#define vmm_notpresent(x) ((((x)>>2)&3) != 3)
#define vmm_stack __asm__ __volatile__ ("mov sp, %0" : : "r"(LDYN_ccb + __PAGESIZE) : "memory");
/* 512G-s tábla betöltése, vmm_page() használja */
#define vmm_pageprologue if(virt>>48) __asm__ __volatile__ ("mrs %0, ttbr1_el1" : "=r" (memroot)); \
            else if(!memroot) __asm__ __volatile__ ("mrs %0, ttbr0_el1" : "=r" (memroot))
/* 512G-s tábla betöltése és a megfelelő tcb kiválasztása, vmm_*() függvények használják */
#define vmm_getmemroot(p) if((virt_t)(p)<SDYN_ADDRESS) { memroot = tcb->memroot; } else { \
        __asm__ __volatile__ ("mrs %0, ttbr1_el1" : "=r" (memroot)); tcb = &idle_tcb; }
/* vezérlés átadása az adott címtérre */
#define vmm_enable(t) __asm__ __volatile__( \
        "mov x29, xzr; msr spsr_el1, xzr; msr elr_el1, %0; msr sp_el0, %1; mov sp, #4096; eret" : : \
        "r"((t)->pc), "r"((t)->sp) : "memory")
