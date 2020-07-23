/*
 * core/aarch64/dbg.c
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
char *dbg_systables[] = { "dma", "acpi", "mmio", "efi", NULL };  /* lásd arch.h */
char *dbg_regs[]={ "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x28", "x27", "x29", "x30", NULL };
char *dbg_fareg = "far"; /* fault address, lapfordítási hiba címét tartalmazó regiszter neve */

uint64_t spsr, esr, numbrk=0, numwtp=0;

/**
 * Platform specifikus debugger inicializálás
 */
void dbg_init()
{
    __asm__ __volatile__ ("mrs %0, far_el1" : "=r" (dbg_faultaddr));
    __asm__ __volatile__ ("mrs %0, esr_el1" : "=r" (esr));
    __asm__ __volatile__ ("mrs %0, spsr_el1" : "=r" (spsr));
}

/**
 * Platform specifikus debugger visszaállítás
 */
void dbg_fini()
{
}

/**
 * kiírja, hogy a lapcímtáblában melyik bit milyen attribútumot takar
 */
void dbg_paginghelp()
{
    kprintf_center(34,15); dbg_setpos();
    kprintf("                                  \n"
            "  ./L    58 OS/Z linked page      \n"
            "  ./N    63 NSTable               \n"
            "  0-3 62-61 APTable               \n"
            "  ./U    60 UXNTable              \n"
            "  ./P    59 PXNTable              \n"
            "  ./P    53 PXN                   \n"
            "  G/.    11 non-Global            \n");
    kprintf("  ./A    10 Accessed Flag         \n"
            "  N/I/O 9-8 SHareability          \n"
            "  C/D/. 4-2 Cache/Device/noCache  \n"
            "  S/U     6 Supervisor/User       \n"
            "  W/R     7 Readable/Writable     \n"
            "  ./X    54 non-eXecutable        \n"
            "                                  \n"
    );
}

/**
 * a P lapcímtárbejegyzés attribútumainak dumpolása
 */
void dbg_pagingflags(uint64_t p)
{
    kprintf("%c%c%x%c%c%c",(p>>58)&1?'L':'.',(p>>63)&1?'N':'.',(p>>61)&3,(p>>60)&1?'U':'.',(p>>59)&1?'P':'.');
    kprintf("%c%c%c%x",(p>>53)&1?'P':'.',(p>>11)&1?'.':'G',(p>>10)&1?'A':'.',(p>>8)&3?(((p>>8)&3)==1?'I':'O'):'N');
    kprintf("%c%c%c%c",(p>>2)&3?((p>>2)==1?'D':'.'):'C',(p>>6)&1?'U':'S',(p>>7)&1?'R':'W',p&(1UL<<54)?'.':'X');
}

/**
 * speciális regiszterek dumpolása, fault address cím lekérdezése
 */
void dbg_dumpregs()
{
    kprintf("far %8x tbr %8x esr %8x spsr %4x\n", dbg_faultaddr, ((tcb_t*)0)->memroot, esr, spsr);
}

/**
 * CPU Kontroll Blokk architektúrafüggő mezőinek dumpolása
 */
void dbg_dumpccb(unused ccb_t *ccb)
{
}

/**
 * ki/bekapcsolja a lépésenkénti utasításvégrehajtást
 */
void dbg_singlestep(bool_t enable)
{
    register uint64_t reg;
    __asm__ __volatile__ ("mrs %0, mdscr_el1" : "=r" (reg));
    if(enable) reg |= (1<<13)|1; else reg &= ~1;      /* KDE és SS bitek. A KDE-t nem töröljük, mert lehet breakpoint */
    __asm__ __volatile__ ("msr mdscr_el1, %0" :: "r" (reg));
    if(enable) spsr |= (1<<21); else spsr &= ~(1<<21);
    __asm__ __volatile__ ("msr spsr_el1, %0" :: "r" (spsr));
}

/**
 * visszaadja az előző utasítás címét
 */
