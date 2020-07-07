/*
 * core/x86_64/dbg.c
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
 * @subsystem debugger
 * @brief Beépített Debugger, architektúra függő eljárások
 */

#include <arch.h>
#include "disasm.h"

/* külső erőforrások */
extern void dbg_setpos();
extern void dbg_settheme();
extern void kprintf_center(int w, int h);
extern char *sprintf(char *dst,char* fmt, ...);
extern uint8_t dbg_inst, dbg_step;
extern char *dbg_err, *strbrk;
extern virt_t dbg_faultaddr;

/*** rendszerleíró sztringek ***/
char *dbg_systables[] = { "dma", "acpi", "smbi", "efi", "pcie", "dsdt", "apic", "ioapic", "hpet", NULL }; /* lásd arch.h */
char *dbg_regs[]={ "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rbp", NULL };
char *dbg_fareg = "cr2"; /* fault address, lapfordítási hiba címét tartalmazó regiszter neve */

uint64_t dr6, numbrk=0;

/**
 * kiírja, hogy a lapcímtáblában melyik bit milyen attribútumot takar
 */
void dbg_paginghelp()
{
    kprintf_center(34,12); dbg_setpos();
    kprintf(
        "                                  \n"
        "  ./L    11 OS/Z linked page      \n"
        "  ./G     8 Global                \n"
        "  ./T     7 sloT                  \n"
        "  ./D     6 Dirty flag            \n"
        "  ./A     5 Accessed flag         \n"
        "  C/.     4 Cache/noCache         \n"
        "  ./W     3 cache Write-through   \n"
        "  S/U     2 Supervisor/User       \n"
        "  R/W     1 Readable/Writable     \n"
        "  ./X    63 non-eXecutable        \n"
        "                                  \n"
    );
}

/**
 * a P lapcímtárbejegyzés attribútumainak dumpolása
 */
void dbg_pagingflags(uint64_t p)
{
    kprintf("%c%c%c%c%c",(p>>11)&1?'L':'.',(p>>8)&1?'G':'.',(p>>7)&1?'T':'.',(p>>6)&1?'D':'.',(p>>5)&1?'A':'.');
    kprintf("%c%c%c%c%c",(p>>4)&1?'.':'C',(p>>3)&1?'W':'.',(p>>2)&1?'U':'S',(p>>1)&1?'W':'R',p&(1UL<<PG_NX_BIT)?'.':'X');
}

/**
 * speciális regiszterek dumpolása, fault address cím lekérdezése
 */
void dbg_dumpregs()
{
    tcb_arch_t *tcb = (tcb_arch_t*)0;
    __asm__ __volatile__( "movq %%cr2, %0" : "=a"(dbg_faultaddr));
    kprintf("cr2 %8x cr3 %8x rip 0%1x:%6x flg %8x\n", dbg_faultaddr, tcb->common.memroot, tcb->cs, tcb->pc, tcb->rflags);
}

/**
 * CPU Kontroll Blokk architektúrafüggő mezőinek dumpolása
 */
void dbg_dumpccb(ccb_t *ccb)
{
    kprintf(", APIC Id: %4x, Logical APIC Id: %4x", ccb->apicid, ccb->lapicid);
}

/**
 * ki/bekapcsolja a lépésenkénti utasításvégrehajtást
 */
void dbg_singlestep(bool_t enable)
{
    tcb_arch_t *tcb = (tcb_arch_t*)0;
    if(enable) { tcb->rflags |= (1<<8); } else { tcb->rflags &= ~(1<<8); }
}

/**
 * visszaadja az előző utasítás címét
 */
virt_t dbg_previnst(virt_t addr)
{
    /* ezt lehetne úgy, hogy i-- ciklusban addig futtatjuk a disasm(i,NULL)-t, míg addr-t nem ad vissza, de ez biztosabb */
    return addr - 1;
}

/**
 * dekódol vagy hexában dumpol egy utasítást
 */
virt_t dbg_disasm(virt_t addr, char *str)
{
    virt_t start = addr, p;

    addr = disasm(addr, !dbg_inst? str : NULL);
    if(dbg_inst && str) {
        if(addr - start > 1 && *((uint8_t*)start) == 0x90) {
            /* ha több nop volt, akkor is csak egyszer adjuk vissza */
            str = sprintf(str, " %d x 90", addr - start);
        } else {
            for(p = start; p < addr; p++) { str = sprintf(str, "%01x ", *((uint8_t*)p)); }
            str--;
        }
        *str=0;
    }
    return addr;
}

/**
 * listázza a breakpointokat vagy beállít egy új breakpointot O címen, M módra, L hosszan
 */
