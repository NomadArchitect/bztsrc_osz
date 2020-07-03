/*
 * core/x86_64/platform.S
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
 * @brief Architektúra függő, platform specifikus függvények
 */

#include <arch.h>

.section .data
nocpu:
.asciz "CPU feature %a not supported"

.section .text

/**
 * CPU üresjárata
 */
func platform_idle
1:  hlt
    cmpb    $0, LDYN_usrccb + ccb_hasatask
    jz      1b
    xorq    %rax, %rax
    mov     $SYS_yield, %al
    syscall
    jmp     1b
endf

/**
 * CPU ellenőrzése
 */
func platform_cpu
    push    %rbx
    xorq    %rax, %rax
    incb    %al
    cpuid
    /* minimum processzor család 6 */
    movq    $0x796C696D6166, %rsi
    cmp     $0x0600, %ax
    jb      1f
    /* FXSR */
    movq    $0x52535846, %rsi
    bt      $24, %edx
    jnc     1f
    /* TSC */
    movq    $0x00435354, %rsi
    bt      $4, %edx
    jnc     1f
    /* MSR */
    movq    $0x0052534D, %rsi
    bt      $5, %edx
    jnc     1f
    /* PAT */
    movq    $0x00544150, %rsi
    bt      $16, %edx
    jnc     1f
    /* SSSE3 (alpha számításhoz kell) */
#if OPTIMIZE == 1
/*
    movq    $0x3345535353, %rsi
    bt      $9, %ecx
    jnc     1f
*/
#endif
#if __x2apic__
    /* x2APIC */
    movq    $0x434950413278, %rsi
    bt      $21, %ecx
    jnc     1f
#endif
#if OPTIMIZE == 1
    /* AVX */
    movq    $0x00585641, %rsi
    bt      $28, %ecx
    jnc     1f
#endif

    movl    $0x80000001, %eax
    cpuid
    /* NX */
    movq    $0x0000584E, %rsi
    bt      $20, %edx
    jnc     1f
    /* SYSCALL */
    movq    $0x4C4C4143535953, %rsi
    bt      $11, %edx
    jnc     1f
    pop     %rbx
    ret
    /* funkció nem támogatott */
1:  movq    $nocpu, %rdi
    call    kpanic
endf

/**
 * eldönti egy címről, hogy elérhető-e, lehet, hogy nincs leképezve
 *
 * bool_t platform_memfault(void *ptr)
 */
func platform_memfault
    movq    %rdi, %rsi
    movb    $0, sys_fault
    lodsb
    xorq    %rax, %rax
    movb    %al, sys_fault
    ret
endf

/**
 * syscall hívás
 * Be:
 *    funkció: rax
 *    paraméterek: rdi, rsi, rdx, r10, r8, r9
 *    visszatérési cím: rcx, flagek: r11
 * Ki:
 *    visszatérési érték: rdx:rax
 *    hibakód: rdi
 */
func platform_syscall
    cli
    cld
    /* nem mentünk teljes állapotot, csak a PC-t, SP-t, híváskódot és a kötelező callee-saved regisztereket */
    movq    %rcx, TCBFLD_PC
    movq    %r11, TCBFLD_FLAGS
    movq    %rsp, TCBFLD_SP
    movq    %r10, %rcx
    /* átkapcsolunk taszk független, CPU-nkénti core veremre */
    movq    $LDYN_ccb + __PAGESIZE, %rsp
    call    platform_save2
    pushq   %r12
    pushq   %rax
    /* üzenet fogadás? */
    orq     %rax, %rax
    jnz     1f
    call    msg_recv
    /* ha nem volt üzenet, akkor blokkolunk és yield() */
    orq     %rax, %rax
    jnz     6f
    subq    $2, TCBFLD_PC
    xor     %rdi, %rdi
    call    sched_block
    jmp     2f
1:  cmpq    $SYS_yield, %rax
    jne     3f
2:  call    sched_pick
    jmp     5f
3:  /* a core-nak szól az üzenet? */
    cmpq    $0xFFFF, %rax
    ja      4f
    call    msg_core
    jmp     5f
4:  call    msg_send
5:  /* csak akkor szabad betölteni az állapotot, ha történt taszkkapcsolás */
    cmpb    $0, LDYN_ccb + ccb_flags
    jz      6f
    movb    $0, LDYN_ccb + ccb_flags
    /* át kell kapcsolnunk taszkonkénti core veremre */
    movq    $TCBFLD_PC, %rsp
    call    platform_load
    iretq
6:  call    platform_load2
    movq    LDYN_ccb + ccb_core_errno, %rdi
    movq    TCBFLD_PC, %rcx
    movq    TCBFLD_FLAGS, %r11
    movq    TCBFLD_SP, %rsp
    sysretq
endf

/**
 * CPU állapotának lementése
 */
func platform_save
    movq    %rcx, TCBFLD_GPR +  16
    movq    %rdx, TCBFLD_GPR +  24
    movq    %rsi, TCBFLD_GPR +  32
    movq    %r8,  TCBFLD_GPR +  48
    movq    %r9,  TCBFLD_GPR +  56
    movq    %r10, TCBFLD_GPR +  64
    movq    %r11, TCBFLD_GPR +  72
    movq    %r12, TCBFLD_GPR +  80
