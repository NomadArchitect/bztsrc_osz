/*
 * include/osZ/limits.h
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
 * @brief ISO C99 Sztandard: 7.10/5.2.4.2.1 Rendszer limitációk és méretek
 */

#ifndef _LIMITS_H_
#define _LIMITS_H   1

#define TMP_MAX         8
#define FILENAME_MAX    216                         /* 256 - sizeof(dirent header) */
#define TCB_MAXACE      128                         /* maximum ennyi ACE lehet egy taszkon */

#define __WORDSIZE      64
#define __PAGEBITS      12
#define __PAGESIZE      (1UL<<__PAGEBITS)
#define __SLOTSIZE      (__PAGESIZE*__PAGESIZE/8)
#define __BUFFSIZE      (__SLOTSIZE/2)

/* memória értékek, OS/Z specifikus címek */
#define TCB_ADDRESS     0                           /* aktuális címtér Taszk Kontrol Blokkja */
#define TEXT_ADDRESS    (2*__SLOTSIZE)              /* 4M kódszegmens, egyben a verem teteje */
#define DYN_ADDRESS     (0x0000000100000000UL)      /* 4G dinamikus adatszegmens, lásd bztalloc.c */
#define BUF_ADDRESS     (0x0000008000000000UL-DYN_ADDRESS) /* 512G-4G rendszer bufferek */
#define SDYN_ADDRESS    (0xFFFFFF8000000000UL)      /* -512G megosztott memória, lásd bztalloc.c */

#define KBYTE           1024
#define MBYTE           (1024*1024)
#define GBYTE           (1024*1024*1024)

#define CHAR_BIT        8
#define SCHAR_MIN       (-128)
#define SCHAR_MAX       127
#define UCHAR_MAX       255
#define CHAR_MIN        SCHAR_MIN
#define CHAR_MAX        SCHAR_MAX
#define SHRT_MIN        (-32768)
#define SHRT_MAX        32767
#define USHRT_MAX       65535
#define INT_MIN         (-INT_MAX - 1)
#define INT_MAX         2147483647
#define UINT_MAX        4294967295U
#define LONG_MAX        9223372036854775807L
#define LONG_MIN        (-LONG_MAX - 1L)
#define ULONG_MAX       18446744073709551615UL

#define RAND_MAX        ULONG_MAX                   /* a legnagyobb érték, amit a rand() vissza tud adni.  */

#endif /* limits.h */
