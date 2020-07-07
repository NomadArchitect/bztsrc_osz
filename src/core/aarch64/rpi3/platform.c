/*
 * core/aarch64/rpi3/platform.c
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
 * @subsystem platform
 * @brief Platform specifikus funkciók
 */

#include <platform.h>

extern void armtmr_init();
extern void systmr_init();

volatile uint32_t  __attribute__((aligned(16))) mbox[16];
extern int scry;

/**
 * VideoCore mailbox hívás
 */
uint8_t mbox_call(uint8_t ch, volatile uint32_t *mbox)
{
    uint32_t r;

    do{ cpu_relax; }while(*MBOX_STATUS & MBOX_FULL);
    *MBOX_WRITE = (((uint32_t)((uint64_t)mbox)&~0xF) | (ch&0xF));
    while(1) {
        do{ cpu_relax; }while(*MBOX_STATUS & MBOX_EMPTY);
        r=*MBOX_READ;
        if((uint8_t)(r&0xF)==ch)
            return (r&~0xF)==(uint32_t)((uint64_t)mbox) && mbox[1]==MBOX_RESPONSE;
    }
    return false;
}

/**
 * platform függő környezeti változó alapértékek, env_init() hívja
 */
void platform_env()
{
    /* AUX buffer kiüresítése */
    register unused uint64_t r;
    while((*AUX_MU_LSR&0x01)) r=(uint64_t)(*AUX_MU_IO);
    scry=-1;
}

/**
 * platform függő környezeti értékek értelmezése, env_init() hívja
 */
unsigned char *platform_parse(unsigned char *env)
{
    /* nincs platform specifikus opció */
    return ++env;
}

/**
 * számítógép kikapcsolása, kprintf_poweroff() hívja
 */
void platform_poweroff()
{
    uint64_t r=10000;
    /* AUX buffer kiürítése */
    do{ cpu_relax; }while(r-- && (!(*AUX_MU_LSR&0x40)||(*AUX_MU_LSR&0x01))); *AUX_ENABLE = 0;
    __asm__ __volatile__("dsb sy; isb");
    /* MMU gyorsítótár kikapcsolása */
    __asm__ __volatile__ ("mrs %0, sctlr_el1" : "=r" (r));
    r&=~((1<<12) |   /* I törlése, nincs utasítás gyorsítótár */
         (1<<2));    /* C törlése, nincs adat gyorsítótár se */
    __asm__ __volatile__ ("msr sctlr_el1, %0; isb" : : "r" (r));
    /* eszközök lekapcsolása egyesével */
    for(r=0;r<16;r++) {
        mbox[0]=8*4;
        mbox[1]=MBOX_REQUEST;
        mbox[2]=0x28001;        /* set power state */
        mbox[3]=8;
        mbox[4]=8;
        mbox[5]=(uint32_t)r;    /* device id */
        mbox[6]=0;              /* bit 0: off, bit 1: no wait */
        mbox[7]=0;
        mbox_call(MBOX_CH_PROP,mbox);
    }
    /* GPIO lábak nullába hozása */
    *GPFSEL0 = 0; *GPFSEL1 = 0; *GPFSEL2 = 0; *GPFSEL3 = 0; *GPFSEL4 = 0; *GPFSEL5 = 0;
    *GPPUD = 0;
    for(r=150;r>0;r--) cpu_relax;
    *GPPUDCLK0 = 0xffffffff; *GPPUDCLK1 = 0xffffffff;
    for(r=150;r>0;r--) cpu_relax;
    *GPPUDCLK0 = 0; *GPPUDCLK1 = 0;        /* GPIO konfiguráció hattatása */
    /* SoC (GPU + CPU) kikapcsolása */
    r = *PM_RSTS; r &= ~0xfffffaaa;
    r |= 0x555;    /* a 63-as partíció jelenti a leállítást */
    *PM_RSTS = PM_WDOG_MAGIC | r;
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;

    /* qemu-n nem működik a kikapcsolás, viszont ha -semihosting kapcsolóval indítjuk, akkor ez kikapcsolja */
    __asm__ __volatile__ ("mov w0, 0x18;"       /* x0 = 0x18 angel_SWIreason_ReportException */
        "mov x1, #0x20000; add x1, x1, #0x26;"  /* x1 = 0x20026 ADP_Stopped_ApplicationExit */
        "hlt #0xf000");                         /* semihosting hívás */
}

