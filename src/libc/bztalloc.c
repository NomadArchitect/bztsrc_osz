/*
 * libc/bztalloc.c
 *
 * Copyright (C) 2016 bzt (bztsrc@gitlab)
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
 * @subsystem libc
 * @brief Memória allokátor és deallokátor
 */

#ifdef _OSZ_CORE_
# include <arch.h>
#else
# include <osZ.h>
#endif
#include "bztalloc.h"

/***
 * Tervezési szempontok:
 * 1. allokátor adat és alkalmazás adat nem keveredhet
 * 2. a szabad blokkokat vissza kell adni az OS-nek, amint lehet, nincs pazarlás
 * 3. ugyanaz az algoritmus taszk lokális, kernel és megosztott memóriára (ezért a jelzőbitezés)
 *
 * Memória kiosztás (példa):
 * 0. blokk: allocmap_t mutatók
 * 1. blokk: első allocmap_t
 * 2. blokk: csomag adatok (több is, ha a quantum kicsi)
 * 3. blokk: második allocmap_t terület
 * 4. blokk: másik adag csomag
 * 5. blokk: első nagy quantumú csomag
 * 6. blokk: második nagy quantumú csomag
 * ...stb.
 *
 * Kis egységek (quantum>=8 && quantum<PAGESIZE):
 *  ptr egy címigazított lapra mutat, a bitmap minden bitje quantumnyi adatot jelent, OS memória foglalás chunksize-ban történik
 *
 * Közepes egységek (quantum>=PAGESIZE && quantum<ALLOCSIZE):
 *  ptr egy címigazított lapra mutat, a bitmap minden bitje quantumnyi adatot jelent, OS memória foglalás quantum-ban történik
 *
 * Nagy egységek (quantum>=ALLOCSIZE):
 *  ptr egy címigazított nagy blokkra mutat, size[0] tartalmazza az allocsize méretű blokkok számát
 *
 * OS/Z-ben az aréna címe DYN_ADDRESS (taszk TLS), SDYN_ADDRESS (megosztott) vagy CDYN_ADDRESS (core globális).
 * Az arena paraméter egy *allocmap_t tömb, az első elem tartalmazza a méretét és a jelzőbitet.
 */

/**
 * memória felszabadítása és ha lehetséges, RAM felszabadítása a rendszer számára. alacsony szint, használd inkább a free()-t
 */