virt_t dbg_previnst(virt_t addr)
{
    /* AArch64-en minden utasítás fixen 4 bájt */
    return addr - 4;
}

/**
 * dekódol vagy hexában dumpol egy utasítást
 */
virt_t dbg_disasm(virt_t addr, char *str)
{
    virt_t start = addr, p;

    addr = disasm(addr, !dbg_inst? str : NULL);
    if(dbg_inst && str) {
        if(addr - start > 4 && *((uint32_t*)start) == 0xd503201f) {
            /* ha több nop volt, akkor is csak egyszer adjuk vissza */
            str = sprintf(str, " %d x %x", (addr - start)>>2, 0xd503201f);
        } else {
            for(p = start; p < addr; p += 4) { str = sprintf(str,"%04x ", *((uint32_t*)p)); }
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
    register uint64_t cr=0, msk;
    uint wrps, brps;

    /* lekérjük, hány regiszter van implementálva, ARM DDI0487 D10-2426 szerint legalább kettő mindenképp van */
    __asm__ __volatile__ ("mrs %0, id_aa64dfr0_el1" : "=r" (cr));
    wrps = ((cr >> 20) & 15) + 1; if(wrps > 4) wrps = 4;
    brps = ((cr >> 12) & 15) + 1; if(brps > 4) brps = 4;

    if(!o && !m && !l) {
        /* breakpointok listázása. AArch64-en külön van breakpoint és watchpoint */
        kprintf_center(30, 2+wrps+brps); dbg_setpos();
        kprintf("                              \n");
        __asm__ __volatile__ ("mrs %0, dbgbcr0_el1; mrs %1, dbgbvr0_el1" : "=r" (cr), "=r"(o));
        kprintf("  %cBRK0 %8x %c x  \n",numbrk==0?'>':' ', o, cr&1?'d':'.');
        __asm__ __volatile__ ("mrs %0, dbgbcr1_el1; mrs %1, dbgbvr1_el1" : "=r" (cr), "=r"(o));
        kprintf("  %cBRK1 %8x %c x  \n",numbrk==1?'>':' ', o, cr&1?'d':'.');
        if(brps > 2) {
            __asm__ __volatile__ ("mrs %0, dbgbcr2_el1; mrs %1, dbgbvr2_el1" : "=r" (cr), "=r"(o));
            kprintf("  %cBRK2 %8x %c x  \n",numbrk==2?'>':' ', o, cr&1?'d':'.');
        }
        if(brps > 3) {
            __asm__ __volatile__ ("mrs %0, dbgbcr3_el1; mrs %1, dbgbvr3_el1" : "=r" (cr), "=r"(o));
            kprintf("  %cBRK3 %8x %c x  \n",numbrk==3?'>':' ', o, cr&1?'d':'.');
        }

        __asm__ __volatile__ ("mrs %0, dbgwcr0_el1; mrs %1, dbgwvr0_el1" : "=r" (cr), "=r"(o));
        l = __builtin_popcount((cr >> 5) & 255); m = (cr >> 3) & 3;
        kprintf("  %cWTP0 %8x %c %c  \n",numwtp==0?'>':' ', o,
            cr&1 && l?(l==1?'b':(l==1?'w':(l==2?'d':'q'))):'.', cr&1 && m?(m==1?'r':(m==2?'w':'a')):'.');
        __asm__ __volatile__ ("mrs %0, dbgwcr1_el1; mrs %1, dbgwvr1_el1" : "=r" (cr), "=r"(o));
        l = __builtin_popcount((cr >> 5) & 255); m = (cr >> 3) & 3;
        kprintf("  %cWTP1 %8x %c %c  \n",numwtp==1?'>':' ', o,
            cr&1 && l?(l==1?'b':(l==1?'w':(l==2?'d':'q'))):'.', cr&1 && m?(m==1?'r':(m==2?'w':'a')):'.');
        if(wrps > 2) {
            __asm__ __volatile__ ("mrs %0, dbgwcr2_el1; mrs %1, dbgwvr2_el1" : "=r" (cr), "=r"(o));
            l = __builtin_popcount((cr >> 5) & 255); m = (cr >> 3) & 3;
            kprintf("  %cWTP2 %8x %c %c  \n",numwtp==2?'>':' ', o,
                cr&1 && l?(l==1?'b':(l==1?'w':(l==2?'d':'q'))):'.', cr&1 && m?(m==1?'r':(m==2?'w':'a')):'.');
        }
        if(wrps > 3) {
            __asm__ __volatile__ ("mrs %0, dbgwcr3_el1; mrs %1, dbgwvr3_el1" : "=r" (cr), "=r"(o));
            l = __builtin_popcount((cr >> 5) & 255); m = (cr >> 3) & 3;
            kprintf("  %cWTP3 %8x %c %c  \n",numwtp==3?'>':' ', o,
                cr&1 && l?(l==1?'b':(l==1?'w':(l==2?'d':'q'))):'.', cr&1 && m?(m==1?'r':(m==2?'w':'a')):'.');
        }
        kprintf("                              \n");
    } else {
        /* új breakpoint definilása. Véges számú regiszterünk van, amiket körkörösen használunk */
        if(!m) {
            o &= ~3; msk = (15<<5)|1;
            switch(numbrk) {
                case 0: __asm__ __volatile__("msr dbgbvr0_el1, %0; msr dbgbcr0_el1, %1" : : "r"(o), "r"(msk)); break;
                case 1: __asm__ __volatile__("msr dbgbvr1_el1, %0; msr dbgbcr1_el1, %1" : : "r"(o), "r"(msk)); break;
                case 2: __asm__ __volatile__("msr dbgbvr2_el1, %0; msr dbgbcr2_el1, %1" : : "r"(o), "r"(msk)); break;
                case 3: __asm__ __volatile__("msr dbgbvr3_el1, %0; msr dbgbcr3_el1, %1" : : "r"(o), "r"(msk)); break;
            }
            numbrk++; if(numbrk >= brps) numbrk = 0;
        } else if(m<4) {
            if(!l) l=4;
            /* D10-2689 szerint 8-al osztható címnek kell lennie, l = bas, m = lsc */
            switch(l) {
                case 1: l=(1<<(o&7)); break;            /* byte,  qword offszet, 1 bit 0, 1, 2, 3, 4, 5, 6, 7 */
                case 2: o &= ~1; l=(3<<(o&7)); break;   /* word,  qword offszet, 2 bit 0-1, 2-3, 4-5, 6-7 */
                case 4: o &= ~3; l=(15<<(o&7)); break;  /* dword, qword offszet, 4 bit 0-3, 4-7 */
                case 8: l=255; break;                   /* qword, qword offszet, mind a 8 bit */
            }
            o &= ~7; msk = (l<<5)|(m<<3)|1;
            switch(numwtp) {
                case 0: __asm__ __volatile__ ("msr dbgwvr0_el1, %0; msr dbgwcr0_el1, %1" : : "r"(o), "r"(msk)); break;
                case 1: __asm__ __volatile__ ("msr dbgwvr1_el1, %0; msr dbgwcr1_el1, %1" : : "r"(o), "r"(msk)); break;
                case 2: __asm__ __volatile__ ("msr dbgwvr2_el1, %0; msr dbgwcr2_el1, %1" : : "r"(o), "r"(msk)); break;
                case 3: __asm__ __volatile__ ("msr dbgwvr3_el1, %0; msr dbgwcr3_el1, %1" : : "r"(o), "r"(msk)); break;
            }
            numwtp++; if(numwtp >= wrps) numwtp = 0;
        }
        __asm__ __volatile__ ("mrs %0, mdscr_el1" : "=r" (cr));
        cr |= (1<<15)|(1<<13);     /* MDE és KDE bitek */
        __asm__ __volatile__ ("msr mdscr_el1, %0" :: "r" (cr));
    }
}
