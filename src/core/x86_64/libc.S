/*
 * core/x86_64/libc.S
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
1:  wbinvd
    lock
    btsq    %rdi, (%rsi)
    jnc     1f
    pause
    jmp     1b
1:  ret
endf

/**
 * jelző felszabadítása
 *
 * void lockrelease(int bit, uint64_t *ptr);
 */
func lockrelease
    lock
    btrq    %rdi, (%rsi)
    ret
endf

/**
 * memória másolása, architektúra specifikus implementáció
 *
 * void *memcpy(void *dst, const void *src, size_t len)
 */
func memcpy
    cld
    /* bemenő paraméterek ellenőrzése */
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
    /* ha kis adatmennyiség */
    cmpq    $512, %rdx
    jb      1f
    /* ha igazított mind a forrás, mind a cél */
    movb    %sil, %al
    xorb    %dil, %al
    andb    $15, %al
    jnz     1f
    /* nagy blokk másolása, 256 bájtosával */
0:  movq    %rdx, %rcx
    xorq    %rdx, %rdx
    movb    %cl, %dl
    shrq    $8, %rcx
0:  prefetchnta 256(%rsi)
    prefetchnta 288(%rsi)
    prefetchnta 320(%rsi)
    prefetchnta 352(%rsi)
    prefetchnta 384(%rsi)
    prefetchnta 416(%rsi)
    prefetchnta 448(%rsi)
    prefetchnta 480(%rsi)
    movdqa    0(%rsi), %xmm0
    movdqa   16(%rsi), %xmm1
    movdqa   32(%rsi), %xmm2
    movdqa   48(%rsi), %xmm3
    movdqa   64(%rsi), %xmm4
    movdqa   80(%rsi), %xmm5
    movdqa   96(%rsi), %xmm6
    movdqa  112(%rsi), %xmm7
    movdqa  128(%rsi), %xmm8
    movdqa  144(%rsi), %xmm9
    movdqa  160(%rsi), %xmm10
    movdqa  176(%rsi), %xmm11
    movdqa  192(%rsi), %xmm12
    movdqa  208(%rsi), %xmm13
    movdqa  224(%rsi), %xmm14
    movdqa  240(%rsi), %xmm15
    movntdq %xmm0,   0(%rdi)
    movntdq %xmm1,  16(%rdi)
    movntdq %xmm2,  32(%rdi)
    movntdq %xmm3,  48(%rdi)
    movntdq %xmm4,  64(%rdi)
    movntdq %xmm5,  80(%rdi)
    movntdq %xmm6,  96(%rdi)
    movntdq %xmm7, 112(%rdi)
    movntdq %xmm8, 128(%rdi)
    movntdq %xmm9, 144(%rdi)
    movntdq %xmm10,160(%rdi)
    movntdq %xmm11,176(%rdi)
    movntdq %xmm12,192(%rdi)
    movntdq %xmm13,208(%rdi)
    movntdq %xmm14,224(%rdi)
    movntdq %xmm15,240(%rdi)
    addq    $256, %rsi
    addq    $256, %rdi
    decq    %rcx
    jnz     0b
    /* kis blokk másolása */
1:  movq    %rdx, %rcx
    shrq    $3, %rcx
    or      %rcx, %rcx
    jz      1f
    repnz   movsq
1:  movb    %dl, %cl
    andb    $0x7, %cl
    jz      2f
    repnz   movsb
2:  movq    %rdi, %rax
    ret
endf

/**
 * memória feltötlése adott karakterrel, architektúra specifikus implementáció
 *
 * void *memset(void *dst, uint8_t chr, size_t len)
 */
func memset
    cld
    orq     %rdx, %rdx
    jz      2f
    orq     %rdi, %rdi
    jz      2f
    xorq    %rax, %rax
    movb    %sil, %al
    movq    $0x101010101010101, %rcx
    imul    %rcx, %rax
    movq    %rdx, %rcx
    shrq    $3, %rcx
    orq     %rcx, %rcx
    jz      1f
    repnz   stosq
1:  movb    %dl, %cl
    andb    $0x7, %cl
    repnz   stosb
2:  movq    %rdi, %rax
    ret
endf

/**
 * memória összehasonlítása, architektúra specifikus implementáció
 *
 * int memcmp(const void *s1, const void *s2, size_t len)
 */
func memcmp
    cld
    xorq    %rax, %rax
    orq     %rdx, %rdx
    jz      1f
    cmpq    %rsi, %rdi
    je      1f
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    movq    %rdx, %rcx
    rep     cmpsb
    jz      1f
2:  incb    %al
1:  ret
endf
