/*
 * fs/fcb.h
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
 * @brief Fájl Kontroll Blokk
 */

#include <osZ.h>

#ifndef _FCB_H_
#define _FCB_H_ 1

/* lehetséges fájl típusok */
#define FCB_TYPE_REG_FILE   0
#define FCB_TYPE_REG_DIR    1
#define FCB_TYPE_DEVICE     2
#define FCB_TYPE_PIPE       3
#define FCB_TYPE_SOCKET     4
#define FCB_TYPE_UNION      5

#define FCB_FLAG_EXCL       (1<<0)

/* írási buffer */
typedef struct {
    fpos_t offs;
    size_t size;
    uint8_t *data;
    void *next;
} writebuf_t;

/* normál fájlok és könyvtárak */
typedef struct {
    fid_t storage;
    ino_t inode;
    fpos_t filesize;
    blksize_t blksize;
    writebuf_t *buf;
    uint8_t ver;
} __attribute__((packed)) fcb_reg_t;

/* könyvtár uniók */
typedef struct {
    fid_t storage;
    ino_t inode;
    fpos_t filesize;
    fid_t *unionlist;
} __attribute__((packed)) fcb_union_t;

/* eszközök */
typedef struct {
    fid_t storage;
    ino_t inode;
    fpos_t filesize;
    blksize_t blksize;
} __attribute__((packed)) fcb_dev_t;

typedef struct {
    uint64_t idx;
} __attribute__((packed)) fcb_pipe_t;

typedef struct {
    uint64_t idx;
} __attribute__((packed)) fcb_socket_t;

typedef struct {
    char *abspath;
    uint64_t nopen;
    uint8_t type;
    uint8_t flags;
    uint16_t fs;
    mode_t mode;
    union {
        fcb_reg_t reg;
        fcb_dev_t device;
        fcb_union_t uni;
        fcb_pipe_t pipe;
        fcb_socket_t socket;
    } data;
} __attribute__((packed)) fcb_t;

extern uint64_t nfcb;
extern uint64_t nfiles;
extern fcb_t *fcb;

extern fid_t fcb_get(char *abspath);
extern fid_t fcb_add(char *abspath, uint8_t type);
extern void fcb_del(fid_t idx);
extern void fcb_free();
extern char *fcb_readlink(fid_t idx);
extern fid_t fcb_unionlist_build(fid_t idx, void *buf, size_t size);
extern bool_t fcb_write(fid_t idx, off_t offs, void *buf, size_t size);
extern bool_t fcb_flush(fid_t idx);

#if DEBUG
extern void fcb_dump();
#endif

#endif
