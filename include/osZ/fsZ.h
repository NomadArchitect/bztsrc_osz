/*
 * include/osZ/fsZ.h
 *
 * Copyright (C) 2017 bzt (bztsrc@gitlab)
 * https://opensource.org/licenses/MIT
 *
 * A szabad felhasználás jogát ezennel ráruházom bármely személyre
 * aki jelen mű és dokumentációjának egy példányát (a továbbiakban
 * “Szoftver”) megszerezte, hogy a szoftvert korlátozás nélkül
 * - beleértve a használat, másolás, módosítás, kiadás, terjesztés,
 * és/vagy értékesítés jogát - felhasználhassa, és a továbbadott
 * példánnyal hasonlóan járhasson el, amennyiben a következő feltételek
 * teljesülnek:
 *
 * A fenti szerzői jogot és felhasználási jogot biztosító szöveget
 * fel kell tüntetni a Szoftver minden teljes és rész másolataiban is.
 *
 * A SZOFTVER “ÚGY, AHOGY VAN” KERÜL TERJESZTÉSE, MINDENNEMŰ JÓTÁLLÁS
 * NÉLKÜL, KÖZVETVE VAGY KÖZVETETTEN, BELEÉRTVE A KERESKEDELMI JÓTÁLLÁS
 * VAGY EGY ADOTT CÉLRA TÖRTÉNŐ FELHASZNÁLÁSRA ALKALMASSÁGOT. A SZERZŐI
 * JOG TULAJDONOSA SEMMILYEN KÖRÜLMÉNYEK KÖZÖTT NEM TEHETŐ FELELŐSSÉ
 * BÁRMILYEN MÓDON, BELEÉRVE A SZOFTVER RENDELTETÉSSZERŰ HASZNÁLATA
 * SORÁN, VAGY RENDELTETÉSSZERŰ HASZNÁLATTÓL ELTÉRŐ FELHASZNÁLÁS
 * SORÁN A SZOFTVER ÁLTAL ESETLEGESEN OKOZOTT KÁROKÉRT.
 *
 * FONTOS: az FS/Z lemezformátum MIT licenszű, használhatod megszorítás
 * nélkül, ingyen és bérmentve. A jogot, hogy bárki FS/Z formázott lemezeket
 * és lemezképeket hozzon létre illetve hogy ilyen programokat írjon
 * ezennel megadom. Az OS/Z-beli FS/Z ellenben CC-by-nc-sa licenszű.
 *
 * @brief FS/Z fájlrendszer lemezformátumának definíciói és struktúrái
 */

#ifndef _FS_Z_H_
#define _FS_Z_H_    1

#define FSZ_VERSION_MAJOR 1
#define FSZ_VERSION_MINOR 0
#define FSZ_SECSIZE 4096

/* a logikai szektorcím 128 bites, hogy biztosan jövőálló legyen, és a szuperblokkhoz
 * képest számolódik. A jelenlegi implementációk ebből csak 64 bitet kezelnek, ami
 * 64 Zettabájtos lemezméretet jelent az alapértelmzett 4096 bájtos szektorméret mellett. */

/* a CRC32-höz az ANSI metódust, a 0x04c11db7 polinómú CCITT32 CRC-t használja
 * (ugyanazt, mint az EFI GPT vagy a gzip ellenőrzőösszege) */

#ifndef packed
#define packed __attribute__((packed))
#endif

/* sizeof = 16, egy Access Control Entry, UUID a legutolsó bájt nélkül, az ACL lista egy eleme */
typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[7];
    uint8_t  access;
} packed FSZ_Access;

/* a hozzáférési bitek a legutolsó bájtban vannak tárolva. Ennek egyeznie
 * kell az include/osZ/types.h-ban definiált A_* jelzőkkel */
#define FSZ_READ    (1<<0)
#define FSZ_WRITE   (1<<1)
#define FSZ_EXEC    (1<<2)          /* futtatás vagy keresés */
#define FSZ_APPEND  (1<<3)
#define FSZ_DELETE  (1<<4)
#define FSZ_SUID    (1<<6)          /* felhasználóazonosító öröklése futtatáskor */
#define FSZ_SGID    (1<<7)          /* ACL öröklése, setgid mint olyan nincs az OS/Z-ben */

