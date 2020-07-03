/*
 * core/env.c
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
 * @subsystem platform
 * @brief Környezeti változók értelmezése (lásd FS0:\BOOTBOOT\CONFIG vagy /sys/config)
 */

#include <core.h>
#include "../libui/pixbuf.h"        /* a PBT_* definíciók miatt kell */

/*** kiolvasott változók ***/
uint64_t dmabuf;
uint64_t debug;
uint64_t display;
uint64_t quantum;
uint64_t flags;
char lang[6];                               /* "xx\0" vagy "xx_XX\0" */
char display_drv[128], display_env[128];    /* megjelenítő paraméterei */
bool_t asktime = false;

/* külső erőforrások */
extern void kprintf_putchar(int c);
extern void kprintf_clearline();
extern uint32_t kx;

/* platform specifikus változók értelmezése */
extern unsigned char *platform_parse(unsigned char *env);

/**
 * UTC másodperc időbélyeg kiszámítása lokális BCD vagy bináris időstringből
 */
uint64_t env_getts(char *p, int16_t timezone)
{
    uint64_t j, r = 0, y, m, d, h, i, s;
    uint64_t md[12] = {31,0,31,30,31,30,31,31,30,31,30,31};

    if(timezone < -1440 || timezone > 1440) timezone = 0;           /* érvénytelen időzóna esetén UTC */

    /* BCD és bináris formátum detekálása */
    if(p[0]>=0x20 && p[0]<=0x30 && (unsigned char)p[1]<=0x99 &&     /* év */
       p[2]>=0x01 && p[2]<=0x12 &&                                  /* hónap */
       p[3]>=0x01 && p[3]<=0x31 &&                                  /* nap */
       p[4]<=0x23 && p[5]<=0x59 && p[6]<=0x60                       /* óra, perc, mperc */
    ) {
        /* BCD */
        y = ((p[0]>>4)*1000)+(p[0]&0x0F)*100+((p[1]>>4)*10)+(p[1]&0x0F);
        m = ((p[2]>>4)*10)+(p[2]&0x0F);
        d = ((p[3]>>4)*10)+(p[3]&0x0F);
        h = ((p[4]>>4)*10)+(p[4]&0x0F);
        i = ((p[5]>>4)*10)+(p[5]&0x0F);
        s = ((p[6]>>4)*10)+(p[6]&0x0F);
    } else {
        /* bináris */
        y = (p[1]<<8)+p[0];
        m = p[2];
        d = p[3];
        h = p[4];
        i = p[5];
        s = p[6];
    }
    /* szökőév? Ha igen, akkor februárt módosítjuk */
    md[1] = ((y&3)!=0 ? 28 : ((y%100) == 0 && (y%400) != 0? 28 : 29));

    /* Epoch óta eltelt napok. Csalunk kicsit */
    r = 16801; /* előre kiszámított napok száma (1970.jan.1 - 2016.jan.1.) */
    for(j=2016;j<y;j++)
        r += ((j&3)!=0 ? 365 : ((j%100)==0 && (j%400)!=0?365:366));
    for(j=1;j<m;j++) r += md[j-1];          /* ebben az évben */
    r += d-1;                               /* ebben a hónapban */
    r *= 24*60*60;                          /* másodpercre alakítás */
    r += h*60*60 + (timezone + i)*60 + s;   /* időzóna korrekció */
    /* szökőmásodpercet itt nem kezeljük */
    return r;
}

/**
 * hexa érték értelmezése
 */
unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)
{
    while(*s == ' ' || *s == '\t') s++;
    if(*s == '0' && *(s+1) == 'x') s += 2;
    do{
        *v <<= 4;
        if(*s >= '0' && *s <= '9')      *v += (uint64_t)((unsigned char)(*s)-'0');
        else if(*s >= 'a' && *s <= 'f') *v += (uint64_t)((unsigned char)(*s)-'a'+10);
        else if(*s >= 'A' && *s <= 'F') *v += (uint64_t)((unsigned char)(*s)-'A'+10);
        s++;
    } while((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'));
    if(*v < min) *v = min;
    if(max != 0 && *v > max) *v = max;
    return s;
}

/**
 * decimális érték értelmezése, hexára vált 0x előtag esetén
 */
unsigned char *env_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)
{
    while(*s == ' ' || *s == '\t') s++;
    if(*s == '0' && *(s+1) == 'x') return env_hex(s+2, v, min, max);
    *v=0;
    do{
        *v *= 10;
        *v += (uint64_t)((unsigned char)(*s)-'0');
        s++;
    } while(*s >= '0' && *s <= '9');
    if(*v < min) *v = min;
    if(max !=0 && *v > max) *v = max;
    return s;
}

/**
 * időzóna értelmezése
 */
unsigned char *env_tz(unsigned char *s)
{
    int sign=1;
    uint64_t v;

    while(*s == ' ' || *s == '\t') s++;
    if(*s == '-') { s++; sign = -1; }
    if(*s >= '0' && *s <= '9') {
        s=env_dec(s, &v, 0, 1440);
        bootboot.timezone = (int16_t)(sign*v);
        asktime = false;
        return s;
    } else if(*s == 'a') {
        bootboot.timezone = 0;
        asktime = true;
    }
    return s+1;
}

/**
 * logikai hamis (false) értelmezése, egyébként alapból igazat ad vissza
 */
unsigned char *env_bool(unsigned char *s, uint64_t *v, uint64_t flag)
{
    while(*s == ' ' || *s == '\t') s++;
    if(!(*s == '0' || *s == 'f' || *s == 'F' || *s == 'n' || *s == 'N' || *s == 'h' || *s == 'H' || *s == 'k' || *s == 'K' ||
        *s == 'd' || *s == 'D' || (*s == 'o' && *(s+1) == 'f') || (*s == 'O' && *(s+1) == 'F'))) *v |= flag;
    return s+1;
}

/**
 * megjelenítő értelmezése
 */
unsigned char *env_display(unsigned char *s)
{
    uint64_t tmp;
    char *t, *e;

    while(*s == ' ' || *s == '\t') s++;
    if(*s >= '0' && *s <= '9') {
        s = env_dec(s, &tmp, 0, 3);
        display = (uint8_t)tmp;
    } else {
        display = 1;/*PBT_MONO_COLOR;*/
        /* skip separators */
        while(*s == ' ' || *s == '\t') s++;
        if(s[0] == 'm' && s[1] == 'm')  display = PBT_MONO_MONO;
        if(s[0] == 'm' && s[1] == 'c')  display = PBT_MONO_COLOR;
        if(s[0] == 's' && s[1] == 'm')  display = PBT_STEREO_MONO;
        if(s[0] == 'a' && s[1] == 'n')  display = PBT_STEREO_MONO;
        if(s[0] == 's' && s[1] == 'c')  display = PBT_STEREO_COLOR;
        if(s[0] == 'v' && s[1] == 'r')  display = PBT_STEREO_COLOR;
        while(*s != 0 && *s != '\n' && *s != ',') s++;
    }
    /* ha van megadva eszközmeghajtó név is */
    if(*s==',') {
        s++;
        memcpy(display_drv, "/sys/drv/display/",17);
        t = display_drv + 17;
        while(*s != 0 && *s != '\n' && *s != '/' && *s != '.' && *s != ':' && (uint64_t)(t-display_drv+4) < sizeof(display_drv))
            *t++ = *s++;
        if(*s == ':') {
            /* kikényszerített DISPLAY környezeti változó értéke */
            e = display_env;
            s++;
            while(*s != 0 && *s != '\n' && (uint64_t)(e-display_env+1) < sizeof(display_env))
                *e++ = *s++;
            *e = 0;
            /* kikényszerített távoli végpont */
            t = display_drv + 17 + 6;
            memcpy(display_drv + 17, "remote", 6);
        }
        memcpy(t, ".so", 4);
        while(*s != 0 && *s != '\n') s++;
    }
    return s;
}

/**
 * nyelvi kód értelmezése
 */
unsigned char *env_lang(unsigned char *s)
{
    uint i = 0;
    while(*s == ' ' || *s == '\t') s++;
    lang[0] = 0;

    while(i < sizeof(lang)-1 && ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || *s == '_')) lang[i++] = *s++;
    if(i<2 || !lang[0]) { lang[0] = 'e'; lang[1] = 'n'; i = 2; }
    lang[i] = 0;
    return s;
}

/**
 * debug jelzők értelmezése
 */
