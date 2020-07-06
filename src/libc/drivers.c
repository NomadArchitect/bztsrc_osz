/*
 * libc/drivers.c
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
 * @brief Csak eszközmeghajtók számára elérhető funkciók
 */
#include <osZ.h>

/**
 * irq üzenetek kérése (csak eszközmeghajtók hívhatják)
 */
public void drv_regirq(uint16_t irq)    { mq_send1(SRV_CORE, SYS_regirq, irq); }

/**
 * másodpercenkénti időzítő üzenetek kérése (csak eszközmeghajtók hívhatják, USHRT_MAX irq üzeneteket küld)
 */
public void drv_regtmr()                { mq_send0(SRV_CORE, SYS_regtmr); }

/**
 * eszközmeghajtóprogram keresése eszközspecifikáció alapján (csak eszközmeghajtók hívhatják)
 * visszatérési érték: 1 ha van, 0 ha nincs
 */
public void drv_find(char *spec, char*drv, int len) { mq_send3(SRV_CORE, SYS_drvfind, (virt_t)spec, (virt_t)drv, len); }

/**
 * eszközmeghajtóprogram hozzáadása (csak eszközmeghajtók hívhatják)
 */
public void drv_add(char *drv, void *memspec)       { mq_send2(SRV_CORE, SYS_drvadd, (virt_t)drv, (virt_t)memspec); }

/**
 * eszközhivatkozás hozzáadása (csak eszközmeghajtók és szolgáltatások hívhatják)
 */
public int mknod(const char *devname, dev_t minor, mode_t mode, blksize_t size, blkcnt_t cnt)
{
    msg_t *msg;

    if(!devname || !devname[0] || devname[0] == '/') {
        seterr(EINVAL);
        return -1L;
    }
    msg = mq_call5(SRV_FS, SYS_mknod|MSG_PTRDATA, devname, strlen(devname)+1, minor, (((uint64_t)mode)<<32)|size, cnt);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

