/*
 * libc/string.c
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
 * @brief a string.h-ban definiált függvények megvalósítása, van egy (platform)/string.S is
 */

#include <osZ.h>
#include "libc.h"

/* Nem szabad hibát dobniuk NULL-ra vagy nulla hosszra. Az str* függvényeknek UTF-8 kompatíbilisnek kell lenniük. */

/* egyezniük kell a signal.h-ban definiáltakkal */
char *sigs[] = { "NONE", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "EMT", "FPE", "KILL", "BUS", "SEGV",
    "SYS", "PIPE", "ALRM", "TERM", "URG", "STOP", "TSTP", "CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU", "XFSZ",
    "VTALRM", "PROF", "WINCH", "INFO", "USR1", "USR2" };

/**
 * utf-8 szekvenciává alakítja az UNICODE-ot, majd lépteti a sztringmutatót
 */
void chr(uint8_t **s, uint32_t u)
{
    if(u<0x80) { **s=u; *(*s+1)=0; *s += 1;
    } else if(u<0x800) {
        *(*s+0)=((u>>6)&0x1F)|0xC0;
        *(*s+1)=(u&0x3F)|0x80;
        *(*s+2)=0;
        *s += 2;
    } else if(u<0x10000) {
        *(*s+0)=((u>>12)&0x0F)|0xE0;
        *(*s+1)=((u>>6)&0x3F)|0x80;
        *(*s+2)=(u&0x3F)|0x80;
        *(*s+3)=0;
        *s += 3;
    } else {
        *(*s+0)=((u>>18)&0x07)|0xF0;
        *(*s+1)=((u>>12)&0x3F)|0x80;
        *(*s+2)=((u>>6)&0x3F)|0x80;
        *(*s+3)=(u&0x3F)|0x80;
        *(*s+4)=0;
        *s += 4;
    }
}

/**
 * lépteti a sztringmutatót a következő utf-8 karakterre és visszaadja az UNICODE kódpontot
 */
uint32_t ord(uint8_t **s)
{
    uint32_t c = **s;
    /* utf-8 -> unicode */
    if((**s & 128) != 0) {
        if((**s & 32) == 0 ) {
            c = ((**s & 0x1F)<<6)|(*(*s+1) & 0x3F);
            *s += 1;
        } else
        if((**s & 16) == 0 ) {
            c = ((**s & 0xF)<<12)|((*(*s+1) & 0x3F)<<6)|(*(*s+2) & 0x3F);
            *s += 2;
        } else
        if((**s & 8) == 0 ) {
            c = ((**s & 0x7)<<18)|((*(*s+1) & 0x3F)<<12)|((*(*s+2) & 0x3F)<<6)|(*(*s+3) & 0x3F);
            *s += 3;
        } else
            c = 0;
    }
    *s += 1;
    return c;
}

/**
 * lépteti a sztringmutatót a következő utf-8 karakterre és visszaadja a kisbetűs változatot
 */
