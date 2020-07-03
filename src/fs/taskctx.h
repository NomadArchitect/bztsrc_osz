/*
 * fs/taskctx.h
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
 * @brief taszk kontextus fájlrendszer szolgáltatásokhoz
 */

#include <osZ.h>

/* bit 12-15 belső használatú jelzők */
#define OF_MODE_TTY     (1<<14)
#define OF_MODE_READDIR (1<<15)
/* bit 16- hibaállapot jelzők */
#define OF_MODE_EOF     (1<<16)

/* egy nyitott fájlt ír le */
typedef struct {
    fid_t fid;
    mode_t mode;
    fpos_t offs;
    uint32_t unionidx;
    uint32_t trid;
} openfiles_t;

/* fájl műveletek taszk kontextusa */
typedef struct {
    pid_t pid;              /* taszk azonosító */
    fid_t rootdir;          /* gyökér könyvtár fcb */
    fid_t workdir;          /* munkakönyvtár fcb */
    size_t workoffs;        /* blokkcímre igazított offszet íráshoz / olvasáshoz */
    size_t workleft;        /* fennmaradó bájtok száma írásnál / olvasásnál */
    uint64_t nopenfiles;    /* megnyitott fájlok száma */
    uint64_t nfiles;        /* ténylegesen nyitott fájlok száma */
    openfiles_t *openfiles; /* nyitott fájlok tömb */
    msg_t msg;              /* felfüggesztett üzenet */
    void *next;             /* következő taszk kontextus */
} taskctx_t;

/* taszk kontextusok */
extern uint64_t ntaskctx;
extern taskctx_t *taskctx[];

/* aktuális kontextus */
extern taskctx_t *ctx;

/* taszk kontextus műveletek */
extern taskctx_t *taskctx_get(pid_t pid);
extern void taskctx_del(pid_t pid);
extern void taskctx_rootdir(taskctx_t *tc, fid_t fid);
extern void taskctx_workdir(taskctx_t *tc, fid_t fid);
extern uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs, uint64_t idx);
extern bool_t taskctx_close(taskctx_t *tc, uint64_t idx, bool_t dontfree);
extern bool_t taskctx_seek(taskctx_t *tc, uint64_t idx, off_t offs, uint8_t whence);
extern bool_t taskctx_validfid(taskctx_t *tc, uint64_t idx);
extern dirent_t *taskctx_readdir(taskctx_t *tc, fid_t idx, void *ptr, size_t size);
extern size_t taskctx_read(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size);
extern size_t taskctx_write(taskctx_t *tc, fid_t idx, void *ptr, size_t size);

#if DEBUG
extern void taskctx_dump();
#endif
