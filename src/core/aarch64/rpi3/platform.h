/*
 * core/aarch64/rpi3/platform.h
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
 * @brief Platform specifikus definíciók
 */

#include <arch.h>

#define ARMTMR_FREQ 38400000
#define ARMTMR_STEP 1000
#define SCHED_IRQ   64
#define CLOCK_IRQ   65

#define MMIO_BASE       0xfffffffff8000000

/* megszakításvezérlő kontroller */
#define IRQ_BASIC_PEND  ((volatile uint32_t*)(MMIO_BASE+0x0000B200))
#define IRQ_PENDING_1   ((volatile uint32_t*)(MMIO_BASE+0x0000B204))
#define IRQ_PENDING_2   ((volatile uint32_t*)(MMIO_BASE+0x0000B208))
#define IRQ_FIQ_CTRL    ((volatile uint32_t*)(MMIO_BASE+0x0000B20C))
#define IRQ_ENABLE_1    ((volatile uint32_t*)(MMIO_BASE+0x0000B210))
#define IRQ_ENABLE_2    ((volatile uint32_t*)(MMIO_BASE+0x0000B214))
#define IRQ_BASIC_ENABL ((volatile uint32_t*)(MMIO_BASE+0x0000B218))
#define IRQ_DISABLE_1   ((volatile uint32_t*)(MMIO_BASE+0x0000B21C))
#define IRQ_DISABLE_2   ((volatile uint32_t*)(MMIO_BASE+0x0000B220))
#define IRQ_BASIC_DISAB ((volatile uint32_t*)(MMIO_BASE+0x0000B224))

/* ARM QA7 control kontroller */
/* CPU magonkénti időzítő */
#define ARMTMR_CS       ((volatile uint32_t*)(MMIO_BASE+0x01000000))
#define ARMTMR_PRESCALE ((volatile uint32_t*)(MMIO_BASE+0x01000000))
#define ARMTMR_C0_ENA   ((volatile uint32_t*)(MMIO_BASE+0x01000040))
#define ARMTMR_C1_ENA   ((volatile uint32_t*)(MMIO_BASE+0x01000044))
#define ARMTMR_C2_ENA   ((volatile uint32_t*)(MMIO_BASE+0x01000048))
#define ARMTMR_C3_ENA   ((volatile uint32_t*)(MMIO_BASE+0x0100004C))
#define ARMTMR_C0_PEND  ((volatile uint32_t*)(MMIO_BASE+0x01000060))
#define ARMTMR_C1_PEND  ((volatile uint32_t*)(MMIO_BASE+0x01000064))
#define ARMTMR_C2_PEND  ((volatile uint32_t*)(MMIO_BASE+0x01000068))
#define ARMTMR_C3_PEND  ((volatile uint32_t*)(MMIO_BASE+0x0100006C))
#define ARMTMR_PEND(x)  ((volatile uint32_t*)(MMIO_BASE+0x01000060+(x*4)))
/* globális időzítő */
#define ARMTMR_LOCALIRQ ((volatile uint32_t*)(MMIO_BASE+0x01000024))
#define ARMTMR_LOCALTMR ((volatile uint32_t*)(MMIO_BASE+0x01000034))
#define ARMTMR_LOCALCLR ((volatile uint32_t*)(MMIO_BASE+0x01000038))

/* Power Management, energiagazdálkodás, kikapcsolás, újraindítás */
#define PM_RSTC         ((volatile uint32_t*)(MMIO_BASE+0x0010001c))
#define PM_RSTS         ((volatile uint32_t*)(MMIO_BASE+0x00100020))
#define PM_WDOG         ((volatile uint32_t*)(MMIO_BASE+0x00100024))
#define PM_WDOG_MAGIC   0x5a000000
#define PM_RSTC_FULLRST 0x00000020

/* Random Number Generator, véletlenszám generátor */
#define RNG_CTRL        ((volatile uint32_t*)(MMIO_BASE+0x00104000))
#define RNG_STATUS      ((volatile uint32_t*)(MMIO_BASE+0x00104004))
#define RNG_DATA        ((volatile uint32_t*)(MMIO_BASE+0x00104008))
#define RNG_INT_MASK    ((volatile uint32_t*)(MMIO_BASE+0x00104010))

/* UART0, egy PL011 csip */
#define UART0_DR        ((volatile uint32_t*)(MMIO_BASE+0x00201000))
#define UART0_FR        ((volatile uint32_t*)(MMIO_BASE+0x00201018))
#define UART0_IBRD      ((volatile uint32_t*)(MMIO_BASE+0x00201024))
#define UART0_FBRD      ((volatile uint32_t*)(MMIO_BASE+0x00201028))
#define UART0_LCRH      ((volatile uint32_t*)(MMIO_BASE+0x0020102C))
#define UART0_CR        ((volatile uint32_t*)(MMIO_BASE+0x00201030))
#define UART0_IMSC      ((volatile uint32_t*)(MMIO_BASE+0x00201038))
#define UART0_ICR       ((volatile uint32_t*)(MMIO_BASE+0x00201044))

/* UART1, mini-AUX */
#define AUX_ENABLE      ((volatile uint32_t*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile uint32_t*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile uint32_t*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile uint32_t*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile uint32_t*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile uint32_t*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile uint32_t*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile uint32_t*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile uint32_t*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile uint32_t*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile uint32_t*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile uint32_t*)(MMIO_BASE+0x00215068))

/* VideoCore levelesláda */
#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile uint32_t*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile uint32_t*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile uint32_t*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile uint32_t*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile uint32_t*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile uint32_t*)(VIDEOCORE_MBOX+0x20))
#define MBOX_REQUEST    0
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* GPIO kontroller */
#define GPFSEL0         ((volatile uint32_t*)(MMIO_BASE+0x00200000))
#define GPFSEL1         ((volatile uint32_t*)(MMIO_BASE+0x00200004))
#define GPFSEL2         ((volatile uint32_t*)(MMIO_BASE+0x00200008))
#define GPFSEL3         ((volatile uint32_t*)(MMIO_BASE+0x0020000C))
#define GPFSEL4         ((volatile uint32_t*)(MMIO_BASE+0x00200010))
#define GPFSEL5         ((volatile uint32_t*)(MMIO_BASE+0x00200014))
#define GPSET0          ((volatile uint32_t*)(MMIO_BASE+0x0020001C))
#define GPSET1          ((volatile uint32_t*)(MMIO_BASE+0x00200020))
#define GPCLR0          ((volatile uint32_t*)(MMIO_BASE+0x00200028))
#define GPLEV0          ((volatile uint32_t*)(MMIO_BASE+0x00200034))
#define GPLEV1          ((volatile uint32_t*)(MMIO_BASE+0x00200038))
#define GPEDS0          ((volatile uint32_t*)(MMIO_BASE+0x00200040))
#define GPEDS1          ((volatile uint32_t*)(MMIO_BASE+0x00200044))
#define GPHEN0          ((volatile uint32_t*)(MMIO_BASE+0x00200064))
#define GPHEN1          ((volatile uint32_t*)(MMIO_BASE+0x00200068))
#define GPPUD           ((volatile uint32_t*)(MMIO_BASE+0x00200094))
#define GPPUDCLK0       ((volatile uint32_t*)(MMIO_BASE+0x00200098))
#define GPPUDCLK1       ((volatile uint32_t*)(MMIO_BASE+0x0020009C))
