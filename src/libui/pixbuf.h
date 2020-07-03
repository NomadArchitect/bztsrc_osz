/*
 * libui/pixbuf.h
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
 * @subsystem felület
 * @brief alacsony szintű pixelbuffer rutinok
 */

#ifndef _PIXBUF_H_
#define _PIXBUF_H_ 1

/* pixel buffer típusok */
#define PBT_MONO_MONO    0  /* mm monokróm 2D */
#define PBT_MONO_COLOR   1  /* mc sima 2D színes */
#define PBT_STEREO_MONO  2  /* sm szürkeárnyalatos vörös-cián 3D (anaglif) */
#define PBT_STEREO_COLOR 3  /* sc sztereó színes 3D (polarizált szemüveg, VR sisak stb. eszközfüggő) */

#define RGB(r,g,b)          (r<<16|g<<8|b)
#define pixbuf_color(t,c)   (!(t&1)?(((c>>16)&0xff)+((c>>8)&0xff)+(c&0xff))/3:c)
#define pixbuf_pixelsize(t) (!(t&1)?1:4)
#define pixbuf_stereo(t)    ((t>1)?1:0)
#define pixbuf_bufsize(t)   ((!(t&1)?1:4)*(t>1?2:1))

/* font jellemzők */
#define FNTA_STRIKETHROUGHT (1<<0)
#define FNTA_UNDERLINE      (1<<1)
#define FNTA_TRANSPARENT    (1<<7)

#ifndef _AS

void pixbuf_putc(uint32_t *pixbuf, uint8_t ptype, uint32_t pw, uint32_t ph, uint32_t fg, uint32_t bg, uint8_t attr, int c);
void pixbuf_puts(uint32_t *pixbuf, uint8_t ptype, uint32_t pw, uint32_t ph, uint32_t fg, uint32_t bg, uint8_t attr,
    int32_t x, int32_t y, const char *str);
void pixbuf_rect(uint32_t *pixbuf, uint8_t ptype, uint32_t pw, uint32_t ph, uint32_t color,
    int32_t x, int32_t y, uint32_t w, uint32_t h);
void pixbuf_filledrect(uint32_t *pixbuf, uint8_t ptype, uint32_t pw, uint32_t ph, uint32_t color,
    int32_t x, int32_t y, uint32_t w, uint32_t h);
void pixbuf_blit(uint32_t *src, uint8_t sptype, uint32_t spw, uint32_t sph, int32_t sx, int32_t sy, uint32_t sw, uint32_t sh,
    uint32_t *dst, uint8_t dptype, uint32_t dpw, uint32_t dph, int32_t dx, int32_t dy, uint32_t dw, uint32_t dh);

#endif

#endif
