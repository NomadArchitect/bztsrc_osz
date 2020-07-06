/*
 * core/core.h
 *
 * Copyright (c) 2016 bzt (bztsrc@gitlab)
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
 * @brief Core funkciók (ring 0 / EL1)
 */

#ifndef _OSZ_CORE_H
#define _OSZ_CORE_H

/*** Konfigurálás ***/
#define PHYMEM_MIN      32 /* Mb, minimálisan szükséges RAM */
#define MAXCPUS       1024 /* max CPU-k száma, __PAGESIZE/8 többszöröse. BOOTBOOT maximum ennyit tud jelenleg */
#define CPUSTACK      3584 /* CPU-nkénti verem mérete, lapméret - sizeof(ccb_t) maximum */
#define TRACEBUFSIZ    256 /* a nyomkövetési buffer mérete */

/*** Sztandard headerök ***/
#include <osZ/errno.h>                              /* hibakódok */
#include <osZ/limits.h>                             /* limitek és memóriacímek */
#include <osZ/stdint.h>                             /* szabvány változótípusok */
#include <osZ/stdarg.h>                             /* változó számú paraméterlista */
#include <osZ/types.h>                              /* rendszer által definiált változótípusok */
#include <osZ/syscall.h>                            /* rendszerszolgáltatások definíciói */
#include <osZ/debug.h>                              /* debug jelzők */
#include <osZ/syslog.h>                             /* rendszernapló prioritások */
#ifndef _AS
#include "../../loader/bootboot.h"                  /* rendszerbetöltő paraméterei */
#include "../libc/bztalloc.h"                       /* memóriaallokátor */
#endif
#include "lang.h"                                   /* nyelvi fordítások kezelése */
#include "env.h"                                    /* környezeti változók */
#include "task.h"                                   /* Taszk Kontroll Blokk */
#include "pmm.h"                                    /* fizikai memóriakezelő */
#include "vmm.h"                                    /* virtuális memóriakezelő */
#include "elf.h"                                    /* futtaható fájlok kezelése */
#include "fault.h"                                  /* platform független kivételkezelők */
#include "msg.h"                                    /* üzenetküldés */

#define bssalign __attribute__((section(".bss.aligned")))

/*** Futási szintek ***/
#define RUNLVL_BOOT     0                           /* korai szakasz, egy-az-egy címleképezés */
#define RUNLVL_VMEM     1                           /* korai szakasz, virtuális címtér leképezés */
#define RUNLVL_COOP     2                           /* kooperatív multitaszk, eszközkezelő inicializálás */
#define RUNLVL_NORM     3                           /* normál működés, pre-emptív ütemezés */
#define RUNLVL_SHDN     4                           /* leállítás vagy újraindítás */

/*** Azonosító bájtok ***/
#define OSZ_CCB_MAGIC "CPUB"                        /* CPU Kontrol Blokk */
#define OSZ_CCB_MAGICH 0x42555043

#define OSZ_TCB_MAGIC "TASK"                        /* Taszk Kontrol Blokk */
#define OSZ_TCB_MAGICH 0x4B534154

#define OSZ_LANG_MAGIC "LANG"                       /* nyelvi fájl */
#define OSZ_LANG_MAGICH 0x474E414C

#define OSZ_FONT_MAGIC "SFN2"                       /* konzol font */
#define OSZ_FONT_MAGICH 0x324E4653

#ifndef _AS

/*** Virtuális címek importálása a linkelő szkriptből ***/
extern BOOTBOOT bootboot;                           /* bootboot struktúra */
extern unsigned char environment[__PAGESIZE];       /* környezeti változók */

/*** korai konzol és debugger font ***/
typedef struct {
    unsigned char  magic[4];                        /* SFN2 */
    unsigned int   size;                            /* font teljes mérete */
    unsigned char  type;                            /* típus, monospace-nek kell lennie */
    unsigned char  features;                        /* nem használjuk */
    unsigned char  width;                           /* a font szélessége */
    unsigned char  height;                          /* a font magassága */
    unsigned char  baseline;                        /* az alapvonal poziciója */
    unsigned char  underline;                       /* az aláhúzás poziciója */
    unsigned short fragments_offs;                  /* glif darabkák címe */
    unsigned int   characters_offs;                 /* UNICODE - glif leképező tábla */
    unsigned int   ligature_offs;                   /* ligatúrák, nem használjuk */
    unsigned int   kerning_offs;                    /* kerning, nem használjuk */
    unsigned int   cmap_offs;                       /* paletta, nem használjuk */
} __attribute__((packed)) sfn_t;
extern sfn_t *font;

