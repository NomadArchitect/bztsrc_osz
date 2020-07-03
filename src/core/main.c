/*
 * core/main.c
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
 * @brief Core, fő ciklus
 *
 *   Memória térkép
 *       -512G megosztott felhasználói memória  (0xFFFFFF8000000000)    globális
 *         -4G CPU-nként más leképezés          (0xFFFFFFFF00000000)    CPU lokális
 *         -3G core megosztott memória          (0xFFFFFFFF40000000)    globális
 *       -128M MMIO (például Raspberry Pi-n)    (0xFFFFFFFFF8000000)    globális
 *        -64M kprintf-nek a framebuffer        (0xFFFFFFFFFC000000)    globális
 *         -2M           bootboot[1] struct     (0xFFFFFFFFFFE00000)    globális
 *         -2M + 1page   environment[2]         (0xFFFFFFFFFFE01000)    globális
 *         -2M + 2pages  core text szegmens  v  (0xFFFFFFFFFFE02000)    globális
 *         ..0           core boot verem     ^  (0x0000000000000000)    CPU-nként más cím
 *
 *       0-16G felh.  egy-az-egy leképezve[3]   (0x0000000400000000)    taszk lokális
 *
 *   [1] lásd https://gitlab.com/bztsrc/bootboot/blob/master/bootboot.h
 *   [2] lásd sys/config és env.c. Sima szöveg, kulcs=érték párok,
 *       újsor karakterrel elválasztva.
 *   [3] új taszk létrehozásakor egy új címtér kerül leképezésre az induláskori
 *       egy-az-egyben a RAM 0 - 2^48 leképezés helyett.
 */

#include <arch.h>               /* a vmm_stack miatt kell */

/**********************************************************************
 *                         OS/Z Életciklus                            *
 **********************************************************************/
void main()
{
    /*** 1. lépés: motorikus reflexek ***/
    /* konzol inicializálása */
    kprintf_init();
    /* alacsony szintű hardver inicializálás */
    platform_dbginit(); /* soros vonali debug konzol (opcionális) */
    platform_cpu();     /* CPU ellenőrzése */
    platform_srand();   /* véletlenszám generátor inicializálása */
    /* környezeti változók beolvasása */
    env_init();
    /* fizikai és virtuális memória kezelő indítása */
    pmm_init();
    vmm_init();
    /* most hogy már van rendes virtuális memória, átkapcsolunk CPU-nkénti veremre, és értesítjük a pmm-et */
    vmm_stack;
    pmm_vmem();
    /* megszakításkezelő, falióra, eszközmeghajtók inicializálása */
    drivers_init();
    /* "FS" taszk betöltése */
    service_add(SRV_FS, "/sys/fs");

#if 0
    /*** 2. lépés: kommunikációs képességek ***/
    /* initialize "UI" task to handle user input / output */
    service_add(SRV_UI, "/sys/ui");

    /* other means of communication */
/*
    if(flags & FLAG_SYSLOG) {
        // start "syslog" task so others can log errors
        service_add(SRV_syslog, "/sys/syslog");
    }
    if(flags & FLAG_INET) {
        // initialize "inet" task for ipv4 and ipv6 routing
        service_add(SRV_inet, "/sys/inet");
    }
    if(flags & FLAG_SOUND) {
        // initialize "sound" task to handle audio channels
        service_add(SRV_sound, "/sys/sound");
    }
*/

    /*** 3. lépés: kelj fel és járj. ***/
    service_add(SRV_init, flags & FLAG_RESCUESH ? "/sys/bin/sh" : (
#if DEBUG
        debug&DBG_TESTS? "/sys/bin/test" :
#endif
        "/sys/init"));
#endif
    /* elindítjuk az ütemezést. A meghajtók inicializálnak, és a sched_pick() sorra kiválasztja őket... */
    drivers_start();

    /*** 4. lépés: "rest in deep and dreamless slumber" (a'la WW), azaz aludj el szépen kis Balázs ***/
    /* ...és elvileg sose jutunk ide. Helyette akkor kapcsoljuk ki a gépet, amikor az init taszk meghívja az exit()-et */
    kprintf_poweroff();
}
