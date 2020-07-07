/*
 * core/msg.h
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
 * @subsystem taszk
 * @brief üzenet sor fejléce
 */

#define MQ_BUF      16384
#define MQ_MAX      ((MQ_BUF - __PAGESIZE) / sizeof(msg_t))

#ifndef _AS
c_assert(MQ_BUF > __PAGESIZE && MQ_BUF < __BUFFSIZE && MQ_MAX > 64);

typedef struct {
    uint64_t mq_start;      /* körkörös üzenetek eleje */
    uint64_t mq_end;
    uint64_t mq_serial;     /* kimenő szériaszám */
    uint64_t mq_total;      /* összes feldolgozott üzenet */
    uint64_t mq_bufstart;   /* leképező buffer eleje */
    uint64_t mq_bufend;
    uint64_t mq_dummy1;
    uint64_t mq_dummy2;
} packed msghdr_t;

bool_t msg_allowed(tcb_t *sender, pid_t dest, evt_t event);
msg_t *msg_recv(uint64_t serial);
uint64_t msg_send(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, evt_t dst, uint64_t serial);
void msg_notify(pid_t dst, evt_t event, uint64_t a);

#endif
