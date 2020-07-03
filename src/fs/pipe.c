/*
 * fs/pipe.c
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
 * @subsystem fájlrendszer
 * @brief csővezetékek kezelése
 */

#include <osZ.h>
#include "pipe.h"
#include "taskctx.h"

public uint64_t npipe = 0;
public pipe_t *pipe = NULL;

/* külső erőforrások */
extern stat_t st;                      /* stat buffer a pipe_stat-hoz */
extern bool_t ackdelayed;              /* aszinkron olvasás jelző */

/**
 * új csővezeték hozzáadása
 */
uint64_t pipe_add(fid_t fid, pid_t pid)
{
    uint64_t i, j = -1U;

    /* üres hely keresése */
    for(i = 0; i < npipe; i++) {
        if(!pipe[i].nopen) { j = i; break; }
    }
    if(j == -1U) {
        /* nincs üres hely, hozzáadunk egyet */
        i = npipe;
        npipe++;
        pipe = (pipe_t*)realloc(pipe, npipe * sizeof(pipe_t));
        if(!pipe) return -1U;
    } else
        i = j;
    /* mezők kitöltése */
    pipe[i].fid = fid;
    pipe[i].reader = 0;
    pipe[i].size = pipe[i].rd = 0;
    pipe[i].owner = getuidp(pid);
    return i;
}

/**
 * csővezeték eltávolítása
 */
void pipe_del(uint64_t idx)
{
    pipe_t *p;

    /* paraméterek ellenőrzése */
    if(idx >= npipe) return;
    p = &pipe[idx];
    /* hivatkozások csökkentése */
    if(p->nopen > 0) p->nopen--;
    /* felszabadítjuk, ha nullára csökkent */
    if(!p->nopen) {
        if(p->data) free(p->data);
        p->data = NULL;
        p->reader = 0;
        p->size = p->rd = 0;
        idx = npipe;
        while(idx > 0 && !pipe[idx-1].nopen) idx--;
        if(idx != npipe) {
            npipe = idx;
            pipe = (pipe_t*)realloc(pipe, npipe * sizeof(pipe_t));
        }
    }
}

/**
 * adatok írása csővezetékbe
 */
public bool_t pipe_write(uint64_t idx, virt_t ptr, size_t size)
{
    taskctx_t *tc = NULL;
    pipe_t *p;
    uint64_t j;

    /* paraméterek ellenőrzése */
    if(idx >= npipe) {
        seterr(EPIPE);
        return false;
    }
    p = &pipe[idx];
#if DEBUG
    if(_debug&DBG_FILEIO)
        dbg_printf("FS: file write(pipe %d, %x[%d])\n", idx, ptr, size);
#endif
    p->wrtime = time();
    /* ha van blokkolt olvasó, aki írásra vár */
    if(p->reader) {
        /* ne használd a taskctx_get hívást, mert az hozzáad egy őjat ha nem lenne */
        tc = taskctx[p->reader & 0xFF];
        if(tc) {
            while(tc) { if(tc->pid == p->reader) break; tc = tc->next; }
            if(tc && EVT_FUNC(tc->msg.evt) != SYS_fread) tc = NULL;
        }
    }
    /* ha van blokkolt olvasó, aki legalább annyit akar olvasni, mint amit írunk, és a buffer üres,
     * nem volt korábban direkt írás sem, akkor teljes egészében kihagyjuk a bufferelést */
    if(tc && tc->msg.data.ptr.size >= size && !p->data && !p->size && p->size != -1U) {
#if DEBUG
        if(_debug&DBG_FILEIO)
            dbg_printf("FS: file read(pipe %d, %x[%d], direct)\n", idx, tc->msg.data.ptr.ptr, size);
#endif
        /* adatok másolása közvetlenül az olvasó bufferébe */
        if((int64_t)tc->msg.data.ptr.ptr < 0)
            memcpy((void*)tc->msg.data.ptr.ptr, (void*)ptr, size);
        else
            tskcpy(p->reader, (void*)tc->msg.data.ptr.ptr, (void*)ptr, size);
        /* jelezzük, hogy direktben másoltunk (csak egyszer tehetjük meg, mielőtt az olvasó olvasna) */
        p->size = -1U;
        goto ack;
    }
    if(p->size == -1U) p->size = 0;
    /* adatok hozzáfűzése a bufferhez */
    p->data = realloc(p->data, p->size+size);
    if(!p->data) return false;
    memcpy(p->data+p->size, (void*)ptr, size);
    p->size += size;
    /* ha van blokkolt olvasó, akkor felébresztjük. ez akkor is meghívódik, ha többet írunk, mint amit olvasna */
    if(tc) {
        size = pipe_read(idx, (virt_t)tc->msg.data.ptr.ptr, tc->msg.data.ptr.size, tc->pid);
        /* válasz küldése az olvasónak */
ack:    j = errno();
        mq_send(0, 0, j, 0, 0, 0, EVT_DEST(tc->pid) | EVT_ack, tc->msg.serial);
        /* felfüggesztett üzenet törlése */
        tc->msg.evt = 0;
    }
    return true;
}

