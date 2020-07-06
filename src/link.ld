/*
 * src/link.ld
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
 * @brief OS/Z linkelő szkript
 */

PHDRS
{
  text PT_LOAD FILEHDR PHDRS;
  data PT_LOAD;
  dynamic PT_DYNAMIC;
}

SECTIONS
{
/* kódszegmens. Ezt nem töltjük be mindig, csak linkeljük, mivel nem írható */
    . = SEGMENT_START("text-segment", 0) + SIZEOF_HEADERS;
    .text . + SIZEOF_HEADERS : {
        *(.crt0) *(.ctors) *(.init) *(.crt0.init) *(.fini) *(.dtors) *(.crt0.fini)
        *(.text .text.* .stub .gnu.linkonce.t.*) *(.plt) *(.iplt) *(.plt.got) *(.plt.bnd)
    } :text
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) *(.eh*) } :text
    .dynsym : { *(.dynsym) } :text
    .dynstr : { *(.dynstr) } :text
    .rela.dyn : { *(.rela.*) } :text

/* adatszegmens. Fontos, hogy laphatáron kezdődjön a jogosultságok miatt, ez lemásolódik minden címtérbe */
    . = ALIGN(4096);
    .got : { *(.got) *(.igot) } :data                       /* a GOT és GOT.PLT szekciót az adatokhoz tesszük, mert a binárisok */
    .dynamic : { *(.dynamic) } :data :dynamic               /* minden címtér esetén más és más címre töltődnek be (ASLR), ezért */
    .data : { *(.data .data.* .gnu.linkonce.d.*) } :data    /* címterenként más offszetekkel kell feltölteni ezeket a táblákat */

    .bss (NOLOAD) : { *(.bss .bss.* .sbss* .gnu.linkonce.b.*) *(COMMON) } :data

/* szekciók, amikre nincs szükség az OS/Z esetében */
    /DISCARD/ : { *(.note*) *(.gnu*) *(.comment*) *(.interp) }
}
