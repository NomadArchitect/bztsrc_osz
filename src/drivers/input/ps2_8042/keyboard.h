/*
 * drivers/input/ps2_8042/keyboard.h
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
 * @brief PS2 billentyűzet eszközmeghajtó
 */

uint8_t keyboard_leds, keyboard_release, keyboard_skip, keyboard_code;
uint8_t keyboard_usb[] = {  /* PS/2 scancode set 2-ből USB HID scancode-okká alakító tábla */
  50, 66, 72, 62, 60, 58, 59, 69,108, 67, 65, 63, 61, 43, 53,  0,109,226,225,136,224, 20, 30,  0,110,  0, 29, 22,  4, 26, 31,104,
 111,  6, 27,  7,  8, 33, 32,105,112, 44, 25,  9, 23, 21, 34,106,113, 17,  5, 11, 10, 28, 35,  0,114,  0, 16, 13, 24, 36, 37,  0,
 115, 54, 14, 12, 18, 39, 38,  0,  0, 55, 56, 15, 51, 19, 45,  0,  0,135, 52,  0, 47, 46,  0,  0, 57,229, 40, 48,  0, 49,107,  0,
   0,  0,140,  0,138,  0, 42,139,  0, 89,137, 92, 95,  0,  0,  0, 98, 99, 90, 93, 94, 96, 41, 83, 68, 87, 91, 86, 85, 97, 71,  0,
   0,  0,  0, 64,154,117,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,230, 70,  0,228,234,  0,  0,  0,  0,236,  0,  0,  0,  0,227,
   0,129,  0,127,  0,  0,  0,231,  0,  0,  0,  0,  0,  0,  0,101,  0,  0,128,  0,232,  0,121,102,  0,  0,240,120,  0,122,  0,248,
   0,  0,  0,123,124,  0,125,  0,  0,  0, 84,  0,  0,235,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 88,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0, 77,  0, 80, 74,  0,  0,  0, 73, 76, 81,  0, 79, 82,  0,  0,  0,  0, 78,  0, 70, 75,  0,  0
};

/**
 * PS/2 billentyűzet inicializálása
 */
static inline void keyboard_init()
{
    ps2_cmd0(0xAE);                 /* engedélyezzük az első portot */
    if(ps2_kbd0(0xFF) != 0xFA)      /* reszet */
        return;
    ps2_kbd0(0xF5);                 /* letiltjuk a scan-t */
    ps2_aux0(0xF5);                 /* letiltjuk az adatcsomagokat */
    ps2_kbd0(0xF6);                 /* alapértékek beállítása */
    ps2_kbd1(0xF0, 2);              /* scancode set 2 */
    ps2_kbd1(0xF3, 0x17);           /* ismétlés és késletetés alapértékek */
    ps2_kbd1(0xED, 0);              /* ledek kikapcsolása alapból */
    while(inb(PS2_CTRL)&3) inb(PS2_DATA);
    ps2_kbd0(0xF4);                 /* engedélyezzük a scan-t */
    ps2_aux0(0xF4);                 /* engedélyezzük az adatcsomagokat */
    keyboard_leds = keyboard_release = keyboard_skip = keyboard_code = 0;
    devices |= 1;
    mq_send2(SRV_UI, SYS_keyrelease, 0, 0);
}

/**
 * megszakításkezelő
 */
static inline void keyboard_irq()
{
    uint8_t key;

    if(!(inb(PS2_CTRL)&1)) return;  /* néha fantom IRQ-t kapunk a már kiolvasott echo miatt */
    key = ps2_read();
    if(keyboard_skip) { keyboard_skip--; return; }
    switch(key) {
        /* case 0xAA: keyboard_init(); return; */
        case 0xE0: keyboard_code |= 0x80; return;
        case 0xE1: keyboard_skip = 7; keyboard_code = 2; break; /* csak egy E1... kódunk van, azt leképezzük egy nemhasználtra */
        case 0xF0: keyboard_release = true; return;
        default: keyboard_code |= key & 0x7F;
    }
    if(!keyboard_release) {
        switch(keyboard_code) {
            case 0x7E: keyboard_leds ^= 1; ps2_kbd1(0xED, keyboard_leds); break;    /* ScrollLock */
            case 0x77: keyboard_leds ^= 2; ps2_kbd1(0xED, keyboard_leds); break;    /* NumLock */
            case 0x58: keyboard_leds ^= 4; ps2_kbd1(0xED, keyboard_leds); break;    /* CapsLock */
        }
    }
    keyboard_code = keyboard_usb[keyboard_code];
/*
dbg_printf("key %d %x\n",keyboard_release,keyboard_code);
*/
    if(keyboard_code) mq_send2(SRV_UI, keyboard_release? SYS_keyrelease : SYS_keypress, (uint64_t)keyboard_code, 0);
    keyboard_code = 0; keyboard_release = false;
    while(inb(PS2_CTRL)&3) inb(PS2_DATA);
}
