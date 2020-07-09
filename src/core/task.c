/*
 * core/task.c
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
 * @brief taszk funkciók
 */

#include <arch.h>                   /* szükségünk van a vmm_switch()-re */

uint32_t task_nextcpu = 0;          /* azt mutatja, melyik CPU-ra kell betölteni a taszkot. Ezt több CPU is növelheti gond nélkül */

interp_t interpreters[] = {         /* bájtkód értelmezők listája */
    { "\000asm", 0, 4, "/bin/wasi" },
    { "CSBC", 0, 4, "/bin/csi" }
};

/**
 * létrehoz egy új taszkot
 */
tcb_t *task_new(uint8_t priority)
{
    ccb_t *ccb;
    tcb_t *tcb;
    uint64_t n;
    uint32_t i;

    /* paraméterek ellenőrzése */
    if(priority>=PRI_IDLE) {
        seterr(EINVAL);
        return NULL;
    }
    /* ha nincs fix cpu megadva, akkor arra töltjük be, amelyiken a legkevesebb taszk fut */
    if(task_nextcpu >= numcores) {
        for(n=(uint64_t)-1UL, i=0, ccb=(ccb_t*)(CCBS_ADDRESS); i < numcores; i++, ccb++)
            if(ccb->numtasks < n) { n = ccb->numtasks; task_nextcpu = i; }
    }
    /* új címtér létrehozása */
    tcb = vmm_new(task_nextcpu, priority);
    tcb->state = TCB_STATE_RUNNING;
    memcpy(&tcb->owner, "system", 7);
    tcb->owner.Data4[7] = A_EXEC;
    ((ccb_t*)(CCBS_ADDRESS+tcb->cpuid*__PAGESIZE))->numtasks++;
#if DEBUG
    if(debug&DBG_TASKS)
        kprintf("task_new(cpu=%d,pri=%d,memroot=%x) pid %x\n", tcb->cpuid, priority, tcb->memroot, tcb->pid);
#endif

    /* átkapcsolunk a taszk címterére */
    vmm_switch(tcb->memroot);
    return tcb;
}

/**
 * lemásolja az aktuális taszkot
 */
tcb_t *task_fork(tcb_t *tcb)
{
#if DEBUG
    if(debug&DBG_TASKS)
        kprintf("task_fork(pid %x) pid %x\n", tcb->pid, 0);
#endif
    return tcb;
}

/**
 * megszüntet egy taszkot
 */
void task_del(tcb_t *tcb)
{
    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH || tcb->pid == idle_tcb.pid) {
        seterr(EINVAL);
        return;
    }
    ((ccb_t*)(CCBS_ADDRESS+tcb->cpuid*__PAGESIZE))->numtasks--;
    elf_unload(tcb->elfbin);            /* kiszedjük a binárisokat */
    /* nem szüntethetjük meg a címteret, ha rajta állunk, ezért előbb elmentjük a TCB-t és átkapcsolunk egy másikra */
    if(runlevel >= RUNLVL_COOP) {
        vmm_page(0, LDYN_tmpmap3, tcb->pid << __PAGEBITS, PG_CORE_RWNOCACHE|PG_PAGE);
        sched_remove(tcb);              /* biztosra megyünk, hogy a sched_pick() ne minket válasszon */
        sched_pick();
        tcb = (tcb_t*)LDYN_tmpmap3;
    }
    vmm_del(tcb);                       /* töröljük az egész címteret */
}

/**
 * egy futtatható paramétereinek betöltése a taszk címterébe. Mivel az exec() hiba esetén vissza
 * kell térjen az eredeti kódszegmensre, ezért a felső bufferterületet használjuk ideiglenesen
 */
