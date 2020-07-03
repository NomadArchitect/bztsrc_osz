/*
 * core/aarch64/fault.c
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
 * @subsystem platform
 * @brief kivétel kezelő
 */
#include <arch.h>

void fault_intr(uint64_t esr, uint64_t far, uint64_t spsr)
{
    register uint16_t exc = esr>>26;

    switch(exc) {
        case 0x24:
        case 0x25: fault_pagefault(exc, esr, far); break;
        case 0x30: /* breakpoint EL0 */
        case 0x31: /* breakpoint EL1 */
        case 0x32: /* steppoint EL0 */
        case 0x33: /* steppoint EL1 */
        case 0x34: /* watchpoint EL0 */
        case 0x35: /* watchpoint EL1 */
        case 0x3c: /* BRK */ fault_dbg(); break;
        default:
            kpanic("Exception esr=%x far=%x spsr=%x", esr, far, spsr);
    }
}