/*********************************************************
 *            első szektor, a szuperblokk                *
 *********************************************************/
typedef struct {
    uint8_t     loader[512];        /*   0 betöltőprogram számára fenntartva */
    uint8_t     magic[4];           /* 512 */
    uint8_t     version_major;      /* 516 */
    uint8_t     version_minor;      /* 517 */
    uint8_t     flags;              /* 518 jelzőbitek */
    uint8_t     raidtype;           /* 519 raid típusa */
    uint16_t    logsec;             /* 520 logikai szektorméret, 0=2048,1=4096(alapért),2=8192... */
    uint16_t    physec;             /* 522 hány fizikai szektor tesz ki egy logikait, alapért 8 */
    uint16_t    maxmounts;          /* 524 a maximum felcsatolások száma a következő kötelező fsck-ig */
    uint16_t    currmounts;         /* 526 aktuális felcsatolásszámláló */
    uint64_t    numsec;             /* 528 összes logikai szektor száma */
    uint64_t    numsec_hi;          /*      128 bit */
    uint64_t    freesec;            /* 544 a legelső szabad szektor ha defragmentált, egyébként */
    uint64_t    freesec_hi;         /*     a legutolsó használt szektor+1 */
    uint64_t    rootdirfid;         /* 560 a gyökér fájlrendszer logikai szektorszáma, tipikusan LSN 1 */
    uint64_t    rootdirfid_hi;
    uint64_t    freesecfid;         /* 576 a szabad rekordok inodeja (FSZ_SectorList allokált) */
    uint64_t    freesecfid_hi;
    uint64_t    badsecfid;          /* 592 hibás szektorok inodeja (FSZ_SectorList allokált) */
    uint64_t    badsecfid_hi;
    uint64_t    indexfid;           /* 608 keresőindex inodeja, nulla ha nincs index */
    uint64_t    indexfid_hi;
    uint64_t    metafid;            /* 624 címkék inodeja, nulla ha nincsenek címkék */
    uint64_t    metafid_hi;
    uint64_t    journalfid;         /* 640 naplófájl inodeja, nulla ha a naplózás ki van kapcsolva */
    uint64_t    journalfid_hi;
    uint64_t    journalhead;        /* 656 a napló kezdő, naplófájlon belüli logikai szektor száma */
    uint64_t    journaltail;        /* 664 a napló utolsó, naplófájlon belüli logikai szektor száma */
    uint64_t    journalmax;         /* 672 a naplófájlban lefoglalható logikai szektorok maximális száma */
    uint8_t     encrypt[28];        /* 680 titkosító maszk vagy nulla, ha nem titkosított */
    uint32_t    enchash;            /* 708 titkosító jelszó CRC32, hogy ne kelljen rossz jelszóval visszakódolni */
    uint64_t    createdate;         /* 712 létrehozás UTC mikroszekundum időbélyege */
    uint64_t    lastmountdate;      /* 720 legutolsó felcsatolás időpontja */
    uint64_t    lastcheckdate;      /* 728 legutolsó fsck időpontja */
    uint64_t    lastchangedate;     /* 736 a szuperblokk legutolsó kiírásának időpontja */
    uint8_t     uuid[16];           /* 744 fájlrendszer egyedi UUID */
    FSZ_Access  owner;              /* 760 tulajdonos UUID */
    uint8_t     reserved[240];      /* 776 hozzáférési listák (ACL) számára fenntartott */
    uint8_t     magic2[4];          /*1016 */
    uint32_t    checksum;           /*1020 az 512-1020 bájt CRC32-je */
    uint8_t     raidspecific[FSZ_SECSIZE-1024];
} packed FSZ_SuperBlock;

#define FSZ_MAGIC "FS/Z"

