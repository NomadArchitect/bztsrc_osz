/*
 * core/fs.c
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
 * @subsystem fájlrendszer
 * @brief FS taszk előtti, FS/Z initrd-t kezelő funkciók (csak olvasás)
 */

#include <core.h>
#include <osZ/fsZ.h>

uint64_t sys_fid;
uint64_t fs_size, fstab_size = 0;
virt_t fs_initrd = 0, fstab = 0;

/**
 * visszaadja az initrd-n lévő fájl tartalmának kezdőcímét és fs_size változóban a méretét
 */
void *fs_locate(char *fn)
{
    /* az indulás korai szakaszában fizikai címeket használunk, aztán a kernelterület */
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)(fs_initrd ? fs_initrd : bootboot.initrd_ptr);
    FSZ_Inode *in;
    FSZ_DirEnt *ent;
    FSZ_DirEntHeader *hdr;
    size_t ss;
    uint64_t sec, i;
    uint8_t *inl;
    int j,r = 64;
    char *s,*e;

    /* paraméterek ellenőrzése */
    fs_size = 0;
    if(fn==NULL || fn[0]==0 || fn[1]==0 || memcmp(sb->magic,FSZ_MAGIC,4)){
        seterr(EINVAL);
        return NULL;
    }
    /* FS/Z */
    ss = 1<<(sb->logsec+11);
pathagain:
    in = (FSZ_Inode *)((virt_t)sb+sb->rootdirfid*ss);
    /* Inode lekérése a hierarchiából */
    if(!memcmp(fn, "root:", 5)) fn += 5;
    if(*fn == '/') fn++;
    s = e = fn;
    i = 0;
#if DEBUG
    if(debug&DBG_FILEIO)
        kprintf("fs_locate(initrd=%x,fn=%s)\n", sb, fn);
#endif
again:
    while(*e!='/'&&*e!=':'&&*e!=';'&&*e!='#'&&*e>=32){e++;}
    if(*e=='/'){e++;}
    if(!memcmp(in->magic,FSZ_IN_MAGIC,4)){
        /* inode tartalmazza az adatot? */
        inl = sb->flags&FSZ_SB_FLAG_BIGINODE? in->data.big.inlinedata : in->data.small.inlinedata;
        if(!memcmp(inl,FSZ_DIR_MAGIC,4)){
            ent = (FSZ_DirEnt *)(inl);
        } else if(!memcmp((void *)((virt_t)sb+in->sec*ss),FSZ_DIR_MAGIC,4)){
            /* hivatkozott szektor "betöltése" */
            ent = (FSZ_DirEnt *)((virt_t)sb+in->sec*ss);
        } else if(!memcmp(in->filetype, FSZ_FILETYPE_SYMLINK, 4)) {
            /* szimbólikus hivatkozás */
            fn = (char*)(inl);
            if(--r) goto pathagain; else goto complex;
        } else if(sys_fid && !memcmp(in->filetype, FSZ_FILETYPE_UNION, 4)) {
            /* könyvtár uniót nem tudjuk itt még rendesen lekezelni, de megnézzük a sys/-ben, a bin/ és lib/ esetén működni fog */
            in = (FSZ_Inode*)((virt_t)sb+sys_fid*ss);
            s = e = fn;
            if(--r) goto again; else goto complex;
        } else {
            goto complex;
        }
        /* fejléc átugrása */
        hdr=(FSZ_DirEntHeader *)ent; ent++;
        /* könyvtárbejegyzések ellenőrzése */
        j=hdr->numentries;
        while(j-->0){
            if(!sys_fid && !memcmp(ent->name, "sys/", 4)) sys_fid = ent->fid;
            if(!memcmp(ent->name,s,e-s)) {
                if(*(e-1)!='/') {
                    i=ent->fid;
                    break;
                } else {
                    s=e;
                    in=(FSZ_Inode *)((virt_t)sb+ent->fid*ss);
                    goto again;
                }
            }
            ent++;
        }
    } else {
        i=0;
    }
    /* ha van inode-unk, a tartalmát betöltjük */
    if(i!=0) {
        /* fid -> inode ptr -> data ptr */
        FSZ_Inode *in=(FSZ_Inode *)((virt_t)sb+i*ss);
        if(!memcmp(in->magic,FSZ_IN_MAGIC,4)){
            /* fájl verziók */
            sec = 0;
            if(*e==';') {
                e++; if(*e=='-') e++;
                switch(*e) {
                    case '1': fs_size = in->version1.size; j = in->version1.flags; sec = in->version1.sec; break;
                    case '2': fs_size = in->version2.size; j = in->version2.flags; sec = in->version2.sec; break;
                    case '3': fs_size = in->version3.size; j = in->version3.flags; sec = in->version3.sec; break;
                    case '4': fs_size = in->version4.size; j = in->version4.flags; sec = in->version4.sec; break;
                    case '5': fs_size = in->version5.size; j = in->version5.flags; sec = in->version5.sec; break;
                }
                e++;
            }
            /* fájl offszetet nem támogatjuk */
            if(*e=='#') goto complex;
            /* adatok megkeresése */
            if(!sec) { fs_size = in->size; j = in->flags; sec = in->sec; }
            switch(FSZ_FLAG_TRANSLATION(j)) {
                case FSZ_IN_FLAG_INLINE:
                    /* beágyazott adat */
                    return (sb->flags&FSZ_SB_FLAG_BIGINODE? (void*)&in->data.big.inlinedata : (void*)&in->data.small.inlinedata);

                case FSZ_IN_FLAG_SECLIST:
                case FSZ_IN_FLAG_SDINLINE:
                    /* beágyazott szektorlista vagy szektorkönyvtár */
                    return (void*)((virt_t)sb + ss * *((sb->flags&FSZ_SB_FLAG_BIGINODE?
                        (uint64_t*)&in->data.big.inlinedata : (uint64_t*)&in->data.small.inlinedata)));

                case FSZ_IN_FLAG_DIRECT:
                    /* direkt adat */
                    return (void*)((virt_t)sb + in->sec * ss);

                /* szektorkönyvtár (csak egy szintet támogatunk, és nem lehet benne lyuk) */
                case FSZ_IN_FLAG_SECLIST0:
                case FSZ_IN_FLAG_SD:
                    return (void*)((virt_t)sb +
                        (unsigned int)(((FSZ_SectorList *)((virt_t)sb+sec*ss))->sec) * ss);

                default:
                    /* ezt használjuk a nyelvi fájl betöltésére is */
complex:            kpanic("%s", txt[0]==NULL||!txt[0][0]? "Unsupported FS/Z complexity in initrd" : TXT_initrdtoocomplex);
            }
        }
    }
    seterr(ENOENT);
    return NULL;
}
