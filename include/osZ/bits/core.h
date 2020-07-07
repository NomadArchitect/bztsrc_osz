/*
 * include/osZ/bits/core.h
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
 * @brief OS/Z Core szolgáltatások (rendszerhívás kódok)
 */

#ifndef _BITS_CORE_H
#define _BITS_CORE_H 1

/* szolgáltatások */
#define SYS_recv        ( 0) /* üzenet fogadása */
#define SYS_yield       ( 1) /* processzoridő feladása */
#define SYS_exit        ( 2) /* kilépés */
#define SYS_fork        ( 3) /* taszk lemásolása */
#define SYS_execve      ( 4) /* futtatható betöltése */
#define SYS_usleep      ( 5) /* taszk altatása adott mikroszekundumig */
#define SYS_regirq      ( 6) /* taszk hozzáadása az IRT-hez */
#define SYS_regtmr      ( 7) /* taszk hozzáadása az IRT-hez a falióra IRQ-jához */
#define SYS_drvfind     ( 8) /* meghajtóprogram keresése eszközspecifikációhoz */
#define SYS_drvadd      ( 9) /* meghajtóprogram hozzáadása */
#define SYS_stimezone   (10) /* rendszeridő időzónájának beállítása percekben */
#define SYS_stimebcd    (11) /* rendszeridő beállítása BCD idősztringből */
#define SYS_stime       (12) /* rendszeridő beállítása mikroszekundumban */
#define SYS_time        (13) /* időbélyeg visszaadása mikroszekundumban */
#define SYS_srand       (14) /* entrópia növelése */
#define SYS_rand        (15) /* véletlenszám lekérése */
#define SYS_getentropy  (16) /* buffer feltöltése véletlenszámokkal */
#define SYS_meminfo     (17) /* memória állapotának lekérdezése */
#define SYS_memmap      (18) /* memória leképezése a címtérbe */
#define SYS_memunmap    (19) /* memória felszabadítása a címtérből */
#define SYS_trace       (20) /* nyomkövetés ki/bekapcsolása */
#define SYS_getpid      (21) /* visszaadja a hivó taszk pidjét */
#define SYS_getppid     (22) /* visszaadja a hívó taszk szülőjének pidjét */
#define SYS_getuid      (23) /* visszaadja egy adott taszk tulajdonosát */
#define SYS_getgid      (24) /* visszaadja egy adott taszk jogosultságait */
#define SYS_tskcpy      (25) /* memória másolása adott taszk címterébe */
#define SYS_log         (26) /* korai rendszernapló írása */

/* események */
#define EVT_ack         (0x3FFA) /* visszaigazolás */
#if DEBUG
#define SYS_dbgprintf   (0x3FFD) /* kiírás a debug konzolra */
#endif
#define EVT_exit        (0x3FFE) /* taszk befejeződés értesítés */
#define EVT_fork        (0x3FFF) /* taszk másolás értesítés */

#endif /* bits/core.h */
