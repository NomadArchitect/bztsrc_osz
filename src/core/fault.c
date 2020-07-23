/*
 * core/fault.c
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
 * @brief platform független kivétel kezelők
 */

#include <arch.h>                   /* vmm_notpresent() makró */

extern uint8_t pmm_entries_buf;

void fault_dbg()
{
#if DEBUG
    dbg_start("Breakpoint", false);
#else
    syslog(LOG_ERR, "compiled without debugger");
#endif
}

void fault_pagefault(unused int16_t exc, uint64_t errcode, virt_t addr)
{
    if(vmm_notpresent(errcode) && ((addr > __PAGESIZE && addr < TEXT_ADDRESS) ||
        (addr >= DYN_ADDRESS  && addr < BUF_ADDRESS) || (addr >= SDYN_ADDRESS && addr < LDYN_ADDRESS) ||
        (addr >= CDYN_ADDRESS && addr < CDYN_TOP) ||     addr > (virt_t)&pmm_entries_buf) ) {
            vmm_map((tcb_t*)0, addr, 0, __PAGESIZE, PG_USER_RW);
    } else
        kpanic("page fault: notpresent %d addr %x", vmm_notpresent(errcode), addr);
}
