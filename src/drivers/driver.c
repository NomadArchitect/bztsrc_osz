/*
 * drivers/driver.c
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
 * @brief eszközmeghajtó diszpécser és alap funkciók
 */

#include <osZ.h>
#include <driver.h>

drvmem_t drv_phymem = { 0 };            /* rendszerbufferek és rendszertáblák fizikai címei */
mem_entry_t drv_bufmem[256] = { 0 };    /* leképzett memória listája */

/**
 * irq üzenetek kérése (csak eszközmeghajtók hívhatják)
 */
public void drv_regirq(uint16_t irq)                { mq_send1(SRV_CORE, SYS_regirq, irq); }

/**
 * másodpercenkénti időzítő üzenetek kérése (csak eszközmeghajtók hívhatják, USHRT_MAX irq üzeneteket küld)
 */
public void drv_regtmr()                            { mq_send0(SRV_CORE, SYS_regtmr); }

/**
 * eszközmeghajtóprogram keresése eszközspecifikáció alapján (csak eszközmeghajtók hívhatják)
 * visszatérési érték: 1 ha van, 0 ha nincs
 */
public void drv_find(char *spec, char *drv, int len){ mq_send3(SRV_CORE, SYS_drvfind, (virt_t)spec, (virt_t)drv, len); }

/**
 * eszközmeghajtóprogram hozzáadása (csak eszközmeghajtók hívhatják)
 */
public void drv_add(char *drv, mem_entry_t *memspec){ mq_send2(SRV_CORE, SYS_drvadd, (virt_t)drv, (virt_t)memspec); }

/**
 * rendszermemória virtuális címének lekérdezése (csak eszközmeghajtók hívhatják)
 */
public void *drv_virtmem(phy_t addr)
{
    mem_entry_t *mem;
    virt_t v;
    if(addr)
        for(mem = drv_bufmem, v = BUF_ADDRESS; mem->base && mem->size; mem++) {
            if(addr >= mem->base && addr < mem->base + mem->size)
                return (void*)(addr - mem->base + v);
            /* biztonság kedvéért lapra kerekítjük a méretet */
            v += ((mem->size+__PAGESIZE-1) & ~(__PAGESIZE-1));
        }
    return NULL;
}

/*** fő eseménykezelő ciklus ***/
public int main(unused int argc, unused char **argv)
{
    msg_t *msg;

    drv_init();

    while(1) {
        msg = mq_recv();
        switch(EVT_FUNC(msg->evt)) {
            case DRV_IRQ:
                drv_irq(msg->data.scalar.arg0,msg->data.scalar.arg1);
                if(msg->data.scalar.arg0 != USHRT_MAX) mq_send1(SRV_CORE, EVT_ack, msg->data.scalar.arg0);
                break;
            case DRV_open: drv_open(msg->data.scalar.arg0, msg->data.scalar.arg1); break;
            case DRV_close: drv_close(msg->data.scalar.arg0); break;
            case DRV_read: drv_read(msg->data.scalar.arg0); break;
            case DRV_write: drv_write(msg->data.scalar.arg0); break;
            case DRV_ioctl: drv_ioctl(msg->data.scalar.arg0); break;
#if DEBUG
            /* ennek nem szabad előfordulnia, mivel csak a core és az FS taszk küldhet üzenetet */
            default: dbg_printf("driver pid %x: unknown message type\n", getpid()); break;
#endif
        }
    }
    /* ide sose juthat el, de biztos, ami biztos */
    return EX_UNAVAILABLE;
}
