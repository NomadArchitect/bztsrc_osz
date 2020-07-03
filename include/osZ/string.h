/*
 * include/osZ/string.h
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
 * @brief ISO C99 Sztandard: 7.21 Sztring kezelés
 */

#ifndef _STRING_H
#define _STRING_H   1

#ifndef _AS
/* N bájt másolása SRC-ből a DST taszk címterének DEST címére  */
extern void tskcpy (pid_t dst, void *dest, void *src, size_t n);

/* N bájtot nulláz az S címen  */
extern void *memzero (void *s, size_t n);
/* N bájt másolása SRC-ből DEST-be */
extern void *memcpy (void *dest, const void *src, size_t n);
/* N bájt másolása SRC-ből DEST-be, a sztringet átlapolhatják egymást  */
extern void *memmove (void *dest, const void *src, size_t n);
/* S feltöltése N bájtnyi C karakterrel */
extern void *memset (void *s, int c, size_t n);
/* N bájt összehasonlítása S1 és S2 címen */
extern int memcmp (const void *s1, const void *s2, size_t n);
/* Nem UTF-8 biztos, C karakter legelső vagy legutolsó előfordulását adja vissza, lásd strchr() */
extern void *memchr (const void *s, int c, size_t n);
extern void *memrchr (const void *s, int c, size_t n);
/* visszaadja a legelső 1 bit pozicióját, 0-át ha nincs. legalacsonyabb helyiértékű bit 1, legmagasabb 64.  */
extern uint64_t ffs (uint64_t i);

/* NEDDLELEN hosszú NEEDLE első előfordulásának keresése a HAYSTACKLEN hosszú HAYSTACK-ben */
extern void *memmem (const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);

/* visszaadja az ERRNUM `errno' kód szöveges megfelelőjét sztringben */
extern char *strerror (int errnum);
/* visszaadja a SIG szignál szöveges nevét sztringben */
extern char *strsignal (int sig);

/* utf-8 szekvenciává alakítja az UNICODE-ot, majd lépteti a sztringmutatót */
void chr(uint8_t **s, uint32_t u);
/* lépteti a sztringmutatót a következő utf-8 karakterre és visszaadja az UNICODE kódpontot */
uint32_t ord(uint8_t **s);
/* lépteti a sztringmutatót a következő utf-8 karakterre és visszaadja a kisbetűs utf-8 változatot */
uint32_t strtolower(uint8_t **s);

/* visszaadja az S sztring bájtjainak (nem karaktereinek) számát */
extern size_t strlen (const char *s);
extern size_t strnlen (const char *s, size_t maxlen);
/* visszaadja a többbájtos karakterek (UTF-8 szekvenciák) számát S-ben */
extern size_t mbstrlen (const char *s);
/* visszaadja az UTF-8 karakterek számát S-ben, maximum N hosszig  */
extern size_t mbstrnlen (const char *s, size_t n);

/* S1 és S2 sztringek összehasonlítása  */
extern int strcmp (const char *s1, const char *s2);
/* S1 és S2 összehasonlítása legfeljebb N hosszan  */
extern int strncmp (const char *s1, const char *s2, size_t n);
/* S1 és S2 összehasonlítása, kis- és nagybetű függetlenül */
extern int strcasecmp (const char *s1, const char *s2);
/* S1 és S2 összehasonlítása legfeljebb N bájt hosszan, kis- és nagybetű függetlenül */
extern int strncasecmp (const char *s1, const char *s2, size_t n);

/* SRC hozzáfúzése DEST-hez */
extern char *strcat (char *dest, const char *src);
/* legfeljebb N karakter hozzáfűzése SRC-ből DEST-hez  */
extern char *strncat (char *dest, const char *src, size_t n);

/* sztring másolása SRC-ből DEST-be */
extern char *strcpy (char *dest, const char *src);
/* nem több, mint N karakter másolása SRC-ből DEST-be  */
extern char *strncpy (char *dest, const char *src, size_t n);
/* sztring másolása kisbetüsítve SRC-ből DEST-be */
extern char *strcasecpy (char *dest, const char *src);
/* nem több, mint N karakter másolása kisbetüsítve SRC-ből DEST-be  */
extern char *strncasecpy (char *dest, const char *src, size_t n);

/* S duplikálása egy újonnan allokált bufferbe  */
extern char *strdup (const char *s);
extern char *strndup (const char *s, size_t n);
/* ugyanaz, csak kisbetűsre konvertálva */
extern char *strcasedup (const char *s);
extern char *strncasedup (const char *s, size_t n);

/* OS/Z-ben ezek a kereső funkciók UTF-8 biztosak, és UTF-8 karaktert várnak */
extern char *strchr (const char *s, uint32_t c);
extern char *strrchr (const char *s, uint32_t c);
/* NEEDLE első előfordulásának keresése a HAYSTACK sztringben */
extern char *strstr (const char *haystack, const char *needle);
/* hasonló a `strstr'-hez, de kis- és nagybetű független  */
extern char *strcasestr (const char *haystack, const char *needle);

/* elérési út fájlnév részét adja vissza */
extern char *basename (const char *s);
/* elérési út könyvtár részét adja vissza */
extern char *dirname (const char *s);

/* S feldarabolása DELIM karaktereknél */
extern char *strtok (char *s, const char *delim);
extern char *strtok_r (char *s, const char *delim, char **ptr);
/* visszaadja a DELIM határolt sztringet, 0-ával lezárva, *STRINGP-t pedig a következő bájtra állítva */
extern char *strsep (char **stringp, const char *delim);

#endif

#endif /* string.h  */