/* ---- Általános adatok ----- */
extern uint8_t runlevel;                            /* futási szint */
extern pid_t services[NUMSRV];                      /* rendszerszolgáltatások taszkjai */
extern uint32_t numcores;                           /* a CPU magok száma */
extern uint64_t srand[4], systables[8];             /* véletlenszámgenerátor és rendszertáblák */
extern uint64_t clock_ts, clock_ns, clock_ticks;    /* pontos idő másodperc, nanoszek és az indulás óta eltelt mikroszek */
extern uint64_t clock_freq;                         /* pontos idő frissítési frekvenciája */
extern int16_t clock_tz;                            /* aktuális időzóna percekben */
extern uint16_t sched_irq, clock_irq;               /* megszakítási vonalak */
extern tcb_t idle_tcb;                              /* rendszer taszk, üresjárat */
extern virt_t fs_initrd;                            /* az initrd virtuális címe a core területén */
extern uint64_t __stack_chk_guard;                  /* gcc-nek kell a stack canary-hoz */

/*** Függvény prototípusok ***/
/* ---- Alacsony szintű könyvtár (nincs libc) ----- */
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t mbstrlen(const char *s);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strncpy(char *dst, const char *src, size_t n);
char *sprintf(char *dst, char *fmt, ...);
char *vsprintf(char *dst, char *fmt, va_list args);
void kentropy();                                    /* entrópia növelése */
void __stack_chk_guard_setup();
void __stack_chk_fail();

/* ----- Szinkronizálás ----- */
void lockacquire(int bit, uint64_t *ptr);           /* addig vár, míg nem sikerül lefoglalnia a jelzőt */
void lockrelease(int bit, uint64_t *ptr);           /* jelző felszabadítása */

/* ----- Naplózás ----- */
void syslog(int pri, char* fmt, ...);               /* rendszernapló írása */

/* ----- Boot konzol ----- */
void kprintf_init();                                /* konzol inicializálása a kprintf és kpanic függvényekhez */
void kprintf_reset();                               /* alapértelmezett szinek és a kurzor bal felső sarokba mozgatása */
void kprintf_fg(uint32_t color);                    /* szinek beállítása */
void kprintf_bg(uint32_t color);
void kprintf_scrolloff();                           /* szkrollozás kikapcsolása */
void kprintf(char* fmt, ...);                       /* formázott szöveg kiírása a konzolra */
void kpanic(char *reason, ...);                     /* ok megjelenítése és leállás, vagy debugger indítása */
/** Viszlát képernyők */
void kprintf_poweroff();                            /* számítógép kikapcsolása */
void kprintf_reboot();                              /* számítógép újraindítása */

/* ----- I18N ----- */
void lang_init();                                   /* szótárfájl betöltése *txt[]-be a lang[]-hoz */

/* ----- Fájlrendszer ----- */
extern uint64_t fs_size;                            /* fájl keresése az initrd-n, visszaadja a fizikai memória címet, */
void *fs_locate(char *fn);                          /* a fájl méretét pedig a fs_size változóban */

/* ----- Eszközkezelők és rednszerszolgáltatások ----- */
void drivers_init();                                /* beépített eszközmeghajtók inicializálása, megszakítások konfigurálása */
void drivers_regintr(uint16_t irq, pid_t task);     /* megszakításkezelő taszk regisztrálása */
void drivers_unregintr(pid_t task);                 /* megszakításkezelő taszk eltávolítása a listából */
void drivers_intr(uint16_t irq);                    /* üzenet küldése a regisztrált megszakításkezelő taszk(ok)nak */
char *drivers_find(char *spec);                     /* visszaadja az eszközspec-hez tartozó meghajtó fájlnevét */
void drivers_add(char *cmd, pmm_entry_t *devspec);  /* új eszközmeghajtó taszk hozzáadása */
void service_add(int srv, char *cmd);               /* új rendszer szolgáltatási taszk hozzáadása */
void drivers_start();                               /* kooperatív multitaszk indítása, eszközmeghajtók inicializálása */
void drivers_ready();                               /* akkor hívódik, ha minden eszközmeghajtó inicializálódott */

/* ----- Falióra ----- */
void clock_init();                                  /* falióra inicializálása */
void clock_intr(uint16_t irq);                      /* falióra megszakításkezelője */
void clock_ack();                                   /* hardver visszaigazolás a falióra lekezeltségéről */

