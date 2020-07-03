/*
 * core/aarch64/rpi4/intr.c
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
 * @subsystem megszakítás
 * @brief Broadcom GiC Interrupt Controller és ARM Control eszközmeghajtó
 *
 * FIXME: ARM GiC
 */

#include <platform.h>

char intr_name[] = "BCM2711";

/**
 * megszakításvezérlő inicializálása
 */
uint intr_init()
{
    *(IRQ_FIQ_CTRL) = 0;
    /* letiltjuk az összes IRQ-t */
    *(IRQ_DISABLE_1) = 0
        | (1U << 1)         /* IRQ 1 BCM óra #1 */
        | (1U << 3)         /* IRQ 3 BCM óra #3 */
        | (1U << 9)         /* IRQ 9 USB vezérlő */
        | (1U << 29);       /* IRQ29 UART1 (AUX) */
    *(IRQ_DISABLE_2) = 0
        | (1U << (43-32))   /* IRQ43 i2c spi SLV */
        | (1U << (45-32))   /* IRQ45 pwa0 */
        | (1U << (46-32))   /* IRQ46 pwa1 */
        | (1U << (48-32))   /* IRQ48 smi */
        | (1U << (49-32))   /* IRQ49 GPIO #0 */
        | (1U << (50-32))   /* IRQ50 GPIO #1 */
        | (1U << (51-32))   /* IRQ51 GPIO #2 */
        | (1U << (52-32))   /* IRQ52 GPIO #3 */
        | (1U << (53-32))   /* IRQ53 i2c */
        | (1U << (54-32))   /* IRQ54 spi */
        | (1U << (55-32))   /* IRQ55 pcm */
        | (1U << (57-32))   /* IRQ57 UART0 (PL011) */
        | (1U << (61-32))   /* IRQ61 rng */
        | (1U << (62-32));  /* IRQ62 sdhc */

    /* a globális falióra inicializálása, amit valamiért lokálisnak hív az ARM QA7 doksi */
    *(ARMTMR_CS) = 0;                   /* léptetés 1, ütemező pedig a kristály forrás, nem az APB */
    *(ARMTMR_PRESCALE) = 0x80000000;    /* 1-el történő léptetésnél az előosztó */
    *(ARMTMR_LOCALIRQ) = 0;             /* a local timer IRQ-t a 0-ás CPU magnak küldjük, az IRQ vonalára (és nem FIQ-ra) */
    *(ARMTMR_LOCALTMR) = ((1<<28) | ((ARMTMR_FREQ/ARMTMR_STEP) & 0x0fffffff));  /* engedélyezés bit és frekvencia */
    clock_freq = ARMTMR_STEP;
    clock_irq = CLOCK_IRQ;

    sched_irq = SCHED_IRQ;
    return 64;
}

/**
 * visszaadja, melyik IRQ hívódott, ez ARM specifikus, x86-on nincs
 */
uint16_t intr_getirq()
{
    register uint64_t cpuid;
    /* falióra megszakítás */
    if(*(ARMTMR_C0_PEND) & (1<<11)) return CLOCK_IRQ;
    /* CPU magonkénti megszakítások */
    __asm__ __volatile__("mrs %0, mpidr_el1": "=r"(cpuid));
    cpuid &= 3;
    if(*(ARMTMR_PEND(cpuid)) & (1<<1)) return SCHED_IRQ;
    /* általános eszközök megszakításai */
    return 63 - __builtin_clzl(((uint64_t)*(IRQ_PENDING_2) << 32) | *(IRQ_PENDING_1));
}

/**
 * egy adott IRQ vonal engedélyezése
 */
void intr_enable(uint16_t irq)
{
    if(irq<32)
        *(IRQ_ENABLE_1) = (1U << irq);
    else if(irq<64)
        *(IRQ_ENABLE_2) = (1U << (irq-32));
    else if(irq==SCHED_IRQ) {
        *(ARMTMR_C0_ENA) = (1<<1);      /* nCNTPNSIRQ engedélyezése */
        *(ARMTMR_C1_ENA) = (1<<1);
        *(ARMTMR_C2_ENA) = (1<<1);
        *(ARMTMR_C3_ENA) = (1<<1);
    } else if(irq==CLOCK_IRQ) {
        *(ARMTMR_LOCALTMR) |= (1<<29);  /* megszakítás engedélyezés */
    }
}

/**
 * egy adott IRQ vonal letiltása
 */
void intr_disable(uint16_t irq)
{
    if(irq<32)
        *(IRQ_DISABLE_1) = (1U << irq);
    else if(irq<64)
        *(IRQ_DISABLE_2) = (1U << (irq-32));
    else if(irq==SCHED_IRQ) {
        *(ARMTMR_C0_ENA) = 0;
        *(ARMTMR_C1_ENA) = 0;
        *(ARMTMR_C2_ENA) = 0;
        *(ARMTMR_C3_ENA) = 0;
    } else if(irq==CLOCK_IRQ)
        *(ARMTMR_LOCALTMR) &= ~(1<<29);
}

/**
 * generál egy megszakítást usec mikroszekundum múlva
 */
void intr_nextsched(uint64_t usec)
{
    register uint64_t c;
    if(!usec) usec++;
    if(usec>quantum) usec=quantum;
    /* kiszámoljuk a következő megszakítás idejét, és IMASK=0, ENABLE=1 -el elindítjuk a számlálót */
    __asm__ __volatile__("mrs %0, cntpct_el0" : "=r"(c));
    c+=(ARMTMR_FREQ*usec)/1000000;
    __asm__ __volatile__("msr cntp_cval_el0, %0; mov x0, #1; msr cntp_ctl_el0, x0" : : "r"(c) : "x0");
}

/**
 * falióra megszakítás visszaigazolása
 */
void clock_ack()
{
    *(ARMTMR_LOCALCLR) = (1<<31);
}

