/*
 * drivers/bridge/pci/main.c
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
 * @subsystem eszközmeghajtók
 * @brief PCI busz felderítés
 */

#include <osZ.h>
#include <driver.h>
#include <pci.h>

/*
#define PCI_BAR_SIZE(i) (*((uint32_t*)(LDYN_tmpmap1+__PAGESIZE-8*4+(i<<2))))
*/

char pcipath[32];
mem_entry_t memspec[8];

/*
 * PCI configurációs regiszterek írása / olvasása
 */
static inline uint32_t pci_config_read(uint8_t bus, uint8_t dev, uint8_t fnc, uint32_t reg)
{
    outl(0xCF8,
        ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)fnc << 8) | (reg & 0xfc) | ((reg & 0xf00) << 16) | 0x80000000);
    return inl(0xCFC);
}

static inline void pci_config_write(uint8_t bus, uint8_t dev, uint8_t fnc, uint32_t reg, uint32_t val)
{
    outl(0xCF8,
        ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)fnc << 8) | (reg & 0xfc) | ((reg & 0xf00) << 16) | 0x80000000);
    outl(0xCFC, val);
}

/**
 * busz pásztázása, eszközök detektálása vagy eszköz inicializálás (eszközmeghajtók implementálják)
 */
public void drv_init()
{
    uint8_t b, d, f, m;
    uint32_t dev, sub, cls;
/*
    uint8_t n;
    uint32_t *v;
    phy_t p, s;
    uint i, j;
*/
    char drv[256];

    for(b = 0; b < 254; b++) {
        for(d = 0; d < 32; d++) if(pci_config_read(b, d, 0, PCI_DEV_MODEL) != -1U) {
            m = (pci_config_read(b, d, 0, PCI_DEV_HEADER)>>16)&0x80? 7 : 1;
            for(f = 0; f < m; f++) {
                dev = pci_config_read(b,d,f,0);
                if(dev != -1U) {
                    sub = pci_config_read(b, d, f, PCI_DEV_SUBSYS);
                    cls = pci_config_read(b, d, f, PCI_DEV_CLASS);
                    drv[0] = 0;
                    /* először megnézzük a teljes alrendszer modellre van-e egyezés */
                    sprintf(pcipath, "pci%2x:%2x:%2x:%2x", PCI_VENDOR(dev), PCI_DEVICE(dev), PCI_VENDOR(sub), PCI_DEVICE(sub));
/*syslog(LOG_INFO, pcipath);*/
                    drv_find(pcipath, drv, sizeof(drv));
                    if(!drv[0]) {
                        /* ha nincs, akkor a modellhez van-e meghajtó */
                        sprintf(pcipath, "pci%2x:%2x", PCI_VENDOR(dev), PCI_DEVICE(dev));
                        drv_find(pcipath, drv, sizeof(drv));
                        if(!drv[0]) {
                            /* végül hogy az osztályhoz és altípushoz van-e */
                            sprintf(pcipath, "cls%1x:%1x", PCI_CLASS(cls), PCI_SUBCLASS(cls));
                            drv_find(pcipath, drv, sizeof(drv));
                        }
                    }
#if DEBUG
                    if(_debug&DBG_DEVICES)
                        dbg_printf("PCI %d.%d.%d (%2x:%2x (%2x:%2x) class %1x:%1x) driver %S\n", b, d, f, PCI_VENDOR(dev),
                            PCI_DEVICE(dev), PCI_VENDOR(sub), PCI_DEVICE(sub), PCI_CLASS(cls), PCI_SUBCLASS(cls), drv);
#endif
                    /* ha találtunk eszközmeghajtót */
                    if(drv[0]) {
                        memset(memspec, 0, sizeof(memspec));
#if 0
                        /* még nincs taszkunk, ezért az idle-nek könyveljük ezt az egy lapot */
                        memspec[0].base = (phy_t)pmm_alloc(&idle_tcb, 1);
                        memspec[0].size = 0x8000000000000001UL; /* jelezzük a drivers.c-nek, hogy foglalt lap */
                        vmm_page(0, LDYN_tmpmap1, memspec[0].base, PG_CORE_RWNOCACHE|PG_PAGE);
                        for(v = (uint32_t*)LDYN_tmpmap1, i = 0; i < 256; i += 4, v++) *v = pci_config_read(b, d, f, i);
                        /* megnézzük van-e MMIO a BAR-okban */
                        n = *((uint8_t*)(LDYN_tmpmap1 + PCI_DEV_HEADER+2)) & 0x7F;
                        n = (!n? 6 : (n==1? 2 : 0));    /* PCI2PCI bridge-ben csak 2 BAR van, CardBus-ban meg egy se */
                        v = (uint32_t*)(LDYN_tmpmap1 + PCI_DEV_BARS);
                        for(j = 1, i = 0; i < n && j < 7; i++, v++) {
                            /* ha MMIO és nem IO cím */
                            if(!(*v & 1)) {
                                /* MMIO esetén trükkös megtalálni a méretet */
                                sub = PCI_DEV_BARS + (i << 2);
                                pci_config_write(b, d, f, sub, 0xffffffffU);
                                PCI_BAR_SIZE(i) = ~(pci_config_read(b, d, f, sub) & 0xfffffff0U) + 1;
                                memspec[j].size = ((PCI_BAR_SIZE(i) + __PAGESIZE - 1) >> __PAGEBITS) << __PAGEBITS;
                                pci_config_write(b, d, f, sub, *v);
                                /* ha 64 bites cím, akkor két BAR helyet foglal */
                                if(((*v >> 1) & 3) == 2) { memspec[j].base = ((phy_t)v[1] << 32) | (v[0] & ~0xf); v++; i++; }
                                else memspec[j].base = v[0] & ~0xf;
                                j++;
                            }
                        }
                        /* PCI 2 PCI híd esetén van még két egyéb memória cím is */
                        if(n==2) {
                            if (*((uint16_t*)(LDYN_tmpmap1 + 0x20)) && *((uint16_t*)(LDYN_tmpmap1 + 0x22)) && j < 7) {
                                PCI_BAR_SIZE(2) = *((uint16_t*)(LDYN_tmpmap1 + 0x22));
                                memspec[j].base = *((uint16_t*)(LDYN_tmpmap1 + 0x20));
                                memspec[j++].size = ((PCI_BAR_SIZE(2) + __PAGESIZE - 1) >> __PAGEBITS) << __PAGEBITS;
                            }

                            p = ((phy_t)(*((uint32_t*)(LDYN_tmpmap1 + 0x28))) << 16) | *((uint16_t*)(LDYN_tmpmap1 + 0x24));
                            s = ((phy_t)(*((uint32_t*)(LDYN_tmpmap1 + 0x2C))) << 16) | *((uint16_t*)(LDYN_tmpmap1 + 0x26));
                            if(p && s && j < 7) {
                                PCI_BAR_SIZE(3) = s;
                                memspec[j].base = p;
                                memspec[j++].size = ((PCI_BAR_SIZE(3) + __PAGESIZE - 1) >> __PAGEBITS) << __PAGEBITS;
                            }
                        }
#endif
                        /* eszközmeghajtó taszk hozzáadása. drv-ben a meghajtó, memspec-ben a leképezendő memória listája */
                        drv_add(drv, memspec);
                    } /* ha volt hozzá meghajtó */
                } /* ha van eszköz */
            } /* funkció */
        } /* eszköz */
    } /* busz */
}

/**
 * drv_regirq() vagy drv_regtmr() esetén hívódik, utóbbinál irq == USHRT_MAX (eszközmeghajtók implementálják)
 */
public void drv_irq(unused uint16_t irq, unused uint64_t ticks)
{
}

/**
 * akkor hívódik, ha valaki megnyitja a mknod() által kreált fájlt (eszközmeghajtók implementálják)
 */
public void drv_open(unused dev_t device, unused uint64_t mode)
{
}

/**
 * amikor bezárják az eszközfájlt (eszközmeghajtók implementálják)
 */
public void drv_close(unused dev_t device)
{
}

/**
 * olvasás az eszközfájlból (eszközmeghajtók implementálják)
 */
public void drv_read(unused dev_t device)
{
}

/**
 * írás az eszközfájlba (eszközmeghajtók implementálják)
 */
public void drv_write(unused dev_t device)
{
}

/**
 * eszközparancs (eszközmeghajtók implementálják)
 */
public void drv_ioctl(unused dev_t device)
{
}
