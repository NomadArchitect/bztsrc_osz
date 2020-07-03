/*
 * core/x86_64/ibmpc/platform.c
 *
 * Copyright (c) 2016 bzt (bztsrc@gitlab)
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
 * @brief Platform specifikus funkciók
 */

#include <arch.h>

/**
 * platform függő környezeti változó alapértékek, env_init() hívja
 */
void platform_env()
{
    systables[systable_pcie_idx] = 0;   /* mp-t nem használjuk, helyette a pcie címét tároljuk, de az most nincs */
    dmabuf = 256;                       /* lapokban, az 1M */
    bootboot.numcores = numcores = 1;   /* LAPIC hiányában csak egy CPU-t tudunk kezelni */
}

/**
 * platform függő környezeti értékek értelmezése, env_init() hívja
 */
unsigned char *platform_parse(unsigned char *env)
{
    /* nincs platform specifikus opció */
    return ++env;
}

/**
 * számítógép kikapcsolása, kprintf_poweroff() hívja
 */
void platform_poweroff()
{
    /* Bochs kikapcsolás hekk */
    char *s = "Shutdown";
    /* qemu-n is működik, ha -device isa-debug-exit,iobase=0x8900,iosoze=0x04 kapcsolóval indítjuk */
    while (*s) {
        __asm__ __volatile__("movw $0x8900, %%dx; outb %0, %%dx" : : "a"(*s++));
    }
    /* APM poweroff implementálása?
     * Nem biztos, hogy van BIOS, lehet UEFI-ről bootoltunk */
}

/**
 * számítógép újraindítása, kprintf_reboot() hívja
 */
void platform_reset()
{
    __asm__ __volatile__("movb $0xFE, %al; outb %al, $0x64");
}

/**
 * futtatás felfüggesztése
 */
void platform_halt()
{
    __asm__ __volatile__("1: cli; hlt; jmp 1b");
}

/**
 * felébresztés üresjáratból
 */
void platform_awakecpu(uint16_t cpuid __attribute__((unused)))
{
    /* nincs lapic, úgyhogy csak egy processzort tudunk kezelni, ami meg fut */
}

/**
 * soros vonali debug konzol inicializálása
 */
void platform_dbginit()
{
    /* ha DEBUG nélkül fordítjuk, akkor ennek egy üres függvénynek kell lennie */
#if DEBUG
    __asm__ __volatile__(
        "movw $0x3f9, %%dx;"
        "xorb %%al, %%al;outb %%al, %%dx;"               /* IER int off */
        "movb $0x80, %%al;addb $2,%%dl;outb %%al, %%dx;" /* LCR set divisor mode */
        "movb $1, %%al;subb $3,%%dl;outb %%al, %%dx;"    /* DLL divisor lo 115200 */
        "xorb %%al, %%al;incb %%dl;outb %%al, %%dx;"     /* DLH divisor hi */
        "incb %%dl;outb %%al, %%dx;"                     /* FCR fifo off */
        "movb $0x43, %%al;incb %%dl;outb %%al, %%dx;"    /* LCR 8N1, break on */
        "movb $0x8, %%al;incb %%dl;outb %%al, %%dx;"     /* MCR Aux out 2 */
        "xorb %%al, %%al;subb $4,%%dl;inb %%dx, %%al"    /* clear receiver/transmitter */
    :::"rax","rdx");
    /* debug regiszterek törlése */
    __asm__ __volatile__("xorq %%rax, %%rax; movq %%rax, %%dr6":::"rax");

}

/**
 * egy bájt küldése a debug konzolra
 */
void platform_dbgputc(uint8_t c)
{
    __asm__ __volatile__(
        "xorl %%ebx, %%ebx; movb %0, %%bl;"
#ifndef NOBOCHSCONSOLE
        /* bochs e9 port hack */
        "movb %%bl, %%al;outb %%al, $0xe9;"
#endif
        /* karakter kiírása a soros portra */
        "movl $10000,%%ecx;movw $0x3fd,%%dx;"
        /* küldő buffer üres? */
        "1:inb %%dx, %%al;pause;"
        "cmpb $0xff,%%al;je 2f;"
        "dec %%ecx;jz 2f;"
        "andb $0x20,%%al;jz 1b;"
        /* kiküldés */
        "subb $5,%%dl;movb %%bl, %%al;outb %%al, %%dx;2:"
    ::"r"(c):"rax","rbx","rcx","rdx");
#endif
}
