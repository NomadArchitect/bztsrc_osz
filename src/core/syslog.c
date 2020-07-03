/*
 * core/syslog.c
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
 * @subsystem konzol
 * @brief Korai rendszernaplózó. Megosztott területet használ a "syslog" taszkkal
 */

#include <core.h>

/* a buffer */
char bssalign syslog_buffer[__PAGESIZE];
char *syslog_ptr = syslog_buffer;

/**
 * korai RFC5424 kompatíbilis naplózó
 */
void syslog(int pri, char* fmt, ...)
{
    bool_t hasService = (runlevel == RUNLVL_NORM && services[-SRV_syslog]);
    char *p, *oldptr;
    va_list args;
    va_start(args, fmt);

    /* paraméterek ellenőrzése */
    if(fmt == NULL || (((virt_t)fmt>>48)!=0 && ((virt_t)fmt>>48)!=0xffff) || !fmt[0])
        fmt = "???";

    /* ha még nem fut a naplózó szolgáltatás */
    if(!hasService) {
        /* körbefordítjuk, ha túlcsordulna */
        while(*syslog_ptr) syslog_ptr++;
        if((virt_t)syslog_ptr >= (virt_t)(syslog_buffer + __PAGESIZE-256)) {
            syslog_ptr = syslog_buffer;
        }
        oldptr = syslog_ptr;

        /* dátum, idő BCD-ben, processz előtag */
        p = (char*)&bootboot.datetime;
        syslog_ptr = sprintf(syslog_ptr, "%1x%1x-", p[0], p[1]);
        syslog_ptr = sprintf(syslog_ptr, "%1x-%1xT", p[2], p[3]);
        syslog_ptr = sprintf(syslog_ptr, "%1x:%1x:%1x", p[4], p[5], p[6]);
        *syslog_ptr = (bootboot.timezone<0?'-':'+'); syslog_ptr++;
        syslog_ptr = sprintf(syslog_ptr, "%1d%1d:",
            (bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/600,
            ((bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/60)%10);
        syslog_ptr = sprintf(syslog_ptr, "%1d%1d - boot - - - ",
            ((bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)%60)/10,
            (bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)%10);
    } else
        syslog_ptr = syslog_buffer;

    /* üzenet átmásolása */
    syslog_ptr = vsprintf(syslog_ptr, fmt, args);
    while(*(syslog_ptr-1)=='\n') syslog_ptr--;

    if(!hasService) {
        *syslog_ptr++ = '\n';
        *syslog_ptr = 0;
        /* ha hibaüzenet, akkor pirosra váltunk */
        if(pri == LOG_ERR
#if !DEBUG
            && (debug&DBG_LOG)
#endif
        ) {
            kprintf_fg(0xFF8080);
#if DEBUG
            dbg_putchar(27);
            dbg_putchar('[');
            dbg_putchar('3');
            dbg_putchar('1');
            dbg_putchar('m');
#endif
        }
        if(debug&DBG_LOG) {
            kprintf(oldptr);
#if DEBUG
        } else {
            /* kiküldés a soros vonali debug konzolra */
            for(p=oldptr;oldptr<syslog_ptr;oldptr++)
                dbg_putchar(*oldptr);
#endif
        }
        if(pri == LOG_ERR
#if !DEBUG
            && (debug&DBG_LOG)
#endif
        ) {
            kprintf_fg(0xC0C0C0);
#if DEBUG
            dbg_putchar(27);
            dbg_putchar('[');
            dbg_putchar('0');
            dbg_putchar('m');
#endif
        }
    } else {
        *syslog_ptr = 0;
        /* üzenet küldése a rendszernaplózó szolgáltatásnak */
        msg_send((virt_t)&syslog_buffer, pri, LOG_KERN, (virt_t)"core", 0, 0,
            EVT_DEST(SRV_syslog) | EVT_FUNC(SYS_syslog) | MSG_PTRDATA, 0);
    }
}
