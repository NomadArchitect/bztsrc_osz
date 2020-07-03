/*
 * core/env.h
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
 * @subsystem platform
 * @brief Környezeti változók
 */

/* jelzők */
#define FLAG_ASLR       1   /* Address Space Layour Randomization, címterület kiosztás véletlenszerűsítése */
#define FLAG_SYSLOG     2   /* syslog szolgáltatás */
#define FLAG_INET       4   /* internet szolgáltatás */
#define FLAG_SOUND      8   /* hang szolgáltatás */
#define FLAG_PRINT     16   /* nyomtatási sor szolgáltatás */
#define FLAG_RESCUESH  32   /* rendszerjavító parancssor */
#define FLAG_SPACEMODE 64   /* űrjármű üzemmód */

#ifndef _AS

extern uint64_t dmabuf;     /* DMA buffer mérete lapokban */
extern uint64_t debug;      /* debug jelzők */
extern uint64_t quantum;    /* maximális CPU idő mikroszekundumokban */
extern uint64_t display;    /* megjelenítő típusa */
extern uint64_t flags;      /* jelzők, lásd FLAG_* */
extern char lang[];         /* nyelvkód */

void env_init();

#endif
