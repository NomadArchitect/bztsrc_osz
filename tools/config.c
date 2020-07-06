/*
 * tools/config.c
 *
 * Copyright (C) 2020 bzt (bztsrc@gitlab)
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
 * @subsystem eszközök
 * @brief Egyszerű curses segédprogram a konfiguráláshoz
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#define __USE_MISC 1            /* DT_* enum miatt */
#include <dirent.h>

typedef struct {
    char *arch;
    char *name;
    char *desc;
} plat_t;
typedef struct {
    char *name;
    char *desc;
    char *setup;
} comp_t;
typedef struct {
    int val;
    char *name;
    int langcode;
} list_t;
typedef struct {
    char *layout;
    char **name;
    int num;
} kbd_t;

/* fordítások */
char **lang, *lang_en[] = {
    /*  0 */ "memory error",
    /*  1 */ "unable to read",
    /*  2 */ "unable to write",
    /*  3 */ "Configuration saved.",
    /*  4 */ "OS/Z Configuration",
    /*  5 */ "Compile time options",
    /*  6 */ "Boot options",
    /*  7 */ "Platform",
    /*  8 */ "Compiler",
    /*  9 */ "Compile options",
    /* 10 */ "Disk image generation",
    /* 11 */ "System services",
    /* 12 */ "Time zone",
    /* 13 */ "Screen resolution",
    /* 14 */ "Display visual",
    /* 15 */ "Display options",
    /* 16 */ "Interface language",
    /* 17 */ "Keyboard type",
    /* 18 */ "Keyboard layout",
    /* 19 */ "Debug options",
    /* 20 */ "OK",
    /* 21 */ "Save",
    /* 22 */ "Quit",
    /* 23 */ "Choose architecture and platform",
    /* 24 */ "Select compiler",
    /* 25 */ "Additional options",
    /* 26 */ "Internal debugger and debug console",
    /* 27 */ "Compile with debug symbols",
    /* 28 */ "Use optimized code and utilize SIMD",
    /* 29 */ "",
    /* 30 */ "",
    /* 31 */ "",
    /* 32 */ "",
    /* 33 */ "",
    /* 34 */ "Initrd place",
    /* 35 */ "partition",
    /* 36 */ "System",
    /* 37 */ "Public data",
    /* 38 */ "Private data",
    /* 39 */ "size",
    /* 40 */ "Raspberry Pi does not support initrd on a partition.",
    /* 41 */ "Autodetect (if there's an RTC)",
    /* 42 */ "Select system services to start on boot up",
    /* 43 */ "Automatic reboot on failure",
    /* 44 */ "Rescue shell (instead of init)",
    /* 45 */ "System log service",
    /* 46 */ "Internet service (TCP/IP)",
    /* 47 */ "Sound service",
    /* 48 */ "Print service",
    /* 49 */ "",
    /* 50 */ "",
    /* 51 */ "min",
    /* 52 */ "Select time zone",
    /* 53 */ "Always ask user",
    /* 54 */ "Force UTC (Greenwich meantime)",
    /* 55 */ "Set time zone in minutes",
    /* 56 */ "Mono Monochrome (small memory)",
    /* 57 */ "Mono trueColor (default)",
    /* 58 */ "Stereo Monochrome (anaglyph)",
    /* 59 */ "Stereo trueColor (Real 3D)",
    /* 60 */ "Double buffering (more memory)",
    /* 61 */ "Automatically focus new windows",
    /* 62 */ "Left handed pointers",
    /* 63 */ "",
    /* 64 */ "",
    /* 65 */ "",
    /* 66 */ "Select language",
    /* 67 */ "Select keyboard type",
    /* 68 */ "Select keyboard layout",
    /* 69 */ "Select alternative layout",
    /* 70 */ "None",
    /* 71 */ "Select debug messages to show",
    /* 72 */ "Debugger prompt before the first task switch",
    /* 73 */ "Send syslog to boot console",
    /* 74 */ "Dump detected devices",
    /* 75 */ "Run system tests (instead of init)",
    /* 76 */ "Dump memory map to debug console (boot log has it)",
    /* 77 */ "Show task creation",
    /* 78 */ "Trace ELF loading",
    /* 79 */ "Dump run-time linker imported symbols",
    /* 80 */ "Dump run-time linker exported symbols",
    /* 81 */ "Dump IRQ Routing Table",
    /* 82 */ "Trace scheduler",
    /* 83 */ "Trace message queue (aka Linux strace)",
    /* 84 */ "Trace physical memory allocations",
    /* 85 */ "Trace virtual memory mapping (core)",
    /* 86 */ "Trace virtual memory allocation (libc malloc)",
    /* 87 */ "Trace block level I/O",
    /* 88 */ "Trace file level I/O",
    /* 89 */ "Trace file systems (mounts and such)",
    /* 90 */ "Trace block cache",
    /* 91 */ "Trace UI requests and messages",
    /* 92 */ "Trace print job messages",
    /* 93 */ "",
    /* 94 */ "",
    /* 95 */ "",
    /* 96 */ "",
    /* 97 */ "",
    /* 98 */ "",
    /* 99 */ "",
}, *lang_hu[] = {
    /*  0 */ "memória hiba",
    /*  1 */ "nem tudom olvasni",
    /*  2 */ "nem tudom írni",
    /*  3 */ "Konfiguráció elmentve.",
    /*  4 */ "OS/Z Konfigurálás",
    /*  5 */ "Fordítási opciók",
    /*  6 */ "Futás idejű opciók",
    /*  7 */ "Platform",
    /*  8 */ "Fordító",
    /*  9 */ "Fordító paraméterek",
    /* 10 */ "Lemezkép generálás",
    /* 11 */ "Rendszer szolgáltatások",
    /* 12 */ "Időzóna",
    /* 13 */ "Képernyőfelbontás",
    /* 14 */ "Megjelenítés",
    /* 15 */ "Megjelenítési opciók",
    /* 16 */ "Felület nyelve",
    /* 17 */ "Billentyűzet típusa",
    /* 18 */ "Billentyűzet kiosztás",
    /* 19 */ "Debug opciók",
    /* 20 */ "OK",
    /* 21 */ "Elment",
    /* 22 */ "Kilép",
    /* 23 */ "Válassz architektúrát és platformot",
    /* 24 */ "Válassz fordítót",
    /* 25 */ "További opciók",
    /* 26 */ "Beépített debugger és debug konzol",
    /* 27 */ "Debug szimbólumokkal fordítás",
    /* 28 */ "Optimalizált és SIMD kód generálás",
    /* 29 */ "",
    /* 30 */ "",
    /* 31 */ "",
    /* 32 */ "",
    /* 33 */ "",
    /* 34 */ "Initrd helye",
    /* 35 */ "partíció",
    /* 36 */ "Rendszer",
    /* 37 */ "Publikus adat",
    /* 38 */ "Privát adat",
    /* 39 */ "méret",
    /* 40 */ "Raspberry Pi nem támogatja az initrd partíciót",
    /* 41 */ "Autodetektált (ha van RTC)",
    /* 42 */ "Válaszd ki, mely rendszer szolgáltatások induljanak",
    /* 43 */ "Automatikus újraindítás hiba esetén",
    /* 44 */ "Parancsértelmező (init helyett)",
    /* 45 */ "Naplózás szolgáltatás",
    /* 46 */ "Internet szolgáltatás (TCP/IP)",
    /* 47 */ "Hangkeverő szolgáltatás",
    /* 48 */ "Nyomtatási sor szolgáltatás",
    /* 49 */ "",
    /* 50 */ "",
    /* 51 */ "perc",
    /* 52 */ "Időzóna megadása",
    /* 53 */ "Mindig kérdezze meg a felhasználót",
    /* 54 */ "Mindig legyen UTC (greenwichi középidő)",
    /* 55 */ "Beállítás fixre percekben",
    /* 56 */ "Mono Monokróm (kevés memóriát igényel)",
    /* 57 */ "Mono trueColor (alapértelmezett)",
    /* 58 */ "Sztereó Monokróm (anaglif)",
    /* 59 */ "Sztereó trueColor (igazi 3D hatás)",
    /* 60 */ "Kettősbufferelés (dupla memóriát igényel)",
    /* 61 */ "Új ablakok automatikus fókuszálása",
    /* 62 */ "Balkezes egér mutató",
    /* 63 */ "",
    /* 64 */ "",
    /* 65 */ "",
    /* 66 */ "Nyelvválasztás",
    /* 67 */ "Válassz billentyűzet típust",
    /* 68 */ "Válassz elsődleges kiosztást",
    /* 69 */ "Válassz másodlagos kiosztást",
    /* 70 */ "Nincs",
    /* 71 */ "Válaszd ki, mely üzenetek jelenjenek meg",
    /* 72 */ "Debug paranccsor a legelső taszkkapcsolás előtt",
    /* 73 */ "Rendszernapló küldése a korai konzolra",
    /* 74 */ "Detektált eszközök listázása",
    /* 75 */ "Rendszertesztek futtatása (init helyett)",
    /* 76 */ "Memóriatérkép listázása a debug konzolra",
    /* 77 */ "Taszk létrehozás nyomkövetése",
    /* 78 */ "ELF betöltés nyomkövetése",
    /* 79 */ "Futás idejű importált szimbólumok nyomkövetése",
    /* 80 */ "Futás idejű exportált szimbólumok nyomkövetése",
    /* 81 */ "IRQ Routing Table listázása",
    /* 82 */ "Ütemező nyomkövetése",
    /* 83 */ "Üzenetek nyomkövetése (mint a Linux strace)",
    /* 84 */ "Fizikai memóriafoglalás nyomkövetése",
    /* 85 */ "Virtuális memórialeképezés nyomkövetése (core)",
    /* 86 */ "Virtuális memóriafoglalás nyomkövetése (libc malloc)",
    /* 87 */ "Blokk szintű B/K nyomkövetése",
    /* 88 */ "Fájl szintű B/K nyomkövetése",
    /* 89 */ "Fájlrendszerek nyomkövetése (felcsatolás, stb.)",
    /* 90 */ "Blokk gyorsítótár nyomkövetése",
    /* 91 */ "Felhasználói felület üzenetek nyomkövetése",
    /* 92 */ "Nyomtatási sor nyomkövetése",
    /* 93 */ "",
    /* 94 */ "",
    /* 95 */ "",
    /* 96 */ "",
    /* 97 */ "",
    /* 98 */ "",
    /* 99 */ "",
};

