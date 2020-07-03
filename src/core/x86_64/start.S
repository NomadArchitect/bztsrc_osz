/*
 * core/x86_64/start.S
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
 * @brief Belépési pont
 */

#include <platform.h>               /* szükségünk van a PIC címére */

.section .text
.global _start
.type _start, STT_FUNC
.global idt64
.type idt64, STT_OBJECT
.size idt64, 10
.type gdt64, STT_OBJECT
.size gdt64, 10
.type gdt64_start, STT_OBJECT
.size gdt64_start, 0x38

/**********************************************************************
 *                   OS/Z betöltő belépési pontja                     *
 **********************************************************************/
_start:
    cli
    cld
xchg %bx,%bx
    /* szegmensregiszterek beállítása */
    lgdt    gdt64
    jmp     1f
    nop
1:  movl    $0x1b, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    $0x10, %eax
    movl    %eax, %ss
    /* taszk regiszter */
    movl    $0x28 + 3, %eax
    ltr     %ax
    /* megszakítástábla regiszter */
    lidt    idt64

    /* NX és SYSCALL engedélyezése */
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $0x801, %eax
    wrmsr
    /* STAR */
    xorq    %rcx, %rcx
    movl    $0xC0000081, %ecx
    xorl    %eax, %eax
    movl    $0x00130008, %edx
    wrmsr
    /* LSTAR */
    incl    %ecx
    movq    $platform_syscall, %rax
    movq    %rax, %rdx
    shrq    $32, %rdx
    wrmsr
    /* SFMASK */
    incl    %ecx
    incl    %ecx
    xorl    %eax, %eax
    wrmsr

    /* OSFXSR és TSC (valamint DBG) engedélyezése */
    movq    %cr4, %rax
#if DEBUG
    orw     $0x208, %ax
#else
    orw     $0x200, %ax
#endif
    andb    $0xFB, %al
    movq    %rax, %cr4

    /* helyet csinálunk a CPU-nkénti cr3-nak és SysV ABI szerint igazítjuk a vermet */
    xorq    %rax, %rax
    pushq   %rax

    lock    incw numcores

    /* AP leállítása egyelőre */
    incb    %al
    cpuid
    shrl    $24, %ebx
    cmpw    %bx, bootboot + 0xC /* bootboot.bspid */
    jne     1f
    /* PIC átprogramozása, hogy ne ütközzenek a kivételekkel. Ezt akkor is meg kell tenni, ha egyébként le van tiltva. */
    movb    $0x11, %al
    outb    %al, $PIC_MASTER
    outb    %al, $PIC_SLAVE
    movb    $0x20, %al
    outb    %al, $PIC_MASTER_DATA
    movb    $0x28, %al
    outb    %al, $PIC_SLAVE_DATA
    movb    $0x4, %al
    outb    %al, $PIC_MASTER_DATA
    movb    $0x2, %al
    outb    %al, $PIC_SLAVE_DATA
    movb    $0x1, %al
    outb    %al, $PIC_MASTER_DATA
    outb    %al, $PIC_SLAVE_DATA
    /* letiltjuk. A platform majd engedélyezi, ha kell neki */
    movb    $0xFF, %al
    outb    %al, $PIC_MASTER_DATA
    outb    %al, $PIC_SLAVE_DATA
    /* ugrás a C nyelvű main() funkcióra a 64 bites kódszegmensben */
    pushq   $0x08
    pushq   $main
    lretq

    /* LAPIC hiányában csak egy CPU-t tudunk kezelni */
#ifdef __ibmpc__
1:  hlt
    jmp     1b
#else
    /* várakozás, míg RUNLVL_COOP szintet el nem érjük, aztán ugrás ap_start-ra */
1:  wbinvd
    pause
    cmpb    runlevel, $RUNLVL_COOP
    jne     1b
    pushq   $0x08
    pushq   $ap_start
    lretq
#endif

/**********************************************************************
 *            Alkalmazás Processzor(ok) belépési pontja               *
 **********************************************************************/
ap_start:
    /* CPU-nkénti memórialeképezés betöltése */
    popq    %rax
    mov     %rax, %cr3
    /* átkapcsolunk taszk független, CPU-nkénti core veremre */
    movq    $LDYN_ccb + __PAGESIZE, %rsp
    /* engedélyezzük a processzorspecifikus funkciókat */
    call    platform_cpu
    /* kiválasztunk egy taszkot futásra */
    call    sched_pick
    /* átadjuk rá a vezérlést */
    movq    $TCBFLD_PC, %rsp
    xor     %rbp, %rbp
    iretq

    /* Megszakítás Leíró Tábla Mutató */
    .balign  16
idt64:
    .word   (32+224)*16-1
    .quad   idt

    /* Globális Leíró Tábla Mutató */
    .balign  16
gdt64:
    .word   0x37
    .quad   gdt64_start
    /* Globális Leíró Tábla */
    .balign  16
gdt64_start:
    .quad 0x0000000000000000    /* mindig nulla */
    .quad 0x002098000000ffff    /* 08 core CS */
    .quad 0x008092000000ffff    /* 10 core DS */
    .quad 0x0080f2000000ffff    /* 18 user DS */
    .quad 0x0020f8000000ffff    /* 20 user CS */
    .quad 0x8000890000000068    /* 28 tss, az LBSS_ccb-re kell mutasson */
    .quad 0x00000000ffffffff
