/*
 * include/osZ/ui.h
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
 * @brief OS/Z felhasználói felület szolgáltatások
 */

#ifndef _UI_H
#define _UI_H 1

/* ablak jelzők */
#define UIW_MAPPED  (1<<0)
#define UIW_FULLSCR (1<<1)
#define UIW_DOCK    (1<<2)

/* az ablak esemény jelzőkért lásd sys/event.h */

/* kurzorok */
#define UIC_none -1
#define UIC_default 0
#define UIC_pointer 1
#define UIC_clicked 2
#define UIC_help 3
#define UIC_kill 4
#define UIC_notallowed 5
#define UIC_cell 6
#define UIC_crosshair 7
#define UIC_text 8
#define UIC_vtext 9
#define UIC_alias 10
#define UIC_copy 11
#define UIC_move 12
#define UIC_scroll 13
#define UIC_colresize 14
#define UIC_rowresize 15
#define UIC_nresize 16
#define UIC_eresize 17
#define UIC_sresize 18
#define UIC_wresize 19
#define UIC_neresize 20
#define UIC_nwresize 21
#define UIC_seresize 22
#define UIC_swresize 23
#define UIC_ewresize 24
#define UIC_nsresize 25
#define UIC_neswresize 26
#define UIC_nwseresize 27
#define UIC_zoomin 28
#define UIC_zoomout 29
#define UIC_grab 30
#define UIC_grabbing 31
#define UIC_progress 32 /* magasabb értékek ezt animálálják */

#ifndef _AS
#include <osZ/event.h>

/*** libc által biztosított funkciók (ablakkezelés) ***/
dpy_t ui_opendisplay(const char *name);
win_t *ui_createpixbuf(dpy_t dpy, uint32_t width, uint32_t height);
win_t *ui_createwindow(dpy_t dpy, const char *type, int32_t x, int32_t y, uint32_t width, uint32_t height,
    uint32_t flags, uint32_t evtmask);
void ui_eventmask(win_t *win, uint32_t evtmask);
void ui_mapwindow(win_t *win);
void ui_unmapwindow(win_t *win);
void ui_setwindowtitle(win_t *win, char *s);
void ui_setcursor(win_t *win, int shape);
void ui_focuswindow(win_t *win);
void ui_movewindow(win_t *win, int32_t x, int32_t y);
void ui_resizewindow(win_t *win, uint32_t w, uint32_t h);
void ui_destroywindow(win_t *win);
void ui_flush(win_t *win);
void ui_datapacket(uint32_t winid, const char *ptr, size_t size);
void ui_loadcursor(char *file);

/*** libui által biztosított funkciók (rajzolás) ***/
/*void ui_setfont(win_t *win, psf_t *fnt, size_t size);*/
void ui_puts(win_t *win, uint32_t fg, uint32_t bg, uint8_t attr, int32_t x, int32_t y, const char *str);
void ui_rect(win_t *win, uint32_t color, int32_t x, int32_t y, uint32_t w, uint32_t h);
void ui_filledrect(win_t *win, uint32_t color, int32_t x, int32_t y, uint32_t w, uint32_t h);
bool_t ui_blit(win_t *src, win_t *dst, int32_t sx, int32_t sy, uint32_t sw, uint32_t sh,
    int32_t dx, int32_t dy, uint32_t dw, uint32_t dh);
#endif

#endif /* ui.h */