#define FSZ_SB_FLAG_BIGINODE   (1<<0)  /* azt jelzi, hogy az indoeméret 2048 bájt (ACL hossza 96 nem pedig 32) */
#define FSZ_SB_JOURNAL_DATA    (1<<1)  /* nemcsak a metaadatok, hanem naplózza a fájladatokat is */
#define FSZ_SB_EALG_SHACBC     (0<<2)  /* SHA-XOR-CBC metódussal titkosítva */
#define FSZ_SB_EALG_AESCBC     (1<<2)  /* AES-256-CBC metódussal titkosítva */
/* a 3. és 4. bit fenntartva jövőbeli titkosító metódusok kódolásához */
#define FSZ_SB_EALG(x)         ((x>>2)&7)

#define FSZ_SB_SOFTRAID_NONE   0xff    /* egy lemez */
#define FSZ_SB_SOFTRAID0          0    /* tükrözés */
#define FSZ_SB_SOFTRAID1          1    /* összefűzés */
#define FSZ_SB_SOFTRAID5          5    /* xorolt blokkok */

/*********************************************************
 *                   I-node szektor                      *
 *********************************************************/
/* fid: file id, olyan logikai szektorszám, ami egy inode-ot tartlmazó szektorra mutat */

/* sizeof = 32 */
typedef struct {
    uint64_t    sec;
    uint64_t    sec_hi;
    uint64_t    numsec;
    uint32_t    chksum;
    uint32_t    flags;
} packed FSZ_SectorList;
/* több helyen is használt, szabad és hibás szektorlista, plusz minden FSZ_IN_FLAG_SECLIST* leképezés. */

/* sizeof = 16 */
typedef struct {
    uint64_t    sec;
    uint32_t    sec_hi;
    uint32_t    chksum;
} packed FSZ_SDEntry;
/* a FSZ_IN_FLAG_SD* leképezés használja. */

/* fájlverzió struktúra. Ezzel lehet mutatni a version5, version4 stb. mezőkre. */
/* sizeof = 64 */
typedef struct {
    uint64_t    sec;
    uint64_t    sec_hi;
    uint64_t    size;
    uint64_t    size_hi;
    uint64_t    modifydate;
    uint64_t    flags;
    FSZ_Access  owner;
} packed FSZ_Version;

/* sizeof = 4096 */
typedef struct {
    uint8_t     magic[4];       /*   0 mágikus 'FSIN' */
    uint32_t    checksum;       /*   4 CRC32, filetype és inlinedata (nem beleértve) közötti bájtokra */
    uint8_t     filetype[4];    /*   8 fő mime típus első négy bájtja, pl: text,imag,vide,audi,appl stb. */
    uint8_t     mimetype[36];   /*  12 mime altípus, pl: plain, html, gif, jpeg stb. (*) */
    uint8_t     encrypt[28];    /*  56 titkosítómaszk vagy nulla */
    uint32_t    enchash;        /*  76 titkosító jelszó CRC32, hogy ne kelljen rossz jelszóval visszakódolni */
    uint64_t    changedate;     /*  80 1970. jan. 1 00:00:00 UTC óta eltelt mikroszekundumok száma, inode módosítás ideje */
    uint64_t    accessdate;     /*  88 utolsó elérés ideje (ha implementált, egyébként nulla) */
    uint64_t    numblocks;      /*  96 az ehhez az inode-hoz lefoglalt blokkok száma (**) */
    uint64_t    numlinks;       /* 104 erre az inode-ra mutató hivatkozások száma */
    uint64_t    metalabel;      /* 112 a meta cimkék kezdő logikai szektorszáma */
    uint64_t    metalabel_hi;
    FSZ_Version version5;       /* 128 az előző legrégebbi verzió (ha a verziózás engedélyezve van) */
    FSZ_Version version4;       /* 192    mind ugyanolyan formátumú mint a current, lásd FSZ_Version fentebb */
    FSZ_Version version3;       /* 256 */
    FSZ_Version version2;       /* 320 */
    FSZ_Version version1;       /* 384 */
    /* FSZ_Version current; itt azért nem használtam FSZ_Version struktúrát, hogy ne kelljen annyit gépelni hivatkozáskor */
    uint64_t         sec;       /* 448 current jelenlegi (vagy egyetlen) verzió (***) */
    uint64_t         sec_hi;
    uint64_t         size;      /* 464 fájl mérete */
    uint64_t         size_hi;
    uint64_t         modifydate;/* 480 */
    uint64_t         flags;     /* 488 leképezés, lásd FSZ_IN_FLAG_* (***) */
    /* owner a legutolsó a current struktúrában, amit az ACL követ, ezért úgy is lehet értelmezni, hogy az első elem a
     * hozzáférési listában. Ez a kontroll ACE, azt is megmondja, ki módosíthatja az ACL listát. */
    FSZ_Access       owner;     /* 496 */
    union {
        struct {
          FSZ_Access groups[32];/* 512 összesen 32 FSZ_Access bejegyzés, csoporttagságok */
          uint8_t    inlinedata[FSZ_SECSIZE-1024];
        } small;
        struct {
          FSZ_Access groups[96];/* 512 összesen 96 FSZ_Access bejegyzés, csoporttagságok */
          uint8_t    inlinedata[FSZ_SECSIZE-2048];
        } big;
    } data;
} packed FSZ_Inode;

