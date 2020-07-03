OS/Z betöltők
=============

1. *x86_64-efi* a javasolt megoldás x86_64 architektúrán.
    [bootboot.efi](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.efi) (94k)

2. *x86_64-bios* BIOS, Multiboot (GRUB) és El Torito (CDROM) kompatíbilis, ELAVULT betöltő.
    [boot.bin](https://gitlab.com/bztsrc/bootboot/raw/master/boot.bin) (512 bájt, MBR, VBR és CDROM boot szektor), [bootboot.bin](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.bin) (11k, betölthető a boot.bin által, BBS Expansion ROM-ként, Multiboot-al (GRUB), valamint Linux kernelként)

3. *aarch64-rpi* ARMv8 betöltő Raspberry Pi 3-hoz, 4-hez
    [bootboot.img](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.img) (31k)

BOOTBOOT Protokoll
==================

A forrás és a dokumentáció átköltözött a saját [repójába](https://gitlab.com/bztsrc/bootboot).

Titkosítás támogatása
=====================

Titkosított FS/Z lemezképek hozhatók létre a "mkfs (imgfile) encrypt sha/aes" paranccsal. Minden betöltő támogatja a SHA-XOR-CBC kódolást, de csak az UEFI verzió tud AES-256-CBC kódolt initrd-ről bootolni.
