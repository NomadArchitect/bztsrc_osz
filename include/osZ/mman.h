/*
 * include/osZ/mman.h
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
 * @brief Memória Kezelés, bztalloc használja
 */

#ifndef _MMAN_H
#define _MMAN_H 1

/* hozzáférési jelzők */
#define PROT_READ       0x1     /* olvasható lap  */
#define PROT_WRITE      0x2     /* írható lap  */
#define PROT_EXEC       0x4     /* futtatható lap, kompatibilitásból, nincs és nem is lesz implementálva */
#define PROT_NONE       0x0     /* elérhető a lap */

/* megosztás jelzők (csak egy választható) */
#define MAP_SHARED      0x01    /* megosztott lap */
#define MAP_PRIVATE     0x02    /* privát lap */

/* egyéb jelzők  */
#define MAP_FIXED       0x10    /* pontos cím */
#define MAP_FILE        0
#define MAP_ANONYMOUS   0x20    /* nemkapcsolódik fájl leíró hozzá */
#define MAP_ANON        MAP_ANONYMOUS
#define MAP_SPARSE      0x40    /* címtér módosítás csak, fizikai lap foglalás nélkül */
#define MAP_LOCKED      0x100   /* lap kizárólagossá tétele */
#define MAP_NONBLOCK    0x200   /* ne blokkolódjon B/K műveleteknél */
#define MAP_CORE        0x800   /* csak felügyeleti szintről elérhető */

/* az `mmap' visszatérési értéke hiba esetén */
#define MAP_FAILED  ((void *) -1)

#ifndef _AS

/* Map addresses starting near ADDR and extending for LEN bytes.
   The return value is the actual mapping address chosen or MAP_FAILED
   for errors (in which case `errno' is set).  A successful `mmap' call
   deallocates any previous mapping for the affected region.  */
extern void *mmap (void *addr, size_t len, int prot, int flags, fid_t fid, off_t offs);

/* Deallocate any mapping for the region starting at ADDR and extending LEN
   bytes.  Returns 0 if successful, -1 for errors (and sets errno).  */
extern int munmap (void *addr, size_t len);

#endif

#endif /* mman.h  */