uint32_t strtolower(uint8_t **s)
{
    uint32_t c = **s;
    /* utf-8 -> unicode */
    if((**s & 128) != 0) {
        if((**s & 32) == 0 ) {
            c = ((**s & 0x1F)<<6)|(*(*s+1) & 0x3F);
            *s += 1;
        } else
        if((**s & 16) == 0 ) {
            c = ((**s & 0xF)<<12)|((*(*s+1) & 0x3F)<<6)|(*(*s+2) & 0x3F);
            *s += 2;
        } else
        if((**s & 8) == 0 ) {
            c = ((**s & 0x7)<<18)|((*(*s+1) & 0x3F)<<12)|((*(*s+2) & 0x3F)<<6)|(*(*s+3) & 0x3F);
            *s += 3;
        } else
            c = 0;
    }
    *s += 1;

    /* remekbeszabott unicode tábla konverzió. Hogy miért nem lehetett mindig egymás mellé rakni a kis és nagybetűket...? */
    if((c >='A' && c <='Z') || (c >= 0xC0 && c <= 0xDE) || (c>=0x391 && c<=0x3AB) || (c>=0x410 && c<=0x43F)) c += 32; else
    if((c>=0x400 && c<=0x40F)) c += 0x50; else if((c>=0x530 && c<=0x558) || (c>=0x10A0 && c<=0x10C5)) c += 0x30; else
    if((c>=0x660 && c<=0x669)) c -= 0x660-'0'; else if((c>=0x6F0 && c<=0x6F9)) c -= 0x6F0-'0'; else
    if((c>=0x7C0 && c<=0x7C9)) c -= 0x7C0-'0'; else if((c>=0xED0 && c<=0xED9)) c -= 0xED0-'0'; else
    if((c>=0xF20 && c<=0xF29)) c -= 0x7C0-'0'; else if((c>=0x1040 && c<=0x1049)) c -= 0x1040-'0'; else
    if((c>=0x1369 && c<=0x1372)) c -= 0x1369-'1'; else if((c>=0x17E0 && c<=0x17E9)) c -= 0x17E0-'0'; else
    if((c>=0x1810 && c<=0x1819)) c -= 0x1810-'0'; else if((c>=0x19D0 && c<=0x19D9)) c -= 0x19D0-'0'; else
    if((c>=0x1A80 && c<=0x1A89)) c -= 0x1A80-'0'; else if((c>=0x1A90 && c<=0x1A99)) c -= 0x1A90-'0'; else
    if((c>=0x1BB0 && c<=0x1BB9)) c -= 0x1BB0-'0'; else if((c>=0x1C40 && c<=0x1C49)) c -= 0x1C40-'0'; else
    if((c>=0x1C50 && c<=0x1C59)) c -= 0x1C50-'0'; else if((c>=0x3021 && c<=0x3029)) c -= 0x3021-'1';
    if(c>=0x1F00 && c<=0x1FFF && ((c&15) > 7)) c -= 8; else
    if((!(c&1) && ((c>=0x100 && c<=0x136) || (c>=0x14A && c<=0x176) || (c>=0x1A0 && c<=0x1A4) || (c>=0x182 && c<=0x184) ||
        (c>=0x1DE && c<=0x1EE) || (c>=0x1F8 && c<=0x21E) || (c>=0x222 && c<=0x232) || (c>=0x246 && c<=0x24E) ||
        (c>=0x3D8 && c<=0x3FA) || (c>=0x460 && c<=0x480) || (c>=0x48A && c<=0x4BE) || (c>=0x4D0 && c<=0x524) ||
        (c>=0x1E00 && c<=0x1EF8))) ||
        ((c&1) && ((c>=0x139 && c<=0x147) || (c>=0x179 && c<=0x17D) || (c>=0x1B3 && c<=0x1B5) || (c>=0x1CD && c<=0x1DB) ||
        (c>=0x4C1 && c<=0x4CD)))) c++;
    else
        switch(c) {
            case 0x178: c=0xFF; break; case 0x181: c=0x180; break; case 0x1F6: c=0x195; break; case 0x1F7: c=0x1BF; break;
            case 0x220: c=0x19E; break; case 0x23D: c=0x19A; break; case 0x243: c=0x180; break; case 0x3007: c='0'; break;
            case 0x18E: case 0x18F: c=0x1DD; break;
            case 0x187: case 0x18B: case 0x191: case 0x198: case 0x19D: case 0x1A7: case 0x1AC: case 0x1AF: case 0x1B8:
            case 0x1BC: case 0x1F4: case 0x23B: case 0x241: c++; break;
            case 0x1C4: case 0x1C5: c=0x1C6; break; case 0x1C7: case 0x1C8: c=0x1C9; break;
            case 0x1CA: case 0x1CB: c=0x1CC; break; case 0x1F1: case 0x1F2: c=0x1F3; break;
        }

    /* unicode -> utf-8 */
    if(c < 0x80) return c;
    if(c < 0x800) return (((c&0x3F)|0x80)<<8) | (((c>>6)&0x1F)|0xC0);
    if(c < 0x10000) return (((c&0x3F)|0x80)<<16) | ((((c>>6)&0x3F)|0x80)<<8) | (((c>>12)&0x0F)|0xE0);
    return (((c&0x3F)|0x80)<<24) | ((((c>>6)&0x3F)|0x80)<<16) | ((((c>>12)&0x3F)|0x80)<<8) | (((c>>18)&0x07)|0xF0);
}