unsigned char *env_debug(unsigned char *s)
{
    uint64_t tmp;

    if(*s >= '0' && *s <= '9') {
        s = env_dec(s, &tmp, 0, 0xFFFF);
        debug = (uint16_t)tmp;
        return s;
    }
    debug = 0;
    while(*s != 0 && *s != '\n') {
        /* szeparátorok kihagyása */
        if(*s==' '||*s=='\t'||*s==',') { s++; continue; }
        /* lezáró karakterek */
        if((s[0]=='f'&&s[1]=='a') || (s[0]=='n'&&(s[1]=='o'||s[1]=='e'))) { debug = 0; break; }
        /* debug jelzők */
        if(s[0]=='d' && s[1]=='e')              debug |= DBG_DEVICES;
        if(s[0]=='l' && s[1]=='o')              debug |= DBG_LOG;
#if DEBUG
        if(s[0]=='p' && s[1]=='r')              debug |= DBG_PROMPT;
        if(s[0]=='m' && s[1]=='s')              debug |= DBG_MSG;
        if(s[0]=='m' && s[1]=='m')              debug |= DBG_MEMMAP;
        if(s[0]=='t' && s[1]=='a')              debug |= DBG_TASKS;
        if(s[0]=='e' && s[1]=='l')              debug |= DBG_ELF;
        if(s[0]=='r' && (s[1]=='i'||s[2]=='i')) debug |= DBG_RTIMPORT;
        if(s[0]=='r' && (s[1]=='e'||s[2]=='e')) debug |= DBG_RTEXPORT;
        if(s[0]=='i' && s[1]=='r')              debug |= DBG_IRQ;
        if(s[0]=='s' && s[1]=='c')              debug |= DBG_SCHED;
        if(s[0]=='p' && s[1]=='m')              debug |= DBG_PMM;
        if(s[0]=='v' && s[1]=='m')              debug |= DBG_VMM;
        if(s[0]=='m' && s[1]=='a')              debug |= DBG_MALLOC;
        if(s[0]=='b' && s[1]=='l')              debug |= DBG_BLKIO;
        if(s[0]=='f' && s[1]=='i')              debug |= DBG_FILEIO;
        if(s[0]=='f' && s[1]=='s')              debug |= DBG_FS;
        if(s[0]=='u' && s[1]=='i')              debug |= DBG_UI;
        if(s[0]=='t' && s[1]=='e')              debug |= DBG_TESTS;
#endif
        s++;
    }
    return s;
}

/**
 * segédfunkció az idősztring szerkesztéséhez
 */
void env_asktime_setdigit(uint8_t c, uint32_t i)
{
    uint8_t *p = bootboot.datetime;

    if(c > '9') c = '0';
    if(c < '0') c = '9';
    c -= '0';
    switch(i) {
        case 14:
        case 15: if(c < 5) bootboot.timezone -= 60; else bootboot.timezone += 60; break;
        case 16: if(c < 5) bootboot.timezone -= 10; else bootboot.timezone += 10; break;
        case 17: if(c < 5) bootboot.timezone--; else bootboot.timezone++; break;
        default:
            p[i/2] &= i%2? 0xF0 : 0xF;
            p[i/2] |= i%2? c : c*16;
            if(p[2] < 0x01) p[2] = 0x12;
            if(p[2] > 0x12) p[2] = 0x01;
            if(p[3] < 0x01) p[3] = 0x31;
            if(p[3] > 0x31) p[3] = 0x01;
            if(p[4] > 0x23) p[4] = 0x23;
            if(p[5] > 0x59) p[5] = 0x59;
            if(p[6] > 0x59) p[6] = 0x59;
            break;
    }
    if(bootboot.timezone < -1440) bootboot.timezone = 1440;
    if(bootboot.timezone > 1440) bootboot.timezone = -1440;
}

/**
 * Pontos idő bekérése a felhasználótól. Ez nagyon korai, még semmink sincs. Nincsen billentyűzet eszközünk
 * se megszakítások, se memóriagazdálkodás például, ezért a korai indító konzolt használjuk pollozva.
 */
