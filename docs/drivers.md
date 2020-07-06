OS/Z Eszközmeghajtók
====================

Supported devices
-----------------

 * VESA 2.0 VBE, GOP, VideoCore (a [betöltő](https://gitlab.com/bztsrc/osz/blob/master/loader) állítja, 32 bites frame buffer)
 * x86_64: syscall, NX védelem
 * x86_64-ibmpc: [PIC](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S), [PIT](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pit.S), [RTC](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/rtc.S)
 * PS2 [billentyűzet](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/input/ps2_8042/keyboard.h) és [egér](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/input/ps2_8042/mouse.h)
 * AArch64: SVC, WNX védelem
 * aarch64-rpi: [BCM megszakítás vezérlő](https://gitlab.com/bztsrc/osz/blob/master/src/core/aarch64/rpi/intr.c), ARM Generic Timers, ARM Local Timer

Tervezett eszközök
------------------

 * x86_64-apic: HPET

Leírás
------

Kétfajta eszközmeghajtó létezik: szoftveres és hardveres meghajtó. A szoftveres meghajtók valamelyik rendszer szolgáltatás
címterébe töltődnek be (például a fájlrendszer meghajtók vagy a tty és x11 meghajtók), míg a hardveres meghajtóknak
saját külön taszkjuk van.

A meghajtók megosztott függvénykönyvtárak, amiket a saját címterükbe tölt be a rendszer egy közös diszpécser után, lásd [driver.c](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c).
Elérik a B/K címeket az in/out függvényekkel és az MMIO is le van képezve a címterükbe. Ettől eltekintve az eszközmeghajtók
sima felhasználói szintű alkalmazások. A `core` mindig egy adott platformra fordul, amit az ARCH és PLATFORM változók szabályoznak
a [Config](https://gitlab.com/bztsrc/osz/blob/master/Config)-ban. A lehetséges kombinációkért lásd a [fordítás](https://gitlab.com/bztsrc/osz/blob/master/docs/compile.md)-nál.

### Könyvtárak

Az eszközmeghajtók az [src/drivers](https://gitlab.com/bztsrc/osz/blob/master/src/drivers) alatt találhatók, kategorizálva.
Minden [eszköz osztály](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/README.md) pontosan egy könyvtár, és alatta minden
eszközmeghajtónak külön almappája van. A lefordított meghajtó a `sys/drv/(osztály)/(almappa).so` alá kerül. Páldául az
`src/drivers/input/ps2_8042/main.c` a `sys/drv/input/ps2_8042.so`-ba kerül lefordításra.

Megjegyzés: hatékonyságnövelés érdekében a CPU kezelő, memória kezelő és megszakítás vezérlő a [core](https://gitlab.com/bztsrc/osz/tree/master/src/core)-ba
került, nincs külön meghajtójuk. Hogy leceréljük ezeket, a `PLATFORM` vátozót kell állítani és újrafordítani.

| PLATFORM     | Megszakítás vezérlő  | Ütemező időzítő | Falióra          | Rendszerbusz |
| ------------ | -------------------- | --------------- | ---------------- | ------------ |
| x86_64-ibmpc | PIC                  | PIT             | RTC              | PCI          |
| x86_64-apic  | IOAPIC               | LAPIC Timer     | HPET             | PCI express  |
| aarch64-rpi  | BCM Interrupt Ctrl   | ARM CNTP_TVAL   | ARM Local Timer  | -            |

### Fájlok

A meghajtó fájlai között kettő kitüntetett szerepet kap:

| Név  | Leírás |
| ---- | ----------- |
| devices | eszközazonosítók listája, amiket ez a meghajtó kezel |
| platforms | azon platformok listája, amikre ezt az eszközmeghajtót le kell fordítani |

Mindkét fájl újsor (0x0A) határolt szólista. A devices fájlban a következő bejegyzések lehetnek:

| Bejegyzés | Leírás |
| --------- | ----------- |
| *     | bármi, azt jelenti, hogy a busz pásztázástól függetlenül be kell tölteni a meghajtót |
| pciXXXX:XXXX:XXXX:XXXX | PCI vendor és device id pár alrendszer vendor id és alrendszer id párral |
| pciXXXX:XXXX | PCI vendor és device id pár, alrendszer azonosítók nélkül |
| clsXX:XX | PCI osztály definíció |
| FS    | fájlrendszer meghajtó |
| UI    | felhasználói felület meghajtó |
| inet  | hálózati protokoll meghajtó |
| (stb) |  |

Ezek az információk az eszközmeghajtó elérési útjával együtt a `sys/drivers` fájlba kerülnek, amit a
[tools/elftool.c](https://gitlab.com/bztsrc/osz/blob/master/tools/elftool.c) állít elő. Ezt az adatbázist
bootolásnál induláskor használja a rendszer annak megállapítására, hogy melyik eszközmeghajtókat kell
betölteni a felderített eszközökhöz.

A platforms fájl csak felsorolja a platformokat, például "x86_64-ibmpc" vagy "aarch64-rpi". Használható joker is,
például "x86_64-*". Fordításkor ellenőrzi a rendszer, és ha az a platform amire épp fordítunk nincs felsorolva, akkor a
meghajtót nem fordítja le. Ez egy egyszerű módja annak, hogy elkerüljük a nem kompatíbilis eszközmeghajtók lefordítását,
ugyanakkor lehetővé tegyük, hogy a platformfüggetlen meghajtókat (mint páldául az USB adattároló) ne kelljen újraírni
minden platformhoz. Ha a platforms fájl hiányzik, az eszközmeghajtó minden platformra lefordul.

Eszközmeghajtók licenszelési szempontból kivételt képeznek, a forrásukat nem kötelező megosztani, és bináris terjesztés
is mengedett. Ehhez a megfelelő architektúrára fordított, megoszható és dinamikusan linkelhető ELF binárist "(ARCH).so" néven
kell a könyvtárba helyezni a devices és platforms fájlok mellé.

### Eszközmeghajtók írása

Ha eszközmeghajtóval szeretnéd bővíteni az OS/Z-t, [itt](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-driver.md)
találsz hozzá leírást.
