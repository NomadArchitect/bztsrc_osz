/*
 * core/x86_64/idt.h
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
 * @subsystem memória
 * @brief Megszakítás Leíró Tábla (IDT) definíciói
 */

/* IDT konstansok: 1ddd0tttt0000iiiissssssssssssssss
 * d: dpl, t: F trap, E int, i: ist, s: code selector */
#define IDT_EXC 0x8F010008
#define IDT_NMI 0x8F020008
#define IDT_INT 0x8E010008

#define IDT_GATE(n,type,offset) do{idt[n*2]=((uint64_t)((((uint64_t)(offset)>>16)&(uint64_t)0xFFFF)<<48) | \
    (uint64_t)((uint64_t)(type)<<16) | ((uint64_t)(offset) & (uint64_t)0xFFFF)); idt[n*2+1]=((uint64_t)(offset)>>32);}while(0)

#ifndef _AS
extern uint64_t idt[512];
#endif
