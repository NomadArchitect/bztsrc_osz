/*
 * core/x86_64/arch.h
 *
 * Copyright (c) 2017 bzt (bztsrc@gitlab)
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
 * @brief Architektúra függő headerök
 */

#include "../core.h"
#include "vmm.h"
#include "ccb.h"
#include "task.h"
#include "idt.h"

#define ARCH_ELFEM          EM_X86_64   /* ELF architektúra kód */

/* Rendszerleíró táblák */
#define systable_dma_idx    0
#define systable_acpi_idx   1
#define systable_smbi_idx   2
#define systable_efi_idx    3
#define systable_pcie_idx   4           /* mp táblát nem használjuk */
#define systable_dsdt_idx   5
#define systable_apic_idx   6
#define systable_ioapic_idx 7
#define systable_hpet_idx   8

#define cpu_relax __asm__ __volatile__("pause":::"memory");
#define breakpoint __asm__ __volatile__("xchg %bx, %bx")