void env_asktime()
{
    uint8_t t[32], *p;
    uint32_t i=0,e=0,cur=0;
    uint16_t c;
    /* Clang félrefordítja az osztást, ha ezek nem 64 bitesek... */
    uint64_t tz,t1,t2,t3,t4;

    kprintf("%s\n", TXT_asktime&&TXT_asktime[0]? TXT_asktime : "Current date and time?");
    while(true) {
        p = bootboot.datetime;
        tz = bootboot.timezone<0? -bootboot.timezone : bootboot.timezone;
        t1 = tz/600; t2 = (tz/60)%10; t3 = (tz%60)/10; t4 = tz%10;
        sprintf((char*)&t, "%1x%1x-%1x-%1x %1x:%1x:%1x GMT%c%1d%1d:%1d%1d", p[0], p[1], p[2], p[3], p[4], p[5], p[6],
            bootboot.timezone<0?'-':'+', t1, t2, t3, t4);
/* Raspberry Pi-n a soros portot használjuk, akkor is, ha egyébként debug konzol nélkül fordítottuk a core-t */
#if DEBUG || defined(__rpi__)
        platform_dbgputc('\r');
#endif
        i = 0;
        kprintf_fg(0xC0C0C0);
        kprintf_bg(0);
        for(kx=0; t[kx] != 0; kx++) {
            if(t[kx]>='0' && t[kx]<='9') {
                if(i == e) {
                    kprintf_fg(0);
                    kprintf_bg(0xC0C0C0);
                    cur = kx;
                }
                i++;
            }
            kprintf_putchar(t[kx]);
#if DEBUG==0 && defined(__rpi__)
            platform_dbgputc(t[kx]);
#endif
            if(cur == kx) {
                kprintf_fg(0xC0C0C0);
                kprintf_bg(0);
            }
        }
#if DEBUG || defined(__rpi__)
        platform_dbgputc('\r');
        if(cur > 0) {
            platform_dbgputc(27);
            platform_dbgputc('[');
            if(cur > 9)
                platform_dbgputc(((cur/10)%10)+'0');
            platform_dbgputc((cur%10)+'0');
            platform_dbgputc('C');
        }
#endif
        /* trükkös billentyűkiolvasás, mivel soros portról CSI eszkép szekvenciákat is olvashatunk */
        c = platform_waitkey();
        if(c == 1 || c == 28) break;
#if DEBUG || defined(__rpi__)
        /* eleve karakterkódot kaptunk billentyúkód helyett? */
        if(c & 0x8000) {
            c &= ~0x8000;
            if(c == '\n' || c == '\r') break;
            if(c == 27) {
                c = platform_waitkey();
                if(c == 0x801b) break;
                else
                if(c == (0x8000|'[')) {
                    c = platform_waitkey();
                    switch(c) {
                        case 0x8000|'A': c = 328; break;
                        case 0x8000|'B': c = 336; break;
                        case 0x8000|'C': c = 333; break;
                        case 0x8000|'D': c = 331; break;
                        default:  c = 0;
                    }
                } else
                    c = 0;
            } else
            if(c >= '0' && c <= '9') c -= '0'-1;
        }
#endif
        /* most a c-ben konzisztens a billentyűkód (scancode) */
        switch(c) {
            /* balra */  case 331: if(e > 0) e--; break;
            /* jobbra */ case 333: if(e < 17) e++; break;
            /* fel */    case 328: env_asktime_setdigit(e<14? t[cur]+1 : '9', e); break;
            /* le */     case 336: env_asktime_setdigit(e<14? t[cur]-1 : '0', e); break;
            /* számok */ default:  env_asktime_setdigit(c+'0'-1, e); if(e < 17) e++; break;
        }
    }
    /* soros konzol törlése */
#if DEBUG || defined(__rpi__)
    platform_dbgputc('\r');
    platform_dbgputc(27); platform_dbgputc('['); platform_dbgputc('2'); platform_dbgputc('K');  /* sor törlés */
    platform_dbgputc(27); platform_dbgputc('['); platform_dbgputc('A');                         /* kurzor fel */
    platform_dbgputc(27); platform_dbgputc('['); platform_dbgputc('2'); platform_dbgputc('K');  /* sor törlés */
#endif
    /* képernyő törlése */
    kprintf_clearline();
    kprintf_reset();
    kprintf_clearline();
}

/**
 * a környezeti változók szöveg értelmezése
 */
