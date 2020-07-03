/*
 * include/osZ/stdlib.h
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
 * @brief ISO C99 Sztandard: 7.20 általános funkciók, plusz OS/Z specifikus szolgáltatások
 */

#ifndef _STDLIB_H
#define _STDLIB_H 1

/* '_exit' paramétere, a program visszatérési értéke  */
#define EXIT_FAILURE    1   /* hiba történt  */
#define EXIT_SUCCESS    0   /* sikeres futás  */
#include <osZ/sysexits.h>   /* szolgáltatások visszatérési értékei */
#include <osZ/mman.h>       /* a memóriaallokátor jelzői miatt */

/* a többkarakteres betűk maximális hossza  */
#define MB_CUR_MAX  4

#if DEBUG
/* debug jelzők és felhasználói módú kprintf */
#include <osZ/debug.h>
#ifndef _AS
extern uint64_t _debug;
extern void dbg_printf(char * fmt, ...);
#endif
#endif

#ifndef _AS
/* memória allokátor */
void *bzt_alloc(uint64_t *arena, size_t a, void *ptr, size_t s, int flag);
void bzt_free(uint64_t *arena, void *ptr);
#if DEBUG
void bzt_dumpmem(uint64_t *arena);
#endif

/* taszk specifikus memória allokálás (Task Local Storage) */
/**
 * S bájt lefoglalása
 *
 * void *malloc(size_t s)
 */
#define malloc(s) bzt_alloc((void*)DYN_ADDRESS,8,NULL,s,MAP_PRIVATE)
/**
 * N darab, egyenként S bájt lefoglalása
 *
 * void *calloc(uint n, size_t s)
 */
#define calloc(n,s) bzt_alloc((void*)DYN_ADDRESS,8,NULL,n*s,MAP_PRIVATE)
/**
 * korábbi P lefoglalás méretének megváltoztatása S-re
 *
 * void *realloc(void *p, size_t s)
 */
#define realloc(p,s) bzt_alloc((void*)DYN_ADDRESS,8,p,s,MAP_PRIVATE)
/**
 * ISO C címhelyes allokálás, S bájt lefoglalása A-val osztható címen
 *
 * void *aligned_alloc(uint_t a, size_t s)
 */
#define aligned_alloc(a,s) bzt_alloc((void*)DYN_ADDRESS,a,NULL,s,MAP_PRIVATE)
/**
 * 'malloc', 'realloc' vagy 'calloc' által lefoglalt memória felszabadítása
 *
 * void free(void *p)
 */
#define free(p) bzt_free((void*)DYN_ADDRESS,p)

/* megosztott memória allokálás (Shared Memory) */
/**
 * S bájt lefoglalása megosztott memóriában
 *
 * void *smalloc(size_t s)
 */
#define smalloc(s) bzt_alloc((void*)SDYN_ADDRESS,8,NULL,s,MAP_SHARED)
/**
 * N darab, egyenként S bájt lefoglalása megosztott memóriában
 *
 * void *scalloc(uint n, size_t s)
 */
#define scalloc(n,s) bzt_alloc((void*)SDYN_ADDRESS,8,NULL,n*s,MAP_SHARED)
/**
 * korábbi P megosztott lefoglalás méretének megváltoztatása S-re
 *
 * void *srealloc(void *p, size_t s)
 */
#define srealloc(p,s) bzt_alloc((void*)SDYN_ADDRESS,8,p,s,MAP_SHARED)
/**
 * címhelyes, megosztott allokálás, S bájt lefoglalása A-val osztható címen
 *
 * void *saligned_alloc(uint_t a, size_t s)
 */
#define saligned_alloc(s) bzt_alloc((void*)SDYN_ADDRESS,a,NULL,s,MAP_SHARED)
/**
 * megosztott memória felszabadítása
 *
 * void sfree(void *p)
 */
#define sfree(p) bzt_free((void*)SDYN_ADDRESS,p)

/**
 * X abszolút értékét adja vissza
 */
#define abs(x) ((x)<0?-(x):(x))
/**
 * X és Y közül a kissebb
 */
#define min(x,y) ((x)<(y)?(x):(y))
/**
 * X és Y közül a nagyobb
 */
#define max(x,y) ((x)>(y)?(x):(y))
/**
 * X és Y távolságát adja vissza
 */
#define distance(x,y) (max(x,y)-min(x,y))

void lockacquire(int bit, uint64_t *ptr);   /* felfüggeszti a futást (yield), míg nem sikerül a jelzőbitet lefoglalni */
void lockrelease(int bit, uint64_t *ptr);   /* jelzőbit felszabadítása */
int lockid(uint64_t id, uint64_t *ptr);     /* megadott azonosítóval foglalja le a jelzőt. 0-át ad vissza, ha sikerült */

