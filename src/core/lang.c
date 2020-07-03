/*
 * core/lang.c
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
 * @subsystem memória
 * @brief Többnyelvű fordítások támogatása
 */

#include <core.h>

char *txt[TXT_LAST];                            /* betöltött nyelvi szótár, fordítások */
char libcerrstrs[(ENOTIMPL+1)*TXT_LIBCSIZE];    /* libc errno szövegek */
char osver[256];                                /* OS verzió */
extern char kpanicsuffix2[];                    /* külső erőforrások, ezt még fordtani kell, de speciálisan */

/* egyeznie kell az errno.h-val */
char *errstrs[] = { ""/*Siker, nincs üzenet*/, "EPERM", "EAGAIN", "ESRCH", "EFAULT", "EINVAL", "EBUSY", "EACCES",
    "ENOMEM", "ENOEXEC", "ERTEXEC", "EEXIST", "ENOENT", "ENODEV", "ENOTDIR", "EISDIR", "ENOTUNI", "EISUNI", "ENOFS",
    "EBADFS", "EROFS", "ENOSPC", "ENOTSUP", "EIO", "EPIPE", "ESPIPE", "ENOTSHM", "EBADF", "ENOTBIG", "ENOTIMPL"
};

/**
 * nyelvi fájl betöltése, értelmezése és a szótár feltöltése
 */
void lang_init()
{
    char fn[]="/sys/lang/core.\0\0\0\0\0";
    char *s,*e,*a;
    int i=0,l,k;

    /* szótárfájl betöltése. Ez még nagyon korai, fizikai címet használ, lásd pmm_vmem() */
    memset(txt,0,sizeof(txt));
    memcpy(fn+15, lang, strlen(lang));
    s=fs_locate(fn);
    if(!s && (lang[0]!='e' || lang[1]!='n')) {
        /* ha nem találtuk meg, akkor az angollal próbálkozunk */
        lang[0]=fn[15]='e'; lang[1]=fn[16]='n'; lang[2]=fn[17]=0;
        s = fs_locate(fn);
    }
    /* nyelvi fájlok szerkezete, 16 bájtos fejléccel indulnak:
     *   0. 4  azonosító 'LANG'
     *   4. 4  méret bájtokban (fejléccel)
     *   8. 4  crc32 ellenőrző összeg (fejléc nélkül)
     *  12. 4  sztringek száma
     * ezt követik a zéróval lezárt sztringek */
    if(!s || memcmp(s,OSZ_LANG_MAGIC,4) || *((uint32_t*)(s+12))!=0x32
         /* || *((uint32_t*)(s+8))!=crc32a_calc(s+16,*((uint32_t*)(s+4))-16) */
        )
        kpanic("Unable to load language dictionary: %s", &fn);
    s+=16;
    /* core sztringek fordításai */
    while(i < TXT_LAST) {
        txt[i++] = s;
        while(*s) s++;
        s++;
    }
    /* libc sztringek fordításai */
    a = libcerrstrs;
    for(i=0; i<=ENOTIMPL; i++) {
        e = a;
        if(i) {
            l = strlen(errstrs[i]);
            memcpy(e, errstrs[i], l); e += l;
            *e++ = ' ';
        }
        while(*s) *e++ = *s++;
        *e++ = *s++;
        if(e > a+TXT_LIBCSIZE) kpanic("translation too long (max %d): %s", TXT_LIBCSIZE, a);
        a += TXT_LIBCSIZE;
    }
    /* beépített core sztringek átírása lefordított verziókkal */
    sprintf((char*)&osver, OSZ_NAME " " OSZ_VER " " OSZ_ARCH "-" OSZ_PLATFORM " (build " OSZ_BUILD ")\n"
        "Copyright (c) 2016-2019 bzt, CC-by-nc-sa\n%s\n\n", TXT_legalnotice);
    /* pániküzenet fordítása, és középreigazítása szóközökkel */
    memset(kpanicsuffix2+110,' ',54);
    k = mbstrlen(TXT_restart);
    i = ((55-k)/2); l = strlen(TXT_restart);
    memcpy(kpanicsuffix2+110+i, TXT_restart, l);
    /* szóközök hozzáadása. Vicces, mert strlen >= mbstrlen */
    memset(kpanicsuffix2+110+i+l, ' ', 54-k-i);
    kpanicsuffix2[110+l+54-k] = '\n';
    memset(kpanicsuffix2+110+l+54-k+1, ' ', 54);
    kpanicsuffix2[110+l+54-k+55] = '\n';
    kpanicsuffix2[110+l+54-k+56] = 0;
}
