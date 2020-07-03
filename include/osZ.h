/*
 * include/osZ.h
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
 * @brief Kötelező definíciós fájl minden OS/Z alkalmazáshoz
 */

#ifndef _OS_Z_
#define _OS_Z_ 1

#include <osZ/errno.h>
#include <osZ/limits.h>
#include <osZ/stdint.h>
#include <osZ/stdarg.h>
#include <osZ/types.h>
#include <osZ/mman.h>
#include <osZ/syscall.h>
#include <osZ/platform.h>
#include <osZ/stdio.h>
#include <osZ/stdlib.h>
#include <osZ/string.h>
#include <osZ/syslog.h>
#include <osZ/sound.h>
#include <osZ/inet.h>
#include <osZ/ui.h>
#include <osZ/init.h>

#endif /* osZ.h */