bool_t task_execinit(tcb_t *tcb, char *cmd, char **argv, char **envp)
{
    elfctx_t *elfctx = (elfctx_t*)(BUF_ADDRESS + __PAGESIZE);
    uint i, j, argc = 1, np = 4, ns = 1;
    uint64_t *stack;
    char *elfptr, *intrp = NULL, *c;
    elfcache_t *elf;
    tcb_arch_t *tcba = (tcb_arch_t*)tcb;

    /* paraméterek ellenőrzése */
    if(!cmd || !cmd[0] || tcb->magic != OSZ_TCB_MAGICH) {
        seterr(EINVAL);
        return false;
    }
#if DEBUG
    if(debug&DBG_TASKS)
        kprintf("task_execinit(cmd='%s') pid %x\n", cmd, tcb->pid);
#endif

    /* megnézzük, hogy a futtatható létezik-e. Eszközmeghajtókat csak egyszer tölthetünk be */
    seterr(SUCCESS);
    elf = elf_getfile(cmd);
    if(!elf || !elf->numpages || !elf->data || !elf->data[0] || (tcb->priority==PRI_DRV && elf->numref)) return false;
    vmm_page(0, LDYN_tmpmap2, elf->data[0], PG_CORE_RWNOCACHE | PG_PAGE);
    elfptr = (char*)(LDYN_tmpmap2 + (virt_t)(elf->data[0]&((1<<__PAGEBITS)-1)));

    /* ellenőrizzük, hogy esetleg szkript-e, összeszámoljuk a sztringek hosszát és a mutatók számát */
    if(elfptr[0]=='#' && elfptr[1]=='!') {
        intrp = c = elfptr+2;
        while(*c && *c!='\n') {
            if(*c==' ') {
                argc++;
                while(*c==' ') c++;
                continue;
            }
            c++;
        }
        if(intrp+1024 < c) {
            seterr(ENOEXEC);
            return false;
        }
        argc++;
        ns += c-intrp;
    } else {
        /* megnézzük, hogy platform független bájtkód-e a futtatható */
        for(i = 0; i < (int)(sizeof(interpreters)/sizeof(interpreters[0])); i++)
            if (!memcmp(elfptr + interpreters[i].offs, interpreters[i].magic, interpreters[i].len)) {
                intrp = interpreters[i].interp;
                argc++;
                ns += strlen(intrp)+1;
                break;
            }
    }
    if(envp) for(np++,i=0; envp[i]; i++,np++) ns += strlen(envp[i])+1;
    if(argv) for(i=0; argv[i]; i++,argc++)    ns += strlen(argv[i])+1;
    np += argc;
    ns += strlen(cmd)+1;

    /* kiszámoljuk a teljes verem méretét */
    i = ((ns + np*8 + 15)/16)*16;
    j = (((i+__PAGESIZE-1)>>__PAGEBITS)<<__PAGEBITS);
    if(j>__BUFFSIZE) {
        seterr(ENOMEM);
        return false;
    }

    /* megbizonyosodunk róla, hogy bufferterület üres legyen. Akkora, mint a dynbss kezdőcíme, 4G */
    vmm_free(tcb, BUF_ADDRESS, DYN_ADDRESS);
    /* legelőször a vermet foglaljuk le, hogy a fában végig PG_USER_RW legyen */
    vmm_map(tcb, BUF_ADDRESS + TEXT_ADDRESS - j, 0, j, PG_USER_RW);
    /* az üzenetsor helyén tároljuk az ELF ideiglenes változókat, ez csak olvasható felhasználói szintről */
    vmm_map(tcb, BUF_ADDRESS + __PAGESIZE, 0, __PAGESIZE, PG_USER_RW);
    /* aztán az aktuális TCB-t képezzük le a buffer területre, csak felügyeleti hozzáféréssel */
    vmm_map(tcb, BUF_ADDRESS, tcb->pid << __PAGEBITS, __PAGESIZE, PG_CORE_RW);
    tcba->sp = TEXT_ADDRESS - i;
    stack = (uint64_t*)(BUF_ADDRESS + tcba->sp);
    c = (char*)stack + np*8;
    *stack++ = argc;                                            /* argc */
    *stack++ = (uint64_t)(tcba->sp + 24);                       /* argv */
    *stack   = (uint64_t)(tcba->sp + 24 + (envp?8:0) + argc*8); /* envp */
    elfctx->env_ptr = *stack++;
    tcb->cmdline = (uint32_t)((uint64_t)c-BUF_ADDRESS);
    /* argv[0] */
    if(intrp) {
        while(*intrp!=0 && *intrp!='\n') {
            *stack++ = (uint64_t)c-BUF_ADDRESS;
            while(*intrp!=0 && *intrp!='\n' && *intrp!=' ') *c++ = *intrp++;
            while(*intrp==' ') intrp++;
            *c++ = 0;
        }
    }
    *stack++ = (uint64_t)c-BUF_ADDRESS;
    while(*cmd!=0 && *cmd!='\n' && *cmd!=' ') *c++ = *cmd++;
    *c++ = 0;
    /* többi paraméter */
    if(argv) {
        while(*argv) {
            *stack++ = (uint64_t)c-BUF_ADDRESS;
            c = strncpy(c, *argv, ns)+1;
            argv++;
        }
    }
    *c++ = 0;       /* dupla null karakter a parancssor után */
    *stack++ = 0;   /* parancssor mutatókat is lezárjuk egy null-al */
    /* környezeti változók */
    if(envp) {
        while(*envp) {
            *stack++ = (uint64_t)c-BUF_ADDRESS;
            c = strncpy(c, *envp, ns)+1;
            envp++;
        }
        *stack++ = 0;
    }

    /* ha még nincs a taszk dynbss-e inicializálva, most megtesszük. Ha fork() után jöttünk ide, akkor már van.
     * felesleges ellenőrizni, mivel a vmm_map() hibával kiszáll, ha már le van valami ide képezve, ez nem POSIX mmap() */
    vmm_map(tcb, DYN_ADDRESS, 0, __PAGESIZE, PG_USER_RW);

    return true;
}

