Felhasználói alkalmazások
=========================

Függvénykönyvtárak, programok és felhasználói szolgáltatások, amik az /usr alá kerülnek felcsatolásra.

Ez a könyvtár, bár usr (UNIX Shared Resources) a neve, alapvetően eltér az OS/Z-ben a többi UNIX-tól. Az
első alkönyvtárszint a csomag neve, és a szokásos alkönyvtárak ez alá kerültek. Ez szándékosan felrúgja
a [Fájl Hierarchia Sztandard](http://www.pathname.com/fhs/)ot, mert így sokkal könnyebb a csomagkezelőnek
nyomon követnie a fájlokat. Olyan ez, mint az /usr/X11R6 vagy az /opt Linux alatt, vagy akár az /Applications
könyvtár MaxOSX alatt.

A POSIX-os és FHS-es kompatibilitás kedvéért az OS/Z uniókat használ, például

```
/usr/share -> /usr/.../share
```

vagy

```
/etc -> /sys/etc, /usr/.../etc
```

Példák:
---------

| OS/Z változat        | Tipikus helye UNIXon |
| -------------------- | -------------------- |
| /usr/X11R6/bin       | /usr/X11R6/bin       |
| /usr/sys/include     | /usr/include         |
| /usr/openssl/include | /usr/include/openssl |
| /usr/gcc/bin/gcc     | /usr/bin/gcc         |
| /usr/gnupg/lib       | /usr/lib/gnupg       |
| /usr/GeoIP/share     | /usr/share/GeoIP     |
| /usr/sys/man         | /usr/share/man       |
| /usr/vlc/man/man1    | /usr/share/man/man1  |
