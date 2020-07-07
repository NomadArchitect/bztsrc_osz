/*
 * libc/syslog.c
 *
 * Copyright (c) 2020 bzt (bztsrc@gitlab)
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
 * @brief a syslog.h-ban definiált függvények megvalósítása
 */

#include <osZ.h>

char *syslog_ident = NULL;
int syslog_facility = LOG_USER, syslog_option = 0, syslog_pri = LOG_UPTO(LOG_NOTICE);

/**
 * rendszernaplózóhoz küldött üzenetek beállítása
 */
void openlog(char *ident, int option, int facility)
{
    syslog_ident = ident;
    syslog_option = option;
    syslog_facility = facility;
}

/**
 * rendszernaplózóhoz küldött üzenetek alaphelyzetbe állítása
 */
void closelog()
{
    syslog_ident = NULL;
    syslog_option = 0;
    syslog_facility = LOG_USER;
    syslog_pri = LOG_UPTO(LOG_NOTICE);
}

/**
 * prioritás beállítása
 */
int setlogmask(int mask)
{
    if(mask) syslog_pri = mask;
    return syslog_pri;
}

/**
 * formázott üzenet küldése a rendszernaplózónak, paraméterlistával
 */
void vsyslog(int pri, char *fmt, va_list ap)
{
    char buf[__PAGESIZE];
    size_t s;
    if(!fmt || !*fmt || !(syslog_pri & LOG_MASK(pri))) return;
    s = vsnprintf(buf, sizeof(buf)-1, fmt, ap);
    /* ez trükkös, mert ha még nem inicializált a syslog, akkor a korai konzolt kell használni */
    mq_send3(SRV_CORE, SYS_log|MSG_PTRDATA, buf, s+1, pri);
    if(errno() == EACCES) {
        /* üzenet küldése a rendszernaplózó szolgáltatásnak */
        mq_send5(SRV_syslog, SYS_syslog|MSG_PTRDATA, buf, s, pri, syslog_facility, syslog_ident ? syslog_ident : "user");
        /* ha kérték, akkor a sztandard hibakimenetre is */
        if(syslog_option & LOG_PERROR)
            fwrite(stderr, buf, s);
    }
}

/**
 * formázott üzenet küldése a rendszernaplózónak, változó számú paraméterrel
 */
void syslog(int pri, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsyslog(pri, fmt, args);
}

