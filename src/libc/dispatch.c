/*
 * libc/dispatch.c
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
 * @brief üzenet sor diszpécser
 */

#include <osZ.h>
#include <osZ/elf.h>

public pid_t mq_caller = 0;         /* a hívó pidje */
public uint8_t mq_ack = 0;

/**
 * assembly segédfüggvény, az stdlib.S-ben
 */
extern retdq_t mq_dispatchcall(uint64_t arg0,uint64_t arg1,uint64_t arg2,uint64_t arg3,uint64_t arg4,uint64_t arg5, virt_t func);

/**
 * üzenet sor diszpécser. Kiveszi az üzenetet, meghívja a feldolgozó függvényt és visszaküldi a választ.
 * vagy ENOEXEC hibát ad, vagy nem tér vissza
 */
public uint64_t mq_dispatch()
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)TEXT_ADDRESS;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)ehdr + ehdr->e_phoff + 2*ehdr->e_phentsize);
    Elf64_Dyn *d;
    Elf64_Sym *sym = NULL, *sym_end = NULL;
    Elf64_Sym *symfunc;
    uint64_t syment = 0, e;
    uint16_t func, maxfunc;
    retdq_t ret;
    msg_t *msg;

    /*** ELF értelmezése, hogy megtaláljuk a hívható függvényeket ***/
    d = (Elf64_Dyn *)((uint32_t)phdr->p_offset+(uint64_t)ehdr);
    while(d->d_tag != DT_NULL && (sym==NULL || sym_end==NULL || syment==0)) {
        switch(d->d_tag) {
            case DT_STRTAB: sym_end = (Elf64_Sym *)((uint8_t *)ehdr + (uint32_t)(d->d_un.d_ptr&0xFFFFFFFFUL)); break;
            case DT_SYMTAB: sym = (Elf64_Sym *)((uint8_t *)ehdr + (uint32_t)(d->d_un.d_ptr&0xFFFFFFFFUL)); break;
            case DT_SYMENT: syment = (uint16_t)d->d_un.d_val; break;
        }
        d++;
    }
    if(syment == 0) syment = sizeof(Elf64_Sym);
    maxfunc = ((virt_t)sym_end - (virt_t)sym)/syment;
    if(sym == NULL || sym_end == NULL || sym_end <= sym || syment < 8 || maxfunc >= SHRT_MAX/2) {
        seterr(ENOEXEC);
        return EX_SOFTWARE;
    }

    /*** fő ciklus ***/
    while(1) {
        msg = mq_recv();
        if(MSG_ISRESP(msg->evt)) continue;
        func = EVT_FUNC(msg->evt);
        symfunc = (Elf64_Sym*)((virt_t)sym + (virt_t)(func * syment));
        seterr(SUCCESS);
        ret.ret0 = ret.ret1 = 0;
        if(func > 0 && func < maxfunc &&                       /* határon belül van? */
           ELF64_ST_BIND(symfunc->st_info) == STB_GLOBAL &&    /* globális funkciót jelöl? */
           ELF64_ST_TYPE(symfunc->st_info) == STT_FUNC
        ) {
            mq_caller = EVT_SENDER(msg->evt);
            mq_ack = 1;
            ret = mq_dispatchcall(
                  msg->data.scalar.arg0, msg->data.scalar.arg1, msg->data.scalar.arg2,
                  msg->data.scalar.arg3, msg->data.scalar.arg4, msg->data.scalar.arg5,
                  (virt_t)symfunc->st_value + (virt_t)TEXT_ADDRESS
                );
            mq_caller = 0;
        } else
            seterr(EPERM);
        /* válasz küldése a hívónak ha hiba volt vagy mq_ack-ban kérve lett */
        e = errno();
        if(e || mq_ack)
            mq_send(ret.ret0, ret.ret1, e, 0, 0, 0, (msg->evt&0x3FFF) | (mq_ack==2&&!e?MSG_PTRDATA:0) | EVT_ack, msg->serial);
    }
    /* elvileg sose jutunk ide */
    return EX_OK;
}

