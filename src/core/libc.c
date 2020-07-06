/*
 * core/libc.c
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
 * @subsystem memória
 * @brief Alacsony szintű függvénykönyvtár a core-hoz
 */

#include <arch.h>                   /* az ARCH_MEMFUNC definíció miatt */

uint8_t runlevel = RUNLVL_BOOT;     /* futási szint, lásd RUNLVL_* */
uint8_t sys_fault = 0;              /* rendszerhiba jelző (kivételkezelők állítják), főleg a debugger használja */
uint32_t numcores = 0;              /* a CPU magok száma (csak a címigazítás miatt 32 bites, egyébként 16) */
uint64_t srand[4];                  /* véletlen generátor magja */
uint64_t systables[8];              /* rendszertáblázatok (platform függő) */
pid_t services[NUMSRV];             /* rendszerszolgáltatások taszkjai */

/* gcc-nek kell automatikus stack overflow / underrun ellenőrzéshez */
uint64_t __stack_chk_guard = 0xBAD57AC6BAD57AC6UL;
void __stack_chk_fail() { kpanic("core stack smash"); }

void seterr(int errno)
{
    ((ccb_t*)LDYN_ccb)->core_errno = errno;
}

#ifdef ARCH_NO_MEMFUNCS
/**
 * memória másolása, általános implementáció
 */
void *memcpy(void *dst, const void *src, size_t n)
{
    register uint8_t *a=dst,*b=(uint8_t*)src;
    if(src && dst && n>0 && src!=dst) {
#ifdef __builtin_memcpy
        return __builtin_memcpy(dst, src, n);
#else
        if(((uint64_t)src&7)==0 && ((uint64_t)dst&7)==0 && n>8)
            while(n>8) { *((uint64_t*)a)=*((uint64_t*)b); a+=8; b+=8; n-=8; }
        while(n-->0) *a++=*b++;
#endif
    }
    return a;
}

/**
 * memória feltötlése adott karakterrel, általános implementáció
 */
void *memset(void *dst, uint8_t c, size_t n)
{
    register uint8_t *a=dst, b=(uint8_t)c;
    register uint64_t *d=dst, e;
    if(dst && n>0) {
#ifdef __builtin_memset
        return __builtin_memset(dst, c, n);
#else
        if(((uint64_t)dst&7)==0 && n>8){
            e=((uint64_t)b<<56) | ((uint64_t)b<<48)| ((uint64_t)b<<40) | ((uint64_t)b<<32) |
                ((uint64_t)b<<24) |((uint64_t)b<<16) | ((uint64_t)b<<8)| b;
            while(n>8) { *d++=e; a+=8; n-=8; }
        }
        while(n-->0) *a++=c;
#endif
    }
    return a;
}

/**
 * memória összehasonlítása, általános implementáció
 */
int memcmp(const void *s1, const void *s2, size_t n)
{
    register const uint8_t *a=s1,*b=s2;
    if(s1 && s2 && n>0 && s1!=s2) {
#ifdef __builtin_memcmp
        return __builtin_memcmp(s1, s2, n);
#else
        while(n-->0){ if(*a!=*b){return *a-*b;}a++;b++;}
#endif
    }
    return 0;
}
#endif

/**
 * utf-8 karakterek száma egy sztringben
 */
size_t mbstrlen(const char *s)
{
    register size_t c=0;
    if(s) {
        while(*s) {
            if((*s & 128) != 0) {
                if((*s & 32) == 0 ) s++; else
                if((*s & 16) == 0 ) s+=2; else
                if((*s & 8) == 0 ) s+=3;
            }
            c++;
            s++;
        }
    }
    return c;
}

/**
 * bájtok száma egy sztringben
 */
size_t strlen(const char *s)
{
    register size_t c=0;
    if(s) { while(*s++) c++; }
    return c;
}

/**
 * két sztring összehasonlítása
 */
int strcmp(const char *s1, const char *s2)
{
    if(s1 && s2 && s1!=s2) {
        do{if(*s1!=*s2){return *s1-*s2;}s1++;s2++;}while(*s1!=0);
        return *s1-*s2;
    }
    return 0;
}

/**
 * legfeljebb N karakter hosszú sztring másolása
 */
