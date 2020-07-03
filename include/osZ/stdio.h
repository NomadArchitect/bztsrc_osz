/*
 * include/osZ/stdio.h
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
 * @brief ISO C99 Sztandard: 7.19 ki- és bemenet, valamint minden fájl és könyvtár manipuláció
 */

#ifndef _STDIO_H
#define _STDIO_H 1

/* alapértelmezett buffer méret  */
#ifndef BUFSIZ
# define BUFSIZ 65536
#endif

#define EOF (-1)

/* a `tmpfile' alapértelmezett elérési útja  */
#define P_tmpdir    "/tmp/"

/* az `fseek' lehetséges harmadik paramétere, nem módosítható */
#define SEEK_SET    0   /* pozicionálás a fájl elejétől */
#define SEEK_CUR    1   /* pozicionálás az aktuális poziciótól */
#define SEEK_END    2   /* pozicionálás a fájl végétől */

/* sztandard fájl leírók */
#define STDIN_FILENO    1   /* sztandard bemenet */
#define STDOUT_FILENO   2   /* sztandard kimenet */
#define STDERR_FILENO   3   /* sztandard hiba kimenet */
#define stdin           STDIN_FILENO
#define stdout          STDOUT_FILENO
#define stderr          STDERR_FILENO

/* az `access' második paramérete, össze-VAGY-olhatók */
#define R_OK    A_READ      /* olvasási hozzáférés */
#define W_OK    A_WRITE     /* írási hozzáférés */
#define X_OK    A_EXEC      /* futtatási jogosultság */
#define A_OK    A_APPEND    /* hozzáfűzési hozzáférés */
#define D_OK    A_DELETE    /* törlési jogosultság */
#define F_OK    0           /* létezik vizsgálat */

/* open paraméterei */
#define O_READ      (1<<0)  /* olvasás */
#define O_WRITE     (1<<1)  /* írás */
#define O_EXCL      (1<<2)  /* kizárólagos hozzáférés (lock) */
#define O_APPEND    (1<<3)  /* hozzáfűzés */
#define O_CREAT     (1<<4)  /* létrehozás, ha nem létezik */
#define O_TRUNC     (1<<5)  /* meglévő fájl kiüresítése */
#define O_NONBLOCK  (1<<6)  /* nem blokkoló hozzáférés */
#define O_ASYNC     (1<<7)  /* aszinkron, nincs gyorsítótárba visszaírás */
#define O_TMPFILE   (1<<8)  /* bezáráskor törlés */
#define O_FIFO      (1<<9)  /* nevesített csővezeték */
#define O_RDONLY    O_READ
#define O_WRONLY    O_WRITE
#define O_RDWR      (O_READ|O_WRITE)

#define O_AMSK      (O_READ|O_WRITE|O_APPEND|O_EXCL)

/* a stat_t jelzői és makrói */
#define S_IFLG   0xFF000000 /* maszk */
#define S_IFLNK	 0x01000000 /* fsdrv specifikus, szimbolikus hivatkozás */
#define S_IFUNI  0x02000000 /* fsdrv specifikus, unió */
#define S_IFCHR  0x03000000 /* karakteres eszköz, ha blksize==1 */
#define S_IFMT     0xFF0000 /* maszk */
#define S_IFREG    0x000000 /* FCB_TYPE_REG_FILE */
#define S_IFDIR    0x010000 /* FCB_TYPE_REG_DIR */
#define S_IFDEV    0x020000 /* FCB_TYPE_DEVICE */
#define S_IFIFO    0x030000 /* FCB_TYPE_PIPE */
#define S_IFSOCK   0x040000 /* FCB_TYPE_SOCKET */
#define S_IFTTY    0x080000 /* TTY, nincs az fcb listában */
#define S_IMODE    0x000FFF /* hozzáférési mód maszkja (0xF000 belső jelzők) */

#define S_ISLNK(m)	(((m) & S_IFLG) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISUNI(m)	(((m) & S_IFMT) == S_IFDIR && ((m) & S_IFLG) == S_IFUNI)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFDEV && ((m) & S_IFLG) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFDEV && ((m) & S_IFLG) != S_IFCHR)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
#define S_ISTTY(m)	(((m) & S_IFMT) == S_IFTTY)

/* fioctl kódok (második paraméter) */
#define IOCTL_reset     (0)

#ifndef _AS

/* kiírja üzenetben az errno értékének jelentését */
extern void perror (char *cmd, char *s, ...);

/* gyökérkönyvtár beállítása, csak a rendszergazda hívhatja  */
extern fid_t chroot (const char *path);

/* munkakönyvtár beállítása  */
extern fid_t chdir (const char *path);

/* visszaadja a munkakönyvtárat egy allokált bufferben */
extern char *getcwd ();

/* statikus felcsatolási pont létrehozása */
extern int mount(const char *dev, const char *mnt, const char *opts);