/**
 * betöltés befejezése, a kódszegmens lecserélése vagy a kódbuffer eldobása
 */
bool_t task_execfini(tcb_t *tcb)
{
    elfctx_t *elfctx = (elfctx_t*)(BUF_ADDRESS + __PAGESIZE);

    /* paraméterek ellenőrzése */
    if(tcb->magic != OSZ_TCB_MAGICH)
        return false;

#if DEBUG
    if(debug&DBG_TASKS)
        kprintf("task_execfini(errno=%d, %s) pid %x\n", ((ccb_t*)LDYN_ccb)->core_errno,
            ((ccb_t*)LDYN_ccb)->core_errno? "ROLLBACK" : "OK", tcb->pid);
#endif

    /* ha nem volt hiba a betöltés közben */
    if(((ccb_t*)LDYN_ccb)->core_errno == SUCCESS) {
        elf_unload(tcb->elfbin);                                    /* az aktív kódszegmensben lévő objektumok felszabadítása */
        tcb->elfbin = elfctx->elfbin;
        memset((void*)(BUF_ADDRESS + __PAGESIZE), 0, __PAGESIZE);   /* betöltő ideiglenes változóit kitöröljük */
        vmm_swaptextbuf(tcb);                                       /* megcseréljük a buffert és a kódszegmenst */
        task_nextcpu = numcores;                                    /* a következő taszkot a soronkövetkező CPU-ra töltjük be */
    } else {
        elf_unload(elfctx->elfbin);                                 /* a bufferben lévő objektumok felszabadítása */
    }
    vmm_free(tcb, BUF_ADDRESS, DYN_ADDRESS);    /* felszabadítjuk a buffert. Siker esetén ez a régi kódszegmenst jelenti */
    return (((ccb_t*)LDYN_ccb)->core_errno == SUCCESS);
}
