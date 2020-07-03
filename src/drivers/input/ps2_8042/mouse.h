/*
 * drivers/input/ps2_8042/mouse.h
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
 * @brief PS2 egér eszközmeghajtó
 */

uint8_t mouse_pktsize, mouse_cnt, mouse_buf[8];

/**
 * PS/2 Intellimouse inicializálása
 */
static __inline__ bool_t mouse_init()
{
    ps2_cmd0(0xA8);                 /* engedélyezzük a második portot */
    if(ps2_aux0(0xFF) != 0xFA)      /* reszet és önteszt */
        return false;
    ps2_kbd0(0xF5);                 /* letiltjuk a scan-t */
    ps2_aux0(0xF5);                 /* letiltjuk az adatcsomagokat */
    ps2_aux0(0xF6);                 /* alapértékek beállítása */
    mouse_pktsize = 3;
    /* 4 bájtos csomagok */
    ps2_aux1(0xF3, 200);            /* Z tengely engedélyezése */
    ps2_aux1(0xF3, 100);
    ps2_aux1(0xF3, 80);
    if(ps2_aux0(0xF2) == 0xFA && ps2_read() == 3) {
        mouse_pktsize = 4;
        ps2_aux1(0xF3, 200);        /* 4. és 5. gomb engedélyezése */
        ps2_aux1(0xF3, 200);
        ps2_aux1(0xF3, 80);
    }
    ps2_aux1(0xF3, 100);            /* alapértelmezett érzékenység */
    ps2_aux1(0xE8, 2);              /* 4 léptetés milliméterenként */
    ps2_aux0(0xE6);                 /* arány 1:1 */
    while(inb(PS2_CTRL)&3) inb(PS2_DATA);
    ps2_kbd0(0xF4);                 /* engedélyezzük a scan-t */
    ps2_aux0(0xF4);                 /* engedélyezzük az adatcsomagokat */
    mouse_cnt = 0;
    devices |= 2;
    mq_send4(SRV_UI, SYS_pointer, (12UL<<32), 0, 0, 0);
    return true;
}

/**
 * megszakításkezelő
 */
static __inline__ void mouse_irq()
{
    if(!(inb(PS2_CTRL)&1)) return;  /* néha fantom IRQ-t kapunk a már kiolvasott ack miatt */
    if(mouse_cnt >= sizeof(mouse_buf)-1) mouse_cnt = 0;
    mouse_buf[mouse_cnt++] = ps2_read();
    if(mouse_cnt < mouse_pktsize) return;
    mouse_cnt = 0;
    mq_send4(SRV_UI, SYS_pointer, ((mouse_buf[3]&0x80)? 0 : (mouse_buf[3]&0x30)>>1) | (mouse_buf[0]&7) | (12UL<<32),
        (uint64_t)((int64_t)(mouse_buf[0]&0x10?~0xFFUL:0) | (int64_t)mouse_buf[1]),
        (uint64_t)(-((int64_t)(mouse_buf[0]&0x20?~0xFFUL:0) | (int64_t)mouse_buf[2])),
        (uint64_t)((int64_t)(mouse_buf[3]&0x08?~0x0FUL:0) | (int64_t)(mouse_buf[3]&0xF)));
}