/* felcsatolás megszüntetése, a paraméter lehet eszköz vagy könyvtár */
extern int umount(const char *path);

/* fájl leíró duplikálása, másolással  */
extern fid_t dup (fid_t stream);

/* fájl leíró duplikálása, de stream2 újramegnyitásával  */
extern fid_t dup2 (fid_t stream, fid_t stream2);

/* átmeneti fájl létrehozása és megnyitása írása/olvasásra */
extern fid_t tmpfile (void);

/* fájl jellemzők lekérése egy csak olvasható bufferbe  */
extern stat_t *lstat (const char *path);

/* st_dev-ben visszaadott eszköz jellemzők lekérése csak olvasható bufferbe */
extern stat_t *dstat (fid_t fd);

/* fájl jellemzők lekérése fájl leíró alapján, csak olvasható bufferbe */
extern stat_t *fstat (fid_t fd);

/* fájl megnyitása, és új fájl leíró visszaadása */
extern fid_t fopen (const char *filename, mode_t oflag);
/* fájl megnyitása, meglévő fájl leíró lecserélésével */
extern fid_t freopen (const char *filename, mode_t oflag, fid_t stream);
/* fájl leíró lezárása */
extern int fclose (fid_t stream);
/* összes fájl leíró lezárása */
extern int fcloseall (void);
/* aktuális pozició állítása egy fájl leírón */
extern int fseek (fid_t stream, off_t off, int whence);
/* pozició visszaállítása a kezdetre fájl vagy könyvtár leírón */
extern void rewind (fid_t stream);
/* fájl leíróhoz tartozó aktuális poziciót adja vissza */
extern fpos_t ftell (fid_t stream);
/* hiba és fájlvége jelző törlése a fájl leirón  */
extern void fclrerr (fid_t stream);
/* fájlvége jelző lekérdezése  */
extern bool_t feof (fid_t stream);
/* hiba jelző lekérdezése  */
extern int ferror (fid_t stream);
/* bináris adatok olvasása fájl leiróból */
extern size_t fread (fid_t stream, void *ptr, size_t size);
/* bináris adatok írása fájl leiróba */
extern size_t fwrite (fid_t stream, void *ptr, size_t size);
/* speciális utasítás eszköznek */
extern int fioctl(fid_t fid, uint64_t code, void *buff, size_t size);
/* egy UTF-8 karakter beolvasása fáj leiróból */
extern int fgetc (fid_t stream);
/* egy UTF-8 karakter olvasása a sztandard bemenetről */
extern int getchar (void);
/* egy UTF-8 karakter írása a fájl leiróba */
extern int fputc (fid_t stream, int c);
/* egy UTF-8 karakter kiírása a sztandard kimenetre */
extern int putchar (int c);
/* sztring kiírása fájl leiróba */
extern int fputs (fid_t stream, char *s);
/* sztring és újsor karakter kiírása a sztandard kimenetre */
extern int puts (char *s);
/* fájl leiró bufferének kiürítése, vagy mindé, ha a fájl leiró -1 */
extern int fflush (fid_t stream);

/* visszaadja a teljes kanonikus elérési úttal a fájl nevét egy allokált bufferben */
extern char *realpath (const char *name);
/* visszaadja egy szimbolikus link vagy unió cél elérési útját egy allokált bufferben */
extern char *readlink (const char *name);
/* könyvtár megnyitása és fájl leiró visszaadása */
extern fid_t opendir (const char *path);
/* következő könyvtárbejegyzés lekérdezése egy csak olvasható bufferbe, NULL-t ad vissza, ha nincs több */
extern dirent_t *readdir(fid_t dirstream);
/* könyvtár leiró lezárása, 0-át ad vissza, ha rendben, -1 -et ha nem sikerült */
extern int closedir (fid_t dirstream);
/* formázott kimenet írása a fájl leiróba */
extern int fprintf (fid_t stream, const char *format, ...);
/* formázott kimenet írása a sztandard kimenetre */
extern int printf (const char *format, ...);
/* formázott kimenet írása egy sztringbe  */
extern int sprintf (char *s, const char *format, ...);
/* formázott kimenet írása fájl leiróba paraméterlista alapján */
extern int vfprintf (fid_t stream, const char *format, va_list arg);
/* formázott kimenet írása a sztandard kimenetre paraméterlista alapján */
extern int vprintf (const char *format, va_list arg);
/* formázott kimenet írása egy sztringbe paraméterlista alapján */
extern int vsprintf (char *s, const char *format, va_list arg);
/* adott maximális számú karakter kiírása  */
extern int snprintf (char *s, size_t maxlen, const char *format, ...);
extern int vsnprintf (char *s, size_t maxlen, const char *format, va_list arg);
/* a terminál eszközének elérési útját adja vissza, csak olvasható bufferben. NULL-t ad vissza nem terminál esetén */
extern char *ttyname (fid_t fd);
/* a terminál eszközének elérési útját adja vissza a megadott címre */
extern int ttyname_r (fid_t fd, char *buf, size_t buflen);
/* ha a fájl leiró egy terminál, 1-et ad vissza, egyébként 0-át */
extern bool_t isatty (fid_t fd);

