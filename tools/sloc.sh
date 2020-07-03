#!/bin/sh

#
# tools/sloc.sh
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
# @brief Segédprogram forráskód sorainak kiszámítására
#

function countlines
{
    cat $@ | sed 's/\\[nrtbxe]/\ /g' | sed -r ':a; s%(.*)/\*.*\*/%\1%; ta; /\/\*/ !b; N; ba' | grep -v '^[\ ]*$' | wc -l
}

if [ "${LANG:0:2}" = "hu" ]; then
    echo "Forráskódsorok Száma (SLoC)"
else
    echo "Number of Source Lines Of Code"
fi
echo "------------------------------"
for dirs in `find ../src -maxdepth 1 -type d|grep -ve '^../src$'|sort`; do
    name=${dirs##*/}
    echo -ne "$name:\t"
    [ ${#name} -lt 7 ] && echo -ne "\t"
    if [ "$name" == "core" ]; then
        crec=$(countlines `find ../src/core -maxdepth 1|grep -e '\.[ch]$'|grep -v dbg|grep -v disasm`)
        pltc=$(countlines `find ../src/core -mindepth 2|grep -e '\.[ch]$'|grep -v dbg|grep -v disasm`)
        plta=$(countlines `find ../src/core -mindepth 2|grep -e '\.S$'|grep -v dbg|grep -v disasm`)
        dbg=$[$(countlines `find ../src/core|grep -e '\.[chS]$'`)-$crec-$pltc-$plta]
        echo -e "$[$crec+$pltc+$plta] ($crec + arch $[$pltc+$plta], c $[$crec+$pltc] + asm $plta; +debugger $dbg)"
    else
        if [ "$name" == "libc" ]; then
            crec=$(countlines `find ../src/libc -maxdepth 1|grep -e '\.[ch]$'`)
            pltc=$(countlines `find ../src/libc -mindepth 2|grep -e '\.[ch]$'`)
            plta=$(countlines `find ../src/libc -mindepth 2|grep -e '\.S$'`)
            echo -e "$[$crec+$pltc+$plta] ($crec + arch $[$pltc+$plta], c $[$crec+$pltc] + asm $plta)"
        else
            countlines `find $dirs|grep -e '\.[chS]$'`
        fi
    fi
done
