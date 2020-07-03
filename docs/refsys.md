OS/Z Függvényreferenciák
========================

Prototípusok
------------

### Megszakítás

[void clock_ack();](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/rtc.S#L79)
  falióra megszakítás visszaigazolása

[void clock_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/clock.c#L46)
  falióra inicializálása

[void clock_intr(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/core/clock.c#L60)
  falióra megszakítás kezelője

[void intr_disable(uint16_t irq);](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S#L78)
  egy adott IRQ vonal letiltása

[void intr_enable(uint16_t irq);](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S#L58)
  egy adott IRQ vonal engedélyezése

[uint intr_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/intr.c#L43)
  megszakításvezérlő inicializálása

[void intr_nextsched(uint64_t usec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/intr.c#L86)
  generál egy megszakítást usec mikroszekundum múlva (PIT one-shot)

[bool_t intr_nextschedremainder()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/intr.c#L70)
  segédfüggvény, ha a számláló nem képes egy megszakítással usec-et várni

[void pic_init();](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S#L41)
  csip inicializálás

[void rtc_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/rtc.S#L41)
  valós idejű óra inicializálása

### Memória

[void *kalloc(size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L189)
  fizikai memória lefoglalása és leképezése a kernel memóriába

[void *kalloc_gap(size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L194)
  üres "lyuk" lefoglalása a kernel memóriába

[void kentropy()](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L149)
  Véletlenszám generátor bitjeinek összekeverése entrópianövelés céljából

[void kfree(void *p)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L204)
  kernel memória felszabadítása

[void *krealloc(void *p, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L199)
  kernel memória átméretezése

[void *ksalloc(void *p, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L209)
  fizikai memória lefoglalása és leképezése a megosztott kernel memóriába

[void ksfree(void *p)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L219)
  megosztott kernel memória felszabadítása

[void *ksrealloc(void *p, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L214)
  megosztott kernel memória átméretezése

[void lang_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/lang.c#L44)
  nyelvi fájl betöltése, értelmezése és a szótár feltöltése

[size_t mbstrlen(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L100)
  utf-8 karakterek száma egy sztringben

[int memcmp(const void *s1, const void *s2, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L84)
  memória összehasonlítása, általános implementáció

[void *memcpy(void *dst, const void *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L46)
  memória másolása, általános implementáció

[void *memset(void *dst, uint8_t c, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L63)
  memória feltötlése adott karakterrel, általános implementáció

[void* pmm_alloc(tcb_t *tcb, uint64_t pages)](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L174)
  folyamatos fizikai memória lefoglalása. Ficikai címmel tér vissza

[void* pmm_allocslot(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L239)
  egy egybefüggő, címigazított blokk (2M) lefoglalása. Fizikai címmel tér vissza

[void pmm_free(tcb_t *tcb, phy_t base, size_t pages)](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L310)
  Fizikai memória felszabadítása és a szabad listához adása

[void pmm_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L54)
  Fizikai Memória Kezelő inicializálása

[void pmm_vmem()](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L157)
  inicializálás befejezése, miután lett virtuális memóriánk

[char *sprintf(char *dst,char* fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L266)
  sztring összerakása formázás és paraméterek alapján

[int strcmp(const char *s1, const char *s2)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L128)
  két sztring összehasonlítása

[size_t strlen(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L119)
  bájtok száma egy sztringben

[char *strncpy(char *dst, const char *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L138)
  legfeljebb N karakter hosszú sztring másolása

[void vmm_del(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L372)
  virtuális cimtér megszüntetése, és a hozzá tartozó fizikai RAM felszabadítása

[void vmm_free(tcb_t *tcb, virt_t bss, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L284)
  memória leképezés megszüntetése a virtuális cimtérben, és a hozzá tartozó fizikai RAM felszabadítása

[void vmm_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c#L46)
  Virtuális Memória Kezelő inicializálása

[void *vmm_map(tcb_t *tcb, virt_t bss, phy_t phys, size_t size, uint64_t access)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L92)
  általános memória leképezése a virtuális címtérbe. Ha phys 0, új memóriát foglal. Többek között a bztalloc is hívja

[void *vmm_maptext(tcb_t *tcb, virt_t bss, phy_t *phys, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L196)
  kódszegmmens leképezése a virtuális címtérbe. Ez mindig a taszk kódbufferébe dolgozik, elf_load() hívja

[tcb_t *vmm_new(uint16_t cpu, uint8_t priority)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c#L151)
  létrehoz egy új virtuális címteret

[void vmm_page(virt_t memroot, virt_t virt, phy_t phys, uint64_t access)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L38)
  egy fizikai lap leképezése a virtuális címtérbe, nem igényel tcb-t, optimalizált, csak a core használja

[void vmm_swaptextbuf(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L251)
  aktív kódszegmmens és a taszk kódbufferének cseréje, task_execfini() hívja, ha sikeres volt a betöltés

### Debugger

[void dbg_brk(virt_t o, uint8_t m, uint8_t l)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L131)
  listázza a breakpointokat vagy beállít egy új breakpointot O címen, M módra, L hosszan

[virt_t dbg_disasm(virt_t addr, char *str)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L111)
  dekódol vagy hexában dumpol egy utasítást

[void dbg_dumpccb(ccb_t *ccb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L88)
  CPU Kontroll Blokk architektúrafüggő mezőinek dumpolása

[void dbg_dumpregs()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L79)
  speciális regiszterek dumpolása, fault address cím lekérdezése

[void dbg_pagingflags(uint64_t p)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L71)
  a P lapcímtárbejegyzés attribútumainak dumpolása

[void dbg_paginghelp()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L50)
  kiírja, hogy a lapcímtáblában melyik bit milyen attribútumot takar

[int dbg_parsecmd(char *cmd, uint64_t cmdlast)](https://gitlab.com/bztsrc/osz/blob/master/src/core/dbg.c#L710)
  parancsértelmező

[virt_t dbg_previnst(virt_t addr)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L103)
  visszaadja az előző utasítás címét

[void dbg_singlestep(bool_t enable)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L95)
  ki/bekapcsolja a lépésenkénti utasításvégrehajtást

[void dbg_start(char *header, bool_t ispanic)](https://gitlab.com/bztsrc/osz/blob/master/src/core/dbg.c#L805)
  beépített debugger

### Taszk

[void drivers_add(char *drv, pmm_entry_t *devspec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L233)
  új eszközmeghajtó taszk hozzáadása

[char *drivers_find(char *spec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L128)
  visszaadja az eszközspec-hez tartozó első meghajtó fájlnevét. Tipikus eszközspec "pciVVVV:MMMM" vagy "clsCC:SS"

[void drivers_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L55)
  az idle taszk és az eszközmeghajtók inicializálása

[void drivers_intr(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L213)
  üzenet küldése a megszakításkezelő taszkoknak

[void drivers_ready()](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L409)
  akkor hívódik, amikor az összes eszközmeghajtó taszk incializálódott és blokkolódott teendőre várva

[void drivers_regintr(uint16_t irq, pid_t task)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L146)
  megszakításkezelő taszk hozzáadása a listához

[void drivers_start()](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L387)
  kooperatív ütemezés, eszközmeghajtó és rendszerszolgáltatás taszkok futtatása

[void drivers_unregintr(pid_t task)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L178)
  megszakításkezelő taszk eltávolítása a listából

[bool_t msg_allowed(tcb_t *sender, pid_t dest, evt_t event)](https://gitlab.com/bztsrc/osz/blob/master/src/core/security.c#L55)
  ellenőrzi, hogy az aktuális taszk elküldhet-e egy bizonyos üzenetet

[msgret_t msg_core(uint64_t a, uint64_t b, uint64_t c, uint64_t d,](https://gitlab.com/bztsrc/osz/blob/master/src/core/msgcore.c#L47)
  az SRV_CORE taszknak küldött üzenetek feldolgozása

[void msg_notify(pid_t pid, evt_t event, uint64_t a)](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c#L259)
  taszk értesítése, core nevében küld üzenetet. Ezt csakis a core használhatja

[msg_t *msg_recv(uint64_t serial)](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c#L35)
  üzenetek fogadása

[uint64_t msg_send(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, evt_t event, uint64_t serial)](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c#L118)
  üzenetek küldése

[void sched_add(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L94)
  egy taszk hozzáadása valamelyik aktív prioritási sorhoz

[void sched_awake(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L159)
  taszk mozgatása blokkolt vagy hibernált állapotból aktív sorba
   TCB_STATE_BLOCKED -> TCB_STATE_RUNNING

[void sched_block(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L209)
  taszk átmozgatása aktív sorból blokkolt sorba
   TCB_STATE_RUNNING -> TCB_STATE_BLOCKED

[void sched_delay(tcb_t *tcb, uint64_t usec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L254)
  a megadott mikroszekundumig blokkolja a taszkot

[void sched_hybernate(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L238)
  blokkolt sorból háttértárolóra küld egy taszkot
   TCB_STATE_BLOCKED -> TCB_STATE_HYBERND

[void sched_intr(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L82)
  ütemező megszakítás kezelője

[void sched_migrate(tcb_t *tcb, uint16_t cpuid)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L296)
  taszk átmozgatása egy másik processzor magra

[void sched_pick()](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L334)
  a következő taszk kiválasztása futásra. Ez egy agyonoptimalizált,
  O(1)-es, rekurzív prioritáslistás ütemező

[void sched_remove(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L120)
  taszk kivétele prioritási sorból

[void service_add(int srv, char *cmd)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L309)
  új rendszerszolgáltatás taszk hozzáadása. Kicsit kilóg a sorból, de sok a közös elem az eszközmeghajtókkal, itt a helye

[bool_t task_allowed(tcb_t *tcb, char *grp, uint8_t access)](https://gitlab.com/bztsrc/osz/blob/master/src/core/security.c#L36)
  ellenőrzi, hogy az adott taszk rendelkezik-e a megadott jogosultsággal

[void task_del(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L86)
  megszüntet egy taszkot

[bool_t task_execfini(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L231)
  betöltés befejezése, a kódszegmens lecserélése vagy a kódbuffer eldobása

[bool_t task_execinit(tcb_t *tcb, char *cmd, char **argv, char **envp)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L107)
  egy futtatható paramétereinek betöltése a taszk címterébe. Mivel az exec() hiba esetén vissza
  kell térjen az eredeti kódszegmensre, ezért a felső bufferterületet használjuk ideiglenesen

[tcb_t *task_fork(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L75)
  lemásolja az aktuális taszkot

[tcb_t *task_new(uint8_t priority)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L40)
  létrehoz egy új taszkot

### Futtathatók

[elfcache_t *elf_getfile(char *fn)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L52)
  betölt egy ELF objektumot a háttértárolóról a gyorsítótárba

[bool_t elf_load(char *fn, phy_t extrabuf, size_t extrasiz, uint8_t reent)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L163)
  betölt egy ELF objektumot a gyorsítótárból az aktuális címtérbe, rekurzívan a függvénykönyvtáraival együtt

[virt_t elf_lookupsym(char *sym)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L691)
  cím keresése szimbólumhoz, sztring -> cím

[bool_t elf_rtlink()](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L335)
  aktuális címtérben összelinkeli az ELF-eket (run-time linker)

[virt_t elf_sym(virt_t addr, bool_t onlyfunc)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L609)
  szimbólum keresése címhez, cím -> sztring

[void elf_unload(elfbin_t *elfbin)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L118)
  kiszedi az ELF objektumokat az aktuális címtérből, és ha kell, a gyorsítótárból is

### Platform

[void env_asktime()](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L299)
  Pontos idő bekérése a felhasználótól. Ez nagyon korai, még semmink sincs. Nincsen billentyűzet eszközünk
  se megszakítások, se memóriagazdálkodás például, ezért a korai indító konzolt használjuk pollozva.

[void env_asktime_setdigit(uint8_t c, uint32_t i)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L269)
  segédfunkció az idősztring szerkesztéséhez

[unsigned char *env_bool(unsigned char *s, uint64_t *v, uint64_t flag)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L154)
  logikai hamis (false) értelmezése, egyébként alapból igazat ad vissza

[unsigned char *env_debug(unsigned char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L225)
  debug jelzők értelmezése

[unsigned char *env_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L116)
  decimális érték értelmezése, hexára vált 0x előtag esetén

[unsigned char *env_display(unsigned char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L164)
  megjelenítő értelmezése

[uint64_t env_getts(char *p, int16_t timezone)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L52)
  UTC másodperc időbélyeg kiszámítása lokális BCD vagy bináris időstringből

[unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L98)
  hexa érték értelmezése

[void env_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L402)
  a környezeti változók szöveg értelmezése

[unsigned char *env_lang(unsigned char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L211)
  nyelvi kód értelmezése

[unsigned char *env_tz(unsigned char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L133)
  időzóna értelmezése

[void fault_intr(uint64_t exc, uint64_t errcode, uint64_t dummy)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/fault.c#L32)
  x86-on nincs igazi fault_intr(), mivel az IDT más címre adja eleve a vezérlést, lásd faultidt.S

[void lockacquire(int bit, uint64_t *ptr);](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L35)
  jelző lefoglalása többprocesszoros rendszeren

[void lockrelease(int bit, uint64_t *ptr);](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L49)
  jelző felszabadítása

[int memcmp(const void *s1, const void *s2, size_t len)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L169)
  memória összehasonlítása, architektúra specifikus implementáció

[void *memcpy(void *dst, const void *src, size_t len)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L59)
  memória másolása, architektúra specifikus implementáció

[void *memset(void *dst, uint8_t chr, size_t len)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L143)
  memória feltötlése adott karakterrel, architektúra specifikus implementáció

[void platform_awakecpu(uint16_t cpuid)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L78)
  felébresztés üresjáratból

[void platform_cpu()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L51)
  CPU ellenőrzése

[void platform_dbginit()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L85)
  soros vonali debug konzol inicializálása

[void platform_dbgputc(uint8_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L107)
  egy bájt küldése a debug konzolra

[void platform_env()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L33)
  platform függő környezeti változó alapértékek, env_init() hívja

[void platform_halt()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L71)
  futtatás felfüggesztése

[void platform_idle()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L39)
  CPU üresjárata

[void platform_load()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L222)
  CPU állapotának visszatöltése

[bool_t platform_memfault(void *ptr)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L116)
  eldönti egy címről, hogy elérhető-e, lehet, hogy nincs leképezve

[unsigned char *platform_parse(unsigned char *env)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L42)
  platform függő környezeti értékek értelmezése, env_init() hívja

[void platform_poweroff()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L50)
  számítógép kikapcsolása, kprintf_poweroff() hívja

[void platform_reset()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L64)
  számítógép újraindítása, kprintf_reboot() hívja

[void platform_save()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L188)
  CPU állapotának lementése

[void platform_srand()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L258)
  véletlenszám generátor inicializálása

[void platform_syscall()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L129)
  syscall hívás
  Be:
     funkció: rax
     paraméterek: rdi, rsi, rdx, r10, r8, r9
     visszatérési cím: rcx, flagek: r11
  Ki:
     visszatérési érték: rdx:rax
     hibakód: rdi

[void platform_waitkey()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L297)
  korai konzol implementáció, a kprintf használja

### Fájlrendszer

[bool_t cache_cleardirty(fid_t fd, blkcnt_t lsn)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L243)
  módosított jelző törlése és a kiírt blokk számláló növelése az eszközön

[void cache_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L307)
  blokk gyorsítótár dumpolása, debuggoláshoz

[void cache_flush()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L213)
  blokk gyorsítótár kiírása az eszközökre

[void cache_free()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L277)
  a gyorsítótár legalább felének felszabadítása a legrégebben használt blokkok kiírásával

[blkcnt_t cache_freeblocks(fid_t fd, blkcnt_t needed)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L147)
  egy adott eszköz összes blokkjának felszabadítása

[void* cache_getblock(fid_t fd, blkcnt_t lsn)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L67)
  blokk beolvasása a gyorsítótárból, beállítja az ackdelayed-et ha nincs a blokk a tárban

[void cache_init()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L56)
  blokkgyorsítótár inicializálása

[bool_t cache_setblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L104)
  blokk letárolása a gyorsítótárba, eszközmeghajtók üzenetére

[char *canonize(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L96)
  hasonló a realpath()-hoz, de csak memóriából dolgozik, és nem oldja fel a szimbólikus hivatkozásokat és a fel könyvtárat

[uint32_t devfs_add(char *name, pid_t drivertask, dev_t device, mode_t mode, blksize_t blksize, blkcnt_t blkcnt)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L54)
  eszköz hozzáadása

[void devfs_del(uint32_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L88)
  eszköz eltávolítása

[void devfs_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L220)
  eszközlista dumpolása, debuggoláshoz

[size_t devfs_getdirent(fpos_t offs)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L164)
  dirent_t visszaadása egy devfs bejegyzéshez

[void devfs_init()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L188)
  devfs inicializálása

[uint32_t devfs_lastused(bool_t all)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L148)
  legrégebben használt eszközt adja vissza

[void devfs_used(uint32_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L129)
  eszköz használtnak jelölése

[bool_t dofsck(fid_t fd, bool_t fix)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L658)
  fájlrendszer ellenőrzése egy eszközön

[fid_t fcb_add(char *abspath, uint8_t type)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L68)
  elérési út keresése az fcb listában, és hozzáadás, ha nem találta

[void fcb_del(fid_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L96)
  fcb bejegyzés eltávolítása

[void fcb_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L452)
  Fájl Kontrol Blokkok listájának dumpolása, debuggoláshoz

[bool_t fcb_flush(fid_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L351)
  fcb írási buffer kiírása az eszközre (blokk gyorsítótárba)

[void fcb_free()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L132)
  összes nemhasznált bejegyzés törlése az fcb listából

[fid_t fcb_get(char *abspath)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L42)
  elérési út keresése az fcb listában

[char *fcb_readlink(fid_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L165)
  unió listájának vagy szimbolikus hivatkozás céljának visszaadása

[size_t fcb_unionlist_add(fid_t **fl, fid_t f, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L149)
  fid_t hozzáadása az unió fl listához, ha még nincs benne

[fid_t fcb_unionlist_build(fid_t idx, void *buf, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L184)
  unió fid listájának összeállítása

[bool_t fcb_write(fid_t idx, off_t offs, void *buf, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L264)
  adatok írása egy fcb-be, a módosítás bufferbe kerül

[void *fs_locate(char *fn)](https://gitlab.com/bztsrc/osz/blob/master/src/core/fs.c#L38)
  visszaadja az initrd-n lévő fájl tartalmának kezdőcímét és fs_size változóban a méretét

[int16_t fsdrv_detect(fid_t dev)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c#L75)
  fájlrendszer detektálása eszközön vagy fájlban

[void fsdrv_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c#L99)
  fájlrendszer meghajtók dumpolása debuggoláshoz

[int16_t fsdrv_get(char *name)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c#L63)
  fájlrendszer értelmező kikeresése név alapján

[uint16_t fsdrv_reg(const fsdrv_t *fs)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c#L42)
  fájlrendszer értelmező regisztrálása

[fpos_t getoffs(char *abspath)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L211)
  offszet lekérése elérési útból

[uint8_t getver(char *abspath)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L200)
  verzió lekérése elérési útból

[fid_t lookup(char *path, bool_t creat)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L425)
  fcb index visszaadása abszolút elérési úthoz. Errno-t EAGAIN-re állíthatja gyorsítótárhiány esetén

[uint16_t mtab_add(char *dev, char *file, char *opts)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L48)
  új felcsatolási pont hozzáadása

[bool_t mtab_del(char *dev, char *file)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L118)
  felcsatolási pont megszüntetése

[void mtab_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L202)
  felcsatolási pontok dumpolása debuggoláshoz

[void mtab_fstab(char *ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L147)
  az /etc/fstab értemezése és fájlrendszerek felcsatolása

[void mtab_init()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L41)
  felcsatolási pontok inicializálása

[char *pathcat(char *path, char *filename)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L77)
  fájlnév hozzáfűzése az elérési úthoz. A path buffernek elég nagynak kell lennie

[pathstack_t *pathpop()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L68)
  legutóbbi inode kivétele az elérési út veremből

[void pathpush(ino_t lsn, char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L51)
  inode hivatkozás berakása az elérési út verembe. forgatjuk, ha túl nagyra nőne

[uint64_t pipe_add(fid_t fid, pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L42)
  új csővezeték hozzáadása

[void pipe_del(uint64_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L68)
  csővezeték eltávolítása

[void pipe_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L237)
  csővezetékek dumpolása debuggoláshoz

[size_t pipe_read(uint64_t idx, virt_t ptr, size_t s, pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L156)
  olvasás a csővezetékből

[stat_t *pipe_stat(uint64_t idx, mode_t mode)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L215)
  stat_t visszaadása csővezetékhez

[bool_t pipe_write(uint64_t idx, virt_t ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L94)
  adatok írása csővezetékbe

[void *readblock(fid_t fd, blkcnt_t lsn)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L222)
  blokk beolvasása tárolóból (zárolt eszköz esetén is működnie kell)

[stat_t *statfs(fid_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L612)
  stat_t struktúra lekérése fcb indexhez

[bool_t taskctx_close(taskctx_t *tc, uint64_t idx, bool_t dontfree)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L215)
  eltávolítás a megnyitott fájlok listájából

[void taskctx_del(pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L70)
  taszk kontextus eltávolítása

[void taskctx_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L596)
  taszk kontextusok dumpolása debuggoláshoz

[taskctx_t *taskctx_get(pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L43)
  taszk kontextus visszaadása (ha nincs, akkor létrehoz egy újat)

[uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs, uint64_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L124)
  megnyitott fájlok listájához hozzáadás

[size_t taskctx_read(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L406)
  olvasás fájlból, ptr vagy egy virtuális cím ctx->pid címterében (nem írható direktben) vagy megosztott memória (írható)

[dirent_t *taskctx_readdir(taskctx_t *tc, fid_t idx, void *ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L317)
  a soronkövetkező könyvtárbejegyzés (dirent) beolvasása

[void taskctx_rootdir(taskctx_t *tc, fid_t fid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L102)
  gyökérkönyvtár beállítása

[bool_t taskctx_seek(taskctx_t *tc, uint64_t idx, off_t offs, uint8_t whence)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L266)
  pozíció beállítása megnyitott fájl leírón

[bool_t taskctx_validfid(taskctx_t *tc, uint64_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L305)
  annak ellenőrzése, hogy a fájl leíró érvényes-e az adott kontextusban

[void taskctx_workdir(taskctx_t *tc, fid_t fid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L113)
  munkakönyvtár beállítása

[size_t taskctx_write(taskctx_t *tc, fid_t idx, void *ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L538)
  írás fájlba, ptr akár virtuális cím, akár megosztott memóriacím, mindenképp olvasható

[bool_t writeblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L355)
  blokk kiírása tárolóra megfelelő prioritással (zárolt eszköz esetén is működnie kell)

### Konzol

[void kpanic(char *reason, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L231)
  pánik képernyő, ha van, akkor meghívja a debuggert

[void kprintf(char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L914)
  segédfüggvény a változó paraméterszámú változathoz

[void kprintf_bg(uint32_t color)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L91)
  háttér színének beállítása

[void kprintf_center(int w, int h)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L101)
  kurzor beállítása egy középre igazított ablak bal felső sarkába

[void kprintf_clearline()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L742)
  aktuális sor törlése

[void kprintf_clock()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L479)
  kirakja az időt a jobb felső sarokban

[static __inline__ void kprintf_dumpascii(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L400)
  hasonló, de a vezérlőkaraktereket kihagyja, %A

[static __inline__ void kprintf_dumpmem(uint8_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L490)
  memória dump, %D

[static __inline__ void kprintf_dumppagetable(volatile uint64_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L564)
  címfordító tábla dump, %P

[static __inline__ void kprintf_dumppagewalk(uint64_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L589)
  címfordítás bejárás dump, %p

[static __inline__ void kprintf_dumptcb(uint8_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L527)
  Taszk Kontroll Blokk dumpolás, %T

[void kprintf_fade()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L160)
  képernyő elhomályosítása

[void kprintf_fg(uint32_t color)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L81)
  előtér színének beállítása

[void kprintf_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L109)
  korai konzol inicializálása

[void kprintf_poweroff()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L206)
  viszlát képernyő és a számítógép kikapcsolása

[static __inline__ void kprintf_putascii(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L391)
  ascii érték megjelenítése, %a

[static __inline__ void kprintf_putbin(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L443)
  bináris formázás, %b

[void kprintf_putchar(int c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L318)
  egy unicode karakter megjelenítése

[static __inline__ void kprintf_putdec(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L418)
  decimálisra formázott szám, %d

[static void kprintf_puthex(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L370)
  hexadecimális formázás, %x

[void kprintf_reboot()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L182)
  viszlát képernyő és a számítógép újraindítása

[void kprintf_reset()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L70)
  konzol alapállapot. Fehér szín feketén, kurzor a bal felső sarokban

[void kprintf_scrolloff()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L728)
  szkrollozás kikapcsolása

[void kprintf_scrollscr()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L645)
  képernyő görgetése, és várakozás, ha srcy be van állítva

[void kprintf_unicodetable()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L631)
  tesztelésre, unicode kódtábla megjelenítése, 0-2047

[static __inline__ void kprintf_uuid(uuid_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L459)
  uuid egyedi azonosító formázás, %U

[void syslog(int pri, char* fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c#L37)
  korai RFC5424 kompatíbilis naplózó

[void vkprintf(char *fmt, va_list args, bool_t asciiz)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L764)
  szöveg megjelenítése a korai konzolon

Fájlok
------

| Fájl | Alrendszer | Leírás |
| ---- | ---------- | ------ |
| [core/clock.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/clock.c) | megszakítás | falióra funkciók. Nanoszekundumban számol, de csak 1/clock_freq másodpercenként frissül |
| [core/x86_64/ibmpc/intr.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/intr.c) | megszakítás | Megszakításvezérlő és Intel 8253 Programmable Interval Timer eszközmeghajtó |
| [core/x86_64/ibmpc/pic.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S) | megszakítás | i8259A Programmable Interrupt Controller megszakításvezérlő eszközmeghajtó |
| [core/x86_64/ibmpc/rtc.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/rtc.S) | megszakítás | Motorola MC146818 Real Time Clock eszközmeghajtó, periodikus falióra |
| [core/core.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h) | memória | Core funkciók (ring 0 / EL1) |
| [core/lang.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/lang.c) | memória | Többnyelvű fordítások támogatása |
| [core/lang.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/lang.h) | memória | Nyelvi fordítások szótára |
| [core/libc.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c) | memória | Alacsony szintű függvénykönyvtár a core-hoz |
| [core/pmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c) | memória | Fizikai Memória Kezelő |
| [core/pmm.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.h) | memória | Fizikai Memória Kezelő |
| [core/vmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c) | memória | Virtuális Memória Kezelő, architektúra független rész |
| [core/vmm.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.h) | memória | Virtuális Memória Kezelő |
| [core/x86_64/idt.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/idt.h) | memória | Megszakítás Leíró Tábla (IDT) definíciói |
| [core/x86_64/vmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c) | memória | Virtuális Memória Kezelő, architektúra függő rész |
| [core/x86_64/vmm.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.h) | memória | Virtuális Memória Kezelő, architektúra függő rész |
| [core/dbg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/dbg.c) | debugger | Beépített Debugger |
| [core/x86_64/dbg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c) | debugger | Beépített Debugger, architektúra függő eljárások |
| [core/x86_64/disasm.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/disasm.h) | debugger | Disassembler |
| [core/drivers.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c) | taszk | eszközmeghajtó taszk funkciók |
| [core/msg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c) | taszk | üzenetküldés, a core-nak küldött üzenetek feldolgozása az msgcore.c-ben van |
| [core/msg.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.h) | taszk | üzenet sor fejléce |
| [core/msgcore.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/msgcore.c) | taszk | a core-nak küldött üzenetek feldolgozása, az általános üzenetküldés az msg.c-ben van |
| [core/sched.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c) | taszk | Taszk ütemező |
| [core/security.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/security.c) | taszk | üzenetküldés biztonsági házirend |
| [core/task.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c) | taszk | taszk funkciók |
| [core/task.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.h) | taszk | taszk funkciók |
| [core/x86_64/ccb.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ccb.h) | taszk | CPU Kontrol Blokk. architektúra függő struktúra |
| [core/x86_64/task.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/task.h) | taszk | Taszk Kontrol Blokk. Architectúra függő struktúra |
| [core/elf.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c) | futtathatók | ELF betöltő és értelmező |
| [core/elf.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.h) | futtathatók | ELF betöltő és értelmező |
| [core/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c) | platform | Környezeti változók értelmezése (lásd FS0:\BOOTBOOT\CONFIG vagy /sys/config) |
| [core/env.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.h) | platform | Környezeti változók |
| [core/fault.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/fault.c) | platform | platform független kivétel kezelők |
| [core/fault.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/fault.h) | platform | platform független kivétel kezelők |
| [core/main.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/main.c) | platform | Core, fő ciklus |
| [core/x86_64/arch.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/arch.h) | platform | Architektúra függő headerök |
| [core/x86_64/fault.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/fault.c) | platform | kivétel kezelő |
| [core/x86_64/faultidt.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/faultidt.S) | platform | alacsony szintű kivétel kezelők |
| [core/x86_64/ibmpc/platform.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c) | platform | Platform specifikus funkciók |
| [core/x86_64/ibmpc/platform.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.h) | platform | Platform specifikus definíciók |
| [core/x86_64/libc.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S) | platform | Alacsony szintű függvénykönyvtár |
| [core/x86_64/platform.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S) | platform | Architektúra függő, platform specifikus függvények |
| [core/x86_64/start.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/start.S) | platform | Belépési pont |
| [core/fs.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/fs.c) | fájlrendszer | FS taszk előtti, FS/Z initrd-t kezelő funkciók (csak olvasás) |
| [fs/cache.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c) | fájlrendszer | blokk gyorsítótár |
| [fs/devfs.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c) | fájlrendszer | beépített eszköz fájlrendszer (devfs) |
| [fs/fcb.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c) | fájlrendszer | Fájl Kontrol Blokkok kezelése |
| [fs/fsdrv.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c) | fájlrendszer | Fájl Rendszer Meghajtók kezelése |
| [fs/main.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/main.c) | fájlrendszer | FS taszk |
| [fs/mtab.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c) | fájlrendszer | felcsatolási pontok kezelése |
| [fs/pipe.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c) | fájlrendszer | csővezetékek kezelése |
| [fs/taskctx.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c) | fájlrendszer | taszk kontextus fájlrendszer szolgáltatásokhoz |
| [fs/vfs.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c) | fájlrendszer | VFS illesztési réteg |
| [core/kprintf.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c) | konzol | Felügyeleti szinten futó printf implementáció, korai konzol |
| [core/syslog.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c) | konzol | Korai rendszernaplózó. Megosztott területet használ a "syslog" taszkkal |

