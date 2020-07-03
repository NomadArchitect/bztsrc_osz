/*
 * include/osZ/event.h
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
 * @brief események (nem várt üzenetek)
 */

#ifndef _EVENT_H
#define _EVENT_H 1

/* mivel bármelyik alkalmazás kaphat ilyen üzenetet, fontos, hogy ne
 * ütközzön a rendszer üzenetkódokkal, kizárólag 0x3F00 - 0x3FF0 */

/*** billentyűzet események ***/
/* billentyűzet eseménymaszk */
#define EVT_keyrelease  (1<<0)
#define EVT_allkeys     (1<<1)

#define UI_keyevent     (0x3F00)
/* módosítók */
#define UIK_SHIFT   (1<<0)
#define UIK_CTRL    (1<<1)
#define UIK_ALT     (1<<2)
#define UIK_SUPER   (1<<3)
/* flip-flop jellegű módosítók */
#define UIK_NUML    (1<<4)
#define UIK_CAPSL   (1<<5)
#define UIK_SCRL    (1<<6)
/* billentyűfelengedés */
#define UIK_RELEASE (1<<7)

#ifndef _AS
typedef struct {
    evt_t       evt;
    uint64_t    winid;
    keymap_t    keycode;
    uint64_t    keyflags;
} ui_keyevent_t;
#endif

/*** mutató események ***/
/* mutató eseménymaszk */
#define EVT_btnrelease  (1<<2)
#define EVT_ptrmove     (1<<3)

#define UI_ptrevent     (0x3F01)

#ifndef _AS
typedef struct {
    evt_t       evt;
    uint64_t    winid;
    uint8_t     ptrid;
    uint8_t     shape;
    uint16_t    btn;
    uint32_t    x;
    uint32_t    y;
    uint32_t    z;
} ui_ptrevent_t;
#endif

/*** ablak események ***/
#define EVT_winfocus    (1<<4)
#define EVT_enterleave  (1<<5)
#define EVT_winresize   (1<<6)

#define UI_winfocus     (0x3F02)
#define UI_winblur      (0x3F03)
#define UI_winenter     (0x3F04)
#define UI_winleave     (0x3F05)
#define UI_winresize    (0x3F06)

#ifndef _AS
typedef struct {
    evt_t       evt;
    uint64_t    winid;
} ui_winnotify_t;

typedef struct {
    evt_t       evt;
    uint64_t    winid;
    uint32_t    w;
    uint32_t    h;
    uint32_t    s;
} ui_winresize_t;
#endif

#endif  /* event.h */
