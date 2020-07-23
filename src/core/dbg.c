/*
 * core/dbg.c
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
 * @brief Beépített Debugger
 */

#include <arch.h>

/* architektúrafüggő részben implementált */
extern char *dbg_systables[], *dbg_regs[], *dbg_fareg;  /* architectúra definíciós sztringek, regiszternevek stb. */
void dbg_init();                                        /* platform specifikus inicializálás */
void dbg_fini();                                        /* platform specifikus visszaállítás */
void dbg_paginghelp();                                  /* lapcímtár attribútumainak leírása */
void dbg_pagingflags(uint64_t p);                       /* egy lapcímtárbejegyzés attribútumainak dumpolása */
void dbg_dumpregs();                                    /* speciális regiszterek dumpolása (pl. cr2) */
void dbg_dumpccb(ccb_t *ccb);                           /* CPU Kontroll Blokk architektúrafüggő mezőinek dumpolása */
void dbg_singlestep(bool_t enable);                     /* ki/bekapcsolja a lépésenkénti utasításvégrehajtást */
virt_t dbg_previnst(virt_t addr);                       /* visszaadja az előző utasítás címét */
virt_t dbg_disasm(virt_t addr, char *str);              /* dekódol vagy hexában dumpol egy utasítást */
void dbg_brk(virt_t o, uint8_t m, uint8_t l);           /* listázza vagy beállít egy új breakpointot O címen, M módra, L hosszan */

/* külső erőforrások. Direktben használunk pár belsős kprintf függvényt és változót */
extern uint32_t fg, bg;
extern int kx, ky, fx, maxx, maxy, scry;
extern uint8_t volatile cnt, reent;
void kprintf_fade();
void kprintf_unicodetable();
void kprintf_putchar(int c);
virt_t elf_sym(virt_t addr, bool_t onlyfunc);
virt_t elf_lookupsym(char *sym);
unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern uint64_t pmm_size;
extern pmm_entry_t pmm_entries_buf;
extern char display_drv[], display_env[], intr_name[], syslog_buffer[];
virt_t disasm(virt_t addr, char *str);                  /* ezt itt csak azért használjuk, hogy visszaadja a köv. utasítás címét */

/* globális debug változók */
uint8_t volatile __attribute__((aligned(4))) dbg_indump;    /* ezt assemblyből is hivatkozzuk, ezért fontos a címigazítás */
uint8_t dbg_tui, dbg_isshft, dbg_tab, dbg_unit, dbg_full, dbg_btidx, dbg_inst, dbg_step;
virt_t dbg_lastsym, dbg_faultaddr, dbg_bt[8];
char *dbg_err, *dbg_symfile, cmdhist[512];
uint64_t dbg_label, buf_reloc;
uint32_t dbg_theme[7];
uint32_t theme_panic[7] = {0x100000,0x400000,0x800000,0x9c3c1b,0xcc6c4b,0xec8c6b,0xffdd33};
uint32_t theme_debug[7] = {0x000020,0x000040,0x101080,0x3e32a2,0x7c71da,0x867ade,0xffdd33};

enum {
    key_up=1, key_down, key_left, key_right, key_pgup, key_pgdn, key_del, key_back,
    key_f1=11, key_f2, key_f3, key_redraw=14, key_esc=27
};

enum {
    tab_code, tab_data, tab_msg, tab_tcb, tab_sched, tab_ram, tab_sysinfo, tab_env,
    tab_last
};
char *tabs[] = { "Code", "Data", "Messages", "Task", "CPU", "RAM", "Sysinfo", "Environment" };
virt_t dbg_codepos, dbg_lastpos, dbg_datapos, dbg_logpos, dbg_envpos;
phy_t dbg_dataphy;
uint dbg_mqpos, dbg_cpuid, dbg_rampos;
char *prio[] = { "SYSTEM   ", "RealTime ", "Driver   ", "Service  ", "High prio", "Normal   ", "Low prio ", "IDLE ONLY" };
char *disp[] = { "MONO_MONO", "MONO_COLOR", "STEREO_MONO", "STEREO_COLOR" };
char *fbt[] = { "ARGB", "RGBA", "ABGR", "BGRA" };

/*** segédfüggvények ***/

void dbg_putchar(uint16_t c)
{
    if(c<8 || (dbg_indump==2 && (c<' '||c>=127)))
        c='.';
    /* újsor karakter konvertálása kocsivissza és soremelésre */
    if(c=='\n')
        platform_dbgputc('\r');
    /* unicode konvertálása utf-8-ra */
    if(c<0x80) {
        platform_dbgputc(c);
    } else if(c<0x800) {
        platform_dbgputc(((c>>6)&0x1F)|0xC0);
        platform_dbgputc((c&0x3F)|0x80);
    } else /*if(c<0x10000)*/ {
        platform_dbgputc(((c>>12)&0x0F)|0xE0);
        platform_dbgputc(((c>>6)&0x3F)|0x80);
        platform_dbgputc((c&0x3F)|0x80);
/*
    } else {
        platform_dbgputc(((c>>18)&0x07)|0xF0);
        platform_dbgputc(((c>>12)&0x3F)|0x80);
        platform_dbgputc(((c>>6)&0x3F)|0x80);
        platform_dbgputc((c&0x3F)|0x80);
*/
    }
}

void dbg_setpos()
{
    if(dbg_tui) {
        dbg_putchar(27); dbg_putchar('[');
        dbg_putchar('0'+(((ky+1)/100)%10)); dbg_putchar('0'+(((ky+1)/10)%10)); dbg_putchar('0'+((ky+1)%10));
        dbg_putchar(';');
        dbg_putchar('0'+(((kx+1)/100)%10)); dbg_putchar('0'+(((kx+1)/10)%10)); dbg_putchar('0'+((kx+1)%10));
        dbg_putchar('H');
    }
}

void dbg_settheme(uint32_t f)
{
    fg = f;
    if(dbg_tui) {
        dbg_putchar(27); dbg_putchar('[');
        dbg_putchar((fg == dbg_theme[6] || fg == dbg_theme[4] || fg == dbg_theme[5])? '1' : '0');
        dbg_putchar(';'); dbg_putchar('3');
        if(fg == dbg_theme[6]) { dbg_putchar('3'); } else
        if(fg == 0x800000 || fg == 0x000080) { dbg_putchar('1'); } else
        if(fg==dbg_theme[3] || fg==dbg_theme[4]) { dbg_putchar(dbg_theme[0]==theme_debug[0]?'6':(fg==dbg_theme[3]?'7':'1')); } else
        if(fg==dbg_theme[0] || fg==dbg_theme[1] || fg==dbg_theme[2]) { dbg_putchar(dbg_theme[0]==theme_debug[0]?'4':'1'); } else
            dbg_putchar('7');
        if(bg == dbg_theme[2]) { dbg_putchar(';'); dbg_putchar('4'); dbg_putchar(dbg_theme[0]==theme_debug[0]?'4':'1'); }
        dbg_putchar('m');
    }
}