/* statikus opciók */
comp_t compilers[] = {
    { "gcc", "GNU gcc + ld", "CC=${ARCH}-elf-gcc -Wno-builtin-declaration-mismatch\nLD=${ARCH}-elf-ld" },
    { "clang", "LLVM Clang + lld", "CC=clang --target=${ARCH}-elf -Wno-builtin-requires-header -Wno-incompatible-library-redeclaration\n"
        "ifeq (${ARCH},x86_64)\nLD=ld.lld -m elf_x86_64\nelse\nLD=ld.lld -m aarch64elf\nendif" }
};
char *screens[] = {
    "640x480", "768x576", "800x600", "1024x768", "1280x720", "1368x768", "1280x800", "1152x864", "1152x900", "1440x900",
    "1280x960", "1280x1024", "1400x1050", "1680x1050", "1600x1200", "1920x1200", "1792x1344", "1856x1392", "1920x1440"
};
list_t compopts[] = { { 0, "DEBUG", 26 }, { 0, "DBGSYM", 27 }, { 1, "OPTIMIZE", 28 } };
list_t srvopts[] = { { 0, "spacemode", 43 }, { 0, "rescueshell", 44 }, { 1, "syslog", 45 }, { 1, "inet", 46 }, { 1, "sound", 47 },
    { 1, "print", 48 } };
list_t visopts[] = { { 0, "mm", 56 }, { 0, "mc", 57 }, { 0, "sm", 58 }, { 0, "sc", 59 } };
list_t dspopts[] = { { 0, "dblbuf", 60 }, { 0, "focusnew", 61 }, { 0, "lefthanded", 62 } };
list_t dbgopts[] = { { 0, "prompt", 72 }, { 0, "log", 73 }, { 0, "dev", 74 }, { 0, "test", 75 }, { 0, "mem", 76 },
    { 0, "task", 77 }, { 0, "elf", 78 }, { 0, "ri", 79 }, { 0, "re", 80 }, { 0, "irq", 81 }, { 0, "sched", 82 },
    { 0, "msg", 83 }, { 0, "pm", 84 }, { 0, "vm", 85 }, { 0, "ma", 86 }, { 0, "blk", 87 }, { 0, "file", 88 }, { 0, "fs", 89 },
    { 0, "cache", 90 }, { 0, "ui", 91 }, { 0, "print", 92 } };

/* detektált opciók */
int numplatform = 0, numlangs = 0, numkbd = 0, numextra = 0, scr = 3, platw = 0, chk, chkscr, chkh, chkl;
plat_t *platform = NULL;
char **langs = NULL, arch[32] = { 0 }, kernel[256] = { 0 }, visdrv[256] = { 0 }, **extra = NULL;
kbd_t *kbds = NULL;
int row = 0, col = 0, confsave = 0;
int selplat = 0, noinitrd = 0, espsize = 8256, usrsize = 768, varsize = 4, homesize = 4, cmplr = 0;
int tz = -99999, vis = 1, sellang = 0, selkbd = 0, sellyt = 0, selalt = 0;
void ctrlc(int sig);
void writeconfig();

#ifndef __OSZ__
struct termios otio, ntio;
int flg;