/**
 * olvasás a csővezetékből
 */
public size_t pipe_read(uint64_t idx, virt_t ptr, size_t s, pid_t pid)
{
    pipe_t *p;

    if(idx >= npipe) {
        seterr(EPIPE);
        return 0;
    }
    p = &pipe[idx];
    /* csővezeték zárolása olvasáshoz */
    lockid(pid, &p->reader);
    if(p->reader != pid) {
        seterr(EACCES);
        return 0;
    }
    if(p->size == -1U) p->size = 0;
    if(p->rd>p->size) p->rd = p->size;
    /* biztonság kedvéért ellenőrizzük, hogy mindent kiolvastunk-e a bufferből, nem szabadna */
    if(p->data && p->rd == p->size) {
        free(p->data);
        p->size = p->rd = 0;
        p->data = NULL;
    }
#if DEBUG
    if(_debug&DBG_FILEIO)
        dbg_printf("FS: file read(pipe %d, %x[%d], pipebufsize %d)\n", idx, ptr, s, p->size-p->rd);
#endif
    p->rdtime = time();
    /* ha van mit kiolvasni a bufferből */
    if(p->data) {
        if(p->rd+s < p->size) {
            /* kevesebbet olvasunk, mint ami a bufferben van */
            if((int64_t)ptr < 0)
                memcpy((void*)ptr, p->data+p->rd, s);
            else
                tskcpy(pid, (void*)ptr, p->data+p->rd, s);
            p->rd += s;
        } else {
            /* többet szeretnénk olvasni, odaadjuk ami van és felszabadítjuk a buffert */
            s = p->size-p->rd;
            if((int64_t)ptr < 0)
                memcpy((void*)ptr, p->data+p->rd, s);
            else
                tskcpy(pid, (void*)ptr, p->data+p->rd, s);
            free(p->data);
            p->size = p->rd = 0;
            p->data = NULL;
        }
        /* zárolás feloldása */
        p->reader = 0;
        return s;
    }
    /* üres a buffer, blokkoljuk az olvasót */
    ackdelayed = true;
    return 0;
}

/**
 * stat_t visszaadása csővezetékhez
 */
stat_t *pipe_stat(uint64_t idx, mode_t mode)
{
    if(idx >= npipe) return NULL;
    memzero(&st,sizeof(stat_t));
    st.st_dev = pipe[idx].fid;
    st.st_ino = -1U;
    st.st_mode = (mode&S_IMODE) | S_IFIFO | (mode & OF_MODE_TTY? S_IFTTY : 0) | (mode & O_WRITE? O_APPEND : 0);
    memcpy(st.st_type, "appl", 4);
    memcpy(st.st_mime, "pipe", 4);
    st.st_nlink = pipe[idx].nopen;
    st.st_owner = pipe[idx].owner;
    st.st_blksize = 1;
    st.st_blocks = pipe[idx].size;
    st.st_atime = pipe[idx].rdtime;
    st.st_mtime = pipe[idx].wrtime;
    st.st_ctime = max(st.st_atime, st.st_mtime);
    return &st;
}

#if DEBUG
/**
 * csővezetékek dumpolása debuggoláshoz
 */
void pipe_dump()
{
    uint64_t i;

    dbg_printf("Pipes %d:\n",npipe);
    for(i = 0; i < npipe; i++) {
        dbg_printf("%3d. %3d %d %x '%U'\n", i, pipe[i].nopen, pipe[i].size, pipe[i].reader, &pipe[i].owner);
    }
}
#endif
