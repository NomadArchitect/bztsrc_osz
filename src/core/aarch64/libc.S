/*
 * core/aarch64/libc.S
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
 * @subsystem platform
 * @brief Alacsony szintű függvénykönyvtár
 */

#include <arch.h>

.section .text

/**
 * jelző lefoglalása többprocesszoros rendszeren
 *
 * void lockacquire(int bit, uint64_t *ptr);
 */
func lockacquire
    /* x0 = 1 << x0 */
    mov     x2, #1
    lsl     x0, x2, x0
#if 0
    /* elemi x2 = [x1], [x1] |= x0 */
1:  ldseta  x2, x0, [x1]
    and     x2, x2, x0
    cbnz    x2, 1b
#else
    prfm    pstl1keep, [x1]         /* prefetch */
1:  ldaxr   x3, [x1]                /* x3 = [x1] */
    and     x4, x3, x0              /* ha (x3 & x0) akkor újra */
    cbnz    x4, 1b
    orr     x3, x3, x0              /* x3 |= x0 */
    stxr    w4, x3, [x1]            /* [x1] = x3 */
    cbnz    w4, 1b
#endif
    ret
endf

/**
 * jelző felszabadítása
 *
 * void lockrelease(int bit, uint64_t *ptr);
 */
func lockrelease
    /* x0 = 1 << x0 */
    mov     x2, #1
    lsl     x0, x2, x0
#if 0
    /* elemi x3 = [x1], [x1] &= ~x0 */
    ldclrl  x3, x0, [x1]
#else
    ldaxr   x3, [x1]                /* x3 = [x1] */
    bic     x3, x3, x0              /* x3 &= ~x0 */
    stxr    w4, x3, [x1]            /* [x1] = x3 */
#endif
    ret
endf

/**
 * memória másolása, architektúra specifikus implementáció
 *
 * void *memcpy(void *dst, const void *src, size_t len)
 */
func memcpy
    /* paraméterek ellenőrzése */
    cbz     x0, 2f
    cbz     x1, 2f
    cbz     x2, 2f
    /* ha kis adatmennyiség, vagy nincs igazítva */
    and     x3, x2, #0xFFFFFFFFFFFFFE00
    cbz     x3, 1f
    and     x3, x0, #0xF
    cbnz    x3, 1f
    and     x3, x1, #0xF
    cbnz    x3, 1f
    /* nagy blokk másolása, 256 bájtosával */
    ldp      q0,  q1, [x1, #32 *0]
    ldp      q2,  q3, [x1, #32 *1]
    ldp      q4,  q5, [x1, #32 *2]
    ldp      q6,  q7, [x1, #32 *3]
    ldp      q8,  q9, [x1, #32 *4]
    ldp     q10, q11, [x1, #32 *5]
    ldp     q12, q13, [x1, #32 *6]
    ldp     q14, q15, [x1, #32 *7]
    lsr     x3, x2, #8
    and     x2, x2, #0xFF
0:  add     x1, x1, #256
    stp      q0,  q1, [x0, #32 *0]
    ldp      q0,  q1, [x1, #32 *0]
    stp      q2,  q3, [x0, #32 *1]
    ldp      q2,  q3, [x1, #32 *1]
    stp      q4,  q5, [x0, #32 *2]
    ldp      q4,  q5, [x1, #32 *2]
    stp      q6,  q7, [x0, #32 *3]
    ldp      q6,  q7, [x1, #32 *3]
    stp      q8,  q9, [x0, #32 *4]
    ldp      q8,  q9, [x1, #32 *4]
    stp     q10, q11, [x0, #32 *5]
    ldp     q10, q11, [x1, #32 *5]
    stp     q12, q13, [x0, #32 *6]
    ldp     q12, q13, [x1, #32 *6]
    stp     q14, q15, [x0, #32 *7]
    ldp     q14, q15, [x1, #32 *7]
    add     x0, x0, #256
    sub     x3, x3, #1
    cbnz    x3, 0b
    /* kis blokk másolása */
    cbz     x2, 2f
1:  and     x3, x0, #0x7
    cbnz    x3, 1f
    and     x3, x1, #0x7
    cbnz    x3, 1f
    lsr     x4, x2, #3
    cbz     x4, 1f
    and     x2, x2, #0x7
0:  ldr     x3, [x1], #8
    str     x3, [x0], #8
    sub     x4, x4, #1
    cbnz    x4, 0b
    cbz     x2, 2f
1:  ldrb    w3, [x1], #1
    strb    w3, [x0], #1
    sub     x2, x2, #1
    cbnz    x2, 1b
2:  ret
endf

/**
 * memória feltötlése adott karakterrel, architektúra specifikus implementáció
 *
 * void *memset(void *dst, uint8_t chr, size_t len)
 */
func memset
    /* paraméterek ellenőrzése */
    cbz     x0, 2f
    cbz     x2, 2f
    and     x3, x0, #0x7
    cbnz    x3, 1f
    and     x3, x2, #0xFFFFFFFFFFFFFFF0
    cbz     x3, 1f
    /* feltöltés 16 bájtosával */
    and     x1, x1, #0xFF
    orr     x1, x1, x1, lsl #8
    orr     x1, x1, x1, lsl #16
    orr     x1, x1, x1, lsl #32
    lsr     x3, x2, #4
    and     x2, x2, #0xF
0:  stp     x1, x1, [x0], #16
    sub     x3, x3, #1
    cbnz    x3, 0b
    /* kis blokk feltöltése */
    cbz     x2, 2f
1:  strb    w1, [x0], #1
    sub     w2, w2, #1
    cbnz    x2, 1b
2:  ret
endf

/**
 * memória összehasonlítása, architektúra specifikus implementáció
 *
 * int memcmp(const void *s1, const void *s2, size_t len)
 */
func memcmp
    mov     x3, x0
    mov     x0, xzr
    cbz     x3, 2f
    cbz     x1, 2f
    cbz     x2, 2f
    mov     x4, xzr
1:  ldrb    w0, [x3], #1
    ldrb    w4, [x1], #1
    sub     w0, w0, w4
    cbnz    w0, 2f
    sub     x2, x2, #1
    cbnz    x2, 1b
2:  ret
endf