void getstdindim(int *r, int *c)
{
    struct winsize ws;
    ioctl(0, TIOCGWINSZ, &ws);
    *r = ws.ws_row; *c = ws.ws_col;
}

void restorestdin()
{
    tcsetattr(0, TCSANOW, &otio);
}

void setupstdin()
{
    signal(SIGINT, ctrlc);
    flg = fcntl(0, F_GETFL);
    tcgetattr(0, &otio);
    ntio = otio;
    ntio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(0, TCSANOW, &ntio);
}

char getcsi()
{
    char c, d;
    c = getchar();
    if(c == 0x1b) {
        fcntl(0, F_SETFL, flg | O_NONBLOCK);
        c = getchar();
        if(c == -1) { fcntl(0, F_SETFL, flg); return 0x1b; }
        while(c != -1) { d = c; c = getchar(); }
        fcntl(0, F_SETFL, flg);
        switch(d) {
            case 'A': return 1;
            case 'B': return 2;
            case 'C': return 3;
            case 'D': return 4;
            default: return 0;
        }
    }
    return c;
}
#else
void getstdindim(int *r, int *c)
{
}

void restorestdin()
{
}

void setupstdin()
{
}

char getcsi()
{
}
#endif

void ctrlc(int sig)
{
    restorestdin();
    printf("\033[0m\033[H\033[J\033[?25h");
    if(confsave) { writeconfig(); printf("%s\n",lang[3]); }
    exit(0);
}

void err(char *s)
{
    restorestdin();
    printf("\033[0m\033[H\033[J%s");
    fprintf(stderr,"%s\n",s);
    exit(1);
}

long int fsiz;
char* readfileall(char *file)
{
    char *data=NULL;
    FILE *f;
    fsiz = 0;
    f=fopen(file,"rb");
    if(f){
        fseek(f,0L,SEEK_END);
        fsiz=ftell(f);
        fseek(f,0L,SEEK_SET);
        data=(char*)malloc(fsiz+1);
        if(!data) {
memerr:     err(lang[0]);
        }
        if(fsiz && !fread(data, fsiz, 1, f)) err(lang[1]);
        fclose(f);
        data[fsiz] = 0;
    } else {
        fsiz = 0;
        data=(char*)malloc(1);
        if(!data) goto memerr;
        data[0] = 0;
    }
    return data;
}

int mystrlen(const char *s)
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

void *myrealloc(void *ptr, int size) {
    void *data = realloc(ptr, size);
    if(!data) err(lang[0]);
    return data;
}

void readopts()
{
    FILE *f;
    DIR *dir, *dir2;
    struct dirent *ent, *ent2;
    char dn[64], line[128];
    int i, j;
    if((dir = opendir("../src/core")) != NULL) {
        while((ent = readdir(dir)) != NULL) {
            if(!strcmp(ent->d_name,".") || !strcmp(ent->d_name,"..") || ent->d_type!=DT_DIR) continue;
            sprintf(dn, "../src/core/%s", ent->d_name);
            if((dir2 = opendir(dn)) != NULL) {
                while((ent2 = readdir(dir2)) != NULL) {
                    if(!strcmp(ent2->d_name,".") || !strcmp(ent2->d_name,"..") || ent2->d_type!=DT_DIR) continue;
                    sprintf(dn, "../src/core/%s/%s/Makefile", ent->d_name, ent2->d_name);
                    f = fopen(dn, "r");
                    if(f) {
                        line[0] = '\n';
                        fgets(line, sizeof(line), f);
                        for(i = 0; line[i] != '\n'; i++);
                        line[i] = 0;
                        for(i = 0; line[i] == '#' || line[i] == ' '; i++);
                        platform = (plat_t*)myrealloc(platform, (numplatform+1) * sizeof(plat_t));
                        platform[numplatform].arch = myrealloc(NULL, strlen(ent->d_name) + 1);
                        strcpy(platform[numplatform].arch, ent->d_name);
                        platform[numplatform].name = myrealloc(NULL, strlen(ent2->d_name) + 1);
                        strcpy(platform[numplatform].name, ent2->d_name);
                        j = strlen(line + i); if(j > platw) platw = j;
                        platform[numplatform].desc = myrealloc(NULL, j + 1);
                        strcpy(platform[numplatform].desc, line + i);
                        numplatform++;
                        fclose(f);
                    }
                }
                closedir(dir2);
            }
        }
        closedir(dir);
    }
    if((dir = opendir("../etc/lang")) != NULL) {
        while((ent = readdir(dir)) != NULL) {
            if(memcmp(ent->d_name,"core.", 5)) continue;
            sprintf(dn, "../etc/lang/%s", ent->d_name);
            f = fopen(dn, "r");
            if(f) {
                line[0] = '\n';
                fgets(line, sizeof(line), f);
                for(i = 0; line[i] != '\n'; i++);
                line[i] = 0;
                langs = myrealloc(langs, (numlangs+1) * sizeof(char*));
                langs[numlangs] = myrealloc(NULL, strlen(line) + 4);
                sprintf(langs[numlangs], "%s %s",ent->d_name + 5, line);
                numlangs++;
                fclose(f);
            }
        }
        closedir(dir);
    }
    if((dir = opendir("../etc/etc/kbd")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if(!strcmp(ent->d_name,".") || !strcmp(ent->d_name,"..") || ent->d_type!=DT_DIR) continue;
            kbds = (kbd_t*)myrealloc(kbds, (numkbd+1) * sizeof(kbd_t));
            memset(&kbds[numkbd], 0, sizeof(kbd_t));
            kbds[numkbd].layout = myrealloc(NULL, strlen(ent->d_name) + 1);
            strcpy(kbds[numkbd].layout, ent->d_name);
            sprintf(dn, "../etc/etc/kbd/%s", ent->d_name);
            if((dir2 = opendir(dn)) != NULL) {
                while((ent2 = readdir(dir2)) != NULL) {
                    if(!strcmp(ent2->d_name,".") || !strcmp(ent2->d_name,"..") || ent2->d_type==DT_DIR) continue;
                    kbds[numkbd].name = myrealloc(kbds[numkbd].name, (kbds[numkbd].num+1) * sizeof(char *));
                    kbds[numkbd].name[kbds[numkbd].num] = myrealloc(NULL, strlen(ent2->d_name) + 1);
                    strcpy(kbds[numkbd].name[kbds[numkbd].num], ent2->d_name);
                    kbds[numkbd].num++;
                }
                closedir(dir2);
            }
            numkbd++;
        }
        closedir(dir);
    }
}

