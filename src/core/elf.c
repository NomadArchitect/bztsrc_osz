/*
 * core/elf.c
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
 * @subsystem futtathatók
 * @brief ELF betöltő és értelmező
 */

#include <arch.h>                   /* szükségünk van az ARCH_ELFEM-re */
#include <osZ/elf.h>

/* a betöltött ELF-ek listát a taszk memóriájában tároljuk */
#define realloc(p,s) bzt_alloc((void*)DYN_ADDRESS,8,p,s,MAP_PRIVATE)
#define free(p) bzt_free((void*)DYN_ADDRESS,p)

typedef struct {                    /* relokálandó címek listája */
    virt_t offs;
    char *sym;
} rela_t;

uint64_t nelfcache = 0;             /* gyorsítótár elemeinek száma */
elfcache_t *elfcache = NULL;        /* gyorsítótár */

/* ezeket a változókat azért emeljük ki, hogy feleslegesen ne terheljük a rekurzív hívás vermét */
tcb_t *elftcb = (tcb_t*)BUF_ADDRESS;
elfctx_t *elfctx = (elfctx_t*)(BUF_ADDRESS + __PAGESIZE);

char *startsym = "_start";

/**
 * betölt egy ELF objektumot a háttértárolóról a gyorsítótárba
 */
elfcache_t *elf_getfile(char *fn)
{
    uint i;
    elfcache_t *elf = NULL;
    uint8_t *data;

    /* paraméterek ellenőrzése */
    if(fn==NULL || fn[0]==0 || fn[1]==0){
        seterr(EINVAL);
        return NULL;
    }
    if(!memcmp(fn,"root:",5)) fn += 5;
    if(*fn=='/') fn++;

    /* megnézzük, benne van-e már a gyorsítótárban */
    for(i=0; i<nelfcache; i++) {
        /* üres hely a gyorsítótárban? */
        if(!elfcache[i].fn && !elf) {
            elf = &elfcache[i];
            continue;
        }
        /* egyezik? */
        if(!strcmp(elfcache[i].fn, fn))
            return &elfcache[i];
    }

    /* ha nincs üres hely, hozzáadunk egy újat */
    if(!elf) {
        elfcache = (elfcache_t*)ksrealloc(elfcache, (nelfcache+1) * sizeof(elfcache_t));
        if(elfcache) {
            /* mivel az elfcache bejegyzés címe változhat realloc-kor, ezért indexeket tárolunk */
            elf = &elfcache[nelfcache];
            elf->idx = nelfcache++;
        }
    }
    if(elf) {
        elf->fn = ksalloc(strlen(fn)+1);
        if(elf->fn) {
            strncpy(elf->fn, fn, 65536);
            if(runlevel < RUNLVL_NORM) {
                /* ha még az indulás korai szakaszában vagyunk és nincs még FS szolgáltatás */
                data = fs_locate(fn);
                if(data && fs_size>0) {
                    /* fizikai címmé alakítjuk */
                    data -= fs_initrd-bootboot.initrd_ptr;
                    /* mivel initrd-ről töltjük, memóriában az egész fájl, könnyen feltöltjük a tömböt */
                    elf->numpages = (fs_size+__PAGESIZE-1)/__PAGESIZE;
                    elf->data = ksalloc(elf->numpages * sizeof(phy_t));
                    if(elf->data) {
                        for(i=0;i<elf->numpages;i++,data+=__PAGESIZE)
                            elf->data[i] = (phy_t)data;
                        return elf;
                    }
                }
            } else {
                /* már átkapcsoltunk taszk lokális címterekre, és lehet, hogy az ELF
                 * egy felcsatolt adathordozón található, ezért az FS taszkot használjuk */
            }
        }
    }
    /* a coreerrno megfelelő hibakódot tartalmaz */
    return NULL;
}

/**
 * kiszedi az ELF objektumokat az aktuális címtérből, és ha kell, a gyorsítótárból is
 */
void elf_unload(elfbin_t *elfbin)
{
    elfcache_t *elf;
    elfbin_t *eb = elfbin;
    uint i, l = nelfcache;

    /* paraméterek ellenőrzése */
    if(!elfbin || !nelfcache) return;

#if DEBUG
    if(debug&DBG_ELF)
        kprintf("elf_unload(eb=%x)\n", elfbin);
#endif

    /* csökkentjük a hivatkozások számát minden betöltött elf binárison */
    for(; eb && eb->addr; eb++) {
#if DEBUG
        if(eb->elfcacheidx >= nelfcache) kpanic("elfbin->elfcacheidx %d >= nelfcache %d", eb->elfcacheidx, nelfcache);
#else
        if(eb->elfcacheidx < nelfcache)
#endif
            elfcache[eb->elfcacheidx].numref--;
    }
    free(elfbin);

    /* megnézzük, vannak-e törölhető bejegyzések */
    for(i=0, elf=elfcache; i<nelfcache; i++, elf++) {
        if(!elf->numref) {
            ksfree(elf->fn);   elf->fn = NULL;
            ksfree(elf->data); elf->data = NULL;
            elf->numpages = 0;
            if(i < l) l = i;
        } else
            l = nelfcache;
    }

    /* a gyorsítótár végéről felszabadítjuk az üres bejegyzéseket */
    if(l <= nelfcache-1) {
        nelfcache = l+1;
        elfcache = (elfcache_t*)ksrealloc(elfcache, nelfcache * sizeof(elfcache_t));
    }
}