/* ----- Ütemező ----- */
void sched_intr(uint16_t irq);                      /* ütemező megszakításkezelője */
void sched_add(tcb_t *tcb);                         /* taszk hozzáadása az ütemező prioritási sorához */
void sched_remove(tcb_t *tcb);                      /* taszk kivétele a prioritási sorból */
void sched_awake(tcb_t *tcb);                       /* taszk felébresztése */
void sched_block(tcb_t *tcb);                       /* taszk altatása */
void sched_hybernate(tcb_t *tcb);                   /* taszk háttértárra küldése */
void sched_delay(tcb_t *tcb, uint64_t usec);        /* megadott mikroszekundumig altatja a taszkot */
void sched_migrate(tcb_t *tcb, uint16_t cpuid);     /* taszk átköltöztetése másik processzormagra */
void sched_pick();                                  /* a következő taszk kiválasztása futásra */
#if DEBUG
void sched_dump(uint16_t cpuid);                    /* prioritási sorok dumpolása, ha cpuid==-1 akkor minden CPU-n */
#endif

/* ----- Memóriakezelés ----- */
/**
 * fizikai memória lefoglalása és leképezése a kernel memóriába
 *
 * void *kalloc(size_t s)
 */
#define kalloc(s) bzt_alloc((void*)CDYN_ADDRESS,8,NULL,s,MAP_PRIVATE)
/**
 * üres "lyuk" lefoglalása a kernel memóriába
 *
 * void *kalloc_gap(size_t s)
 */
#define kalloc_gap(s) bzt_alloc((void*)CDYN_ADDRESS,8,NULL,s,MAP_PRIVATE|MAP_SPARSE)
/**
 * kernel memória átméretezése
 *
 * void *krealloc(void *p, size_t s)
 */
#define krealloc(p,s) bzt_alloc((void*)CDYN_ADDRESS,8,p,s,MAP_PRIVATE)
/**
 * kernel memória felszabadítása
 *
 * void kfree(void *p)
 */
#define kfree(p) bzt_free((void*)CDYN_ADDRESS,p)
/**
 * fizikai memória lefoglalása és leképezése a megosztott kernel memóriába
 *
 * void *ksalloc(void *p, size_t s)
 */
#define ksalloc(s) bzt_alloc((void*)SDYN_ADDRESS,8,NULL,s,MAP_CORE)
/**
 * megosztott kernel memória átméretezése
 *
 * void *ksrealloc(void *p, size_t s)
 */
#define ksrealloc(p,s) bzt_alloc((void*)SDYN_ADDRESS,8,p,s,MAP_CORE)
/**
 * megosztott kernel memória felszabadítása
 *
 * void ksfree(void *p)
 */
#define ksfree(p) bzt_free((void*)SDYN_ADDRESS,p)

/* ----- Debugger ----- */
#if DEBUG
#define _debug debug                                /* libc változó megfelelője a core-ban */
#define dbg_printf kprintf                          /* libc függvény megfelelője a core-ban */
void dbg_start(char *header, bool_t ispanic);       /* a beépített debugger engedélyezése */
void dbg_putchar(uint16_t c);                       /* egy karakter küldése a debug konzolra (unicode) */
#endif

/* ----- Platform ----- */
/*** Ezeket a funkciókat a platform specifikus rész implementálja ***/
void platform_cpu();                                /* CPU ellenőrzése és inicializálása */
bool_t platform_memfault(void *addr);               /* eldönti egy címről, elérhető-e, lehet, hogy nincs leképezve */
void platform_srand();                              /* a véletlenszám generátor inicializálása */
void platform_env();                                /* alapértelmezett környezeti értékek beállítása, env_init() hívja */
void platform_poweroff();                           /* számítógép kikapcsolása, a kprintf_poweroff() hívja */
void platform_reset();                              /* számítógép újraindítása, a kprintf_reset() hívja */
void platform_halt();                               /* futtatás felfüggesztése, komplett stop */
void platform_idle();                               /* üresjárat, csak megszakításra léphet tovább, az idle taszk kódja */
void platform_awakecpu(uint16_t id);                /* adott processzort felébreszti az üresjáratból */
void platform_dbginit();                            /* a debug konzol (soros port) inicializálása */
void platform_dbgputc(uint8_t c);                   /* egy bájt kiírása a debug konzolra (ascii) */
uint64_t platform_waitkey();                        /* billentyűleütésre várakozás, kprintf és a debugger használja */
/* megszakításvezérlő és időzítók funkciói */
uint intr_init();                                   /* megszakításkezelők és vezérlő inicializálása */
void intr_enable(uint16_t irq);                     /* hardveres megszakítás engedélyezése */
void intr_disable(uint16_t irq);                    /* hardveres megszakítás letiltása */
void intr_nextsched(uint64_t usec);                 /* generál egy megszakítást usec mikroszekundum múlva az ütemezéshez */
bool_t intr_nextschedremainder();                   /* ha az időzítőnek több megszakítás is kell az nsec eléréséhez */

#endif

#endif /* _OSZ_CORE_H */
