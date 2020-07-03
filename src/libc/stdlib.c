/*
 * libc/stdlib.c
 *
 * Copyright (c) 2019 bzt (bztsrc@gitlab)
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
 * @brief az stdlib.h-ban definiált függvények megvalósítása
 */
#include <osZ.h>
#include "libc.h"
#include "crc32.h"

/* linkelő által feltöltött címek és változók */
public const alignmem __attribute__((section(".rodata"))) _libc_t _libc;
public char **__argumen;
public char **__environ;
public uint64_t errn;
#if DEBUG
public uint64_t _debug;
#endif

typedef void (*atexit_t)(void);
uint atexit_num = 0;
atexit_t *atexit_hooks = NULL;

/**
 * hibakód beállítása
 */
public void seterr(int e)       { errn = e; }

/**
 * hibakód lekérdezése
 */
public int errno()              { return errn; }

/**
 * nyelvi szótárfájl betöltése, hibánál nullával tér vissza
 */
/*
public int lang_init(char *prefix, int txtc, char **txt)
{
    stat_t *st;
    char fn[128], *s, *lang = getenv("LANG");
    int f, i=0;

    if(!prefix || txtc<1 || !txt) return 0;
    txt[0] = NULL;
    snprintf((char*)&fn, sizeof(fn)-1, "%s.%s", prefix, lang? lang : "en");
    st = lstat((char*)&fn);
    if(st != NULL && st->st_size > 0) {
        f = fopen((char*)&fn, O_RDONLY);
        if(f) {
            s = malloc(st->st_size);
            if(s) {
                fread(f, s, st->st_size);
                if(memcmp(s,"LANG",4) || *(int*)(s+12) != txtc ||
                    *(uint32_t*)(s+8) != crc32a_calc(s+16,*(uint32_t*)(s+4)-16)) {
                    fclose(f);
                    return 0;
                }
                s += 16;
                while(i<txtc) {
                    txt[i++] = s;
                    while(*s) s++;
                    s++;
                }
            }
            fclose(f);
        }
    }
    return i;
}
*/
/*
public void dumpmsg(msg_t *msg, int errno)
{
    if(getpid()==0x4E) {
dbg_printf("recv msg serial %d len %d%4D",msg->serial,msg->arg1,msg);
    if(msg->evt&MSG_PTRDATA)
        dbg_printf("%2D",msg->ptr);
    }
}
*/

/**
 * nyomkövetés ki/bekapcsolása
 */
public void trace(bool_t enable)    { mq_send1(SRV_CORE, SYS_trace, enable); }

/**
 * memória lefoglalálsa és leképezése a címtérbe
 */
public void *mmap(void *addr, size_t len, int prot, int flags, fid_t fid, off_t offs)
{
    if((flags & MAP_ANONYMOUS) || fid==(fid_t)-1)
        return (void*)mq_send3(SRV_CORE, SYS_memmap, (virt_t)addr, len, flags);
    return (void*)mq_send6(SRV_FS, SYS_mmap, (virt_t)addr, len, prot, flags, fid, offs);
}

/**
 * memória felszabadítása a címtérből
 */
public int munmap(void *addr, size_t len)
{
    mq_send2(SRV_CORE, SYS_memmap, (virt_t)addr, len);
    return 0;
}

/* segédfüggvények a számkonverziókhoz */
unsigned char *stdlib_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)
{
    if(*s=='0' && *(s+1)=='x')
        s+=2;
    *v=0;
    do{
        *v <<= 4;
        if(*s>='0' && *s<='9')
            *v += (uint64_t)((unsigned char)(*s)-'0');
        else if(*s >= 'a' && *s <= 'f')
            *v += (uint64_t)((unsigned char)(*s)-'a'+10);
        else if(*s >= 'A' && *s <= 'F')
            *v += (uint64_t)((unsigned char)(*s)-'A'+10);
        s++;
    } while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F'));
    if(*v < min)
        *v = min;
    if(max!=0 && *v > max)
        *v = max;
    return s;
}

unsigned char *stdlib_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)
{
    if(*s=='0' && *(s+1)=='x')
        return stdlib_hex(s+2, v, min, max);
    *v=0;
    do{
        *v *= 10;
        *v += (uint64_t)((unsigned char)(*s)-'0');
        s++;
    } while(*s>='0'&&*s<='9');
    if(*v < min)
        *v = min;
    if(max!=0 && *v > max)
        *v = max;
    return s;
}

/**
 * decimális (vagy 0x előtag esetén hexadecimális) sztring átalakítássa 32 bites integer számmá
 */
