/*
 * include/osZ/types.h
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
 * @brief OS/Z specifikus rendszer típusok
 */

#ifndef _TYPES_H
#define _TYPES_H    1

/*** láthatóság ***/
/* helyben linkelt, futás időben linkelt (so), hívható más taszkokból és üzenetekből is */
#define public __attribute__ ((__visibility__("default")))
/* csak helyben linkelt objektumokból elérhető (nem so) */
#define private __attribute__ ((__visibility__("hidden")))
/* helyben linkelt, futás időben linkelt, de másik címtérből nem hivható */
#define protected __attribute__ ((__visibility__("protected")))
/* címigazított, gyors memcpy()-hoz */
#define alignmem __attribute__ ((aligned(16)))

/*** általános definíciók ***/
#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef null
#define null NULL
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef true
#define true 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef false
#define false 0
#endif

#define offsetof(t,m) __builtin_offsetof(t,m)

/* ANSI C kompatíbilis trükk fordítás idejű ellenőrzésekhez */
#define c_assert(c) extern char cassert[(c)?1:-1]

/*** hozzáférési bitek a uuid.Data4[7]-ben ***/
#define A_READ    (1<<0)
#define A_WRITE   (1<<1)
#define A_EXEC    (1<<2)    /* futtatás vagy keresés */
#define A_APPEND  (1<<3)
#define A_DELETE  (1<<4)
#define A_SUID    (1<<6)    /* felhasználó beállítása futtatáskor */
#define A_SGID    (1<<7)    /* ACL öröklés, nincs csoport OS/Z-ben */

/*** libc ***/
#ifdef _AS
.macro func name
.global \name
.type \name\(), STT_FUNC
.size \name\(), 9999f-.
\name\():
.endm

.macro endf
9999:
.endm
#else

typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} __attribute__((packed)) uuid_t;
#define UUID_ACCESS(a) (a.Data4[7])

typedef unsigned char uchar;
typedef unsigned int uint;
typedef uint8_t bool_t;     /* logikai típus, true (igaz) vagy false (hamis) */
typedef uint64_t size_t;    /* buffer méret */
typedef uint64_t fid_t;     /* fájl index */
typedef uint64_t ino_t;     /* inode szám */
typedef uint64_t dev_t;     /* eszköz index */
typedef uuid_t gid_t;       /* ACE, csoport azonosító */
typedef uint32_t mode_t;    /* hozzáférési mód */
typedef uint64_t nlink_t;   /* hivatkozások száma */
typedef uuid_t uid_t;       /* felhasználó azonosító */
typedef int64_t off_t;      /* offszet */
typedef uint64_t pid_t;     /* processz azonosító */
typedef uint64_t id_t;      /* általános azonosító */
typedef uint32_t blksize_t; /* blokk méret */
typedef uint64_t blkcnt_t;  /* blokkszám */
typedef uint64_t fpos_t;    /* fájl pozíció */
typedef uint32_t keymap_t;  /* billkiosztás típus */
typedef uint64_t time_t;    /* időbélyeg mikroszekundumban */
typedef uint16_t dpy_t;     /* képernyő azonosító */
typedef uint64_t phy_t;     /* fizikai cím */
typedef uint64_t virt_t;    /* virtuális cím */
typedef uint64_t evt_t;     /* eseménytípus */

typedef struct {
    uint64_t ret0;
    uint64_t ret1;
} retdq_t;

typedef struct {
    uint64_t free;
    uint64_t total;
} meminfo_t;

/* ablak struktúra */
typedef struct {
    uint32_t    id;
    uint32_t    w;
    uint32_t    h;
    uint32_t    type;
    void        *gc;
    void        *data;
} win_t;

typedef struct {
    dev_t       st_dev;     /* ID of device, *not* major/minor */
    ino_t       st_ino;     /* inode of the file */
    mode_t      st_mode;    /* mode */
    uint8_t     st_type[4]; /* file type */
    uint8_t     st_mime[44];/* mime type */
    nlink_t     st_nlink;   /* number of hard links */
    uid_t       st_owner;   /* owner user id */
    off_t       st_size;    /* file size */
    blksize_t   st_blksize; /* block size */
    blkcnt_t    st_blocks;  /* number of allocated blocks */
    time_t      st_atime;   /* access time in microsec timestamp */
    time_t      st_mtime;   /* modification time in microsec timestamp */
    time_t      st_ctime;   /* status change time in microsec timestamp */
} stat_t;

typedef struct {
    dev_t       d_dev;      /* ID of device, *not* major/minor */
    ino_t       d_ino;      /* inode number */
    uint8_t     d_icon[8];  /* short mime type for icon */
    uint64_t    d_filesize; /* file size */
    uint32_t    d_type;     /* entry type, st_mode >> 16 */
    uint32_t    d_len;      /* name length */
    char        d_name[FILENAME_MAX];
} dirent_t;

/*
typedef void __signalfn(int);
typedef __signalfn *sighandler_t;
*/
typedef void (*sighandler_t) (int);

/* type returned by syscalls mq_call() and mq_recv() */
typedef struct {
    evt_t evt;     /* MSG_DEST(pid) | MSG_FUNC(funcnumber) | MSG_PTRDATA */
    union {
        /* !MSG_PTRDATA, only scalar values */
        struct {
            uint64_t arg0;
            uint64_t arg1;
            uint64_t arg2;
            uint64_t arg3;
            uint64_t arg4;
            uint64_t arg5;
        } scalar;
        /* MSG_PTRDATA, buffer mapped along with message */
        struct {
            void *ptr;      /* Buffer address if MSG_PTRDATA, otherwise undefined */
            uint64_t size;  /* Buffer size if MSG_PTRDATA */
            uint64_t type;  /* Buffer type if MSG_PTRDATA */
            uint64_t attr0;
            uint64_t attr1;
            uint64_t attr2;
        } ptr;
    } data;
    uint64_t serial;
} __attribute__((packed)) msg_t;
/* bits in evt: (63)TTT..TTT P FFFFFFFFFFFFFFF(0)
 *  where T is a task id or subsystem id, P true if message has a pointer,
 *  F is a function number from 1 to 32766. Function numbers 0 and 32767 are reserved. */
#define EVT_DEST(t)   ((uint64_t)(t)<<16)
#define EVT_SENDER(m) ((pid_t)((m)>>16))
#define EVT_FUNC(m)   ((uint16_t)((m)&0x3FFF))
#define MSG_REGDATA   (0)
#define MSG_PTRDATA   (0x4000)
#define MSG_RESPONSED (0x8000)
#define MSG_PTR(m)    (m.arg0)
#define MSG_SIZE(m)   (m.arg1)
#define MSG_MAGIC(m)  (m.arg2)
#define MSG_ISREG(m)  (!((m)&MSG_PTRDATA))
#define MSG_ISPTR(m)  ((m)&MSG_PTRDATA)
#define MSG_ISRESP(m) ((m)&MSG_RESPONSED)
#endif

#endif /* types.h */
