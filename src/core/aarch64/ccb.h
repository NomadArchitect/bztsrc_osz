/*
 * core/aarch64/ccb.h
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
 * @brief CPU Kontrol Blokk, architektúra függő struktúra
 */

/* asmhez, egyeznie kell a cbb mezőivel */
#define ccb_flags       0x10
#define ccb_hasatask    0x14
#define ccb_core_errno  0x18

/* prioritási szintekért lásd src/core/task.h */
#ifndef _AS
typedef struct {
    uint32_t magic;         /* +00 CPU Kontrol Blokk azonosító 'CPUB' */
    uint32_t id;            /* +04 CPU mag sorszám */
    phy_t sharedroot;       /* +08 megosztott memória címe, 512G, ttbr1_el1 */
    uint32_t flags;         /* +10 taszkkapcsolás jelzők */
    uint32_t hasactivetask; /* +14 van futtatható taszkja */
    uint64_t core_errno;    /* +18 core hibakód */
    pid_t current;          /* +20 futásra kijelölt taszk */
    pid_t hd_timerq;        /* +28 időzítő sor fej (alarm) */
    pid_t hd_blocked;       /* blokkolt sor fej */
    pid_t hd_active[8];     /* prioritási sorok (aktív taszk fejek) */
    pid_t cr_active[8];     /* prioritási sorok (aktuális taszkok) */
    uint64_t sched_ticks;   /* ütemező számláló a következő riasztáshoz */
    uint64_t sched_delta;   /* ütemező számláló növelése */
    uint64_t numtasks;      /* erre a CPU-ra ütemezett taszkok száma */
    /* a fennmaradó részt a rendszer hívás használja CPU-nkénti veremnek */
} __attribute__((packed)) ccb_t;

c_assert(sizeof(ccb_t) <= __PAGESIZE - CPUSTACK);

#endif
