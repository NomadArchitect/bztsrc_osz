OS/Z - Hogyan Sorozat #1 - Tesztelés
====================================

Előszó
------

Mielőtt mélyebbre merülnénk, fontos azzal kezdeni, miként lehet az OS/Z-t telepítés nélkül kipróbálni.

Mindig akkor pusholok a git repóba, amikor a legjobb tudomásom szerint hiba nélkül [lefordul](https://gitlab.com/bztsrc/osz/blob/master/docs/compile.md)
a forrás egy `make clean all` paranccsal. Bár mindent elkövetek, nincs garancia rá, hogy valamilyen előre nem látható okból mégse
menne, vagy hogy a jól leforduló kód valami miatt ne kresselhetne el :-)

A [legfrissebb lemezkép](https://gitlab.com/bztsrc/osz/tree/master/bin/)el indítható az OS/Z emulátorban vagy igazi vason. Például

```shell
qemu-system-x86_64 -hda bin/osZ-latest-x86_64-ibmpc.img
```

De a kényelem kedvéért make szabályokat csináltam, amik megkönnyítik a dolgot.

Az aktuálisan lefordított lemezkép indítása a neki megfelelő környezetben:

```shell
make test
```

Vagy egy betű hozzábiggyesztésével ki is választható a környezet. A lemezkép indítása qemu-val TianoCore UEFI förmverrel

```shell
make teste
```

Indítás qemu-al BIOS környezetben

```shell
make testq
```

Indítás bochs emulátorral BIOS környezetben

```shell
make testb
```

Végül indítás VirtualBox-al (ez lehet BIOS vagy UEFI is, attól függően, hogy konfiguráltad):

```shell
make testv
```

Ha érdekel, hogyan lehet [debuggolni](https://gitlab.com/bztsrc/osz/blob/master/docs/howto2-debug.md), olvass tovább.
