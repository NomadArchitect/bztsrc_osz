/*
 * src/core/link.ld
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
 * @brief Linkelő szkript a magasmemóriába szerkesztett OS/Z core-hoz
 */

LOAD_ADDRESS = 0xffffffffffe00000;
STACK_LIMIT =  0xfffffffffffffc00;

ENTRY(_start)

PHDRS
{
  text PT_LOAD FILEHDR PHDRS;
}

SECTIONS
{
    /* LLVM lld bug, ne használj . += 4096; - ot, valamiért 0xffffffffffe01020 lesz an environment címe tőle... */
    bootboot = LOAD_ADDRESS;
    environment = LOAD_ADDRESS + 4096;
    . = SEGMENT_START("text-segment", LOAD_ADDRESS + 8192) + SIZEOF_HEADERS;
    .text . : {
        PROVIDE(__code = .);
        KEEP(*(.text.boot)) *(.text .text.startup .gnu.linkonce.t*) /* kód */
        *(.rodata .rodata.* .gnu.linkonce.r*)                       /* csak olvasható adat */
        . = ALIGN(2048);
        KEEP(*(.text.aligned))   /* kerülőmegoldás kód szegmens igazításra */
        . = ALIGN(4096);
        PROVIDE(__data = .);                                        /* írható adat */
        *(.data .data.* .gnu.linkonce.d*)
    } :text
    . += 0x4000;  /* ld bug kikerülése. Nem teszi a symtab-ot és strtab-ot a text szegmensbe, és a bss máskülönben belelógna */

    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss)
        *(COMMON)
        . = ALIGN(4096);
        KEEP(*(.bss.aligned))
        __bss_end = .;
        . = ALIGN(4096);
        pmm_entries_buf = .;   /* a Fizikai Memória Kezelő szabad memória listája */
    } :text

/* szekciók, amikre nincs szükség az OS/Z esetében */
    /DISCARD/ : { *(.note*) *(.gnu*) *(.comment*) *(.interp) *(.eh*) }
}
