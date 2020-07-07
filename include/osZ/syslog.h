/*
 * include/osZ/syslog.h
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
 * @brief OS/Z specifikus rendszer típusok
 */

#ifndef _SYSLOG_H
#define _SYSLOG_H 1

/* prioritások */
#define LOG_EMERG   0   /* a rendszer használhatatlanná vált */
#define LOG_ALERT   1   /* azonnal közbe kell avatkozni */
#define LOG_CRIT    2   /* kritikus, rendszerszolgáltatás kiesés */
#define LOG_ERR     3   /* hiba történt */
#define LOG_WARNING 4   /* nem normális működésre figyelmeztetés */
#define LOG_NOTICE  5   /* normális, de figyelmet igényő működés */
#define LOG_INFO    6   /* információ */
#define LOG_DEBUG   7   /* debug szintű üzenet */

/* alrendszer kódok (facility), igyekeztem úgy alakítani, hogy minnél POSIX kompatíbilisebb legyen... */
#define LOG_CORE    (0<<3)  /* core üzenetek (LOG_KERN) */
#define LOG_USER    (1<<3)  /* tetszőleges felhasználói szintű üzenet */
#define LOG_MAIL    (2<<3)  /* levelező alrendszer */
#define LOG_INIT    (3<<3)  /* init alrendszer (LOG_DAEMON) */
#define LOG_AUTH    (4<<3)  /* biztonsággal, azonosítással kapcsolatos */
#define LOG_SYSLOG  (5<<3)  /* a syslog által generált üzenetek */
#define LOG_LPR     (6<<3)  /* nyomtatási sor alrendszer */
#define LOG_INET    (7<<3)  /* hálózat alrendszer (LOG_NEWS) */
#define LOG_FS      (8<<3)  /* FS taszk alrendszer (LOG_UUCP) */
#define LOG_CRON    (9<<3)  /* ütemezett feladatok üzenetei */
#define LOG_AUTHPRIV (10<<3)/* biztonsággal, azonosítással kapcsolatos (privát) */
#define LOG_SOUND   (11<<3) /* hangkeverő üzenetei (LOG_FTP) */

/* a setlogmask paramétere */
#define LOG_MASK(pri)   (1 << (pri))            /* egy prioritás */
#define LOG_UPTO(pri)   ((1 << ((pri)+1)) - 1)  /* minden prioritás pri-ig bezárólag */

/* openlog opciók (nincs nagyon használva) */
#define LOG_PERROR  0x20    /* az stderr-ra is küldje az üzenetet */

#ifndef _AS
/* rendszernaplózóhoz küldött üzenetek alaphelyzetbe állítása */
extern void closelog(void);

/* rendszernaplózóhoz küldött üzenetek beállítása */
extern void openlog(char *ident, int option, int facility);

/* prioritás beállítása */
extern int setlogmask(int mask);

/* formázott üzenet küldése a rendszernaplózónak, változó számú paraméterrel */
extern void syslog(int pri, char *fmt, ...);

/* formázott üzenet küldése a rendszernaplózónak, paraméterlistával */
extern void vsyslog(int pri, char *fmt, va_list ap);
#endif

#endif /* syslog.h */