void readconfig()
{
    char *data, *s, *e, *n;
    int i, j, k;
    data = readfileall("../Config");
    if(data) {
        for(s = data; *s; s++) {
            if(*s == '#') while(*s && *s!='\n') s++;
            if(!memcmp(s, "ARCH", 4)) {
                s += 4; while(*s == ' ' || *s == '=') s++;
                for(e = s; *e && *e != '\n'; e++);
                memcpy(arch, s, e - s);
            } else
            if(!memcmp(s, "PLATFORM", 8)) {
                s += 8; while(*s == ' ' || *s == '=') s++;
                for(e = s; *e && *e != '\n'; e++);
                for(i = 0; i < numplatform; i++)
                    if(!strcmp(arch, platform[i].arch) && strlen(platform[i].name) == e - s &&
                        !memcmp(s, platform[i].name, e - s)) { selplat = i; break; }
            } else
            if(!memcmp(s, "DEBUG", 5)) { s += 5; while(*s == ' ' || *s == '=') { s++; } if(*s == '1') compopts[0].val = 1; } else
            if(!memcmp(s, "OPTIMIZE", 8)) { s += 8; while(*s == ' ' || *s == '=') { s++; } if(*s == '1') compopts[2].val = 1; } else
            if(!memcmp(s, "NOINITRD", 8)) { s += 8; while(*s == ' ' || *s == '=') { s++; } if(*s == '1') noinitrd = 1; } else
            if(!memcmp(s, "ESPSIZE", 7)) { s += 7; while(*s == ' ' || *s == '=') { s++; } espsize = atoi(s); } else
            if(!memcmp(s, "USRSIZE", 7)) { s += 7; while(*s == ' ' || *s == '=') { s++; } usrsize = atoi(s); } else
            if(!memcmp(s, "VARSIZE", 7)) { s += 7; while(*s == ' ' || *s == '=') { s++; } varsize = atoi(s); } else
            if(!memcmp(s, "HOMESIZE", 8)) { s += 8; while(*s == ' ' || *s == '=') { s++; } homesize = atoi(s); } else
            if(!memcmp(s, "CFLAGS", 6)) { s += 6; while(*s == ' ' || *s == '=') { s++; } if(s[0]=='-'&&s[1]=='g') compopts[1].val=1; } else
            if(!memcmp(s, "CC", 2)) {
                for(s += 2,cmplr = -1; cmplr == -1 && *s && *s != '\n'; s++) {
                    for(i = 0; i < sizeof(compilers) / sizeof(compilers[0]); i++)
                        if(!memcmp(s, compilers[i].name, strlen(compilers[i].name))) { cmplr = i; break; }
                }
                if(cmplr == -1) cmplr = 0;
            }
        }
    }
    data = readfileall("../etc/config");
    if(data) {
        for(s = data; s < data + fsiz && *s; s++) {
            while(s < data + fsiz && *s && (*s == '\n' || *s == ' ')) s++;
            if(!*s) break;
            if(*s == '#' || (s[0] == '/' && s[1] == '/')) { while(*s && *s!='\n') s++; continue; }
            if(s[0] == '/' && s[1] == '*') { s += 2; while(*s && s[-1]!='*' && s[0]!='/') { s++; } continue; }
            for(n = s; n < data + fsiz && *n && *n != '\n'; n++);
            if(!memcmp(s, "screen=", 7)) {
                s += 7;
                for(i = 0; i < sizeof(screens) / sizeof(screens[0]); i++)
                    if(!memcmp(s, screens[i], strlen(screens[i]))) { scr = i; break; }
            } else
            if(!memcmp(s, "kernel=", 7)) {
                s += 7; for(e = s; *e && *e != '\n'; e++);
                memcpy(kernel, s, e - s);
            } else
            if(!memcmp(s, "tz=", 3)) { s += 3; if(*s == 'a') tz = -99998; else tz = atoi(s); } else
            if(!memcmp(s, "display=", 8)) {
                s += 8;
                for(i = 0; i < sizeof(visopts) / sizeof(visopts[0]); i++)
                    if(!memcmp(s, visopts[i].name, 2)) { vis = i; break; }
                s += 2; if(*s == ',') { for(e = s; *e && *e != '\n'; e++); memcpy(visdrv, s, e - s); }
            } else
            if(!memcmp(s, "lang=", 5)) {
                s += 5;
                for(i = 0; i < numlangs; i++)
                    if(!memcmp(s, langs[i], 2)) { sellang = i; break; }
            } else
            if(!memcmp(s, "keyboard=", 9)) {
                s += 9;
                for(i = 0; i < numkbd; i++)
                    if(!memcmp(s, kbds[i].layout, strlen(kbds[i].layout))) { selkbd = i; break; }
                while(*s && *s != '\n' && *s != ',') s++;
                if(*s == ',') {
                    s++;
                    for(i = 0; i < kbds[selkbd].num; i++)
                        if(!memcmp(s, kbds[selkbd].name[i], strlen(kbds[selkbd].name[i]))) { sellyt = selalt = i; break; }
                    while(*s && *s != '\n' && *s != ',') s++;
                    if(*s == ',') {
                        s++;
                        for(i = 0; i < kbds[selkbd].num; i++)
                            if(!memcmp(s, kbds[selkbd].name[i], strlen(kbds[selkbd].name[i]))) { selalt = i; break; }
                    }
                }
            } else
            if(!memcmp(s, "debug=", 6)) {
                s += 6;
                do {
                    if(*s == ',') s++;
                    for(i = 0; i < sizeof(dbgopts) / sizeof(dbgopts[0]); i++)
                        if(!memcmp(s, dbgopts[i].name, 2)) { dbgopts[i].val = 1; break; }
                    while(*s && *s != '\n' && *s != ',') s++;
                } while(*s == ',');
            } else {
                for(i = k = 0; i < sizeof(dspopts) / sizeof(dspopts[0]); i++) {
                    j = strlen(dspopts[i].name);
                    if(s[j] == '=' && !memcmp(s, dspopts[i].name, j)) {
                        s += j + 1; k = 1;
                        dspopts[i].val = (*s == 't' || *s == '1' || *s == 'e');
                        break;
                    }
                }
                for(i = 0; i < sizeof(srvopts) / sizeof(srvopts[0]); i++) {
                    j = strlen(srvopts[i].name);
                    if(s[j] == '=' && !memcmp(s, srvopts[i].name, j)) {
                        s += j + 1; k = 1;
                        srvopts[i].val = (*s == 't' || *s == '1' || *s == 'e');
                        break;
                    }
                }
                if(!k) {
                    extra = (char**)myrealloc(extra, (numextra+1)*sizeof(char*));
                    extra[numextra] = myrealloc(NULL, n - s + 1);
                    memcpy(extra[numextra], s, n - s);
                    extra[numextra][n - s] = 0;
                    numextra++;
                }
            }
            s = n;
        }
    }
}

