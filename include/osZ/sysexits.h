/*
 * include/osZ/sysexits.h
 *
 * Copyright (c) 2020 bzt (bztsrc@gitlab)
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
 * @brief OS/Z eszközmeghajtó és rendszerszolgáltatás kilépési kódok
 */

#ifndef _SYSEXITS_H
#define _SYSEXITS_H 1

#define EX_OK       0   /* minden rendben volt */
#define EX_USAGE    64  /* hibás paraméterek */
#define EX_DATAERR  65  /* hibás adatformátum */
#define EX_NOINPUT  66  /* a bemenet nem elérhető */
#define EX_NOUSER   67  /* címzett ismeretlen */
#define EX_NOHOST   68  /* ismeretlen hosztnév */
#define EX_UNAVAILABLE  69  /* szolgáltatás nem elérhető */
#define EX_SOFTWARE 70  /* belső szoftver hiba */
#define EX_OSERR    71  /* rendszerhiba (pl fork nem sikerült) */
#define EX_OSFILE   72  /* kritikus rendszerfájl hiányzik */
#define EX_CANTCREAT 73 /* nem lehet a (felhasználói) kimeneti fájlt létrehozni */
#define EX_IOERR    74  /* általános B/K hiba */
#define EX_TEMPFAIL 75  /* átmeneti hiba, később újra kell próbálni */
#define EX_PROTOCOL 76  /* távoli oldali protokoll hiba */
#define EX_NOPERM   77  /* hozzáférési hiba */
#define EX_CONFIG   78  /* konfigurációs hiba */

#define EX__BASE    64  /* innen indulnak a hibakódok */
#define EX__MAX     78  /* a legnagyobb hibakód */

#endif /* sysexits.h */