/**
 * átmozgat N bájtot SRC-ből DST-be úgy, hogy a két terület átfedheti egymást
 */
public void *memmove(void *dst, const void *src, size_t n)
{
    register uint8_t *a=dst,*b=(uint8_t*)src;
    if(src && dst && n>0) {
#ifdef __builtin_memmove
        return __builtin_memmove(dst, src, n);
#else
        if((a<b && a+n>b) || (b<a && b+n>a)) {
            a+=n-1; b+=n-1; while(n-->0) *a--=*b--;
        } else {
            return memcpy(dst, src, n);
        }
#endif
    }
    return a;
}

/**
 * bájt első előfordulásának keresése. Nem UTF-8 biztos
 */
public void *memchr(const void *s, int c, size_t n)
{
    register uint8_t *e, *p=(uint8_t*)s;
    if(s && n) {
#ifdef __builtin_memchr
        return __builtin_memchr(s, c, n);
#else
        for(e=p+n; p<e; p++) if(*p==(uint8_t)c) return p;
#endif
    }
    return NULL;
}

/**
 * bájt utolsó előfordulásának keresése. Nem UTF-8 biztos
 */
public void *memrchr(const void *s, int c, size_t n)
{
    register uint8_t *e, *p=(uint8_t*)s;
    if(s && n) {
#ifdef __builtin_memchr
        return __builtin_memrchr(s, c, n);
#else
        for(e=p+n; p<e; --e) if(*e==(uint8_t)c) return e;
#endif
    }
    return NULL;
}


/**
 * NEDDLELEN hosszú NEEDLE első előfordulásának keresése a HAYSTACKLEN hosszú HAYSTACK-ben
 */
public void *memmem(const void *haystack, size_t hl, const void *needle, size_t nl)
{
    char *c = (char*)haystack;
    if(!haystack || !needle || !hl || !nl || nl > hl) return NULL;
#ifdef __builtin_memmem
    return __builtin_memmem(haystack, hl, neddle, nl);
#else
    hl -= nl;
    while(hl) {
        if(!memcmp(c, needle, nl)) return c;
        c++; hl--;
    }
    return NULL;
#endif
}

/**
 * NEDDLELEN hosszú NEEDLE utolsó előfordulásának keresése a HAYSTACKLEN hosszú HAYSTACK-ben
 */
public void *memrmem(const void *haystack, size_t hl, const void *needle, size_t nl)
{
    char *c = (char*)haystack;
    if(!haystack || !needle || !hl || !nl || nl > hl) return NULL;
#ifdef __builtin_memrmem
    return __builtin_memrmem(haystack, hl, neddle, nl);
#else
    hl -= nl;
    c += hl;
    while(hl) {
        if(!memcmp(c, needle, nl)) return c;
        c--; hl--;
    }
    return NULL;
#endif
}

/**
 * visszaadja az ERRNUM 'errno' kód szöveges megfelelőjét sztringben
 */
public char *strerror(int errnum)
{
    return errnum < 1 ? "" : (errnum < (int)(sizeof(_libc.errstrs)/TXT_LIBCSIZE) ?
        (char*)&_libc.errstrs + errnum*TXT_LIBCSIZE : (char*)&_libc.errstrs);
}

/**
 * visszaadja a SIG szignál szöveges nevét sztringben
 */