void writeconfig()
{
    FILE *f = stdout;
    char s[32];
    int i, j = 0;
    sprintf(s,"%d",tz < -9999 ? 0 : tz);
    /* fordítási opciók lementése */
    f = fopen("../Config","w");
    if(f) {
        fprintf(f,
            "# OS/Z - Copyright (c) 2020 bzt (bztsrc@gitlab), CC-by-nc-sa\n"
            "# Use is subject to license terms.\n"
            "# generated by \"make config\"\n"
            "\n"
            "# --- common configuration ---\n"
            "ARCH = %s\n"
            "PLATFORM = %s\n"
            "DEBUG = %d\n"
            "OPTIMIZE = %d\n"
            "\n"
            "# --- disk layout ---\n"
            "NOINITRD = %d\n"
            "ESPSIZE = %d\n"
            "USRSIZE = %d\n"
            "VARSIZE = %d\n"
            "HOMESIZE = %d\n\n"
            "# --- build system ---\n"
            "CFLAGS = %s-O2 -fno-plt -fvisibility=hidden -ansi -pedantic -Wall -Wextra\n"
            "include $(dir $(lastword $(MAKEFILE_LIST)))src/core/$(ARCH)/$(PLATFORM)/Makefile.opt\n"
            "%s\n",
            platform[selplat].arch,platform[selplat].name,compopts[0].val,compopts[2].val,
            noinitrd,espsize,usrsize,varsize,homesize,
            compopts[1].val?"-g ":"",compilers[cmplr].setup);
        fclose(f);
    }
    /* futás idejű opciók lementése */
    f = fopen("../etc/config","w");
    if(f) {
        fprintf(f,
            "// BOOTBOOT Options\n"
            "// Use is subject to license terms.\n"
            "// generated by \"make config\"\n"
            "\n"
            "// --- loader options ---\n"
            "screen=%s\n"
            "kernel=%s\n"
            "\n"
            "// --- core options ---\n"
            "%stz=%s\n",
            screens[scr],kernel[0]?kernel:"sys/core",tz==-99999?"// ":"",tz==-99998?"ask":s);
        if(numextra) for(i = 0; i < numextra; i++) fprintf(f, "%s\n", extra[i]);
        fprintf(f,
            "\n"
            "// --- UI options ---\n"
            "lang=%c%c\n"
            "keyboard=%s,%s%s%s\n"
            "display=%s%s\n",
            langs[sellang][0],langs[sellang][1],
            kbds[selkbd].layout,kbds[selkbd].name[sellyt],sellyt!=selalt?",":"",sellyt!=selalt?kbds[selkbd].name[selalt]:"",
            visopts[vis].name,visdrv);
        for(i = 0; i < sizeof(dspopts) / sizeof(dspopts[0]); i++)
            fprintf(f, "%s=%s\n",dspopts[i].name,dspopts[i].val?"true":"false");
        fprintf(f,"\n// --- system services ---\n");
        for(i = 0; i < sizeof(srvopts) / sizeof(srvopts[0]); i++)
            fprintf(f, "%s=%s\n",srvopts[i].name,srvopts[i].val?"true":"false");
        for(i = j = 0; i < sizeof(dbgopts) / sizeof(dbgopts[0]); i++) j += dbgopts[i].val;
        if(j) {
            fprintf(f,"\n// --- debug options ---\ndebug=");
            for(i = j = 0; i < sizeof(dbgopts) / sizeof(dbgopts[0]); i++)
                fprintf(f, "%s%s",dbgopts[i].val?(!j++?"":","):"",dbgopts[i].val?dbgopts[i].name:"");
            fprintf(f,"\n");
        }
        fprintf(f,"\n");
        fclose(f);
    }
}

enum { PRE, SUF, VER, HOR, NW, NE, SW, SE };
char **t, *terms[3][8] = {
    { "", "", "\xe2\x94\x80", "\xe2\x94\x82", "\xe2\x94\x8c", "\xe2\x94\x90", "\xe2\x94\x94", "\xe2\x94\x98" },
    { "\033(0", "\033(B", "q", "x", "l", "k", "m", "j" },
    { "", "", "-", "|", "+", "+", "+", "+" }
};
int menu = 0, mainitem = 0, mainbtn = 0;

char ctrlchklist(char c, list_t *list)
{
    switch(c) {
        case 1: if(chk > 0) chk--; else chk = chkl-1; break;
        case 2: if(chk < chkl-1) chk++; else chk = 0; break;
        case 10: menu = 0; col = 0; mainbtn = 0; c = 0; break;
        case ' ': list[chk].val ^= 1; break;
    }
    return c;
}

char ctrlselect(char c, int *curr, int max)
{
    switch(c) {
        case 1: if(*curr > 0) (*curr)--; else *curr = max-1; break;
        case 2: if(*curr < max-1) (*curr)++; else *curr = 0; break;
        case 10: if(menu==12) { menu=13; selalt=sellyt; } else { menu = 0; col = 0; mainbtn = 0; } c = 0; break;
    }
    return c;
}

void bg() {
    int i;
    printf("\033[36;44;1m\033[H\033[J %s\n %s", lang[4], t[PRE]);
    for(i = 0; i < col-2; i++) printf(t[VER]);
    printf("%s\033[%d;1H\033[0m\033[%d;44m[1] UTF-8, [2] VT100, [3] VT52",t[SUF],row, t == terms[2]?36:94);
}

void drawline(int x, int y, int w, int i, int a, char *sel, char *ina)
{
    int j;
    printf("\033[%s;%dm\033[%d;%dH%s%s%s \033[%sm",a?"0;30":ina,a?47:(t==terms[2]?46:100),y,x,t[PRE],t[HOR],t[SUF],i?sel:"30;1");
    for(j = 0; j < w - 4; j++) printf(" ");
    printf("\033[%s;%dm %s%s%s\033[40m  \033[%d;%dH\033[%s;%dm",a?"37;1":ina,a?47:(t==terms[2]?46:100),t[PRE],t[HOR],t[SUF],y,x+3,
        i?sel:(a?"0;30":"0;34"),i?1:(a?47:(t==terms[2]?46:100)));
}

void drawboxtop(int x, int y, int w, int i, int a,char *ina)
{
    int j;
    printf("\033[%d;%dH\033[%s;%dm%s%s",y,x,a?"0;30":ina,a?47:(t==terms[2]?46:100),t[PRE],t[NW]);
    for(j = 0; j < w - 2; j++) printf(t[VER]);
    printf("\033[%sm%s%s%s\033[%s;%dm\033[%d;%dH",a?"37;1":ina,t[NE],t[SUF], i?"\033[40m  ":"", a?"1;30":"1;34",
        a?47:(t==terms[2]?46:100), y, x + 3);
}

void drawboxbtm(int x, int y, int w, int a, char *ina)
{
    int j;
    printf("\033[%d;%dH\033[%s;%dm%s%s\033[%sm",y,x,a?"0;30":ina,a?47:(t==terms[2]?46:100),t[PRE],t[SW],a?"37;1":ina);
    for(j = 0; j < w - 2; j++) printf(t[VER]);
    printf("%s%s\033[40m  \033[0m",t[SE],t[SUF]);
}

