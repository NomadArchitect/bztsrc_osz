OS/Z Függvényreferenciák
========================

Prototípusok
------------
Összesen 168 függvény van definiálva.

### Eszközmeghajtók
[void drv_add(char *drv, mem_entry_t *memspec)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c#L53)
  eszközmeghajtóprogram hozzáadása (csak eszközmeghajtók hívhatják)
 
[void drv_close(dev_t device)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/bridge/pci/main.c#L170)
  amikor bezárják az eszközfájlt (eszközmeghajtók implementálják)
 
[void drv_find(char *spec, char *drv, int len)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c#L47)
  eszközmeghajtóprogram keresése eszközspecifikáció alapján (csak eszközmeghajtók hívhatják)
  visszatérési érték: 1 ha van, 0 ha nincs
 
[void drv_init()](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/bridge/pci/main.c#L59)
  busz pásztázása, eszközök detektálása vagy eszköz inicializálás (eszközmeghajtók implementálják)
 
[void drv_ioctl(dev_t device)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/bridge/pci/main.c#L191)
  eszközparancs (eszközmeghajtók implementálják)
 
[void drv_irq(uint16_t irq, uint64_t ticks)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/bridge/pci/main.c#L156)
  drv_regirq() vagy drv_regtmr() esetén hívódik, utóbbinál irq == USHRT_MAX (eszközmeghajtók implementálják)
 
[void drv_open(dev_t device, uint64_t mode)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/bridge/pci/main.c#L163)
  akkor hívódik, ha valaki megnyitja a mknod() által kreált fájlt (eszközmeghajtók implementálják)
 
[void drv_read(dev_t device)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/bridge/pci/main.c#L177)
  olvasás az eszközfájlból (eszközmeghajtók implementálják)
 
[void drv_regirq(uint16_t irq)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c#L37)
  irq üzenetek kérése (csak eszközmeghajtók hívhatják)
 
[void drv_regtmr()](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c#L42)
  másodpercenkénti időzítő üzenetek kérése (csak eszközmeghajtók hívhatják, USHRT_MAX irq üzeneteket küld)
 
[void *drv_virtmem(phy_t addr)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c#L58)
  rendszermemória virtuális címének lekérdezése (csak eszközmeghajtók hívhatják)
 
[void drv_write(dev_t device)](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/bridge/pci/main.c#L184)
  írás az eszközfájlba (eszközmeghajtók implementálják)
 
[bool_t env_bool(char *key, bool_t def)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/env.c#L63)
  logikai érték változó visszaadása (csak eszközmeghajtók és szolgáltatások számára elérhető)
 
[uint64_t env_num(char *key, uint64_t def, uint64_t min, uint64_t max)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/env.c#L38)
  szám típusú változó visszaadása (csak eszközmeghajtók és szolgáltatások számára elérhető)
 
[char *env_str(char *key, char *def)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/env.c#L94)
  sztring visszaadása egy allokált bufferben, amit a hívónak kell felszabadítania (csak eszközmeghajtók és szolgáltatások
  számára elérhető)
 
### Libc
[void abort()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L240)
  programfutás azonnali felfüggesztése
 
[#define abs(x)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L123)
  X abszolút értékét adja vissza
 
[void *aligned_alloc(uint_t a, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L78)
  ISO C címhelyes allokálás, S bájt lefoglalása A-val osztható címen

[int atexit(void (*func)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L211)
  funkció regisztrálása 'exit'-el való kilépéskor
 
[int atoi(char *c)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L170)
  decimális (vagy 0x előtag esetén hexadecimális) sztring átalakítássa 32 bites integer számmá
 
[long int atol(char *c)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L182)
  decimális (vagy 0x előtag esetén hexadecimális) sztring átalakítássa 64 bites long integer számmá
 
[char *basename(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L589)
  elérési út fájlnév részét adja vissza egy újonnan allokált bufferben
 
[void *bsearch(void *key, void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *))](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L194)
  KEY felezéses keresése BASE-ben, ami NMEMB darab SIZE méretű elemből áll, a CMP funkció használatával
 
[void *bzt_alloc(uint64_t *arena, size_t a, void *ptr, size_t s, int flag)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/bztalloc.c#L152)
  címigazított memória lefoglalása egy arénában. alacsony szint, használd inkább a malloc(), calloc(), realloc() hívást
 
[void bzt_free(uint64_t *arena, void *ptr)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/bztalloc.c#L67)
  memória felszabadítása és ha lehetséges, RAM felszabadítása a rendszer számára. alacsony szint, használd inkább a free()-t
 
[void *calloc(uint n, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L66)
  N darab, egyenként S bájt lefoglalása

[fid_t chdir(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L71)
  a taszk munkakönyvtárának beállítása PATH-ra.
 
[void chr(uint8_t **s, uint32_t u)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L41)
  utf-8 szekvenciává alakítja az UNICODE-ot, majd lépteti a sztringmutatót
 
[fid_t chroot(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L55)
  beállítja a PATH-ot gyökérkönyvtárnak (az abszolút elérési út kiindulópontja).
  ezt csak rendszergazda jogosultságokkal hívható.
 
[int closedir(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L524)
  STREAM könyvtárleíró lezárása. 0-át ad vissza, ha sikeres, -1 -et ha nem.
 
[void closelog()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/syslog.c#L46)
  rendszernaplózóhoz küldött üzenetek alaphelyzetbe állítása
 
[uint32_t crc32a_calc(char *start, size_t length)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/crc32.h#L108)
  EFI kompatíbilis (ANSI) ellenörzőösszeg kiszámítása
 
[uint32_t crc32c_calc(char *start, size_t length)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/crc32.h#L98)
  CRC32 Castagnoli ellenörzőösszeg kiszámítása
 
[void dbg_bztdump(uint64_t *arena)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/bztalloc.c#L429)
  foglaltsági memóriatérkép listázása, debuggolási célra (csak ha DEBUG = 1)
 
[void dbg_msg(msg_t *msg)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L47)
  egy üzenet formázott dumpolása (csak ha DEBUG = 1)
 
[void dbg_printf(const char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L133)
  debug üzenet kiírása a kernel konzolra (csak ha DEBUG = 1)

[char *dirname(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L606)
  elérési út könyvtár részét adja vissza egy újonnan allokált bufferben
 
[#define distance(x,y)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L135)
  X és Y távolságát adja vissza
 
[stat_t *dstat(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L397)
  a STREAM fájlhoz tartozó, st_dev-ben visszaadott eszköz attribútumainak lekérése csak olvasható bufferbe
 
[fid_t dup(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L143)
  STREAM duplikálása, egy új fájlleírót ad vissza ugyanarra a megnyitott fájlra
 
[fid_t dup2(fid_t stream, fid_t stream2)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L152)
  STREAM duplikálása STREAM2-re, a STREAM2-t lezárja és megnyitja ugyanarra a fájlra
 
[int errno()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L63)
  hibakód lekérdezése
 
[void exit(int status)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L228)
  az 'atexit' által regisztrált függvények hívása, majd kilépés STATUS kóddal
 
[int fclose(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L210)
  lezárja a STREAM fájlleírón megnyitott fájlt.
 
[int fcloseall()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L219)
  minden fájlleírót lezár.
 
[void fclrerr(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L254)
  az EOF és hibajelzők törlése STREAM fájlleírón.
 
[bool_t feof(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L262)
  visszaadja az EOF fájl vége jelzőt a STREAM fájlleíróhoz.
 
[int ferror(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L271)
  visszaadja a STREAM fájlleíróhoz tartozó hiba jelzőbiteket.
 
[int fflush(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L370)
  írási buffer kiürítése STREAM fájlleírón, vagy mindegyiken ha STREAM -1.
 
[uint64_t ffs(uint64_t i)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/string.S#L200)
  visszaadja az első beállított bit pozícióját I-ben, vagy 0-át, ha nincs.
  a legkissebb helyiértékű bit az 1-es, a legmagasabb a 64.

[int fgetc(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L311)
  egy UTF-8 karakter olvasása STREAM fájlleíróból.
 
[fid_t fopen(const char *filename, mode_t oflag)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L180)
  megnyit egy fájlt és visszad egy STREAM fájlleírót hozzá
 
[pid_t fork()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L249)
  az aktuális címtér klónozása
 
[int fprintf(fid_t stream, const char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L731)
  formázott szöveg kiírása STREAM fájlleíróba
 
[int fputc(fid_t stream, int c)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L331)
  egy UTF-8 karakter kiírása STREAM fájlleíróba.
 
[int fputs(fid_t stream, char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L351)
  sztring kiírása STREAM fájlleíróba.
 
[size_t fread(fid_t stream, void *ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L280)
  adatok olvasása STREAM fájlleíróból.
 
[void free(void *p)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L84)
  'malloc', 'realloc' vagy 'calloc' által lefoglalt memória felszabadítása

[fid_t freopen(const char *filename, mode_t oflag, fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L195)
  lecserél egy meglévő STREAM fájlleírót egy újonnan megnyitott fájlra
 
[int fseek(fid_t stream, off_t off, int whence)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L228)
  beállítja a pozíciót egy STREAM fájlleírón.
 
[stat_t *fstat(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L411)
  STREAM fájlleírón megnyitott fájl, csővezeték vagy socket attribútumainak lekérése csak olvasható bufferbe
 
[fpos_t ftell(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L237)
  STREAM fájlleíró aktuális pozícióját adja vissza.
 
[size_t fwrite(fid_t stream, void *ptr, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L295)
  adatok kiírása STREAM fájlleíróba.
 
[int getchar()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L323)
  egy UTF-8 karakter olvasása stdin fájlleíróból.
 
[char *getcwd()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L86)
  az aktuális munkakönyvtár visszaadása egy allokált bufferben
 
[int getentropy(void *buffer, size_t length)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L269)
  egy buffer feltöltése kriptográfiában használható véletlen adatokkal
 
[uid_t getuid(void)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L213)
  visszaadja a hívó taszk felhasználójának azonosítóját

[uid_t getuidp(pid_t pid)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L226)
  visszaadja egy taszk felhasználójának azonosítóját

[int ioctl(fid_t stream, uint64_t code, void *buff, size_t size)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L161)
  B/K parancs küldése egy STREAM-en megnyitott eszköznek. BUFF lehet NULL.
 
[bool_t isatty(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L829)
  1-et ad vissza ha a STREAM fájlleíró egy terminálon van megnyitva, egyébként 0-át
 
[void lockacquire(int bit, uint64_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L150)
  felfüggeszti a futást (yield), míg nem sikerül a jelzőbitet lefoglalni

[int lockid(uint64_t id, uint64_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L177)
  megadott azonosítóval foglalja le a jelzőt. Nem blokkol, 0-át ad vissza, ha sikerült

[void lockrelease(int bit, uint64_t *mem)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L166)
  jelzőbit felszabadítása

[stat_t *lstat(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L379)
  PATH attribútumainak lekérése a csak olvasható bufferbe
 
[void *malloc(size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L60)
  S bájt lefoglalása

[#define max(x,y)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L131)
  X és Y közül a nagyobb
 
[size_t mbstrlen(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L284)
  visszaadja a sztring karaktereinek számát (ami lehet kevesebb, mint a bájthossz)
 
[size_t mbstrnlen(const char *s, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L304)
  visszaadja a sztring karaktereinek számát legfeljebb N bájt hosszig
 
[void *memchr(const void *s, int c, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L174)
  bájt első előfordulásának keresése. Nem UTF-8 biztos
 
[int memcmp(void *s1, void *s2, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/string.S#L177)
  összehasonlít N bájtot S1 és S2 címen.

[void *memcpy(void *dest, void *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/string.S#L37)
  átmásol N bájtot SRC-ből DEST-be nagy sebességgel.

[meminfo_t meminfo()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L201)
  visszaadja a szabad és összes memória lapok számát

[void *memmem(const void *haystack, size_t hl, const void *needle, size_t nl)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L207)
  NEDDLELEN hosszú NEEDLE első előfordulásának keresése a HAYSTACKLEN hosszú HAYSTACK-ben
 
[void *memmove(void *dst, const void *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L154)
  átmozgat N bájtot SRC-ből DST-be úgy, hogy a két terület átfedheti egymást
 
[void *memrchr(const void *s, int c, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L190)
  bájt utolsó előfordulásának keresése. Nem UTF-8 biztos
 
[void *memrmem(const void *haystack, size_t hl, const void *needle, size_t nl)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L226)
  NEDDLELEN hosszú NEEDLE utolsó előfordulásának keresése a HAYSTACKLEN hosszú HAYSTACK-ben
 
[void *memset(void *s, int c, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/string.S#L150)
  Az S sztring N bájtját C-vel tölti fel.

[void *memzero(void *s, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/string.S#L126)
  az S sztring N bájtját kinullázza.

[#define min(x,y)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L127)
  X és Y közül a kissebb
 
[int mknod(const char *devname, dev_t minor, mode_t mode, blksize_t size, blkcnt_t cnt)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L40)
  eszközhivatkozás hozzáadása (csak eszközmeghajtók és szolgáltatások hívhatják)
 
[void *mmap(void *addr, size_t len, int prot, int flags, fid_t fid, off_t offs)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L111)
  memória lefoglalálsa és leképezése a címtérbe
 
[int mount(const char *dev, const char *mnt, const char *opts)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L97)
  statikus felcsatolási pont hozzáadása
 
[msg_t *mq_call(arg0, arg1, arg2, arg3, arg4, arg5, event)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L65)
  blokkoló, üzenet küldése és várakozás válaszra

[uint64_t mq_dispatch()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/dispatch.c#L42)
  üzenet sor diszpécser. Kiveszi az üzenetet, meghívja a feldolgozó függvényt és visszaküldi a választ.
  vagy ENOEXEC hibát ad, vagy nem tér vissza
 
[uint64_t mq_ismsg()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L105)
  visszaadja a sorszámot vagy 0-át, ha nincs várakozó üzenet

[msg_t *mq_recv()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L90)
  blokkoló, addig várakozik, míg üzenet nem érkezik

[uint64_t mq_send(arg0, arg1, arg2, arg3, arg4, arg5, event, serial)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L45)
  nem-blokkoló, üzenet küldése

[int munmap(void *addr, size_t len)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L121)
  memória felszabadítása a címtérből
 
[fid_t opendir(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L464)
  a PATH-on található könyvtár vagy unió megnyitása, STREAM könyvtárleírót ad vissza
 
[void openlog(char *ident, int option, int facility)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/syslog.c#L36)
  rendszernaplózóhoz küldött üzenetek beállítása
 
[uint32_t ord(uint8_t **s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L68)
  lépteti a sztringmutatót a következő utf-8 karakterre és visszaadja az UNICODE kódpontot
 
[void perror(char *cmd, char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L759)
  stderr-re kiírja szövegesen az errno hibakód jelentését.
 
[int printf(const char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L749)
  formázott szöveg kiírása stdout-ra
 
[int putchar(int c)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L343)
  egy UTF-8 karakter kiírása stdout fájlleíróba.
 
[int puts(char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L359)
  sztring és egy sorvége kiírása stdout fájlleíróba.
 
[void qsort(void *aa, size_t n, size_t es, int (*cmp)(void *, void *))](https://gitlab.com/bztsrc/osz/blob/master/src/libc/qsort.c#L80)
  gyorsrendezés AA-ban, ami N elemű, ES elemméretű tömb, CMP összehasonlítás használatával
 
[uint64_t rand()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L264)
  0 és URAND_MAX közötti, kritográfiában használható véletlenszám lekérdezése
 
[dirent_t *readdir(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L502)
  könyvtárbejegyzés olvasása STREAM könyvtárleíróból. Csak olvasható dirent bufferrel tér vissza,
  vagy NULL-al, ha nincs több bejegyzés.
 
[char *readlink(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L442)
  a PATH szimbólikus hivatkozás vagy unió céljának lekérése allokált bufferbe
 
[void *realloc(void *p, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L72)
  korábbi P lefoglalás méretének megváltoztatása S-re

[char *realpath(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L425)
  kanonikus abszolút elérési út visszaadása PATH-hoz egy allokált bufferben
 
[void rewind(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L246)
  a STREAM fájlleíró vagy opendir könyvtárleírót az elejére pozícionálja.
 
[void *saligned_alloc(uint_t a, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L110)
  címhelyes, megosztott allokálás, S bájt lefoglalása A-val osztható címen

[void *scalloc(uint n, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L98)
  N darab, egyenként S bájt lefoglalása megosztott memóriában

[void seterr(int e)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L58)
  hibakód beállítása
 
[int setlogmask(int mask)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/syslog.c#L57)
  prioritás beállítása
 
[void sfree(void *p)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L116)
  megosztott memória felszabadítása

[void *smalloc(size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L92)
  S bájt lefoglalása megosztott memóriában

[int snprintf(char *dst, size_t size, const char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L700)
  formázott szöveg kiírása DST sztringbe, legfeljebb SIZE bájt hosszan
 
[int sprintf(char *dst, const char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L690)
  formázott szöveg kiírása DST sztringbe
 
[void srand(uint64_t seed)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L259)
  véletlenszám generátor entrópiájának növelése
 
[void *srealloc(void *p, size_t s)](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h#L104)
  korábbi P megosztott lefoglalás méretének megváltoztatása S-re

[void stime(uint64_t usec)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L274)
  rendszeridő beállítása
 
[void stimebcd(char *timestr)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L279)
  rendszeridő beállítása BCD sztringből (0xÉÉ 0xÉÉ 0xHH 0xNN 0xÓÓ 0xPP 0xMM)
 
[void stimezone(int16_t min)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L284)
  rendszeridő időzónájának beállítása percekben (UTC -1440 .. +1440)
 
[int strcasecmp(const char *s1, const char *s2)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L350)
  kis- és nagybetűfüggetlen sztring összehasonlítás, 0-át ad vissza, ha egyezik
 
[char *strcasecpy(char *dst, const char *src)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L425)
  SRC kisbetűsített másolása DST-be
 
[char *strcasedup(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L475)
  kisbetűsített S duplikálása egy újonnan allokált bufferbe
 
[char *strcasestr(const char *haystack, const char *needle)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L554)
  hasonló a 'strstr'-hez, de kis- és nagybetű függetlenül keresi az első előfordulást
 
[char *strcat(char *dst, const char *src)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L377)
  SRC sztring hozzáfűzése DST sztringhez
 
[char *strchr(const char *s, uint32_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L499)
  karakter első előfordulásának keresése a sztringben, ez UTF-8 biztos
 
[int strcmp(const char *s1, const char *s2)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L325)
  sztring összehasonlítás, 0-át ad vissza, ha egyezik
 
[char *strcpy(char *dst, const char *src)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L402)
  SRC sztring másolása DST-be
 
[char *strdup(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L450)
  S duplikálása egy újonnan allokált bufferbe
 
[char *strerror(int errnum)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L246)
  visszaadja az ERRNUM 'errno' kód szöveges megfelelőjét sztringben
 
[size_t strlen(const char *s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L263)
  visszaadja a sztring bájthosszát
 
[int strncasecmp(const char *s1, const char *s2, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L363)
  kis- és nagybetűfüggetlen sztring összehasonlítás legfeljebb N hosszig, 0-át ad vissza, ha egyezik
 
[char *strncasecpy(char *dst, const char *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L437)
  legfeljebb N hosszú SRC kisbetűsített másolása DST-be
 
[char *strncasedup(const char *s, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L487)
  legfeljebb N kisbetűs karakter duplikálása S stringből egy újonnan allokált bufferbe
 
[char *strncat(char *dst, const char *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L389)
  legfeljebb N hosszú SRC sztring hozzáfűzése DST sztringhez
 
[int strncmp(const char *s1, const char *s2, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L337)
  sztring összehasonlítás legfeljebb N hosszig, 0-át ad vissza, ha egyezik
 
[char *strncpy(char *dst, const char *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L413)
  legfeljebb N hosszú SRC sztring másolása DST-be
 
[char *strndup(const char *s, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L461)
  legfeljebb N karakter duplikálása S stringből egy új allokált bufferbe
 
[size_t strnlen(const char *s, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L273)
  visszaadja a sztring bájthosszát legfeljebb N hosszig
 
[char *strrcasestr(const char *haystack, const char *needle)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L571)
  hasonló a 'strrstr'-hez, de kis- és nagybetű függetlenül keresi az utolsó előfordulást
 
[char *strrchr(const char *s, uint32_t c)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L516)
  karakter utolsó előfordulásának keresése, UTF-8 biztos
 
[char *strrstr(const char *haystack, const char *needle)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L546)
  NEEDLE utolsó előfordulásának keresése a HAYSTACK sztringben
 
[char *strsep(char **stringp, const char *delim)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L673)
  visszaadja a DELIM határolt sztringet, 0-ával lezárva, *STRINGP-t pedig a következő bájtra állítva
 
[char *strsignal(int sig)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L255)
  visszaadja a SIG szignál szöveges nevét sztringben
 
[char *strstr(const char *haystack, const char *needle)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L538)
  NEEDLE első előfordulásának keresése a HAYSTACK sztringben
 
[char *strtok(char *s, const char *delim)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L656)
  S feldarabolása DELIM karaktereknél
 
[char *strtok_r(char *s, const char *delim, char **ptr)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L665)
  S feldarabolása DELIM karaktereknél, a tagok a PTR tömbbe kerülnek
 
[uint32_t strtolower(uint8_t **s)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L94)
  lépteti a sztringmutatót a következő utf-8 karakterre és visszaadja a kisbetűs változatot
 
[void syslog(int pri, char *fmt, ...)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/syslog.c#L89)
  formázott üzenet küldése a rendszernaplózónak, változó számú paraméterrel
 
[uint64_t time()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L289)
  rendszeridő lekérdezése mikroszekundumban
 
[fid_t tmpfile()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L171)
  létrehoz egy ideiglenes fájlt és megnyitja írásra / olvasásra
 
[void trace(bool_t enable)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L106)
  nyomkövetés ki/bekapcsolása
 
[void tskcpy(pid_t dst, void *dest, void *src, size_t n)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c#L681)
  memória másolása egy másik taszk címterébe
 
[char *ttyname(fid_t stream)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L815)
  visszaadja a STREAM fájlleíróhoz tartozó terminál eszköz nevét, vagy NULL-t
  a visszaadtott érték csak a kovetkező h0vásig érvényes
 
[int ttyname_r(fid_t stream, char *buf, size_t buflen)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L803)
  nem több, mint BUFLEN bájtot lement a STREAM-hez tartozó terminál eszköz elérési útjából, 0-át ad vissza, ha sikerült
 
[int umount(const char *path)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L128)
  statikus felcsatolási pont eltávolítása, a PATH lehet eszköznév vagy könyvtár is
 
[void usleep(uint64_t usec)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c#L254)
  a futó taszk altatása USEC mikroszekundumig
 
[int vfprintf(fid_t stream, const char *fmt, va_list args)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L710)
  formázott szöveg kiírása STREAM fájlleíróba, argumentumlistával
 
[int vprintf(const char *fmt, va_list args)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L741)
  formázott szöveg kiírása stdout-ra, argumentumlistával
 
[int vsnprintf(char *dst, size_t size, const char *format, va_list args)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L553)
  egyszerű sprintf implementáció, formázott szöveg írása DST-be, maximum SIZE hosszan, argumentumlistával
 
[int vsprintf(char *dst, const char *fmt, va_list args)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c#L682)
  formázott szöveg kiírása DST sztringbe, argumentumlistával
 
[void vsyslog(int pri, char *fmt, va_list ap)](https://gitlab.com/bztsrc/osz/blob/master/src/libc/syslog.c#L66)
  formázott üzenet küldése a rendszernaplózónak, paraméterlistával
 
[void yield()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S#L189)
  felfüggeszti a futást

Fájlok
------

| Fájl | Alrendszer | Leírás |
| ---- | ---------- | ------ |
| [include/osZ/stdlib.h](https://gitlab.com/bztsrc/osz/blob/master/src/include/osZ/stdlib.h) | libc | ISO C99 Sztandard: 7.20 általános funkciók, plusz OS/Z specifikus szolgáltatások |
| [drivers/bridge/pci/main.c](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/bridge/pci/main.c) | eszközmeghajtók | PCI busz felderítés |
| [drivers/driver.c](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c) | eszközmeghajtók | eszközmeghajtó diszpécser és alap funkciók |
| [libc/bztalloc.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/bztalloc.c) | libc | Memória allokátor és deallokátor |
| [libc/bztalloc.h](https://gitlab.com/bztsrc/osz/blob/master/src/libc/bztalloc.h) | libc | Memória allokátor és deallokátor header |
| [libc/crc32.h](https://gitlab.com/bztsrc/osz/blob/master/src/libc/crc32.h) | libc | ellenörzőösszeg számító eljárások |
| [libc/dispatch.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/dispatch.c) | libc | üzenet sor diszpécser |
| [libc/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/env.c) | eszközmeghajtók | induló környezet értelmező függvények |
| [libc/env.h](https://gitlab.com/bztsrc/osz/blob/master/src/libc/env.h) | eszközmeghajtók | induló környezet szolgáltatások |
| [libc/libc.h](https://gitlab.com/bztsrc/osz/blob/master/src/libc/libc.h) | libc | belső struktúrák és változók |
| [libc/qsort.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/qsort.c) | libc | Gyorsrendezés az OpenBSD-ből, kis módosításokkal az OS/Z-hez |
| [libc/stdio.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdio.c) | libc | az stdio.h-ban definiált függvények megvalósítása |
| [libc/stdlib.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c) | libc | az stdlib.h-ban definiált függvények megvalósítása |
| [libc/string.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/string.c) | libc | a string.h-ban definiált függvények megvalósítása, van egy (platform)/string.S is |
| [libc/syslog.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/syslog.c) | libc | a syslog.h-ban definiált függvények megvalósítása |
| [libc/x86_64/crt0.S](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/crt0.S) | libc | Zéró szintű C futtatás (x86_64-crt0) |
| [libc/x86_64/platform.h](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/platform.h) | libc | platform függő libc definíciók |
| [libc/x86_64/stdlib.S](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S) | libc | alacsony szintű sztandard libc funkciók, lásd stdlib.h |
| [libc/x86_64/string.S](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/string.S) | libc | alacsony szintű sztring kezelés |