public void bzt_free(uint64_t *arena, void *ptr)
{
    uint i,j,k,cnt=0;
    uint64_t l,sh;
    allocmap_t *am;
    chunkmap_t *ocm=NULL; /*régi csomag térkép (chunk map) */

    if(ptr==NULL)
        return;

    lockacquire(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_free(%x, %x)\n", arena, ptr);
#endif
    /* végigmegyünk az allocmap-okon */
    for(i=1;i<MAXARENA && cnt<numallocmaps(arena);i++) {
        if(arena[i]==0)
            continue;
        cnt++;
        /* keresünk egy csomagot ehhez a quantum mérethez */
        am=(allocmap_t*)arena[i];
        for(j=0;j<am->numchunks;j++) {
            if( am->chunk[j].ptr <= ptr &&
                (uint8_t*)ptr < (uint8_t*)am->chunk[j].ptr +
                (am->chunk[j].quantum<ALLOCSIZE? chunksize(am->chunk[j].quantum) : am->chunk[j].map[0])) {
                    ocm=&am->chunk[j];
                    l=0;
                    if(ocm->quantum<ALLOCSIZE) {
                        sh=((uint8_t*)ptr-(uint8_t*)ocm->ptr)/ocm->quantum;
                        ocm->map[sh>>6] &= ~(1UL<<(sh%64));
                        /* utolsó foglalás volt ebben a csomagban? */
                        l=0; for(k=0;k<BITMAPSIZE;k++) l+=ocm->map[k];
                        if(ocm->quantum<__PAGESIZE) {
                            if(l==0)
#ifndef _OSZ_CORE_
                                munmap(ocm->ptr,chunksize(ocm->quantum));
#else
                                vmm_free(0,(virt_t)ocm->ptr,chunksize(ocm->quantum));
#endif
                        } else {
#ifndef _OSZ_CORE_
                            munmap(ptr,ocm->quantum);
#else
                            vmm_free(0,(virt_t)ptr,ocm->quantum);
#endif
                        }
                    } else {
#ifndef _OSZ_CORE_
                        munmap(ocm->ptr,ocm->map[0]);
#else
                        vmm_free(0,(virt_t)ocm->ptr,ocm->map[0]);
#endif
                    }
                    if(l==0) {
                        /* van még csomag? */
                        if(j+1<am->numchunks) {
                            memcpy( &((allocmap_t*)arena[i])->chunk[j],
                                    &((allocmap_t*)arena[i])->chunk[j+1],
                                    (((allocmap_t*)arena[i])->numchunks-j-1)*sizeof(chunkmap_t));
                        }
                        ((allocmap_t*)arena[i])->numchunks--;
                        /* utolsó csomag ebben az allocmap struktúrában? */
                        if(((allocmap_t*)arena[i])->numchunks==0) {
#ifndef _OSZ_CORE_
                            munmap((void *)arena[i], ALLOCSIZE);
#else
                            vmm_free(0, (virt_t)arena[i], ALLOCSIZE);
#endif
                            arena[i]=0;
                            arena[0]--;
                        }
                    }
                    lockrelease(63,arena);
                    return;
            }
        }
    }
    lockrelease(63,arena);
    seterr(EFAULT);
}

/**
 * címigazított memória lefoglalása egy arénában. alacsony szint, használd inkább a malloc(), calloc(), realloc() hívást
 */
public __attribute__((malloc)) void *bzt_alloc(uint64_t *arena, size_t a, void *ptr, size_t s, int flag)
{
    uint64_t q,sh,sf;
    uint i,j,k,l,fc=0,cnt=0,ocmi=0,ocmj=0;
    uint8_t *fp, *sp, *lp, *end;
    allocmap_t *am;       /* foglalási térkép (allocation map) */
    chunkmap_t *ncm=NULL; /* új csomag térkép (new chunk map) */
    chunkmap_t *ocm=NULL; /* régi csomag térkép (old chunk map) */
    uint64_t maxchunks = (ALLOCSIZE-8)/sizeof(chunkmap_t);
    int prot =
#ifdef _OSZ_CORE_
        flag == MAP_CORE? PG_CORE_RWNOCACHE : (flag & MAP_SHARED? PG_USER_RWNOCACHE : PG_USER_RW);
    int aprot = flag & MAP_SHARED? PG_USER_RWNOCACHE : PG_USER_RW;
    /* megosztott memóriában minimum 4k-t kell foglalni, hogy tudjuk kezelni a hozzáféréseket */
    if(arena == (uint64_t*)SDYN_ADDRESS && flag != MAP_CORE && s > 0 && s < __PAGESIZE) s = __PAGESIZE;
#else
        PROT_READ | PROT_WRITE;
#endif
    flag |= MAP_FIXED | MAP_ANONYMOUS;

    if(s==0) {
        bzt_free(arena,ptr);
        return NULL;
    }

    /* quantum kiszámítása */
    for(q=8;q<s;q<<=1);
    /* a legkissebb címigazítás maga a quantum */
    if(a<q) a=q;
    /* shiftelt értékek */
    sh=(uint64_t)-1; sf=a/q;

    lockacquire(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc(%x, %x, %x, %d) quantum %d\n", arena, a, ptr, s, q);
#endif
    /* legeslegelső hívás, amikor még nincs egyetlen allocmap se */
    if(!numallocmaps(arena)) {
        if(
        /* ez elvileg ALLOCSIZE-ot foglalna, de arra hagyatkozunk, hogy a pagefault majd ad új lapot */
#ifndef _OSZ_CORE_
          mmap((void*)((uint64_t)arena+ARENASIZE), __PAGESIZE, prot, flag&~MAP_SPARSE, -1, 0)
#else
          vmm_map(0, (virt_t)arena+ARENASIZE, 0, __PAGESIZE, aprot)
#endif
          ==MAP_FAILED) {
            seterr(ENOMEM);
            lockrelease(63,arena);
            return NULL;
        }
        arena[0]++;
        arena[1]=(uint64_t)arena+ARENASIZE;
    }
    fp=(void*)((uint64_t)arena+ARENASIZE+ALLOCSIZE);
    sp=lp=NULL;
    /* végigmegyünk az allocmap-okon */
    for(i=1;i<MAXARENA && cnt<numallocmaps(arena);i++) {
        if(arena[i]==0)
            continue;
        cnt++;
        /* keresünk egy csomagot ehhez a quantum mérethez */
        am=(allocmap_t*)arena[i];
        if(((uint8_t*)am)+ALLOCSIZE > fp)
            fp = ((uint8_t*)am)+ALLOCSIZE;
        if(lp==NULL || ((uint8_t*)am)+ALLOCSIZE < lp)
            lp = ((uint8_t*)am)+ALLOCSIZE;
        if(fc==0 && am->numchunks<maxchunks)
            fc=i;
        for(j=0;j<am->numchunks;j++) {
            end=(uint8_t*)am->chunk[j].ptr +
                (am->chunk[j].quantum<ALLOCSIZE? chunksize(am->chunk[j].quantum) : am->chunk[j].map[0]);
            if(am->chunk[j].quantum==q && q<ALLOCSIZE && ((uint64_t)(am->chunk[j].ptr)&(a-1))==0) {
                for(k=0;k<BITMAPSIZE*64;k+=sf) {
                    if(((am->chunk[j].map[k>>6])&(1UL<<(k%64)))==0) { ncm=&am->chunk[j]; sh=k; break; }
                }
            }
            if(ptr!=NULL && am->chunk[j].ptr <= ptr && (uint8_t*)ptr < end) {
                    ocm=&am->chunk[j];
                    ocmi=i; ocmj=j;
            }
            if(sp==NULL || (uint8_t*)am->chunk[j].ptr < sp)
                sp=am->chunk[j].ptr;
            if(end > fp)
                fp=end;
        }
    }
    /* ugyanaz, mint eddig, az új méret ugyanabba a quantumba esik */
    if(q<ALLOCSIZE && ptr!=NULL && ncm!=NULL && ncm==ocm) {
        lockrelease(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc ret1 %x\n", ptr);
#endif
        return ptr;
    }
    /* ha van elég nagy, címigazított terület az allocmap-ek után, azt használjuk */
    if(lp!=NULL && sp!=NULL &&
        lp+((s+ALLOCSIZE-1)/ALLOCSIZE)*ALLOCSIZE < sp &&
        ((uint64_t)lp&(a-1))==0)
            fp=lp;
    if(ncm==NULL) {
        /* első allocmap nem teli csomagokkal */
        if(fc==0) {
            /* van elég hely egy új allocmap-nek? */
            if(i>=numallocmaps(arena) ||
#ifndef _OSZ_CORE_
                mmap(fp, ALLOCSIZE, prot, flag&~MAP_SPARSE, -1, 0)
#else
                vmm_map(0, (virt_t)fp, 0, ALLOCSIZE, aprot)
#endif
                ==MAP_FAILED) {
                    seterr(ENOMEM);
                    lockrelease(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc ret2 %x\n", ptr);
#endif
                    return NULL;
            }
            arena[0]++;
            /* átléptük épp a laphatárt? */
            if((arena[0]*sizeof(void*)/__PAGESIZE)!=((arena[0]+1)*sizeof(void*)/__PAGESIZE)) {
                if(
#ifndef _OSZ_CORE_
                mmap(&arena[arena[0]], __PAGESIZE, prot, flag&~MAP_SPARSE, -1, 0)
#else
                vmm_map(0, (virt_t)&arena[arena[0]], 0, __PAGESIZE, aprot)
#endif
                ==MAP_FAILED) {
                    seterr(ENOMEM);
                    lockrelease(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc ret3 %x\n", ptr);
#endif
                    return NULL;
                }
            }
            arena[i]=(uint64_t)fp;
            fp+=ALLOCSIZE;
            fc=i;
        }
        /* új csomag hozzáadása */
        am=(allocmap_t*)arena[fc];
        if(q>=ALLOCSIZE)
            fp=(void*)((((uint64_t)fp+a-1)/a)*a);
        i=am->numchunks++;
        am->chunk[i].quantum = q;
        am->chunk[i].ptr = fp;
        for(k=0;k<BITMAPSIZE;k++)
            am->chunk[i].map[k]=0;
        /* kis egységméretű quantumok foglalása */
        if((flag&MAP_SPARSE)==0 && q<__PAGESIZE &&
#ifndef _OSZ_CORE_
        mmap(fp, chunksize(q), prot, flag, -1, 0)
#else
        vmm_map(0, (virt_t)fp, 0, chunksize(q), prot)
#endif
        ==MAP_FAILED) {
            seterr(ENOMEM);
            lockrelease(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc ret4 %x\n", ptr);
#endif
            return NULL;
        }
        ncm=&am->chunk[i];
        sh=0;
    }
    if(q<ALLOCSIZE) {
        if(sh!=(uint64_t)-1) {
            ncm->map[sh>>6] |= (1UL<<(sh%64));
            fp=(uint8_t*)ncm->ptr + sh*ncm->quantum;
            /* közepes egységméretű quantumok foglalása */
            if((flag&MAP_SPARSE)==0 && q>=__PAGESIZE &&
#ifndef _OSZ_CORE_
            mmap(fp, ncm->quantum, prot, flag, -1, 0)
#else
            vmm_map(0, (virt_t)fp, 0, ncm->quantum, prot)
#endif
            ==MAP_FAILED)
                fp=NULL;
        } else
            fp=NULL;
    } else {
        s=((s+ALLOCSIZE-1)/ALLOCSIZE)*ALLOCSIZE;
        /* nagy egységméretű quantumok foglalása */
        if((flag&MAP_SPARSE)==0 &&
#ifndef _OSZ_CORE_
        mmap(fp, s, prot, flag, -1, 0)
#else
        vmm_map(0, (virt_t)fp, 0, s, prot)
#endif
        ==MAP_FAILED)
            fp=NULL;
        else
            ncm->map[0]=s;
    }
    if(fp==NULL){
        seterr(ENOMEM);
        lockrelease(63,arena);
#if DEBUG
        if(_debug&DBG_MALLOC)
            dbg_printf("bzt_alloc ret5 %x\n", ptr);
#endif
        return NULL;
    }
    if((flag&MAP_SPARSE)==0 && ocm==NULL)
        memset(fp, 0, ncm->quantum);
    /* régi memória felszabadítása */
    if(ocm!=NULL) {
        if((flag&MAP_SPARSE)==0){
            memcpy(fp,ptr,ocm->quantum);
            if(ncm->quantum>ocm->quantum)
                memset(fp+ocm->quantum, 0, ncm->quantum-ocm->quantum);
        }
        l=0;
        if(ocm->quantum<ALLOCSIZE) {
            sh=((uint8_t*)ptr-(uint8_t*)ocm->ptr)/ocm->quantum;
            ocm->map[sh>>6] &= ~(1UL<<(sh%64));
            /* utolsó foglalás volt ebben a csomagban? */
            l=0; for(k=0;k<BITMAPSIZE;k++) l+=ocm->map[k];
            if(ocm->quantum<__PAGESIZE) {
                if(l==0)
#ifndef _OSZ_CORE_
                    munmap(ocm->ptr,chunksize(ocm->quantum));
#else
                    vmm_free(0,(virt_t)ocm->ptr,chunksize(ocm->quantum));
#endif
            } else {
#ifndef _OSZ_CORE_
                munmap(ptr,ocm->quantum);
#else
                vmm_free(0,(virt_t)ptr,ocm->quantum);
#endif
            }
        } else {
#ifndef _OSZ_CORE_
            munmap(ocm->ptr,ocm->map[0]);
#else
            vmm_free(0,(virt_t)ocm->ptr,ocm->map[0]);
#endif
        }
        if(l==0) {
            /* nem az utolsó csomag? */
            if(ocmj+1<am->numchunks) {
                memcpy( &((allocmap_t*)arena[ocmi])->chunk[ocmj],
                        &((allocmap_t*)arena[ocmi])->chunk[ocmj+1],
                        (((allocmap_t*)arena[ocmi])->numchunks-ocmj-1)*sizeof(chunkmap_t));
            }
            ((allocmap_t*)arena[ocmi])->numchunks--;
            /* utolsó csomag ebben az allocmap struktúrában? */
            if(((allocmap_t*)arena[ocmi])->numchunks==0) {
#ifndef _OSZ_CORE_
                munmap((void *)arena[ocmi], ALLOCSIZE);
#else
                vmm_free(0, (virt_t)arena[ocmi], ALLOCSIZE);
#endif
                arena[ocmi]=0;
                arena[0]--;
            }
        }
    }
    lockrelease(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc ret6 %x\n", fp);
#endif
    return fp;
}

#if DEBUG
/**
 * foglaltsági memóriatérkép listázása, debuggolási célra (csak ha DEBUG = 1)
 */
public void dbg_bztdump(uint64_t *arena)
{
    uint i,j,k,l,n,o,cnt;
    int mask[]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
    char *bmap=".123456789ABCDEF";
    uint16_t *m;
    dbg_printf("-----Arena %x, %s%s, ",
        arena,(uint64_t)arena==(uint64_t)DYN_ADDRESS?"lts":(
#ifdef CDYN_ADDRESS
        (uint64_t)arena==(uint64_t)CDYN_ADDRESS?"core":
#endif
        "shared"),(arena[0] & (1UL<<63))!=0?" LOCKED":"");
    dbg_printf("allocmaps: %d/%d, chunks: %d, %d units\n",
        numallocmaps(arena), MAXARENA, (ALLOCSIZE-8)/sizeof(chunkmap_t), BITMAPSIZE*64);
    if(numallocmaps(arena) > MAXARENA) return;
    o=1; cnt=0;
    for(i=1;i<MAXARENA && cnt<numallocmaps(arena);i++) {
        if(arena[i]==0)
            continue;
        cnt++;
        dbg_printf("  --allocmap %d %x, chunks: %d--\n",i,arena[i],((allocmap_t*)arena[i])->numchunks);
        for(j=0;j<((allocmap_t*)arena[i])->numchunks;j++) {
            dbg_printf("%3d. %6d %x - %x ",o++,
                ((allocmap_t*)arena[i])->chunk[j].quantum,
                ((allocmap_t*)arena[i])->chunk[j].ptr,
                (uint8_t*)(((allocmap_t*)arena[i])->chunk[j].ptr)+
                    (((allocmap_t*)arena[i])->chunk[j].quantum<ALLOCSIZE?
                        chunksize(((allocmap_t*)arena[i])->chunk[j].quantum) :
                        ((allocmap_t*)arena[i])->chunk[j].map[0]));
            if(((allocmap_t*)arena[i])->chunk[j].quantum<ALLOCSIZE) {
                m=(uint16_t*)&((allocmap_t*)arena[i])->chunk[j].map;
                for(k=0;k<BITMAPSIZE*4;k++) {
                    l=0; for(n=0;n<16;n++) { if(*m & mask[n]) l++; }
                    dbg_printf("%c",bmap[l]);
                    m++;
                }
                dbg_printf("%6dk\n",chunksize(((allocmap_t*)arena[i])->chunk[j].quantum)/1024);
            } else {
                dbg_printf("(no bitmap) %8dM\n",((allocmap_t*)arena[i])->chunk[j].map[0]/1024/1024);
            }
        }
    }
    dbg_printf("\n");
}
#endif
