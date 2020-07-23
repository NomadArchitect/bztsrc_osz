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
 *                         OS/Z elindítása                            *
 **********************************************************************/
void main()
{
    kprintf_init();     /* konzol inicializálása */
    platform_dbginit(); /* soros vonali debug konzol (opcionális, DEBUG define kapcsolja) */
    platform_cpu();     /* CPU ellenőrzése */
    platform_srand();   /* véletlenszám generátor inicializálása */
    env_init();         /* környezeti változók beolvasása */
    pmm_init();         /* fizikai és virtuális memória kezelő indítása */
    vmm_init();
    vmm_stack;          /* most hogy már van rendes virtuális memória, átkapcsolunk CPU-nkénti veremre, és értesítjük a pmm-et */
    pmm_vmem();
    drivers_init();     /* megszakításkezelő, falióra, eszközmeghajtók, rendszerszolgáltatások inicializálása */
    /* ...és elvileg sose jutunk ide. Helyette akkor kapcsoljuk ki a gépet, amikor az init taszk meghívja az exit(0)-át */
    kprintf_poweroff();
}
