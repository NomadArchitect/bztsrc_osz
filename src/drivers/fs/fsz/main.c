/*
 * drivers/fs/fsz/main.c
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
 * @brief FS/Z fájlrendszer meghajtó
 */

#include <osZ.h>
#include <vfs.h>
#include <osZ/fsZ.h>

#define MAXINDIRECT 8

void resizefs(fid_t fd);
uint64_t allocateblock(fid_t fd);

uint64_t lastsec = 0;   /* utolsó beolvasott szektor */

/**
 * magic bájtok ellenőrzése a detektáláshoz
 */
bool_t detect(fid_t fd, void *blk)
{
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)blk;
    bool_t ret = !memcmp(sb->magic, FSZ_MAGIC,4) && !memcmp(sb->magic2,FSZ_MAGIC,4) &&
        sb->checksum == crc32a_calc((char*)&sb->magic, 508) ? true : false;
    if(ret) {
        /* blokkméret meghatározása */
        fcb[fd].data.device.blksize = 1<<(sb->logsec+11);
        /* ellenőrizzük, hogy a fájlrendszerméret helyes-e */
        if(fcb[fd].data.device.filesize != sb->numsec*fcb[fd].data.device.blksize) {
            resizefs(fd);
        }
    }
    return ret;
}

/**
 * fájl keresése az eszközön, amin ez a fájlrendszer található
 */
