/*
 * drivers/input/ps2_8042/ps2.h
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
 * @brief PS2 vezérlő kommunikáció
 */

#define PS2_DATA    0x60
#define PS2_CTRL    0x64

/**
 * kommunikáció a PS/2 vezérlővel
 */
uint8_t ps2_read()
{
    register uint16_t ret = 0xFF, cnt = 1024;
    while(!(inb(PS2_CTRL)&1) && --cnt) cpu_relax;
    if(cnt) ret = inb(PS2_DATA);
    return ret;
}

void ps2_write(uint8_t b)
{
    register uint16_t cnt = 1024;
    while((inb(PS2_CTRL)&2) && --cnt) cpu_relax;
    if(cnt) outb(PS2_DATA, b);
}

void ps2_cmd0(uint8_t cmd)
{
    register uint16_t ret = 3, cnt = 1024;
    while(ret&3 && --cnt) {
        cpu_relax;
        ret = inb(PS2_CTRL);
        if(ret&1) inb(PS2_DATA);
    }
    if(cnt) outb(PS2_CTRL, cmd);
}

static __inline__ void ps2_cmd1(uint8_t cmd, uint8_t arg)
{
    ps2_cmd0(cmd);
    ps2_write(arg);
}

/**
 * parancs küldése az első porton lévő eszköznek (billentyűzet)
 */
static __inline__ uint8_t ps2_kbd0(uint8_t cmd)
{
    ps2_write(cmd);
    return ps2_read();
}

static __inline__ uint8_t ps2_kbd1(uint8_t cmd, uint8_t arg)
{
    ps2_write(cmd);
    if(ps2_read() == 0xFA)
        ps2_write(arg);
    return ps2_read();
}

/**
 * parancs küldése a második porton lévő eszköznek (egér)
 */
static __inline__ uint8_t ps2_aux0(uint8_t cmd)
{
    ps2_cmd1(0xD4, cmd);
    return ps2_read();
}

static __inline__ uint8_t ps2_aux1(uint8_t cmd, uint8_t arg)
{
    ps2_cmd1(0xD4, cmd);
    if(ps2_read() == 0xFA) {
        ps2_cmd0(0xD4);
        ps2_write(arg);
    }
    return ps2_read();
}