/**
 * betölt egy ELF objektumot a gyorsítótárból az aktuális címtérbe, rekurzívan a függvénykönyvtáraival együtt
 */
bool_t elf_load(char *fn, phy_t extrabuf, size_t extrasiz, uint8_t reent)
{
    elfcache_t *elfc;
    Elf64_Ehdr *elf;
    Elf64_Phdr *phdr_c, *phdr_d, *phdr_l;
    Elf64_Dyn *dyn;
    uint i, n;
    virt_t v;
    /* rekurziós szinttől függő változók, ezeket muszáj a veremben tartani */
    Elf64_Dyn *d = NULL;
    char *strtable = NULL;

    /* paraméterek ellenőrzése */
    /* a maximális szintnél figyelembe kell venni, hogy kicsi a verem (1k), és minden rekurzió kb 100 bájtot jelent */
    if(!fn || !fn[0] || reent>3) {
        seterr(EINVAL);
        return false;
    }
    elfc = elf_getfile(fn);
    if(((ccb_t*)LDYN_ccb)->core_errno || elfc==NULL || !elfc->numpages || !elfc->data || !elfc->data[0]) {
        if(!((ccb_t*)LDYN_ccb)->core_errno) seterr(EINVAL);
        return false;
    }
    if(elfctx->nextaddr < TEXT_ADDRESS) elfctx->nextaddr = TEXT_ADDRESS;

    /* betöltöttük már ezt az ELF-et ebbe a címtérbe? */
    for(i=0; i<elfctx->nelfbin; i++) {
        if(elfc->idx == elfctx->elfbin[i].elfcacheidx) {
            if(i>0 && elfctx->nelfbin>i+1) {
                /* átrendezzük a hívási listát, hogy a hivatkozott kerüljön előre */
                v = elfctx->elfbin[i].entry;
                for(; i<elfctx->nelfbin-1; i++)
                    elfctx->elfbin[i].entry = elfctx->elfbin[i+1].entry;
                elfctx->elfbin[i].entry = v;
            }
            return true;
        }
    }

#if DEBUG
    if(debug&DBG_ELF)
        kprintf("elf_load(fn=%s) pid %x\n", fn, elftcb->pid);
#endif
    /* hozzáadjuk a címtér binárisai listához. Ezt a taszk memóriájában foglaljuk le */
    elfctx->elfbin = (elfbin_t*)realloc(elfctx->elfbin, (elfctx->nelfbin+2) * sizeof(elfbin_t));
    if(!elfctx->elfbin) return false;

    /* leképezzük a futtatható első két lapját */
    vmm_page(0, LDYN_tmpmap2, elfc->data[0], PG_CORE_RWNOCACHE | PG_PAGE);
    vmm_page(0, LDYN_tmpmap3, elfc->data[0], PG_CORE_RWNOCACHE | PG_PAGE);
    elf = (Elf64_Ehdr *)(LDYN_tmpmap2 + (virt_t)(elfc->data[0]&((1<<__PAGEBITS)-1)));

    /* megnézzük, hogy a futtatható tényleg erre az architektúrára fordított ELF-e */
    if(elf==NULL || (memcmp(elf->e_ident,ELFMAG,SELFMAG) && memcmp(elf->e_ident,"OS/Z",4)) ||
        elf->e_ident[EI_CLASS]!=ELFCLASS64 || elf->e_ident[EI_DATA]!=ELFDATA2LSB ||
        elf->e_ident[EI_OSABI]!=ELFOSABI_OSZ || elf->e_machine!=ARCH_ELFEM || elf->e_phnum<3 ||
        elf->e_phoff + elf->e_phnum*elf->e_phentsize > 2*__PAGESIZE) goto elferr;

    /* növeljük a véletlenszámgenerátor entrópiáját */
    srand[((elfctx->nextaddr>>__PAGEBITS)+0)&3] *= 16807;
    srand[((elfctx->nextaddr>>__PAGEBITS)+1)&3] ^= elfc->data[0];
    srand[((elfctx->nextaddr>>__PAGEBITS)+2)&3] *= 16807;
    srand[((elfctx->nextaddr>>__PAGEBITS)+3)&3] ^= elf->e_shoff<<16;
    kentropy();

    /* szegmensek betöltése és ellenőrzése */
    phdr_c = (Elf64_Phdr *)((uint8_t *)elf + elf->e_phoff);
    phdr_d = (Elf64_Phdr *)((uint8_t *)elf + elf->e_phoff + 1*elf->e_phentsize);
    phdr_l = (Elf64_Phdr *)((uint8_t *)elf + elf->e_phoff + 2*elf->e_phentsize);
    if( /* kódszegmens, betöltendő, olvasható, futtatható, de nem írható */
        phdr_c->p_type!=PT_LOAD || phdr_c->p_offset!=0 || (phdr_c->p_flags&PF_W)!=0 ||
        /* adatszegmens, lapcímhatárra igazított, olvasható, írható, de nem futtatható */
        phdr_d->p_type!=PT_LOAD || (phdr_d->p_vaddr&(__PAGESIZE-1))!=0 || (phdr_d->p_flags&PF_X)!=0 ||
        /* dinamikus linkelési adatok */
        phdr_l->p_type!=PT_DYNAMIC ||
        /* ez nagyon valószínűtlen, hibás linkelő kell hozzá, de azért ellenőrizzük */
        (uint64_t)elf->e_entry > (uint64_t)phdr_c->p_filesz
        ) {
elferr:
            if(runlevel < RUNLVL_NORM)
                syslog(LOG_ERR, "Not a valid ELF: %s", fn);
            seterr(ENOEXEC);
            return false;
    }

    /* kicsit megcifrázzuk a címteret (Address Space Layout Randomization), véletlenszerűen átugrunk pár (0-7) lapnyi címet */
    if(flags & FLAG_ASLR)
        elfctx->nextaddr += __PAGESIZE*((srand[elfc->idx&3]>>(((virt_t)elfc>>__PAGEBITS)%55))&7);
    /* mivel az elfcache bejegyzés címe változhat realloc-kor, ezért indexeket tárolunk */
    elfctx->elfbin[elfctx->nelfbin].elfcacheidx = elfc->idx;
    elfctx->elfbin[elfctx->nelfbin].addr = elfctx->nextaddr;

    /* betöltjük a kódszegmenst */
#if DEBUG
    if(debug&DBG_ELF)
        kprintf("  elf text %x, %d pages (%d bytes)\n", elfctx->nextaddr,
            ((phdr_c->p_filesz + __PAGESIZE-1)>>__PAGEBITS), phdr_c->p_filesz);
#endif
    if(!vmm_maptext(elftcb, BUF_ADDRESS + elfctx->nextaddr, elfc->data, phdr_c->p_filesz)) goto elferr;

    /* átállítjuk a mutatókat az átmeneti helyről a buffer területére */
    elf = (Elf64_Ehdr *)(BUF_ADDRESS + elfctx->nextaddr);
    dyn = (Elf64_Dyn *)((uint8_t *)elf + phdr_l->p_offset);
    elfctx->elfbin[elfctx->nelfbin].entry = elfctx->nextaddr + (virt_t)elf->e_entry;
    /* betöltés könyvelése */
    elfc->numref++;
    elfctx->nextaddr += ((phdr_c->p_filesz + __PAGESIZE-1)>>__PAGEBITS)<<__PAGEBITS;

    /* lefoglaljuk és bemásoljuk az adatszegmenst */
    i = ((phdr_d->p_offset + __PAGESIZE-1)>>__PAGEBITS);
    n = phdr_d->p_filesz;
#if DEBUG
    if(debug&DBG_ELF)
        kprintf("  elf data %x, %d pages (%d bytes) bss %x - %x\n", elfctx->nextaddr,((phdr_d->p_memsz + __PAGESIZE-1)>>__PAGEBITS),
            n, elfctx->nextaddr + phdr_d->p_filesz, elfctx->nextaddr + phdr_d->p_memsz);
#endif
    if(!vmm_map(elftcb, BUF_ADDRESS + elfctx->nextaddr, 0, phdr_d->p_memsz, PG_USER_RW|PG_PAGE)) goto elferr;
    for(v = BUF_ADDRESS + elfctx->nextaddr; n>0; i++,v+=__PAGESIZE) {
        /* mivel fizikai címeink vannak, ideiglenesen belapozzuk a tmpmap2-be */
        vmm_page(0, (virt_t)LDYN_tmpmap2, elfc->data[i], PG_CORE_RONOCACHE|PG_PAGE);
        if(n >= __PAGESIZE) {
            memcpy((void*)v, (void*)LDYN_tmpmap2, __PAGESIZE);
            n -= __PAGESIZE;
        } else {
            memcpy((void*)v, (void*)LDYN_tmpmap2, n);
            break;
        }
    }
    elfctx->nextaddr += ((phdr_d->p_memsz + __PAGESIZE-1)>>__PAGEBITS)<<__PAGEBITS;
    elfctx->elfbin[elfctx->nelfbin].end = elfctx->nextaddr;
    elfctx->nelfbin++;

    /* felhasználói szintről nem elérhető extra bufferek leképezése az adatterület után */
    if(extrabuf && extrasiz && !elfctx->extra_ptr) {
#if DEBUG
    if(debug&DBG_ELF)
        kprintf("  elf extr %x, @%x %d bytes\n", elfctx->nextaddr, extrabuf, extrasiz);
#endif
        elfctx->extra_ptr = elfctx->nextaddr;
        extrasiz = ((extrasiz+__PAGESIZE-1)>>__PAGEBITS)<<__PAGEBITS;
        vmm_map(elftcb, BUF_ADDRESS + elfctx->nextaddr, extrabuf, extrasiz, PG_USER_RO|PG_PAGE);
        elfctx->nextaddr += extrasiz;
    }

    /* az egészet megismételjük rekurzívan a megosztott függvénykönyvtárakra */
    d = dyn;
    /* először tudnunk kell a sztringtábla címét, lehet később van a táblában, mint az első lib */
    while(d->d_tag != DT_NULL) {
        if(d->d_tag == DT_STRTAB) { strtable = (char *)elf + d->d_un.d_ptr; break; }
        d++;
    }
    if(strtable) {
        d = dyn;
        while(d->d_tag != DT_NULL) {
            if(d->d_tag == DT_NEEDED) {
                /* ha nem abszolút elérési út, akkor elétesszük a /lib/-et */
                i = 0; n = strlen(strtable + d->d_un.d_ptr)+1;
                if(strtable[d->d_un.d_ptr]!='/') { i=5; memcpy(elfctx->libpath, "/lib/", 5); }
                if(n > sizeof(elfctx->libpath)-i-1) n = sizeof(elfctx->libpath)-i-1;
                memcpy(elfctx->libpath+i, strtable + d->d_un.d_ptr, n);
                /* meghívjuk önmagunkat */
                elf_load(elfctx->libpath, 0, 0, reent+1);
            }
            d++;
        }
    }

    return true;
}

