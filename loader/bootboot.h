/*
 * bootboot.h
 *
 * Copyright (C) 2017 - 2020 bzt (bztsrc@gitlab)
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
 * Ez a fájl a BOOTBOOT Protokoll csomag része.
 * @brief A BOOTBOOT struktúra
 *
 */

#ifndef _BOOTBOOT_H_
#define _BOOTBOOT_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define BOOTBOOT_MAGIC "BOOT"

/* a 0-ás és 1-es szintű betöltők alapértelmezett virtuális címei */
#define BOOTBOOT_MMIO   0xfffffffff8000000  /* memóriába leképezett B/K virtuális címe */
#define BOOTBOOT_FB     0xfffffffffc000000  /* frémbuffer virtuális címe */
#define BOOTBOOT_INFO   0xffffffffffe00000  /* bootboot struktúra virtuális címe */
#define BOOTBOOT_ENV    0xffffffffffe01000  /* environment sztring virtuális címe */
#define BOOTBOOT_CORE   0xffffffffffe02000  /* core betölthető szegmens kezdetének címe */

/* minimum protokoll szint:
 *  bevasalt kernelnév, statikus kernel memória címek */
#define PROTOCOL_MINIMAL 0
/* statikus protokoll szint:
 *  kernelnév az environment-ből, statikus kernel memória címek */
#define PROTOCOL_STATIC  1
/* dinamikus protokoll szint:
 *  kernelnév az environment-ből, kernel memória címek az ELF vagy PE szimbólumokból */
#define PROTOCOL_DYNAMIC 2
/* nagyüelöl (big-endian) jelző */
#define PROTOCOL_BIGENDIAN 0x80

/* betöltő típusok, csupán jelzés értékű */
#define LOADER_BIOS (0<<2)
#define LOADER_UEFI (1<<2)
#define LOADER_RPI  (2<<2)

/* frémbuffer pixel formátuma, csak 32 bites mód támogatott */
#define FB_ARGB   0
#define FB_RGBA   1
#define FB_ABGR   2
#define FB_BGRA   3

/* mmap bejegyzés, a típus a méret legkevésbé szignifikáns alsó terádjában
 * (félbájtjában) van tárolva. Ez azt jelenti, csak 16 bájt többszöröse lehet
 * a méret (nem probléma, a legtöbb modern firmver úgyis lapokban, 4096
 * bájtos méretben adja vissza a méretet). */
typedef struct {
  uint64_t   ptr;
  uint64_t   size;
} __attribute__((packed)) MMapEnt;
#define MMapEnt_Ptr(a)  (a->ptr)
#define MMapEnt_Size(a) (a->size & 0xFFFFFFFFFFFFFFF0)
#define MMapEnt_Type(a) (a->size & 0xF)
#define MMapEnt_IsFree(a) ((a->size&0xF)==1)

#define MMAP_USED     0   /* nem használható. Fenntartott vagy ismeretlen régió */
#define MMAP_FREE     1   /* használható memória */
#define MMAP_ACPI     2   /* acpi memória, tranzisztens és nem tranzisztens egyaránt */
#define MMAP_MMIO     3   /* memóriába leképzett B/K régió */

#define INITRD_MAXSIZE 16 /* Mb */

typedef struct {
  /* az első 64 bájt platform független */
  uint8_t    magic[4];    /* mágikus 'BOOT' */
  uint32_t   size;        /* a bootboot struktúra mérete, minimum 128 */
  uint8_t    protocol;    /* lásd PROTOCOL_* és LOADER_* fentebb */
  uint8_t    fb_type;     /* frémebuffer típusa, lásd FB_* fentebb */
  uint16_t   numcores;    /* processzormagok száma */
  uint16_t   bspid;       /* indító (Bootsrap) processor azonosító (a Local APIC Id x86_64-on) */
  int16_t    timezone;    /* időzóna, percekben -1440..1440 */
  uint8_t    datetime[8]; /* dátum BCD-ben ééééhhnnóóppmm UTC (időzónától független) */
  uint64_t   initrd_ptr;  /* memórialemezkép poziciója és mérete a memóriában */
  uint64_t   initrd_size;
  uint8_t    *fb_ptr;     /* frémbuffer fizikai címe és dimenziója */
  uint32_t   fb_size;
  uint32_t   fb_width;
  uint32_t   fb_height;
  uint32_t   fb_scanline;

  /* a többi (újabb 64 bájt) platformspecifikus */
  union {
    struct {
      uint64_t acpi_ptr;
      uint64_t smbi_ptr;
      uint64_t efi_ptr;
      uint64_t mp_ptr;
      uint64_t unused0;
      uint64_t unused1;
      uint64_t unused2;
      uint64_t unused3;
    } x86_64;
    struct {
      uint64_t acpi_ptr;
      uint64_t mmio_ptr;
      uint64_t efi_ptr;
      uint64_t unused0;
      uint64_t unused1;
      uint64_t unused2;
      uint64_t unused3;
      uint64_t unused4;
    } aarch64;
  } arch;

  /* a 128. bájttól a bootboot méretig, több MMapEnt[] bejegyzés */
  MMapEnt    mmap;
  /* használat:
   * MMapEnt *mmap_ent = &bootboot.mmap;
   * majd mmap_ent++; amíg át nem lépi a bootboot->size-ban megadott méretet */
} __attribute__((packed)) BOOTBOOT;


#ifdef  __cplusplus
}
#endif

#endif