uint8_t locate(fid_t fd, ino_t parent, locate_t *loc)
{
    FSZ_SuperBlock *sb;
    FSZ_Inode *in;
    FSZ_DirEnt *dirent;
    uint8_t *inlinedata;
    ino_t lsn, *sdblk[MAXINDIRECT];
    uint16_t sdptr[MAXINDIRECT];
    uint64_t bp, sdmax, i, l;
    locate_t lo;
    char *c, *e;

    /* paraméterek ellenőrzése */
    if(!loc || !loc->path || fd >= nfcb || !fcb[fd].abspath) return NOTFOUND;
    /* szuperblokk beolvasása */
    sb = (FSZ_SuperBlock *)readblock(fd, 0);
    if(!sb) return NOBLOCK;

    /* gyökérkönyvtártól indulunk, ha nincs szülő megadva */
    lsn = parent==0? sb->rootdirfid : parent;
    sdmax = fcb[fd].data.device.blksize/16;

nextdir:
    for(e = loc->path; *e != '/' && !PATHEND(*e); e++);
    if(*e == '/') e++;
    /* fel könyvtár lekezelése */
    if(e-loc->path >= 2 && loc->path[0] == '.' && loc->path[1] == '.' && loc->path[2] != '.') {
        pathstack_t *last = pathpop();
        if(last == NULL) {
            return UPDIR;
        }
        lsn = last->inode;
        loc->path = last->path;
        strcpy(last->path, e);
        goto nextdir;
    }
    /* inode beolvasása */
    if(!lsn) return NOBLOCK;
    in = (FSZ_Inode*)readblock(fd, lsn);
    if(!in) return NOBLOCK;
    if(memcmp(in->magic, FSZ_IN_MAGIC, 4)) return FSERROR;
    inlinedata = sb->flags & FSZ_SB_FLAG_BIGINODE? in->data.big.inlinedata : in->data.small.inlinedata;
/*dbg_printf("  %04d: %x '%s' %x\n", lsn, in, in->filetype, FSZ_FLAG_TRANSLATION(in->flags));*/

    /* beágyazott adatot tartalmazó típusok */
    if(!memcmp(in->filetype, FSZ_FILETYPE_SYMLINK, 4)) {
        loc->fileblk = inlinedata;
        return SYMINPATH;
    }

    /* elérési út vége */
    if(PATHEND(*loc->path)) {
        loc->inode = lsn;
        loc->filesize = in->size;
        if(!memcmp(in->filetype, FSZ_FILETYPE_DIR, 4))   loc->type = FCB_TYPE_REG_DIR; else
        if(!memcmp(in->filetype, FSZ_FILETYPE_UNION, 4)) loc->type = FCB_TYPE_UNION; else
            loc->type = FCB_TYPE_REG_FILE;
        return SUCCESS;
    }

    if(!memcmp(in->filetype, FSZ_FILETYPE_UNION, 4)) {
        loc->fileblk = inlinedata;
        return UNIONINPATH;
    }

    /* inode adatok betöltése */
    memzero(sdptr, MAXINDIRECT * sizeof(uint16_t));
    memzero(sdblk, MAXINDIRECT * sizeof(void*));
    l = FSZ_FLAG_TRANSLATION(in->flags);
    switch(l) {
        case FSZ_IN_FLAG_INLINE:
            loc->fileblk = inlinedata;
            break;

        case FSZ_IN_FLAG_SECLIST:
        case FSZ_IN_FLAG_SECLIST0:
        case FSZ_IN_FLAG_SECLIST1:
        case FSZ_IN_FLAG_SECLIST2:
            /* TEENDŐ: extentek */
            break;

        case FSZ_IN_FLAG_DIRECT:
            if(!in->sec) return NOBLOCK;
            loc->fileblk = readblock(fd, in->sec);
            if(!loc->fileblk) return NOBLOCK;
            break;

        case FSZ_IN_FLAG_SDINLINE:
            l = 1;
            sdblk[0] = (ino_t*)inlinedata;
            goto sd;
            break;

        default:
            /* első szintű sd beolvasása */
            if(!in->sec) return NOBLOCK;
            sdblk[0] = readblock(fd, in->sec);
sd:         if(!sdblk[0]) return NOBLOCK;
            if(l >= MAXINDIRECT) return FSERROR;
            /* minden sd beolvasása az indirektségi lánc mentén */
            for(i = 1; i < l; i++) {
                if(!sdblk[i-1][0]) return NOBLOCK;
                sdblk[i] = readblock(fd, sdblk[i-1][0]);
                if(!sdblk[i]) return NOBLOCK;
            }
            /* az utolsó indirektségi sd által mutatott blokk beolvasása */
            if(!sdblk[i-1][0]) return NOBLOCK;
            loc->fileblk = readblock(fd, sdblk[i-1][0]);
            if(!loc->fileblk) return NOBLOCK;
            break;
    }

    if(!memcmp(in->filetype, FSZ_FILETYPE_REG_APP, 4)) {
        return FILEINPATH;
    } else

    if(!memcmp(in->filetype, FSZ_FILETYPE_DIR, 4)) {
        dirent = (FSZ_DirEnt *)(loc->fileblk);
        if(memcmp(((FSZ_DirEntHeader *)dirent)->magic, FSZ_DIR_MAGIC, 4)) return FSERROR;
        i = ((FSZ_DirEntHeader *)dirent)->numentries;
        bp = 0;
        while(i) {
            dirent++; bp += sizeof(FSZ_DirEnt);
            if(bp >= fcb[fd].data.device.blksize) {
                /* JAV: blokk végére értünk, új blokkot kell betölteni sdblk-ból és átállítani rá a dirent mutatót */
                sdmax--; sdmax++;
            }
            if(!dirent->name[0]) break;
            /* a '...' joker az elérési útban */
            if(e-loc->path >= 3 && loc->path[0] == '.' && loc->path[1] == '.' && loc->path[2] == '.') {
                /* csak alkönyvtárakba nézünk be */
                if(dirent->name[strlen((char*)dirent->name)-1] == '/') {
                    lo.path = e;
                    lo.creat = false;
                    if(locate(fd, dirent->fid, &lo) == SUCCESS) {
                        c = strdup(e);
                        if(!c) return NOBLOCK;
                        strcpy(loc->path, (char*)dirent->name);
                        strcat(loc->path, c);
                        free(c);
                        loc->fileblk = lo.fileblk;
                        loc->filesize = lo.filesize;
                        loc->inode = lo.inode;
                        loc->type = lo.type;
                        return SUCCESS;
                    }
                }
            } else
            /* egyszerű fájlnév egyezés */
            if(!memcmp(dirent->name, loc->path, e-loc->path)) {
                pathpush(lsn, loc->path);
                lsn = dirent->fid;
                loc->path = e;
                goto nextdir;
            }
            i--;
        }
    }
    /* TEENDŐ: ha loc->creat, akkor létre kell hozni a könyvtárakat az elérési út mentén */
    if(loc->creat) {
/*dbg_printf("locate create inode lsn %d '%s'\n",lsn,loc->path);*/
        loc->inode = lsn;
        i = allocateblock(fd);
/*dbg_printf("allocated block %d\n",i);*/
        if(i == -1U) return NOSPACE;
    }
    return NOTFOUND;
}

