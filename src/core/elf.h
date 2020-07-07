/*
 * core/elf.h
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
 * @subsystem futtathatók
 * @brief ELF betöltő és értelmező
 */

#ifndef _AS

typedef struct {
    char *fn;
    phy_t *data;
    uint64_t idx;
    uint32_t numpages;
    uint32_t numref;
} elfcache_t;

typedef struct {
    uint64_t elfcacheidx;
    virt_t addr;
    virt_t end;
    virt_t entry;
} elfbin_t;

/* az ELF betöltő és a futási idejű linkelő kontextusa */
typedef struct {
    virt_t nextaddr;
    virt_t extra_ptr;
    virt_t env_ptr;
    uint64_t nelfbin;
    elfbin_t *elfbin;
    char libpath[128];
} elfctx_t;

elfcache_t *elf_getfile(char *fn);                          /* betölt egy ELF objektumot a háttértárolóból a gyorsítótárba */
bool_t elf_load(char *fn, phy_t eb, size_t es, uint8_t re); /* betölt egy ELF objektumot a gyorsítótárból az aktuális címtérbe */
void elf_unload(elfbin_t *elfbin);                          /* kiszedi au ELF objektumokat az aktuális címtérből */
bool_t elf_rtlink(pmm_entry_t *devspec);                    /* aktuális címtérben összelinkeli az ELF-eket (run-time linker) */

#endif