void drawshd(int x, int y, int w) {
    int i;
    printf("\033[40m\033[%d;%dH",y,x+2);
    for(i = 0; i < w; i++) printf(" ");
    printf("\033[0m");
}

void drawchklist(list_t *list, int len, int title)
{
    int x, y, w = 0, m = 0, h, i, j;
    for(i = 0; i < len; i++) {
        j = mystrlen(list[i].name) + 1;
        if(j > m) m = j;
        j = mystrlen(lang[list[i].langcode]) + 1;
        if(j > w) w = j;
    }
    w += m + 9; if(w < mystrlen(lang[title])+8) w = mystrlen(lang[title])+8;
    chkh = (len > row - 6 ? row - 6: len); h = chkh + 2; chkl = len;
    x = (col - w) / 2; if(x < 1) x = 1;
    y = (row - h) / 2 + 1; if(y < 1) y = 1;
    if(chk < chkscr) chkscr = chk;
    if(chk > chkscr + chkh - 1) chkscr = chk - chkh + 1;
    if(chkscr < 0) chkscr = 0;
    drawboxtop(x,y,w,0,1,"0;37"); printf(" %s ", lang[title]);
    for(i = 0; i < chkh; i++) {
        drawline(x,y+1+i,w,chkscr+i==chk,1,"37;44;1","0;37");
        printf("[%c] %s%s%s\033[%d;%dH%s",list[chkscr+i].val?'*':' ',chkscr+i!=chk?"\033[34m":"",
            list[chkscr+i].name,chkscr+i!=chk?"\033[30m":"",y+1+i,x+7+m,lang[list[chkscr+i].langcode]);
    }
    drawboxbtm(x,y+h-1,w,1,"0;37");
    drawshd(x,y+h,w);
}

void drawselect(list_t *list, int len, int title, int curr)
{
    int x, y, w = 0, m = 0, h, i, j;
    for(i = 0; i < len; i++) {
        j = mystrlen(list[i].name) + 1;
        if(j > m) m = j;
        j = mystrlen(lang[list[i].langcode]) + 1;
        if(j > w) w = j;
    }
    w += m + 5; if(w < mystrlen(lang[title])+8) w = mystrlen(lang[title])+8;
    chkh = (len > row - 6 ? row - 6: len); h = chkh + 2; chkl = len;
    x = (col - w) / 2; if(x < 1) x = 1;
    y = (row - h) / 2 + 1; if(y < 1) y = 1;
    drawboxtop(x,y,w,0,1,"0;37"); printf(" %s ", lang[title]);
    if(curr < chkscr) chkscr = curr;
    if(curr > chkscr + chkh - 1) chkscr = curr - chkh + 1;
    if(chkscr < 0) chkscr = 0;
    for(i = 0; i < chkh; i++) {
        drawline(x,y+1+i,w,chkscr+i==curr,1,"37;44;1","0;37");
        printf("%s%s%s\033[%d;%dH%s",chkscr+i!=curr?"\033[34m":"",
            list[chkscr+i].name,chkscr+i!=curr?"\033[30m":"",y+1+i,x+3+m,lang[list[chkscr+i].langcode]);
    }
    drawboxbtm(x,y+h-1,w,1,"0;37");
    drawshd(x,y+h,w);

}