#define FSZ_IN_MAGIC "FSIN"

/* (*) az IANA szerint csak két mime típus csoport nem fér ebbe bele )mindkettő vendor specifikus):
 * application/vnd.collabio.xodicuments.* és application/vnd.openxmlformats-officedocument.*
 * ezeket le kell rövidíteni. Minden más, több, mint 1700 mime típus egyedi 4+36 bájton. */

/* sima fájlok, a 4. karakter sosem ':' */
#define FSZ_FILETYPE_REG_TEXT   "text"  /* fő mime típus */
#define FSZ_FILETYPE_REG_IMAGE  "imag"
#define FSZ_FILETYPE_REG_VIDEO  "vide"
#define FSZ_FILETYPE_REG_AUDIO  "audi"
#define FSZ_FILETYPE_REG_APP    "appl"
#define FSZ_FILETYPE_REG_BOOT   "boot"  /* ugyanaz, mint az "appl", csak a defrag nem helyezheti át */
/* speciális bejegyzések, a 4. karakter mindig ':' */
#define FSZ_FILETYPE_DIR        "dir:"  /* könyvtár */
#define FSZ_FILETYPE_UNION      "uni:"  /* könyvtárunió, a beágyazott adat egy nullával elválasztott elérési út lista */
#define FSZ_FILETYPE_INTERNAL   "int:"  /* fájlrendszerspecifikus fájlok, pl szabad és hibás szektorlista vagy meta cimkék */
#define FSZ_FILETYPE_SYMLINK    "lnk:"  /* szimbólikus hivatkozás, a beágyazott adat a mutatott elérési út */
/* fájlrendszerspecifkus mime típusok */
/* FSZ_FILETYPE_DIR-al használatos */
#define FSZ_MIMETYPE_DIR_ROOT   "fs-root"  /* gyökérkönyvtár (csak a helyreállíthatóság miatt) */
/* FSZ_FILETYPE_INTERNAL-al használatos */
#define FSZ_MIMETYPE_INT_FREELST "fs-free-sectors" /* szabad szektorok listája */
#define FSZ_MIMETYPE_INT_BADLST  "fs-bad-sectors"  /* hibás szektorok listája */
#define FSZ_MIMETYPE_INT_META    "fs-meta-labels"  /* meta cimkék */
#define FSZ_MIMETYPE_INT_INDEX   "fs-search-index" /* kereső gyorsítótár */
#define FSZ_MIMETYPE_INT_JOURNAL "fs-journal"      /* naplózási bejegyzések (csak metaadatokat tartalmaz) */
#define FSZ_MIMETYPE_INT_JOURDAT "fs-journal-data" /* naplózási bejegyzések (fájladatokat is tartalmaz) */

/* (**) numblocks-ba beleszámítódik minden szektorkönyvtár, indirekt szektor lista és adatszektor, de
 * a csupa nullát tartalmazó szektorok (lyukak) nem, az összes verzióra együttesen */