platform_save2:
    movq    %rax, TCBFLD_GPR +   0
    movq    %rbx, TCBFLD_GPR +   8
    movq    %rdi, TCBFLD_GPR +  40
    movq    %r13, TCBFLD_GPR +  88
    movq    %r14, TCBFLD_GPR +  96
    movq    %r15, TCBFLD_GPR + 104
    movq    %rbp, TCBFLD_GPR + 112
/*
#if OPTIMIZE == 1
    xorl    %eax, %eax
    decl    %eax
    mov     %eax, %edx
    xsave   TCBFLD_FP
#else
*/
    fxsave  TCBFLD_FP
/*
#endif
*/
    ret
endf

/**
 * CPU állapotának visszatöltése
 */
func platform_load
    movq    TCBFLD_GPR +   0, %rax
    movq    TCBFLD_GPR +  16, %rcx
    movq    TCBFLD_GPR +  24, %rdx
    movq    TCBFLD_GPR +  32, %rsi
    movq    TCBFLD_GPR +  40, %rdi
    movq    TCBFLD_GPR +  48, %r8
    movq    TCBFLD_GPR +  56, %r9
    movq    TCBFLD_GPR +  64, %r10
    movq    TCBFLD_GPR +  72, %r11
    movq    TCBFLD_GPR +  80, %r12
platform_load2:
    movq    TCBFLD_GPR +   8, %rbx
    movq    TCBFLD_GPR +  88, %r13
    movq    TCBFLD_GPR +  96, %r14
    movq    TCBFLD_GPR + 104, %r15
    movq    TCBFLD_GPR + 112, %rbp
/*
#if OPTIMIZE == 1
    pushq   %rax
    xorl    %eax, %eax
    decl    %eax
    mov     %eax, %edx
    xrstor  TCBFLD_FP
    popq    %rax
#else
*/
    fxrstor TCBFLD_FP
/*
#endif
*/
    ret
endf

/**
 * véletlenszám generátor inicializálása
 */
func platform_srand
    movq    %rbx, %r8
    movq    $srand, %rdi
    movq    $LOAD_ADDRESS, %rsi
    movw    $512, %cx
1:  lodsq
    addq    %rax, (%rdi)
    subq    %rax, 8(%rdi)
    xorq    %rax, 16(%rdi)
    addq    %rax, 24(%rdi)
    dec     %cx
    jnz     1b
    call    kentropy
    movq    $srand, %rdi
    xorq    %rax, %rax
    incb    %al
    cpuid
    xorq    %rax, (%rdi)
    xorq    %rbx, 8(%rdi)
    xorq    %rcx, 16(%rdi)
    xorq    %rdx, 24(%rdi)
    /* ha az rdrand támogatott használjuk */
    bt      $30, %ecx
    jnc     1f
    rdrand  %rbx
    xorq    %rbx, (%rdi)
    rdrand  %rbx
    addq    %rbx, 8(%rdi)
    rdrand  %rbx
    subq    %rbx, 16(%rdi)
    rdrand  %rbx
    xorq    %rbx, 24(%rdi)
1:  movq    %r8, %rbx
    call    kentropy
    ret
endf

/**
 * korai konzol implementáció, a kprintf használja
 */
func platform_waitkey
    xorb    %ah, %ah
0:  xorb    %al, %al
    xorb    %cl, %cl
    /* van leütött billentyű? */
1:  inb     $0x64, %al
    bt      $5, %ax
    jc      1b
    bt      $0, %ax
    jc      2f
    pause
#if DEBUG
    movw    $0x3fd, %dx
    inb     %dx, %al
    /* van egyáltalán soros portunk? */
    cmpb    $0xFF, %al
    je      1b
    /* várakozik fogadott bájt? */
    andb    $1, %al
    jz      1b
    subb    $5, %dl
    inb     %dx, %al
    orb     %al, %al
    jz      1b
    movb    $0x80, %ah
    jmp     8f
#else
    jmp     1b
#endif
    /* igen, karakterkód (scancode) kiolvasása */
2:  inb     $0x60, %al
    orb     %al, %al
    jz      1b
    movb    %al, %dl
#if DEBUG
    /* ne fogadja el az LAlt gombot. Akkor használom, amikor ALT+Q-t vagy ALT-F4-et ütök a bochs-ban :-) */
    cmpb    $0x38, %dl
    je      0b
    cmpb    $0x38+0x80, %dl
    je      0b
    /* shift gomb kezelése a debugger számára */
    cmpb    $54, %dl
    je      2f
    cmpb    $42, %dl
    jne     1f
2:  movb    $1, dbg_isshft
    jmp     0b
1:  cmpb    $54+0x80, %dl
    je      2f
    cmpb    $42+0x80, %dl
    jne     3f
2:  movb    $0, dbg_isshft
    jmp     0b
3:
#endif
    /* kiterjesztett karakterkód ellenőrzése */
    cmpb    $0xE0, %dl
    jne     1f
    movb    $1, %ah
    jmp     0b
1:  cmpb    $0xE1, %dl
    jne     1f
    movb    $2, %ah
    jmp     0b
1:  cmpb    $0xFF, %dl
    jne     1f
    movb    $3, %ah
    jmp     0b
    /* addig várunk, míg felengedés nem jön */
1:  btr     $7, %dx
    jc      platform_waitkey
    /* aztán meg addig míg jön. qemu néha duplán küldi a karaktereket */
1:  inb     $0x64, %al
    bt      $0, %ax
    jnc     7f
    pause
    inb     $0x60, %al
    jmp     1b
7:  movb    %dl, %al
8:  /* kiterjesztett karakterkód ax-ban */
    ret
endf