void env_init()
{
    unsigned char *env = environment;
    unsigned char *env_end = environment+__PAGESIZE;
    uint64_t tmp;

    /* paraméterek ellenőrzése, még nincsenek fordítások */
    if(memcmp(&bootboot.magic,BOOTBOOT_MAGIC,4) || bootboot.size>__PAGESIZE) kpanic("BOOTBOOT environment corrupt");

    /* alapértelmezett értékek beállítása */
    flags = FLAG_SYSLOG | FLAG_INET | FLAG_SOUND | FLAG_PRINT;
    dmabuf = 0;
    quantum = 1000;
    display = PBT_MONO_COLOR;
    debug = DBG_NONE;
    asktime = *((uint32_t*)&bootboot.datetime) == 0;
    lang[0]='e'; lang[1]='n'; lang[2]=0;

    /* platform függő alapértékek */
    memcpy((void*)&systables, (void*)&bootboot.arch, 64);
    platform_env();

    /* környezeti változók szövegének értelmezése */
    while(env < env_end && *env!=0) {
        /* kommentek és szóközök kihagyása */
        if((env[0]=='/'&&env[1]=='/') || env[0]=='#') { while(env < env_end && env[0]!=0 && env[0]!='\n') env++; }
        if(env[0]=='/'&&env[1]=='*') { env+=2; while(env < env_end && env[0]!=0 && env[-1]!='*' && env[0]!='/') env++; }
        if(*env=='\r'||*env=='\n'||*env=='\t'||*env==' ') { env++; continue; }
        /* kulcsok keresése */
        if(!memcmp(env, "tz=", 3))          { env += 3; env = env_tz(env); } else
        if(!memcmp(env, "lang=", 5))        { env += 5; env = env_lang(env); } else
        if(!memcmp(env, "dmabuf=", 7))      { env += 7; env = env_dec(env, &dmabuf, 0, 2048); } else
        if(!memcmp(env, "aslr=", 5))        { env += 5; env = env_bool(env, &flags, FLAG_ASLR); } else
        if(!memcmp(env, "syslog=", 7))      { env += 7; env = env_bool(env, &flags, FLAG_SYSLOG); } else
        if(!memcmp(env, "internet=", 10))   { env +=10; env = env_bool(env, &flags, FLAG_INET); } else
        if(!memcmp(env, "sound=", 6))       { env += 6; env = env_bool(env, &flags, FLAG_SOUND); } else
        if(!memcmp(env, "print=", 6))       { env += 6; env = env_bool(env, &flags, FLAG_PRINT); } else
        if(!memcmp(env, "rescueshell=", 12)){ env +=12; env = env_bool(env, &flags, FLAG_RESCUESH); } else
        if(!memcmp(env, "spacemode=", 11))  { env +=11; env = env_bool(env, &flags, FLAG_SPACEMODE); } else
        if(!memcmp(env, "quantum=", 8))     { env += 8; env = env_dec(env, &quantum, 10, 1000000); } else
        if(!memcmp(env, "display=", 8))     { env += 8; env = env_display(env); } else
        if(!memcmp(env, "debug=", 6))       { env += 6; env = env_debug(env); } else
            env = platform_parse(env);                  /* platform függő kulcsok keresése */
    }
    lang_init();                                        /* nyelvi fordítások betöltése */

    if(asktime && !(flags & FLAG_SPACEMODE)) {          /* pontos idő bekérése */
        env_asktime();
        srand[(1000*bootboot.datetime[7] + 100*bootboot.datetime[6] + bootboot.datetime[5])&3] *= 16807;
        kentropy();
    }

    /* indítási napló írása, most hogy már van pontos időnk */
    syslog(LOG_INFO, OSZ_NAME " " OSZ_VER " " OSZ_ARCH "-" OSZ_PLATFORM " %s", &lang);
    tmp=srand[0]^srand[1];
    syslog(LOG_INFO, "Started uuid %4x-%2x-%2x-%8x", (uint32_t)tmp,(uint16_t)(tmp>>32),(uint16_t)(tmp>>48),srand[2]^srand[3]);
    syslog(LOG_INFO, "Frame buffer %d x %d @%x sc %d",bootboot.fb_width,bootboot.fb_height,bootboot.fb_ptr,bootboot.fb_scanline);
}
