/*
 * core/security.c
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
 * @subsystem taszk
 * @brief üzenetküldés biztonsági házirend
 */

#include <core.h>

/* root uuid, fontos, hogy nullákkal legyen feltöltve a vége */
uint8_t rootuid[15] = { 'r', 'o', 'o', 't', 0,0,0,0,0,0,0,0,0,0,0 };

/**
 * ellenőrzi, hogy az adott taszk rendelkezik-e a megadott jogosultsággal
 */
bool_t task_allowed(tcb_t *tcb, char *grp, uint8_t access)
{
    uint8_t *g;
    uint i, j;
    if(grp == NULL || !grp[0]) return false;
    j = strlen(grp);
    if(j > 15) j = 15;
    for(i=0; i < TCB_MAXACE; i++) {
        g = (uint8_t *)(&tcb->acl[i]);
        /* biztonság kedvéért, lista vége ellenőrzés */
        if(!g[0]) return false;
        /* ha bármilyen hozzáférést kértünk vagy rendelkezik a megadottal */
        if(!memcmp(g, grp, j)) return (access == 0 || (g[15] & access) == access) ? true : false;
    }
    return false;
}

/**
 * ellenőrzi, hogy az aktuális taszk elküldhet-e egy bizonyos üzenetet
 */
bool_t msg_allowed(tcb_t *sender, pid_t dest, evt_t event)
{
    evt_t e = EVT_FUNC(event);

    /* Core szolgáltatások */
    if((int64_t)dest == SRV_CORE) {
        if((e == SYS_regirq || e == SYS_regtmr || e == SYS_drvfind || e == SYS_drvadd) && sender->priority != PRI_DRV) goto noaccess;
        if((e == SYS_tskcpy || e == EVT_ack) && sender->priority != PRI_DRV && sender->priority != PRI_SRV) goto noaccess;
        if(!memcmp((void*)&sender->owner, rootuid, 15)) return true;
        if((e == SYS_stimezone || e == SYS_stime || e == SYS_stimebcd) && !task_allowed(sender, "time", A_WRITE)) goto noaccess;
        return true;
    }

    /* FS taszk */
    if((int64_t)dest == SRV_FS || dest == services[-SRV_FS]) {
        if(e == SYS_mountfs && sender->pid != services[-SRV_FS]) goto noaccess;
        if(e == SYS_getty && sender->pid != services[-SRV_UI]) goto noaccess;
        if((e == SYS_mknod || e == SYS_setblock) && sender->priority != PRI_DRV && sender->priority != PRI_SRV) goto noaccess;
        if(!memcmp((void*)&sender->owner, rootuid, 15)) return true;
        if(e == SYS_chroot && !task_allowed(sender, "chroot", A_WRITE)) goto noaccess;
        if((e == SYS_mount || e == SYS_umount) && !task_allowed(sender, "storage", A_WRITE)) goto noaccess;
        return true;
    }

    /* UI taszk */
    if((int64_t)dest == SRV_UI || dest == services[-SRV_UI]) {
        if(task_allowed(sender, "noui", 0)) goto noaccess;
        if(e == SYS_devprogress && sender->pid != services[-SRV_FS]) goto noaccess;
        return true;
    }

    /* syslog taszk */
    if((int64_t)dest == SRV_syslog || dest == services[-SRV_syslog]) {
        if(task_allowed(sender, "nosyslog", 0)) goto noaccess;
        return true;
    }

    /* inet taszk */
    if((int64_t)dest == SRV_inet || dest == services[-SRV_inet]) {
        if(task_allowed(sender, "noinet", 0)) goto noaccess;
        return true;
    }

    /* sound taszk */
    if((int64_t)dest == SRV_sound || dest == services[-SRV_sound]) {
        if(task_allowed(sender, "nosound", 0)) goto noaccess;
        return true;
    }

    /* init task */
    if((int64_t)dest == SRV_init || dest == services[-SRV_init]) {
        if(task_allowed(sender, "noinit", 0)) goto noaccess;
        return true;
    }

    /* ha idáig eljutottunk, akkor el lehet küldeni az üzenetet */
    return true;
noaccess:
    seterr(EPERM);
    return false;
}
