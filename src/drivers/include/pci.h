/*
 * drivers/include/pci.h
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
 * @subsystem eszközmeghajtók
 * @brief PCI definíciók
 */

#ifndef _DRV_PCI_H
#define _DRV_PCI_H

#define PCI_HEADER_DEVICE   0
#define PCI_HEADER_BRIDGE   1
#define PCI_HEADER_CARDBUS  2

#define PCI_DEV_MODEL       0
#define PCI_DEV_CLASS       8
#define PCI_DEV_HEADER      0x0C
#define PCI_DEV_BARS        0x10
#define PCI_DEV_SUBSYS      0x2C
#define PCI_VENDOR(x)       (x & 0xffff)
#define PCI_DEVICE(x)       (x >> 16)
#define PCI_CLASS(x)        ((x >> 24) & 0xff)
#define PCI_SUBCLASS(x)     ((x >> 16) & 0xff)

#define PCI_MMIO_BASE       (DEVSPEC_BASE + __PAGESIZE)

/*
 * egy eszközmeghajtó vagy
 *   pcidev_t *pcidev = (pcidev_t*)DEVSPEC_BASE;
 * vagy
 *   pcibridge_t *pcidev = (pcibridge_t*)DEVSPEC_BASE;
 * definíciót használ. A struktúrában hivatkozott memóriák le vannak képezve, laphatáron kezdődően a következő laptól.
 */

#ifndef _AS
/**
 * sima PCI eszköz esetén (DEVSPEC_BASE[0xE] == PCI_HEADER_DEVICE)
 */
typedef struct {
    uint16_t vendor;
    uint16_t device;

    uint16_t command;
    uint16_t status;

    uint8_t revision;
    uint8_t progif;
    uint8_t subclass;
    uint8_t classcode;

    uint8_t cacheline_siz;
    uint8_t latency_timer;
    uint8_t header;
    uint8_t bist;

    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;

    uint32_t cardbus_cis_ptr;

    uint16_t subsystem_vendor;
    uint16_t subsystem_device;

    uint32_t exansionrom;

    uint8_t capabilities;
    uint8_t res1;
    uint16_t res2;

    uint32_t res3;

    uint8_t irq_line;
    uint8_t irq_pin;
    uint8_t min_grant;
    uint8_t max_latency;

    uint8_t reserved[__PAGESIZE-64-8*4];

    uint32_t bar0_size;
    uint32_t bar1_size;
    uint32_t bar2_size;
    uint32_t bar3_size;
    uint32_t bar4_size;
    uint32_t bar5_size;
    uint32_t bar6_size;
    uint32_t bar7_size;
} __attribute__ ((packed)) pcidev_t;
c_assert(sizeof(pcidev_t)==__PAGESIZE);

/**
 * PCI 2 PCI híd esetén (DEVSPEC_BASE[0xE] == PCI_HEADER_BRIDGE)
 */
typedef struct {
    uint16_t vendor;
    uint16_t device;

    uint16_t command;
    uint16_t status;

    uint8_t revision;
    uint8_t progif;
    uint8_t subclass;
    uint8_t classcode;

    uint8_t cacheline_siz;
    uint8_t latency_timer;
    uint8_t header;
    uint8_t bist;

    uint32_t bar0;
    uint32_t bar1;

    uint8_t pri_bus_num;
    uint8_t sec_bus_num;
    uint8_t sub_bus_num;
    uint8_t sec_latency_timer;

    uint8_t io_base_lo;
    uint8_t io_limit_lo;
    uint16_t sec_status;

    uint16_t mmio_base_lo;
    uint16_t mmio_limit_lo;
    uint16_t mem_base_lo;
    uint16_t mem_limit_lo;

    uint32_t mem2_base_hi;
    uint32_t mem2_base_lo;

    uint16_t io_base_hi;
    uint16_t io_limit_hi;

    uint8_t capabilities;
    uint8_t res1;
    uint16_t res2;

    uint32_t expansionrom;

    uint8_t irq_line;
    uint8_t irq_pin;
    uint16_t bridge_cntr;

    uint8_t reserved[__PAGESIZE-64-8*4];

    uint32_t bar0_size;
    uint32_t bar1_size;
    uint32_t bar2_size;
    uint32_t bar3_size;
    uint32_t bar4_size;
    uint32_t bar5_size;
    uint32_t bar6_size;
    uint32_t bar7_size;
} __attribute__ ((packed)) pcibridge_t;
c_assert(sizeof(pcibridge_t)==__PAGESIZE);

/**
 * CardBus esetén (DEVSPEC_BASE[0xE] == PCI_HEADER_CARDBUS), nincs ilyen meghajtónk egyelőre
 */
#endif

#endif
