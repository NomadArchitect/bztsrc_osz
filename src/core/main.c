/*
 * core/main.c
 *
 * Copyright (c) 2016-2020 bzt (bztsrc@gitlab)
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
 * @brief Core, fő platformfüggetlen lépések a BSP-n
 */

#include <arch.h>               /* a vmm_stack miatt kell, egyébként elég lenne a core.h */

/**********************************************************************
 *                         OS/Z Életciklus                            *
 **********************************************************************/
void main()
{
    /*** 1. lépés: motorikus reflexek ***/
    kprintf_init();     /* konzol inicializálása */
    /* alacsony szintű hardver inicializálás */
    platform_dbginit(); /* soros vonali debug konzol (opcionális, DEBUG define kapcsolja) */
    platform_cpu();     /* CPU ellenőrzése */
    platform_srand();   /* véletlenszám generátor inicializálása */
    env_init();         /* környezeti változók beolvasása */
    pmm_init();         /* fizikai és virtuális memória kezelő indítása */
    vmm_init();
    vmm_stack;          /* most hogy már van rendes virtuális memória, átkapcsolunk CPU-nkénti veremre, és értesítjük a pmm-et */
    pmm_vmem();
    drivers_init();     /* megszakításkezelő, falióra, eszközmeghajtók inicializálása */
    /* "FS" taszk betöltése */
    service_add(SRV_FS, "/sys/fs");

#if 0
    /*** 2. lépés: kommunikációs képességek betöltése ***/
    /* az "UI" taszk betöltése a felhasználi interakciók kezelésére */
    service_add(SRV_UI, "/sys/ui");
    /* egyéb, opcionális kommunikációs lehetőségek támogatása */
/*
    if(flags & FLAG_SYSLOG) service_add(SRV_syslog, "/sys/syslog");
    if(flags & FLAG_INET)   service_add(SRV_inet,   "/sys/inet");
    if(flags & FLAG_SOUND)  service_add(SRV_sound,  "/sys/sound");
    if(flags & FLAG_PRINT)  service_add(SRV_print,  "/sys/print");
*/

    /*** 3. lépés: kelj fel és járj! ***/
    service_add(SRV_init, flags & FLAG_RESCUESH ? "/sys/bin/sh" : (
#if DEBUG
        debug&DBG_TESTS? "/sys/bin/test" :
#endif
        "/sys/init"));
#endif
    /* elindítjuk az ütemezést. A meghajtók inicializálnak, és a sched_pick() sorra kiválasztja őket... */
    drivers_start();

    /*** 4. lépés: "rest in deep and dreamless slumber" (a'la WW), azaz aludj el szépen kis Balázs ***/
    /* ...és elvileg sose jutunk ide. Helyette akkor kapcsoljuk ki a gépet, amikor az init taszk meghívja az exit(0)-át */
    kprintf_poweroff();
}
