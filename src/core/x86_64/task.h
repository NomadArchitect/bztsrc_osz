/*
 * core/x86_64/task.h
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
 * @brief Taszk Kontrol Blokk. Architectúra függő struktúra
 */

/* struktúra mezőinek poziciói, asm-hez */
#define TCBFLD_PC           (__PAGESIZE-40)
#define TCBFLD_FLAGS        (__PAGESIZE-24)
#define TCBFLD_SP           (__PAGESIZE-16)
#define TCBFLD_FP           (TCBFLD_GPR+8*16)

#ifndef _AS

typedef struct {
    /*** általános ***/
    tcb_t common;               /* architectúra független rész, 2560 bájt */

    /*** architectúra függő rész ***/
    uint64_t gpr[16];           /* általános célú regiszterek mentési területe */
    uint8_t fp[512];            /* média és lebegőpontos regiszterek mentési területe */
    uint64_t lastpc;            /* nyomkövetéshez */

    /* fennmaradó rész a megszakítási verem */
    uint8_t stack[3*512-128/*sizeof(gpr)*/-512/*sizeof(fx)*/-8-40];
    /* az utolsó 40 bájt a megszakítási verem teteje */
    uint64_t pc;
    uint64_t cs;
    uint64_t rflags;
    uint64_t sp;
    uint64_t ss;
} __attribute__((packed)) tcb_arch_t;

c_assert(sizeof(tcb_arch_t) == __PAGESIZE);

#endif
