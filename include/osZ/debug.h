/*
 * include/osZ/debug.h
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
 * @brief OS/Z Debug jelzők
 */

#ifndef _DEBUG_H
#define _DEBUG_H 1

#define DBG_NONE     0          /* nincs üzenet */
#define DBG_PROMPT   (1<<0)     /* pr */
#define DBG_MEMMAP   (1<<1)     /* mm */
#define DBG_TASKS    (1<<2)     /* ta */
#define DBG_ELF      (1<<3)     /* el */
#define DBG_RTIMPORT (1<<4)     /* ri */
#define DBG_RTEXPORT (1<<5)     /* re */
#define DBG_IRQ      (1<<6)     /* ir */
#define DBG_DEVICES  (1<<7)     /* de */
#define DBG_SCHED    (1<<8)     /* sc */
#define DBG_MSG      (1<<9)     /* ms */
#define DBG_LOG      (1<<10)    /* lo */
#define DBG_PMM      (1<<11)    /* pm */
#define DBG_VMM      (1<<12)    /* vm */
#define DBG_MALLOC   (1<<13)    /* ma */
#define DBG_BLKIO    (1<<14)    /* bl */
#define DBG_FILEIO   (1<<15)    /* fi */
#define DBG_FS       (1<<16)    /* fs */
#define DBG_CACHE    (1<<17)    /* ca */
#define DBG_UI       (1<<18)    /* ui */
#define DBG_TESTS    (1<<19)    /* te */

#endif /* debug.h */
