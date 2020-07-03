/*
 * core/vmm.h
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
 * @brief Virtuális Memória Kezelő
 */

/*** Core Memória Leképezés ***/
#define LDYN_ADDRESS    0xffffffff80000000                  /* cpu lokális dinamikus memória -2G */
#define CCBS_ADDRESS    0xffffffffc0000000                  /* cpu globális CPU Kontrol Blokkok -1G */
#define CDYN_ADDRESS    (CCBS_ADDRESS+(((MAXCPUS*__PAGESIZE)+__SLOTSIZE-1)/__SLOTSIZE)*__SLOTSIZE) /* cpu globális memória -1G+4M */
#define CDYN_TOP        0xfffffffff8000000                  /* a dinamikus core memória felső határa */
#define FBUF_ADDRESS    0xfffffffffc000000                  /* framebuffer leképezés */
#define CORE_ADDRESS    0xffffffffffe02000                  /* core text szegmens */

/*** Átmeneti leképezések címei ***/
#define LDYN_1G         (LDYN_ADDRESS+2*__PAGESIZE+((LDYN_ADDRESS>>30) & 511)*8)
#define LDYN_2M         (LDYN_ADDRESS+3*__PAGESIZE)
#define LDYN_4K         (LDYN_ADDRESS+4*__PAGESIZE)
#define LDYN_ccb        (LDYN_ADDRESS)                      /* aktuális CCB, ennek a címnek kell a gdt-ben a tss-nél lennie */
#define LDYN_ccbptr     (LDYN_4K+0)
#define LDYN_tcbalarm   (LDYN_ADDRESS+__PAGESIZE)           /* következő TCB az ébresztési sorban */
#define LDYN_tcbalrmptr (LDYN_4K+8)
#define LDYN_usrccb     (LDYN_ADDRESS+5*__PAGESIZE)         /* aktuális CCB felhasználói szintről olvashatóan (idle-nek) */
#define LDYN_usrccbptr  (LDYN_4K+5*8)
#define LDYN_cpuclk     (LDYN_ADDRESS+6*__PAGESIZE)         /* CPU-nkénti időzítő MMIO */
#define LDYN_cpuclkptr  (LDYN_4K+6*8)
#define LDYN_tmpctrl    (LDYN_ADDRESS+7*__PAGESIZE)         /* átmeneti leképezés kontrollapja */
#define LDYN_tmpctrlptr (LDYN_4K+7*8)
#define LDYN_tmpmap1    (LDYN_ADDRESS+8*__PAGESIZE)         /* átmenetileg leképzett lap #1 (pmm_alloc használja) */
#define LDYN_tmpmap1ptr (LDYN_4K+8*8)
#define LDYN_tmpmap2    (LDYN_ADDRESS+9*__PAGESIZE)         /* átmenetileg leképzett lap #2 */
#define LDYN_tmpmap2ptr (LDYN_4K+9*8)
#define LDYN_tmpmap3    (LDYN_ADDRESS+10*__PAGESIZE)        /* átmenetileg leképzett lap #3 */
#define LDYN_tmpmap3ptr (LDYN_4K+10*8)
#define LDYN_tmpmap4    (LDYN_ADDRESS+11*__PAGESIZE)        /* átmenetileg leképzett lap #4 */
#define LDYN_tmpmap4ptr (LDYN_4K+11*8)
#define LDYN_tmpmap5    (LDYN_ADDRESS+12*__PAGESIZE)        /* átmenetileg leképzett lap #5 */
#define LDYN_tmpmap5ptr (LDYN_4K+12*8)
#define LDYN_tmpmap6    (LDYN_ADDRESS+13*__PAGESIZE)        /* átmenetileg leképzett lap #6 */
#define LDYN_tmpmap6ptr (LDYN_4K+13*8)
#define LDYN_tmpmap7    (LDYN_ADDRESS+14*__PAGESIZE)        /* átmenetileg leképzett lap #7 */
#define LDYN_tmpmap7ptr (LDYN_4K+14*8)
#define LDYN_tmpslot    (LDYN_ADDRESS+__SLOTSIZE)           /* átmenetileg leképzett blokk */
#define LDYN_tmpslotptr (LDYN_2M+8)

#ifndef _AS

/*** Platform függő funkciók ***/
void vmm_init();                                                                    /* lapozás inicializálása */
tcb_t *vmm_new(uint16_t cpu, uint8_t priority);                                     /* új címtér létrehozása */
/*** Platform független funkciók ***/
void vmm_page(virt_t memroot, virt_t virt, phy_t phys, uint64_t access);            /* egy lap leképezése a címtérbe */
void *vmm_map(tcb_t *tcb, virt_t bss, phy_t phys, size_t size, uint64_t access);    /* egy adatbbuffer leképezése a címtérbe */
void *vmm_maptext(tcb_t *tcb, virt_t bss, phy_t *phys, size_t size);                /* kódszegmens leképezése */
void vmm_swaptextbuf(tcb_t *tcb);                                                   /* kódszegmens és kódbuffer felcserélése */
void vmm_free(tcb_t *tcb, virt_t bss, size_t size);                                 /* leképezés felszabadítása */
void vmm_del(tcb_t *tcb);                                                           /* komplett címtér megszüntetése */

#endif
