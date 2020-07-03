/*
 * core/lang.h
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
 * @brief Nyelvi fordítások szótára
 */

/* core fordítások */
#define TXT_asktime txt[0]
#define TXT_initrdtoocomplex txt[1]
#define TXT_notsupported txt[2]
#define TXT_filenametoolong txt[3]
#define TXT_missing txt[4]
#define TXT_sharedmissing txt[5]
#define TXT_panic txt[6]
#define TXT_rebooting txt[7]
#define TXT_shutdownfinished txt[8]
#define TXT_turnoff txt[9]
#define TXT_restart txt[10]
#define TXT_pressakey txt[11]
#define TXT_shouldnothappen txt[12]
#define TXT_outofram txt[13]
#define TXT_notenoughmem txt[14]
#define TXT_nodmamem txt[15]
#define TXT_mounterr txt[16]
#define TXT_structallocerr txt[17]
#define TXT_uiexit txt[18]
#define TXT_legalnotice txt[19]

#define TXT_LAST 20

/* ezt követően (beleértve a TXT_LAST-adik sort) jönnek a libc sztringek, egyenként max TXT_LIBCSIZE hosszúak */
#define TXT_LIBCSIZE 128

#ifndef _AS

extern char *txt[];                                 /* betöltött nyelvi szótár, fordítások */
extern char libcerrstrs[(ENOTIMPL+1)*TXT_LIBCSIZE]; /* libc errno szövegek */
extern char osver[256];                             /* OS verzió */

#endif
