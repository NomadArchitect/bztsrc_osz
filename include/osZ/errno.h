/*
 * include/osZ/errno.h
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
 * @brief A rendelkezésre álló errno értékek
 */

#ifndef _ERRNO_H
#define _ERRNO_H 1

#ifndef _AS
extern void seterr(int errno);
extern int errno();
#endif

#define SUCCESS      0  /* nincs hiba */
#define EPERM        1  /* művelet nem engedélyezett */
#define EAGAIN       2  /* próbáld újra */
#define ESRCH        3  /* nincs ilyen processz */
#define EFAULT       4  /* hibás cím */
#define EINVAL       5  /* érvénytelen paraméter */
#define EBUSY        6  /* eszköz vagy erőforrás foglalt */
#define EACCES       7  /* hozzáférés megtagadva */
#define ENOMEM       8  /* elfogyott a memória */
#define ENOEXEC      9  /* futtatható fájl formátum probléma */
#define ERTEXEC     10  /* futási idejű linkelési hiba */
#define EEXIST      11  /* fájl már létezik */
#define ENOENT      12  /* nincs ilyen fájl vagy könyvtár */
#define ENODEV      13  /* nincs ilyen eszköz */
#define ENOTDIR     14  /* nem könyvtár */
#define EISDIR      15  /* ez egy könyvtár */
#define ENOTUNI     16  /* nem unió */
#define EISUNI      17  /* ez egy unió */
#define ENOFS       18  /* fájlrendszer nem található */
#define EBADFS      19  /* sérült fájlrendszer */
#define EROFS       20  /* írásvédett fájlrendszer */
#define ENOSPC      21  /* nincs elég szabad hely */
#define ENOTSUP     22  /* fájltípus nem támogatott ezen a fájlrendszeren */
#define EIO         23  /* B/K hiba */
#define EPIPE       24  /* törött csővezeték */
#define ESPIPE      25  /* érvénytelen pozicionálás */
#define ENOTSHM     26  /* nem megosztott memória buffer */
#define EBADF       27  /* hibás fájl sorszám */
#define ENOTBIG     28  /* buffer nem elég nagy */
/* muszáj az utolsó errno-nak lennie */
#define ENOTIMPL    29  /* nincs implementálva */

#define ERETRY      254 /* syscall automatikus újrahívása */
#define EUNKWN      255 /* ismeretlen ok */

#endif /* errno.h */
