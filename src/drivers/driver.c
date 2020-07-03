/*
 * drivers/driver.c
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
 * @subsystem eszközmeghajtó
 * @brief eszközmeghajtó diszpécser
 */

#include <osZ.h>
#define _DRIVER_C
#include "include/driver.h"

/**
 * fő ciklus
 */
public int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    msg_t *msg;

    if(!drv_init())
        exit(EX_UNAVAILABLE);

    while(1) {
        msg = mq_recv();
        switch(EVT_FUNC(msg->evt)) {
            case DRV_IRQ:
                drv_irq(msg->data.scalar.arg0,msg->data.scalar.arg1);
                if(msg->data.scalar.arg0 != USHRT_MAX) mq_send1(SRV_CORE, EVT_ack, msg->data.scalar.arg0);
                break;
            case DRV_open: drv_open(msg->data.scalar.arg0, msg->data.scalar.arg1); break;
            case DRV_close: drv_close(msg->data.scalar.arg0); break;
            case DRV_read: drv_read(msg->data.scalar.arg0); break;
            case DRV_write: drv_write(msg->data.scalar.arg0); break;
            case DRV_ioctl: drv_ioctl(msg->data.scalar.arg0); break;
#if DEBUG
            /* ennek nem szabad előfordulnia, mivel csak a core és az FS taszk küldhet üzenetet */
            default: dbg_printf("driver pid %x: unknown message type\n", getpid()); break;
#endif
        }
    }
}
