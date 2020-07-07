/*
 * core/kprintf.c
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
 * @subsystem konzol
 * @brief Felügyeleti szinten futó printf implementáció, korai konzol
 */

#include <arch.h>

extern volatile unsigned char _binary_font_start;
extern uint8_t sys_fault;
extern char libcerrstrs[];
extern virt_t elf_sym(virt_t addr, bool_t onlyfunc);
#if DEBUG
extern uint8_t volatile dbg_indump, dbg_tui;
extern void dbg_setpos();
#endif

sfn_t *font = (sfn_t*)&_binary_font_start;
char tmpstr[33];                    /* átmeneti tároló számformázáshoz */
int fx, kx, ky, maxx, maxy, scry;   /* első oszlop, koordináták, maximumok, szkrollozás */
uint32_t fg, bg;                    /* szinek */
uint8_t volatile cnt, reent;        /* paraméterhossz és rekurzió számláló, a volatile a gcc optimalizáló miatt kell */
uint64_t kprintf_lck = 0;
void stupidgcckprintf(char *fmt, ...);

/* konstans sztringek és ascii-art */
char kpanicrip[] = " @%x ";
char kpanicsym[] = "<%s>  ";
char kpanictlb[] = "paging error resolving %8x at step %d.";
char kpanicsuffix[] =
    "                                                      \n"
    "   __|  _ \\  _ \\ __|   _ \\  \\    \\ |_ _|  __|         \n"
    "  (    (   |   / _|    __/ _ \\  .  |  |  (            \n"
    " \\___|\\___/ _|_\\___|  _| _/  _\\_|\\_|___|\\___|         \n";
char kpanicsuffix2[] =
    "                                                      \n"
    "                       KEEP CALM                      \n"  /* ez egy mém, ne fordítsd le */
    "               AND RESTART YOUR COMPUTER              \n"  /* ezt a lang_init() fordítja és igaztja középre */
    "                                                      \n"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"; /* a középreigazítás hosszabb 3-4 bájtos karakterekkel */
char nullstr[] = "(null)";

extern uint8_t _binary_logo_start;  /* tga, 64x64, rle tömörített, palettás, balra fent irányítottságú */

/**
 * konzol alapállapot. Fehér szín feketén, kurzor a bal felső sarokban
 */
void kprintf_reset()
{
    kx = ky = fx = 0;
    scry = maxy;
    reent = 0;
    fg = 0xC0C0C0;
    bg = 0;
}

/**
 * előtér színének beállítása
 */
void kprintf_fg(uint32_t color)
{
    switch(bootboot.fb_type) {
        case FB_ABGR: fg=((color>>16)&0xff)|(color&0xff00)|((color&0xff)<<16); break;
        default: fg=color; break;
    }
}

/**
 * háttér színének beállítása
 */
void kprintf_bg(uint32_t color)
{
    switch(bootboot.fb_type) {
        case FB_ABGR: bg=((color>>16)&0xff)|(color&0xff00)|((color&0xff)<<16); break;
        default: bg=color; break;
    }
}

/**
 * kurzor beállítása egy középre igazított ablak bal felső sarkába
 */
void kprintf_center(int w, int h)
{
    kx = fx = (maxx-w)/2;
    ky = (maxy-h)/2-1;
}

/**
 * korai konzol inicializálása
 */
