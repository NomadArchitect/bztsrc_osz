/*
 * drivers/include/x86_64/platform.h
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
 * @brief platform függő eszközmeghajtó definíciók
 */

#ifndef _DRV_PLATFORM_H
#define _DRV_PLATFORM_H

#ifndef _AS
static __inline__ uint8_t inb(uint64_t port)
{
    register uint8_t ret;
    __asm__ __volatile__("inb %1, %0;" : "=a"(ret) : "Nd"((uint16_t)port));
    return ret;
}

static __inline__ uint16_t inw(uint64_t port)
{
    register uint16_t ret;
    __asm__ __volatile__("inw %1, %0;" : "=a"(ret) : "Nd"((uint16_t)port));
    return ret;
}

static __inline__ uint32_t inl(uint64_t port)
{
    register uint32_t ret;
    __asm__ __volatile__("inl %1, %0;" : "=a"(ret) : "Nd"((uint16_t)port));
    return ret;
}

static __inline__ void outb(uint64_t port, uint8_t val) { __asm__ __volatile__("outb %0, %1" :: "a"(val), "Nd"((uint16_t)port)); }
static __inline__ void outw(uint64_t port, uint16_t val){ __asm__ __volatile__("outw %0, %1" :: "a"(val), "Nd"((uint16_t)port)); }
static __inline__ void outl(uint64_t port, uint32_t val){ __asm__ __volatile__("outl %0, %1" :: "a"(val), "Nd"((uint16_t)port)); }
#endif

#endif