uint8_t dbg_getkey()
{
    uint16_t c = platform_waitkey();
    if(!(c & 0x8000)) {
        /* PS2 billentyűkód konvertálása ASCII karakterkódra */
        switch(c) {
            case 1: c = key_esc; break;
            case 14: c = key_back; break;
            case 15: c = '\t'; break;
            case 28: c = '\n'; break;
            case 57: c = ' '; break;
            case 59: c = key_f1; break;
            case 60: c = key_f2; break;
            case 61: c = key_f3; break;
            case 62: c = key_redraw; break;
            case 328: c = key_up; break;
            case 329: c = key_pgup; break;
            case 331: c = key_left; break;
            case 333: c = key_right; break;
            case 336: c = key_down; break;
            case 337: c = key_pgdn; break;
            case 339: c = key_del; break;
            default:
                if(c>=2&&c<=13) {
                    if(dbg_isshft)  c = "!@#$%^&*()_+"[c-2];
                    else            c = "1234567890-="[c-2];
                } else
                if(c>=16 && c<=27)  c = "qwertyuiop[]"[c-16]; else
                if(c>=30 && c<=40)  c = "asdfghjkl;'"[c-30]; else
                if(c>=44 && c<=53)  c = "zxcvbnm,./"[c-44]; else
                    c = 0;
        }
    } else {
        /* ANSI terminál CSI kódok konvertálása ASCII karakterkódokká */
        c &= ~0x8000; dbg_isshft = false;
        switch(c) {
            case 3: c = key_esc; break;
            case 8: case 127: c = key_back; break;
            case 10: case 13: c = '\n'; break;
            case 12: c = key_redraw; break;
            case 27:
                c = platform_waitkey() & 0xFF;
                if(c == 27) c = key_esc; else
                if(c == '[') {
                    c = platform_waitkey() & 0xFF;
                    switch(c) {
                        case 'A': c = key_up; break;
                        case 'a': c = key_up; dbg_isshft = true; break;
                        case 'B': c = key_down; break;
                        case 'b': c = key_down; dbg_isshft = true; break;
                        case 'C': c = key_right; break;
                        case 'c': c = key_right; dbg_isshft = true; break;
                        case 'D': c = key_left; break;
                        case 'd': c = key_left; dbg_isshft = true; break;
                        case 'Z': c = '\t'; dbg_isshft = true; break;
                        case '3': c = platform_waitkey() & 0xFF; if(c=='~') c = key_del; break;
                        case '5': c = platform_waitkey() & 0xFF; if(c=='~') c = key_pgup; break;
                        case '6': c = platform_waitkey() & 0xFF; if(c=='~') c = key_pgdn; break;
                        case '1':
                            c = platform_waitkey() & 0xFF;
                            if(c=='1') { c = platform_waitkey() & 0xFF; if(c=='~') c = key_f1; } else
                            if(c=='2') { c = platform_waitkey() & 0xFF; if(c=='~') c = key_f2; } else
                            if(c=='3') { c = platform_waitkey() & 0xFF; if(c=='~') c = key_f3; } else
                            if(c=='4') { c = platform_waitkey() & 0xFF; if(c=='~') c = key_redraw; }
                            break;
                        default: c = 0; break;
                    };
                }
                break;
        }
    }
    return c;
}

virt_t dbg_getaddr(char *cmd, size_t size, uint64_t base)
{
    char *s, **regname = dbg_regs;
    uint64_t *regval = (uint64_t*)TCBFLD_GPR, ret = -1UL, ts=0, ind=0, i;

    if(*cmd=='*') { cmd++; ind=1; }
    s=cmd;
    if(size==0 || *cmd==0) return base;
    if((*cmd<'0'||*cmd>'9')&&*cmd!='-'&&*cmd!='+') {
        while((virt_t)cmd < (virt_t)s+size && *cmd!=0 && *cmd != ' ' && *cmd != '-' && *cmd != '+') cmd++;
        ts = (virt_t)cmd-(virt_t)s;
        /* regiszternevek */
        if(ts == 2 && s[0] == 'p' && s[1] == 'c') ret = ((tcb_arch_t*)0)->pc; else
        if(ts == 2 && s[0] == 's' && s[1] == 'p') ret = ((tcb_arch_t*)0)->sp; else
        if((ts == 2 && s[0] == 'f' && s[1] == 'a') || (!dbg_fareg[ts] && !memcmp(dbg_fareg, s, ts))) ret = dbg_faultaddr; else
            for(i = 0; *regname; i++, regname++, regval++)
                if(!regname[ts] && !memcmp(regname, s, ts)) ret = *regval;
        /* szimbólumok */
        if(ret == -1UL) ret = elf_lookupsym(s);
        if(ret == -1UL) dbg_err = "No such symbol";
        else base = ret;
    }
    ret = 0;
    /* bázishoz képesti eltolás */
    while(size>0 && (*cmd==' '||*cmd=='-'||*cmd=='+')) { cmd++; size--; }
    if(size && *cmd!=0) {
        env_hex((unsigned char*)cmd, (uint64_t*)&ret, 0, 0);
        if(*(cmd-1)=='-') base -= ret; else
        if(*(cmd-1)=='+') base += ret; else
            base = ret;
    }
    if(ind) base = *((uint64_t*)base);
    return base;
}

void dbg_switchnext()
{
    tcb_t *tcb = (tcb_t*)0;
    ccb_t *ccb;
    pid_t pid = 0;
    int i, p;

    if(tcb->next) pid = tcb->next;
    else {
        dbg_cpuid = tcb->cpuid; p = tcb->priority + 1;
        while(!pid) {
            ccb = (ccb_t*)(CCBS_ADDRESS + (dbg_cpuid << __PAGEBITS));
            for(i = p; i <= PRI_IDLE; i++) { pid = ccb->hd_active[i]; break; }
            if(!pid) {
                dbg_cpuid++; if(dbg_cpuid >= numcores) dbg_cpuid = 0;
                if(dbg_cpuid == tcb->cpuid) break;
                p = PRI_SYS;
            }
        }
    }
    if(pid) {
        vmm_page(0, LDYN_tmpmap3, pid << __PAGEBITS, PG_CORE_RWNOCACHE | PG_PAGE);
        vmm_switch(((tcb_t*)LDYN_tmpmap3)->memroot);
    }
}

void dbg_switchprev()
{
    tcb_t *tcb = (tcb_t*)0;
    ccb_t *ccb;
    pid_t pid = 0;
    int i, p;

    if(tcb->prev) pid = tcb->prev;
    else {
        dbg_cpuid = tcb->cpuid; p = tcb->priority - 1;
        while(!pid) {
            ccb = (ccb_t*)(CCBS_ADDRESS + (dbg_cpuid << __PAGEBITS));
            if(p) for(i = p; i >= 0; i--) { pid = ccb->hd_active[i]; break; }
            if(!pid) {
                if(!dbg_cpuid) dbg_cpuid = numcores-1; else dbg_cpuid--;
                if(dbg_cpuid == tcb->cpuid) break;
                p = PRI_IDLE;
            }
        }
    }
    if(pid) {
        vmm_page(0, LDYN_tmpmap3, pid << __PAGEBITS, PG_CORE_RWNOCACHE | PG_PAGE);
        vmm_switch(((tcb_t*)LDYN_tmpmap3)->memroot);
    }
}

void dbg_help()
{
    bg = dbg_theme[2]; dbg_settheme(dbg_theme[5]);
    if(dbg_tab == tab_data) dbg_paginghelp();
    else {
        fx = kx = (maxx-54)/2; ky = 2; dbg_setpos();
        kprintf("                                                      \n"
                "         ---==<  OS/Z Debugger " OSZ_VER "  >==---         \n"
                "                                                      \n"
                " Keyboard Shortcuts                                   \n"
                "  F1 - this help, F2 - breakpoints, F2 - UNICODE tbl  \n"
                "  Tab - switch panels (shift+ backwards)              \n"
                "  Esc - exit debugger / clear command                 \n"
                "  Enter - repeat step instruction / execute command   \n"
                "  Left / Right - previous-next task / move cursor     \n");
        kprintf("  Up / Down - scroll / (shift+) command history       \n"
                "  PgUp / PgDn - move in backtrace, scroll a page      \n"
                "                                                      \n"
                " Commands                                             \n"
                "  (none) - repeat last step or continue command       \n"
                "  Step - step instruction                             \n"
                "  Continue - continue execution                       \n"
                "  REset, REboot - reboot computer                     \n"
                "  Quit, HAlt - power off computer                     \n");
        kprintf("  Help - this help, Help Flags - help on flags        \n"
                "  TUi - toggle video terminal support on serial line  \n"
                "  Full - toggle full window mode                      \n"
                "  Pid X - switch to task                              \n"
                "  Prev - switch to previous task                      \n"
                "  Next - switch to next task                          \n"
                "  Tcb - show current task's Task Control Block        \n"
                "  Messages - list messages in current task's queue    \n"
                "  CPu, SCheduler - show all task queues and CCB info  \n");
        kprintf("  Ram - show RAM information and allocation           \n"
                "  SYsinfo - system information                        \n"
                "  Instruction X, Disasm X - instruction disassemble   \n"
                "  Instruction, Disasm - toggle disassemble mode       \n"
                "  eXamine [/b1w2d4q8s] X - examine memory at X        \n"
                "  Break [/b12d4q8rwapx] X - set a breakpoint at X     \n"
                "                                                      \n");
    }
    if(dbg_tui) { dbg_putchar(27); dbg_putchar('['); dbg_putchar('0'); dbg_putchar('m'); }
    dbg_getkey();
}