public int atoi(char *c)
{
    uint64_t r;
    int s = false;
    if(*c == '-') { s = true; c++; }
    stdlib_dec((uchar *)c, &r, 0, 0xFFFFFFFFUL);
    return s? -r : r;
}

/**
 * decimális (vagy 0x előtag esetén hexadecimális) sztring átalakítássa 64 bites long integer számmá
 */
public long int atol(char *c)
{
    uint64_t r;
    int s = false;
    if(*c == '-') { s = true; c++; }
    stdlib_dec((uchar *)c, &r, 0, 0xFFFFFFFFFFFFFFFFUL);
    return s? -r : r;
}

/**
 * KEY felezéses keresése BASE-ben, ami NMEMB darab SIZE méretű elemből áll, a CMP funkció használatával
 */
public void *bsearch(void *key, void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *))
{
    uint64_t s=0, e=nmemb, m;
    int ret;
    while (s < e) {
        m = s + (e-s)/2;
        ret = cmp(key, (uint8_t*)base + m*size);
        if (ret < 0) e = m; else
        if (ret > 0) s = m+1; else
            return (void *)((uint8_t*)base + m*size);
    }
    return NULL;
}

/**
 * funkció regisztrálása 'exit'-el való kilépéskor
 */
public int atexit(void (*func) (void))
{
    /* POSIX megengedi, hogy többször is szerepeljen ugyanaz. Az OS/Z nem. */
    uint i;
    for(i=0; i<atexit_num; i++)
        if(atexit_hooks[i] == func)
            return EEXIST;
    atexit_hooks = realloc(atexit_hooks, (atexit_num+1)*sizeof(atexit_t));
    if(!atexit_hooks || errno())
        return errno();
    atexit_hooks[atexit_num++] = func;
    return SUCCESS;
}

/**
 * az 'atexit' által regisztrált függvények hívása, majd kilépés STATUS kóddal
 */
public void exit(int status)
{
    /* az atexit függvények hívása regisztráció fordított sorrendjében */
    while(atexit_num) (*atexit_hooks[--atexit_num])();

    mq_send2(SRV_CORE, SYS_exit, status, false);
    __builtin_unreachable();
}

/**
 * programfutás azonnali felfüggesztése
 */
public void abort()
{
    mq_send2(SRV_CORE, SYS_exit, 0, true);
    __builtin_unreachable();
}

/**
 * az aktuális címtér klónozása
 */
public pid_t fork()                 { return mq_send0(SRV_CORE, SYS_fork); }

/**
 * a futó taszk altatása USEC mikroszekundumig
 */
public void usleep(uint64_t usec)   { mq_send1(SRV_CORE, SYS_usleep, usec); }

/**
 * véletlenszám generátor entrópiájának növelése
 */
public void srand(uint64_t seed)    { mq_send1(SRV_CORE, SYS_srand, seed); }

/**
 * 0 és URAND_MAX közötti, kritográfiában használható véletlenszám lekérdezése
 */
public uint64_t rand()              { return mq_send0(SRV_CORE, SYS_rand); }

/**
 * egy buffer feltöltése kriptográfiában használható véletlen adatokkal
 */
public int getentropy(void *buffer, size_t length) { return mq_send2(SRV_CORE, SYS_getentropy, (virt_t)buffer, length); }

/**
 * rendszeridő beállítása
 */
public void stime(uint64_t usec)    { mq_send1(SRV_CORE, SYS_stime, usec); }

/**
 * rendszeridő beállítása BCD sztringből (0xÉÉ 0xÉÉ 0xHH 0xNN 0xÓÓ 0xPP 0xMM)
 */
public void stimebcd(char *timestr) { mq_send2(SRV_CORE, SYS_stimebcd | MSG_PTRDATA, (virt_t)timestr, 7); }

/**
 * rendszeridő időzónájának beállítása percekben (UTC -1440 .. +1440)
 */
public void stimezone(int16_t min)  { mq_send1(SRV_CORE, SYS_stimezone, min); }

/**
 * rendszeridő lekérdezése mikroszekundumban
 */
public uint64_t time()              { return mq_send0(SRV_CORE, SYS_time); }

public pid_t getpid()               { return (pid_t)mq_send0(SRV_CORE, SYS_getpid); }
public pid_t getppid()              { return (pid_t)mq_send0(SRV_CORE, SYS_getppid); }
public gid_t *getgid()              { return (gid_t*)mq_send2(SRV_CORE,SYS_getgid,(uint64_t)malloc(TCB_MAXACE*sizeof(gid_t)),0); }
public gid_t *getgidp(pid_t p)      { return (gid_t*)mq_send2(SRV_CORE,SYS_getgid,(uint64_t)malloc(TCB_MAXACE*sizeof(gid_t)),p); }