/**
 * aktuális címtérben összelinkeli az ELF-eket (run-time linker)
 */
bool_t elf_rtlink()
{
    tcb_arch_t *tcba = (tcb_arch_t*)elftcb;
    Elf64_Ehdr *elf;
    Elf64_Phdr *phdr_l;
    Elf64_Dyn *d;
    Elf64_Sym *sym = NULL, *s;
    Elf64_Rela *rela = NULL, *jmprel = NULL;
    uint64_t nrelas = 0;
    rela_t *relas = NULL, *rel;
    virt_t vo, vm, *stck, args;
    char *strtable = NULL;
    uint i, j, e, strsz=0, syment=0, relasz, relaent, jmpsz;

    /* paraméterek ellenőrzése */
    if(elftcb->magic != OSZ_TCB_MAGICH || !elfctx->nelfbin || !elfctx->elfbin) {
err:    if(relas) free(relas);
        seterr(ERTEXEC);
        return false;
    }
    seterr(SUCCESS);

#if DEBUG
    if(debug&DBG_ELF)
        kprintf("elf_rtlink(nelf=%d) pid %x\n", elfctx->nelfbin, elftcb->pid);
#endif

    /* a betöltött ELF-ek száma végleges, helyet csinálunk a hívásukhoz a veremben */
    args = tcba->sp + 8;
    stck = (virt_t*)(BUF_ADDRESS + tcba->sp - 16);
    j = (tcba->sp >> __PAGEBITS) & 511;
    tcba->sp -= (elfctx->nelfbin)*16;
    e = (tcba->sp >> __PAGEBITS) & 511;
    vo = BUF_ADDRESS + j*__PAGESIZE;
    for(i = j; i < e; i++, vo += __PAGESIZE)
        vmm_map(elftcb, vo, 0, __PAGESIZE, PG_USER_RW|PG_PAGE);

    /*** relokálandó hivatkozások összegyűjtése ***/
    for(e=0; e<elfctx->nelfbin; e++) {
        elf = (Elf64_Ehdr *)(BUF_ADDRESS + elfctx->elfbin[e].addr);
        phdr_l = (Elf64_Phdr *)((uint8_t *)elf + elf->e_phoff + 2*elf->e_phentsize);
        vm = BUF_ADDRESS + elfctx->elfbin[e].end;
        if(!e) {
            if(elf->e_type != ET_EXEC) goto err;
            /* ha ez az első ELF, akkor beállítjuk a futtatási címet */
            tcba->pc = elfctx->elfbin[0].entry;
        } else {
            if(elf->e_type != ET_DYN) goto err;
            /* egyébként lerakjuk a verembe (ahonnan majd a crt0 veszi ki) */
            stck -= 2;
            *stck = elfctx->elfbin[e].entry;
        }
        d = (Elf64_Dyn *)((uint8_t *)elf + phdr_l->p_offset);
        syment = relaent = relasz = jmpsz = 0; strtable = NULL; sym = NULL;
        if((uint)phdr_l->p_memsz) {
            syment = sizeof(Elf64_Sym);
            relaent = sizeof(Elf64_Rela);
            rela = jmprel = NULL;
            while(d->d_tag != DT_NULL) {
                switch(d->d_tag) {
                    case DT_STRTAB:   strtable = (char *)elf + d->d_un.d_ptr; break;
                    case DT_SYMTAB:   sym = (Elf64_Sym *)((uint8_t *)elf + d->d_un.d_ptr); break;
                    case DT_SYMENT:   syment = d->d_un.d_val; break;
                    case DT_RELAENT:  relaent = d->d_un.d_val; break;
                    case DT_RELASZ:   relasz = d->d_un.d_val; break;
                    case DT_PLTRELSZ: jmpsz = d->d_un.d_val; break;
                    case DT_RELA:     rela = (Elf64_Rela *)((char *)elf + d->d_un.d_ptr); break;
                    case DT_JMPREL:   jmprel = (Elf64_Rela *)((char *)elf + d->d_un.d_ptr); break;
                    default:          break;
                }
                d++;
            }
        }
        /* Megpróbáljuk megtalálni a relokációs táblát. Ez borzalom, mert a linkelők nem jól csinálják. A gcc például x86_64
         * esetén egy táblába rakja (rela == jmprel és relasz <= jmpsz), míg AArch64 esetén külön (rela + relasz == jmprel).
         * Pedig ugyanaz a linkelő szkript... Sajnos itt a Section Headers beli .rela.dyn szekciót nem használhatjuk, pedig
         * az mindig jól van feltöltve, minden architektúrán, mind ld és lld esetén. Az SCO ELF spec tök egyértelmű, hogy a
         * DT_RELA szekcióban mindig benne kéne lennie az összes relokációt tartalmazó teljes táblának. Olyan viszont sehol
         * sincs leírva, hogy a DT_JMPREL-ben kéne benne lennie bármiféle adat relokációknak...

            AArch64: rela + relasz == jmprel && rela != jmprel
            Dynamic section at offset 0x40b0 contains 15 entries:
              Tag        Type                         Name/Value
             0x0000000000000002 (PLTRELSZ)           336 (bytes)
             0x0000000000000017 (JMPREL)             0x38b0
             0x0000000000000007 (RELA)               0x3778
             0x0000000000000008 (RELASZ)             312 (bytes)
            Relocation section '.rela.dyn' at offset 0x3778 contains 27 entries:

            x86_64: rela + relasz != jmprel && rela == jmprel
            Dynamic section at offset 0x20a8 contains 14 entries:
              Tag        Type                         Name/Value
             0x0000000000000002 (PLTRELSZ)           432 (bytes)
             0x0000000000000017 (JMPREL)             0x16d0
             0x0000000000000007 (RELA)               0x16d0
             0x0000000000000008 (RELASZ)             96 (bytes)
            Relocation section '.rela.dyn' at offset 0x16d0 contains 18 entries:
        */
        if(!rela && jmprel && jmpsz) { rela = jmprel; relasz = jmpsz; }
        vo = (virt_t)rela + relasz;
        if(jmprel && jmprel < rela) rela = jmprel;                      /* két kezdő cím közül a kissebb (ha nem nulla) */
        if((virt_t)jmprel + jmpsz > vo) vo = (virt_t)jmprel + jmpsz;    /* két vége cím közül a nagyobb */
        relasz = vo - (virt_t)rela;                                     /* méret = vége - kezdő cím */
        /* ha nincs relokációs tábla */
        if(!relaent || !relasz || !rela) continue;
#if DEBUG
        if(debug&DBG_RTIMPORT)
            kprintf("  import %x (%d bytes):\n", (virt_t)rela - BUF_ADDRESS, relasz);
#endif

        /* GOT adatok és PLT bejegyzések */
        for(i = 0; i < relasz / relaent; i++) {
            vo = (virt_t)elf + (int64_t)rela->r_offset;
            /* ez nagyon valószínűtlen, hibás linkelő kell hozzá, de azért ellenőrizzük */
            if(vo < (virt_t)elf || vo > vm) {
#if DEBUG
                kpanic("pid %x: out of bounds rela %x", elftcb->pid, vo - BUF_ADDRESS);
#else
                syslog(LOG_ERR, "pid %x: out of bounds rela %x", elftcb->pid, vo - BUF_ADDRESS);
                goto err;
#endif
            }

            s = (Elf64_Sym *)((uint8_t *)sym + ELF64_R_SYM(rela->r_info) * syment);
            if(syment && sym != NULL && s != NULL && strtable != NULL && *(strtable + s->st_name)!=0) {
                srand[(s->st_name)&3] *= 16807;
                /* relokálandó cím, amit nem tudunk még feloldani */
                relas = (rela_t*)realloc(relas, (((nrelas+128)>>7)<<7) * sizeof(rela_t));
                rel = &relas[nrelas++];
                /* elmentjük a pozicíót és a szimbólum nevét */
                rel->offs = vo;
                rel->sym = strtable + s->st_name;
#if DEBUG
                if(debug&DBG_RTIMPORT)
                    kprintf("    %x %c %s\n", vo - BUF_ADDRESS,
                        ELF64_R_TYPE(rela->r_info)==R_X86_64_JUMP_SLOT ||
                        ELF64_R_TYPE(rela->r_info)==R_AARCH64_JUMP_SLOT
                        ?'T':'D', strtable + s->st_name);
#endif
            } else {
                srand[(vo>>4)&3] *= 16807;
                /* relokálandó cím, amit fel tudunk oldani */
#if DEBUG
                if(debug&DBG_RTIMPORT)
                    kprintf("    %x %c base+%x\n", vo - BUF_ADDRESS,
                        ELF64_R_TYPE(rela->r_info)==R_X86_64_JUMP_SLOT ||
                        ELF64_R_TYPE(rela->r_info)==R_AARCH64_JUMP_SLOT
                        ?'T':'D', *((uint64_t*)vo));
#endif
                *((virt_t*)vo) = (virt_t)elfctx->elfbin[e].addr + (virt_t)rela->r_addend;
            }

            rela = (Elf64_Rela *)((uint8_t *)rela + relaent);
        }
    }

    /*** relokálható szimbólumok kigyűjtése ***/
    for(e=0; e<elfctx->nelfbin; e++) {
        elf = (Elf64_Ehdr *)(BUF_ADDRESS + elfctx->elfbin[e].addr);
        phdr_l = (Elf64_Phdr *)((uint8_t *)elf + elf->e_phoff + 2*elf->e_phentsize);
        /* a továbbiakban nincs szükségünk ezekre az adatokra, és mivel felhasználói területen tárolódnak,
         * inkább kitöröljük öket a biztonság kedvéért. Egyedül az elfcacheidx marad meg, ami meg nem kritikus */
#if !DEBUG
        elfctx->elfbin[e].addr = elfctx->elfbin[e].end = elfctx->elfbin[e].entry = (virt_t)-1;
#endif
        /* dinamikus táblából kikeressük a dinamikus szimbólumok szekciót */
        d = (Elf64_Dyn *)((uint8_t *)elf + phdr_l->p_offset);
        strsz = syment = 0; strtable = NULL; sym = NULL;
        if((uint)phdr_l->p_memsz) {
            syment = sizeof(Elf64_Sym);
            while(d->d_tag != DT_NULL) {
                switch(d->d_tag) {
                    case DT_STRTAB: strtable = (char *)elf + d->d_un.d_ptr; break;
                    case DT_STRSZ:  strsz = d->d_un.d_val; break;
                    case DT_SYMTAB: sym = (Elf64_Sym *)((uint8_t *)elf + d->d_un.d_ptr); break;
                    case DT_SYMENT: syment = d->d_un.d_val; break;
                    default:        break;
                }
                d++;
            }
        }
        /* ha nincs szimbólum tábla */
        if(!strtable || !strsz || !sym || !syment) continue;
#if DEBUG
        if(debug&DBG_RTEXPORT) kprintf("  export %x (%d bytes):\n", (virt_t)sym - BUF_ADDRESS, strtable-(char*)sym);
#endif
        for(s = sym, i = 0; i<(strtable-(char*)sym)/syment && s->st_name < strsz; i++) {
            /* ebben a fájlban van definiálva? */
            if(s->st_value && strtable[s->st_name]) {
                srand[(s->st_name)&3] ^= s->st_value;
                vo = (virt_t)elf + (virt_t)s->st_value;
                /* core adatok és paraméterek exportálása felhasználói szintre */
                if(ELF64_ST_TYPE(s->st_info)==STT_OBJECT &&
                   ELF64_ST_BIND(s->st_info)==STB_GLOBAL &&
                   ELF64_ST_VISIBILITY(s->st_other)==STV_DEFAULT) {
                    /* csak eszközmeghajtók és rendszerszolgáltatások számára elérhető adatok */
                    if(elftcb->priority == PRI_DRV || elftcb->priority == PRI_SRV) {
                        j = bootboot.size - ((virt_t)&bootboot.mmap - (virt_t)&bootboot);
                        if(!memcmp(strtable + s->st_name, "_memorymap", 11) && s->st_size >= j)
                            { memcpy((void*)vo, &bootboot.mmap, j); } else
                        if(!memcmp(strtable + s->st_name, "_platform", 10) && s->st_size >= 16) /* legalább 16 bájtot elvárunk */
                            { memcpy((void*)vo, OSZ_PLATFORM, 1 + sizeof OSZ_PLATFORM); } else
                        if(!memcmp(strtable + s->st_name, "_environment", 13) && s->st_size == 8)
                            { *((virt_t*)vo) = elfctx->extra_ptr; } else
                        if(!memcmp(strtable + s->st_name, "_fb_struct", 11) && s->st_size >= 14) {
                            memcpy((void*)vo, &bootboot.fb_width, 12);
                            *((uint8_t*)vo+12) = bootboot.fb_type;
                            *((uint8_t*)vo+13) = display;
                        }
                    }
                    /* bármelyik taszk számára elérhető, libc adatok */
#if DEBUG
                    if(!memcmp(strtable + s->st_name, "_debug", 7) && s->st_size >= 8) { *((virt_t*)vo) = debug; } else
#endif
                    if(!memcmp(strtable + s->st_name, "__argumen", 10) && s->st_size >= 8)
                        { *((virt_t*)vo) = args; } else
                    if(!memcmp(strtable + s->st_name, "__environ", 10) && s->st_size >= 8)
                        { *((virt_t*)vo) = elfctx->env_ptr; } else
                    /* mivel ez .rodata szekcióban van, amit linkelünk, ezért elég egyszer bemásolni a lefordított sztringeket */
                    if(!memcmp(strtable + s->st_name, "_libc", 6) && !*((uint8_t*)vo) && s->st_size >= (ENOTIMPL+1)*TXT_LIBCSIZE) {
                        vmm_page(elftcb->memroot, vo, (phy_t)-1, PG_USER_RW|PG_PAGE);
                        if(vo & (__PAGESIZE-1)) vmm_page(elftcb->memroot, vo + __PAGESIZE, (phy_t)-1, PG_USER_RW|PG_PAGE);
                        memcpy((void*)vo, &libcerrstrs, (ENOTIMPL+1)*TXT_LIBCSIZE);
                        memcpy((void*)(vo + (ENOTIMPL+1)*TXT_LIBCSIZE), &osver, strlen(osver)+1);
                        vmm_page(0, vo, (phy_t)-1, PG_USER_RO|PG_PAGE|PG_SHARED);
                        if(vo & (__PAGESIZE-1)) vmm_page(0, vo + __PAGESIZE, (phy_t)-1, PG_USER_RO|PG_PAGE|PG_SHARED);
                    }
                }
                vo -= BUF_ADDRESS;
#if DEBUG
                if(debug&DBG_RTEXPORT) kprintf("    %x %s:", vo, strtable + s->st_name);
#endif
                /* megnézzük, hol van szükség erre a szimbólumra? most már tudjuk a címét */
                for(j=0, rel = relas; j<nrelas; j++,rel++) {
                    if(rel->offs != 0 && !memcmp(rel->sym, strtable + s->st_name, strlen(rel->sym)+1)) {
#if DEBUG
                        if(debug&DBG_RTEXPORT) kprintf(" %x", rel->offs - BUF_ADDRESS);
#endif
                        *((virt_t *)rel->offs) = vo;
                        rel->offs = 0;
                    }
                }
#if DEBUG
                if(debug&DBG_RTEXPORT) kprintf("\n");
#endif
            }
            s = (Elf64_Sym *)((uint8_t *)s + syment);
        }
    }

    /* megnézzük, hogy minden szimbólumot fel tudtunk-e oldani */
    for(j=0, rel = relas; j<nrelas; j++,rel++) {
        if(rel->offs!=0) {
            /* ez valószínűbb hiba, programozói tévedés is okozhatja */
            if(runlevel < RUNLVL_COOP)
                kpanic("%s: %s '%s'", elftcb->cmdline + BUF_ADDRESS, TXT_sharedmissing, rel->sym);
            else {
                syslog(LOG_ERR, "pid %x: unresolved symbol '%x'", elftcb->pid, rel->sym);
                goto err;
            }
        }
    }
    free(relas);

    return true;
}

