/*
 * fs/pipe.h
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
 * @brief csővezetékek kezelése
 */

#include <osZ.h>

#ifndef _PIPE_H_
#define _PIPE_H_ 1

/* csővezeték buffer */
typedef struct {
    pid_t    reader;    /* kizárólagos eléréshez pid (csak egy olvasó lehet) */
    fid_t    fid;       /* névtelen csővezetékeknél -1 */
    uint64_t nopen;     /* írók és olvasók száma */
    size_t   rd;        /* olvasás mérete bájtokban */
    size_t   size;      /* buffer mérete */
    uint8_t  *data;     /* buffer */
    uid_t    owner;     /* csővezeték tulajdonosa */
    time_t   rdtime;    /* utolsó olvasás ideje */
    time_t   wrtime;    /* utolsó írás ideje */
} pipe_t;

extern uint64_t npipe;
extern pipe_t *pipe;

extern uint64_t pipe_add(fid_t fid, pid_t pid);
extern void pipe_del(uint64_t idx);
extern bool_t pipe_write(uint64_t idx, virt_t ptr, size_t size);
extern size_t pipe_read(uint64_t idx, virt_t ptr, size_t size, pid_t pid);
extern stat_t *pipe_stat(uint64_t idx, mode_t mode);

#if DEBUG
extern void pipe_dump();
#endif

#endif