void yield();                               /* CPU idő feladása */

meminfo_t meminfo();                        /* memóriaállapot lekérdezése */

/* ellenörző összeg számolása */
uint32_t crc32c_calc(char *start, size_t length);
uint32_t crc32a_calc(char *start, size_t length);

/* többnyelvűség támogatása */
/* pont nyelvkód-ot fűz a prefixhez, és az így kapott fájlből beolvas nem több, mint txtc
 * szöveget a txt-be. A beolvasott szövegek számát adja vissza. */
int lang_init(char *prefix, int txtc, char **txt);

/* várakozás USEC mikroszekundumig, vagy amíg egy nem blokkolt vagy nem figyelmen kívül hagyott szignál érkezik */
void usleep(uint64_t usec);

/* buffer feltöltése véletlen adatokkal. 0-ával tér vissza, ha sikeres, -1 -el ha hiba történt */
int getentropy (void *buffer, size_t length);
/* véletlenszám visszaadása 0 és RAND_MAX között, beleértve a határértékeket is  */
uint64_t rand (void);
/* véletlenszám generátor entrópiájának növelése */
void srand (uint64_t seed);

/* funkció regisztrálása 'exit'-el való kilépéskor  */
int atexit (void (*func) (void));

/* az 'atexit' által regisztrált függvények hívása, majd kilépés STATUS kóddal */
void exit (int status) __attribute__ ((__noreturn__));

/* futás megszakítása és azonnali kilépés a programból */
void abort (void)  __attribute__ ((__noreturn__));

/* visszaadja a UNIX EPOCH óta eltelt időt MIKROSZEKUNDUMban */
uint64_t time();
/* rendszeridő beállítása MIKROSZEKUNDUMban, speciális jogosultság szükséges */
void stime(uint64_t utctimestamp);
void stimebcd (char *timestr);
void stimezone (int16_t min);

/* visszaadja az UTF-8 karakterek számát S-ben, maximum N hosszig, POSIX kompatibilitás miatt definiáljuk */
#define mblen(s,n) mbstrnlen(s,n)

/* KEY felezéses keresése BASE-ben, ami NMEMB darab SIZE méretű elemből áll, a CMP funkció használatával */
void *bsearch (void *key, void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *));

/* NMEMB elemű, SIZE elemméretű BASE tömb gyorsrendezése CMP használatával */
void qsort (void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *));

unsigned char *stdlib_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
unsigned char *stdlib_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);

/* sztring konvertálása 32 bites számmá  */
int atoi (char *c);
/* sztring konvertálása 64 bites számmá  */
long int atol (char *c);

/* visszaadja a hívó taszk azonosítóját  */
pid_t getpid (void);

/* visszaadja a hívó taszk szülőjének azonosítóját  */
pid_t getppid (void);

/* visszaadja a hívó taszk felhasználójának azonosítóját  */
uid_t getuid (void);
/* visszaadja egy taszk felhasználójának azonosítóját */
uid_t getuidp (pid_t pid);

/* visszaadja az aktuális taszk jogosultsági lista mutatóját. a listát a hívónak kell felszabadítania */
gid_t *getgid (void);
/* visszaadja egy taszk jogosultsági lista mutatóját. a listát a hívónak kell felszabadítania */
gid_t *getgidp (pid_t pid);

/*** unimplemented ***/
#if 0

/* Convert a string to a floating-point number.  */
extern double atof (char *__nptr);
/* Convert a string to a floating-point number.  */
extern double strtod (char *nptr, char **endptr);
/* Likewise for 'float' and 'long double' sizes of floating-point numbers.  */
extern float strtof (char *nptr, char **endptr);
extern long double strtold (char *nptr, char **endptr);
/* Convert a string to a long integer.  */
extern long int strtol (char *nptr, char **endptr, int base);
/* Convert a string to an unsigned long integer.  */
extern unsigned long int strtoul (char *nptr, char **endptr, int base);
/* Convert a string to a quadword integer.  */
extern long long int strtoll (char *nptr, char **endptr, int base);
/* Convert a string to an unsigned quadword integer.  */
extern unsigned long long int strtoull (char *nptr, char **endptr, int base);

/* Schedule an alarm.  In SECONDS seconds, the process will get a SIGALRM.
   If SECONDS is zero, any currently scheduled alarm will be cancelled.
   The function returns the number of seconds remaining until the last
   alarm scheduled would have signaled, or zero if there wasn't one.
   There is no return value to indicate an error, but you can set 'errno'
   to 0 and check its value after calling 'alarm', and this might tell you.
   The signal may come late due to processor scheduling.  */