#if DEBUG
extern virt_t dbg_lastsym;
extern char *dbg_symfile;
#endif
extern uint8_t sys_fault;
/**
 * szimbólum keresése címhez, cím -> sztring
 */
virt_t elf_sym(virt_t addr, bool_t onlyfunc)
{
    tcb_t *tcb = (tcb_t*)0;
    elfbin_t *eb;
    Elf64_Ehdr *ehdr = NULL;
    Elf64_Phdr *phdr_l;
    Elf64_Sym *s = NULL;
    Elf64_Dyn *d;
    char *strtable = NULL, *last = NULL;
    uint i, strsz = 0, numsym = 0, syment = sizeof(Elf64_Sym);
    virt_t vo, l;

    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH || addr < TEXT_ADDRESS || (addr >= DYN_ADDRESS && addr < CORE_ADDRESS)) return 0;
    if(addr >= CORE_ADDRESS) {
        ehdr = (Elf64_Ehdr *)CORE_ADDRESS;
#if DEBUG
        dbg_symfile = "/sys/core";
#endif
    } else {
        for(i = 0, eb = tcb->elfbin; eb->addr; eb++, i++)
            if(addr >= eb->addr && addr < eb->end) {
                ehdr = (Elf64_Ehdr *)eb->addr;
                if(!i) last = startsym;
#if DEBUG
                dbg_symfile = elfcache[eb->elfcacheidx].fn;
#endif
                break;
            }
    }
    if(!ehdr || platform_memfault(ehdr)) return 0;