void redraw()
{
    int r = 0, c = 0, x, y, i, j, w = 42, h = 18, es,us,vs,hs;
    char s[32],sel[16],ina[16],iab[16],initrdstr[]="  ####";

    getstdindim(&r, &c);
    if(r != row || c != col) { row = r; col = c; bg(); }
    x = (col - w) / 2; if(x < 1) x = 1;
    y = (row - h) / 2; if(y < 1) y = 1;

    strcpy(sel,!menu?"37;44;1":"34;40;1");
    strcpy(ina,!menu?"0;37":"34;1");
    strcpy(iab,!menu?"30":"34");
    drawboxtop(x,y,w,0,!menu,ina); printf(" %s ", lang[5]);
    drawline(x,y+ 1,w,mainitem== 0,!menu,sel,ina); printf("%s (%s/%s)",lang[7],platform[selplat].arch,platform[selplat].name);
    drawline(x,y+ 2,w,mainitem== 1,!menu,sel,ina); printf("%s (%s)",lang[8],compilers[cmplr].name);
    drawline(x,y+ 3,w,mainitem== 2,!menu,sel,ina); printf("%s",lang[9]);
    drawline(x,y+ 4,w,mainitem== 3,!menu,sel,ina); printf("%s",lang[10]);
    drawboxbtm(x,y+5,w,!menu,ina);

    drawboxtop(x,y+6,w,1,!menu,ina); printf(" %s ", lang[6]);
    drawline(x,y+ 7,w,mainitem== 4,!menu,sel,ina); printf("%s",lang[11]);
    sprintf(s, "GMT%s%d %s",tz>0?"+":"",tz,lang[51]);
    drawline(x,y+ 8,w,mainitem== 5,!menu,sel,ina); printf("%s (%s)",lang[12],tz==-99999?"detect":(tz==-99998?"ask":(!tz?"UTC":s)));
    drawline(x,y+ 9,w,mainitem== 6,!menu,sel,ina); printf("%s (%s)",lang[13],screens[scr]);
    drawline(x,y+10,w,mainitem== 7,!menu,sel,ina); printf("%s (%s)",lang[14],visopts[vis].name);
    drawline(x,y+11,w,mainitem== 8,!menu,sel,ina); printf("%s",lang[15]);
    drawline(x,y+12,w,mainitem== 9,!menu,sel,ina); printf("%s (%s)",lang[16],langs[sellang]);
    drawline(x,y+13,w,mainitem==10,!menu,sel,ina); printf("%s (%s)",lang[17],kbds[selkbd].layout);
    drawline(x,y+14,w,mainitem==11,!menu,sel,ina); printf("%s (%s%s%s)",lang[18],kbds[selkbd].name?kbds[selkbd].name[sellyt]:"",
        sellyt!=selalt?", ":"",sellyt!=selalt && kbds[selkbd].name?kbds[selkbd].name[selalt]:"");
    drawline(x,y+15,w,mainitem==12,!menu,sel,ina); printf("%s",lang[19]);
    drawboxbtm(x,y+16,w,!menu,ina);

    printf("\033[%d;%dH\033[%s;%dm%s%s",y+17,x,!menu?"37;1":ina,!menu?47:(t==terms[2]?46:100),t[PRE],t[NW]);
    for(i = 0; i < w - 2; i++) printf(t[VER]);
    printf("\033[%s;%dm%s%s\033[40m  ",!menu?"0;30":ina,!menu?47:(t==terms[2]?46:100),t[NE],t[SUF]);

    printf("\033[%d;%dH\033[%s;%dm%s%s\033[30m",y+18,x,!menu?"37;1":ina,!menu?47:(t==terms[2]?46:100),t[PRE],t[HOR]);
    for(i = 0; i < w - 2; i++) printf(" ");
    printf("\033[%s;%dm%s%s\033[40m  ",!menu?"0;30":ina,!menu?47:(t==terms[2]?46:100),t[HOR],t[SUF]);
    printf("\033[%d;%dH\033[0;%d;%sm< \033[%sm%s\033[%sm >",y+18,x+4,!menu?47:(t==terms[2]?46:100),!menu && mainbtn==0?sel:iab,
        !menu && mainbtn==0?"33;44;1":iab, lang[20], !menu && mainbtn==0?sel:iab);
    printf("\033[%d;%dH\033[0;%d;%sm< \033[%sm%s\033[%sm >",y+18,x+(w/2-(mystrlen(lang[21])+4)/2),!menu?47:(t==terms[2]?46:100),
        !menu && mainbtn==1?sel:iab, !menu && mainbtn==1?"33;44;1":iab, lang[21], !menu && mainbtn==1?sel:iab);
    printf("\033[%d;%dH\033[0;%d;%sm< \033[%sm%s\033[%sm >",y+18,x+w-6-mystrlen(lang[22]),!menu?47:(t==terms[2]?46:100),
        !menu && mainbtn==2?sel:iab,!menu && mainbtn==2?"33;44;1":iab, lang[22], !menu && mainbtn==2?sel:iab);

    printf("\033[%d;%dH\033[%s;%dm%s%s\033[%s;%dm",y+19,x,!menu?"37;1":ina,!menu?47:(t==terms[2]?46:100),t[PRE],t[SW],
        !menu?"0;30":ina,!menu?47:(t==terms[2]?46:100));
    for(i = 0; i < w - 2; i++) printf(t[VER]);
    printf("%s%s\033[40m  ",t[SE],t[SUF]);
    drawshd(x,y+20,w);

    switch(menu) {
        case 1:
            w = 24 + platw; if(w < mystrlen(lang[23])+8) w = mystrlen(lang[23])+8; h = numplatform + 2;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[23]);
            for(i = 0; i < numplatform; i++) {
                drawline(x,y+1+i,w,selplat==i,1,"37;44;1",ina);
                printf("%s/%s\033[%d;%dH%s",platform[i].arch,platform[i].name,y+1+i,x+18,platform[i].desc);
            }
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 2:
            w = 8 + 8 + 16; if(w < mystrlen(lang[24])+8) w = mystrlen(lang[24])+8; h = (sizeof(compilers)/sizeof(compilers[0])) + 2;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[24]);
            for(i = 0; i < sizeof(compilers)/sizeof(compilers[0]); i++) {
                drawline(x,y+1+i,w,cmplr==i,1,"37;44;1",ina);
                printf("%s\033[%d;%dH%s",compilers[i].name,y+1+i,x+12,compilers[i].desc);
            }
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 3: drawchklist(compopts, sizeof(compopts)/sizeof(compopts[0]), 25); break;
        case 4:
            x = 3; w = col - 4; h = 10;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[10]);
            drawline(x,y+1,w,0,1,"37;44;1",ina);
            drawline(x,y+2,w,0,1,"37;44;1",ina);
            printf("\033[%d;%dH",y+2,x+2);
            j = espsize + usrsize + varsize + homesize;
            es = espsize * (col - 17) / j;
            us = usrsize * (col - 17) / j; if(usrsize && !us) { us = 1; es--; }
            vs = varsize * (col - 17) / j; if(varsize && !vs) { vs = 1; es--; }
            hs = homesize * (col - 17) / j; if(homesize && !hs) { hs = 1; es--; }
            if(es + us + vs + hs != col - 17) es = col - 17 - us - vs - hs;
            printf("\033[1;33;%dm",noinitrd?43:46); for(i = 0; i < es; i++) printf("%c",noinitrd||i>=sizeof(initrdstr)-1?' ':initrdstr[i]);
            printf("\033[1;31;41m"); for(i = 0; i < us; i++) printf(" ");
            printf("\033[0;32;42m"); for(i = 0; i < vs; i++) printf(" ");
            printf("\033[0;35;45m"); for(i = 0; i < hs; i++) printf(" ");
            printf("\033[0;30;47m\033[%d;%dH%4d.%d Mb",y+2,col-12,j/1024,(j%1024)/100);
            drawline(x,y+3,w,0,1,"37;44;1",ina);
            drawline(x,y+4,w,chk==0,1,"37;44;1",ina); printf("               %s (%s)",lang[34],noinitrd?lang[35]:"ESP:/BOOTBOOT");
            drawline(x,y+5,w,chk==1,1,"37;44;1",ina); printf("-<%6d >+ Kb %s %s",espsize,noinitrd?"Initrd":"/boot",lang[35]);
            drawline(x,y+6,w,chk==2,1,"37;44;1",ina); printf("-<%6d >+ Kb /usr %s",usrsize,lang[36]);
            drawline(x,y+7,w,chk==3,1,"37;44;1",ina); printf("-<%6d >+ Kb /var %s",varsize,lang[37]);
            drawline(x,y+8,w,chk==4,1,"37;44;1",ina); printf("-<%6d >+ Kb /home %s",homesize,lang[37]);
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 5: drawchklist(srvopts, sizeof(srvopts)/sizeof(srvopts[0]), 42); break;
        case 6:
            w = 60; h = 6;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[52]);
            drawline(x,y+1,w,chk==0,1,"37;44;1",ina); printf("  detect   %s",lang[41]);
            drawline(x,y+2,w,chk==1,1,"37;44;1",ina); printf("  ask      %s",lang[53]);
            drawline(x,y+3,w,chk==2,1,"37;44;1",ina); printf("  UTC      %s",lang[54]);
            drawline(x,y+4,w,chk==3,1,"37;44;1",ina); printf("-<%5d >+ %s",tz >= -1440 && tz <= 1440 ? tz : 0,lang[55]);
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 7:
            w = 8 + 16; if(w < mystrlen(lang[13])+8) w = mystrlen(lang[13])+8; h = (sizeof(screens)/sizeof(screens[0])) + 2;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[13]);
            for(i = 0; i < sizeof(screens)/sizeof(screens[0]); i++) {
                drawline(x,y+1+i,w,scr==i,1,"37;44;1",ina);
                printf("%s",screens[i]);
            }
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 8: drawselect(visopts, sizeof(visopts)/sizeof(visopts[0]), 14, vis); break;
        case 9: drawchklist(dspopts, sizeof(dspopts)/sizeof(dspopts[0]), 15); break;
        case 10:
            w = 8 + 16; if(w < mystrlen(lang[66])+8) w = mystrlen(lang[66])+8; h = numlangs + 2;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[66]);
            for(i = 0; i < numlangs; i++) {
                drawline(x,y+1+i,w,sellang==i,1,"37;44;1",ina);
                printf("%s%c%c%s %s",sellang!=i?"\033[34m":"",
                    langs[i][0],langs[i][1],sellang!=i?"\033[30m":"",langs[i]+3);
            }
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 11:
            w = 8 + 16; if(w < mystrlen(lang[67])+8) w = mystrlen(lang[67])+8; h = numkbd + 2;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[67]);
            for(i = 0; i < numkbd; i++) {
                drawline(x,y+1+i,w,selkbd==i,1,"37;44;1",ina);
                printf("%s",kbds[i].layout);
            }
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 12:
            w = 8 + 16; if(w < mystrlen(lang[68])+8) w = mystrlen(lang[68])+8; h = kbds[selkbd].num + 2;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[68]);
            for(i = 0; i < kbds[selkbd].num; i++) {
                drawline(x,y+1+i,w,sellyt==i,1,"37;44;1",ina);
                printf("%s",kbds[selkbd].name[i]);
            }
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 13:
            w = 8 + 16; if(w < mystrlen(lang[69])+8) w = mystrlen(lang[69])+8; h = kbds[selkbd].num + 2;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina); printf(" %s ", lang[69]);
            for(i = 0; i < kbds[selkbd].num; i++) {
                drawline(x,y+1+i,w,selalt==i,1,"37;44;1",ina);
                printf("%s",sellyt==i?lang[70]:kbds[selkbd].name[i]);
            }
            drawboxbtm(x,y+h-1,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 14: /* harmadlagos kiosztás */ break;
        case 15: /* negyedleges */ break;
        case 16:
            w = mystrlen(lang[40])+8; h = 3;
            x = (col - w) / 2; if(x < 1) x = 1;
            y = (row - h) / 2 + 1; if(y < 1) y = 1;
            drawboxtop(x,y,w,0,1,ina);
            drawline(x,y+1,w,0,1,"37;44;1",ina); printf(" %s ", lang[40]);
            drawboxbtm(x,y+2,w,1,ina);
            drawshd(x,y+h,w);
        break;
        case 17: drawchklist(dbgopts, sizeof(dbgopts)/sizeof(dbgopts[0]), 71); break;
    }
    printf("\033[0m\033[%d;%dH\033[?25l",row,col);
}

