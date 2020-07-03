/*
 * libc/aarch64/crt0.S
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
 * @subsystem libc
 * @brief Zéró szintű C futtatás (aarch64-crt0)
 *
 * Hívási konvenció: SystemV ARM64 ABI
 *  integer vagy mutató: X0, X1, X2, X3, X4, X5, X6, X7
 *  visszatérési érték: X0 / X1:X0
 *  veremkeret: X29
 *  visszatérési cím: X30
 *  veremigazítás függvény első utasításánál: %15 = 0
 */

.global _start
.type _start, STT_FUNC
.size _start, 1f-_start+3f-2f+5f-4f
.extern main
.type main, STT_FUNC
.extern exit
.type exit, STT_FUNC

/*
 * Futtatás menete:
 *  1. opcionális megosztott könyvtárak _start-jai meghívódnak egyesével
 *  2. ctors-ok és .init szekciók meghívódnak
 *  3. a fő main() hívása
 *  4. .fini szekciók és dtors-ok hívása
 *  5. ugrás a libc-ben definiált exit() föggvényre
 */
.section .crt0, "ax"
    /* belépési pont */
_start:
    /* függvénykönyvtárak inicializálása */
    ldr     x0, [sp], #16
    cbz     x0, 1f
    blr     x0
    b       _start
1:  /* linkelő ide szerkeszti a ctors és .init szekciókat */

.section .crt0.init, "ax"
    /* parancssori paraméterek betöltése */
2:  ldp     x0, x1, [sp]
    ldr     x2, [sp, #16]
    /* main(argc,argv,envp) hívása */
    bl      main
    str     x0, [sp, #-16]!     /* push x0 */
3:  /* linkelő ide szerkeszti a .fini szekciókat és a dtors-okat */

.section .crt0.fini, "ax"
4:  ldr     x0, [sp], #16       /* pop x0 */
    b       exit
5:
