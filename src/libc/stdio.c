/*
 * libc/stdio.c
 *
 * Copyright (C) 2016 bzt (bztsrc@gitlab)
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
 * @brief az stdio.h-ban definiált függvények megvalósítása
 */
#include <osZ.h>

#define DIRENTMAX 512
c_assert(DIRENTMAX>sizeof(dirent_t));

dirent_t *stdlib_direntbuf = NULL;
stat_t *stdlib_statbuf = NULL;
char *stdlib_nullstr = "(null)";
char *stdlib_ttyname;

/**
 * eszközhivatkozás hozzáadása (csak eszközmeghajtók és szolgáltatások hívhatják)
 */
public int mknod(const char *devname, dev_t minor, mode_t mode, blksize_t size, blkcnt_t cnt)
{
    msg_t *msg;

    if(!devname || !devname[0] || devname[0] == '/') {
        seterr(EINVAL);
        return -1L;
    }
    msg = mq_call5(SRV_FS, SYS_mknod|MSG_PTRDATA, devname, strlen(devname)+1, minor, (((uint64_t)mode)<<32)|size, cnt);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * beállítja a PATH-ot gyökérkönyvtárnak (az abszolút elérési út kiindulópontja).
 * ezt csak rendszergazda jogosultságokkal hívható.
 */
public fid_t chroot(const char *path)
{
    msg_t *msg;

    if(!path || !path[0]) {
        seterr(EINVAL);
        return -1UL;
    }
    msg = mq_call2(SRV_FS, SYS_chroot|MSG_PTRDATA, path, strlen(path)+1);
    return errno() ? -1UL : (fid_t)msg->data.scalar.arg0;
}

/**
 * a taszk munkakönyvtárának beállítása PATH-ra.
 */
public fid_t chdir(const char *path)
{
    msg_t *msg;

    if(!path || !path[0]) {
        seterr(EINVAL);
        return -1UL;
    }
    msg = mq_call2(SRV_FS, SYS_chdir|MSG_PTRDATA, path, strlen(path)+1);
    return errno() ? -1UL : (fid_t)msg->data.scalar.arg0;
}

/**
 * az aktuális munkakönyvtár visszaadása egy allokált bufferben
 */
public char *getcwd()
{
    msg_t *msg = mq_call0(SRV_FS, SYS_getcwd);
    if(errno())
        return NULL;
    return strdup(msg->data.buf.ptr);
}

/**
 * statikus felcsatolási pont hozzáadása
 */
public int mount(const char *dev, const char *mnt, const char *opts)
{
    char *path;
    int i,j,k;
    msg_t *msg;

    if(!dev || !dev[0] || !mnt || !mnt[0]) {
        seterr(EINVAL);
        return -1L;
    }
    i = strlen(dev); j = strlen(mnt); k = strlen(opts);
    path = (char*)malloc(i+j+k+3);
    if(!path)
        return -1L;
    memcpy(path, (void*)dev, i+1);
    memcpy(path+i+1, (void*)mnt, j+1);
    if(opts!=NULL && opts[0]!=0)
        memcpy(path+i+1+j+1, (void*)opts, k+1);
    msg = mq_call2(SRV_FS, SYS_mount|MSG_PTRDATA, path, i+j+k+3);
    i = errno();
    free(path); /* törli az errno-t, ha sikeres */
    if(i) {
        seterr(i);
        return -1L;
    }
    return (int)msg->data.scalar.arg0;
}

/**
 * statikus felcsatolási pont eltávolítása, a PATH lehet eszköznév vagy könyvtár is
 */
public int umount(const char *path)
{
    msg_t *msg;

    if(!path || !path[0]) {
        seterr(EINVAL);
        return -1L;
    }
    msg = mq_call2(SRV_FS, SYS_umount|MSG_PTRDATA, path, strlen(path)+1);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * STREAM duplikálása, egy új fájlleírót ad vissza ugyanarra a megnyitott fájlra
 */
public fid_t dup(fid_t stream)
{
    msg_t *msg = mq_call1(SRV_FS, SYS_dup, stream);
    return errno() ? 0 : (fid_t)msg->data.scalar.arg0;
}

/**
 * STREAM duplikálása STREAM2-re, a STREAM2-t lezárja és megnyitja ugyanarra a fájlra
 */
public fid_t dup2(fid_t stream, fid_t stream2)
{
    msg_t *msg = mq_call2(SRV_FS, SYS_dup2, stream, stream2);
    return errno() ? 0 : (fid_t)msg->data.scalar.arg0;
}

/**
 * B/K parancs küldése egy STREAM-en megnyitott eszköznek. BUFF lehet NULL.
 */
public int ioctl(fid_t stream, uint64_t code, void *buff, size_t size)
{
    /* az FS-nek küldjük, de a válasz a meghajtótól fog érkezni */
    msg_t *msg = mq_call4(SRV_FS, SYS_ioctl | (buff && size>0? MSG_PTRDATA : 0), buff, size, stream, code);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * létrehoz egy ideiglenes fájlt és megnyitja írásra / olvasásra
 */
public fid_t tmpfile()
{
    msg_t *msg = mq_call0(SRV_FS, SYS_tmpfile);
    return errno() ? 0 : (fid_t)msg->data.scalar.arg0;
}

/**
 * megnyit egy fájlt és visszad egy STREAM fájlleírót hozzá
 */
public fid_t fopen(const char *filename, mode_t oflag)
{
    msg_t *msg;

    if(!filename || !filename[0]) {
        seterr(EINVAL);
        return 0;
    }
    msg = mq_call4(SRV_FS, SYS_fopen|MSG_PTRDATA, filename, strlen(filename)+1, oflag, -1L);
    return errno() ? 0 : (fid_t)msg->data.scalar.arg0;
}

/**
 * lecserél egy meglévő STREAM fájlleírót egy újonnan megnyitott fájlra
 */
public fid_t freopen(const char *filename, mode_t oflag, fid_t stream)
{
    msg_t *msg;

    if(!filename || !filename[0]) {
        seterr(EINVAL);
        return 0;
    }
    msg = mq_call4(SRV_FS, SYS_fopen|MSG_PTRDATA, filename, strlen(filename)+1, oflag, stream);
    return errno() ? 0 : (fid_t)msg->data.scalar.arg0;
}

/**
 * lezárja a STREAM fájlleírón megnyitott fájlt.
 */
public int fclose(fid_t stream)
{
    msg_t *msg = mq_call1(SRV_FS, SYS_fclose, stream);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * minden fájlleírót lezár.
 */
public int fcloseall()
{
    msg_t *msg = mq_call0(SRV_FS, SYS_fcloseall);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * beállítja a pozíciót egy STREAM fájlleírón.
 */
public int fseek(fid_t stream, off_t off, int whence)
{
    msg_t *msg = mq_call3(SRV_FS, SYS_fseek, stream, off, whence);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * STREAM fájlleíró aktuális pozícióját adja vissza.
 */
public fpos_t ftell(fid_t stream)
{
    msg_t *msg = mq_call1(SRV_FS, SYS_ftell, stream);
    return errno() ? 0 : (fpos_t)msg->data.scalar.arg0;
}

/**
 * a STREAM fájlleíró vagy opendir könyvtárleírót az elejére pozícionálja.
 */
public void rewind(fid_t stream)
{
    mq_call1(SRV_FS, SYS_rewind, stream);
}

/**
 * az EOF és hibajelzők törlése STREAM fájlleírón.
 */
public void fclrerr(fid_t stream)
{
    mq_call1(SRV_FS, SYS_fclrerr, stream);
}

/**
 * visszaadja az EOF fájl vége jelzőt a STREAM fájlleíróhoz.
 */
public bool_t feof(fid_t stream)
{
    msg_t *msg = mq_call1(SRV_FS, SYS_feof, stream);
    return errno() ? true : (bool_t)msg->data.scalar.arg0;
}

/**
 * visszaadja a STREAM fájlleíróhoz tartozó hiba jelzőbiteket.
 */
public int ferror(fid_t stream)
{
    msg_t *msg = mq_call1(SRV_FS, SYS_ferror, stream);
    return errno() ? 1 : (int)msg->data.scalar.arg0;
}

/**
 * adatok olvasása STREAM fájlleíróból.
 */
public size_t fread(fid_t stream, void *ptr, size_t size)
{
    msg_t *msg;

    if(size >= __BUFFSIZE && (int64_t)ptr > 0) {
        seterr(ENOTSHM);
        return 0;
    }
    msg = mq_call3(SRV_FS, SYS_fread, ptr, size, stream);
    return errno() ? 0 : (size_t)msg->data.scalar.arg0;
}

/**
 * adatok kiírása STREAM fájlleíróba.
 */
public size_t fwrite(fid_t stream, void *ptr, size_t size)
{
    msg_t *msg;

    if(size >= __BUFFSIZE && (int64_t)ptr > 0) {
        seterr(ENOTSHM);
        return 0;
    }
/*dbg_printf("fwrite(%x,%d) pid %x%D\n",ptr,size,getpid(),ptr);*/
    msg = mq_call3(SRV_FS, SYS_fwrite|MSG_PTRDATA, ptr, size, stream);
    return errno() ? 0 : (size_t)msg->data.scalar.arg0;
}

/**
 * egy UTF-8 karakter olvasása STREAM fájlleíróból.
 */
public int fgetc(fid_t stream)
{
    char c[4];
    int i = 0;

    while(fread(stream, &c[i], 1) && c[i]&0x80 && i<4) i++;
    return *((int*)&c[0]);
}

/**
 * egy UTF-8 karakter olvasása stdin fájlleíróból.
 */
public int getchar()
{
    return fgetc(STDIN_FILENO);
}

/**
 * egy UTF-8 karakter kiírása STREAM fájlleíróba.
 */
public int fputc(fid_t stream, int c)
{
    char *b = (char*)&c;
    size_t s = 1;

    while(s<4 && b[s-1]&0x80) s++;
    return fwrite(stream, &c, s);
}

/**
 * egy UTF-8 karakter kiírása stdout fájlleíróba.
 */
public int putchar(int c)
{
    return fputc(STDOUT_FILENO, c);
}

/**
 * sztring kiírása STREAM fájlleíróba.
 */
public int fputs(fid_t stream, char *s)
{
    return fwrite(stream, s, strlen(s));
}

/**
 * sztring és egy sorvége kiírása stdout fájlleíróba.
 */
public int puts(char *s)
{
    int r = fwrite(STDOUT_FILENO, s, strlen(s));

    if(!errno()) r += fwrite(STDOUT_FILENO, "\n", 1);
    return r;
}

/**
 * írási buffer kiürítése STREAM fájlleírón, vagy mindegyiken ha STREAM -1.
 */
public int fflush(fid_t stream)
{
    msg_t *msg = mq_call1(SRV_FS, SYS_fflush, stream);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * PATH attribútumainak lekérése a csak olvasható bufferbe
 */
public stat_t *lstat(const char *path)
{
    if(!path || !path[0]) {
        seterr(EINVAL);
        return NULL;
    }
    if(!stdlib_statbuf) {
        stdlib_statbuf = malloc(sizeof(stat_t));
        if(!stdlib_statbuf)
            return NULL;
    }
    mq_call3(SRV_FS, SYS_lstat|MSG_PTRDATA, path, strlen(path)+1, stdlib_statbuf);
    return errno() ? NULL : (stat_t*)stdlib_statbuf;
}

/**
 * a STREAM fájlhoz tartozó, st_dev-ben visszaadott eszköz attribútumainak lekérése csak olvasható bufferbe
 */
public stat_t *dstat(fid_t stream)
{
    if(!stdlib_statbuf) {
        stdlib_statbuf = malloc(sizeof(stat_t));
        if(!stdlib_statbuf)
            return NULL;
    }
    mq_call3(SRV_FS, SYS_dstat, stream, 0, stdlib_statbuf);
    return errno() ? NULL : (stat_t*)stdlib_statbuf;
}

/**
 * STREAM fájlleírón megnyitott fájl, csővezeték vagy socket attribútumainak lekérése csak olvasható bufferbe
 */
public stat_t *fstat(fid_t stream)
{
    if(!stdlib_statbuf) {
        stdlib_statbuf = malloc(sizeof(stat_t));
        if(!stdlib_statbuf)
            return NULL;
    }
    mq_call3(SRV_FS, SYS_fstat, stream, 0, stdlib_statbuf);
    return errno() ? NULL : (stat_t*)stdlib_statbuf;
}

/**
 * kanonikus abszolút elérési út visszaadása PATH-hoz egy allokált bufferben
 */
char *realpath(const char *path)
{
    msg_t *msg;

    if(!path || !path[0]) {
        seterr(EINVAL);
        return NULL;
    }
    msg = mq_call2(SRV_FS, SYS_realpath|MSG_PTRDATA, path, strlen(path)+1);
    if(errno())
        return NULL;
    return strdup((char*)msg->data.buf.ptr);
}

/**
 * a PATH szimbólikus hivatkozás vagy unió céljának lekérése allokált bufferbe
 */
char *readlink(const char *path)
{
    char *buf;
    msg_t *msg;

    if(!path || !path[0]) {
        seterr(EINVAL);
        return NULL;
    }
    msg = mq_call2(SRV_FS, SYS_readlink|MSG_PTRDATA, path, strlen(path)+1);
    if(errno() || !msg->data.buf.size)
        return NULL;
    buf = malloc(msg->data.buf.size);
    if(!buf)
        return NULL;
    memcpy(buf, msg->data.buf.ptr, msg->data.buf.size);
    return buf;
}

/**
 * a PATH-on található könyvtár vagy unió megnyitása, STREAM könyvtárleírót ad vissza
 */
public fid_t opendir(const char *path)
{
    fid_t fd=-1;
    char *buf = NULL;
    msg_t *msg;
    size_t s;

    if(!path || !path[0]) {
        seterr(EINVAL);
        return -1L;
    }
    s = strlen(path)+1;
    /* ha egy jokert tartalmazó unióba futunk bele, akkor az FS lefordítja
     * könyvtárra és megkér, hogy újra hívjuk meg az opendir-t. Ez azért kell,
     * mert a dirent buffer a felhasználói memóriában kell legyen, méghozzá
     * sorba rendezett tárhely blokkokkal. Nem okozna problémát, ha a
     * könyvtárbejegyzések nem lóghatnának át blokkhatáron. */
again:
    msg = mq_call3(SRV_FS, SYS_opendir|MSG_PTRDATA, path, s, fd);
    if(msg->data.scalar.arg3 > 0) {
        buf = malloc(msg->data.scalar.arg3);
        if(!buf)
            return -1L;
        fd = msg->data.scalar.arg0;
        s = msg->data.scalar.arg3;
        path = buf;
        mq_call3(SRV_FS, SYS_fread, buf, s, fd);
        if(!errno())
            goto again;
    }
    if(buf)
        free(buf);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * könyvtárbejegyzés olvasása STREAM könyvtárleíróból. Csak olvasható dirent bufferrel tér vissza,
 * vagy NULL-al, ha nincs több bejegyzés.
 */
public dirent_t *readdir(fid_t stream)
{
    /* az FS/Z esetében úgy terveztem, hogy ne lógjanak át a könyvtárbejegyzések blokkhatáron,
     * de más fájlrendszereknél (pl ext2) ez nem biztos. Ezért felhasználói memóriában kell egy
     * buffer, amibe fread-el előreolvasunk, hogy az egész biztos a memóriában legyen */
    if(!stdlib_direntbuf) {
        stdlib_direntbuf = malloc(DIRENTMAX);
        if(!stdlib_direntbuf)
            return NULL;
    }
    if(!fread(stream, stdlib_direntbuf, DIRENTMAX))
        return NULL;
    mq_call4(SRV_FS, SYS_readdir|MSG_PTRDATA, stdlib_direntbuf, DIRENTMAX, stream, stdlib_direntbuf);
    if(errno())
        return NULL;
    return (dirent_t*)stdlib_direntbuf;
}

/**
 * STREAM könyvtárleíró lezárása. 0-át ad vissza, ha sikeres, -1 -et ha nem.
 */
public int closedir(fid_t stream)
{
    msg_t *msg = mq_call1(SRV_FS, SYS_fclose, stream);
    return errno() ? -1L : (int)msg->data.scalar.arg0;
}

/**
 * segédfüggvény hexába alakításhoz
 */
private uint64_t sprintf_hex(uint64_t arg, uint8_t cnt, char tmpstr[])
{
    int i = 16;
    tmpstr[i] = 0;
    do {
        char n=arg & 0xf;
        tmpstr[--i]=n<10?'0'+n:'A'+n-10;
        arg>>=4;
    } while(arg!=0&&i>0);
    if(cnt>0&&cnt<=8) {
        while(i>16-cnt*2) {
            tmpstr[--i]='0';
        }
    }
    return i;
}

/**
 * egyszerű sprintf implementáció, formázott szöveg írása DST-be, maximum SIZE hosszan, argumentumlistával
 */
public int vsnprintf(char *dst, size_t size, const char *format, va_list args)
{
    uint64_t arg,i,j,s;
    char *fmt,*p,*orig=dst,*end=dst+size;
    uint8_t reent=0;
    uint8_t cnt=0;
    char tmpstr[33];

    fmt = format==NULL ? stdlib_nullstr : (char*)format;

    arg = 0;
    while(fmt[0]!=0) {
        /* paraméter hivatkozás */
        if(fmt[0]=='%' && !reent) {
            fmt++; cnt=0;
            if(fmt[0]=='%') {
                goto put;
            }
            while(fmt[0]>='0'&&fmt[0]<='9') {
                cnt *= 10;
                cnt += fmt[0]-'0';
                fmt++;
            }
            if(fmt[0]!='s')
                arg = va_arg(args, int64_t);
            if(fmt[0]=='c') {
                if(orig!=NULL && end>dst)
                    *dst = (char)arg;
                dst++;
                fmt++;
                continue;
            }
            reent++;
            if(orig!=NULL && end>dst)
                *dst=0;
            if(fmt[0]=='l') fmt++;
            if(fmt[0]=='a') {
                if(cnt==0 || cnt>8) cnt=8;
                if(orig!=NULL && end>dst+cnt)
                    memcpy((void*)dst,(void*)&arg,cnt);
                dst+=cnt;
            } else
            if(fmt[0]=='d') {
                i=18; s=((int64_t)arg)<0;
                if(s) arg*=-1;
                if(arg>99999999999999999L)
                    arg=99999999999999999L;
                tmpstr[i]=0;
                do {
                    tmpstr[--i]='0'+(arg%10);
                    arg/=10;
                } while(arg!=0&&i>0);
                if(s)
                    tmpstr[--i]='-';
                if(cnt>0&&cnt<18) {
                    while(i>18U-cnt) {
                        tmpstr[--i]=' ';
                    }
                }
                if(orig!=NULL && end>dst+(18-i))
                    memcpy((void*)dst,(void*)&tmpstr[i],18-i);
                dst+=18-i;
            } else
            if(fmt[0]=='x') {
                i=sprintf_hex(arg, cnt, tmpstr);
                if(orig!=NULL && end>dst+(16-i))
                    memcpy((void*)dst,(void*)&tmpstr[i],16-i);
                dst+=16-i;
            } else
            if(fmt[0]=='s') {
                p = va_arg(args, char*);
                if(p == NULL)
                    p = stdlib_nullstr;
                i=strlen(p);
                if(cnt>0 && i>cnt) i=cnt;
                if(orig!=NULL)
                    memcpy((void*)dst, (void*)p, end>dst+i ? i : (uint64_t)(end-dst));
                dst+=i;
                if(i<cnt) {
                    cnt-=i;
                    if(orig!=NULL) {
                        while(cnt-->0)
                            if(end>dst)
                                *dst++=' ';
                    } else
                        dst+=cnt;
                }
            } else
            if(fmt[0]=='U') {
                uuid_t *u=(uuid_t*)arg;
                i=sprintf_hex(u->Data1, 8, tmpstr); tmpstr[16]='-';
                if(orig!=NULL && end>dst+(17-i))
                    memcpy((void*)dst,(void*)&tmpstr[i],17-i);
                dst+=17-i;
                i=sprintf_hex(u->Data2, 4, tmpstr); tmpstr[16]='-';
                if(orig!=NULL && end>dst+(17-i))
                    memcpy((void*)dst,(void*)&tmpstr[i],17-i);
                dst+=17-i;
                i=sprintf_hex(u->Data3, 4, tmpstr); tmpstr[16]='-';
                if(orig!=NULL && end>dst+(17-i))
                    memcpy((void*)dst,(void*)&tmpstr[i],17-i);
                dst+=17-i;
                for(j=0;j<8;j++) {
                    i=sprintf_hex(u->Data3, 2, tmpstr);
                    if(orig!=NULL && end>dst+(16-i))
                        memcpy((void*)dst,(void*)&tmpstr[i],16-i);
                    dst+=16-i;
                }
            }
            reent--;
        } else {
put:        if(orig!=NULL && end>dst)
                *dst = *fmt;
            dst++;
        }
        fmt++;
    }
    if(orig!=NULL) {
        if(end>dst)
            *dst=0;
        else
            *end=0;
    }
    return dst-orig;
}

/**
 * formázott szöveg kiírása DST sztringbe, argumentumlistával
 */
public int vsprintf(char *dst, const char *fmt, va_list args)
{
    return vsnprintf(dst, INT_MAX, fmt, args);
}

/**
 * formázott szöveg kiírása DST sztringbe
 */
public int sprintf(char *dst, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vsnprintf(dst, INT_MAX, fmt, args);
}

/**
 * formázott szöveg kiírása DST sztringbe, legfeljebb SIZE bájt hosszan
 */
public int snprintf(char *dst, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vsnprintf(dst, size, fmt, args);
}

/**
 * formázott szöveg kiírása STREAM fájlleíróba, argumentumlistával
 */
int vfprintf(fid_t stream, const char *fmt, va_list args)
{
    size_t size;
    char *buf;
    va_list args2;
    va_copy(args2, args);

    size = vsnprintf(NULL, INT_MAX, fmt, args2)+1;
    buf = (char*)malloc(size);

    if(!buf)
        return 0;
    vsnprintf(buf, size, fmt, args);
    fwrite(stream, buf, size);
    free(buf);
    return size-1;
}

/**
 * formázott szöveg kiírása STREAM fájlleíróba
 */
public int fprintf(fid_t stream, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vfprintf(stream, fmt, args);
}

/**
 * formázott szöveg kiírása stdout-ra, argumentumlistával
 */
public int vprintf(const char *fmt, va_list args)
{
    return vfprintf(STDOUT_FILENO, fmt, args);
}

/**
 * formázott szöveg kiírása stdout-ra
 */
public int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vfprintf(STDOUT_FILENO, fmt, args);
}

/**
 * stderr-re kiírja szövegesen az errno hibakód jelentését.
 */
public void perror(char *cmd, char *fmt, ...)
{
    size_t size;
    char *err, *buf;
    int i, j;
    va_list args;
    va_start(args, fmt);

    size = vsnprintf(NULL, INT_MAX, fmt, args)+1;
    err = strerror(errno());
    i = strlen(err); j = strlen(cmd);
    buf = (char*)malloc(j+size+i+5);

    if(!buf) {
#if DEBUG
        dbg_printf("%s: %s: %s\n", cmd, err, fmt);
#endif
        fwrite(STDERR_FILENO, cmd, j);
        fwrite(STDERR_FILENO, ": ", 2);
        fwrite(STDERR_FILENO, err, i);
        fwrite(STDERR_FILENO, "\n", 1);
        return;
    }
    memcpy(buf, cmd, j);
    buf[j] = ':'; buf[j+1] = ' ';
    memcpy(buf+j+2, err, i);
    i += j+2;
    if(fmt && fmt[0]) {
        buf[i] = ':'; buf[i+1] = ' ';
        vsnprintf(buf+i+2, size, fmt, args);
        i += 2+size;
    }
    buf[i] = '\n';
    buf[i+1] = 0;
#if DEBUG
    dbg_printf("%s", buf);
#endif
    fwrite(STDERR_FILENO, buf, i);
    free(buf);
}

/**
 * nem több, mint BUFLEN bájtot lement a STREAM-hez tartozó terminál eszköz elérési útjából, 0-át ad vissza, ha sikerült
 */
int ttyname_r(fid_t stream, char *buf, size_t buflen)
{
    if(fstat(stream) && buf && buflen>0 && stdlib_statbuf && S_ISTTY(stdlib_statbuf->st_mode)) {
        snprintf(buf, buflen, "/dev/tty%d", (int)stdlib_statbuf->st_ino);
        return 0;
    }
    return errno() ? errno() : EBADF;
}

/**
 * visszaadja a STREAM fájlleíróhoz tartozó terminál eszköz nevét, vagy NULL-t
 * a visszaadtott érték csak a kovetkező h0vásig érvényes
 */
char *ttyname(fid_t stream)
{
    if(!stdlib_ttyname) {
        stdlib_ttyname = malloc(16);
        if(!stdlib_ttyname)
            return NULL;
    }
    return !ttyname_r(stream, stdlib_ttyname, 16) ? stdlib_ttyname : NULL;
}

/**
 * 1-et ad vissza ha a STREAM fájlleíró egy terminálon van megnyitva, egyébként 0-át
 */
bool_t isatty(fid_t stream)
{
    return (fstat(stream) && stdlib_statbuf && S_ISTTY(stdlib_statbuf->st_mode)) ? true : false;
}

