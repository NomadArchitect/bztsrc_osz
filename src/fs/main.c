/*
 * fs/main.c
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
 * @subsystem fájlrendszer
 * @brief FS taszk
 */

#include <osZ.h>
#include "../libc/libc.h"

public char *_fstab_ptr = NULL;     /* mutató az /etc/fstab adataira (valós idejű linkelő tölti ki) */
public size_t _fstab_size = 0;      /* fstab mérete (valós idejű linkelő tölti ki) */
public uint64_t pathmax;            /* elérési út maximális hossza */
extern uint64_t cachelines;         /* blokkgyorsítórár vonalainak száma */
public uint64_t cachelimit;         /* ha ennyi százaléknál kevesebb a szabad RAM, fel kell szabadítani a blokkgyorsítótárat */
bool_t ackdelayed;                  /* asszinkron blokk olvasás jelzőbitje */
bool_t nomem = false;               /* gyorsítótárat fell kell szabadítani jelzés */

extern stat_t st;                   /* tty csővezetétekhez kell */

public int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
/*
    msg_t *msg;
    char *str;

    str=strcasedup("ÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP");
    dbg_printf("Hello %d %d %d %s %d\n",1,2,3,"world",4);
    dbg_printf("megint %s %d %s\n%s", str, argc,argv[0],&_libc.osver);
*/
    while(1)
        mq_recv();
}

