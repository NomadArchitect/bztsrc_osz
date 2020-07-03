#!/usr/bin/php
<?php
/*
 * tools/mkref.php
 *
 * Copyright (c) 2019 bzt (bztsrc@gitlab)
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
 * @subsystem eszközök
 * @brief függvény-keresztreferencia generáló
 */

/* ez csak magyarul van, mivel a forrásban a kommentek úgyis magyarul vannak */

$outfile = @$_SERVER['argv'][1];
$infiles = @array_slice($_SERVER['argv'], 2);

if(empty($outfile) || empty($infiles))
    die($_SERVER['argv'][0]." <kimenet.md> <bemeneti fájl1> [bementi fájl2 [...]]\n");

sort($infiles);
$files = []; $funcs = [];

foreach($infiles as $fn) {
    $data = file_get_contents($fn);
    $ifn = explode("\n", substr($data, 6))[0];
    if(substr(realpath($fn), -strlen($ifn)) != $ifn) die("Hibás fájlnév a kommentben: ".$ifn." != ".$fn."\n");
    if(!preg_match("/@subsystem\ (.*)/", $data, $m)) die("Nincs @subsystem: ".$fn."\n");
    $subsystem = $m[1];
    if(!preg_match("/@brief\ (.*)/", $data, $m)) die("Nincs @brief: ".$fn."\n");
    $brief = $m[1];
    $files[$subsystem][] = ['f'=>$ifn, 'b'=>$brief];
    for($ln = 1,$i=0;$i<strlen($data);$i++) {
        if($data[$i]=="\n") $ln++;
        if(substr($data, $i, 4)=="/**\n") {
            $ol=$ln;$k=$m=$n=0;
            for($j=$i+3;substr($data, $i, 3)!="*/\n";$i++) {
                if($data[$i]=="\n") $ln++;
                if(substr($data, $i, 4)=="\n *\n"||substr($data, $i, 5)=="\n * \n") { $k=$i; $m=$i+7; }
            }
            if(!$k) $k=$i;
            $i+=3;
            if(!$m) {
                for($m=$i;$data[$i]!="\n"&&$data[$i]!=";"&&$data[$i]!="{"&&substr($data,$i-1,3)!=") (";$i++);
                $n=$i--;
            } else {
                $n=$i-3;
            }
            if($data[$m]!="/" && substr($data, $m, 7)!="private" && substr($data, $m, 6)!="extern") {
                if(substr($data, $m, 6)=="public") $m+=7;
                $p=str_replace("__attribute__((malloc)) ","",str_replace(" __attribute__((unused))","",
                    trim(substr($data, $m, $n-$m))));
                if(substr($p,0,5)=="func ") $p="void ".substr($p,5)."()";
                preg_match("/([^\ \*\(]+)\(/", $p, $m);
                if(empty($m[1])) die($ifn.": ".$p."\n");
                $funcs[$subsystem][] = ['n'=>$m[1], 'p'=>$p, 'f'=>$ifn,'l'=>$ol,
                    'd'=>"  ".trim(str_replace("\n * ","\n  ",substr($data, $j, $k-$j)))];
            }
        }
    }

}

$f=fopen($outfile, "w");
fwrite($f, "OS/Z Függvényreferenciák\n========================\n\nPrototípusok\n------------\n\n");
$lc = "";
foreach($funcs as $c=>$fs) {
    if($lc!=$c) { $lc=$c; fwrite($f,"### ".ucfirst($c)."\n\n"); }
    sort($fs);
    foreach($fs as $fu)
        fwrite($f,"[".str_replace("]","",$fu['p'])."](https://gitlab.com/bztsrc/osz/blob/master/src/".$fu['f']."#L".$fu['l'].")\n".
            $fu['d']."\n\n");
}
fwrite($f, "Fájlok\n------\n\n| Fájl | Alrendszer | Leírás |\n| ---- | ---------- | ------ |\n");
foreach($files as $c=>$fs) {
    foreach($fs as $fn)
        fwrite($f,"| [".$fn['f']."](https://gitlab.com/bztsrc/osz/blob/master/src/".$fn['f'].") | ".$c." | ".$fn['b']." |\n");
}
fwrite($f,"\n");
fclose($f);
die($outfile." OK\n");
