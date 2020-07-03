/*
 * libc/env.c
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
 * @subsystem libc
 * @brief induló környezet értelmező függvények
 */

#include <osZ.h>

/* induló konfiguráció (valós idejű linker biztosítja, akárcsak az __environ-t) */
public char *_environment=NULL;

/* mivel a kulcsok többször is előfordulhatnak, mindig az utolsót kell figyelembe venni */

/**
 * szám típusú változó visszaadása
 */
public uint64_t env_num(char *key, uint64_t def, uint64_t min, uint64_t max)
{
    char *env = _environment, *env_end = _environment+__PAGESIZE;
    int i, s = false;
    uint64_t ret = def;

    if(_environment == NULL || key == NULL || !key[0]) return def;
    i = strlen(key);
    while(env < env_end && *env) {
        if((env[0]=='/'&&env[1]=='/') || env[0]=='#') { while(env<env_end && *(env+1)!=0 && *env!='\n') env++; }
        if(env[0]=='/'&&env[1]=='*') { env+=2; while(env<env_end && env[1]!=0 && env[-1]!='*' && env[0]!='/') env++; }
        if(env+i<env_end && !memcmp(env,key,i) && env[i]=='=') {
            env+=i+1;
            if(*env=='-') { env++; s=true; } else s=false;
            env=(char*)stdlib_dec((uchar *)env, &ret, min, max);
            if(s) ret=-ret;
        }
        env++;
    }
    return ret;
}

/**
 * logikai érték változó visszaadása
 */
public bool_t env_bool(char *key, bool_t def)
{
    char *env = _environment, *env_end = _environment+__PAGESIZE;
    int i;
    bool_t ret = def;

    if(_environment == NULL || key == NULL || !key[0]) return def;
    i = strlen(key);
    while(env < env_end && *env!=0) {
        if((env[0]=='/'&&env[1]=='/') || env[0]=='#') { while(env<env_end && *(env+1)!=0 && *env!='\n') env++; }
        if(env[0]=='/'&&env[1]=='*') { env+=2; while(env<env_end && env[1]!=0 && env[-1]!='*' && env[0]!='/') env++; }
        if(env+i<env_end && !memcmp(env,key,i) && env[i]=='=') {
            env+=i+1;
            /* 1, true, enabled, on, igaz, be */
            if((*env=='1'||*env=='t'||*env=='T'||*env=='e'||*env=='E'||*env=='i'||*env=='I'||*env=='b'||*env=='B'||
                (*env=='o'&&*(env+1)=='n')||(*env=='O'&&*(env+1)=='N')))
                ret=true;
            /* 0, false, disabled, off, hamis, ki */
            if((*env=='0'||*env=='f'||*env=='F'||*env=='d'||*env=='D'||*env=='h'||*env=='H'||*env=='k'||*env=='K'||
                (*env=='o'&&*(env+1)=='f')||(*env=='O'&&*(env+1)=='F')))
                ret=false;
            while(env<env_end && *env!='\n' && *(env+1)!=0) env++;
        }
        env++;
    }
    return ret;
}

/**
 * sztring visszaadása egy allokált bufferben, amit a hívónak kell felszabadítania
 */
public char *env_str(char *key, char *def)
{
    char *env = _environment, *env_end = _environment+__PAGESIZE, *ret = NULL, *s;
    int i, j;

    if(_environment != NULL && key != NULL && key[0]) {
        i = strlen(key); j = strlen(def);
        while(env < env_end && *env!=0) {
            if((env[0]=='/'&&env[1]=='/') || env[0]=='#') { while(env<env_end && *(env+1)!=0 && *env!='\n') env++; }
            if(env[0]=='/'&&env[1]=='*') { env+=2; while(env<env_end && env[1]!=0 && env[-1]!='*' && env[0]!='/') env++; }
            if(env+i<env_end && !memcmp(env,key,i) && env[i]=='=') {
                env+=i+1;
                for(s=env;s<env_end && *s!='\n' && *s!=0;s++);
                ret=env; j=s-env;
                if(*s==0) break; else env=s;
            }
            env++;
        }
    }
    if(ret == NULL) return strdup(def);
    s = (char*)malloc(j+1);
    if(s != NULL) memcpy(s, ret, j);
    return s;
}