/**
 * fájlrendszer átméretezése
 */
void resizefs(fid_t fd)
{
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)readblock(fd, 0);
/*dbg_printf("FS/Z resizefs, currently %d should be %d\n",sb->numsec, fcb[fd].data.device.filesize/fcb[fd].data.device.blksize);*/
    sb->numsec = fcb[fd].data.device.filesize/fcb[fd].data.device.blksize;
    writeblock(fd, 0, (void*)sb, BLKPRIO_CRIT);
}

/**
 * fájlrendszer konzisztencia ellenőrzése
 */
bool_t checkfs(fid_t fd)
{
    /* paraméter ellenőrzése */
    if(fsck.dev!=fd) return false;
/*    FSZ_SuperBlock *sb=(FSZ_SuperBlock *)readblock(fd, 0);
      dbg_printf("FS/Z fsck\n");*/
    return true;
}

/**
 * stat_t lekérése inodeból
 */
bool_t stat(fid_t fd, ino_t file, stat_t *st)
{
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)readblock(fd, 0);
    FSZ_Inode *in = (FSZ_Inode*)readblock(fd, file);

/*dbg_printf("fsz stat storage %d file %d\n",fd,file);*/
    lastlink = NULL;
    /* paraméterek ellenőrzése */
    if(fd >= nfcb || !fcb[fd].data.device.blksize || !in || memcmp(in->magic, FSZ_IN_MAGIC, 4)) return false;
    memcpy(st->st_type, in->filetype, 4);
    if(in->filetype[3] != ':') memcpy(st->st_mime,in->mimetype,44);
    if(!memcmp(in->filetype, FSZ_FILETYPE_SYMLINK, 4)) {
        lastlink = (char*)(sb->flags & FSZ_SB_FLAG_BIGINODE? in->data.big.inlinedata : in->data.small.inlinedata);
        st->st_mode &= ~S_IFMT;
        st->st_mode |= S_IFREG;
        st->st_mode |= S_IFLNK;
    }
    if(!memcmp(in->filetype, FSZ_FILETYPE_UNION, 4)) {
        st->st_mode &= ~S_IFMT;
        st->st_mode |= S_IFDIR;
        st->st_mode |= S_IFUNI;
    }
    if(!memcmp(in->filetype, FSZ_FILETYPE_DIR, 4)) {
        st->st_mode &= ~S_IFMT;
        st->st_mode |= S_IFDIR;
    }
    st->st_nlink = in->numlinks;
    memcpy((void*)&st->st_owner, (void*)&in->owner, sizeof(uuid_t));
    st->st_size = in->size;
    st->st_blocks = in->numblocks;
    st->st_atime = in->accessdate;
    st->st_ctime = in->changedate;
    st->st_mtime = in->modifydate;
    return true;
}

/**
 * blokk beolvasása fájlból. visszatérési értékei:
 *   NULL && ackdelayed: blokk nincs a gyorsítótárban, újra kell próbálni, amikor az eszközmeghajtó betöltötte
 *   *s==0: fájl vége
 *   NULL && *s!=0: üres hézag, nullák
 *   !NULL && *s!=0: érvényes fájl adat
 */
