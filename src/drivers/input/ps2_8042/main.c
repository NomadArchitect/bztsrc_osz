/*
 * drivers/input/ps2_8042/main.c
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
 * @subsystem eszközmeghajtók
 * @brief PS2 eszközmeghajtó
 */

#include <osZ.h>
#include <driver.h>

uint8_t devices;
uint64_t lastirq;

#include "ps2.h"
#include "keyboard.h"
#include "mouse.h"

/**
 * PS/2 vezérlő inicializálása
 */
public int drv_init()
{
    uint8_t ret;

    ps2_cmd0(0xAD);                     /* első port kikapcsolása */
    ps2_cmd0(0xA7);                     /* második port kikapcsolása */
    ps2_cmd0(0xAA);                     /* vezérlő önteszt futtatása */
    if(ps2_read() != 0x55) {
#if DEBUG
        dbg_printf("PS2 init error\n");
#endif
        return 0;
    }
    ps2_cmd0(0x20);                     /* konfigurációs bájt kiolvasása */
    ret = ps2_read();
    ret |= 3;                           /* irq-k engedélyezése */
    ret &= ~(1<<6);                     /* scancode fordítás kikapcsolása */
    ps2_cmd1(0x60, ret);
    core_regirq(12); core_regirq(1); /*core_regtmr();*/    /* irq feliratkozások */
    devices = 0; lastirq = 0;           /* hotplug */
    mouse_init();
    keyboard_init();
    return 1;
}

/**
 * IRQ feldolgozó rutin
 */
public void drv_irq(uint16_t irq, uint64_t ticks)
{
    switch(irq) {
        case 1: keyboard_irq(); lastirq = ticks; break;
        case 12: mouse_irq(); lastirq = ticks; break;
        default:
            if(ticks - lastirq > 3000000) {
                if(ps2_aux0(0xF4) == 0xFA) {
                    if(!(devices&2)) mouse_init();
                } else
                    devices &= ~2;
                if(ps2_kbd0(0xEE) == 0xEE) {
                    if(!(devices&1)) keyboard_init();
                } else
                    devices &= ~1;
            }
    }
}

/**
 * a többi meghajtóinterfész nem használt
 */
public void drv_open(dev_t device __attribute__((unused)), uint64_t mode __attribute__((unused)))
{
}

public void drv_close(dev_t device __attribute__((unused)))
{
}

public void drv_read(dev_t device __attribute__((unused)))
{
}

public void drv_write(dev_t device __attribute__((unused)))
{
}

public void drv_ioctl(dev_t device __attribute__((unused)))
{
}