void dbg_help2()
{
    bg = dbg_theme[2]; dbg_settheme(dbg_theme[5]);
    fx = kx = (maxx-54)/2; ky = 2; dbg_setpos();
    kprintf("                                                      \n"
            "         ---==<  OS/Z Debugger " OSZ_VER "  >==---         \n"
            "                                                      \n"
            " Command flags and arguments                          \n"
            "                                                      \n"
            "  I                             - toggle ASCII/hex    \n"
            "  I [base][+/-][offset]         - instruction decode  \n"
            "                                                      \n"
            "  X [flags] base[+/-][offset]   - dump memory         \n");
    kprintf("  XP [flags] base[+/-][offset]  - dump physical mem   \n"
            "   /1, /b - byte granularity (8 bit)                  \n"
            "   /2, /w - word granularity (16 bit)                 \n"
            "   /4, /d /l - double word granularity (32 bit)       \n"
            "   /8, /q /g - quad word granularity (64 bit)         \n"
            "   /s - dump as stack                                 \n"
            "   base - either a regiter name or a symbol or \"sp\"   \n"
            "   offset - a hexadecimal constant, optional          \n"
            "                                                      \n");
    kprintf("  B                             - list breakpoints    \n"
            "  B [flags] base[+/-][offset]   - set breakpoints     \n"
            "   /1, /b - byte length (8 bit)                       \n"
            "   /2 - word length (16 bit)                          \n"
            "   /4, /d /l - double word legnth (32 bit)            \n"
            "   /8, /q /g - quad word length (64 bit)              \n"
            "   /p - break on I/O port access (if supported)       \n"
            "   /a - break on memory access (read or write)        \n"
            "   /r - break on memory read                          \n");
    kprintf("   /w - break on memory write                         \n"
            "   /x - break on execution (default)                  \n"
            "   base - either a regiter name or a symbol or \"pc\"   \n"
            "   offset - a hexadecimal constant, optional          \n"
            "                                                      \n"
            "  S                             - single step         \n"
            "                                                      \n");
    if(dbg_tui) { dbg_putchar(27); dbg_putchar('['); dbg_putchar('0'); dbg_putchar('m'); }
    dbg_getkey();
}

/*** tabok ***/

void dbg_code(virt_t ptr)
{
    char inst[64], *func, **regname = dbg_regs;
    uint64_t *regval = (uint64_t*)TCBFLD_GPR;
    uint i = 0;
    int o;
    virt_t p, s, l = dbg_lastpos, c = dbg_bt[0], lf;

    if(!dbg_full) {
        dbg_settheme(dbg_theme[4]); kprintf("[Registers]\n"); dbg_settheme(dbg_theme[3]);
        dbg_dumpregs();
        for(i = 0; *regname; i++, regname++, regval++)
            kprintf(dbg_inst? "%3s '%A'      %s" : "%3s %8x%s", *regname, *regval, !((i+1)%4)? "\n" : " ");
        kprintf("sp  %8x%s\n", ((tcb_arch_t*)0)->sp, !((i++)%4)? "\n" : "");

        dbg_settheme(dbg_theme[4]); kprintf("\n[Backtrace]\n"); dbg_settheme(dbg_theme[3]);
        for(i=0; i<8 && dbg_bt[i]; i++) {
            dbg_settheme(dbg_theme[i == dbg_btidx? 6 : 3]);
            kprintf("%4x: ", dbg_bt[i]);
            if(!dbg_tui) {
                if(i == dbg_btidx) { dbg_putchar('-'); dbg_putchar('>'); } else { dbg_putchar(' '); dbg_putchar(' '); }
                dbg_putchar(' ');
            }
            p = (virt_t)elf_sym(dbg_bt[i], true);
            if(!p || !dbg_lastsym) kprintf("<?>\n");
            else {
                kprintf("<%s+%x> ", p, dbg_bt[i]-dbg_lastsym);
                if(!dbg_tui) { kx += 2; } while(kx<48) { kprintf(" "); } dbg_settheme(dbg_theme[2]);
                kprintf("%s\n", dbg_symfile);
            }
        }
        kprintf("\n");
    }

    /* kód visszafejtés */
    dbg_settheme(dbg_theme[4]);
    p = ptr? ptr : dbg_bt[dbg_btidx];
    func = (char*)elf_sym(p, true);
    lf = dbg_lastsym;
    if(!func || !dbg_lastsym) kprintf("[Code %8x:<?>]\n", p); else {
        o = p-dbg_lastsym;
        kprintf("[Code %8x:<%s%c%x>]\n", p, func, o>=0? '+' : '-', o>=0 ? o : -o);
    }
    dbg_settheme(dbg_theme[3]);
    while(ky < maxy-2) {
        func = (char*)elf_sym(p, true);
        if(dbg_lastsym && lf != dbg_lastsym) {
            lf = dbg_lastsym;
            if(func) { dbg_settheme(dbg_theme[2]); kprintf("%4x <%s>:\n", lf, func); }
        }
        dbg_settheme(dbg_theme[p == dbg_bt[dbg_btidx]? 6 : 3]);
        kprintf(" %02x",!dbg_inst? p&0xffffffffffffUL : p-dbg_lastsym);
        dbg_putchar(' '); dbg_settheme(dbg_theme[2]);
        kprintf_putchar(c != l && l && (p == l || p == c)? (p == (c < l? c : l)? 0x9d : 0x9e):
            (l && ((p > l && p < c) || (p > c && p < l))? 0x9c : (p == c? 0x9b : ' '))); kx++;
        kprintf_putchar(p == c? 0x9f : (l && p == l? 0x9a : ' ')); kx++;
        dbg_putchar(' '); dbg_settheme(dbg_theme[p == dbg_bt[dbg_btidx]? 6 : 3]);
        dbg_label = s = 0;
        if(platform_memfault((char*)p)) { dbg_settheme(dbg_theme[2]); kprintf("* not available *\n"); break; }
        p = dbg_disasm(p, (char*)&inst);
        if(inst[0] == ' ') { dbg_settheme(dbg_theme[2]); }
        if(dbg_label) s = elf_sym(dbg_label, false);
        if(s && dbg_lastsym && strlen(inst) < 38) {
            kprintf("%38s ", inst);
            dbg_settheme(dbg_theme[2]);
            if(dbg_inst) kprintf("%8x '%A'", dbg_label, dbg_label);
            else         kprintf("<%s+%x>", s, dbg_label-dbg_lastsym);
        } else
            kprintf("%s", inst);
        kprintf("\n");
    }
}

