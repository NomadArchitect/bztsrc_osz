OS/Z Függvényreferenciák
========================

Prototípusok
------------
Összesen 211 függvény van definiálva.

### Konzol
[void kpanic(char *reason, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L239)
  pánik képernyő, ha van, akkor meghívja a debuggert
 
[void kprintf(char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L939)
  segédfüggvény a változó paraméterszámú változathoz
 
[void kprintf_bg(uint32_t color)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L93)
  háttér színének beállítása
 
[void kprintf_center(int w, int h)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L104)
  kurzor beállítása egy középre igazított ablak bal felső sarkába
 
[void kprintf_clearline()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L766)
  aktuális sor törlése
 
[void kprintf_clock()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L495)
  kirakja az időt a jobb felső sarokban
 
[static inline void kprintf_dumpascii(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L412)
  hasonló, de a vezérlőkaraktereket kihagyja, %A
 
[static inline void kprintf_dumpmem(uint8_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L507)
  memória dump, %D
 
[static inline void kprintf_dumppagetable(volatile uint64_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L583)
  címfordító tábla dump, %P
 
[static inline void kprintf_dumppagewalk(uint64_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L609)
  címfordítás bejárás dump, %p
 
[static inline void kprintf_dumptcb(uint8_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L545)
  Taszk Kontroll Blokk dumpolás, %T
 
[void kprintf_fade()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L165)
  képernyő elhomályosítása
 
[void kprintf_fg(uint32_t color)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L82)
  előtér színének beállítása
 
[void kprintf_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L113)
  korai konzol inicializálása
 
[void kprintf_poweroff()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L213)
  viszlát képernyő és a számítógép kikapcsolása
 
[static inline void kprintf_putascii(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L402)
  ascii érték megjelenítése, %a
 
[static inline void kprintf_putbin(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L457)
  bináris formázás, %b
 
[void kprintf_putchar(int c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L327)
  egy unicode karakter megjelenítése
 
[static inline void kprintf_putdec(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L431)
  decimálisra formázott szám, %d
 
[static void kprintf_puthex(int64_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L380)
  hexadecimális formázás, %x
 
[void kprintf_reboot()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L188)
  viszlát képernyő és a számítógép újraindítása
 
[void kprintf_reset()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L70)
  konzol alapállapot. Fehér szín feketén, kurzor a bal felső sarokban
 
[void kprintf_scrolloff()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L751)
  szkrollozás kikapcsolása
 
[void kprintf_scrollscr()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L667)
  képernyő görgetése, és várakozás, ha srcy be van állítva
 
[void kprintf_unicodetable()](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L652)
  tesztelésre, unicode kódtábla megjelenítése, 0-2047
 
[static inline void kprintf_uuid(uuid_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L474)
  uuid egyedi azonosító formázás, %U
 
[void syslog(int pri, char* fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c#L37)
  korai RFC5424 kompatíbilis naplózó
 
[void vkprintf(char *fmt, va_list args, bool_t asciiz)](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c#L788)
  szöveg megjelenítése a korai konzolon
 
### Fájlrendszer
[bool_t cache_cleardirty(fid_t fd, blkcnt_t lsn)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L250)
  módosított jelző törlése és a kiírt blokk számláló növelése az eszközön
 
[void cache_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L316)
  blokk gyorsítótár dumpolása, debuggoláshoz
 
[void cache_flush()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L219)
  blokk gyorsítótár kiírása az eszközökre
 
[void cache_free()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L285)
  a gyorsítótár legalább felének felszabadítása a legrégebben használt blokkok kiírásával
 
[blkcnt_t cache_freeblocks(fid_t fd, blkcnt_t needed)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L152)
  egy adott eszköz összes blokkjának felszabadítása
 
[void* cache_getblock(fid_t fd, blkcnt_t lsn)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L70)
  blokk beolvasása a gyorsítótárból, beállítja az ackdelayed-et ha nincs a blokk a tárban
 
[void cache_init()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L57)
  blokkgyorsítótár inicializálása
 
[bool_t cache_setblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c#L108)
  blokk letárolása a gyorsítótárba, eszközmeghajtók üzenetére
 
[char *canonize(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L99)
  hasonló a realpath()-hoz, de csak memóriából dolgozik, és nem oldja fel a szimbólikus hivatkozásokat és a fel könyvtárat
 
[uint32_t devfs_add(char *name, pid_t drivertask, dev_t device, mode_t mode, blksize_t blksize, blkcnt_t blkcnt)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L54)
  eszköz hozzáadása
 
[void devfs_del(uint32_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L89)
  eszköz eltávolítása
 
[void devfs_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L226)
  eszközlista dumpolása, debuggoláshoz
 
[size_t devfs_getdirent(fpos_t offs)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L168)
  dirent_t visszaadása egy devfs bejegyzéshez
 
[void devfs_init()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L207)
  devfs inicializálása
 
[uint32_t devfs_lastused(bool_t all)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L151)
  legrégebben használt eszközt adja vissza
 
[void devfs_used(uint32_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c#L131)
  eszköz használtnak jelölése
 
[bool_t dofsck(fid_t fd, bool_t fix)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L668)
  fájlrendszer ellenőrzése egy eszközön
 
[fid_t fcb_add(char *abspath, uint8_t type)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L67)
  elérési út keresése az fcb listában, és hozzáadás, ha nem találta
 
[void fcb_del(fid_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L95)
  fcb bejegyzés eltávolítása
 
[void fcb_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L457)
  Fájl Kontrol Blokkok listájának dumpolása, debuggoláshoz
 
[bool_t fcb_flush(fid_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L355)
  fcb írási buffer kiírása az eszközre (blokk gyorsítótárba)
 
[void fcb_free()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L131)
  összes nemhasznált bejegyzés törlése az fcb listából
 
[fid_t fcb_get(char *abspath)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L40)
  elérési út keresése az fcb listában
 
[char *fcb_readlink(fid_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L166)
  unió listájának vagy szimbolikus hivatkozás céljának visszaadása
 
[size_t fcb_unionlist_add(fid_t **fl, fid_t f, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L149)
  fid_t hozzáadása az unió fl listához, ha még nincs benne
 
[fid_t fcb_unionlist_build(fid_t idx, void *buf, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L186)
  unió fid listájának összeállítása
 
[bool_t fcb_write(fid_t idx, off_t offs, void *buf, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c#L267)
  adatok írása egy fcb-be, a módosítás bufferbe kerül
 
[void *fs_locate(char *fn)](https://gitlab.com/bztsrc/osz/blob/master/src/core/fs.c#L38)
  visszaadja az initrd-n lévő fájl tartalmának kezdőcímét és fs_size változóban a méretét
 
[int16_t fsdrv_detect(fid_t dev)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c#L77)
  fájlrendszer detektálása eszközön vagy fájlban
 
[void fsdrv_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c#L102)
  fájlrendszer meghajtók dumpolása debuggoláshoz
 
[int16_t fsdrv_get(char *name)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c#L64)
  fájlrendszer értelmező kikeresése név alapján
 
[uint16_t fsdrv_reg(const fsdrv_t *fs)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c#L42)
  fájlrendszer értelmező regisztrálása
 
[fpos_t getoffs(char *abspath)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L216)
  offszet lekérése elérési útból
 
[uint8_t getver(char *abspath)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L204)
  verzió lekérése elérési útból
 
[fid_t lookup(char *path, bool_t creat)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L433)
  fcb index visszaadása abszolút elérési úthoz. Errno-t EAGAIN-re állíthatja gyorsítótárhiány esetén
 
[uint16_t mtab_add(char *dev, char *file, char *opts)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L49)
  új felcsatolási pont hozzáadása
 
[bool_t mtab_del(char *dev, char *file)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L120)
  felcsatolási pont megszüntetése
 
[void mtab_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L212)
  felcsatolási pontok dumpolása debuggoláshoz
 
[void mtab_fstab(char *ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L150)
  az /etc/fstab értemezése és fájlrendszerek felcsatolása
 
[void mtab_init()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c#L41)
  felcsatolási pontok inicializálása
 
[char *pathcat(char *path, char *filename)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L79)
  fájlnév hozzáfűzése az elérési úthoz. A path buffernek elég nagynak kell lennie
 
[pathstack_t *pathpop()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L69)
  legutóbbi inode kivétele az elérési út veremből
 
[void pathpush(ino_t lsn, char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L51)
  inode hivatkozás berakása az elérési út verembe. forgatjuk, ha túl nagyra nőne
 
[uint64_t pipe_add(fid_t fid, pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L42)
  új csővezeték hozzáadása
 
[void pipe_del(uint64_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L69)
  csővezeték eltávolítása
 
[void pipe_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L242)
  csővezetékek dumpolása debuggoláshoz
 
[size_t pipe_read(uint64_t idx, virt_t ptr, size_t s, pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L159)
  olvasás a csővezetékből
 
[stat_t *pipe_stat(uint64_t idx, mode_t mode)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L219)
  stat_t visszaadása csővezetékhez
 
[bool_t pipe_write(uint64_t idx, virt_t ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c#L96)
  adatok írása csővezetékbe
 
[void *readblock(fid_t fd, blkcnt_t lsn)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L228)
  blokk beolvasása tárolóból (zárolt eszköz esetén is működnie kell)
 
[stat_t *statfs(fid_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L621)
  stat_t struktúra lekérése fcb indexhez
 
[bool_t taskctx_close(taskctx_t *tc, uint64_t idx, bool_t dontfree)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L220)
  eltávolítás a megnyitott fájlok listájából
 
[void taskctx_del(pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L71)
  taszk kontextus eltávolítása
 
[void taskctx_dump()](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L607)
  taszk kontextusok dumpolása debuggoláshoz
 
[taskctx_t *taskctx_get(pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L43)
  taszk kontextus visszaadása (ha nincs, akkor létrehoz egy újat)
 
[uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs, uint64_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L128)
  megnyitott fájlok listájához hozzáadás
 
[size_t taskctx_read(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L415)
  olvasás fájlból, ptr vagy egy virtuális cím ctx->pid címterében (nem írható direktben) vagy megosztott memória (írható)
 
[dirent_t *taskctx_readdir(taskctx_t *tc, fid_t idx, void *ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L325)
  a soronkövetkező könyvtárbejegyzés (dirent) beolvasása
 
[void taskctx_rootdir(taskctx_t *tc, fid_t fid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L104)
  gyökérkönyvtár beállítása
 
[bool_t taskctx_seek(taskctx_t *tc, uint64_t idx, off_t offs, uint8_t whence)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L272)
  pozíció beállítása megnyitott fájl leírón
 
[bool_t taskctx_validfid(taskctx_t *tc, uint64_t idx)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L312)
  annak ellenőrzése, hogy a fájl leíró érvényes-e az adott kontextusban
 
[void taskctx_workdir(taskctx_t *tc, fid_t fid)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L116)
  munkakönyvtár beállítása
 
[size_t taskctx_write(taskctx_t *tc, fid_t idx, void *ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c#L548)
  írás fájlba, ptr akár virtuális cím, akár megosztott memóriacím, mindenképp olvasható
 
[bool_t writeblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c#L362)
  blokk kiírása tárolóra megfelelő prioritással (zárolt eszköz esetén is működnie kell)
 
### Platform
[void env_asktime()](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L308)
  Pontos idő bekérése a felhasználótól. Ez nagyon korai, még semmink sincs. Nincsen billentyűzet eszközünk
  se megszakítások, se memóriagazdálkodás például, ezért a korai indító konzolt használjuk pollozva.
 
[void env_asktime_setdigit(uint8_t c, uint32_t i)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L277)
  segédfunkció az idősztring szerkesztéséhez
 
[unsigned char *env_bool(unsigned char *s, uint64_t *v, uint64_t flag)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L158)
  logikai hamis (false) értelmezése, egyébként alapból igazat ad vissza
 
[unsigned char *env_debug(unsigned char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L232)
  debug jelzők értelmezése
 
[unsigned char *env_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L118)
  decimális érték értelmezése, hexára vált 0x előtag esetén
 
[unsigned char *env_display(unsigned char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L169)
  megjelenítő értelmezése
 
[uint64_t env_getts(char *p, int16_t timezone)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L52)
  UTC másodperc időbélyeg kiszámítása lokális BCD vagy bináris idősztringből
 
[unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L99)
  hexa érték értelmezése
 
[void env_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L412)
  a környezeti változók szöveg értelmezése
 
[unsigned char *env_lang(unsigned char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L217)
  nyelvi kód értelmezése
 
[unsigned char *env_tz(unsigned char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c#L136)
  időzóna értelmezése
 
[void fault_intr(uint64_t exc, uint64_t errcode, uint64_t dummy)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/fault.c#L32)
  x86-on nincs igazi fault_intr(), mivel az IDT más címre adja eleve a vezérlést, lásd faultidt.S
 
[void lockacquire(int bit, uint64_t *ptr)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L35)
  jelző lefoglalása többprocesszoros rendszeren

[void lockrelease(int bit, uint64_t *ptr)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L50)
  jelző felszabadítása

[int memcmp(const void *s1, const void *s2, size_t len)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L173)
  memória összehasonlítása, architektúra specifikus implementáció

[void *memcpy(void *dst, const void *src, size_t len)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L61)
  memória másolása, architektúra specifikus implementáció

[void *memset(void *dst, uint8_t chr, size_t len)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S#L146)
  memória feltötlése adott karakterrel, architektúra specifikus implementáció

[void platform_awakecpu(uint16_t cpuid)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L83)
  felébresztés üresjáratból
 
[void platform_cpu()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L52)
  CPU ellenőrzése
 
[void platform_dbginit()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L91)
  soros vonali debug konzol inicializálása
 
[void platform_dbgputc(uint8_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L113)
  egy bájt küldése a debug konzolra
 
[void platform_env()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L33)
  platform függő környezeti változó alapértékek, env_init() hívja
 
[void platform_halt()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L75)
  futtatás felfüggesztése
 
[void platform_idle()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L39)
  CPU üresjárata
 
[void platform_load()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L228)
  CPU állapotának visszatöltése
 
[bool_t platform_memfault(void *ptr)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L118)
  eldönti egy címről, hogy elérhető-e, lehet, hogy nincs leképezve

[unsigned char *platform_parse(unsigned char *env)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L43)
  platform függő környezeti értékek értelmezése, env_init() hívja
 
[void platform_poweroff()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L52)
  számítógép kikapcsolása, kprintf_poweroff() hívja
 
[void platform_reset()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c#L67)
  számítógép újraindítása, kprintf_reboot() hívja
 
[void platform_save()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L193)
  CPU állapotának lementése
 
[void platform_srand()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L265)
  véletlenszám generátor inicializálása
 
[void platform_syscall()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L132)
  syscall hívás
  Be:
     funkció: rax
     paraméterek: rdi, rsi, rdx, r10, r8, r9
     visszatérési cím: rcx, flagek: r11
  Ki:
     visszatérési érték: rdx:rax
     hibakód: rdi
 
[void platform_waitkey()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S#L305)
  korai konzol implementáció, a kprintf használja
 
### Futtathatók
[elfcache_t *elf_getfile(char *fn)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L52)
  betölt egy ELF objektumot a háttértárolóról a gyorsítótárba
 
[bool_t elf_load(char *fn, phy_t extrabuf, size_t extrasiz, uint8_t reent)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L165)
  betölt egy ELF objektumot a gyorsítótárból az aktuális címtérbe, rekurzívan a függvénykönyvtáraival együtt
 
[virt_t elf_lookupsym(char *sym)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L717)
  cím keresése szimbólumhoz, sztring -> cím
 
[bool_t elf_rtlink(pmm_entry_t *devspec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L338)
  aktuális címtérben összelinkeli az ELF-eket (run-time linker)
 
[virt_t elf_sym(virt_t addr, bool_t onlyfunc)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L634)
  szimbólum keresése címhez, cím -> sztring
 
[void elf_unload(elfbin_t *elfbin)](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c#L119)
  kiszedi az ELF objektumokat az aktuális címtérből, és ha kell, a gyorsítótárból is
 
### Taszk
[void drivers_add(char *drv, pmm_entry_t *devspec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L166)
  új eszközmeghajtó taszk hozzáadása
 
[char *drivers_find(char *spec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L57)
  visszaadja az eszközspec-hez tartozó első meghajtó fájlnevét. Tipikus eszközspec "pciVVVV:MMMM" vagy "clsCC:SS"
 
[void drivers_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L304)
  az idle és FS taszk inicializálása és az eszközmeghajtók betöltése
 
[void drivers_intr(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L145)
  üzenet küldése a megszakításkezelő taszkoknak
 
[void drivers_ready()](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L387)
  akkor hívódik, amikor az összes eszközmeghajtó taszk incializálódott és blokkolódott teendőre várva
 
[void drivers_regintr(uint16_t irq, pid_t task)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L76)
  megszakításkezelő taszk hozzáadása a listához
 
[void drivers_unregintr(pid_t task)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L109)
  megszakításkezelő taszk eltávolítása a listából
 
[bool_t msg_allowed(tcb_t *sender, pid_t dest, evt_t event)](https://gitlab.com/bztsrc/osz/blob/master/src/core/security.c#L56)
  ellenőrzi, hogy az aktuális taszk elküldhet-e egy bizonyos üzenetet
 
[msgret_t msg_core(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, evt_t event)](https://gitlab.com/bztsrc/osz/blob/master/src/core/msgcore.c#L47)
  az SRV_CORE taszknak küldött üzenetek feldolgozása
 
[void msg_notify(pid_t pid, evt_t event, uint64_t a)](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c#L261)
  taszk értesítése, core nevében küld üzenetet. Ezt csakis a core használhatja
 
[msg_t *msg_recv(uint64_t serial)](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c#L35)
  üzenetek fogadása
 
[uint64_t msg_send(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, evt_t event, uint64_t serial)](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c#L119)
  üzenetek küldése
 
[void sched_add(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L95)
  egy taszk hozzáadása valamelyik aktív prioritási sorhoz
 
[void sched_awake(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L162)
  taszk mozgatása blokkolt vagy hibernált állapotból aktív sorba
   TCB_STATE_BLOCKED -> TCB_STATE_RUNNING
 
[void sched_block(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L213)
  taszk átmozgatása aktív sorból blokkolt sorba
   TCB_STATE_RUNNING -> TCB_STATE_BLOCKED
 
[void sched_delay(tcb_t *tcb, uint64_t usec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L260)
  a megadott mikroszekundumig blokkolja a taszkot
 
[void sched_hybernate(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L243)
  blokkolt sorból háttértárolóra küld egy taszkot
   TCB_STATE_BLOCKED -> TCB_STATE_HYBERND
 
[void sched_intr(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L82)
  ütemező megszakítás kezelője
 
[void sched_migrate(tcb_t *tcb, uint16_t cpuid)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L303)
  taszk átmozgatása egy másik processzor magra
 
[void sched_pick()](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L342)
  a következő taszk kiválasztása futásra. Ez egy agyonoptimalizált,
  O(1)-es, rekurzív prioritáslistás ütemező
 
[void sched_remove(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c#L122)
  taszk kivétele prioritási sorból
 
[void service_add(int srv, char *cmd)](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c#L231)
  új rendszerszolgáltatás taszk hozzáadása. Kicsit kilóg a sorból, de sok a közös elem az eszközmeghajtókkal, itt a helye
 
[bool_t task_allowed(tcb_t *tcb, char *grp, uint8_t access)](https://gitlab.com/bztsrc/osz/blob/master/src/core/security.c#L36)
  ellenőrzi, hogy az adott taszk rendelkezik-e a megadott jogosultsággal
 
[void task_del(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L88)
  megszüntet egy taszkot
 
[bool_t task_execfini(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L235)
  betöltés befejezése, a kódszegmens lecserélése vagy a kódbuffer eldobása
 
[bool_t task_execinit(tcb_t *tcb, char *cmd, char **argv, char **envp)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L110)
  egy futtatható paramétereinek betöltése a taszk címterébe. Mivel az exec() hiba esetén vissza
  kell térjen az eredeti kódszegmensre, ezért a felső bufferterületet használjuk ideiglenesen
 
[tcb_t *task_fork(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L76)
  lemásolja az aktuális taszkot
 
[tcb_t *task_new(uint8_t priority)](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c#L40)
  létrehoz egy új taszkot
 
### Debugger
[void dbg_brk(virt_t o, uint8_t m, uint8_t l)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L163)
  listázza a breakpointokat vagy beállít egy új breakpointot O címen, M módra, L hosszan
 
[virt_t dbg_disasm(virt_t addr, char *str)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L142)
  dekódol vagy hexában dumpol egy utasítást
 
[void dbg_dumpccb(ccb_t *ccb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L116)
  CPU Kontroll Blokk architektúrafüggő mezőinek dumpolása
 
[void dbg_dumpregs()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L107)
  speciális regiszterek dumpolása
 
[void dbg_fini()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L66)
  Platform specifikus debugger visszaállítás
 
[void dbg_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L54)
  Platform specifikus debugger inicializálás
 
[void dbg_pagingflags(uint64_t p)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L98)
  a P lapcímtárbejegyzés attribútumainak dumpolása
 
[void dbg_paginghelp()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L76)
  kiírja, hogy a lapcímtáblában melyik bit milyen attribútumot takar
 
[int dbg_parsecmd(char *cmd, uint64_t cmdlast)](https://gitlab.com/bztsrc/osz/blob/master/src/core/dbg.c#L722)
  parancsértelmező
 
[virt_t dbg_previnst(virt_t addr)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L133)
  visszaadja az előző utasítás címét
 
[void dbg_singlestep(bool_t enable)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c#L124)
  ki/bekapcsolja a lépésenkénti utasításvégrehajtást
 
[void dbg_start(char *header, bool_t ispanic)](https://gitlab.com/bztsrc/osz/blob/master/src/core/dbg.c#L819)
  beépített debugger
 
### Memória
[void *kalloc(size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L196)
  fizikai memória lefoglalása és leképezése a kernel memóriába

[void *kalloc_gap(size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L202)
  üres "lyuk" lefoglalása a kernel memóriába

[void kentropy()](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L161)
  véletlenszám generátor bitjeinek összekeverése entrópianövelés céljából
 
[void kfree(void *p)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L214)
  kernel memória felszabadítása

[void *krealloc(void *p, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L208)
  kernel memória átméretezése

[void *ksalloc(void *p, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L220)
  fizikai memória lefoglalása és leképezése a megosztott kernel memóriába

[void ksfree(void *p)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L232)
  megosztott kernel memória felszabadítása

[void *ksrealloc(void *p, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h#L226)
  megosztott kernel memória átméretezése

[void lang_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/lang.c#L44)
  nyelvi fájl betöltése, értelmezése és a szótár feltöltése
 
[size_t mbstrlen(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L107)
  utf-8 karakterek száma egy sztringben
 
[int memcmp(const void *s1, const void *s2, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L90)
  memória összehasonlítása, általános implementáció
 
[void *memcpy(void *dst, const void *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L50)
  memória másolása, általános implementáció
 
[void *memset(void *dst, uint8_t c, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L68)
  memória feltötlése adott karakterrel, általános implementáció
 
[void* pmm_alloc(tcb_t *tcb, uint64_t pages)](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L193)
  folyamatos fizikai memória lefoglalása. Ficikai címmel tér vissza
 
[void* pmm_allocslot(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L259)
  egy egybefüggő, címigazított blokk (2M) lefoglalása. Fizikai címmel tér vissza
 
[void pmm_free(tcb_t *tcb, phy_t base, size_t pages)](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L331)
  fizikai memória felszabadítása és a szabad listához adása
 
[void pmm_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L53)
  fizikai Memória Kezelő inicializálása
 
[void pmm_vmem()](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c#L175)
  inicializálás befejezése, miután lett virtuális memóriánk
 
[char *sprintf(char *dst,char* fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L279)
  sztring összerakása formázás és paraméterek alapján
 
[int strcmp(const char *s1, const char *s2)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L137)
  két sztring összehasonlítása
 
[size_t strlen(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L127)
  bájtok száma egy sztringben
 
[char *strncpy(char *dst, const char *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c#L149)
  legfeljebb N karakter hosszú sztring másolása
 
[void vmm_del(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L377)
  virtuális cimtér megszüntetése, és a hozzá tartozó fizikai RAM felszabadítása
 
[void vmm_free(tcb_t *tcb, virt_t bss, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L288)
  memória leképezés megszüntetése a virtuális cimtérben, és a hozzá tartozó fizikai RAM felszabadítása
 
[void vmm_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c#L46)
  Virtuális Memória Kezelő inicializálása
 
[void *vmm_map(tcb_t *tcb, virt_t bss, phy_t phys, size_t size, uint64_t access)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L93)
  általános memória leképezése a virtuális címtérbe. Ha phys 0, új memóriát foglal. Többek között a bztalloc is hívja
 
[void *vmm_maptext(tcb_t *tcb, virt_t bss, phy_t *phys, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L198)
  kódszegmmens leképezése a virtuális címtérbe. Ez mindig a taszk kódbufferébe dolgozik, elf_load() hívja
 
[tcb_t *vmm_new(uint16_t cpu, uint8_t priority)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c#L152)
  létrehoz egy új virtuális címteret
 
[void vmm_page(virt_t memroot, virt_t virt, phy_t phys, uint64_t access)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L38)
  egy fizikai lap leképezése a virtuális címtérbe, nem igényel tcb-t, optimalizált, csak a core használja
 
[void vmm_swaptextbuf(tcb_t *tcb)](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c#L254)
  aktív kódszegmmens és a taszk kódbufferének cseréje, task_execfini() hívja, ha sikeres volt a betöltés
 
### Megszakítás
[void clock_ack()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/rtc.S#L80)
  falióra megszakítás visszaigazolása

[void clock_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/clock.c#L46)
  falióra inicializálása
 
[void clock_intr(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/core/clock.c#L60)
  falióra megszakítás kezelője
 
[void intr_disable(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S#L80)
  egy adott IRQ vonal letiltása

[void intr_enable(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S#L59)
  egy adott IRQ vonal engedélyezése

[uint intr_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/intr.c#L43)
  megszakításvezérlő inicializálása
 
[void intr_nextsched(uint64_t usec)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/intr.c#L88)
  generál egy megszakítást usec mikroszekundum múlva (PIT one-shot)
 
[bool_t intr_nextschedremainder()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/intr.c#L71)
  segédfüggvény, ha a számláló nem képes egy megszakítással usec-et várni
 
[void pic_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S#L41)
  csip inicializálás

[void rtc_init()](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/rtc.S#L41)
  valós idejű óra inicializálása

Fájlok
------

| Fájl | Alrendszer | Leírás |
| ---- | ---------- | ------ |
| [core/clock.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/clock.c) | megszakítás | falióra funkciók. Nanoszekundumban számol, de csak 1/clock_freq másodpercenként frissül |
| [core/core.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/core.h) | memória | Core funkciók (ring 0 / EL1) |
| [core/dbg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/dbg.c) | debugger | Beépített Debugger |
| [core/drivers.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c) | taszk | eszközmeghajtó taszk funkciók |
| [core/elf.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c) | futtathatók | ELF betöltő és értelmező |
| [core/elf.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.h) | futtathatók | ELF betöltő és értelmező |
| [core/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c) | platform | Környezeti változók értelmezése (lásd FS0:\BOOTBOOT\CONFIG vagy /sys/config) |
| [core/env.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.h) | platform | Környezeti változók |
| [core/fault.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/fault.c) | platform | platform független kivétel kezelők |
| [core/fault.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/fault.h) | platform | platform független kivétel kezelők |
| [core/fs.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/fs.c) | fájlrendszer | FS taszk előtti, FS/Z initrd-t kezelő funkciók (csak olvasás) |
| [core/kprintf.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c) | konzol | Felügyeleti szinten futó printf implementáció, korai konzol |
| [core/lang.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/lang.c) | memória | Többnyelvű fordítások támogatása |
| [core/lang.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/lang.h) | memória | Nyelvi fordítások szótára |
| [core/libc.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/libc.c) | memória | Alacsony szintű függvénykönyvtár a core-hoz |
| [core/main.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/main.c) | platform | Core, fő platformfüggetlen lépések a BSP-n |
| [core/msg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c) | taszk | üzenetküldés, a core-nak küldött üzenetek feldolgozása az msgcore.c-ben van |
| [core/msg.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.h) | taszk | üzenet sor fejléce |
| [core/msgcore.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/msgcore.c) | taszk | a core-nak küldött üzenetek feldolgozása, az általános üzenetküldés az msg.c-ben van |
| [core/pmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c) | memória | Fizikai Memória Kezelő |
| [core/pmm.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.h) | memória | Fizikai Memória Kezelő |
| [core/sched.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c) | taszk | Taszk ütemező |
| [core/security.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/security.c) | taszk | üzenetküldés biztonsági házirend |
| [core/syslog.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c) | konzol | Korai rendszernaplózó. Megosztott területet használ a "syslog" taszkkal |
| [core/task.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c) | taszk | taszk funkciók |
| [core/task.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.h) | taszk | taszk funkciók |
| [core/vmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c) | memória | Virtuális Memória Kezelő, architektúra független rész |
| [core/vmm.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.h) | memória | Virtuális Memória Kezelő |
| [core/x86_64/arch.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/arch.h) | platform | Architektúra függő headerök |
| [core/x86_64/ccb.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ccb.h) | taszk | CPU Kontrol Blokk. architektúra függő struktúra |
| [core/x86_64/dbg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/dbg.c) | debugger | Beépített Debugger, architektúra függő eljárások |
| [core/x86_64/disasm.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/disasm.h) | debugger | Disassembler |
| [core/x86_64/fault.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/fault.c) | platform | kivétel kezelő |
| [core/x86_64/faultidt.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/faultidt.S) | platform | alacsony szintű kivétel kezelők |
| [core/x86_64/ibmpc/intr.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/intr.c) | megszakítás | Megszakításvezérlő és Intel 8253 Programmable Interval Timer eszközmeghajtó |
| [core/x86_64/ibmpc/pic.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S) | megszakítás | i8259A Programmable Interrupt Controller megszakításvezérlő eszközmeghajtó |
| [core/x86_64/ibmpc/platform.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c) | platform | Platform specifikus funkciók |
| [core/x86_64/ibmpc/platform.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.h) | platform | Platform specifikus definíciók |
| [core/x86_64/ibmpc/rtc.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/rtc.S) | megszakítás | Motorola MC146818 Real Time Clock eszközmeghajtó, periodikus falióra |
| [core/x86_64/idt.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/idt.h) | memória | Megszakítás Leíró Tábla (IDT) definíciói |
| [core/x86_64/libc.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/libc.S) | platform | Alacsony szintű függvénykönyvtár |
| [core/x86_64/platform.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S) | platform | Architektúra függő, platform specifikus függvények |
| [core/x86_64/start.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/start.S) | platform | Belépési pont |
| [core/x86_64/task.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/task.h) | taszk | Taszk Kontrol Blokk. Architectúra függő struktúra |
| [core/x86_64/vmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c) | memória | Virtuális Memória Kezelő, architektúra függő rész |
| [core/x86_64/vmm.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.h) | memória | Virtuális Memória Kezelő, architektúra függő rész |
| [fs/cache.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c) | fájlrendszer | blokk gyorsítótár |
| [fs/devfs.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/devfs.c) | fájlrendszer | beépített eszköz fájlrendszer (devfs) |
| [fs/fcb.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fcb.c) | fájlrendszer | Fájl Kontrol Blokkok kezelése |
| [fs/fsdrv.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/fsdrv.c) | fájlrendszer | Fájl Rendszer Meghajtók kezelése |
| [fs/main.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/main.c) | fájlrendszer | FS taszk |
| [fs/mtab.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/mtab.c) | fájlrendszer | felcsatolási pontok kezelése |
| [fs/pipe.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/pipe.c) | fájlrendszer | csővezetékek kezelése |
| [fs/taskctx.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c) | fájlrendszer | taszk kontextus fájlrendszer szolgáltatásokhoz |
| [fs/vfs.c](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c) | fájlrendszer | VFS illesztési réteg |