/* jelzók */
#define FSZ_IN_FLAG_HIST     (1<<8)   /* azt jelzi, hogy a korábbi verziók tárolva vannak */
#define FSZ_IN_FLAG_CHKSUM   (1<<9)   /* fájladatokon ellenprző összeg van */
#define FSZ_IN_EALG_SHACBC   (0<<10)  /* a fájl titkosítva van SHA-XOR-CBC metódussal */
#define FSZ_IN_EALG_AESCBC   (1<<10)  /* a fájl titkosítva van AES-256-CBC metódussal */
/* a 10. és 11. bit fenntartva jövőbeli titkosító metódusok kódolásához */

/* (***) a logikai szektorszám és adatszektor leképezése. Ezek a méretek
 * 4096 bájtos logikai szektormérettel lettek számolva. Ez állítható az
 * FSZ_SuperBlock-ban ha 11 szektorolvasás túl sok lenne a véletlenszerű
 * eléréshez Yotta méretű fájlok esetén.
 * A szektorkönyvtár 128 bites LSN-eket tárol, kivéve, ha FSZ_IN_FLAG_CHECKSUM be van állítva
 * a fájl verziónál, mert akkor "csak" 2^96 bites lehet az LSN.
 * Ha ez mégsem lenne elég, akkor használható FSZ_SectorList (extentek) az adatok tárolására,
 * ami egybefüggő szektorokat tárol, külön ellenőrzőösszeggel. */


/* szektorcím fordítás típusa */
#define FSZ_FLAG_TRANSLATION(x) (uint8_t)((x>>0)&0xFF)

/*  adatméret < szektorméret - 1024 (3072 bájt)
    FSZ_Inode.sec önmagára mutat.
    az adat az inode-ba van beágyazva 1024-től (vagy BIGINODE esetén 2048-tól)
    FSZ_Inode.sec -> FSZ_Inode.sec; adat  */
#define FSZ_IN_FLAG_INLINE  (0xFF<<0)

/*  adatméret < szektorméret (4096)
    az inode direktben mutat az adatra.
    FSZ_Inode.sec -> adat */
#define FSZ_IN_FLAG_DIRECT   (0<<0)

/*  adatméret < (szektorméret - 1024) * szektorméret / 16 (768k)
    FSZ_Inode.sec önmagára mutat, szektorkönyvtár van beágyazva.
    BIGINODE-nál ez a fajta leképezés 512k-ig használható.
    FSZ_Inode.sec -> FSZ_Inode.sec; sd -> adat */
#define FSZ_IN_FLAG_SDINLINE (0x7F<<0)

/*  adatméret < szektorméret * szektorméret / 16 (1 M)
    FSZ_Inode.sec egy szektorkönyvtárra mutat,
    ami egy 256 szektorcímet tartalmazó szektor
    FSZ_Inode.sec -> sd -> adat */
#define FSZ_IN_FLAG_SD       (1<<0)

/*  adatméret < szektorméret * szektorméret / 16 * szektorméret / 16 (256 M)
    FSZ_Inode.sec egy szektorkönyvtárra mutat,
    amiben 256 szektorkönyvtárra mutató szektor van,
    amik összesen 256*256 adatszektorra mutatnak
    FSZ_Inode.sec -> sd -> sd -> adat */
#define FSZ_IN_FLAG_SD2      (2<<0)

/*  adatméret < (64 G)
    FSZ_Inode.sec -> sd -> sd -> sd -> adat */
#define FSZ_IN_FLAG_SD3      (3<<0)

/*  adatméret < (16 T)
    FSZ_Inode.sec -> sd -> sd -> sd -> sd -> adat */
#define FSZ_IN_FLAG_SD4      (4<<0)

/*  adatméret < (4 Peta, egyenlő 4096 Terra)
    FSZ_Inode.sec -> sd -> sd -> sd -> sd -> sd -> adat */
#define FSZ_IN_FLAG_SD5      (5<<0)

/*  adatméret < (1 Exa, egyenlő 1024 Peta)
    FSZ_Inode.sec -> sd -> sd -> sd -> sd -> sd -> sd -> adat */
#define FSZ_IN_FLAG_SD6      (6<<0)