extern unsigned int alarm (uint64_t sec);
/* Set an alarm to go off (generating a SIGALRM signal) in VALUE
   microseconds.  If INTERVAL is nonzero, when the alarm goes off, the
   timer is reset to go off every INTERVAL microseconds thereafter.
   Returns the number of microseconds remaining before the alarm.  */
extern void ualarm (__useconds_t __value, __useconds_t __interval);

/* Suspend the process until a signal arrives.
   This always returns -1 and sets 'errno' to EINTR.  */
extern int pause (void);

/* Return the value of envariable NAME, or NULL if it doesn't exist.  */
extern char *getenv (char *name);
/* Set NAME to VALUE in the environment.
   If REPLACE is nonzero, overwrite an existing value.  */
extern int setenv (char *name, char *value, int replace);

/* Remove the variable NAME from the environment.  */
extern int unsetenv (char *name);

/* Execute the given line as a shell command. */
extern int system (char *command);

/* Return the length of the given multibyte character,
   putting its 'wchar_t' representation in *PWC.  */
extern int mbtowc (wchar_t *pwc, char *s, size_t n);
/* Put the multibyte character represented
   by WCHAR in S, returning its length.  */
extern int wctomb (char *s, wchar_t wchar);

/* Convert a multibyte string to a wide char string.  */
extern size_t mbstowcs (wchar_t *pwcs, char *s, size_t n);
/* Convert a wide char string to multibyte string.  */
extern size_t wcstombs (char *s, wchar_t *pwcs, size_t n);

/* Put the 1 minute, 5 minute and 15 minute load averages into the first
   NELEM elements of LOADAVG.  Return the number written (never more than
   three, but may be less than NELEM), or -1 if an error occurred.  */
extern int getloadavg (double loadavg[], int nelem);

/* Replace the current process, executing PATH with arguments ARGV and
   environment ENVP.  ARGV and ENVP are terminated by NULL pointers.  */
extern int execve (const char *__path, char *const __argv[], char *const __envp[]);

/* Execute PATH with arguments ARGV and environment from 'environ'.  */
extern int execv (const char *__path, char *const __argv[]);

/* Execute PATH with all arguments after PATH until a NULL pointer,
   and the argument after that for environment.  */
extern int execle (const char *__path, const char *__arg, ...);

/* Execute PATH with all arguments after PATH until
   a NULL pointer and environment from 'environ'.  */
extern int execl (const char *__path, const char *__arg, ...);

/* Execute FILE, searching in the 'PATH' environment variable if it contains
   no slashes, with arguments ARGV and environment from 'environ'.  */
extern int execvp (const char *__file, char *const __argv[]);

/* Execute FILE, searching in the 'PATH' environment variable if
   it contains no slashes, with all arguments after FILE until a
   NULL pointer and environment from 'environ'.  */
extern int execlp (const char *__file, const char *__arg, ...);

/* Return nonzero if the calling process is in group GID.  */
extern int group_member (gid_t gid);

/* Set the user ID of the calling process to UID.
   If the calling process is the super-user, set the real
   and effective user IDs, and the saved set-user-ID to UID;
   if not, the effective user ID is set to UID.  */
extern int setuid (uid_t __uid);

/* Set the group ID of the calling process to GID.
   If the calling process is the super-user, set the real
   and effective group IDs, and the saved set-group-ID to GID;
   if not, the effective group ID is set to GID.  */
extern int addgid (gid_t __gid);
extern int delgid (gid_t __gid);

/* Clone the calling process, creating an exact copy.
   Return -1 for errors, 0 to the new process,
   and the process ID of the new process to the old process.  */
extern pid_t fork (void);

/* Clone the calling process, but without copying the whole address space.
   The calling process is suspended until the new process exits or is
   replaced by a call to 'execve'.  Return -1 for errors, 0 to the new process,
   and the process ID of the new process to the old process.  */
extern pid_t vfork (void);

/* Return the login name of the user.  */
extern char *getlogin (void);

/* Put the name of the current host in no more than LEN bytes of NAME.
   The result is null-terminated if LEN is large enough for the full
   name and the terminator.  */
extern int gethostname (char *__name, size_t __len);

/* Set the name of the current host to NAME, which is LEN bytes long.
   This call is restricted to the super-user.  */
extern int sethostname (const char *__name, size_t __len);

/* Encrypt at most 8 characters from KEY using salt to perturb DES.  */
extern char *crypt (const char *__key, const char *__salt);

/* Encrypt data in BLOCK in place if EDFLAG is zero; otherwise decrypt
   block in place.  */
extern void encrypt (char *__glibc_block, int __edflag);

#endif

#endif

#endif /* stdlib.h  */
