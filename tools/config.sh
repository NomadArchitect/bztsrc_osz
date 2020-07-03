#!/bin/sh

#
# tools/tools.sh
#
# Copyright (c) 2017 bzt (bztsrc@gitlab)
# https://creativecommons.org/licenses/by-nc-sa/4.0/
#
# A művet szabadon:
#
# - Megoszthatod — másolhatod és terjesztheted a művet bármilyen módon
#     vagy formában
# - Átdolgozhatod — származékos műveket hozhatsz létre, átalakíthatod
#     és új művekbe építheted be. A jogtulajdonos nem vonhatja vissza
#     ezen engedélyeket míg betartod a licensz feltételeit.
#
# Az alábbi feltételekkel:
#
# - Nevezd meg! — A szerzőt megfelelően fel kell tüntetned, hivatkozást
#     kell létrehoznod a licenszre és jelezned, ha a művön változtatást
#     hajtottál végre. Ezt bármilyen ésszerű módon megteheted, kivéve
#     oly módon ami azt sugallná, hogy a jogosult támogat téged vagy a
#     felhasználásod körülményeit.
# - Ne add el! — Nem használhatod a művet üzleti célokra.
# - Így add tovább! — Ha feldolgozod, átalakítod vagy gyűjteményes művet
#     hozol létre a műből, akkor a létrejött művet ugyanazon licensz-
#     feltételek mellett kell terjesztened, mint az eredetit.
#
# @subsystem eszközök
# @brief Segédprogram az OS/Z konfigurálására, make config hívja
#

COMPCFG=../Config
BOOTCFG=../etc/sys/config