/*  adatméret < (256 Exa)
    FSZ_Inode.sec -> sd -> sd -> sd -> sd -> sd -> sd -> sd -> adat */
#define FSZ_IN_FLAG_SD7      (7<<0)

/*  adatméret < (64 Zetta, egyenlő 65536 Exa) */
#define FSZ_IN_FLAG_SD8      (8<<0)

/*  adatméret < (16 Yotta, egyenlő 16384 Zetta) */
#define FSZ_IN_FLAG_SD9      (9<<0)

/*  mivel a szektorlista szektorszámot is tárol, ezért lehetetlen megmondani,
 *  mekkora fájlokra alkalmazható, ehelyett inkább a töredezettséggel számolunk */

/*  beágyazott szektorlista ((szektorméret - 1024) / 32, 96 bejegyzésig)
    FSZ_Inode.sec önmagára mutat, FSZ_SectorList bejegyzések vannak beágyazva.
    FSZ_Inode.sec -> FSZ_Inode.sec; sl -> adat */
#define FSZ_IN_FLAG_SECLIST  (0x80<<0)

/*  normál szektorlista ((szektorméret - 1024) * szektorméret / 16 / 32, egészen 24576 bejegyzésig)
    FSZ_Inode.sec önmagára mutat, egy szektorkönyvtár van beágyazva
    ami FSZ_SectorList bejegyzéseket tartalmazó szektorokra mutat.
    FSZ_Inode.sec -> FSZ_Inode.sec; sd -> sl -> adat */
#define FSZ_IN_FLAG_SECLIST0 (0x81<<0)

/*  indirekt szektorlista (egészen 32768 bejegyzésig)
    FSZ_Inode.sec egy szektorkönyvtárra mutat, ami meg FSZ_SectorList bejegyzésekre
    FSZ_Inode.sec -> sd -> sl -> adat */
#define FSZ_IN_FLAG_SECLIST1 (0x82<<0)

/*  duplán indirekt szektorlista (egészen 8388608 bejegyzésig)
    FSZ_Inode.sec egy szektorkönyvtárra mutat, ami szektorkönyvtárakra
    mutatnak amik FSZ_SectorList bejegyzésekre mutatnak
    FSZ_Inode.sec -> sd -> sd -> sl -> adat */
#define FSZ_IN_FLAG_SECLIST2 (0x83<<0)

/*  triplán indirekt szektorlista (egészen 2147483648 bejegyzésig)
    FSZ_Inode.sec -> sd -> sd -> sd -> sl -> adat */
#define FSZ_IN_FLAG_SECLIST3 (0x84<<0)

/*********************************************************
 *                      Könyvtár                         *
 *********************************************************/
/* az első bejegyzés a fejléc. */

/* sizeof = 128 */
typedef struct {
    uint8_t     magic[4];
    uint32_t    checksum;       /* a további bejegyzések CRC32-e */
    uint64_t    flags;
    uint64_t    numentries;
    uint64_t    numentries_hi;
    uint8_t     reserved[96];
} packed FSZ_DirEntHeader;

#define FSZ_DIR_MAGIC "FSDR"
#define FSZ_DIR_FLAG_UNSORTED (1<<0)
#define FSZ_DIR_FLAG_HASHED   (2<<0)

/* a könyvtárbejegyzések fix méretűek és lexikografikusan (ábécébe) vannak
 * rendezve. Ez lassabb írást jelent, ugyanakkor hihetetlen gyors olvasást. */

/* sizeof = 128 */
typedef struct {
    uint64_t      fid;                /* a fájl inode-jának a logikai szektorszáma */
    uint64_t      fid_hi;
    uint8_t       length;             /* az UNICODE karakterek száma a névben */
    unsigned char name[111];          /* nullával lezárt UTF-8 sztring, fájlnév */
} packed FSZ_DirEnt;

/*********************************************************
 *                    Könyvtárunió                       *
 *********************************************************/

/* a beágyazott adat egy nullával határolt és lezárt, '...'-ot tartalmazható elérési utak
 * listája. A legutolsó elem egy üres elérési út.
 * Példa unió /usr/bin: beágyazva=/bin(nulla)/usr/.../bin(nulla)(nulla)
 */
