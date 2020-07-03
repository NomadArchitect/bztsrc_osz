/*
 * include/osZ/syscall.h
 *
 * Copyright (c) 2017 bzt (bztsrc@gitlab)
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
 * @brief OS/Z rendszerhívások
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H 1

/**
 * Általában ez a header a rendszerhívások kódjait tartalmazza, de
 * mivel az OS/Z egy mikro-kernel, több rendszer szolgáltatás látja
 * el a feladatot, és a rendszerhívások megoszlanak köztük.
 */

/*** rendszer szolgáltatások (negatív és zéró pid_t) ***/
#define SRV_CORE         0
#define SRV_FS          -1
#define SRV_UI          -2
#define SRV_syslog      -3
#define SRV_inet        -4
#define SRV_sound       -5
#define SRV_print       -6
#define SRV_init        -7

#define NUMSRV           8

/*** rendszerhíváskódok ***/
/* 000000000000xxxx Memória, multitaszk, idózítő és egyéb core szolgáltatás, stdlib.h */
#include <osZ/bits/core.h>
/* FFFFFFFFFFFFxxxx Fájlrendszer szolgáltatások, stdio.h */
#include <osZ/bits/fs.h>
/* FFFFFFFFFFFExxxx Felhasználói Felület szolgáltatások, ui.h */
#include <osZ/bits/ui.h>
/* FFFFFFFFFFFDxxxx Naplózás szolgáltatás, syslog.h */
#include <osZ/bits/syslog.h>
/* FFFFFFFFFFFCxxxx Hálózati szolgáltatások, inet.h */
#include <osZ/bits/inet.h>
/* FFFFFFFFFFFBxxxx Hangkeverő szolgáltatás, sound.h */
#include <osZ/bits/sound.h>
/* FFFFFFFFFFFAxxxx Nyomtatási sor, print.h */
#include <osZ/bits/print.h>
/* FFFFFFFFFFF9xxxx Felhasználói szolgáltatások kezelése, init.h */
#include <osZ/bits/init.h>

/*** libc prototípusok */
#ifndef _AS

/*** üzenet sorok, a tényleges rendszerhívások ***/
/* async, üzenet küldés (nem-blokkoló, kivéve ha a cél sor tele van) */
#define mq_send0(pid,func) mq_send(0,0,0,0,0,0,(uint64_t)(pid)<<16|(func),0)
#define mq_send1(pid,func,a) mq_send((uint64_t)(a),0,0,0,0,0,(uint64_t)(pid)<<16|(func),0)
#define mq_send2(pid,func,a,b) mq_send((uint64_t)(a),(uint64_t)(b),0,0,0,0,(uint64_t)(pid)<<16|(func),0)
#define mq_send3(pid,func,a,b,c) mq_send((uint64_t)(a),(uint64_t)(b),(uint64_t)(c),0,0,0,(uint64_t)(pid)<<16|(func),0)
#define mq_send4(pid,func,a,b,c,d) mq_send((uint64_t)(a),(uint64_t)(b),(uint64_t)(c),(uint64_t)(d),0,0,(uint64_t)(pid)<<16|(func),0)
#define mq_send5(pid,func,a,b,c,d,e) mq_send((uint64_t)(a),(uint64_t)(b),(uint64_t)(c),(uint64_t)(d),(uint64_t)(e),0,(uint64_t)(pid)<<16|(func),0)
#define mq_send6(pid,func,a,b,c,d,e,f) mq_send((uint64_t)(a),(uint64_t)(b),(uint64_t)(c),(uint64_t)(d),(uint64_t)(e),(uint64_t)(f),(uint64_t)(pid)<<16|(func),0)
uint64_t mq_send(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t dst, uint64_t serial);
/* sync, üzenet küldés és várakozás válaszra (blokkol) */
#define mq_call0(pid,func) mq_call(0,0,0,0,0,0,(uint64_t)(pid)<<16|(func))
#define mq_call1(pid,func,a) mq_call((uint64_t)(a),0,0,0,0,0,(uint64_t)(pid)<<16|(func))
#define mq_call2(pid,func,a,b) mq_call((uint64_t)(a),(uint64_t)(b),0,0,0,0,(uint64_t)(pid)<<16|(func))
#define mq_call3(pid,func,a,b,c) mq_call((uint64_t)(a),(uint64_t)(b),(uint64_t)(c),0,0,0,(uint64_t)(pid)<<16|(func))
#define mq_call4(pid,func,a,b,c,d) mq_call((uint64_t)(a),(uint64_t)(b),(uint64_t)(c),(uint64_t)(d),0,0,(uint64_t)(pid)<<16|(func))
#define mq_call5(pid,func,a,b,c,d,e) mq_call((uint64_t)(a),(uint64_t)(b),(uint64_t)(c),(uint64_t)(d),(uint64_t)(e),0,(uint64_t)(pid)<<16|(func))
#define mq_call6(pid,func,a,b,c,d,e,f) mq_call((uint64_t)(a),(uint64_t)(b),(uint64_t)(c),(uint64_t)(d),(uint64_t)(e),(uint64_t)(f),(uint64_t)(pid)<<16|(func))
msg_t *mq_call(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t dst);
/* sync, várakozás üzenetre (blokkol) */
msg_t *mq_recv();
/* async, van új üzenet? sorszámot vagy 0-át ad vissza (nem-blokkoló) */
uint64_t mq_ismsg();
/* sync, üzenetek kiszolgálása (blokkol, nem tér vissza, hacsak hiba nincs) */
uint64_t mq_dispatch();

#endif

#endif /* syscall.h */