/**
 * számítógép újraindítása, kprintf_reboot() hívja
 */
void platform_reset()
{
    uint64_t r=10000;
    /* AUX buffer kiürítése */
    do{ cpu_relax; }while(r-- && (!(*AUX_MU_LSR&0x40)||(*AUX_MU_LSR&0x01))); *AUX_ENABLE = 0;
    __asm__ __volatile__("dsb sy; isb");
    /* MMU gyorsítótár kikapcsolása */
    __asm__ __volatile__ ("mrs %0, sctlr_el1" : "=r" (r));
    r&=~((1<<12) |   /* I törlése, nincs utasítás gyorsítótár */
         (1<<2));    /* C törlése, nincs adat gyorsítótár se */
    __asm__ __volatile__ ("msr sctlr_el1, %0; isb" : : "r" (r));
    /* SoC (GPU + CPU) újraindítása */
    r = *PM_RSTS; r &= ~0xfffffaaa;
    *PM_RSTS = PM_WDOG_MAGIC | r;   /* bootolás a 0-ás partícióról */
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
    /* újraindításra nem találtam még qemu-s kerülőmegoldást */
}

/**
 * futtatás felfüggesztése
 */
void platform_halt()
{
    __asm__ __volatile__("1: wfe; b 1b");
}

/**
 * véletlenszám generátor inicializálása
 */
void platform_srand()
{
    register uint64_t i;

    *RNG_STATUS=0x40000; *RNG_INT_MASK|=1; *RNG_CTRL|=1;
    while(!((*RNG_STATUS)>>24)) cpu_relax;
    for(i=0;i<32;i++) {
        srand[i%4]+=*RNG_DATA;
        srand[i%4]^=(uint64_t)(*RNG_DATA)<<(i%3?16:8);
        if(!(i%5)) kentropy();
    }
}

/**
 * korai konzol implementáció, a kprintf használja
 */
uint64_t platform_waitkey()
{
    register uint64_t r=0;

    do{ cpu_relax; }while(*UART0_FR&0x10);r=(char)(*UART0_DR);
    return (r=='\r'?'\n':r)|0x8000;   /* a 15-ös bit jelzi, hogy nem scancode */
}

/**
 * soros vonali debug konzol inicializálása
 */
void platform_dbginit()
{
    /* erre RPi-n akkor is szükségünk van az env_asktime()-hoz, ha DEBUG nélkül fordítjuk */
    uint32_t r;

    *UART0_CR = 0;         /* UART0 kikapcsolása */
    /* órafrekvencia beállítása a konzisztens baud-hoz */
    mbox[0] = 8*4;
    mbox[1] = MBOX_REQUEST;
    mbox[2] = 0x38002;     /* set clock rate */
    mbox[3] = 12;
    mbox[4] = 8;
    mbox[5] = 2;           /* UART clock */
    mbox[6] = 4000000;     /* 4Mhz */
    mbox[7] = 0;           /* set turbo */
    mbox_call(MBOX_CH_PROP,mbox);

    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); /* gpio14, gpio15 */
    r|=(4<<12)|(4<<15);    /* alt0 */
    *GPFSEL1 = r;

    *GPPUD = 0;
    for(r=150;r>0;r--) cpu_relax;
    *GPPUDCLK0 = (1<<14)|(1<<15);
    for(r=150;r>0;r--) cpu_relax;
    *GPPUDCLK0 = 0;        /* GPIO konfiguráció hattatása */

    *UART0_ICR = 0x7FF;    /* interruptok törlése */
    *UART0_IBRD = 2;       /* 115200 baud */
    *UART0_FBRD = 0xB;
    *UART0_LCRH = 3<<5; /* 8n1 */
    *UART0_CR = 0x301;     /* Tx, Rx, FIFO engedélyezése */
}

/**
 * egy bájt küldése a debug konzolra
 */
void platform_dbgputc(uint8_t c)
{
    do{ cpu_relax; }while(*UART0_FR&0x20); *UART0_DR=c;
}