#if DEBUG
    dbg_lastsym = 0;
#endif

    phdr_l = (Elf64_Phdr *)((uint8_t *)ehdr + ehdr->e_phoff + 2*ehdr->e_phentsize);
    if(ehdr->e_phnum>2 && (uint)phdr_l->p_memsz) {
        /* Dinamikus hedör. Ebben csak az exportált szimbólumok vannak benne */
        d = (Elf64_Dyn *)((uint8_t *)ehdr + phdr_l->p_offset);
        while(d->d_tag != DT_NULL) {
            switch(d->d_tag) {
                case DT_STRTAB: strtable = (char *)ehdr + d->d_un.d_ptr; break;
                case DT_STRSZ:  strsz = d->d_un.d_val; break;
                case DT_SYMTAB: s = (Elf64_Sym *)((uint8_t *)ehdr + d->d_un.d_ptr); break;
                case DT_SYMENT: syment = d->d_un.d_val; break;
                default:        break;
            }
            d++;
        }
        numsym = (strtable-(char*)s)/(syment?syment:sizeof(Elf64_Sym));
    }
    /* ha nincs szimbólum tábla */
    if(!s || !strtable || !strsz || !numsym || platform_memfault(s) || platform_memfault(strtable)) return 0;

    /* szimbólum keresése */
    for(l = 0, i = 0; i < numsym; i++) {
        if(platform_memfault(s) || s->st_name >= strsz) break;
        vo = (virt_t)s->st_value + (addr >= CORE_ADDRESS? 0 : (virt_t)ehdr);
        if(s->st_value && strtable[s->st_name]!='$' && strtable[s->st_name] && vo <= addr && vo > l) {
            if(!onlyfunc || ELF64_ST_TYPE(s->st_info) == STT_FUNC) {
                last = strtable + (uint32_t)s->st_name;
#if DEBUG
                dbg_lastsym = vo;
#endif
            } else {
                last = NULL;
#if DEBUG
                dbg_lastsym = 0;
#endif
            }
            l = vo;
        }
        s = (Elf64_Sym *)((uint8_t *)s + syment);
    }
    if(sys_fault) { sys_fault = false; last = NULL; }
    return (virt_t)last;
}