# opciók beolvasása Makefájlokból
OPTS=()
function getoptions()
{
    OPTS=()
    for fn in `find $1 -mindepth 1 -maxdepth 1 -type d|sort`; do
        OPTS+=(${fn##*/} "`cat $fn/Makefile | head -1 | cut -b 3-`")
    done
}

# Jelenlegi konfiguráció beolvasása
function getconfig()
{
    # --- fordítás idejű opciók ---
    arch=`cat $COMPCFG | grep -v '#' | grep -e ^ARCH | cut -d '=' -f 2 | xargs`
    platform=`cat $COMPCFG | grep -v '#' | grep -e ^PLATFORM | cut -d '=' -f 2 | xargs`
    if [ "`cat $COMPCFG | grep -v '#' | grep -e ^CC | grep gcc`" != "" ]; then cc="gcc"; else cc="clang"; fi
    if [ "`cat $COMPCFG | grep -v '#' | grep -e ^DEBUG | grep 1`" != "" ]; then debug=1; else debug=0; fi
    if [ "`cat $COMPCFG | grep -v '#' | grep -e ^CFLAGS | grep '\-g'`" != "" ]; then dbgsym="-g"; else dbgsym=""; fi
    if [ "`cat $COMPCFG | grep -v '#' | grep -e ^OPTIMIZE | grep 1`" != "" ]; then optimize=1; else optimize=0; fi
    if [ "`cat $COMPCFG | grep -v '#' | grep -e ^NOINITRD | grep 1`" != "" ]; then noinitrd=1; else noinitrd=0; fi
    espsize=`cat $COMPCFG | grep -v '#' | grep -e ^ESPSIZE | cut -d '=' -f 2 | xargs`
    usrsize=`cat $COMPCFG | grep -v '#' | grep -e ^USRSIZE | cut -d '=' -f 2 | xargs`
    varsize=`cat $COMPCFG | grep -v '#' | grep -e ^VARSIZE | cut -d '=' -f 2 | xargs`
    homesize=`cat $COMPCFG | grep -v '#' | grep -e ^HOMESIZE | cut -d '=' -f 2 | xargs`
    # --- futás idejű opciók ---
    tz=`cat $BOOTCFG | grep -v '//' | grep -e ^tz | cut -d '=' -f 2 | xargs`
    # UI
    screen=`cat $BOOTCFG | grep -v '//' | grep -e ^screen | cut -d '=' -f 2 | xargs`
    display=`cat $BOOTCFG | grep -v '//' | grep -e ^display | cut -d '=' -f 2 | cut -d ',' -f 1 | xargs`
    if [ "`cat $BOOTCFG | grep -v '//' | grep -e ^dblbuf | grep true`" != "" ]; then dblbuf="true"; else dblbuf="false"; fi
    if [ "`cat $BOOTCFG | grep -v '//' | grep -e ^focusnew | grep true`" != "" ]; then focusnew="true"; else focusnew="false"; fi
    if [ "`cat $BOOTCFG | grep -v '//' | grep -e ^lefthanded | grep true`" != "" ]; then lefthanded="true"; else lefthanded="false"; fi
    lang=`cat $BOOTCFG | grep -v '//' | grep -e ^lang | cut -d '=' -f 2 | xargs`
    kbd=`cat $BOOTCFG | grep -v '//' | grep -e ^keyboard | cut -d '=' -f 2 | cut -d ',' -f 1 | xargs`
    map=`cat $BOOTCFG | grep -v '//' | grep -e ^keyboard | cut -d '=' -f 2 | cut -d ',' -f 2 | xargs`
    alt=`cat $BOOTCFG | grep -v '//' | grep -e ^keyboard | cut -d '=' -f 2 | cut -d ',' -f 3 | xargs`
    if [ "`cat $BOOTCFG | grep -v '//' | grep -e ^spacemode | grep true`" != "" ]; then spacemode="true"; else spacemode="false"; fi
    # szolgáltatások
    if [ "`cat $BOOTCFG | grep -v '//' | grep -e ^rescueshell | grep true`" != "" ]; then rescueshell="true"; else rescueshell="false"; fi
    if [ "`cat $BOOTCFG | grep -v '//' | grep -e ^syslog | grep false`" == "" ]; then syslog="true"; else syslog="false"; fi
    if [ "`cat $BOOTCFG | grep -v '//' | grep -e ^networking | grep false`" == "" ]; then networking="true"; else networking="false"; fi
    if [ "`cat $BOOTCFG | grep -v '//' | grep -e ^sound | grep false`" == "" ]; then sound="true"; else sound="false"; fi
    # debug üzenetek
    dbgmsg=`cat $BOOTCFG | grep -v '//' | grep -e ^debug | cut -d '=' -f 2 | xargs`
}

# Üzleti logika a konfigurációs értékeken
function configlogic()
{
    if [ "$arch" == "" -o "$platform" == "" ]; then arch="x86_64"; platform="ibmpc"; fi
    if [ "$platform" == "rpi3" -o "$platform" == "rpi4" ]; then noinitrd=0; espmin=8256; else espmin=4096; fi
    if [ $espsize -le $espmin ]; then espsize = $espmin; fi
    if [ $noinitrd -eq 1 ]; then initrd="boot $PARTITION"; boot="Initrd"; else initrd="ESP:/BOOTBOOT/INITRD"; boot="ESP"; fi
    if [ "$tz" == "detect" ]; then tz=""; fi
    if [ "$tz" != "" -a "$tz" != "ask" ]; then tz=$[$tz+0]; fi
    if [ "$tz" == "" -a "$platform" == "rpi3" ]; then tz="ask"; fi
    if [ "$tz" == "" -a "$platform" == "rpi4" ]; then tz="ask"; fi
    if [ "$tz" == "" ]; then tzdet="$AUTODETECT"; else tzdet=""; fi
    if [ "$screen" == "" ]; then screen="1024x768"; fi
    if [ "$display" == "" ]; then display="mc"; fi
    if [ "$lang" == "" ]; then lang="en"; fi
    if [ "$kbd" == "" ]; then kbd="pc105"; fi
    if [ "$map" == "" ]; then map="en_us"; fi
    if [ "$alt" == "" ]; then mapsep=""; else mapsep=", alt: "; fi
    if [ $debug -eq 0 ]; then dbgsym=""; fi
}

# Konfiguráció lementése
function saveconfig()
{
saved=""
cat >tmp <<EOF
# OS/Z - an operating system for hackers
# Copyright (c) 2017 bzt (bztsrc@gitlab), CC-by-nc-sa
# Use is subject to license terms.
# --- generated by "make config" ---

# --- common configuration ---
ARCH = $arch
PLATFORM = $platform
DEBUG = $debug
OPTIMIZE = $optimize

# --- disk layout ---
#setting this to 1 will create a partition for root fs
NOINITRD = $noinitrd
#size of partitions in kilobytes
ESPSIZE = $espsize
USRSIZE = $usrsize
VARSIZE = $varsize
HOMESIZE = $homesize

# --- build system ---
CFLAGS =$dbgsym -O2 -fno-delete-null-pointer-checks -fno-stack-protector -fvisibility=hidden -ansi -pedantic -Wall -Wextra
include \$(dir \$(lastword \$(MAKEFILE_LIST)))src/core/\$(ARCH)/\$(PLATFORM)/Makefile.opt
EOF
# A -Wno-builtin-declaration-mismatch és a -Wno-builtin-requires-header ugyanaz, csak máshogy
# hívja a két fordító. Azért kell, mert nem akarunk egészen POSIX kompatibilisek lenni
if [ "$cc" == "gcc" ]; then
    cat >>tmp <<EOF
CC=\${ARCH}-elf-gcc -Wno-builtin-declaration-mismatch
LD=\${ARCH}-elf-ld
EOF
else
    echo "CC=clang --target=\${ARCH}-elf -Wno-builtin-requires-header -Wno-incompatible-library-redeclaration" >>tmp
    # GNU ld választása, ha az LLVM ld nem található
    if [ ! -x /usr/bin/ld.lld ]; then
        echo "LD=\${ARCH}-elf-ld" >>tmp
    else
        # azok az LLVM fejlesztők nem normálisak. Miért kellett eltérő target sztringet használni a linkernél???
        echo "ifeq (\${ARCH},x86_64)" >>tmp
        echo "LD=ld.lld -m elf_x86_64" >>tmp
        echo "else" >>tmp
        echo "LD=ld.lld -m aarch64elf" >>tmp
        echo "endif" >>tmp
        # echo "LD=ld.lld -m \${ARCH}-elf" >>tmp
    fi
fi
dialog --backtitle "OS/Z $CONFTXT - $SUPERVISE" --cancel-label "$CANCELTXT" --title "$COMPILETIME (Config)" --editbox tmp `stty size` 2>/dev/null 1>&3
[ $? -eq 0 ] && cp tmp ../Config && saved="Config $SAVEDTXT\n"

if [ "$tz" == "" ]; then tz="0"; tzz="//"; else tzz=""; fi
cat >tmp <<EOF
// BOOTBOOT Options
// --- generated by "make config" ---

// requested screen dimension
screen=$screen

// elf or pe binary to load
kernel=sys/core

// number of pages for message queue
nrmqmax=1

// scheduler quantum. A task can run continuously up
// to (quantum) microseconds.
quantum=1000

// override time zone (in minutes, or "ask")
${tzz}tz=$tz

// display visual and optionally a driver name after a comma (',')
// like "display=mc,nvidia"
display=$display
// use double buffering. Requires more RAM, but much better
dblbuf=$dblbuf

// automatically focus new windows
focusnew=$focusnew

// swap pointers
lefthanded=$lefthanded

// start rescue shell and other system services
spacemode=$spacemode
rescueshell=$rescueshell
syslog=$syslog
networking=$networking
sound=$sound

// language of user interface
lang=$lang

// keyboard
keyboard=$kbd,$map

// debug options
debug=$dbgmsg

EOF
dialog --backtitle "OS/Z $CONFTXT - $SUPERVISE" --cancel-label "$CANCELTXT" --title "$BOOTOPTS (etc/sys/config)" --editbox tmp `stty size` 2>/dev/null 1>&3
[ $? -eq 0 ] && cp tmp ../etc/sys/config && saved="${saved}etc/sys/config $SAVEDTXT\n"
rm tmp
}

# Konfiguráló nyelvi fordításai
if [ "${LANG:0:2}" = "hu" ]; then
    CONFTXT="Konfigurálás"
    QUITTXT="Kilépés"
    SAVETXT="Elment"
    CANCELTXT="Mégsem"
    CONFTOOL="Üdvözli az OS/Z konfiguráló"
    COMPILETIME="Fordítási opciók"
    COMPILER="Fordító"
    COMPILEROPT="Fordító paraméterek"
    DISKGEN="Lemezkép generálás"
    BOOTOPTS="Futás idejű opciók"
    SYSTEMSERVICES="Rendszer szolgáltatások"
    TIMEZONE="Időzóna"
    RESOLUTION="Képernyőfelbontás"
    VISUAL="Megjelenítés"
    DISPLAYOPTS="Megjelenítési opciók"
    INTERFACELANG="Felület nyelve"
    KBDTYPE="Billentyűzet típusa"
    KBDLAYOUT="Billentyűzet kiosztás"
    DBGMSG="Debug opciók"
    SELARCH="Válassz architektúrát"
    SELPLAT="Válassz platformot"
    SELCOMP="Válassz fordítót"
    ADDOPTS="További opciók"
    COMPOPT1="Beépített debugger és debug konzol"
    COMPOPT2="Debug szimbólumokkal fordítás"
    COMPOPT3="Optimalizált és SIMD kód generálás"
    INITRDPLACE="Initrd helye"
    PARTITION="partíció"
    SYSPART="Rendszer"
    VARPART="Publikus adat"
    HMEPART="Privát adat"
    AUTODETECT="autodetektált"
    RPIINITRD="Raspberry Pi nem támogatja az initrd partíciót"
    SIZETXT="mérete"
    SELSERVICE="Válaszd ki, mely rendszer szolgáltatások induljanak"
    SPCMOD="Automatikus újraindítás hiba esetén"
    SRVRSH="Parancsértelmező (init helyett)"
    SRVLOG="Naplózás szolgáltatás"
    SRVNET="Internet szolgáltatás"
    SRVSND="Hangkeverő szolgáltatás"
    SELTZ="Időzóna megadása"
    SELASK="Mindig kérdezze meg a felhasználót"
    SELUTC="Mindig legyen UTC (greenwichi középidő)"
    SELMAN="Beállítom most fixre"
    SETTZ="Időzóna megadása percekben"
    USERI="Felhasználói felület"
    SELRES="Válassz képernyőfelbontást"
    SELVIS="Válassz megjelenítőt"
    OPTMM="Mono Monokróm (kevés memóriát igényel)"
    OPTMC="Mono trueColor (alapértelmezett)"
    OPTSM="Sztereó Monokróm (anaglif)"
    OPTSC="Sztereó trueColor (igazi 3D hatás)"
    OPTDBL="Kettősbufferelés (dupla memóriát igényel)"
    OPTFNEW="Új ablakok automatikus fókuszálása"
    OPTLEFT="Balkezes egér mutató"
    SELLANG="Nyelvválasztás"
    SELKBDTYPE="Válassz billentyűzet típust"
    SELKBDLAYOUT="Válassz elsődleges kiosztást"
    SELKBDALT="Válassz másodlagos kiosztást"
    NONETXT="Nincs"
    SELDBG="Válaszd ki, mely üzeneteket jelenjenek meg"
    DBGPRT="debug paranccsor a legelső taszkkapcsolás előtt"
    DBGLOG="Rendszernapló küldése a korai konzolra"
    DBGDEV="Detektált eszközök listázása"
    DBGTEST="Rendszertesztek futtatása (init helyett)"
    DBGMEM="Memóriatérkép listázása a debug konzolra"
    DBGTASK="Taszk létrehozás nyomkövetése"
    DBGELF="ELF betöltés nyomkövetése"
    DBGRI="Futás idejű importált szimbólumok nyomkövetése"
    DBGRE="Futás idejű exportált szimbólumok nyomkövetése"
    DBGIRQ="IRQ Routing Table listázása"
    DBGSCHED="Ütemező nyomkövetése"
    DBGMQ="Üzenetek nyomkövetése (mint a Linux strace)"
    DBGPM="Fizikai memóriafoglalás nyomkövetése"
    DBGVM="Virtuális memórialeképezés nyomkövetése (core)"
    DBGMA="Virtuális memóriafoglalás nyomkövetése (libc malloc)"
    DBGBLK="Blokk szintű B/K nyomkövetése"
    DBGFILE="Fájl szintű B/K nyomkövetése"
    DBGFS="Fájlrendszerek nyomkövetése (felcsatolás, stb.)"
    DBGCACHE="Blokk gyorsítótár nyomkövetése"
    DBGUI="Felhasználói felület üzenetek nyomkövetése"
    SUPERVISE="Ellenőrzés"
    SAVEDTXT="elmentve."
else
    CONFTXT="Configuration"
    QUITTXT="Quit"
    SAVETXT="Save"
    CANCELTXT="Cancel"
    CONFTOOL="Welcome to the OS/Z Configuration tool"
    COMPILETIME="Compile time options"
    COMPILER="Compiler"
    COMPILEROPT="Compiler options"
    DISKGEN="Disk image generation"
    BOOTOPTS="Boot options"
    SYSTEMSERVICES="System services"
    TIMEZONE="Time zone"
    RESOLUTION="Screen resolution"
    VISUAL="Display visual"
    DISPLAYOPTS="Display options"
    INTERFACELANG="Interface language"
    KBDTYPE="Keyboard type"
    KBDLAYOUT="Keyboard layout"
    DBGMSG="Debug options"
    SELARCH="Select architecture"
    SELPLAT="Select platform"
    SELCOMP="Select compiler"
    ADDOPTS="Additional options"
    COMPOPT1="Internal debugger and debug console"
    COMPOPT2="Compile with debug symbols"
    COMPOPT3="Use optimized code and utilize SIMD"
    INITRDPLACE="Initrd place"
    PARTITION="partition"
    SYSPART="System"
    VARPART="Public data"
    HMEPART="Private data"
    AUTODETECT="autodetect"
    RPIINITRD="Raspberry Pi does not support initrd on a partition."
    SIZETXT="size"
    SELSERVICE="Select system services to start on boot up"
    SPCMOD="Automatic reboot on failure"
    SRVRSH="Rescue shell (instead of init)"
    SRVLOG="System log service"
    SRVNET="Internet service"
    SRVSND="Sound service"
    SELTZ="Select time zone"
    SELASK="Always ask user"
    SELUTC="Force UTC (Greenwich meantime)"
    SELMAN="Set manually"
    SETTZ="Set time zone in minutes"
    USERI="User Interface"
    SELRES="Select screen resolution"
    SELVIS="Select display visual"
    OPTMM="Mono Monochrome (small memory)"
    OPTMC="Mono trueColor (default)"
    OPTSM="Stereo Monochrome (anaglyph)"
    OPTSC="Stereo trueColor (Real 3D)"
    OPTDBL="Double buffering (more memory)"
    OPTFNEW="Automatically focus new windows"
    OPTLEFT="Left handed pointers"
    SELLANG="Select language"
    SELKBDTYPE="Select keyboard type"
    SELKBDLAYOUT="Select keyboard layout"
    SELKBDALT="Select alternative layout"
    NONETXT="None"
    SELDBG="Select debug messages to show"
    DBGPRT="debugger prompt before the first task switch"
    DBGLOG="Send syslog to boot console"
    DBGDEV="Dump detected devices"
    DBGTEST="Run system tests (instead of init)"
    DBGMEM="Dump memory map to debug console (boot log has it)"
    DBGTASK="Show task creation"
    DBGELF="Trace ELF loading"
    DBGRI="Dump run-time linker imported symbols"
    DBGRE="Dump run-time linker exported symbols"
    DBGIRQ="Dump IRQ Routing Table"
    DBGSCHED="Trace scheduler"
    DBGMQ="Trace message queue (aka Linux strace)"
    DBGPM="Trace physical memory allocations"
    DBGVM="Trace virtual memory mapping (core)"
    DBGMA="Trace virtual memory allocation (libc malloc)"
    DBGBLK="Trace block level I/O"
    DBGFILE="Trace file level I/O"
    DBGFS="Trace file systems (mounts and such)"
    DBGCACHE="Trace block cache"
    DBGUI="Trace UI requests and messages"
    SUPERVISE="Supervise"
    SAVEDTXT="saved."
fi

# Fő menü
getconfig
saved=""
exec 3>&1
while true; do
    configlogic
    size=`stty size| cut -d ' ' -f 1`
    if [ $size -gt 25 ]; then msize=25; else msize=$[$size-3]; fi
    MENU=$(dialog --backtitle "OS/Z $CONFTXT" --default-item "$MENU" --cancel-label "$QUITTXT" --extra-button --extra-label \
    "$SAVETXT" --colors --menu "$CONFTOOL" $msize 60 $[$msize-4] \
    "" "\Z4  --- $COMPILETIME ---  " \
    1 "Platform ($arch/$platform)" \
    2 "$COMPILER ($cc)" \
    3 "$COMPILEROPT" \
    4 "$DISKGEN" \
    "" "\Z4  --- $BOOTOPTS ---  " \
    5 "$SYSTEMSERVICES" \
    6 "$TIMEZONE ($tz$tzdet)" \
    7 "$RESOLUTION ($screen)" \
    8 "$VISUAL ($display)" \
    9 "$DISPLAYOPTS" \
    10 "$INTERFACELANG ($lang)" \
    11 "$KBDTYPE ($kbd)" \
    12 "$KBDLAYOUT ($map$mapsep$alt)" \
    13 "$DBGMSG" \
    2>&1 1>&3)
    ret=$?
    [ $ret -eq 1 ] && break
    if [ $ret -eq  3 ]; then
        saveconfig
        [ "$saved" != "" ] && break
        continue
    fi
    case $MENU in
        1)
            getoptions ../src/core
            arch=`dialog --backtitle "OS/Z $CONFTXT - Platform" --default-item "$arch" --no-cancel --menu "$SELARCH" 10 40 4 \
            "${OPTS[@]}" \
            2>&1 1>&3`
            getoptions ../src/core/$arch
            platform=$(dialog --backtitle "OS/Z $CONFTXT - Platform" --default-item "$platform" --no-cancel --menu "$SELPLAT" 10 44 4 \
            "${OPTS[@]}" \
            2>&1 1>&3)
        ;;

        2)
            cc=$(dialog --backtitle "OS/Z $CONFTXT - $COMPILER" --default-item "$cc" --no-cancel --menu "$SELCOMP" 10 40 4 \
            gcc "GNU C + ld" \
            clang "LLVM CLang + lld" \
            2>&1 1>&3)
        ;;

        3)
            if [ $debug -eq 1 ]; then opt1="on"; else opt1=""; fi
            if [ $dbgsym != "" ]; then opt2="on"; else opt2=""; fi
            if [ $optimize -eq 1 ]; then opt3="on"; else opt3=""; fi
            ANSWER=$(dialog --backtitle "OS/Z $CONFTXT - $COMPILEROPT" --cancel-label "$CANCELTXT" --checklist "$ADDOPTS" 10 70 4 \
            DEBUG "$COMPOPT1" "$opt1" \
            DBGSYM "$COMPOPT2" "$opt2" \
            OPTIMIZE "$COMPOPT3" "$opt3" \
            2>&1 1>&3)
            if [ "$?" != "1" ]; then
                if [ -z "${ANSWER##*DEBUG*}" ]; then debug=1; else debug=0; fi
                if [ -z "${ANSWER##*DBGSYM*}" ]; then dbgsym="-g"; else dbgsym=""; fi
                if [ -z "${ANSWER##*OPTIMIZE*}" ]; then optimize=1; else optimize=0; fi
            fi
        ;;

        4)
            while true; do
                configlogic
                DMENU=$(dialog --backtitle "OS/Z $CONFTXT - $DISKGEN" --cancel-label "$CANCELTXT" --default-item "$DMENU" \
                --menu "$DISKGEN" 12 60 7 \
                1 "$INITRDPLACE ($initrd)" \
                2 "/boot $boot $PARTITION ($espsize KiB)" \
                3 "/usr $SYSPART $PARTITION ($usrsize KiB)" \
                4 "/var $VARPART $PARTITION ($varsize KiB)" \
                5 "/home $HMEPART $PARTITION ($homesize KiB)" \
                2>&1 1>&3)
                [ $? -eq 1 ] && break
                case $DMENU in
                    1)
                        noinitrd=$[1-$noinitrd]
                        if [ "$platform" == "rpi3" -o "$platform" == "rpi4" ]; then dialog --backtitle "OS/Z $CONFTXT" --msgbox "$RPIINITRD" 5 56; fi
                    ;;

                    2)
                        opt=$(dialog --backtitle "OS/Z $CONFTXT - $DISKGEN" --cancel-label "$CANCELTXT" --rangebox "EFI System $PARTITION (/boot) $SIZETXT, KiB" 8 60 $espmin 4194304 $espsize 2>&1 1>&3)
                        if [ "$opt" != "" ]; then espsize=$opt; fi
                    ;;

                    3)
                        opt=$(dialog --backtitle "OS/Z $CONFTXT - $DISKGEN" --cancel-label "$CANCELTXT" --rangebox "$SYSPART $PARTITION (/usr) $SIZETXT, KiB" 8 60 512 4194304 $usrsize 2>&1 1>&3)
                        if [ "$opt" != "" ]; then usrsize=$opt; fi
                    ;;

                    4)
                        opt=$(dialog --backtitle "OS/Z $CONFTXT - $DISKGEN" --cancel-label "$CANCELTXT" --rangebox "$VARPART $PARTITION (/var) $SIZETXT, KiB" 8 60 4 4194304 $varsize 2>&1 1>&3)
                        if [ "$opt" != "" ]; then varsize=$opt; fi
                    ;;

                    5)
                        opt=$(dialog --backtitle "OS/Z $CONFTXT - $DISKGEN" --cancel-label "$CANCELTXT" --rangebox "$HMEPART $PARTITION (/home) $SIZETXT, KiB" 8 60 4 4194304 $homesize 2>&1 1>&3)
                        if [ "$opt" != "" ]; then homesize=$opt; fi
                    ;;
                esac
            done
        ;;

        5)
            if [ "$spacemode" == "true" ]; then opt1="on"; else opt1=""; fi
            if [ "$rescueshell" == "true" ]; then opt2="on"; else opt2=""; fi
            if [ "$syslog" == "true" ]; then opt3="on"; else opt3=""; fi
            if [ "$networking" == "true" ]; then opt4="on"; else opt4=""; fi
            if [ "$sound" == "true" ]; then opt5="on"; else opt5=""; fi
            ANSWER=$(dialog --backtitle "OS/Z $CONFTXT - $SYSTEMSERVICES" --cancel-label "$CANCELTXT" --checklist "$SELSERVICE" 15 70 5 \
            spacemode "$SPCMOD" "$opt1" \
            rescueshell "$SRVRSH" "$opt2" \
            syslog "$SRVLOG" "$opt3" \
            networking "$SRVNET, TCP/IP" "$opt4" \
            sound "$SRVSND" "$opt5" \
            2>&1 1>&3)
            if [ "$?" != "1" ]; then
                if [ -z "${ANSWER##*spacemode*}" ]; then spacemode="true"; else spacemode="false"; fi
                if [ -z "${ANSWER##*rescueshell*}" ]; then rescueshell="true"; else rescueshell="false"; fi
                if [ -z "${ANSWER##*syslog*}" ]; then syslog="true"; else syslog="false"; fi
                if [ -z "${ANSWER##*networking*}" ]; then networking="true"; else networking="false"; fi
                if [ -z "${ANSWER##*sound*}" ]; then sound="true"; else sound="false"; fi
            fi
        ;;

        6)
            if [ "$tz" != "" -a "$tz" != "ask" -a "$tz" != "0" ]; then tzz=$tz; else tzz=60; fi
            ANSWER=$(dialog --backtitle "OS/Z $CONFTXT - $TIMEZONE" --default-item "$tz" --no-cancel --menu "$SELTZ" 11 60 5 \
            "" "($AUTODETECT)" \
            "ask" "$SELASK" \
            "0" "$SELUTC" \
            "$tzz" "$SELMAN" \
            2>&1 1>&3)
            if [ "$ANSWER" == "$tzz" -a "$ANSWER" != "0" ]; then
                tz=$(dialog --backtitle "OS/Z $CONFTXT - $TIMEZONE" --no-cancel --rangebox "$SETTZ (-1440..1440)" 8 60 -1440 1440 $tzz 2>&1 1>&3)
            else
                tz="$ANSWER"
            fi
        ;;

        7)
            screen=$(dialog --backtitle "OS/Z $CONFTXT - $USERI" --default-item "$screen" --no-cancel --no-items --menu "$SELRES" 13 30 7 \
            "640x480" \
            "768x576" \
            "800x600" \
            "1024x768" \
            "1280x720" \
            "1368x768" \
            "1280x800" \
            "1152x864" \
            "1152x900" \
            "1440x900" \
            "1280x960" \
            "1280x1024" \
            "1400x1050" \
            "1680x1050" \
            "1600x1200" \
            "1920x1200" \
            "1792x1344" \
            "1856x1392" \
            "1920x1440" \
            2>&1 1>&3)
        ;;

        8)
            display=$(dialog --backtitle "OS/Z $CONFTXT - $USERI" --default-item "$display" --no-cancel --menu "$SELVIS" 11 50 5 \
            "mm" "$OPTMM" \
            "mc" "$OPTMC" \
            "sm" "$OPTSM" \
            "sc" "$OPTSC" \
            2>&1 1>&3)
        ;;

        9)
            if [ "$dblbuf" == "true" ]; then opt1="on"; else opt1=""; fi
            if [ "$focusnew" == "true" ]; then opt2="on"; else opt2=""; fi
            if [ "$lefthanded" == "true" ]; then opt3="on"; else opt3=""; fi
            ANSWER=$(dialog --backtitle "OS/Z $CONFTXT - $USERI" --cancel-label "$CANCELTXT" --checklist "$ADDOPTS" 10 70 4 \
            dblbuf "$OPTDBL" "$opt1" \
            focusnew "$OPTFNEW" "$opt2" \
            lefthanded "$OPTLEFT" "$opt3" \
            2>&1 1>&3)
            if [ "$?" != "1" ]; then
                if [ -z "${ANSWER##*dblbuf*}" ]; then dblbuf="true"; else dblbuf="false"; fi
                if [ -z "${ANSWER##*focusnew*}" ]; then focusnew="true"; else focusnew="false"; fi
                if [ -z "${ANSWER##*lefthanded*}" ]; then lefthanded="true"; else lefthanded="false"; fi
            fi
        ;;

        10)
            lang=$(dialog --backtitle "OS/Z $CONFTXT - $USERI" --default-item "$lang" --no-cancel --menu "$SELLANG" 11 50 5 \
            "hu" "Magyar" \
            "ru" "русский" \
            "en" "English" \
            "de" "Deutch" \
            2>&1 1>&3)
        ;;

        11)
            kbds=`ls ../etc/sys/etc/kbd|sort`
            kbd=$(dialog --backtitle "OS/Z $CONFTXT - $USERI" --default-item "$kbd" --no-cancel --no-items --menu "$SELKBDTYPE" 11 40 5 $kbds 2>&1 1>&3)
        ;;

        12)
            maps=`ls ../etc/sys/etc/kbd/$kbd|sort`
            map=$(dialog --backtitle "OS/Z $CONFTXT - $USERI" --default-item "$map" --no-cancel --no-items --menu "$SELKBDLAYOUT" 11 40 5 $maps 2>&1 1>&3)
            maps=`ls ../etc/sys/etc/kbd/$kbd|grep -v "$map"|sort`
            if [ "$maps" != "" ]; then
                alt=$(dialog --backtitle "OS/Z $CONFTXT - $USERI" --default-item "$alt" --no-cancel --no-items --menu "$SELKBDALT" 11 40 5 "($NONETXT)" $maps 2>&1 1>&3)
                if [ "$alt" == "($NONETXT)" ]; then alt=""; fi
            fi
        ;;

        13)
            if [ -z "${dbgmsg##*pr*}" ]; then opt0="on"; else opt0=""; fi
            if [ -z "${dbgmsg##*lo*}" ]; then opt1="on"; else opt1=""; fi
            if [ -z "${dbgmsg##*de*}" ]; then opt2="on"; else opt2=""; fi
            if [ -z "${dbgmsg##*te*}" ]; then opt3="on"; else opt3=""; fi
            if [ -z "${dbgmsg##*me*}" ]; then opt4="on"; else opt4=""; fi
            if [ -z "${dbgmsg##*ta*}" ]; then opt5="on"; else opt5=""; fi
            if [ -z "${dbgmsg##*el*}" ]; then opt6="on"; else opt6=""; fi
            if [ -z "${dbgmsg##*ri*}" ]; then opt7="on"; else opt7=""; fi
            if [ -z "${dbgmsg##*re*}" ]; then opt8="on"; else opt8=""; fi
            if [ -z "${dbgmsg##*ir*}" ]; then opt9="on"; else opt9=""; fi
            if [ -z "${dbgmsg##*sc*}" ]; then optA="on"; else optA=""; fi
            if [ -z "${dbgmsg##*ms*}" ]; then optB="on"; else optB=""; fi
            if [ -z "${dbgmsg##*pm*}" ]; then optC="on"; else optC=""; fi
            if [ -z "${dbgmsg##*vm*}" ]; then optD="on"; else optD=""; fi
            if [ -z "${dbgmsg##*ma*}" ]; then optE="on"; else optE=""; fi
            if [ -z "${dbgmsg##*bl*}" ]; then optF="on"; else optF=""; fi
            if [ -z "${dbgmsg##*fi*}" ]; then optG="on"; else optG=""; fi
            if [ -z "${dbgmsg##*fs*}" ]; then optH="on"; else optH=""; fi
            if [ -z "${dbgmsg##*ca*}" ]; then optI="on"; else optI=""; fi
            if [ -z "${dbgmsg##*ui*}" ]; then optJ="on"; else optJ=""; fi
            if [ $debug -eq 1 ]; then
                ANSWER=$(dialog --backtitle "OS/Z $CONFTXT - $DBGMSG" --cancel-label "$CANCELTXT" --checklist "$SELDBG" $[$size-1] 70 $[$size-7] \
                prompt "$DBGPRT" "$opt0" \
                log "$DBGLOG" "$opt1" \
                dev "$DBGDEV" "$opt2" \
                test "$DBGTEST" "$opt3" \
                mem "$DBGMEM" "$opt4" \
                task "$DBGTASK" "$opt5" \
                elf "$DBGELF" "$opt6" \
                ri "$DBGRI" "$opt7" \
                re "$DBGRE" "$opt8" \
                irq "$DBGIRQ" "$opt9" \
                sched "$DBGSCHED" "$optA" \
                msg "$DBGMQ" "$optB" \
                pm "$DBGPM" "$optC" \
                vm "$DBGVM" "$optD" \
                ma "$DBGMA" "$optE" \
                blk "$DBGBLK" "$optF" \
                file "$DBGFILE" "$optG" \
                fs "$DBGFS" "$optH" \
                cache "$DBGCACHE" "$optI" \
                ui "$DBGUI" "$optJ" \
                2>&1 1>&3)
            else
                ANSWER=$(dialog --backtitle "OS/Z $CONFTXT - $DBGMSG" --cancel-label "$CANCELTXT" --checklist "$SELDBG" 23 70 17 \
                log "$DBGLOG" "$opt1" \
                dev "$DBGDEV" "$opt2" \
                2>&1 1>&3)
            fi
            if [ "$?" != "1" ]; then
                dbgmsg=${ANSWER// /,}
            fi
            ;;
    esac
done
exec 3>&-
echo -e "\e[0m"
clear
echo -ne "$saved"