int main(int argc, char **argv)
{
    char c, *l = getenv("LANG");

    if(l && l[0] == 'h' && l[1] == 'u') lang = lang_hu; else lang  = lang_en;
    readopts();
    readconfig();
    setupstdin();
    if(argv[1] && argv[1][0] == '-' && argv[1][1] >= '1' && argv[1][1] <= '3')
        t = terms[argv[1][1]-'1'];
    else
        t = terms[0];

    do {
        redraw();
        c = getcsi(); if(c >= '1' && c <= '3') { t = terms[c-'1']; col = 0; continue; }
        if(!menu) {
            switch(c) {
                case 1: if(mainitem > 0) mainitem--; else mainitem = 12; break;
                case 2: if(mainitem < 12) mainitem++; else mainitem = 0; break;
                case 3: if(mainbtn < 2) mainbtn++; else mainbtn = 0; break;
                case 4: if(mainbtn > 0) mainbtn--; else mainbtn = 2; break;
                case 10:
                    if(mainbtn) { c = 0x1b; if(mainbtn == 1) confsave = 1; }
                    else {
                        menu = mainitem + (mainitem > 11 ? 5 : 1); col = chk = chkscr = 0;
                        if(menu == 6) {
                            if(tz == -99999) chk = 0; else
                            if(tz == -99998) chk = 1; else
                            if(tz == 0) chk = 2; else
                            chk = 3;
                        }
                    }
                break;
            }
        } else
        if(c == 0x1b) {
            menu = 0; col = 0; mainbtn = 0; c = 0;
        } else
        switch(menu) {
            case 1:
                c = ctrlselect(c, &selplat, numplatform);
                if(noinitrd == 1 && !memcmp(platform[selplat].name, "rpi", 3)) noinitrd = 0;
            break;
            case 2: c = ctrlselect(c, &cmplr, sizeof(compilers)/sizeof(compilers[0])); break;
            case 3: c = ctrlchklist(c, compopts); break;
            case 4:
                switch(c) {
                    case 1: if(chk > 0) chk--; else chk = 4; break;
                    case 2: if(chk < 4) chk++; else chk = 0; break;
                    case 3:
                        switch(chk) {
                            case 0: noinitrd ^= 1; break;
                            case 1: espsize &= ~255; espsize += 256; break;
                            case 2: usrsize &= ~255; usrsize += 256; break;
                            case 3: varsize &= ~255; varsize += 256; break;
                            case 4: homesize &= ~255; homesize += 256; break;
                        }
                    break;
                    case 4:
                        switch(chk) {
                            case 0: noinitrd ^= 1; break;
                            case 1: espsize -= 256; if(espsize < 8256) espsize = 8256; break;
                            case 2: usrsize -= 256; if(usrsize < 256) usrsize = 256; break;
                            case 3: varsize -= 256; if(varsize < 0) varsize = 0; break;
                            case 4: homesize -= 256; if(homesize < 0) homesize = 0; break;
                        }
                    break;
                    case ' ': noinitrd ^= 1; break;
                    case 10: menu = 0; col = 0; mainbtn = 0; c = 0; break;
                }
                if(noinitrd == 1 && !memcmp(platform[selplat].name, "rpi", 3)) { noinitrd = 0; col = 0; menu = 16; }
            break;
            case 5: c = ctrlchklist(c, srvopts); break;
            case 6:
                switch(c) {
                    case 1: if(chk > 0) chk--; else chk = 3; break;
                    case 2: if(chk < 3) chk++; else chk = 0; break;
                    case 3: if(chk == 3) { if(tz < -9999) tz = 0; if(tz < 1440) tz += 5; else tz = -1440; } break;
                    case 4: if(chk == 3) { if(tz < -9999) tz = 0; if(tz > -1440) tz -= 5; else tz = 1440; } break;
                    case 10:
                        if(chk==0) tz = -99999;
                        if(chk==1) tz = -99998;
                        if(chk==2) tz = 0;
                        menu = 0; col = 0; mainbtn = 0; c = 0;
                    break;
                }
            break;
            case 7: c = ctrlselect(c, &scr, sizeof(screens)/sizeof(screens[0])); break;
            case 8: c = ctrlselect(c, &vis, sizeof(visopts)/sizeof(visopts[0])); break;
            case 9: c = ctrlchklist(c, dspopts); break;
            case 10: c = ctrlselect(c, &sellang, numlangs); break;
            case 11: c = ctrlselect(c, &selkbd, numkbd); break;
            case 12: c = ctrlselect(c, &sellyt, kbds[selkbd].num); break;
            case 13: c = ctrlselect(c, &selalt, kbds[selkbd].num); break;
            case 16: menu = 4; c = 0; break;
            case 17: c = ctrlchklist(c, dbgopts); break;
        }
    } while(c!=0x1b);
    ctrlc(SIGINT);
}