#if DEBUG
/**
 * cím keresése szimbólumhoz, sztring -> cím
 */
virt_t elf_lookupsym(char *sym)
{
    tcb_t *tcb = (tcb_t*)0;
    elfbin_t *eb;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr_l;
    Elf64_Sym *s;
    Elf64_Dyn *d;
    char *strtable;
    uint i, strsz, numsym, syment;

    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH || !sym || !sym[0]) return (virt_t)-1;
    for(eb = tcb->elfbin; eb->addr; eb++) {
        ehdr = (Elf64_Ehdr *)eb->addr;
        if(platform_memfault(ehdr)) continue;

        strsz = 0; syment = sizeof(Elf64_Sym); strtable = NULL; s = NULL;
        phdr_l = (Elf64_Phdr *)((uint8_t *)ehdr + ehdr->e_phoff + 2*ehdr->e_phentsize);
        if((uint)phdr_l->p_memsz) {
            /* Dinamikus hedör. Ebben csak az exportált szimbólumok vannak benne */
            d = (Elf64_Dyn *)((uint8_t *)ehdr + phdr_l->p_offset);
            while(d->d_tag != DT_NULL) {
                switch(d->d_tag) {
                    case DT_STRTAB: strtable = (char *)ehdr + d->d_un.d_ptr; break;
                    case DT_STRSZ:  strsz = d->d_un.d_val; break;
                    case DT_SYMTAB: s = (Elf64_Sym *)((uint8_t *)ehdr + d->d_un.d_ptr); break;
                    case DT_SYMENT: syment = d->d_un.d_val; break;
                    default:        break;
                }
                d++;
            }
            numsym = (strtable-(char*)s)/(syment?syment:sizeof(Elf64_Sym));
        }
        /* ha nincs szimbólum tábla */
        if(!s || !strtable || !strsz || !numsym || platform_memfault(s) || platform_memfault(strtable)) continue;

        /* szimbólum keresése */
        for(i = 0; i < numsym; i++) {
            if(platform_memfault(s) || s->st_name >= strsz) break;
            if(s->st_value && strtable[s->st_name]!='$' && strtable[s->st_name] && !strcmp(strtable + (uint32_t)s->st_name, sym))
                return (virt_t)s->st_value + (virt_t)ehdr;
            s = (Elf64_Sym *)((uint8_t *)s + syment);
        }
    }
    sys_fault = false;
    return (virt_t)-1;
}
#endif