void kprintf_init()
{
    uint64_t x, y, line, offs = 0;
    uint8_t *palette = &_binary_logo_start + 0x12;
    uint8_t *data = palette + *((uint8_t*)&_binary_logo_start+5)*3;
    uint8_t rle = 0, cnt = 0;

    kprintf_lck = 0;
    /* képernyő törlése */
    for(y=0;y<bootboot.fb_height;y++){
        line=offs;
        for(x=0;x<bootboot.fb_width/2;x++){
            *((uint64_t*)(FBUF_ADDRESS + line))=(uint64_t)0;
            line+=8;
        }
        offs+=bootboot.fb_scanline;
    }
    /* konzol méretének meghatározása */
    maxx = bootboot.fb_width / (font->width+1);
    maxy = bootboot.fb_height / font->height;
    /* alapértelmezett szín, kurzorpozíció */
    kprintf_reset();

    /* logó megjelenítése sötétebb tónussal */
    offs = ((bootboot.fb_height/2-32) * bootboot.fb_scanline) +
           ((bootboot.fb_width/2-32) * 4);
    for(y=0;y<64;y++){
        line=offs;
        for(x=0;x<64;x++){
            if(!cnt) {
                rle = data[0] & 0x80;
                cnt = (data[0] & 0x7F) + 1;
                data++;
            }
            if(data[0]!=0) {
                /* jobbra shifteljük, hogy sötétebb legyen */
                kprintf_fg( ((palette[data[0]*3+0]>>2)<<0)+
                            ((palette[data[0]*3+1]>>2)<<8)+
                            ((palette[data[0]*3+2]>>2)<<16) );
                *((uint32_t*)(FBUF_ADDRESS + line))=fg;
            }
            if(!--cnt || !rle) data++;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
    fg = 0xC0C0C0;
}

/**
 * képernyő elhomályosítása
 */
void kprintf_fade()
{
    uint64_t offs = 0, line;
    uint32_t x,y, w = maxx*(font->width+1)/2;

    lockacquire(1, &kprintf_lck);
    kprintf_reset();
    /* képernyő sötétítése, két pixelt kezelünk egyszerre */
    for(y=0;y<(unsigned int)maxy*font->height;y++){
        line=offs;
        for(x=0;x<w;x++){
            *((uint64_t*)(FBUF_ADDRESS + line)) =
                (*((uint64_t*)(FBUF_ADDRESS + line))>>2)&0x3F3F3F3F3F3F3F3FUL;
            line+=8;
        }
        offs+=bootboot.fb_scanline;
    }
    lockrelease(1, &kprintf_lck);
}

/**
 * viszlát képernyő és a számítógép újraindítása
 */
void kprintf_reboot()
{
    char *msg=TXT_turnoff&&TXT_turnoff[0]?TXT_turnoff:"TURN OFF YOUR COMPUTER.";

    /* újraindítás kiírása (soros konzolra is) */
    kprintf_init();
#if DEBUG
    dbg_putchar('\n');
#endif
    kprintf("OS/Z %s...\n", TXT_rebooting&&TXT_rebooting[0]?TXT_rebooting:"rebooting");
    /* újraindítás */
    platform_reset();
    /* ha nem sikerült, üzenet megjelenítése és lefagyás */
    fg = 0x29283f;
    kprintf_center(mbstrlen(msg)-2, -8);
    kprintf("%s\n", msg);
#if DEBUG
    dbg_putchar('\n');
#endif
    platform_halt();
}

/**
 * viszlát képernyő és a számítógép kikapcsolása
 */
void kprintf_poweroff()
{
    char *msg=TXT_turnoff&&TXT_turnoff[0]?TXT_turnoff:"TURN OFF YOUR COMPUTER.";

    /* kikapcsolás kiírása (soros konzolra is) */
    kprintf_init();
#if DEBUG
    dbg_putchar('\n');
#endif
    kprintf("OS/Z %s\n", TXT_shutdownfinished&&TXT_shutdownfinished[0]?TXT_shutdownfinished:"shutdown finished.");
    /* kikapcsolás */
    platform_poweroff();

    /* ha nem sikerült, üzenet megjelenítése és lefagyás */
    fg = 0x29283f;
    kprintf_center(mbstrlen(msg)-2, -8);
    kprintf("%s\n", msg);
#if DEBUG
    dbg_putchar('\n');
#endif
    platform_halt();
}

/**
 * pánik képernyő, ha van, akkor meghívja a debuggert
 */
void kpanic(char *reason, ...)
{
    char hdr[128];
    uint32_t x, y;
    uint64_t line, offs;
    uint8_t *palette, *data, rle = 0, cnt = 0;
    virt_t ip;
    va_list args;
    va_start(args, reason);

    /* ha lapfordítási hibát kapunk az üzenet összeállításakor */
    vsprintf(hdr,reason,args);
    /* a legelső libc hibaüzenet az ismeretlen hiba */
    if(sys_fault || !hdr[0]) strncpy(hdr, txt[0]?libcerrstrs:"Unknown reason", sizeof(hdr)-1);
    if((flags & FLAG_SPACEMODE) && runlevel >= RUNLVL_NORM) {
        /* kiírjuk az okot a soros vonali debug konzolra (ha van) és újraindítjuk a gépet */
        kprintf("OS/Z %s: %s", TXT_panic&&TXT_panic[0]?TXT_panic:"panic", hdr);
        kprintf("OS/Z %s...\n", TXT_rebooting&&TXT_rebooting[0]?TXT_rebooting:"rebooting");
        platform_reset();
    }
    /* ha debugger támogatással fordítottuk, akkor itt most pánik esetén jól meghívjuk */
#if DEBUG
    dbg_putchar('\n');
    if(runlevel > RUNLVL_VMEM) {
        dbg_start(hdr, true);
    } else {
        dbg_putchar('\n');
#endif
        /* egyébként csak megjelenítjük a pánik okát */
        kprintf_fade();
        kprintf_fg(0xFFDD33); kprintf_bg(0);
        kprintf("OS/Z %s: %s", TXT_panic&&TXT_panic[0]?TXT_panic:"panic", hdr);
        /* ha már van taszk címterünk, akkor tudjuk azt is, melyik utasítás volt a ludas, és az melyik függvényben volt */
        if(runlevel > RUNLVL_VMEM) {
            ip = ((volatile tcb_arch_t*)0)->pc;
            kprintf(kpanicrip, ip);
            if(((volatile tcb_t*)0)->elfbin) {
                ip = elf_sym(ip, true);
                if(ip) kprintf(kpanicsym, ip);
            }
        }
#if DEBUG
        dbg_putchar('\n'); dbg_putchar('\n');
    }
    dbg_putchar(27); dbg_putchar('['); dbg_putchar('7'); dbg_putchar('m');
#endif
    kprintf_center(54, 5);
    kprintf_fg(0x9c3c1b); kprintf_bg(0x100000);
    kprintf("%s",kpanicsuffix);
    /* logó kirakása vörösre konvertálással */
    palette = &_binary_logo_start + 0x12;
    data = palette + *((uint8_t*)&_binary_logo_start+5)*3;
    offs =
        (((ky-3) * font->height - font->height/2 - (font->height<16?6:0)) * bootboot.fb_scanline) +
        ((fx+46) * (font->width+1) * 4);
    for(y=0;y<64;y++){
        line=offs;
        for(x=0;x<64;x++){
            if(!cnt) {
                rle = data[0] & 0x80;
                cnt = (data[0] & 0x7F) + 1;
                data++;
            }
            if(data[0]!=0) {
                /* vörös felé eltolás */
                kprintf_fg( (((uint8_t)palette[(uint8_t)data[0]*3+0]>>4)<<0)+
                            (((uint8_t)palette[(uint8_t)data[0]*3+1]>>2)<<8)+
                            (((uint8_t)palette[(uint8_t)data[0]*3+2]>>0)<<16) );
                *((uint32_t*)(FBUF_ADDRESS + line))=fg;
            }
            if(!--cnt || !rle) data++;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }
    kprintf_fg(0x500000);
    kprintf("%s",kpanicsuffix2);
    fg = 0xC0C0C0;
#if DEBUG
    dbg_putchar(27); dbg_putchar('['); dbg_putchar('0'); dbg_putchar('m');
#endif
    platform_waitkey();
    kprintf_reboot();
}

/**
 * egy unicode karakter megjelenítése
 */
void kprintf_putchar(int c)
{
    unsigned char *ptr, *chr, *frg;
    unsigned long o, p;
    int i, j, k, l, m, n;

    for(ptr = (unsigned char*)font + font->characters_offs, chr = 0, i = 0; i < 0x110000; i++) {
        if(ptr[0] == 0xFF) { i += 65535; ptr++; }
        else if((ptr[0] & 0xC0) == 0xC0) { j = (((ptr[0] & 0x3F) << 8) | ptr[1]); i += j; ptr += 2; }
        else if((ptr[0] & 0xC0) == 0x80) { j = (ptr[0] & 0x3F); i += j; ptr++; }
        else { if(i == c) { chr = ptr; break; } ptr += 6 + ptr[1] * (ptr[0] & 0x40 ? 6 : 5); }
    }
    if(chr) {
        ptr = chr + 6; o = (unsigned long)FBUF_ADDRESS + ky * font->height * bootboot.fb_scanline + kx * (font->width+1) * 4;
        for(i = n = 0; i < chr[1]; i++, ptr += chr[0] & 0x40 ? 6 : 5) {
            if(ptr[0] == 255 && ptr[1] == 255) continue;
            frg = (unsigned char*)font + (chr[0] & 0x40 ? ((ptr[5] << 24) | (ptr[4] << 16) | (ptr[3] << 8) | ptr[2]) :
                ((ptr[4] << 16) | (ptr[3] << 8) | ptr[2]));
            if((frg[0] & 0xE0) != 0x80) continue;
            for(; n < ptr[1]; n++, o+= bootboot.fb_scanline)
                for(p = o, l = 0; l < font->width+1; l++, p += 4)
                    *((uint32_t*)p) = bg;
            o += (int)(ptr[1] - n) * bootboot.fb_scanline; n = ptr[1];
            k = ((frg[0] & 0x1F) + 1) << 3; j = frg[1] + 1; frg += 2;
            for(m = 1; j; j--, n++, o += bootboot.fb_scanline) {
                for(p = o, l = 0; l < k; l++, p += 4, m <<= 1) {
                    if(m > 0x80) { frg++; m = 1; }
                    *((uint32_t*)p) = (*frg & m) ? fg : bg;
                }
                *((uint32_t*)p)=(c==0x9b||c==0x9d||c==0x9e)&&(*frg & 0x80) ? fg : bg;
            }
        }
        for(; n < font->height; n++, o+= bootboot.fb_scanline)
            for(p = o, l = 0; l < font->width+1; l++, p += 4)
                *((uint32_t*)p) = bg;
    }



#if DEBUG
    /* debug konzolra is kiküldjük */
    if(c==0x9a||c==0x9b) c='-';
    if(c==0x9c) c='|';
    if(c==0x9d) c='/';
    if(c==0x9e) c='\\';
    if(c==0x9f) c='>';
    dbg_putchar(c);
#endif
}

/**
 * hexadecimális formázás, %x
 */
static void kprintf_puthex(int64_t c)
{
    int i=16;

    tmpstr[i]=0;
    do {
        char n=c & 0xf;
        tmpstr[--i]=n<10?'0'+n:'A'+n-10;
        c>>=4;
    } while(c!=0&&i>0);
    if(cnt>0&&cnt<=8) {
        while(i>16-cnt*2) {
            tmpstr[--i]='0';
        }
        i=16-cnt*2;
    }
    stupidgcckprintf(&tmpstr[i]);
}

/**
 * ascii érték megjelenítése, %a
 */
static inline void kprintf_putascii(int64_t c)
{
    *((int64_t*)&tmpstr) = c;
    tmpstr[cnt>0&&cnt<=8?cnt:8]=0;
    stupidgcckprintf(&tmpstr[0]);
}

/**
 * hasonló, de a vezérlőkaraktereket kihagyja, %A
 */
static inline void kprintf_dumpascii(int64_t c)
{
    int i;

    *((uint64_t*)&tmpstr) = c;
    if(cnt<1||cnt>8)
        cnt = 8;
    for(i=0;i<cnt;i++)
        if((uchar)tmpstr[i]==0 ||
            ((uchar)tmpstr[i]<' ' && (uchar)tmpstr[i]!=27) ||
            (uchar)tmpstr[i]>=127)
            tmpstr[i]='.';
    tmpstr[cnt]=0;
    stupidgcckprintf(&tmpstr[0]);
}

/**
 * decimálisra formázott szám, %d
 */
static inline void kprintf_putdec(int64_t c)
{
    int64_t i=18,s=c<0;

    if(s) c*=-1;
    if(c>99999999999999999L)
        c=99999999999999999L;
    tmpstr[i]=0;
    do {
        tmpstr[--i]='0'+(c%10);
        c/=10;
    } while(c!=0&&i>0);
    if(s)
        tmpstr[--i]='-';
    if(cnt>0&&cnt<18) {
        while(i>18-cnt) {
            tmpstr[--i]=' ';
        }
        i=18-cnt;
    }
    stupidgcckprintf(&tmpstr[i]);
}

/**
 * bináris formázás, %b
 */
static inline void kprintf_putbin(int64_t c)
{
    int i=0;
    int64_t j=1<<15;

    do {
        tmpstr[i]=(c&j)==j?'1':'0';
        j>>=1;
        i++;
    } while(j!=0);
    tmpstr[i]=0;
    stupidgcckprintf(&tmpstr[0]);
}

/**
 * uuid egyedi azonosító formázás, %U
 */
static inline void kprintf_uuid(uuid_t *mem)
{
    int i;

    cnt=4;
    kprintf_puthex((uint32_t)mem->Data1);
    kprintf_putchar('-'); kx++;
    cnt=2;
    kprintf_puthex((uint16_t)mem->Data2);
    kprintf_putchar('-'); kx++;
    kprintf_puthex((uint16_t)mem->Data3);
    kprintf_putchar('-'); kx++;
    cnt=1;
    for(i=0;i<8;i++)
        kprintf_puthex(mem->Data4[i]);
}

#if DEBUG
/**
 * kirakja az időt a jobb felső sarokban
 */
void kprintf_clock()
{
    uint32_t ox = kx, oy = ky, of = fg;
    ky = 0; kx = maxx - 9; maxx += 2; kprintf_fg(0xFF0000);
    stupidgcckprintf("%d", clock_ts);
    kx = ox; ky = oy; fg = of; maxx -= 2;
    dbg_putchar('\n');
}

/**
 * memória dump, %D
 */
static inline void kprintf_dumpmem(uint8_t *mem)
{
    volatile uint8_t *ptr;
    int i, j, k=cnt, old=dbg_indump;

    if(k<1 || k>128) k=16;
    for(j=0; j<k; j++) {
        cnt = 4;
        kprintf_puthex((int64_t)mem);
        kprintf_putchar(':');kx++;
        kprintf_putchar(' ');kx++;
        ptr = mem; cnt = 1;
        for(i=0; i<16; i++) {
            if(platform_memfault((uint8_t*)ptr)) break;
            kprintf_puthex((uint8_t)*ptr);
            kprintf_putchar(' ');kx++;
            if(i%4 == 3) {
                kprintf_putchar(' ');kx++;
            }
            ptr++;
        }
        if(sys_fault)
            stupidgcckprintf(" * not mapped *");
        else {
            dbg_indump = 2;
            for(i=0, ptr=mem;i<16; i++,kx++,ptr++) {
                kprintf_putchar(*ptr);
            }
            dbg_indump = old;
        }
        mem += 16;
        stupidgcckprintf("\n");
    }
}

/**
 * Taszk Kontroll Blokk dumpolás, %T
 */
static inline void kprintf_dumptcb(uint8_t *mem)
{
    tcb_arch_t *tcb = (tcb_arch_t*)mem;

    kprintf_puthex((uint64_t)mem);
    kprintf_putchar(':');kx++;
    kprintf_putchar(' ');kx++;
    if(platform_memfault(mem)) kprintf(" * not mapped *");
    else {
        if(tcb->common.magic != OSZ_TCB_MAGICH) kprintf(" * not TCB *");
        else {
            stupidgcckprintf("TCB state "); cnt = 2; kprintf_puthex(tcb->common.state);
            stupidgcckprintf(" flags "); cnt = 2; kprintf_puthex(tcb->common.flags);
            stupidgcckprintf(" CPU #"); cnt = 0; kprintf_putdec(tcb->common.cpuid);
            stupidgcckprintf("\n  pid "); cnt = 6; kprintf_puthex(tcb->common.pid);
            stupidgcckprintf(" parent "); cnt = 6; kprintf_puthex(tcb->common.parent);
            stupidgcckprintf(" forklvl "); cnt = 0; kprintf_putdec(tcb->common.forklevel);
            stupidgcckprintf("\n  prio "); cnt = 2; kprintf_putdec(tcb->common.priority);
            stupidgcckprintf(" prev "); cnt = 6; kprintf_puthex(tcb->common.prev);
            stupidgcckprintf(" next "); cnt = 6; kprintf_puthex(tcb->common.next);
            stupidgcckprintf("\n  memroot "); cnt = 8; kprintf_puthex(tcb->common.memroot);
            stupidgcckprintf(" alloc "); cnt = 4; kprintf_putdec(tcb->common.allocmem);
            stupidgcckprintf(" link "); cnt = 4; kprintf_putdec(tcb->common.linkmem);
            stupidgcckprintf("\n  pc "); cnt = 8; kprintf_puthex(tcb->pc);
            stupidgcckprintf(" sp "); cnt = 8; kprintf_puthex(tcb->sp);
            stupidgcckprintf("\n  ELF "); cnt = 4; kprintf_puthex(tcb->common.cmdline);
            stupidgcckprintf(": ");
            mem = (uint8_t*)((virt_t)tcb->common.cmdline + ((virt_t)tcb >= BUF_ADDRESS? BUF_ADDRESS : 0));
            if(platform_memfault(mem)) stupidgcckprintf(" * not mapped *");
            else stupidgcckprintf((char*)mem);
        }
    }
    stupidgcckprintf("\n");
}

/**
 * címfordító tábla dump, %P
 */
static inline void kprintf_dumppagetable(volatile uint64_t *mem)
{
    volatile int64_t c;
    int i, k=cnt;

    if(k<1 || k>128) k=16;
    mem = (uint64_t*)((uint64_t)mem & ~7UL);
    for(i=0; i<k; i++,mem++) {
        cnt=4;
        kprintf_puthex((int64_t)mem);
        kprintf_putchar(':');kx++;
        kprintf_putchar(' ');kx++;
        cnt=8;
        if(platform_memfault((uint8_t*)mem)) break;
        c=(int64_t)(*mem);
        kprintf_puthex(c);
        kprintf_putchar(' ');kx++;
        kprintf_putchar(vmm_ispresent(*mem)?'P':'!');kx++;
        kx = fx; ky++;
        dbg_putchar('\n');
    }
}

/**
 * címfordítás bejárás dump, %p
 */
static inline void kprintf_dumppagewalk(uint64_t *mem)
{
    tcb_t *tcb = (tcb_t*)0;
    virt_t memroot, addr=(virt_t)mem;
    volatile virt_t *ptr=(virt_t*)LDYN_tmpmap7, page;

    vmm_getmemroot(addr);
    cnt=8;
    kprintf_puthex(addr);
    stupidgcckprintf(":\n512G: ");
    kprintf_puthex(memroot);
    kprintf_putchar(' ');kx++;
    kprintf_putchar(vmm_ispresent(memroot)?'P':'!');kx++;
    stupidgcckprintf("\n  1G: ");
    vmm_page(0, LDYN_tmpmap7, vmm_phyaddr(memroot), PG_CORE_RWNOCACHE | PG_PAGE);
    if(platform_memfault((uint8_t*)LDYN_tmpmap7)) goto end;
    page = ptr[(addr>>30)&511];
    cnt=8;
    kprintf_puthex(page);
    kprintf_putchar(' ');kx++;
    kprintf_putchar(vmm_ispresent(page)?'P':'!');kx++;
    stupidgcckprintf("\n  2M: ");
    vmm_page(0, LDYN_tmpmap7, vmm_phyaddr(page), PG_CORE_RWNOCACHE | PG_PAGE);
    if(platform_memfault((uint8_t*)LDYN_tmpmap7)) goto end;
    page = ptr[(addr>>21)&511];
    cnt=8;
    kprintf_puthex(page);
    kprintf_putchar(' ');kx++;
    kprintf_putchar(vmm_ispresent(page)?'P':'!');kx++;
    stupidgcckprintf("\n  4k: ");
    vmm_page(0, LDYN_tmpmap7, vmm_phyaddr(page), PG_CORE_RWNOCACHE | PG_PAGE);
    if(platform_memfault((uint8_t*)LDYN_tmpmap7)) goto end;
    page = ptr[(addr>>12)&511];
    cnt=8;
    kprintf_puthex(page);
    kprintf_putchar(' ');kx++;
    kprintf_putchar(vmm_ispresent(page)?'P':'!');kx++;
end:stupidgcckprintf("\n");
}

/**
 * tesztelésre, unicode kódtábla megjelenítése, 0-2047
 */
void kprintf_unicodetable()
{
    int x,y;

    for(y = 0; y < 32; y++){
        kx = fx;
        for(x = 0; x < 64; x++) { kprintf_putchar(y*64+x); kx++; }
        dbg_putchar('\n'); ky++;
    }
}
#endif

/**
 * képernyő görgetése, és várakozás, ha srcy be van állítva
 */
void kprintf_scrollscr()
{
    uint64_t offs = 0, tmp = (maxy-1)*font->height*bootboot.fb_scanline, arg;
    uint32_t x,y, line, shift=font->height*bootboot.fb_scanline, w = maxx*(font->width+1)/2, oldfg;
    char *msg=TXT_pressakey&&TXT_pressakey[0]?TXT_pressakey:"Press any key to continue";

    /* várakozás */
    if(scry!=-1) {
        scry++;
        /* ha egy egész képernyőnyit görgettünk, megállunk */
        if(scry >= maxy-2) {
            scry = 0;
            oldfg = fg;
            fg = 0xffffff;
            /* üzenet megjelenítése */
            kx = 0; ky = maxy-1;
#if DEBUG
            dbg_putchar('\n');
#endif
            /* nem szabad meghívni ebből a funkcióból a kprintf-et!!! */
            kprintf_putchar(' ');kx++;
            kprintf_putchar('-');kx++;
            kprintf_putchar('-');kx++;
            kprintf_putchar('-');kx++;
            kprintf_putchar(' ');kx++;
            for(;*msg!=0;msg++) {
                    arg=(int)((unsigned char)msg[0]);
                    if((arg & 128) != 0) {
                        if((arg & 32) == 0 ) {
                            arg=((msg[0] & 0x1F)<<6)|(msg[1] & 0x3F);
                            msg++;
                        } else
                        if((arg & 16) == 0 ) {
                            arg=((msg[0] & 0xF)<<12)|((msg[1] & 0x3F)<<6)|(msg[2] & 0x3F);
                            msg+=2;
                        } else
                        if((arg & 8) == 0 ) {
                            arg=((msg[0] & 0x7)<<18)|((msg[1] & 0x3F)<<12)|((msg[2] & 0x3F)<<6)|(msg[3] & 0x3F);
                            msg+=3;
                        } else
                            arg=0;
                    }
                    kprintf_putchar(arg);
                    kx++;
            }
            kprintf_putchar(' ');kx++;
            kprintf_putchar('-');kx++;
            kprintf_putchar('-');kx++;
            kprintf_putchar('-');kx++;
            kprintf_putchar(' ');kx++;
            /* szín visszaállítása */
            fg = oldfg;
            kx = fx; ky--;
             /* várakozás billentyűleütésre. További várakozás kikapcsolása, ha az Esc */
            if(platform_waitkey() == 1)
                scry = -1;
            /* utolsó sor törlése */
            for(y=0; y < font->height; y++){
                line = tmp;
                for(x=0; x < w; x++){
                    *((uint64_t*)(FBUF_ADDRESS + line)) = (uint64_t)0;
                    line += 8;
                }
                tmp += bootboot.fb_scanline;
            }
        }
        /* képernyő görgetés, két pixelt kezelünk egyszerre */
        for(y=0;y<(unsigned int)maxy*font->height;y++){
            line=offs;
            for(x=0;x<w;x++){
                *((uint64_t*)(FBUF_ADDRESS + line)) =
                    *((uint64_t*)(FBUF_ADDRESS + line + shift));
                line+=8;
            }
            offs+=bootboot.fb_scanline;
        }
    } else {
        ky = 0;
    }
}

/**
 * szkrollozás kikapcsolása
 */
void kprintf_scrolloff()
{
    /* ha az eszközmeghajtók detektálását vagy a naplót dumpoltuk, akkor kikényszerítünk egy várakozást */
    if((debug&DBG_LOG) || (debug&DBG_DEVICES)) {
        /* kényszerített várakozás majd szkrollozás */
        scry = 65536;
        kprintf_scrollscr();
    }
    /* várakozás szkrollozáskor kikapcsolása */
    scry = -1;
}

/**
 * aktuális sor törlése
 */
void kprintf_clearline()
{
    uint64_t line, tmp = ky*font->height*bootboot.fb_scanline;
    uint32_t x, y;

#if DEBUG
    if(dbg_indump)
        return;
#endif
    /* clear the row */
    for(y=0;y<font->height;y++){
        line=tmp;
        for(x=0;x<bootboot.fb_width;x+=2){
            *((uint64_t*)(FBUF_ADDRESS + line)) = (uint64_t)0;
            line+=8;
        }
        tmp+=bootboot.fb_scanline;
    }
}

/**
 * szöveg megjelenítése a korai konzolon
 */
void vkprintf(char *fmt, va_list args, bool_t asciiz)
{
    uint64_t arg;
    char *p;

    if(fmt == NULL || (((virt_t)fmt>>48)!=0 && ((virt_t)fmt>>48)!=0xffff))
        fmt = nullstr;
    if(!fmt[0])
        return;
#if DEBUG
    /* a speciális (esc)[UT CSI paranccsal kezdődik? */
    if(*((uint32_t*)fmt)==(uint32_t)0x54555B1B) {
        /* ha igen, megjelenítjük a unicode kódtáblát */
        kprintf_unicodetable();
        return;
    }
#endif
    arg = 0;
    while(fmt[0]!=0 && (asciiz || fmt[0]>=' ')) {
        /* speciális karakterek lekezelése */
        if(fmt[0]==8) {
            /* backspace */
            kx--;
#if DEBUG
            dbg_putchar(8);
#endif
            kprintf_putchar((int)' ');
        } else
        if(fmt[0]==9) {
            /* tab */
            kx=((kx+8)/8)*8;
#if DEBUG
            dbg_putchar(9);
#endif
        } else
        if(fmt[0]==10) {
            /* soremelés */
            goto newline;
        } else
        if(fmt[0]==13) {
            /* kocsivissza */
            kx=fx;
#if DEBUG
            dbg_putchar(13);
#endif
        } else
        /* paraméterek kezelése */
        if(fmt[0]=='%' && !reent) {
            fmt++; cnt=0;
            if(fmt[0]=='%') {
                goto put;
            }
            while(fmt[0]>='0'&&fmt[0]<='9') {
                cnt *= 10;
                cnt += fmt[0]-'0';
                fmt++;
            }
            if(fmt[0]!='s' && fmt[0]!='S')
                arg = va_arg(args, int64_t);
            if(fmt[0]=='c') {
                kprintf_putchar((int)((unsigned char)arg));
                goto nextchar;
            }
            reent++;
            if(fmt[0]=='l') fmt++;
            switch(fmt[0]) {
#if DEBUG
                case 'D': kprintf_dumpmem((uint8_t*)arg); break;
                case 'T': kprintf_dumptcb((uint8_t*)arg); break;
                case 'P': kprintf_dumppagetable((uint64_t*)arg); break;
                case 'p': kprintf_dumppagewalk((uint64_t*)arg); break;
#endif
                case 'a': kprintf_putascii(arg); break;
                case 'A': kprintf_dumpascii(arg); break;
                case 'd': kprintf_putdec(arg); break;
                case 'x': kprintf_puthex(arg); break;
                case 'b': kprintf_putbin(arg); break;
                case 'U': kprintf_uuid((uuid_t*)arg); break;
                case 'S': /* hasonló a %s-hez, de a vezérlőkaraktereknél leáll, nemcsak NUL-nál */
                    p = va_arg(args, char*);
                    if(p==NULL) p = nullstr;
                    arg = 0;
                    if(p[0]) {
                        vkprintf(p, args, false);
                        while(*p >= ' ') { arg++; p++; }
                    }
                    if(arg<cnt) { cnt-=arg; while(cnt-->0) { kprintf_putchar(' '); kx++; } }
                    break;
                case 's':
                    p = va_arg(args, char*);
                    if(p==NULL) p = nullstr;
                    if(p[0]) {
                        vkprintf(p, args, true);
                        arg=strlen(p);
                    } else
                        arg=0;
                    /* szóközzel feltöltés */
                    if(arg<cnt) { cnt-=arg; while(cnt-->0) { kprintf_putchar(' '); kx++; } }
            }
            reent--;
        } else {
            /* az fmt által mutatott utf-8 szekvencia konvertálása unicode arg-ba */
put:        arg=(uint64_t)((unsigned char)fmt[0]);
            if((arg & 128) != 0) {
                if((arg & 32) == 0 ) {
                    arg=((fmt[0] & 0x1F)<<6)|(fmt[1] & 0x3F);
                    fmt++;
                } else
                if((arg & 16) == 0 ) {
                    arg=((fmt[0] & 0xF)<<12)|((fmt[1] & 0x3F)<<6)|(fmt[2] & 0x3F);
                    fmt+=2;
                } else
                if((arg & 8) == 0 ) {
                    arg=((fmt[0] & 0x7)<<18)|((fmt[1] & 0x3F)<<12)|((fmt[2] & 0x3F)<<6)|(fmt[3] & 0x3F);
                    fmt+=3;
                } else
                    arg=0;
            }
            /* unicode karakter megjelenítése és a kurzor mozgatása */
            kprintf_putchar((int)arg);
nextchar:   kx++;
            if(kx>=maxx) {
newline:        kx=fx;
                ky++;
                if(ky>=maxy-(scry==-1?0:1)) {
                    ky--;
#if DEBUG
                    if(dbg_indump)
                        return;
#endif
                    kprintf_scrollscr();
                }
#if DEBUG
                if(dbg_indump && dbg_tui)
                    dbg_setpos();
                else
                    dbg_putchar('\n');
                if(scry==-1 && !dbg_indump)
#else
                if(scry==-1)
#endif
                    kprintf_clearline();
            }
        }
        fmt++;
    }
}

/**
 * segédfüggvény a változó paraméterszámú változathoz
 */
void kprintf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    lockacquire(1, &kprintf_lck);
    vkprintf(fmt, args, true);
    lockrelease(1, &kprintf_lck);
}
/* erre csakis azért van szükség, mert a vkprintf()-nek képtelenség üres va_list-et átadni AArch64-en */
void stupidgcckprintf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vkprintf(fmt, args, true);
}