/*** unimplemented ***/
#if 0
/* The possibilities for the third argument to `setvbuf'.  */
#define _IOFBF 0		/* Fully buffered.  */
#define _IOLBF 1		/* Line buffered.  */
#define _IONBF 2		/* No buffering.  */

/* Create a one-way communication channel (pipe).
   If successful, two file descriptors are stored in PIPEDES;
   bytes written on PIPEDES[1] can be read from PIPEDES[0].
   Returns 0 if successful, -1 if not.  */
extern int pipe (int pipedes[2]);

/* Make a link to FROM named TO.  */
extern int link (const char *__from, const char *__to);

/* Make a symbolic link to FROM named TO.  */
extern int symlink (const char *__from, const char *__to);

/* Remove the link NAME.  */
extern int unlink (const char *__name);

/* Remove the directory PATH.  */
extern int rmdir (const char *__path);

/* Make all changes done to FD actually appear on disk.  */
extern int fsync (int __fd);

/* Make all changes done to all files actually appear on disk.  */
extern void sync (void) __THROW;

/* Truncate FILE to LENGTH bytes.  */
extern int truncate (const char *__file, __off_t __length);

/* Truncate the file FD is open on to LENGTH bytes.  */
extern int ftruncate (int __fd, __off_t __length);

/* Remove file FILENAME.  */
extern int remove (char *filename);
/* Rename file OLD to NEW.  */
extern int rename (char *oldname, char *newname);
/* Generate a temporary filename.  */
extern char *tmpnam (char *s);
extern char *tempnam (char *dir, char *pfx);
/* Create a new stream that refers to a memory buffer.  */
extern FILE *fmemopen (void *s, size_t len, char *modes);

/* Open a stream that writes into a malloc'd buffer that is expanded as
   necessary.  *BUFLOC and *SIZELOC are updated with the buffer's location
   and the number of characters written on fflush or fclose.  */
extern FILE *open_memstream (char **bufloc, size_t *sizeloc);
/* If BUF is NULL, make STREAM unbuffered.
   Else make it use buffer BUF, of size BUFSIZ.  */
extern void setbuf (FILE *stream, char *buf);
/* Make STREAM use buffering mode MODE.
   If BUF is not NULL, use N bytes of it for buffering;
   else allocate an internal buffer N bytes long.  */
extern int setvbuf (FILE *stream, char *buf, int modes, size_t n);
/* Make STREAM line-buffered.  */
extern void setlinebuf (FILE *stream);

/* Read formatted input from STREAM. */
extern int fscanf (FILE *stream, char *format, ...);
/* Read formatted input from stdin. */
extern int scanf (char *format, ...);
/* Read formatted input from S.  */
extern int sscanf (char *s, char *format, ...);
/* Read formatted input from S into argument list ARG. */
extern int vfscanf (FILE *s, char *format, va_list arg);

/* Read formatted input from stdin into argument list ARG. */
extern int vscanf (char *format, va_list arg);

/* Read formatted input from S into argument list ARG.  */
extern int vsscanf (char *s, char *format, va_list arg);

/* Get a newline-terminated string of finite length from STREAM. */
extern char *fgets (char *s, int n, FILE *stream);
/* Read up to (and including) a DELIMITER from STREAM into *LINEPTR
   (and null-terminate it). *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'd as
   necessary.  Returns the number of characters read (not including the
   null terminator), or -1 on error or EOF. */
extern ssize_t getdelim (char **lineptr, size_t *n, int delimiter, FILE *stream);

/* Like `getdelim', but reads up to a newline. */
extern ssize_t getline (char **lineptr, size_t *n, FILE *stream);

/* Push a character back onto the input buffer of STREAM. */
extern int ungetc (int c, FILE *stream);

/* Get STREAM's position. */
extern int fgetpos (FILE *stream, fpos_t *pos);
/* Set STREAM's position. */
extern int fsetpos (FILE *stream, fpos_t *pos);
/* Return the system file descriptor for STREAM.  */
extern int fileno (FILE *stream);
/* Create a new stream connected to a pipe running the given command. */
extern FILE *popen (char *command, char *modes);

/* Close a stream opened by popen and return the status of its child. */
extern int pclose (FILE *stream);
/* Acquire ownership of STREAM.  */
extern void flockfile (FILE *stream);

/* Try to acquire ownership of STREAM but do not block if it is not
   possible.  */
extern int ftrylockfile (FILE *stream);

/* Relinquish the ownership granted for STREAM.  */
extern void funlockfile (FILE *stream);
#endif

#endif

#endif /* stdio.h */