void *read(fid_t fd, ino_t file, uint ver, fpos_t offs, size_t *s)
{
    FSZ_SuperBlock *sb = (FSZ_SuperBlock*)readblock(fd, 0);
    FSZ_Inode *in = (FSZ_Inode*)readblock(fd, file);
    void *blk;
    uint8_t *inlinedata;
    uint64_t i, j, k, l, bs, sec;
    size_t sz;

/*dbg_printf("fsz read storage %d file %d offs %d s %d\n",fd,file,offs,*s);*/
    /* paraméterek ellenőrzése */
    if(!file || fd >= nfcb || !fcb[fd].data.device.blksize || !sb || !in || memcmp(in->magic, FSZ_IN_MAGIC, 4))
        goto read_err;

    lastsec = file;
    bs = fcb[fd].data.device.blksize;

    switch(ver) {
        case 0: sz = in->size; sec = in->sec; l = FSZ_FLAG_TRANSLATION(in->flags); break;
        case 1: sz = in->version1.size; sec = in->version1.sec; l = FSZ_FLAG_TRANSLATION(in->version1.flags); break;
        case 2: sz = in->version2.size; sec = in->version2.sec; l = FSZ_FLAG_TRANSLATION(in->version2.flags); break;
        case 3: sz = in->version3.size; sec = in->version3.sec; l = FSZ_FLAG_TRANSLATION(in->version3.flags); break;
        case 4: sz = in->version4.size; sec = in->version4.sec; l = FSZ_FLAG_TRANSLATION(in->version4.flags); break;
        case 5: sz = in->version5.size; sec = in->version5.sec; l = FSZ_FLAG_TRANSLATION(in->version5.flags); break;
        default: goto read_err;
    }
    /* fájl vége ellenőrzés */
    if(offs > sz) goto read_err;
    if(offs + *s > sz) {
        *s = sz - offs;
        if(!*s) return NULL;
    }
    inlinedata = sb->flags & FSZ_SB_FLAG_BIGINODE? in->data.big.inlinedata : in->data.small.inlinedata;

    /* egyszerű eset, inodeba ágyazott adat */
    if(l == FSZ_IN_FLAG_INLINE ||
        !memcmp(in->filetype, FSZ_FILETYPE_SYMLINK, 4) || !memcmp(in->filetype, FSZ_FILETYPE_UNION, 4))
        return (void*)(inlinedata + offs);
    /* beágyazott sd, egy szint */
    if(l == FSZ_IN_FLAG_SDINLINE) {
        i = offs/bs;
        i = ((uint64_t*)inlinedata)[i<<1];
        if(!i) return NULL;
        blk = readblock(fd, sec);
        if(!blk) goto read_err;
        return blk;
    }
    /* direktben hivatkozott adatblokk */
    if(!sec) return NULL;
    blk = readblock(fd, sec);
    if(!blk) goto read_err;
    lastsec = sec;
    if(l == FSZ_IN_FLAG_DIRECT)
        return (void*)((uint8_t*)blk + offs);
    /* egyébként szektorkönyvtárak bejárása */
    /* első blokk? */
    if(offs < bs) {
        if(*s > bs - offs)
            *s = bs - offs;
        offs = 0;
    } else {
        offs /= bs;
        offs *= bs;
        if(offs+*s > sz) {
            *s = sz - offs;
            if(!*s) return NULL;
        }
    }
    /* kiszámoljuk, mennyi adatot fed le egy sd bejegyzés a legmagasabb szinten */
    for(j = 1, i = 0; i < l; i++) j *= bs;
/*dbg_printf(" indirect %d j %d offs %d s %d\n",l,j,offs,*s);*/
    /* sd szintek bejárása */
    for(k = 0; k < l; k++) {
        /* melyik sd bejegyzés tartozik ehhez az offset-hez a k.-adik szinten */
        i = (offs/j);
/*dbg_printf(" level %d: sd %d blk %x ",k,i,blk);*/
        i = ((uint64_t*)blk)[i<<1];
/*dbg_printf(" next %d\n",i);*/
        /* üres hézag a fájlban? */
        if(!i) return NULL;
        blk = readblock(fd,i);
        if(!blk) goto read_err;
        lastsec = i;
        j /= bs;
    }
/*dbg_printf(" end %d s %d\n", offs%bs, *s);*/
    /* oké, megvan az adatblokk */
    return (void*)((uint8_t*)blk + (offs % bs));

read_err:
    *s = 0;
    return NULL;
}