/*********************************************************
 *                     Meta cimkék                       *
 *********************************************************/

/* meta cimkék szektorhatárra igazított, nullával lezárt JSON sztringek,
 * a szektor hátralévő része pedig nullákkal van feltöltve.
 *
 * Példa (feltéve hogy a meta címke fájl az LSN 1234-en kezdődik):
 * {"icon":"/usr/firefox/share/icon.png"} (nullák szektorméretik)
 * {"icon":"/usr/vlc/share/icon.svg","downloaded":"http://videolan.org"} (nullák, legalább egy)
 *
 * Az /usr/firefox/bin/firefox inodjában: metalabel=1234
 * Az /usr/vlc/bin/vlc inodjában: metalabel=1235
 *
 * Alapesetben a meta címkék nemléphetik át a szektorméretet. Ha mégis, akkor gondosan kell
 * allokálni őket, hogy egymásutánni szektorokra kerüljön egy meta blokk. Ez megnehezíti a
 * létrehozást, amikor nagyon sok meta cimke (>4096) van egy-egy fájlon, azonban leegyszerűsíti
 * az elérésüket, mivel nem kell LSN-t leképezni a metacímke fájlra. Feltéve, hogy gyakrabban
 * kell a címkéket olvasni, mint írni, és hogy többnyire elég a 4096 bájt, ez ésszerű
 * kompromisszum. Más szóval a meta blokkok egy vagy több egybefüggő szektoron helyezkednek el,
 * és a meta cimke fájl pont úgy fedi le ezeket, mint a hibás szektorokat a hibás szektor fájl.
 */

/*********************************************************
 *                     Keresőindex                       *
 *********************************************************/

/* még nincs definiálva. A kereső cimkék ("search" meta cimkék) inode listái */

/*********************************************************
 *                    Naplózóadatok                      *
 *********************************************************/

/* A naplófájl a szuperblokkban van megadva, a journalhead (fej) és journaltail (vég) mutatók pedig
 * körkörös buffernek használják a fájl területét. A naplófájlt mindig FSZ_IN_FLAG_SECLIST-el és
 * egyetlen extent-el kell allokálni. Minden írási tranzakció több szektort tartalmaz, amit egy
 * tranzakció vége szektor zár le, amiben 16 bájtos rekordok találhatók (a szektorok tényleges LSN-jei).
 * Az első bejegyzés mindig a fejléc. */

/* sizeof = 16 */
typedef struct {
    uint8_t     magic[4];       /* mágikus 'JRTR' */
    uint32_t    checksum;       /* bejegyzések CRC32-je */
    uint64_t    numentries;     /* ebben a tranzakcióban kiírt szektorok száma */
} packed FSZ_JournalTransaction;
/* ezt 16 bájtos szektorcímek követik, egészen FSZ_SECSIZE szektorméretig. */

#define FSZ_JT_MAGIC "JRTR"

/*********************************************************
 *                     Titkosítás                        *
 *********************************************************/

/* Titkosítás állítható a lemezre és egyes fájlokra is (ha enchash nem nulla):
 *   enckey=sha256(sha256(jelszó+só)-4 bájt xorolva az encrypt[]-el)
 *   ha EALG_SHACBC: XOR SHA blokk, nagyon gyors, és elég jó ciklikus blokk titposító, az enckey az iv
 *   ha EALG_AESCBC: AES-256-CBC, kulcs=enckey, iv=substr(enckey,enckey[0]&15,16)
 * Ezáltal a titkosított lemezt/fájlt nem kell újraírni, ha a jelszó változik. Vagy éppen újra lehet
 * titkosítani a lemezt a jelszó változtatása nélkül. Az enchash CRC csak arra van használva, hogy
 * megakadályozzuk a hibás jelszóval való dekódolást. Az adatellenőrzőösszeg az első titkosítóblokkban
 * van direkt tárolva, megnehezítve ezzel a visszafejtést.
 */

#endif /* fsZ.h */