public char *strsignal(int sig)
{
    return sig >= 0 && sig < (int)(sizeof(sigs)/sizeof(sigs[0])) ? sigs[sig] : "?";
}

/**
 * visszaadja a sztring bájthosszát
 */
public size_t strlen(const char *s)
{
    register size_t c=0;
    if(s) { while(*s++) c++; }
    return c;
}

/**
 * visszaadja a sztring bájthosszát legfeljebb N hosszig
 */
public size_t strnlen(const char *s, size_t n)
{
    register const char *e = s+n;
    register size_t c=0;
    if(s) { while(s < e && *s++) c++; }
    return c;
}

/**
 * visszaadja a sztring karaktereinek számát (ami lehet kevesebb, mint a bájthossz)
 */
public size_t mbstrlen(const char *s)
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
 * visszaadja a sztring karaktereinek számát legfeljebb N bájt hosszig
 */
public size_t mbstrnlen(const char *s, size_t n)
{
    register const char *e = s+n;
    register size_t c = 0;
    if(s) {
        while(s < e && *s) {
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
 * sztring összehasonlítás, 0-át ad vissza, ha egyezik
 */
public int strcmp(const char *s1, const char *s2)
{
    if(s1 && s2 && s1!=s2) {
        do{if(*s1!=*s2){return *s1-*s2;}s1++;s2++;}while(*s1!=0);
        return *s1-*s2;
    }
    return 0;
}

/**
 * sztring összehasonlítás legfeljebb N hosszig, 0-át ad vissza, ha egyezik
 */
public int strncmp(const char *s1, const char *s2, size_t n)
{
    register const char *e = s1+n;
    if(s1 && s2 && s1!=s2 && n>0) {
        do{if(*s1!=*s2){return *s1-*s2;}s1++;s2++;}while(*s1!=0 && s1<e);
        return *s1-*s2;
    }
    return 0;
}

/**
 * kis- és nagybetűfüggetlen sztring összehasonlítás, 0-át ad vissza, ha egyezik
 */
public int strcasecmp(const char *s1, const char *s2)
{
    register uint32_t a, b;
    if(s1 && s2 && s1!=s2) {
        do{a=strtolower((uint8_t**)&s1);b=strtolower((uint8_t**)&s2);if(a!=b){return a-b;}}while(*s1!=0);
        return *s1-*s2;
    }
    return 0;
}

/**
 * kis- és nagybetűfüggetlen sztring összehasonlítás legfeljebb N hosszig, 0-át ad vissza, ha egyezik
 */
public int strncasecmp(const char *s1, const char *s2, size_t n)
{
    register const char *e = s1+n;
    register uint32_t a, b;
    if(s1 && s2 && s1!=s2 && n>0) {
        do{a=strtolower((uint8_t**)&s1);b=strtolower((uint8_t**)&s2);if(a!=b){return a-b;}}while(a && s1<e);
        return *s1-*s2;
    }
    return 0;
}

/**
 * SRC sztring hozzáfűzése DST sztringhez
 */
public char *strcat(char *dst, const char *src)
{
    if(src && dst) {
        dst += strlen(dst);
        while(*src) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

/**
 * legfeljebb N hosszú SRC sztring hozzáfűzése DST sztringhez
 */
public char *strncat(char *dst, const char *src, size_t n)
{
    register const char *e = src+n;
    if(src && dst && n>0) {
        dst += strlen(dst);
        while(*src && src<e) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

/**
 * SRC sztring másolása DST-be
 */
public char *strcpy(char *dst, const char *src)
{
    if(src && dst) {
        while(*src) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

/**
 * legfeljebb N hosszú SRC sztring másolása DST-be
 */
public char *strncpy(char *dst, const char *src, size_t n)
{
    register const char *e = src+n;
    if(src && dst && n>0) {
        while(*src && src<e) {*dst++=*src++;} *dst=0;
    }
    return dst;
}

/**
 * SRC kisbetűsített másolása DST-be
 */
public char *strcasecpy(char *dst, const char *src)
{
    uint64_t l=0;
    if(src && dst) {
        while(*src) { l=strtolower((uint8_t**)&src); dst=strcat(dst, (char*)&l); }
    }
    return dst;
}

/**
 * legfeljebb N hosszú SRC kisbetűsített másolása DST-be
 */
public char *strncasecpy(char *dst, const char *src, size_t n)
{
    register const char *e = src+n;
    uint64_t l=0;
    if(src && dst && n>0) {
        while(*src && src<e) { l=strtolower((uint8_t**)&src); dst=strcat(dst, (char*)&l); }
    }
    return dst;
}

/**
 * S duplikálása egy újonnan allokált bufferbe
 */
public char *strdup(const char *s)
{
    int i = strlen(s)+1;
    char *s2 = (char *)malloc(i);
    if(s2 != NULL) memcpy(s2, (void*)s, i);
    return s2;
}

/**
 * legfeljebb N karakter duplikálása S stringből egy új allokált bufferbe
 */
public char *strndup(const char *s, size_t n)
{
    int i = strnlen(s,n);
    char *s2 = (char *)malloc(i+1);
    if(s2 != NULL) {
        memcpy(s2, (void*)s, i);
        s2[i] = 0;
    }
    return s2;
}

/**
 * kisbetűsített S duplikálása egy újonnan allokált bufferbe
 */
public char *strcasedup(const char *s)
{
    int i = strlen(s)+1;
    uint64_t l=0;
    char *d, *s2 = (char *)malloc(i);
    if(s2 != NULL) { d=s2; while(*s) { l=strtolower((uint8_t**)&s); d=strcat(d, (char*)&l); } }
    return s2;
}

/**
 * legfeljebb N kisbetűs karakter duplikálása S stringből egy újonnan allokált bufferbe
 */
public char *strncasedup(const char *s, size_t n)
{
    int i = strnlen(s,n)+1;
    uint64_t l=0;
    char *d, *s2 = (char *)malloc(i);
    if(s2 != NULL) { d=s2; while(*s) { l=strtolower((uint8_t**)&s); d=strcat(d, (char*)&l); } }
    return s2;
}

/**
 * karakter első előfordulásának keresése a sztringben, ez UTF-8 biztos
 */
public char *strchr(const char *s, uint32_t c)
{
    register int n=1;
    if(s) {
        if((c & 128) != 0) {
            if((c & 32) == 0 ) n++; else
            if((c & 16) == 0 ) n+=2; else
            if((c & 8) == 0 )  n+=3;
        }
        return memmem(s, strlen(s), (void*)&c, n);
    }
    return NULL;
}

/**
 * karakter utolsó előfordulásának keresése, UTF-8 biztos
 */
public char *strrchr(const char *s, uint32_t c)
{
    register int n=1;
    register char *e;
    if(s) {
        if((c & 128) != 0) {
            if((c & 32) == 0 ) n++; else
            if((c & 16) == 0 ) n+=2; else
            if((c & 8) == 0 )  n+=3;
        }
        e = (char*)s + strlen(s) - n;
        while(s < e) {
            if(!memcmp(e, (void*)&c, n)) return e;
            s--;
        }
    }
    return NULL;
}

/**
 * NEEDLE első előfordulásának keresése a HAYSTACK sztringben
 */
public char *strstr(const char *haystack, const char *needle)
{
    return memmem(haystack, strlen(haystack), needle, strlen(needle));
}

/**
 * NEEDLE utolsó előfordulásának keresése a HAYSTACK sztringben
 */
public char *strrstr(const char *haystack, const char *needle)
{
    return memrmem(haystack, strlen(haystack), needle, strlen(needle));
}

/**
 * hasonló a 'strstr'-hez, de kis- és nagybetű függetlenül keresi az első előfordulást
 */
public char *strcasestr(const char *haystack, const char *needle)
{
    char *c = (char*)haystack;
    size_t hl = strlen(haystack);
    size_t nl = strlen(needle);
    if(haystack == NULL || needle == NULL || hl == 0 || nl == 0 || nl > hl) return NULL;
    hl -= nl;
    while(hl) {
        if(!strncasecmp(c, needle, nl)) return c;
        c++; hl--;
    }
    return NULL;
}

/**
 * hasonló a 'strrstr'-hez, de kis- és nagybetű függetlenül keresi az utolsó előfordulást
 */
public char *strrcasestr(const char *haystack, const char *needle)
{
    char *c = (char*)haystack;
    size_t hl = strlen(haystack);
    size_t nl = strlen(needle);
    if(haystack == NULL || needle == NULL || hl == 0 || nl == 0 || nl > hl) return NULL;
    hl -= nl;
    c += hl;
    while(hl) {
        if(!strncasecmp(c, needle, nl)) return c;
        c--; hl--;
    }
    return NULL;
}

/**
 * elérési út fájlnév részét adja vissza egy újonnan allokált bufferben
 */
public char *basename(const char *s)
{
    char *r;
    int i, e;
    if(s == NULL) return NULL;
    e = strlen(s)-1;
    if(s[e] == '/') e--;
    for(i = e; i > 1 && s[i-1] != '/'; i--);
    if(i == e) return NULL;
    r = (char*)malloc(e-i+1);
    memcpy(r, (void*)(s+i), e-i);
    return r;
}

/**
 * elérési út könyvtár részét adja vissza egy újonnan allokált bufferben
 */
public char *dirname(const char *s)
{
    char *r;
    int i, e;
    if(s == NULL) return NULL;
    e = strlen(s)-1;
    if(s[e] == '/') e--;
    for(i = e; i > 0 && s[i] != '/'; i--);
    if(i == e || i == 0) return NULL;
    r = (char*)malloc(i);
    memcpy(r, (void*)s, i-1);
    return r;
}

/* segédfüggvény a tokenizálókhoz. ez egyelőre még nem UTF-8 biztos, de annak kéne lennie */
private char *_strtok_r(char *s, const char *d, char **p, uint8_t skip)
{
    int c, sc;
    char *tok, *sp;

    if(d == NULL || (s == NULL && (s=*p) == NULL)) return NULL;
again:
    c = *s++;
    for(sp = (char *)d; (sc=*sp++)!=0;) {
        if(c == sc) {
            if(skip) goto again;
            else { *p=s; *(s-1)=0; return s-1; }
        }
    }

    if (c == 0) { *p=NULL; return NULL; }
    tok = s-1;
    while(1) {
        c = *s++;
        sp = (char *)d;
        do {
            if((sc=*sp++) == c) {
                if(c == 0) s = NULL;
                else *(s-1) = 0;
                *p = s;
                return tok;
            }
        } while(sc != 0);
    }
    return NULL;
}

/**
 * S feldarabolása DELIM karaktereknél
 */
public char *strtok(char *s, const char *delim)
{
    char *p = s;
    return _strtok_r (s, delim, &p, 1);
}

/**
 * S feldarabolása DELIM karaktereknél, a tagok a PTR tömbbe kerülnek
 */
public char *strtok_r(char *s, const char *delim, char **ptr)
{
    return _strtok_r (s, delim, ptr, 1);
}

/**
 * visszaadja a DELIM határolt sztringet, 0-ával lezárva, *STRINGP-t pedig a következő bájtra állítva
 */
public char *strsep(char **stringp, const char *delim)
{
    return _strtok_r (*stringp, delim, stringp, 0);
}

/**
 * memória másolása egy másik taszk címterébe
 */
public void tskcpy(pid_t dst, void *dest, void *src, size_t n)
{
    mq_send4(SRV_CORE, SYS_tskcpy, dst, dest, src, n);
}