/**
 * bufferben lévő könyvtárbejegyzés átalakítása dirent-é, visszaadja a beolvasott méretet
 */
size_t getdirent(void *buf, fpos_t offs, dirent_t *dirent)
{
    size_t s = 0;
    FSZ_DirEnt *ent;

    if(!buf) return 0;
    /* fejléc átugrása */
    if(!offs) {
        if(memcmp(((FSZ_DirEntHeader*)buf)->magic, FSZ_DIR_MAGIC, 4))
            return 0;
        else {
            buf = (char*)buf + sizeof(FSZ_DirEntHeader);
            s = sizeof(FSZ_DirEntHeader);
        }
    }
    ent = (FSZ_DirEnt*)buf;
    if(!ent->fid || !ent->name[0]) return 0;
    dirent->d_ino = ent->fid;
    dirent->d_len = ent->length;
    memcpy(&dirent->d_name, ent->name, strlen((char*)ent->name));
    s += sizeof(FSZ_DirEnt);
    return s;
}

uint64_t allocateblock(fid_t fd)
{
    uint64_t i = -1;
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)readblock(fd, 0);
    FSZ_Inode *in;
    FSZ_SectorList *sl, *ptr;
    size_t s, bs = fcb[fd].data.device.blksize;

    /* nincs a gyorsítótárban? */
    if(!sb) return -1;
    /* van szabad szegmens listánk? */
    if(sb->freesecfid) {
        in = (FSZ_Inode*)readblock(fd,sb->freesecfid);
        if(!in || memcmp(in->magic, FSZ_IN_MAGIC, 4)) return -1;
        sl = sb->flags & FSZ_SB_FLAG_BIGINODE?
            (FSZ_SectorList*)&in->data.big.inlinedata : (FSZ_SectorList*)&in->data.small.inlinedata;
        s = bs - (sb->flags & FSZ_SB_FLAG_BIGINODE? 2048 : 1024);
        if(s > 0 && in->size > 0) {
            for(ptr = sl; (int64_t)s >= 0; s -= sizeof(FSZ_SectorList)) {
                if(ptr->numsec > 0) {
                    i = ptr->sec;
                    /* megbizonyosodunk róla, hogy a visszaadandó blokk a gyorsítótárban legyen */
                    if(!readblock(fd, i)) return -1;
                    in->size -= bs;
                    ptr->sec++;
                    ptr->numsec--;
                    if(!ptr->numsec)
                        memcpy(ptr,ptr+1,s-sizeof(FSZ_SectorList));
                    writeblock(fd, lastsec, (void*)((char*)sl - ((uint64_t)sl % bs)), BLKPRIO_CRIT);
                    return i;
                }
                ptr++;
            }
        }
    }
    /* van szabad helyünk a fájlrendszer végén? */
    if(sb->freesec < sb->numsec) {
        i = sb->freesec;
        /* megbizonyosodunk róla, hogy a visszaadandó blokk a gyorsítótárban legyen */
        if(!readblock(fd, i)) return -1;
        sb->freesec++;
        writeblock(fd, 0, (void*)sb, BLKPRIO_CRIT);
    }
    return i;
}

fsdrv_t drv = {
    "fsz",
    "FS/Z",
    detect,
    locate,
    resizefs,
    checkfs,
    stat,
    getdirent,
    read,
    NULL,
    NULL,
    NULL
};
void drv_init()
{
dbg_printf("fsz1\n");
    fsdrv_reg(&drv);
dbg_printf("fsz2\n");
}