char *strncpy(char *dst, const char *src, size_t n)
{
    register const char *e = src+n;
    if(src && dst && n>0) {
        while(*src && src<e) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

/**
 * véletlenszám generátor bitjeinek összekeverése entrópianövelés céljából
 */
void kentropy() {
    srand[3]^=clock_ts;
    srand[(clock_ts+clock_ns)&3]*=16807;
    srand[(uint8_t)(srand[0]>>16)&3]*=16807;
    srand[(uint8_t)srand[0]&3]<<=1; srand[(uint8_t)(srand[0]>>8)&3]>>=1;
    srand[(uint8_t)srand[1]&3]^=srand[((uint8_t)srand[2])&3];
    srand[(uint8_t)srand[3]&3]*=16807;
    srand[0]^=srand[1]; srand[1]^=srand[2]; srand[2]^=srand[3]; srand[3]^=srand[0];
}

/*** Minimális sprintf implementáció ***/
extern uint8_t volatile reent, cnt; /* rekurzió és paraméter számláló */
extern char tmpstr[33];             /* átmeneti számformázáshoz */
extern char nullstr[];

static __inline__ char *sprintf_putascii(char *dst, int64_t c)
{
    *((int64_t*)&tmpstr) = c;
    tmpstr[cnt>0&&cnt<=8?cnt:8]=0;
    return sprintf(dst,&tmpstr[0]);
}

static __inline__ char *sprintf_putdec(char *dst, int64_t c)
{
    int64_t i=18,s=c<0;
    if(s) c*=-1;
    if(c>99999999999999999L)
        c=99999999999999999L;
    tmpstr[i]=0;
    do {
        tmpstr[--i]='0'+(c%10);
        c/=10;
    } while(c!=0&&i>0);
    if(s)
        tmpstr[--i]='-';
    if(cnt>0&&cnt<18) {
        while(i>18-cnt) {
            tmpstr[--i]=' ';
        }
        i=18-cnt;
    }
    return sprintf(dst,&tmpstr[i]);
}

static __inline__ char *sprintf_puthex(char *dst, int64_t c)
{
    int i=16;
    tmpstr[i]=0;
    do {
        char n=c & 0xf;
        tmpstr[--i]=n<10?'0'+n:'A'+n-10;
        c>>=4;
    } while(c!=0&&i>0);
    if(cnt>0&&cnt<=8) {
        while(i>16-cnt*2) {
            tmpstr[--i]='0';
        }
        i=16-cnt*2;
    }
    return sprintf(dst,&tmpstr[i]);
}

char *vsprintf(char *dst,char* fmt, va_list args)
{
    uint64_t arg;
    char *p;

    if(fmt==NULL || (((virt_t)fmt>>48)!=0 && ((virt_t)fmt>>48)!=0xffff))
        fmt = nullstr;
    if(dst == NULL || !fmt[0])
        return dst;

    arg = 0;
    while(!platform_memfault(fmt) && fmt[0]) {
        /* argument access */
        if(fmt[0]=='%' && !reent) {
            fmt++; cnt=0;
            if(fmt[0]=='%') {
                goto put;
            }
            while(fmt[0]>='0'&&fmt[0]<='9') {
                cnt *= 10;
                cnt += fmt[0]-'0';
                fmt++;
            }
            if(fmt[0]!='s')
                arg = va_arg(args, int64_t);
            if(fmt[0]=='c') {
                *dst = (char)arg; dst++;
                fmt++;
                continue;
            }
            reent++;
            if(fmt[0]=='l') fmt++;
            switch(fmt[0]) {
                case 'a': dst = sprintf_putascii(dst, arg); break;
                case 'd': dst = sprintf_putdec(dst, arg); break;
                case 'x': dst = sprintf_puthex(dst, arg); break;
                case 's': {
                    p = va_arg(args, char*);
                    if(p==NULL) p=nullstr;
                    dst = sprintf(dst, p);
                    arg=strlen(p); if(arg<cnt) { cnt-=arg; while(cnt-->0) *dst++=' '; }
                }
            }
            reent--;
        } else {
put:        *dst++ = *fmt;
        }
        fmt++;
    }
    *dst=0;
    return dst;
}

/**
 * sztring összerakása formázás és paraméterek alapján
 */
char *sprintf(char *dst,char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    return vsprintf(dst,fmt,args);
}