void dbg_brk(virt_t o, uint8_t m, uint8_t l)
{
    uint64_t dr7;

    if(!o && !m && !l) {
        /* breakpointok listázása */
        __asm__ __volatile__( "movq %%dr7, %0;": "=a"(dr7));
        kprintf_center(30, 6); dbg_setpos();
        kprintf("                              \n");
        __asm__ __volatile__("movq %%dr0, %0":"=a"(o)); m = (dr7>>16)&3; l = (dr7>>18)&3;
        kprintf(" %c BRK0 %8x %c %c  \n",numbrk==0?'>':' ', o,
            dr7&(1<<1)?(l==0?'b':(l==1?'w':(l==3?'d':'q'))):'.', dr7&(1<<1)?(m==0?'x':(m==1?'w':(m==2?'p':'w'))):'.');
        __asm__ __volatile__("movq %%dr1, %0":"=a"(o)); m = (dr7>>20)&3; l = (dr7>>22)&3;
        kprintf(" %c BRK1 %8x %c %c  \n",numbrk==1?'>':' ', o,
            dr7&(1<<3)?(l==0?'b':(l==1?'w':(l==3?'d':'q'))):'.', dr7&(1<<3)?(m==0?'x':(m==1?'w':(m==2?'p':'w'))):'.');
        __asm__ __volatile__("movq %%dr2, %0":"=a"(o)); m = (dr7>>24)&3; l = (dr7>>26)&3;
        kprintf(" %c BRK2 %8x %c %c  \n",numbrk==2?'>':' ', o,
            dr7&(1<<5)?(l==0?'b':(l==1?'w':(l==3?'d':'q'))):'.', dr7&(1<<5)?(m==0?'x':(m==1?'w':(m==2?'p':'w'))):'.');
        __asm__ __volatile__("movq %%dr3, %0":"=a"(o)); m = (dr7>>28)&3; l = (dr7>>30)&3;
        kprintf(" %c BRK3 %8x %c %c  \n",numbrk==3?'>':' ', o,
            dr7&(1<<7)?(l==0?'b':(l==1?'w':(l==3?'d':'q'))):'.', dr7&(1<<7)?(m==0?'x':(m==1?'w':(m==2?'p':'w'))):'.');
        kprintf("                              \n");
    } else {
        /* új breakpoint definilása. Véges számú regiszterünk van, amiket körkörösen használunk */
        switch(m) {
            case 1:  m=3; break;  /* olvasás */
            case 2:  m=1; break;  /* írás */
            case 3:  m=1; break;  /* nincs írás-olvasás, úgyhogy írásnak állítjuk */
            case 4:  m=2; break;  /* B/K port */
            default: m=0;l=1;break;  /* futtatás */
        }
        switch(l) {
            case 1:  l=0; break;  /* byte */
            case 2:  l=1; break;  /* word */
            case 4:  l=3; break;  /* dword */
            default: l=2; break;  /* qword */
        }
        switch(numbrk){
            case 0:
                __asm__ __volatile__(
                    "movq %%rax, %%dr0; movq %%dr7, %%rax;andq %%rcx,%%rax;orq %%rdx,%%rax;movq %%rax, %%dr7"
                ::"a"(o), "c"(~((15UL<<16)|(3<<0))), "d"((uint64_t)(((l&3)<<18)|((m&3)<<16)|(o==0?0:(3<<0)))));
                break;
            case 1:
                __asm__ __volatile__(
                    "movq %%rax, %%dr1; movq %%dr7, %%rax;andq %%rcx,%%rax;orq %%rdx,%%rax;movq %%rax, %%dr7"
                ::"a"(o), "c"(~((15UL<<20)|(3<<2))), "d"((uint64_t)(((l&3)<<22)|((m&3)<<20)|(o==0?0:(3<<2)))));
                break;
            case 2:
                __asm__ __volatile__(
                    "movq %%rax, %%dr2; movq %%dr7, %%rax;andq %%rcx,%%rax;orq %%rdx,%%rax;movq %%rax, %%dr7"
                ::"a"(o), "c"(~((15UL<<24)|(3<<4))), "d"((uint64_t)(((l&3)<<26)|((m&3)<<24)|(o==0?0:(3<<4)))));
                break;
            case 3:
                __asm__ __volatile__(
                    "movq %%rax, %%dr3; movq %%dr7, %%rax;andq %%rcx,%%rax;orq %%rdx,%%rax;movq %%rax, %%dr7"
                ::"a"(o), "c"(~((15UL<<30)|(3<<6))), "d"((uint64_t)(((l&3)<<30)|((m&3)<<28)|(o==0?0:(3<<6)))));
                break;
        }
        numbrk++; if(numbrk>3) numbrk=0;
    }
}