void dbg_data(virt_t ptr)
{
    tcb_t *tcb = (tcb_t*)0;
    uint64_t i, x, y, offs, line;
    uint8_t *p = (uint8_t*)ptr;
    volatile virt_t *tlb=(virt_t*)LDYN_tmpmap3, page, memroot;

    while(ky < maxy-(dbg_full?2:5)) {
        kx = 1;
        if((virt_t)p >= LDYN_tmpmap1 && (virt_t)p < LDYN_tmpmap3)
            kprintf("phy %6x: ", dbg_dataphy + (virt_t)p - LDYN_tmpmap1);
        else
            kprintf("%8x: ", p);
        if(platform_memfault(p)) {
            dbg_settheme(dbg_theme[2]);
            kprintf(" * not available *");
            offs = ky*font->height*bootboot.fb_scanline + kx*(font->width+1)*4;
            for(y=0;y<font->height;y++){
                line=offs;
                for(x=kx*(font->width+1);x<bootboot.fb_width;x++){
                    *((uint32_t*)(FBUF_ADDRESS + line)) = bg;
                    line+=4;
                }
                offs+=bootboot.fb_scanline;
            }
            kprintf("\n");
            dbg_settheme(dbg_theme[3]);
            p += dbg_unit==4? 8 : 16;
        } else {
            dbg_indump = 2;
            switch(dbg_unit) {
                case 0:
                    for(i=0; i<16; i++, p++) { kprintf("%1x ", *p); if(i%4 == 3) { kprintf_putchar(' '); kx++; } }
                    p -= 16;
                    for(i=0; i<16; i++, p++) { kprintf("%c", *p); }
                    break;
                case 1:
                    for(i=0; i<8; i++, p += 2) { kprintf("%2x ", *((uint16_t*)p)); if(i%4 == 3) { kprintf_putchar(' '); kx++; } }
                    p -= 16;
                    for(i=0; i<8; i++, p += 2) { kprintf("%c%c ", p[1], p[0]); }
                    break;
                case 2:
                    for(i=0; i<4; i++, p += 4) { kprintf("%4x ", *((uint32_t*)p));  if(i%4 == 3) { kprintf_putchar(' '); kx++; } }
                    p -= 16;
                    for(i=0; i<4; i++, p += 4) { kprintf("%c%c%c%c ", p[3], p[2], p[1], p[0]); }
                    break;
                case 3:
                    for(i=0; i<2; i++, p += 8) { kprintf("%8x ", *((uint64_t*)p)); }
                    p -= 16;
                    for(i=0; i<2; i++, p += 8) { kprintf("%c%c%c%c%c%c%c%c ", p[7], p[6], p[5], p[4], p[3], p[2], p[1], p[0]); }
                    break;
                case 4:
                    page = *((uint64_t*)p);
                    kprintf("%8x  %c%c%c%c%c%c%c%c ", page, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
                    dbg_settheme(dbg_theme[2]);
                    if(page > __SLOTSIZE/2 && page <= TEXT_ADDRESS) kprintf("    <STACK-%x>", TEXT_ADDRESS-page);
                    else {
                        page = elf_sym(page, false);
                        if(page) { kprintf("    <%s+%x>", (char*)page, *((uint64_t*)p)-dbg_lastsym); }
                    }
                    dbg_settheme(dbg_theme[3]);
                    p += 8;
                    break;
            }
            dbg_indump = 1;
            kprintf("\n");
        }
    }

    if(!dbg_full) {
        if(!(ptr >= LDYN_tmpmap1 && ptr < LDYN_tmpmap3)) {
            vmm_getmemroot(ptr);
            kprintf("\n512G: %8x %4d ", memroot, (ptr>>30)&511);
            dbg_pagingflags(memroot);
            kprintf("  1G: ");
            vmm_page(0, LDYN_tmpmap3, vmm_phyaddr(memroot), PG_CORE_RWNOCACHE | PG_PAGE);
            if(platform_memfault((uint8_t*)LDYN_tmpmap3)) goto end;
            page = tlb[(ptr>>30)&511];
            kprintf("%8x %4d ", page, (ptr>>21)&511);
            dbg_pagingflags(page);
            kprintf("\n  2M: ");
            vmm_page(0, LDYN_tmpmap3, vmm_phyaddr(page), PG_CORE_RWNOCACHE | PG_PAGE);
            if(platform_memfault((uint8_t*)LDYN_tmpmap3)) goto end;
            page = tlb[(ptr>>21)&511];
            kprintf("%8x %4d ", page, (ptr>>12)&511);
            dbg_pagingflags(page);
            kprintf("  4k: ");
            vmm_page(0, LDYN_tmpmap3, vmm_phyaddr(page), PG_CORE_RWNOCACHE | PG_PAGE);
            if(platform_memfault((uint8_t*)LDYN_tmpmap3)) {
end:            dbg_settheme(dbg_theme[2]); kprintf(" * not mapped *\n"); dbg_settheme(dbg_theme[3]);
            } else {
                page = tlb[(ptr>>12)&511];
                kprintf("%8x %4d ", page, ptr&(__PAGESIZE-1));
                dbg_pagingflags(page);
                kprintf("\n");
            }
        } else {
            offs = font->height*bootboot.fb_scanline*ky; page = ((uint64_t)bg<<32) | bg;
            for(y = ky*font->height; y<(unsigned int)(maxy-1)*font->height; y++) {
                line = offs;
                for(x=0; x < bootboot.fb_width; x+=2) { *((uint64_t*)(FBUF_ADDRESS + line)) = (uint64_t)page; line += 8; }
                offs += bootboot.fb_scanline;
            }
            dbg_settheme(dbg_theme[2]); kprintf("\n   * no mapping *\n"); dbg_settheme(dbg_theme[3]);
        }
    }
}

void dbg_msg(uint idx)
{
    msghdr_t *msghdr = (msghdr_t*)__PAGESIZE;
    msg_t *msg = (msg_t*)(__PAGESIZE + idx*sizeof(msg_t));
    uint i;

    dbg_settheme(dbg_theme[4]);
    if(!dbg_full) {
        kprintf("[Queue Header]\n"); dbg_settheme(dbg_theme[3]);
        kprintf("msg in: %3d, msg out: %3d, serial: %2x, processed total: %d\n",
            msghdr->mq_start, msghdr->mq_end, msghdr->mq_serial, msghdr->mq_total);
        kprintf("buf in: %3d, buf out: %3d, buf size: %x\n", msghdr->mq_bufstart, msghdr->mq_bufend, __SLOTSIZE - MQ_BUF);
        dbg_settheme(dbg_theme[4]);
        kprintf("\n");
    }

    kprintf("[Messages]\n"); dbg_settheme(dbg_theme[3]);
    for(i = idx; ky < maxy-2; i++, msg++) {
        if(i >= MQ_MAX) { i = 1; msg = (msg_t*)(__PAGESIZE + sizeof(msg_t)); }
        dbg_settheme(dbg_theme[i == msghdr->mq_start? 6 : 3]);
        kprintf(" %3d: ", i);
        if((msghdr->mq_start <= msghdr->mq_end && (i < msghdr->mq_start || i >= msghdr->mq_end)) ||
            (i < msghdr->mq_start && i >= msghdr->mq_end) || msg->evt & MSG_RESPONSED) {
                dbg_settheme(dbg_theme[2]); kprintf(" * empty slot *\n");
        } else {
            kprintf("%4x from %3x evt #%2x(", msg->serial, EVT_SENDER(msg->evt), EVT_FUNC(msg->evt));
            kprintf(msg->evt & MSG_PTRDATA? "*%x[%d]" : "%x,%x", msg->data.scalar.arg0, msg->data.scalar.arg1);
            kprintf(",%x,%x,%x,%x)\n", msg->data.scalar.arg2, msg->data.scalar.arg3, msg->data.scalar.arg4, msg->data.scalar.arg5);
            if(msg->evt & MSG_PTRDATA) kprintf("      %1D", msg->data.buf.ptr);
        }
    }
}

void dbg_tcb()
{
    tcb_t *tcb = (tcb_t*)0;
    char *states[] = { "hybernated", "blocked", "running" };
    uuid_t *ace = (uuid_t*)&tcb->owner;
    uint i;

    if(!dbg_full) {
        dbg_settheme(dbg_theme[4]); kprintf("[Task Control Block]\n"); dbg_settheme(dbg_theme[3]);
        kprintf("pid: %6x, cpu: %d, flags: %1x%s, owner: %s\nexecutable: %s\n\n", tcb->pid, tcb->cpuid, tcb->flags,
            tcb->tracebuf?" + trace":"", &tcb->owner, tcb->cmdline);

        dbg_settheme(dbg_theme[4]); kprintf("[Scheduler]\n"); dbg_settheme(dbg_theme[3]);
        kprintf("priority: %s (%d), state: %s (%1x), next alarm: %d usec\n", prio[tcb->priority], tcb->priority,
            states[tcb->state], tcb->state, tcb->alarmusec);
        kprintf("parent: %6x (%dth), prev: %6x, next %6x\n", tcb->parent, tcb->forklevel, tcb->prev, tcb->next);
        kprintf("sendto: %6x, blocked since: %d usec, total: %d usec\n\n", tcb->blksend, clock_ticks-tcb->blktime, tcb->blkcnt);

        dbg_settheme(dbg_theme[4]); kprintf("[Access Control List]\n"); dbg_settheme(dbg_theme[3]);
        for(i = 0; ky < maxy-2 && ace->Data1 && ace->Data4[7]; i++, ace++)
            kprintf("%4x-%2x-%2x-%1x%1x%1x%1x%1x%1x%1x:%c%c%c%c%c%c%c%s", ace->Data1, ace->Data2, ace->Data3,
                ace->Data4[0], ace->Data4[1], ace->Data4[2], ace->Data4[3], ace->Data4[4], ace->Data4[5], ace->Data4[6],
                ace->Data4[7]&A_READ?'r':'-', ace->Data4[7]&A_WRITE?'w':'-', ace->Data4[7]&A_EXEC?'x':'-',ace->Data4[7]&A_APPEND?
                'a':'-',ace->Data4[7]&A_DELETE?'d':'-',ace->Data4[7]&A_SUID?'s':'-',ace->Data4[7]&A_SGID?'i':'-', i&1?" ":"\n");
        kprintf("%s\n", !i || i&1? "" : "\n");
    }

    dbg_settheme(dbg_theme[4]);
    if(tcb->state == TCB_STATE_HYBERND) {
        kprintf("[Hybernation Info]\n"); dbg_settheme(dbg_theme[3]); kprintf("swapped to: %8x\n", tcb->swapid);
    } else {
        kprintf("[Task Memory]\n"); dbg_settheme(dbg_theme[3]);
        kprintf("mapping: %8x, allocated pages: %7d, linked pages: %7d\n\n", tcb->memroot, tcb->allocmem, tcb->linkmem);
        if(tcb->memroot && tcb->allocmem < 0x100000)
            dbg_bztdump((uint64_t*)DYN_ADDRESS);
        else
            kprintf("Bogus memory map\n");
    }
}

void dbg_sched(uint cpuid)
{
    ccb_t *ccb = (ccb_t*)(CCBS_ADDRESS + (cpuid << __PAGEBITS));
    maxy -= 2;
    if(!dbg_full) {
        dbg_settheme(dbg_theme[4]);kprintf("[CPU Control Block #%d / %d]\n",cpuid,numcores-1);dbg_settheme(dbg_theme[3]);
        kprintf("scheduled pid: %3x", ccb->current); dbg_dumpccb(ccb);
        dbg_settheme(dbg_theme[4]); kprintf("\n\n[Scheduler Queues]\n"); dbg_settheme(dbg_theme[3]); sched_dump(cpuid);
    }

    dbg_settheme(dbg_theme[4]); kprintf("[CPU Global Memory]\n"); dbg_settheme(dbg_theme[3]);
    kx = fx = 0;
    dbg_bztdump((uint64_t*)CDYN_ADDRESS);
    maxy += 2;
}

void dbg_ram(uint rampos)
{
    pmm_entry_t *fmem = (pmm_entry_t*)(&pmm_entries_buf + rampos*sizeof(pmm_entry_t));
    uint64_t allocmem = pmm_totalpages - pmm_freepages, m = pmm_totalpages+1, n;
    uint i;
    char unit;

    dbg_settheme(dbg_theme[4]);
    if(!dbg_full) {
        kprintf("[Physical Memory Manager]\n"); dbg_settheme(dbg_theme[3]);
        kprintf("total: %9d pages, allocated: %9d pages, free: %9d pages\n", pmm_totalpages, allocmem, pmm_freepages);
        if(!dbg_tui) { dbg_putchar('['); } bg = dbg_theme[2]; dbg_settheme(dbg_theme[2]);
        for(i = 0; i < allocmem*(maxx-9)/m; i++) { kprintf_putchar('#'); kx++; }
        bg = dbg_theme[0]; dbg_settheme(dbg_theme[0]);
        for(; i < (uint)(maxx-9); i++) { kprintf_putchar('_'); kx++; }
        bg = dbg_theme[1]; dbg_settheme(dbg_theme[3]); if(!dbg_tui) dbg_putchar(']');
        kprintf(" %2d.%d%d%%\n\n", allocmem*100/m, (allocmem*1000/m)%10, (allocmem*10000/m)%10);
        dbg_settheme(dbg_theme[4]);
    }

    kprintf("[Free Memory Fragments #%d]\n", pmm_size); dbg_settheme(dbg_theme[3]);
    for(i = rampos; i < pmm_size && ky < maxy-2; i++, fmem++) {
        n = fmem->size << __PAGEBITS; unit = ' ';
        if(n >= GBYTE) { unit='G'; n /= GBYTE; } else
        if(n >= MBYTE) { unit='M'; n /= MBYTE; } else
        if(n >= KBYTE) { unit='k'; n /= KBYTE; }
        kprintf("  %8x - %8x, %11d pages %5d %c\n", fmem->base, fmem->base + (fmem->size << __PAGEBITS) - 1, fmem->size, n, unit);
    }
}

void dbg_sysinfo(virt_t ptr)
{
    uint i;

    if(!dbg_full) {
        dbg_settheme(dbg_theme[4]); kprintf("[Operating System]\n"); dbg_settheme(dbg_theme[3]); kprintf("%s",&osver);
        kprintf("system time: %d.%d sec, uptime: %d usec\n", clock_ts, clock_ns, clock_ticks);
        kprintf("intr: %s, quantum: %d usec, clock: %d Hz, debug flags: %x, boot flags: %x\ndisplay: %d (PBT_%s), driver: %s %s\n",
            intr_name, quantum, clock_freq, debug, flags, display, disp[display], display_drv+17, display_env);
        kprintf("framebuffer: @%8x, %dx%d, scanline %d, type: %d %s\n",
            bootboot.fb_ptr, bootboot.fb_width, bootboot.fb_height, bootboot.fb_scanline,bootboot.fb_type,fbt[bootboot.fb_type]);
        kprintf("random seed: %8x%8x%8x%8x\n\n", srand[0], srand[1], srand[2], srand[3]);
        dbg_settheme(dbg_theme[4]); kprintf("[System Tables and Pointers]\n"); dbg_settheme(dbg_theme[3]);
        for(i = 0; i < 8 && dbg_systables[i]; i++)
            kprintf("%s%6s: %8x", !i||(i%3)? "  " : "\n  ", dbg_systables[i], systables[i]);
        dbg_settheme(dbg_theme[4]); kprintf("\n\n[Syslog Buffer]\n"); dbg_settheme(dbg_theme[3]);
    }
    fx = kx = 0; maxy -= 2; kprintf("%s", syslog_buffer + ptr); maxy += 2;
    kprintf("\n");
}

void dbg_env(virt_t ptr)
{
    fx = kx = 0; maxy -= 2; kprintf("%s", environment + ptr); maxy += 2;
    kprintf("\n");
}

/**
 * parancsértelmező
 */
int dbg_parsecmd(char *cmd, uint64_t cmdlast)
{
    uint64_t x, y, m, l;

    if(cmdlast==0 || (cmd[0]=='s'&&cmd[1]!='y') || (cmd[0]=='c' && cmd[1]!='c')) {                              /* step, continue */
        if(cmd[0]=='s') { dbg_step=1; } else if(cmd[0]=='c') { dbg_step=0; } dbg_singlestep(dbg_step); return 2; } else
    if(cmd[0]=='r' && cmd[1]=='e') { dbg_indump = false; kprintf_reboot(); } else                                /* reset, reboot */
    if(cmd[0]=='q'||(cmd[0]=='h' && cmd[1]=='a')) { dbg_indump = false; kprintf_poweroff(); } else                  /* quit, halt */
    if(cmd[0]=='h') {x=0;while(x<cmdlast&&cmd[x]!=' '){x++;}if(cmd[x]!=' ')dbg_help();else dbg_help2();return 1;}else     /* help */
    if(cmd[0]=='t') { if(cmd[1]=='u') dbg_tui = 1-dbg_tui; else dbg_tab = tab_tcb; return 1; }                        /* tcb, tui */
    if(cmd[0]=='m') { dbg_tab = tab_msg; return 1; } else                                                             /* messages */
    if((cmd[0]=='c'&&(cmd[1]=='p'||cmd[1]=='c'))||(cmd[0]=='s'&&cmd[1]=='c')) { dbg_tab = tab_sched; return 1; } else /* cpu, ccb */
    if(cmd[0]=='r') { dbg_tab = tab_ram; return 1; } else                                                                  /* ram */
    if(cmd[0]=='l' || (cmd[0]=='s' && cmd[1]=='y')) { dbg_tab = tab_sysinfo; return 1; } else                          /* sysinfo */
    if(cmd[0]=='e' && cmd[1]=='n') { dbg_tab = tab_env; return 1; } else                                           /* environment */
    if(cmd[0]=='f') { dbg_full=1-dbg_full; return 1; } else                                                               /* full */
    if(cmd[0]=='n') { dbg_switchnext(); return 4; } else                                                                  /* next */
    if(cmd[0]=='p') { x=0; while(x < cmdlast && cmd[x] != ' ') x++;                                               /* prev / pid X */
        if(cmd[x] == ' ') {
            while(x < cmdlast && cmd[x] == ' ') x++;
            y = 0; env_hex((unsigned char*)&cmd[x], (uint64_t*)&y, 0, 0);
            vmm_page(0, LDYN_tmpmap3, y << __PAGEBITS, PG_CORE_RWNOCACHE|PG_PAGE);
            if(!y || ((tcb_t*)LDYN_tmpmap3)->magic != OSZ_TCB_MAGICH) { dbg_err = "Invalid pid"; return 0; }
            vmm_switch(((tcb_t*)LDYN_tmpmap3)->memroot);
        } else
            dbg_switchprev();
        return 4;
    } else
    if(cmd[0]=='i' || cmd[0]=='d') { x = y = 0; while(x<cmdlast && cmd[x]!=' ') x++;                       /* instruction, disasm */
        if(cmd[x] == ' ') {
            while(x < cmdlast && cmd[x] == ' ') x++;
            dbg_codepos = dbg_getaddr(&cmd[x], cmdlast-x, dbg_codepos); if(dbg_err) return 0;
            y=1;
        }
        if(dbg_tab != tab_code || y) dbg_tab = tab_code; else dbg_inst = 1 - dbg_inst;
        return 1;
    } else
    if(cmd[0]=='x'||cmd[0]=='/'||(cmd[0]=='e'&&cmd[1]=='x')) { dbg_tab = tab_data;                                     /* examine */
        x = 0; while(x < cmdlast && cmd[x] != ' ' && cmd[x] != '/') x++;
        while(x < cmdlast && cmd[x] == ' ') x++;
        if(cmd[x] == '/') { x++;
            switch(cmd[x]) {
                case '1': case 'b': dbg_unit = 0; break;
                case '2': case 'w': dbg_unit = 1; break;
                case '4': case 'd': case 'l': dbg_unit = 2; break;
                case '8': case 'q': case 'g': dbg_unit = 3; break;
                case 's': dbg_unit = 4; break;
                default: dbg_err="Unknown unit flag"; return 0;
            }
            while(x < cmdlast && cmd[x] != ' ') x++;
        }
        y = 0;
        while(x < cmdlast && cmd[x] == ' ') x++;
        if(x < cmdlast) {
            dbg_datapos = dbg_getaddr(&cmd[x], cmdlast-x, dbg_datapos); if(dbg_err) return 0;
            if(cmd[1]=='p') {
                dbg_dataphy = vmm_phyaddr(dbg_datapos);
                vmm_page(0, LDYN_tmpmap1, dbg_dataphy, PG_CORE_RWNOCACHE|PG_PAGE);
                vmm_page(0, LDYN_tmpmap2, dbg_dataphy+__PAGESIZE, PG_CORE_RWNOCACHE|PG_PAGE);
                dbg_datapos = LDYN_tmpmap1 + (dbg_datapos&(__PAGESIZE-1));
            }
        }
        return 3;
    } else
    if(cmd[0]=='b') { x=0; while(x<cmdlast && cmd[x]!=' ' && cmd[x]!='/') {x++;} while(x<cmdlast && cmd[x]==' '){x++;}   /* break */
        m = l = 0;
        if(cmd[x] == '/') { x++;
            while(x < cmdlast && cmd[x] != ' ') {
                switch(cmd[x]) {
                    case '1': case 'b': l=1; break;
                    case '2': l=2; break;
                    case '4': case 'd': case 'l': l=4; break;
                    case '8': case 'q': case 'g': l=8; break;
                    case 'r': m=1; if(!l) l=8; break;
                    case 'w': m=2; if(!l) l=8; break;
                    case 'a': m=3; if(!l) l=8; break;
                    case 'p': m=4; if(!l) l=1; break;
                    default:  m=5; break;
                }
                x++;
            }
            while(x < cmdlast && cmd[x] == ' ') x++;
        }
        y = dbg_getaddr(&cmd[x], cmdlast-x, 0); if(dbg_err) return 0;
        bg = dbg_theme[2]; dbg_settheme(dbg_theme[5]);
        dbg_putchar('\n');
        dbg_brk(y, m, l);
        if(!y && !m && !l) dbg_getkey();
        return 1;
    } else
        dbg_err = "Unknown command";
    return 0;
}

/**
 * beépített debugger
 */
void dbg_start(char *header, bool_t ispanic)
{
    tcb_arch_t *tcb = (tcb_arch_t*)0;
    msghdr_t *msghdr = (msghdr_t*)__PAGESIZE;
    uint64_t offs, line, x, y, m, l, len, cmdidx = 0, cmdlast = 0, currhist = sizeof(cmdhist), *ptr, *end;
    uint16_t c, i;
    uint8_t lasttab = tab_last;
    phy_t memroot;
    char *name, cmd[64];

    /* egyszerre csak egy CPU-n futhat */
    if(dbg_indump) return;
    dbg_indump = true;
    cnt = reent = 0;

    /* platform specifikus dolgok */
    dbg_init();

    for(x=0; x<7; x++) {
        kprintf_fg(ispanic? theme_panic[x] : theme_debug[x]);
        dbg_theme[x] = fg;
    }

    kprintf_fade();
    offs = font->height*bootboot.fb_scanline;
    for(y = 0; y < 2*font->height; y++){
        line = offs;
        for(x = 0; x < bootboot.fb_width; x += 2){ *((uint64_t*)(FBUF_ADDRESS + line))=(uint64_t)0; line += 8; }
        offs += bootboot.fb_scanline;
    }
    kx = ky = fx = 0; scry = -1;
    fg = dbg_theme[6]; bg = 0;
    kprintf("OS/Z debug: %s\n", header);

    dbg_rampos = dbg_logpos = dbg_envpos = 0;
    memroot = tcb->common.memroot;

reload:
    /* aktuális címtér adatainak betöltése */
    for(name = (char*)((virt_t)tcb->common.cmdline + strlen((char*)(virt_t)tcb->common.cmdline)-1);
        (virt_t)name > (virt_t)tcb->common.cmdline && *(name-1)!='/'; name--);
    dbg_cpuid = tcb->common.pid == idle_tcb.pid ? ((ccb_t*)LDYN_ccb)->id : tcb->common.cpuid;
    len = (dbg_cpuid>99?3:(dbg_cpuid>9?2:1)) + (tcb->common.pid>0xff? (tcb->common.pid>0xffff?8:2) :1) + strlen(name);
    dbg_codepos = tcb->pc;
    dbg_lastpos = tcb->lastpc;
    if(tcb->lastpc && tcb->lastpc < dbg_codepos && tcb->lastpc + 32 > dbg_codepos) dbg_codepos = tcb->lastpc;
    tcb->lastpc = tcb->pc;
    dbg_datapos = tcb->sp;
    dbg_btidx = dbg_inst = 0;
    memset(&dbg_bt, 0, sizeof(dbg_bt));
    dbg_bt[0] = tcb->pc;
    ptr = (uint64_t*)tcb->sp;
    end = ptr + 256;
    for(i = 1; i < 8 && (virt_t)ptr < TEXT_ADDRESS && ptr < end; ptr++)
        if(*ptr > TEXT_ADDRESS && *ptr < DYN_ADDRESS && elf_sym(*ptr, true)) dbg_bt[i++] = *ptr;
    dbg_mqpos = msghdr->mq_start;
    if(dbg_mqpos < 1) dbg_mqpos = 1;

    /* debugger fő ciklus */
    while(1) {
        /* fejléc */
        if(dbg_tui) {
            kx = ky = fx = 0; dbg_setpos();
            dbg_putchar(27); dbg_putchar('['); dbg_putchar('0'); dbg_putchar('m');
            dbg_putchar(27); dbg_putchar('['); dbg_putchar('2'); dbg_putchar('J');
            bg = 0; dbg_settheme(dbg_theme[6]);
            kprintf("OS/Z debug: %s\n", header);
        } else
            dbg_putchar('\n');
        kx = 0; ky = 1;
        /* tabok */
        for(x=0; x<tab_last; x++) {
            kx++;
            bg = (dbg_tab == x? dbg_theme[1] : 0); dbg_settheme(dbg_theme[dbg_tab == x? 5 : 2]);
            kprintf_putchar(' '); kx++;
            dbg_putchar(dbg_tab == x? '[' : ' '); kprintf(tabs[x]); dbg_putchar(dbg_tab == x? ']' : ' ');
            kprintf_putchar(' '); kx++;
        }
        dbg_putchar('\n');
        /* tab terület törlése. Ez csúnya, mert emulátoron villog tőle a kép, de sajnos muszáj */
        bg = dbg_theme[1];
        offs = font->height*bootboot.fb_scanline*2; m = ((uint64_t)bg<<32) | bg;
        for(y = 2*font->height; y<(unsigned int)((dbg_tab!=lasttab||dbg_tab!=tab_data||dbg_unit==4)?maxy-1:3)*font->height; y++){
            line = offs;
            for(x=0; x < bootboot.fb_width; x+=2) { *((uint64_t*)(FBUF_ADDRESS + line)) = (uint64_t)m; line += 8; }
            offs += bootboot.fb_scanline;
        }
        lasttab = dbg_tab;
        /* tab tartalma */
        fx = kx = 1; ky = 3; dbg_setpos(); dbg_settheme(dbg_theme[3]);
        switch(dbg_tab) {
            case tab_code: dbg_code(dbg_codepos); break;
            case tab_data: dbg_data(dbg_datapos); break;
            case tab_msg: dbg_msg(dbg_mqpos); break;
            case tab_tcb: dbg_tcb(); break;
            case tab_sched: dbg_sched(dbg_cpuid); break;
            case tab_ram: dbg_ram(dbg_rampos); break;
            case tab_sysinfo: dbg_sysinfo(dbg_logpos); break;
            default: dbg_env(dbg_envpos); break;
        }

        /* parancssor */
        ky = maxy-1;
        offs = ky * font->height*bootboot.fb_scanline;
        for(y = ky * font->height; y < bootboot.fb_height; y++){
            line = offs;
            for(x = 0; x < bootboot.fb_width; x += 2){ *((uint64_t*)(FBUF_ADDRESS + line)) = (uint64_t)0; line += 8; }
            offs += bootboot.fb_scanline;
        }
        dbg_err = NULL;
getcmd: dbg_putchar('\r');
        ky = maxy-1;
        kprintf_fg(0x404040); kprintf_bg(0); dbg_settheme(fg);
        kx = maxx-5 - len;
        if(dbg_tui) { dbg_setpos(); dbg_putchar(27); dbg_putchar('['); dbg_putchar('2'); dbg_putchar('K'); }
        kprintf("%s %x/%d%c", name, tcb->common.pid, dbg_cpuid, dbg_tui? '\r' : ' ');
        fx = kx = 0;
        ky = maxy-1; kprintf_fg(0x808080); dbg_settheme(fg);
        cmd[cmdlast] = 0;
        kprintf("dbg> %s",cmd);
        offs = ky * font->height*bootboot.fb_scanline + kx * (font->width + 1) * 4;
        l = (maxx - 5 - len) * (font->width + 1);
        for(y = ky * font->height; y < bootboot.fb_height; y++){
            line = offs;
            for(x = kx * (font->width + 1); x < l; x += 2){ *((uint64_t*)(FBUF_ADDRESS + line)) = (uint64_t)0; line += 8; }
            offs += bootboot.fb_scanline;
        }
        if(dbg_err != NULL) {
            kprintf_fg(0x800000); dbg_settheme(fg); kprintf("  * %s *", dbg_err); kprintf_fg(0x808080); dbg_settheme(fg);
            dbg_err = NULL;
        }
        kprintf_fg(0); kprintf_bg(0x404040);
        kx = 5+cmdidx;
        x = (dbg_tui? 5 : len + 9) + cmdidx;
        if(!dbg_tui) { dbg_putchar(' '); dbg_putchar(' '); dbg_putchar(' '); dbg_putchar(8); dbg_putchar(8); dbg_putchar(8); }
        else {
            dbg_putchar('\r'); dbg_putchar(27); dbg_putchar('[');
            dbg_putchar('0'+((x/100)%10)); dbg_putchar('0'+((x/10)%10)); dbg_putchar('0'+(x%10)); dbg_putchar('C');
        }
        x = cmdidx == cmdlast || cmd[cmdidx] == 0? ' ' : cmd[cmdidx];
        kprintf_putchar(x);
        dbg_putchar(8);
        if(dbg_tui) { dbg_putchar(27); dbg_putchar('['); dbg_putchar('0'); dbg_putchar('m'); }

        c = dbg_getkey();
        switch(c) {
            case key_f1:
                dbg_help();
                dbg_help2();
                lasttab = -1;
                continue;

            case key_f2:
                bg = dbg_theme[2]; dbg_settheme(dbg_theme[5]);
                dbg_brk(0, 0, 0);
                dbg_getkey();
                lasttab = -1;
                continue;

            case key_f3:
                fx = kx = (maxx-64)/2; ky = (maxy-32)/2;
                bg = dbg_theme[2]; dbg_settheme(dbg_theme[5]);
                if(dbg_tui) dbg_setpos(); else dbg_putchar('\n');
                kprintf_unicodetable();
                if(dbg_tui) { dbg_putchar(27); dbg_putchar('['); dbg_putchar('0'); dbg_putchar('m'); }
                dbg_getkey();
                continue;

            case key_left:
                if(!cmdlast) { dbg_switchprev(); goto reload; }
                if(cmdidx > 0) cmdidx--;
                goto getcmd;

            case key_right:
                if(!cmdlast) { dbg_switchnext(); goto reload; }
                if(cmdidx < cmdlast) cmdidx++;
                goto getcmd;

            case key_up:
                if(dbg_isshft) {
                    if(currhist > 0 && cmdhist[currhist-sizeof(cmd)] != 0) {
                        currhist -= sizeof(cmd);
                        memcpy(&cmd[0], &cmdhist[currhist], sizeof(cmd));
                        cmdidx = cmdlast = strlen(cmd);
                    }
                } else {
                    switch(dbg_tab) {
                        case tab_code: dbg_codepos = dbg_previnst(dbg_codepos); break;
                        case tab_data: dbg_datapos -= (dbg_unit == 4? 8 : 16); break;
                        case tab_msg: dbg_mqpos--; if(!dbg_mqpos) dbg_mqpos = MQ_MAX - 1; continue;
                        case tab_sched: if(!dbg_cpuid) dbg_cpuid = numcores - 1; else dbg_cpuid--; continue;
                        case tab_ram: if(dbg_rampos) dbg_rampos--; continue;
                        case tab_sysinfo: if(dbg_logpos > 0) {
                                for(dbg_logpos--; dbg_logpos > 0 && syslog_buffer[dbg_logpos-1] != '\n'; dbg_logpos--);
                            } continue;
                        case tab_env: if(dbg_envpos > 0) {
                                for(dbg_envpos--; dbg_envpos > 0 && environment[dbg_envpos-1] != '\n'; dbg_envpos--);
                            } continue;
                    }
                    continue;
                }
                goto getcmd;

            case key_pgup:
                switch(dbg_tab) {
                    case tab_code: if(dbg_btidx>0) { dbg_btidx--; dbg_codepos = dbg_bt[dbg_btidx]; continue; } break;
                    case tab_data: l=dbg_isshft? __SLOTSIZE : __PAGESIZE; dbg_datapos = ((dbg_datapos-1)/l)*l; continue;
                    case tab_msg: dbg_mqpos = msghdr->mq_start; if(!dbg_mqpos) dbg_mqpos++; continue;
                }
                goto getcmd;

            case key_down:
                if(dbg_isshft) {
                    if(currhist < sizeof(cmdhist)) {
                        currhist += sizeof(cmd);
                        if(currhist <= sizeof(cmdhist) - sizeof(cmd)){
                            memcpy(&cmd[0], &cmdhist[currhist], sizeof(cmd)); cmdidx = cmdlast = strlen(cmd);
                        } else {
                            cmdidx = cmdlast = 0; cmd[cmdidx] = 0;
                        }
                    }
                } else {
                    switch(dbg_tab) {
                        case tab_code: dbg_codepos = disasm(dbg_codepos, NULL); break;
                        case tab_data: dbg_datapos += (dbg_unit == 4? 8 : 16); break;
                        case tab_msg: dbg_mqpos++; if(dbg_mqpos >= MQ_MAX) dbg_mqpos = 1; continue;
                        case tab_sched: dbg_cpuid++; if(dbg_cpuid >= numcores) dbg_cpuid = 0; continue;
                        case tab_ram: if(dbg_rampos+1 < pmm_size) dbg_rampos++; continue;
                        case tab_sysinfo: if(syslog_buffer[dbg_logpos]) {
                                for(dbg_logpos++; syslog_buffer[dbg_logpos] && syslog_buffer[dbg_logpos-1] != '\n'; dbg_logpos++);
                            } continue;
                        case tab_env: if(environment[dbg_envpos]) {
                                for(dbg_envpos++; environment[dbg_envpos] && environment[dbg_envpos-1] != '\n'; dbg_envpos++);
                            } continue;
                    }
                    continue;
                }
                goto getcmd;

            case key_pgdn:
                switch(dbg_tab) {
                    case tab_code: if(dbg_btidx<7&&dbg_bt[dbg_btidx+1]){dbg_btidx++;dbg_codepos=dbg_bt[dbg_btidx];continue;} break;
                    case tab_data: l=dbg_isshft? __SLOTSIZE : __PAGESIZE; dbg_datapos = ((dbg_datapos+l)/l)*l; continue;
                    case tab_msg: dbg_mqpos = msghdr->mq_end; if(!dbg_mqpos) dbg_mqpos++; continue;
                }
                goto getcmd;

            case key_back:
                if(cmdidx>0) { cmdidx--; memcpy(&cmd[cmdidx],&cmd[cmdidx+1],cmdlast-cmdidx+1); cmdlast--; }
                goto getcmd;

            case key_del:
                if(cmdidx<cmdlast) { memcpy(&cmd[cmdidx],&cmd[cmdidx+1],cmdlast-cmdidx+1); cmdlast--; }
                goto getcmd;

            case key_esc:
                if(cmdlast == 0) goto quitdbg;
                cmdidx = cmdlast = 0;
                cmd[cmdidx] = 0;
                goto getcmd;

            case '\t':
                if(dbg_isshft)  { if(dbg_tab == 0) dbg_tab=tab_last-1; else dbg_tab--; }
                else            { dbg_tab++; if(dbg_tab >= tab_last) dbg_tab = 0; }
                continue;

            case '\n':
                if(cmdlast && memcmp(&cmdhist[sizeof(cmdhist)-sizeof(cmd)], &cmd[0], cmdlast)) {
                    memcpy(&cmdhist[0], &cmdhist[sizeof(cmd)], sizeof(cmdhist)-sizeof(cmd));
                    memcpy(&cmdhist[sizeof(cmdhist)-sizeof(cmd)], &cmd[0], sizeof(cmd));
                }
                currhist = sizeof(cmdhist);
                x = dbg_parsecmd(cmd, cmdlast);
                if(x) { dbg_putchar('\n'); cmdidx = cmdlast = 0; cmd[cmdidx] = 0; lasttab = -1; }
                switch(x) {
                    case 0: goto getcmd;
                    case 1: continue;
                    case 2: goto quitdbg;
                    case 3: lasttab = -1; continue;
                    case 4: goto reload;
                }
                break;

            default:
                if(c == key_redraw) continue;
                if(c) {
                    if(cmdlast >= sizeof(cmd)-1) { dbg_err = "Buffer full"; goto getcmd; }
                    if(cmdidx < cmdlast) { for(x = cmdlast; x > cmdidx; x--) cmd[x] = cmd[x-1]; }
                    cmdlast++;
                    cmd[cmdidx++] = c;
                }
                goto getcmd;
        }
    }

quitdbg:
    if(ispanic) { dbg_err = "Unable to continue"; goto getcmd; }                    /* a pánikot nem lehet folytatni */
    vmm_switch(memroot);                                                            /* visszakapcsolunk az eredeti taszkra */
    dbg_fini();                                                                     /* platform spec. dolgok visszaállítása */
    dbg_indump = false;                                                             /* debugger jelző törlése */
    dbg_putchar('\n');                                                              /* soros vonali terminál "törlése" */
    if(services[-SRV_UI]) msg_notify(SRV_UI, SYS_flush, 0); else kprintf_init();    /* képernyő törlése */
}
