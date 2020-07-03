/*
 * drivers/include/driver.h
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
 * @subsystem eszközmeghajtók
 * @brief OS/Z eszköz kezelő szolgáltatások
 */

#ifndef _DRV_DRIVER_H
#define _DRV_DRIVER_H 1

/* üzenetkódok */
#define DRV_IRQ         (0)                                 /* ezt csak a core küldheti */
#define DRV_open        (1)                                 /* többit meg csak az FS taszk */
#define DRV_close       (2)
#define DRV_read        (3)
#define DRV_write       (4)
#define DRV_ioctl       (5)

/* memóriakiosztás */
#define MMIO_BASE       (BUF_ADDRESS)                       /* MMIO terület leképezési címe */
#define DEVSPEC_BASE    (BUF_ADDRESS + (DYN_ADDRESS>>1))    /* eszközspecifikáció címe */

#if !defined(_AS) && !defined(_OSZ_CORE_)

#ifndef _DRIVER_C
#include <platform.h>                                       /* B/K portok definíciói */
#include <env.h>                                            /* indulási környezet */
#endif

typedef struct {
    uint64_t base;                                          /* fizikai MMIO cím */
    uint64_t size;                                          /* méret */
} mem_entry_t;

/*** libc prototípusok */
extern void core_regirq(uint16_t irq);                                              /* IRQ üzenetek kérése */
extern void core_regtmr();                                                          /* timer üzenet kérése */
extern void core_drvfind(char *spec, char *drv, int len);                           /* meghajtóprogram keresése */
extern void core_drvadd(char *drv, mem_entry_t *memspec);                           /* meghajtóprogram hozzáadása */
extern dev_t mknod(const char *devname, mode_t mode, blksize_t size, blkcnt_t cnt); /* eszköz fájl létrehozása */

/*** eszközmeghajtók által implementált funkciók ***/
extern int  drv_init();                                     /* inicializálás, 1-el tér vissza, ha sikerült */
extern void drv_irq(uint16_t irq, uint64_t ticks);          /* regirq() vagy regtmr() esetén hívódik, utóbbinál irq == USHRT_MAX */
extern void drv_open(dev_t device, uint64_t mode);          /* akkor hívódik, ha valaki megnyitja a mknod() által kreált fájlt */
extern void drv_close(dev_t device);                        /* amikor bezárják */
extern void drv_read(dev_t device);                         /* olvasás az eszközfájlból */
extern void drv_write(dev_t device);                        /* írás az eszközfájlba */
extern void drv_ioctl(dev_t device);                        /* eszközparancs */

#endif

#endif /* driver.h */
