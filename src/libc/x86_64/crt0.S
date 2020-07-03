/*
 * libc/x86_64/crt0.S
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
 * @brief Zéró szintű C futtatás (x86_64-crt0)
 *
 * Hívási konvenció: SystemV AMD64 ABI
 *  integer vagy mutató: RDI, RSI, RDX, RCX, R8, R9
 *  lebegő pont: XMM0-XMM7
 *  meg kell őrizniük: RBP, RBX
 *  visszatérési érték: RAX / RDX:RAX
 *  veremkeret: RBP
 *  visszatérési cím: [RSP]
 *  veremigazítás függvény első utasításánál: %15 = 8
 */

.global _start
.type _start, STT_FUNC
.size _start, 2f-_start+4f-3f+6f-5f
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
    leaq    _GLOBAL_OFFSET_TABLE_(%rip), %r15
1:  popq    %rax
    addq    $8, %rsp
    orq     %rax, %rax
    jz      2f
    call    *%rax
    jmp     1b
2:  /* linkelő ide szerkeszti a ctors és .init szekciókat */

.section .crt0.init, "ax"
    /* parancssori paraméterek betöltése */
3:  movq    (%rsp), %rdi
    movq    8(%rsp), %rsi
    movq    16(%rsp), %rdx
    /* main(argc,argv,envp) hívása */
    call    main
    pushq   %rax
4:  /* linkelő ide szerkeszti a .fini szekciókat és a dtors-okat */

.section .crt0.fini, "ax"
5:  popq    %rdi
    jmp     exit@PLT
6:
