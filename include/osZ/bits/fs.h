/*
 * include/osZ/bits/fs.h
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
 * @brief OS/Z Fájlrendszer szolgáltatás (rendszerhívás kódok)
 */

/* blokkgyorsítótár */
#define SYS_nomem       ( 1)
#define SYS_setblock    ( 2)
#define SYS_ackblock    ( 3)
#define SYS_mmap        ( 4)
#define SYS_munmap      ( 5)
/* fájlrendszerek */
#define SYS_mountfs     ( 6)
#define SYS_mount       ( 7)
#define SYS_umount      ( 8)
#define SYS_fsck        ( 9)
/* fájl múveletek */
#define SYS_mknod       (10)
#define SYS_ioctl       (11)
#define SYS_lstat       (12)
#define SYS_dstat       (13)
#define SYS_fstat       (14)
#define SYS_fopen       (15)
#define SYS_fseek       (16)
#define SYS_ftell       (17)
#define SYS_fread       (18)
#define SYS_fwrite      (19)
#define SYS_fflush      (20)
#define SYS_feof        (21)
#define SYS_ferror      (22)
#define SYS_fclrerr     (23)
#define SYS_fclose      (24)
#define SYS_fcloseall   (25)
#define SYS_tmpfile     (26)
#define SYS_dup         (27)
#define SYS_dup2        (28)
#define SYS_link        (29) /**/
#define SYS_symlink     (30) /**/
#define SYS_unlink      (31) /**/
#define SYS_rename      (32) /**/
#define SYS_realpath    (33)
#define SYS_purge       (34) /* remove old versions */
#define SYS_setattr     (35) /* set file attributes like flags and meta */
#define SYS_getattr     (36) /**/
/* könyvtár műveletek */
#define SYS_chroot      (37)
#define SYS_chdir       (38)
#define SYS_getcwd      (39)
#define SYS_mkdir       (40) /**/
#define SYS_mkunion     (41) /**/
#define SYS_opendir     (42)
#define SYS_readdir     (43)
#define SYS_rewind      (44)
#define SYS_readlink    (45)
/* csővezeték műveletek */
#define SYS_mkfifo      (46) /**/
#define SYS_pipe        (47) /**/
#define SYS_popen       (48) /**/
#define SYS_pclose      (49) /**/
#define SYS_getty       (50) /* tty terminál hozzárendelése stdin/stdout/stderr hármashoz */
#define SYS_evtty       (51) /* tty esemény, másik taszk stdin csővezetékébe írás */
/* socket operations */

#if DEBUG
#define SYS_fsdump     (127)
#endif
