OS/Z Bájtkód
============

Egy modern operációs rendszernek támogatnia kell platform-független bájtkód futtatását. Hosszas és alapos tanulmányozás után
arra jutottam, hogy a futtatását shell-szkriptekhez hasonlóan kell kezelni, felhasználói módban futó alkalmazással. Ami a
bájtkódot magát illeti, a következő opciókat vizsgáltam:

- python: bár elsőre jó jelöltnek tűnik, kihajítottam, mert a bájtkódja egy nem sztandardizált förtelem.
- java: egy másik esélyes, de sajnos a bájtkód túlságosan a Java nyelvhez igazodik, a meglévő VM-ek brutálisan nagyok, amiket
    lehetetlen portolni egy hobbi OS-re. A teljes újraírás ugyancsak komplikált, hogy mást ne mondjak.
- lua: a könnyen portolható ANSI C függvénykönyvtár jónak tűnt. Végül azért esett ki, mert túlságosan nyelvhez kötött,
    nem C-szerű a szintaxisa, és nincs jól elkülönítve benne a fordító/értelmező (menthetetlenül össze van mosva a kettő egy libbe).
- wasm: végül a WebAssembly mellett döntöttem. Egyszerű, nyelv-független, könnyen implementálható, és a legtöbb független szakértő
    szerint ez lesz a jövő szabványa. Továbbá van már minimális C implementációja ([wac](https://github.com/kanaka/wac)) és
    fullos C++ implementációja ([wavm](https://github.com/WAVM/WAVM)) is, ráadásul mind böngészőfüggetlen szerencsére.

A futtatható formátum ellenőrzése a [task_execinit](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c) eljárásban
található, míg maga az értelmező az [usr/wasmvm](https://gitlab.com/bztsrc/osz/blob/master/usr/wasmvm) alatt.
