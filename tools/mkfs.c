/*
 * tools/mkfs.c
 *
 * Copyright (c) 2017 bzt (bztsrc@gitlab)
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
 * @brief Segédeszköz FS/Z lemezképek létrehozására
 *
 *  Fordítás: gcc mkfs.c -o mkfs
 *
 * Paraméterek nélkül hívva kiírja a lehetőségeket.
 *
 * Ez egy minimális implementáció, a FS/Z specifikációhoz képest rengeteg megszorítást
 * tartalmaz. Például csak 24 bejegyzést kezel könyvtáranként (csak inode-ba ágyazott bejegyzések), és nem
 * kezel titkosított fájlokat meg lemezképet (csak komplett lemezképeket titkosít / fejt vissza egészben).
 */

#define HAS_AES 1

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_MISC 1            /* DT_* enum miatt */
#include <dirent.h>
#define __USE_XOPEN_EXTENDED 1  /* readlink és strdup prototípus miatt */
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#if HAS_ZLIB
#include <zlib.h>
#endif
#define public
#include "../include/osZ/fsZ.h"
#include "../src/libc/crc32.h"

/*-------------ADATOK-----------*/
char *diskname;     /* egész lemez */
char *stage1;       /* boot szektor kód */
char *espfile;      /* EFI System Partíció képfájl */
char *sys1file;     /* system1 partíció képfájl */
char *sys2file;     /* system2 partíció képfájl */
char *usr1file;     /* usr1 partíció képfájl */
char *usr2file;     /* usr2 partíció képfájl */
char *varfile;      /* var partíció képfájl */
char *homefile;     /* home partíció képfájl */

int hu = 0;         /* magyar nyelvű kimenet */

unsigned int secsize=FSZ_SECSIZE;
FILE *f;
unsigned char *fs;
int size;
long int li=0,read_size=0;
long int ts;   /* aktális időbélyeg microszekundumban */
char *emptysec;/* üres szektor */
int initrd;

/*-------------KÓD-----------*/

/* Mellékes funkciók */
/**
 * beolvas egy teljes fájlt a memóriába, visszaadja a címét a méretét pedig a read_size változóban.
 * ha -DHAS_ZLIB opcióval fordítjuk az mkfs-t, akkor gzippelt fájlokat is be tud olvasni
 */
unsigned char* readfileall(char *file)
{
    unsigned char *data=NULL;
    unsigned char hdr[2];
    FILE *f;
#if HAS_ZLIB
    gzFile g;
#endif
    read_size=0;
    f=fopen(file,"r");
    if(f){
#if HAS_ZLIB
        fread(&hdr,2,1,f);
        if(hdr[0]==0x1f && hdr[1]==0x8b) {
            fseek(f,-4L,SEEK_END);
            fread(&read_size,4,1,f);
        } else {
            fseek(f,0L,SEEK_END);
            read_size=ftell(f);
        }
        fclose(f);
        g=gzopen(file,"r");
#else
        fseek(f,0L,SEEK_END);
        read_size=ftell(f);
        fseek(f,0L,SEEK_SET);
#endif
        data=(unsigned char*)malloc(read_size+secsize+1);
        if(data==NULL) { fprintf(stderr,hu?"mkfs: nem tudok allokálni %ld bájtnyi memóriát\n":
            "mkfs: Unable to allocate %ld memory\n",read_size+secsize+1); exit(1); }
        memset(data,0,read_size+secsize+1);
#if HAS_ZLIB
        gzread(g,data,read_size);
        gzclose(g);
#else
        fread(data,read_size,1,f);
        fclose(f);
#endif
    }
    return data;
}
/**
 * lekér vagy beállít egy integer értéket egy karaktertömbben
 */
int getint(unsigned char *ptr) { return (unsigned char)ptr[0]+(unsigned char)ptr[1]*256+(unsigned char)ptr[2]*256*256+ptr[3]*256*256*256; }
void setint(int val, unsigned char *ptr) { memcpy(ptr,&val,4); }
void setinte(int val, unsigned char *ptr) { char *v=(char*)&val; memcpy(ptr,&val,4); ptr[4]=v[3]; ptr[5]=v[2]; ptr[6]=v[1]; ptr[7]=v[0]; }
/**
 * fájlnevek rendezésére segédfüggvény
 */
static int direntcmp(const void *a, const void *b)
{
    return strcasecmp((char *)((FSZ_DirEnt *)a)->name,(char *)((FSZ_DirEnt *)b)->name);
}
void printf_uuid(FSZ_Access *mem, int accessflags)
{
    printf("%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x",mem->Data1,mem->Data2,mem->Data3,
        mem->Data4[0],mem->Data4[1],mem->Data4[2],mem->Data4[3],mem->Data4[4],mem->Data4[5],mem->Data4[6]);
    if(!accessflags)
        printf("%02x",mem->access);
    else
        printf(":%c%c%c%c%c%c%c",
            mem->access&FSZ_READ?'r':'-',
            mem->access&FSZ_WRITE?'w':'-',
            mem->access&FSZ_EXEC?'x':'-',
            mem->access&FSZ_APPEND?'a':'-',
            mem->access&FSZ_DELETE?'d':'-',
            mem->access&FSZ_SUID?'s':'-',
            mem->access&FSZ_SGID?'i':'-');
}

/**
 * SHA-256
 */
typedef struct {
   uint8_t d[64];
   uint32_t l;
   uint32_t b[2];
   uint32_t s[8];
} SHA256_CTX;
#define SHA_ADD(a,b,c) if(a>0xffffffff-(c))b++;a+=c;
#define SHA_ROTL(a,b) (((a)<<(b))|((a)>>(32-(b))))
#define SHA_ROTR(a,b) (((a)>>(b))|((a)<<(32-(b))))
#define SHA_CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define SHA_MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define SHA_EP0(x) (SHA_ROTR(x,2)^SHA_ROTR(x,13)^SHA_ROTR(x,22))
#define SHA_EP1(x) (SHA_ROTR(x,6)^SHA_ROTR(x,11)^SHA_ROTR(x,25))
#define SHA_SIG0(x) (SHA_ROTR(x,7)^SHA_ROTR(x,18)^((x)>>3))
#define SHA_SIG1(x) (SHA_ROTR(x,17)^SHA_ROTR(x,19)^((x)>>10))
static uint32_t sha256_k[64]={
   0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
   0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
   0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
   0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
   0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
   0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
   0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
   0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
void sha256_t(SHA256_CTX *ctx)
{
   uint32_t a,b,c,d,e,f,g,h,i,j,t1,t2,m[64];
   for(i=0,j=0;i<16;i++,j+=4) m[i]=(ctx->d[j]<<24)|(ctx->d[j+1]<<16)|(ctx->d[j+2]<<8)|(ctx->d[j+3]);
   for(;i<64;i++) m[i]=SHA_SIG1(m[i-2])+m[i-7]+SHA_SIG0(m[i-15])+m[i-16];
   a=ctx->s[0];b=ctx->s[1];c=ctx->s[2];d=ctx->s[3];
   e=ctx->s[4];f=ctx->s[5];g=ctx->s[6];h=ctx->s[7];
   for(i=0;i<64;i++) {
       t1=h+SHA_EP1(e)+SHA_CH(e,f,g)+sha256_k[i]+m[i];
       t2=SHA_EP0(a)+SHA_MAJ(a,b,c);h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
   ctx->s[0]+=a;ctx->s[1]+=b;ctx->s[2]+=c;ctx->s[3]+=d;
   ctx->s[4]+=e;ctx->s[5]+=f;ctx->s[6]+=g;ctx->s[7]+=h;
}
void SHA256_Init(SHA256_CTX *ctx)
{
    ctx->l=0;ctx->b[0]=ctx->b[1]=0;
    ctx->s[0]=0x6a09e667;ctx->s[1]=0xbb67ae85;ctx->s[2]=0x3c6ef372;ctx->s[3]=0xa54ff53a;
    ctx->s[4]=0x510e527f;ctx->s[5]=0x9b05688c;ctx->s[6]=0x1f83d9ab;ctx->s[7]=0x5be0cd19;
}
void SHA256_Update(SHA256_CTX *ctx, const void *data, int len)
{
    uint8_t *d=(uint8_t *)data;
    for(;len--;d++) {
        ctx->d[ctx->l++]=*d;
        if(ctx->l==64) {sha256_t(ctx);SHA_ADD(ctx->b[0],ctx->b[1],512);ctx->l=0;}
    }
}
void SHA256_Final(unsigned char *h, SHA256_CTX *ctx)
{
    uint32_t i=ctx->l;
    ctx->d[i++]=0x80;
    if(ctx->l<56) {while(i<56) ctx->d[i++]=0x00;}
    else {while(i<64) ctx->d[i++]=0x00;sha256_t(ctx);memset(ctx->d,0,56);}
    SHA_ADD(ctx->b[0],ctx->b[1],ctx->l*8);
    ctx->d[63]=ctx->b[0];ctx->d[62]=ctx->b[0]>>8;ctx->d[61]=ctx->b[0]>>16;ctx->d[60]=ctx->b[0]>>24;
    ctx->d[59]=ctx->b[1];ctx->d[58]=ctx->b[1]>>8;ctx->d[57]=ctx->b[1]>>16;ctx->d[56]=ctx->b[1]>>24;
    sha256_t(ctx);
    for(i=0;i<4;i++) {
        h[i]   =(ctx->s[0]>>(24-i*8)); h[i+4] =(ctx->s[1]>>(24-i*8));
        h[i+8] =(ctx->s[2]>>(24-i*8)); h[i+12]=(ctx->s[3]>>(24-i*8));
        h[i+16]=(ctx->s[4]>>(24-i*8)); h[i+20]=(ctx->s[5]>>(24-i*8));
        h[i+24]=(ctx->s[6]>>(24-i*8)); h[i+28]=(ctx->s[7]>>(24-i*8));
    }
}

/**
 * AES-256-CBC
 */
#if HAS_AES
#define GETU32(pt) (((uint32_t)(pt)[0]<<24)^((uint32_t)(pt)[1]<<16)^((uint32_t)(pt)[2]<<8)^((uint32_t)(pt)[3]))
#define PUTU32(ct,st) {(ct)[0]=(uint8_t)((st)>>24);(ct)[1]=(uint8_t)((st)>>16);(ct)[2]=(uint8_t)((st)>>8);(ct)[3]=(uint8_t)(st);}
static const uint32_t Te0[256]={
    0xc66363a5U,0xf87c7c84U,0xee777799U,0xf67b7b8dU,0xfff2f20dU,0xd66b6bbdU,0xde6f6fb1U,0x91c5c554U,
    0x60303050U,0x02010103U,0xce6767a9U,0x562b2b7dU,0xe7fefe19U,0xb5d7d762U,0x4dababe6U,0xec76769aU,
    0x8fcaca45U,0x1f82829dU,0x89c9c940U,0xfa7d7d87U,0xeffafa15U,0xb25959ebU,0x8e4747c9U,0xfbf0f00bU,
    0x41adadecU,0xb3d4d467U,0x5fa2a2fdU,0x45afafeaU,0x239c9cbfU,0x53a4a4f7U,0xe4727296U,0x9bc0c05bU,
    0x75b7b7c2U,0xe1fdfd1cU,0x3d9393aeU,0x4c26266aU,0x6c36365aU,0x7e3f3f41U,0xf5f7f702U,0x83cccc4fU,
    0x6834345cU,0x51a5a5f4U,0xd1e5e534U,0xf9f1f108U,0xe2717193U,0xabd8d873U,0x62313153U,0x2a15153fU,
    0x0804040cU,0x95c7c752U,0x46232365U,0x9dc3c35eU,0x30181828U,0x379696a1U,0x0a05050fU,0x2f9a9ab5U,
    0x0e070709U,0x24121236U,0x1b80809bU,0xdfe2e23dU,0xcdebeb26U,0x4e272769U,0x7fb2b2cdU,0xea75759fU,
    0x1209091bU,0x1d83839eU,0x582c2c74U,0x341a1a2eU,0x361b1b2dU,0xdc6e6eb2U,0xb45a5aeeU,0x5ba0a0fbU,
    0xa45252f6U,0x763b3b4dU,0xb7d6d661U,0x7db3b3ceU,0x5229297bU,0xdde3e33eU,0x5e2f2f71U,0x13848497U,
    0xa65353f5U,0xb9d1d168U,0x00000000U,0xc1eded2cU,0x40202060U,0xe3fcfc1fU,0x79b1b1c8U,0xb65b5bedU,
    0xd46a6abeU,0x8dcbcb46U,0x67bebed9U,0x7239394bU,0x944a4adeU,0x984c4cd4U,0xb05858e8U,0x85cfcf4aU,
    0xbbd0d06bU,0xc5efef2aU,0x4faaaae5U,0xedfbfb16U,0x864343c5U,0x9a4d4dd7U,0x66333355U,0x11858594U,
    0x8a4545cfU,0xe9f9f910U,0x04020206U,0xfe7f7f81U,0xa05050f0U,0x783c3c44U,0x259f9fbaU,0x4ba8a8e3U,
    0xa25151f3U,0x5da3a3feU,0x804040c0U,0x058f8f8aU,0x3f9292adU,0x219d9dbcU,0x70383848U,0xf1f5f504U,
    0x63bcbcdfU,0x77b6b6c1U,0xafdada75U,0x42212163U,0x20101030U,0xe5ffff1aU,0xfdf3f30eU,0xbfd2d26dU,
    0x81cdcd4cU,0x180c0c14U,0x26131335U,0xc3ecec2fU,0xbe5f5fe1U,0x359797a2U,0x884444ccU,0x2e171739U,
    0x93c4c457U,0x55a7a7f2U,0xfc7e7e82U,0x7a3d3d47U,0xc86464acU,0xba5d5de7U,0x3219192bU,0xe6737395U,
    0xc06060a0U,0x19818198U,0x9e4f4fd1U,0xa3dcdc7fU,0x44222266U,0x542a2a7eU,0x3b9090abU,0x0b888883U,
    0x8c4646caU,0xc7eeee29U,0x6bb8b8d3U,0x2814143cU,0xa7dede79U,0xbc5e5ee2U,0x160b0b1dU,0xaddbdb76U,
    0xdbe0e03bU,0x64323256U,0x743a3a4eU,0x140a0a1eU,0x924949dbU,0x0c06060aU,0x4824246cU,0xb85c5ce4U,
    0x9fc2c25dU,0xbdd3d36eU,0x43acacefU,0xc46262a6U,0x399191a8U,0x319595a4U,0xd3e4e437U,0xf279798bU,
    0xd5e7e732U,0x8bc8c843U,0x6e373759U,0xda6d6db7U,0x018d8d8cU,0xb1d5d564U,0x9c4e4ed2U,0x49a9a9e0U,
    0xd86c6cb4U,0xac5656faU,0xf3f4f407U,0xcfeaea25U,0xca6565afU,0xf47a7a8eU,0x47aeaee9U,0x10080818U,
    0x6fbabad5U,0xf0787888U,0x4a25256fU,0x5c2e2e72U,0x381c1c24U,0x57a6a6f1U,0x73b4b4c7U,0x97c6c651U,
    0xcbe8e823U,0xa1dddd7cU,0xe874749cU,0x3e1f1f21U,0x964b4bddU,0x61bdbddcU,0x0d8b8b86U,0x0f8a8a85U,
    0xe0707090U,0x7c3e3e42U,0x71b5b5c4U,0xcc6666aaU,0x904848d8U,0x06030305U,0xf7f6f601U,0x1c0e0e12U,
    0xc26161a3U,0x6a35355fU,0xae5757f9U,0x69b9b9d0U,0x17868691U,0x99c1c158U,0x3a1d1d27U,0x279e9eb9U,
    0xd9e1e138U,0xebf8f813U,0x2b9898b3U,0x22111133U,0xd26969bbU,0xa9d9d970U,0x078e8e89U,0x339494a7U,
    0x2d9b9bb6U,0x3c1e1e22U,0x15878792U,0xc9e9e920U,0x87cece49U,0xaa5555ffU,0x50282878U,0xa5dfdf7aU,
    0x038c8c8fU,0x59a1a1f8U,0x09898980U,0x1a0d0d17U,0x65bfbfdaU,0xd7e6e631U,0x844242c6U,0xd06868b8U,
    0x824141c3U,0x299999b0U,0x5a2d2d77U,0x1e0f0f11U,0x7bb0b0cbU,0xa85454fcU,0x6dbbbbd6U,0x2c16163aU,
};
static const uint32_t Te1[256]={
    0xa5c66363U,0x84f87c7cU,0x99ee7777U,0x8df67b7bU,0x0dfff2f2U,0xbdd66b6bU,0xb1de6f6fU,0x5491c5c5U,
    0x50603030U,0x03020101U,0xa9ce6767U,0x7d562b2bU,0x19e7fefeU,0x62b5d7d7U,0xe64dababU,0x9aec7676U,
    0x458fcacaU,0x9d1f8282U,0x4089c9c9U,0x87fa7d7dU,0x15effafaU,0xebb25959U,0xc98e4747U,0x0bfbf0f0U,
    0xec41adadU,0x67b3d4d4U,0xfd5fa2a2U,0xea45afafU,0xbf239c9cU,0xf753a4a4U,0x96e47272U,0x5b9bc0c0U,
    0xc275b7b7U,0x1ce1fdfdU,0xae3d9393U,0x6a4c2626U,0x5a6c3636U,0x417e3f3fU,0x02f5f7f7U,0x4f83ccccU,
    0x5c683434U,0xf451a5a5U,0x34d1e5e5U,0x08f9f1f1U,0x93e27171U,0x73abd8d8U,0x53623131U,0x3f2a1515U,
    0x0c080404U,0x5295c7c7U,0x65462323U,0x5e9dc3c3U,0x28301818U,0xa1379696U,0x0f0a0505U,0xb52f9a9aU,
    0x090e0707U,0x36241212U,0x9b1b8080U,0x3ddfe2e2U,0x26cdebebU,0x694e2727U,0xcd7fb2b2U,0x9fea7575U,
    0x1b120909U,0x9e1d8383U,0x74582c2cU,0x2e341a1aU,0x2d361b1bU,0xb2dc6e6eU,0xeeb45a5aU,0xfb5ba0a0U,
    0xf6a45252U,0x4d763b3bU,0x61b7d6d6U,0xce7db3b3U,0x7b522929U,0x3edde3e3U,0x715e2f2fU,0x97138484U,
    0xf5a65353U,0x68b9d1d1U,0x00000000U,0x2cc1ededU,0x60402020U,0x1fe3fcfcU,0xc879b1b1U,0xedb65b5bU,
    0xbed46a6aU,0x468dcbcbU,0xd967bebeU,0x4b723939U,0xde944a4aU,0xd4984c4cU,0xe8b05858U,0x4a85cfcfU,
    0x6bbbd0d0U,0x2ac5efefU,0xe54faaaaU,0x16edfbfbU,0xc5864343U,0xd79a4d4dU,0x55663333U,0x94118585U,
    0xcf8a4545U,0x10e9f9f9U,0x06040202U,0x81fe7f7fU,0xf0a05050U,0x44783c3cU,0xba259f9fU,0xe34ba8a8U,
    0xf3a25151U,0xfe5da3a3U,0xc0804040U,0x8a058f8fU,0xad3f9292U,0xbc219d9dU,0x48703838U,0x04f1f5f5U,
    0xdf63bcbcU,0xc177b6b6U,0x75afdadaU,0x63422121U,0x30201010U,0x1ae5ffffU,0x0efdf3f3U,0x6dbfd2d2U,
    0x4c81cdcdU,0x14180c0cU,0x35261313U,0x2fc3ececU,0xe1be5f5fU,0xa2359797U,0xcc884444U,0x392e1717U,
    0x5793c4c4U,0xf255a7a7U,0x82fc7e7eU,0x477a3d3dU,0xacc86464U,0xe7ba5d5dU,0x2b321919U,0x95e67373U,
    0xa0c06060U,0x98198181U,0xd19e4f4fU,0x7fa3dcdcU,0x66442222U,0x7e542a2aU,0xab3b9090U,0x830b8888U,
    0xca8c4646U,0x29c7eeeeU,0xd36bb8b8U,0x3c281414U,0x79a7dedeU,0xe2bc5e5eU,0x1d160b0bU,0x76addbdbU,
    0x3bdbe0e0U,0x56643232U,0x4e743a3aU,0x1e140a0aU,0xdb924949U,0x0a0c0606U,0x6c482424U,0xe4b85c5cU,
    0x5d9fc2c2U,0x6ebdd3d3U,0xef43acacU,0xa6c46262U,0xa8399191U,0xa4319595U,0x37d3e4e4U,0x8bf27979U,
    0x32d5e7e7U,0x438bc8c8U,0x596e3737U,0xb7da6d6dU,0x8c018d8dU,0x64b1d5d5U,0xd29c4e4eU,0xe049a9a9U,
    0xb4d86c6cU,0xfaac5656U,0x07f3f4f4U,0x25cfeaeaU,0xafca6565U,0x8ef47a7aU,0xe947aeaeU,0x18100808U,
    0xd56fbabaU,0x88f07878U,0x6f4a2525U,0x725c2e2eU,0x24381c1cU,0xf157a6a6U,0xc773b4b4U,0x5197c6c6U,
    0x23cbe8e8U,0x7ca1ddddU,0x9ce87474U,0x213e1f1fU,0xdd964b4bU,0xdc61bdbdU,0x860d8b8bU,0x850f8a8aU,
    0x90e07070U,0x427c3e3eU,0xc471b5b5U,0xaacc6666U,0xd8904848U,0x05060303U,0x01f7f6f6U,0x121c0e0eU,
    0xa3c26161U,0x5f6a3535U,0xf9ae5757U,0xd069b9b9U,0x91178686U,0x5899c1c1U,0x273a1d1dU,0xb9279e9eU,
    0x38d9e1e1U,0x13ebf8f8U,0xb32b9898U,0x33221111U,0xbbd26969U,0x70a9d9d9U,0x89078e8eU,0xa7339494U,
    0xb62d9b9bU,0x223c1e1eU,0x92158787U,0x20c9e9e9U,0x4987ceceU,0xffaa5555U,0x78502828U,0x7aa5dfdfU,
    0x8f038c8cU,0xf859a1a1U,0x80098989U,0x171a0d0dU,0xda65bfbfU,0x31d7e6e6U,0xc6844242U,0xb8d06868U,
    0xc3824141U,0xb0299999U,0x775a2d2dU,0x111e0f0fU,0xcb7bb0b0U,0xfca85454U,0xd66dbbbbU,0x3a2c1616U,
};
static const uint32_t Te2[256]={
    0x63a5c663U,0x7c84f87cU,0x7799ee77U,0x7b8df67bU,0xf20dfff2U,0x6bbdd66bU,0x6fb1de6fU,0xc55491c5U,
    0x30506030U,0x01030201U,0x67a9ce67U,0x2b7d562bU,0xfe19e7feU,0xd762b5d7U,0xabe64dabU,0x769aec76U,
    0xca458fcaU,0x829d1f82U,0xc94089c9U,0x7d87fa7dU,0xfa15effaU,0x59ebb259U,0x47c98e47U,0xf00bfbf0U,
    0xadec41adU,0xd467b3d4U,0xa2fd5fa2U,0xafea45afU,0x9cbf239cU,0xa4f753a4U,0x7296e472U,0xc05b9bc0U,
    0xb7c275b7U,0xfd1ce1fdU,0x93ae3d93U,0x266a4c26U,0x365a6c36U,0x3f417e3fU,0xf702f5f7U,0xcc4f83ccU,
    0x345c6834U,0xa5f451a5U,0xe534d1e5U,0xf108f9f1U,0x7193e271U,0xd873abd8U,0x31536231U,0x153f2a15U,
    0x040c0804U,0xc75295c7U,0x23654623U,0xc35e9dc3U,0x18283018U,0x96a13796U,0x050f0a05U,0x9ab52f9aU,
    0x07090e07U,0x12362412U,0x809b1b80U,0xe23ddfe2U,0xeb26cdebU,0x27694e27U,0xb2cd7fb2U,0x759fea75U,
    0x091b1209U,0x839e1d83U,0x2c74582cU,0x1a2e341aU,0x1b2d361bU,0x6eb2dc6eU,0x5aeeb45aU,0xa0fb5ba0U,
    0x52f6a452U,0x3b4d763bU,0xd661b7d6U,0xb3ce7db3U,0x297b5229U,0xe33edde3U,0x2f715e2fU,0x84971384U,
    0x53f5a653U,0xd168b9d1U,0x00000000U,0xed2cc1edU,0x20604020U,0xfc1fe3fcU,0xb1c879b1U,0x5bedb65bU,
    0x6abed46aU,0xcb468dcbU,0xbed967beU,0x394b7239U,0x4ade944aU,0x4cd4984cU,0x58e8b058U,0xcf4a85cfU,
    0xd06bbbd0U,0xef2ac5efU,0xaae54faaU,0xfb16edfbU,0x43c58643U,0x4dd79a4dU,0x33556633U,0x85941185U,
    0x45cf8a45U,0xf910e9f9U,0x02060402U,0x7f81fe7fU,0x50f0a050U,0x3c44783cU,0x9fba259fU,0xa8e34ba8U,
    0x51f3a251U,0xa3fe5da3U,0x40c08040U,0x8f8a058fU,0x92ad3f92U,0x9dbc219dU,0x38487038U,0xf504f1f5U,
    0xbcdf63bcU,0xb6c177b6U,0xda75afdaU,0x21634221U,0x10302010U,0xff1ae5ffU,0xf30efdf3U,0xd26dbfd2U,
    0xcd4c81cdU,0x0c14180cU,0x13352613U,0xec2fc3ecU,0x5fe1be5fU,0x97a23597U,0x44cc8844U,0x17392e17U,
    0xc45793c4U,0xa7f255a7U,0x7e82fc7eU,0x3d477a3dU,0x64acc864U,0x5de7ba5dU,0x192b3219U,0x7395e673U,
    0x60a0c060U,0x81981981U,0x4fd19e4fU,0xdc7fa3dcU,0x22664422U,0x2a7e542aU,0x90ab3b90U,0x88830b88U,
    0x46ca8c46U,0xee29c7eeU,0xb8d36bb8U,0x143c2814U,0xde79a7deU,0x5ee2bc5eU,0x0b1d160bU,0xdb76addbU,
    0xe03bdbe0U,0x32566432U,0x3a4e743aU,0x0a1e140aU,0x49db9249U,0x060a0c06U,0x246c4824U,0x5ce4b85cU,
    0xc25d9fc2U,0xd36ebdd3U,0xacef43acU,0x62a6c462U,0x91a83991U,0x95a43195U,0xe437d3e4U,0x798bf279U,
    0xe732d5e7U,0xc8438bc8U,0x37596e37U,0x6db7da6dU,0x8d8c018dU,0xd564b1d5U,0x4ed29c4eU,0xa9e049a9U,
    0x6cb4d86cU,0x56faac56U,0xf407f3f4U,0xea25cfeaU,0x65afca65U,0x7a8ef47aU,0xaee947aeU,0x08181008U,
    0xbad56fbaU,0x7888f078U,0x256f4a25U,0x2e725c2eU,0x1c24381cU,0xa6f157a6U,0xb4c773b4U,0xc65197c6U,
    0xe823cbe8U,0xdd7ca1ddU,0x749ce874U,0x1f213e1fU,0x4bdd964bU,0xbddc61bdU,0x8b860d8bU,0x8a850f8aU,
    0x7090e070U,0x3e427c3eU,0xb5c471b5U,0x66aacc66U,0x48d89048U,0x03050603U,0xf601f7f6U,0x0e121c0eU,
    0x61a3c261U,0x355f6a35U,0x57f9ae57U,0xb9d069b9U,0x86911786U,0xc15899c1U,0x1d273a1dU,0x9eb9279eU,
    0xe138d9e1U,0xf813ebf8U,0x98b32b98U,0x11332211U,0x69bbd269U,0xd970a9d9U,0x8e89078eU,0x94a73394U,
    0x9bb62d9bU,0x1e223c1eU,0x87921587U,0xe920c9e9U,0xce4987ceU,0x55ffaa55U,0x28785028U,0xdf7aa5dfU,
    0x8c8f038cU,0xa1f859a1U,0x89800989U,0x0d171a0dU,0xbfda65bfU,0xe631d7e6U,0x42c68442U,0x68b8d068U,
    0x41c38241U,0x99b02999U,0x2d775a2dU,0x0f111e0fU,0xb0cb7bb0U,0x54fca854U,0xbbd66dbbU,0x163a2c16U,
};
static const uint32_t Te3[256]={
    0x6363a5c6U,0x7c7c84f8U,0x777799eeU,0x7b7b8df6U,0xf2f20dffU,0x6b6bbdd6U,0x6f6fb1deU,0xc5c55491U,
    0x30305060U,0x01010302U,0x6767a9ceU,0x2b2b7d56U,0xfefe19e7U,0xd7d762b5U,0xababe64dU,0x76769aecU,
    0xcaca458fU,0x82829d1fU,0xc9c94089U,0x7d7d87faU,0xfafa15efU,0x5959ebb2U,0x4747c98eU,0xf0f00bfbU,
    0xadadec41U,0xd4d467b3U,0xa2a2fd5fU,0xafafea45U,0x9c9cbf23U,0xa4a4f753U,0x727296e4U,0xc0c05b9bU,
    0xb7b7c275U,0xfdfd1ce1U,0x9393ae3dU,0x26266a4cU,0x36365a6cU,0x3f3f417eU,0xf7f702f5U,0xcccc4f83U,
    0x34345c68U,0xa5a5f451U,0xe5e534d1U,0xf1f108f9U,0x717193e2U,0xd8d873abU,0x31315362U,0x15153f2aU,
    0x04040c08U,0xc7c75295U,0x23236546U,0xc3c35e9dU,0x18182830U,0x9696a137U,0x05050f0aU,0x9a9ab52fU,
    0x0707090eU,0x12123624U,0x80809b1bU,0xe2e23ddfU,0xebeb26cdU,0x2727694eU,0xb2b2cd7fU,0x75759feaU,
    0x09091b12U,0x83839e1dU,0x2c2c7458U,0x1a1a2e34U,0x1b1b2d36U,0x6e6eb2dcU,0x5a5aeeb4U,0xa0a0fb5bU,
    0x5252f6a4U,0x3b3b4d76U,0xd6d661b7U,0xb3b3ce7dU,0x29297b52U,0xe3e33eddU,0x2f2f715eU,0x84849713U,
    0x5353f5a6U,0xd1d168b9U,0x00000000U,0xeded2cc1U,0x20206040U,0xfcfc1fe3U,0xb1b1c879U,0x5b5bedb6U,
    0x6a6abed4U,0xcbcb468dU,0xbebed967U,0x39394b72U,0x4a4ade94U,0x4c4cd498U,0x5858e8b0U,0xcfcf4a85U,
    0xd0d06bbbU,0xefef2ac5U,0xaaaae54fU,0xfbfb16edU,0x4343c586U,0x4d4dd79aU,0x33335566U,0x85859411U,
    0x4545cf8aU,0xf9f910e9U,0x02020604U,0x7f7f81feU,0x5050f0a0U,0x3c3c4478U,0x9f9fba25U,0xa8a8e34bU,
    0x5151f3a2U,0xa3a3fe5dU,0x4040c080U,0x8f8f8a05U,0x9292ad3fU,0x9d9dbc21U,0x38384870U,0xf5f504f1U,
    0xbcbcdf63U,0xb6b6c177U,0xdada75afU,0x21216342U,0x10103020U,0xffff1ae5U,0xf3f30efdU,0xd2d26dbfU,
    0xcdcd4c81U,0x0c0c1418U,0x13133526U,0xecec2fc3U,0x5f5fe1beU,0x9797a235U,0x4444cc88U,0x1717392eU,
    0xc4c45793U,0xa7a7f255U,0x7e7e82fcU,0x3d3d477aU,0x6464acc8U,0x5d5de7baU,0x19192b32U,0x737395e6U,
    0x6060a0c0U,0x81819819U,0x4f4fd19eU,0xdcdc7fa3U,0x22226644U,0x2a2a7e54U,0x9090ab3bU,0x8888830bU,
    0x4646ca8cU,0xeeee29c7U,0xb8b8d36bU,0x14143c28U,0xdede79a7U,0x5e5ee2bcU,0x0b0b1d16U,0xdbdb76adU,
    0xe0e03bdbU,0x32325664U,0x3a3a4e74U,0x0a0a1e14U,0x4949db92U,0x06060a0cU,0x24246c48U,0x5c5ce4b8U,
    0xc2c25d9fU,0xd3d36ebdU,0xacacef43U,0x6262a6c4U,0x9191a839U,0x9595a431U,0xe4e437d3U,0x79798bf2U,
    0xe7e732d5U,0xc8c8438bU,0x3737596eU,0x6d6db7daU,0x8d8d8c01U,0xd5d564b1U,0x4e4ed29cU,0xa9a9e049U,
    0x6c6cb4d8U,0x5656faacU,0xf4f407f3U,0xeaea25cfU,0x6565afcaU,0x7a7a8ef4U,0xaeaee947U,0x08081810U,
    0xbabad56fU,0x787888f0U,0x25256f4aU,0x2e2e725cU,0x1c1c2438U,0xa6a6f157U,0xb4b4c773U,0xc6c65197U,
    0xe8e823cbU,0xdddd7ca1U,0x74749ce8U,0x1f1f213eU,0x4b4bdd96U,0xbdbddc61U,0x8b8b860dU,0x8a8a850fU,
    0x707090e0U,0x3e3e427cU,0xb5b5c471U,0x6666aaccU,0x4848d890U,0x03030506U,0xf6f601f7U,0x0e0e121cU,
    0x6161a3c2U,0x35355f6aU,0x5757f9aeU,0xb9b9d069U,0x86869117U,0xc1c15899U,0x1d1d273aU,0x9e9eb927U,
    0xe1e138d9U,0xf8f813ebU,0x9898b32bU,0x11113322U,0x6969bbd2U,0xd9d970a9U,0x8e8e8907U,0x9494a733U,
    0x9b9bb62dU,0x1e1e223cU,0x87879215U,0xe9e920c9U,0xcece4987U,0x5555ffaaU,0x28287850U,0xdfdf7aa5U,
    0x8c8c8f03U,0xa1a1f859U,0x89898009U,0x0d0d171aU,0xbfbfda65U,0xe6e631d7U,0x4242c684U,0x6868b8d0U,
    0x4141c382U,0x9999b029U,0x2d2d775aU,0x0f0f111eU,0xb0b0cb7bU,0x5454fca8U,0xbbbbd66dU,0x16163a2cU,
};

static const uint32_t Td0[256]={
    0x51f4a750U,0x7e416553U,0x1a17a4c3U,0x3a275e96U,0x3bab6bcbU,0x1f9d45f1U,0xacfa58abU,0x4be30393U,
    0x2030fa55U,0xad766df6U,0x88cc7691U,0xf5024c25U,0x4fe5d7fcU,0xc52acbd7U,0x26354480U,0xb562a38fU,
    0xdeb15a49U,0x25ba1b67U,0x45ea0e98U,0x5dfec0e1U,0xc32f7502U,0x814cf012U,0x8d4697a3U,0x6bd3f9c6U,
    0x038f5fe7U,0x15929c95U,0xbf6d7aebU,0x955259daU,0xd4be832dU,0x587421d3U,0x49e06929U,0x8ec9c844U,
    0x75c2896aU,0xf48e7978U,0x99583e6bU,0x27b971ddU,0xbee14fb6U,0xf088ad17U,0xc920ac66U,0x7dce3ab4U,
    0x63df4a18U,0xe51a3182U,0x97513360U,0x62537f45U,0xb16477e0U,0xbb6bae84U,0xfe81a01cU,0xf9082b94U,
    0x70486858U,0x8f45fd19U,0x94de6c87U,0x527bf8b7U,0xab73d323U,0x724b02e2U,0xe31f8f57U,0x6655ab2aU,
    0xb2eb2807U,0x2fb5c203U,0x86c57b9aU,0xd33708a5U,0x302887f2U,0x23bfa5b2U,0x02036abaU,0xed16825cU,
    0x8acf1c2bU,0xa779b492U,0xf307f2f0U,0x4e69e2a1U,0x65daf4cdU,0x0605bed5U,0xd134621fU,0xc4a6fe8aU,
    0x342e539dU,0xa2f355a0U,0x058ae132U,0xa4f6eb75U,0x0b83ec39U,0x4060efaaU,0x5e719f06U,0xbd6e1051U,
    0x3e218af9U,0x96dd063dU,0xdd3e05aeU,0x4de6bd46U,0x91548db5U,0x71c45d05U,0x0406d46fU,0x605015ffU,
    0x1998fb24U,0xd6bde997U,0x894043ccU,0x67d99e77U,0xb0e842bdU,0x07898b88U,0xe7195b38U,0x79c8eedbU,
    0xa17c0a47U,0x7c420fe9U,0xf8841ec9U,0x00000000U,0x09808683U,0x322bed48U,0x1e1170acU,0x6c5a724eU,
    0xfd0efffbU,0x0f853856U,0x3daed51eU,0x362d3927U,0x0a0fd964U,0x685ca621U,0x9b5b54d1U,0x24362e3aU,
    0x0c0a67b1U,0x9357e70fU,0xb4ee96d2U,0x1b9b919eU,0x80c0c54fU,0x61dc20a2U,0x5a774b69U,0x1c121a16U,
    0xe293ba0aU,0xc0a02ae5U,0x3c22e043U,0x121b171dU,0x0e090d0bU,0xf28bc7adU,0x2db6a8b9U,0x141ea9c8U,
    0x57f11985U,0xaf75074cU,0xee99ddbbU,0xa37f60fdU,0xf701269fU,0x5c72f5bcU,0x44663bc5U,0x5bfb7e34U,
    0x8b432976U,0xcb23c6dcU,0xb6edfc68U,0xb8e4f163U,0xd731dccaU,0x42638510U,0x13972240U,0x84c61120U,
    0x854a247dU,0xd2bb3df8U,0xaef93211U,0xc729a16dU,0x1d9e2f4bU,0xdcb230f3U,0x0d8652ecU,0x77c1e3d0U,
    0x2bb3166cU,0xa970b999U,0x119448faU,0x47e96422U,0xa8fc8cc4U,0xa0f03f1aU,0x567d2cd8U,0x223390efU,
    0x87494ec7U,0xd938d1c1U,0x8ccaa2feU,0x98d40b36U,0xa6f581cfU,0xa57ade28U,0xdab78e26U,0x3fadbfa4U,
    0x2c3a9de4U,0x5078920dU,0x6a5fcc9bU,0x547e4662U,0xf68d13c2U,0x90d8b8e8U,0x2e39f75eU,0x82c3aff5U,
    0x9f5d80beU,0x69d0937cU,0x6fd52da9U,0xcf2512b3U,0xc8ac993bU,0x10187da7U,0xe89c636eU,0xdb3bbb7bU,
    0xcd267809U,0x6e5918f4U,0xec9ab701U,0x834f9aa8U,0xe6956e65U,0xaaffe67eU,0x21bccf08U,0xef15e8e6U,
    0xbae79bd9U,0x4a6f36ceU,0xea9f09d4U,0x29b07cd6U,0x31a4b2afU,0x2a3f2331U,0xc6a59430U,0x35a266c0U,
    0x744ebc37U,0xfc82caa6U,0xe090d0b0U,0x33a7d815U,0xf104984aU,0x41ecdaf7U,0x7fcd500eU,0x1791f62fU,
    0x764dd68dU,0x43efb04dU,0xccaa4d54U,0xe49604dfU,0x9ed1b5e3U,0x4c6a881bU,0xc12c1fb8U,0x4665517fU,
    0x9d5eea04U,0x018c355dU,0xfa877473U,0xfb0b412eU,0xb3671d5aU,0x92dbd252U,0xe9105633U,0x6dd64713U,
    0x9ad7618cU,0x37a10c7aU,0x59f8148eU,0xeb133c89U,0xcea927eeU,0xb761c935U,0xe11ce5edU,0x7a47b13cU,
    0x9cd2df59U,0x55f2733fU,0x1814ce79U,0x73c737bfU,0x53f7cdeaU,0x5ffdaa5bU,0xdf3d6f14U,0x7844db86U,
    0xcaaff381U,0xb968c43eU,0x3824342cU,0xc2a3405fU,0x161dc372U,0xbce2250cU,0x283c498bU,0xff0d9541U,
    0x39a80171U,0x080cb3deU,0xd8b4e49cU,0x6456c190U,0x7bcb8461U,0xd532b670U,0x486c5c74U,0xd0b85742U,
};
static const uint32_t Td1[256]={
    0x5051f4a7U,0x537e4165U,0xc31a17a4U,0x963a275eU,0xcb3bab6bU,0xf11f9d45U,0xabacfa58U,0x934be303U,
    0x552030faU,0xf6ad766dU,0x9188cc76U,0x25f5024cU,0xfc4fe5d7U,0xd7c52acbU,0x80263544U,0x8fb562a3U,
    0x49deb15aU,0x6725ba1bU,0x9845ea0eU,0xe15dfec0U,0x02c32f75U,0x12814cf0U,0xa38d4697U,0xc66bd3f9U,
    0xe7038f5fU,0x9515929cU,0xebbf6d7aU,0xda955259U,0x2dd4be83U,0xd3587421U,0x2949e069U,0x448ec9c8U,
    0x6a75c289U,0x78f48e79U,0x6b99583eU,0xdd27b971U,0xb6bee14fU,0x17f088adU,0x66c920acU,0xb47dce3aU,
    0x1863df4aU,0x82e51a31U,0x60975133U,0x4562537fU,0xe0b16477U,0x84bb6baeU,0x1cfe81a0U,0x94f9082bU,
    0x58704868U,0x198f45fdU,0x8794de6cU,0xb7527bf8U,0x23ab73d3U,0xe2724b02U,0x57e31f8fU,0x2a6655abU,
    0x07b2eb28U,0x032fb5c2U,0x9a86c57bU,0xa5d33708U,0xf2302887U,0xb223bfa5U,0xba02036aU,0x5ced1682U,
    0x2b8acf1cU,0x92a779b4U,0xf0f307f2U,0xa14e69e2U,0xcd65daf4U,0xd50605beU,0x1fd13462U,0x8ac4a6feU,
    0x9d342e53U,0xa0a2f355U,0x32058ae1U,0x75a4f6ebU,0x390b83ecU,0xaa4060efU,0x065e719fU,0x51bd6e10U,
    0xf93e218aU,0x3d96dd06U,0xaedd3e05U,0x464de6bdU,0xb591548dU,0x0571c45dU,0x6f0406d4U,0xff605015U,
    0x241998fbU,0x97d6bde9U,0xcc894043U,0x7767d99eU,0xbdb0e842U,0x8807898bU,0x38e7195bU,0xdb79c8eeU,
    0x47a17c0aU,0xe97c420fU,0xc9f8841eU,0x00000000U,0x83098086U,0x48322bedU,0xac1e1170U,0x4e6c5a72U,
    0xfbfd0effU,0x560f8538U,0x1e3daed5U,0x27362d39U,0x640a0fd9U,0x21685ca6U,0xd19b5b54U,0x3a24362eU,
    0xb10c0a67U,0x0f9357e7U,0xd2b4ee96U,0x9e1b9b91U,0x4f80c0c5U,0xa261dc20U,0x695a774bU,0x161c121aU,
    0x0ae293baU,0xe5c0a02aU,0x433c22e0U,0x1d121b17U,0x0b0e090dU,0xadf28bc7U,0xb92db6a8U,0xc8141ea9U,
    0x8557f119U,0x4caf7507U,0xbbee99ddU,0xfda37f60U,0x9ff70126U,0xbc5c72f5U,0xc544663bU,0x345bfb7eU,
    0x768b4329U,0xdccb23c6U,0x68b6edfcU,0x63b8e4f1U,0xcad731dcU,0x10426385U,0x40139722U,0x2084c611U,
    0x7d854a24U,0xf8d2bb3dU,0x11aef932U,0x6dc729a1U,0x4b1d9e2fU,0xf3dcb230U,0xec0d8652U,0xd077c1e3U,
    0x6c2bb316U,0x99a970b9U,0xfa119448U,0x2247e964U,0xc4a8fc8cU,0x1aa0f03fU,0xd8567d2cU,0xef223390U,
    0xc787494eU,0xc1d938d1U,0xfe8ccaa2U,0x3698d40bU,0xcfa6f581U,0x28a57adeU,0x26dab78eU,0xa43fadbfU,
    0xe42c3a9dU,0x0d507892U,0x9b6a5fccU,0x62547e46U,0xc2f68d13U,0xe890d8b8U,0x5e2e39f7U,0xf582c3afU,
    0xbe9f5d80U,0x7c69d093U,0xa96fd52dU,0xb3cf2512U,0x3bc8ac99U,0xa710187dU,0x6ee89c63U,0x7bdb3bbbU,
    0x09cd2678U,0xf46e5918U,0x01ec9ab7U,0xa8834f9aU,0x65e6956eU,0x7eaaffe6U,0x0821bccfU,0xe6ef15e8U,
    0xd9bae79bU,0xce4a6f36U,0xd4ea9f09U,0xd629b07cU,0xaf31a4b2U,0x312a3f23U,0x30c6a594U,0xc035a266U,
    0x37744ebcU,0xa6fc82caU,0xb0e090d0U,0x1533a7d8U,0x4af10498U,0xf741ecdaU,0x0e7fcd50U,0x2f1791f6U,
    0x8d764dd6U,0x4d43efb0U,0x54ccaa4dU,0xdfe49604U,0xe39ed1b5U,0x1b4c6a88U,0xb8c12c1fU,0x7f466551U,
    0x049d5eeaU,0x5d018c35U,0x73fa8774U,0x2efb0b41U,0x5ab3671dU,0x5292dbd2U,0x33e91056U,0x136dd647U,
    0x8c9ad761U,0x7a37a10cU,0x8e59f814U,0x89eb133cU,0xeecea927U,0x35b761c9U,0xede11ce5U,0x3c7a47b1U,
    0x599cd2dfU,0x3f55f273U,0x791814ceU,0xbf73c737U,0xea53f7cdU,0x5b5ffdaaU,0x14df3d6fU,0x867844dbU,
    0x81caaff3U,0x3eb968c4U,0x2c382434U,0x5fc2a340U,0x72161dc3U,0x0cbce225U,0x8b283c49U,0x41ff0d95U,
    0x7139a801U,0xde080cb3U,0x9cd8b4e4U,0x906456c1U,0x617bcb84U,0x70d532b6U,0x74486c5cU,0x42d0b857U,
};
static const uint32_t Td2[256]={
    0xa75051f4U,0x65537e41U,0xa4c31a17U,0x5e963a27U,0x6bcb3babU,0x45f11f9dU,0x58abacfaU,0x03934be3U,
    0xfa552030U,0x6df6ad76U,0x769188ccU,0x4c25f502U,0xd7fc4fe5U,0xcbd7c52aU,0x44802635U,0xa38fb562U,
    0x5a49deb1U,0x1b6725baU,0x0e9845eaU,0xc0e15dfeU,0x7502c32fU,0xf012814cU,0x97a38d46U,0xf9c66bd3U,
    0x5fe7038fU,0x9c951592U,0x7aebbf6dU,0x59da9552U,0x832dd4beU,0x21d35874U,0x692949e0U,0xc8448ec9U,
    0x896a75c2U,0x7978f48eU,0x3e6b9958U,0x71dd27b9U,0x4fb6bee1U,0xad17f088U,0xac66c920U,0x3ab47dceU,
    0x4a1863dfU,0x3182e51aU,0x33609751U,0x7f456253U,0x77e0b164U,0xae84bb6bU,0xa01cfe81U,0x2b94f908U,
    0x68587048U,0xfd198f45U,0x6c8794deU,0xf8b7527bU,0xd323ab73U,0x02e2724bU,0x8f57e31fU,0xab2a6655U,
    0x2807b2ebU,0xc2032fb5U,0x7b9a86c5U,0x08a5d337U,0x87f23028U,0xa5b223bfU,0x6aba0203U,0x825ced16U,
    0x1c2b8acfU,0xb492a779U,0xf2f0f307U,0xe2a14e69U,0xf4cd65daU,0xbed50605U,0x621fd134U,0xfe8ac4a6U,
    0x539d342eU,0x55a0a2f3U,0xe132058aU,0xeb75a4f6U,0xec390b83U,0xefaa4060U,0x9f065e71U,0x1051bd6eU,
    0x8af93e21U,0x063d96ddU,0x05aedd3eU,0xbd464de6U,0x8db59154U,0x5d0571c4U,0xd46f0406U,0x15ff6050U,
    0xfb241998U,0xe997d6bdU,0x43cc8940U,0x9e7767d9U,0x42bdb0e8U,0x8b880789U,0x5b38e719U,0xeedb79c8U,
    0x0a47a17cU,0x0fe97c42U,0x1ec9f884U,0x00000000U,0x86830980U,0xed48322bU,0x70ac1e11U,0x724e6c5aU,
    0xfffbfd0eU,0x38560f85U,0xd51e3daeU,0x3927362dU,0xd9640a0fU,0xa621685cU,0x54d19b5bU,0x2e3a2436U,
    0x67b10c0aU,0xe70f9357U,0x96d2b4eeU,0x919e1b9bU,0xc54f80c0U,0x20a261dcU,0x4b695a77U,0x1a161c12U,
    0xba0ae293U,0x2ae5c0a0U,0xe0433c22U,0x171d121bU,0x0d0b0e09U,0xc7adf28bU,0xa8b92db6U,0xa9c8141eU,
    0x198557f1U,0x074caf75U,0xddbbee99U,0x60fda37fU,0x269ff701U,0xf5bc5c72U,0x3bc54466U,0x7e345bfbU,
    0x29768b43U,0xc6dccb23U,0xfc68b6edU,0xf163b8e4U,0xdccad731U,0x85104263U,0x22401397U,0x112084c6U,
    0x247d854aU,0x3df8d2bbU,0x3211aef9U,0xa16dc729U,0x2f4b1d9eU,0x30f3dcb2U,0x52ec0d86U,0xe3d077c1U,
    0x166c2bb3U,0xb999a970U,0x48fa1194U,0x642247e9U,0x8cc4a8fcU,0x3f1aa0f0U,0x2cd8567dU,0x90ef2233U,
    0x4ec78749U,0xd1c1d938U,0xa2fe8ccaU,0x0b3698d4U,0x81cfa6f5U,0xde28a57aU,0x8e26dab7U,0xbfa43fadU,
    0x9de42c3aU,0x920d5078U,0xcc9b6a5fU,0x4662547eU,0x13c2f68dU,0xb8e890d8U,0xf75e2e39U,0xaff582c3U,
    0x80be9f5dU,0x937c69d0U,0x2da96fd5U,0x12b3cf25U,0x993bc8acU,0x7da71018U,0x636ee89cU,0xbb7bdb3bU,
    0x7809cd26U,0x18f46e59U,0xb701ec9aU,0x9aa8834fU,0x6e65e695U,0xe67eaaffU,0xcf0821bcU,0xe8e6ef15U,
    0x9bd9bae7U,0x36ce4a6fU,0x09d4ea9fU,0x7cd629b0U,0xb2af31a4U,0x23312a3fU,0x9430c6a5U,0x66c035a2U,
    0xbc37744eU,0xcaa6fc82U,0xd0b0e090U,0xd81533a7U,0x984af104U,0xdaf741ecU,0x500e7fcdU,0xf62f1791U,
    0xd68d764dU,0xb04d43efU,0x4d54ccaaU,0x04dfe496U,0xb5e39ed1U,0x881b4c6aU,0x1fb8c12cU,0x517f4665U,
    0xea049d5eU,0x355d018cU,0x7473fa87U,0x412efb0bU,0x1d5ab367U,0xd25292dbU,0x5633e910U,0x47136dd6U,
    0x618c9ad7U,0x0c7a37a1U,0x148e59f8U,0x3c89eb13U,0x27eecea9U,0xc935b761U,0xe5ede11cU,0xb13c7a47U,
    0xdf599cd2U,0x733f55f2U,0xce791814U,0x37bf73c7U,0xcdea53f7U,0xaa5b5ffdU,0x6f14df3dU,0xdb867844U,
    0xf381caafU,0xc43eb968U,0x342c3824U,0x405fc2a3U,0xc372161dU,0x250cbce2U,0x498b283cU,0x9541ff0dU,
    0x017139a8U,0xb3de080cU,0xe49cd8b4U,0xc1906456U,0x84617bcbU,0xb670d532U,0x5c74486cU,0x5742d0b8U,
};
static const uint32_t Td3[256]={
    0xf4a75051U,0x4165537eU,0x17a4c31aU,0x275e963aU,0xab6bcb3bU,0x9d45f11fU,0xfa58abacU,0xe303934bU,
    0x30fa5520U,0x766df6adU,0xcc769188U,0x024c25f5U,0xe5d7fc4fU,0x2acbd7c5U,0x35448026U,0x62a38fb5U,
    0xb15a49deU,0xba1b6725U,0xea0e9845U,0xfec0e15dU,0x2f7502c3U,0x4cf01281U,0x4697a38dU,0xd3f9c66bU,
    0x8f5fe703U,0x929c9515U,0x6d7aebbfU,0x5259da95U,0xbe832dd4U,0x7421d358U,0xe0692949U,0xc9c8448eU,
    0xc2896a75U,0x8e7978f4U,0x583e6b99U,0xb971dd27U,0xe14fb6beU,0x88ad17f0U,0x20ac66c9U,0xce3ab47dU,
    0xdf4a1863U,0x1a3182e5U,0x51336097U,0x537f4562U,0x6477e0b1U,0x6bae84bbU,0x81a01cfeU,0x082b94f9U,
    0x48685870U,0x45fd198fU,0xde6c8794U,0x7bf8b752U,0x73d323abU,0x4b02e272U,0x1f8f57e3U,0x55ab2a66U,
    0xeb2807b2U,0xb5c2032fU,0xc57b9a86U,0x3708a5d3U,0x2887f230U,0xbfa5b223U,0x036aba02U,0x16825cedU,
    0xcf1c2b8aU,0x79b492a7U,0x07f2f0f3U,0x69e2a14eU,0xdaf4cd65U,0x05bed506U,0x34621fd1U,0xa6fe8ac4U,
    0x2e539d34U,0xf355a0a2U,0x8ae13205U,0xf6eb75a4U,0x83ec390bU,0x60efaa40U,0x719f065eU,0x6e1051bdU,
    0x218af93eU,0xdd063d96U,0x3e05aeddU,0xe6bd464dU,0x548db591U,0xc45d0571U,0x06d46f04U,0x5015ff60U,
    0x98fb2419U,0xbde997d6U,0x4043cc89U,0xd99e7767U,0xe842bdb0U,0x898b8807U,0x195b38e7U,0xc8eedb79U,
    0x7c0a47a1U,0x420fe97cU,0x841ec9f8U,0x00000000U,0x80868309U,0x2bed4832U,0x1170ac1eU,0x5a724e6cU,
    0x0efffbfdU,0x8538560fU,0xaed51e3dU,0x2d392736U,0x0fd9640aU,0x5ca62168U,0x5b54d19bU,0x362e3a24U,
    0x0a67b10cU,0x57e70f93U,0xee96d2b4U,0x9b919e1bU,0xc0c54f80U,0xdc20a261U,0x774b695aU,0x121a161cU,
    0x93ba0ae2U,0xa02ae5c0U,0x22e0433cU,0x1b171d12U,0x090d0b0eU,0x8bc7adf2U,0xb6a8b92dU,0x1ea9c814U,
    0xf1198557U,0x75074cafU,0x99ddbbeeU,0x7f60fda3U,0x01269ff7U,0x72f5bc5cU,0x663bc544U,0xfb7e345bU,
    0x4329768bU,0x23c6dccbU,0xedfc68b6U,0xe4f163b8U,0x31dccad7U,0x63851042U,0x97224013U,0xc6112084U,
    0x4a247d85U,0xbb3df8d2U,0xf93211aeU,0x29a16dc7U,0x9e2f4b1dU,0xb230f3dcU,0x8652ec0dU,0xc1e3d077U,
    0xb3166c2bU,0x70b999a9U,0x9448fa11U,0xe9642247U,0xfc8cc4a8U,0xf03f1aa0U,0x7d2cd856U,0x3390ef22U,
    0x494ec787U,0x38d1c1d9U,0xcaa2fe8cU,0xd40b3698U,0xf581cfa6U,0x7ade28a5U,0xb78e26daU,0xadbfa43fU,
    0x3a9de42cU,0x78920d50U,0x5fcc9b6aU,0x7e466254U,0x8d13c2f6U,0xd8b8e890U,0x39f75e2eU,0xc3aff582U,
    0x5d80be9fU,0xd0937c69U,0xd52da96fU,0x2512b3cfU,0xac993bc8U,0x187da710U,0x9c636ee8U,0x3bbb7bdbU,
    0x267809cdU,0x5918f46eU,0x9ab701ecU,0x4f9aa883U,0x956e65e6U,0xffe67eaaU,0xbccf0821U,0x15e8e6efU,
    0xe79bd9baU,0x6f36ce4aU,0x9f09d4eaU,0xb07cd629U,0xa4b2af31U,0x3f23312aU,0xa59430c6U,0xa266c035U,
    0x4ebc3774U,0x82caa6fcU,0x90d0b0e0U,0xa7d81533U,0x04984af1U,0xecdaf741U,0xcd500e7fU,0x91f62f17U,
    0x4dd68d76U,0xefb04d43U,0xaa4d54ccU,0x9604dfe4U,0xd1b5e39eU,0x6a881b4cU,0x2c1fb8c1U,0x65517f46U,
    0x5eea049dU,0x8c355d01U,0x877473faU,0x0b412efbU,0x671d5ab3U,0xdbd25292U,0x105633e9U,0xd647136dU,
    0xd7618c9aU,0xa10c7a37U,0xf8148e59U,0x133c89ebU,0xa927eeceU,0x61c935b7U,0x1ce5ede1U,0x47b13c7aU,
    0xd2df599cU,0xf2733f55U,0x14ce7918U,0xc737bf73U,0xf7cdea53U,0xfdaa5b5fU,0x3d6f14dfU,0x44db8678U,
    0xaff381caU,0x68c43eb9U,0x24342c38U,0xa3405fc2U,0x1dc37216U,0xe2250cbcU,0x3c498b28U,0x0d9541ffU,
    0xa8017139U,0x0cb3de08U,0xb4e49cd8U,0x56c19064U,0xcb84617bU,0x32b670d5U,0x6c5c7448U,0xb85742d0U,
};
static const uint8_t Td4[256]={
    0x52U,0x09U,0x6aU,0xd5U,0x30U,0x36U,0xa5U,0x38U,0xbfU,0x40U,0xa3U,0x9eU,0x81U,0xf3U,0xd7U,0xfbU,
    0x7cU,0xe3U,0x39U,0x82U,0x9bU,0x2fU,0xffU,0x87U,0x34U,0x8eU,0x43U,0x44U,0xc4U,0xdeU,0xe9U,0xcbU,
    0x54U,0x7bU,0x94U,0x32U,0xa6U,0xc2U,0x23U,0x3dU,0xeeU,0x4cU,0x95U,0x0bU,0x42U,0xfaU,0xc3U,0x4eU,
    0x08U,0x2eU,0xa1U,0x66U,0x28U,0xd9U,0x24U,0xb2U,0x76U,0x5bU,0xa2U,0x49U,0x6dU,0x8bU,0xd1U,0x25U,
    0x72U,0xf8U,0xf6U,0x64U,0x86U,0x68U,0x98U,0x16U,0xd4U,0xa4U,0x5cU,0xccU,0x5dU,0x65U,0xb6U,0x92U,
    0x6cU,0x70U,0x48U,0x50U,0xfdU,0xedU,0xb9U,0xdaU,0x5eU,0x15U,0x46U,0x57U,0xa7U,0x8dU,0x9dU,0x84U,
    0x90U,0xd8U,0xabU,0x00U,0x8cU,0xbcU,0xd3U,0x0aU,0xf7U,0xe4U,0x58U,0x05U,0xb8U,0xb3U,0x45U,0x06U,
    0xd0U,0x2cU,0x1eU,0x8fU,0xcaU,0x3fU,0x0fU,0x02U,0xc1U,0xafU,0xbdU,0x03U,0x01U,0x13U,0x8aU,0x6bU,
    0x3aU,0x91U,0x11U,0x41U,0x4fU,0x67U,0xdcU,0xeaU,0x97U,0xf2U,0xcfU,0xceU,0xf0U,0xb4U,0xe6U,0x73U,
    0x96U,0xacU,0x74U,0x22U,0xe7U,0xadU,0x35U,0x85U,0xe2U,0xf9U,0x37U,0xe8U,0x1cU,0x75U,0xdfU,0x6eU,
    0x47U,0xf1U,0x1aU,0x71U,0x1dU,0x29U,0xc5U,0x89U,0x6fU,0xb7U,0x62U,0x0eU,0xaaU,0x18U,0xbeU,0x1bU,
    0xfcU,0x56U,0x3eU,0x4bU,0xc6U,0xd2U,0x79U,0x20U,0x9aU,0xdbU,0xc0U,0xfeU,0x78U,0xcdU,0x5aU,0xf4U,
    0x1fU,0xddU,0xa8U,0x33U,0x88U,0x07U,0xc7U,0x31U,0xb1U,0x12U,0x10U,0x59U,0x27U,0x80U,0xecU,0x5fU,
    0x60U,0x51U,0x7fU,0xa9U,0x19U,0xb5U,0x4aU,0x0dU,0x2dU,0xe5U,0x7aU,0x9fU,0x93U,0xc9U,0x9cU,0xefU,
    0xa0U,0xe0U,0x3bU,0x4dU,0xaeU,0x2aU,0xf5U,0xb0U,0xc8U,0xebU,0xbbU,0x3cU,0x83U,0x53U,0x99U,0x61U,
    0x17U,0x2bU,0x04U,0x7eU,0xbaU,0x77U,0xd6U,0x26U,0xe1U,0x69U,0x14U,0x63U,0x55U,0x21U,0x0cU,0x7dU,
};
static const uint32_t rcon[]={
    0x01000000,0x02000000,0x04000000,0x08000000,0x10000000,0x20000000,0x40000000,0x80000000,0x1B000000
};
uint32_t encrd[4*15],decrd[4*15];
uint8_t aes_iv[16];

void aes_init(unsigned char *pass)
{
    uint32_t i=0,j=0,t,*rk;
    memcpy(aes_iv,pass+(pass[0]&0xF),16);
    rk=encrd;
    rk[0]=GETU32(pass   );rk[1]=GETU32(pass+ 4);rk[2]=GETU32(pass+ 8);rk[3]=GETU32(pass+12);
    rk[4]=GETU32(pass+16);rk[5]=GETU32(pass+20);rk[6]=GETU32(pass+24);rk[7]=GETU32(pass+28);
    while (1) {
        t=rk[7];rk[8]=rk[0]^(Te2[(t>>16)&0xff]&0xff000000)^(Te3[(t>>8)&0xff]&0x00ff0000)^
            (Te0[(t)&0xff]&0x0000ff00)^(Te1[(t>>24)]&0x000000ff)^rcon[i];
        rk[9]=rk[1]^rk[8];rk[10]=rk[2]^rk[9];rk[11]=rk[3]^rk[10];
        if(++i==7) break;
        t=rk[11];rk[12]=rk[4]^(Te2[(t>>24)]&0xff000000)^(Te3[(t>>16)&0xff]&0x00ff0000)^
            (Te0[(t>>8)&0xff]&0x0000ff00)^(Te1[(t)&0xff]&0x000000ff);
        rk[13]=rk[5]^rk[12];rk[14]=rk[6]^rk[13];rk[15]=rk[7]^rk[14];rk+=8;
    }
    rk=decrd;
    memcpy(rk,&encrd,sizeof(decrd));
    for(i=0,j=4*14;i<j;i+=4,j-=4) {
        t=rk[i];rk[i]=rk[j];rk[j]=t;t=rk[i+1];rk[i+1]=rk[j+1];rk[j+1]=t;
        t=rk[i+2];rk[i+2]=rk[j+2];rk[j+2]=t;t=rk[i+3];rk[i+3]=rk[j+3];rk[j+3]=t;
    }
    for(i=1;i<14;i++) {
        rk+=4;
        rk[0]=Td0[Te1[(rk[0]>>24)]&0xff]^Td1[Te1[(rk[0]>>16)&0xff]&0xff]^
            Td2[Te1[(rk[0]>>8)&0xff]&0xff]^Td3[Te1[(rk[0])&0xff]&0xff];
        rk[1]=Td0[Te1[(rk[1]>>24)]&0xff]^Td1[Te1[(rk[1]>>16)&0xff]&0xff]^
            Td2[Te1[(rk[1]>>8)&0xff]&0xff]^Td3[Te1[(rk[1])&0xff]&0xff];
        rk[2]=Td0[Te1[(rk[2]>>24)]&0xff]^Td1[Te1[(rk[2]>>16)&0xff]&0xff]^
            Td2[Te1[(rk[2]>>8)&0xff]&0xff]^Td3[Te1[(rk[2])&0xff]&0xff];
        rk[3]=Td0[Te1[(rk[3]>>24)]&0xff]^Td1[Te1[(rk[3]>>16)&0xff]&0xff]^
            Td2[Te1[(rk[3]>>8)&0xff]&0xff]^Td3[Te1[(rk[3])&0xff]&0xff];
    }
}

void aes_enc(unsigned char *data)
{
    uint32_t rd[4*15],*rk=rd,n,s0,s1,s2,s3,t0,t1,t2,t3,l=secsize;
    unsigned char ivec[16],*iv=ivec,*in=data,*out=data;
    memcpy(&rd,&encrd,sizeof(rd));
    memcpy(ivec,aes_iv,sizeof(ivec));
    while(l>=16) {
        for(n=0;n<16;n++) out[n]=*in++^iv[n];
        rk=rd;s0=GETU32(out)^rk[0];s1=GETU32(out+4)^rk[1];s2=GETU32(out+8)^rk[2];s3=GETU32(out+12)^rk[3];
        for(n=14>>1;;) {
            t0=Te0[(s0>>24)]^Te1[(s1>>16)&0xff]^Te2[(s2>>8)&0xff]^Te3[(s3)&0xff]^rk[4];
            t1=Te0[(s1>>24)]^Te1[(s2>>16)&0xff]^Te2[(s3>>8)&0xff]^Te3[(s0)&0xff]^rk[5];
            t2=Te0[(s2>>24)]^Te1[(s3>>16)&0xff]^Te2[(s0>>8)&0xff]^Te3[(s1)&0xff]^rk[6];
            t3=Te0[(s3>>24)]^Te1[(s0>>16)&0xff]^Te2[(s1>>8)&0xff]^Te3[(s2)&0xff]^rk[7];
            rk+=8;
            if(--n==0) break;
            s0=Te0[(t0>>24)]^Te1[(t1>>16)&0xff]^Te2[(t2>>8)&0xff]^Te3[(t3)&0xff]^rk[0];
            s1=Te0[(t1>>24)]^Te1[(t2>>16)&0xff]^Te2[(t3>>8)&0xff]^Te3[(t0)&0xff]^rk[1];
            s2=Te0[(t2>>24)]^Te1[(t3>>16)&0xff]^Te2[(t0>>8)&0xff]^Te3[(t1)&0xff]^rk[2];
            s3=Te0[(t3>>24)]^Te1[(t0>>16)&0xff]^Te2[(t1>>8)&0xff]^Te3[(t2)&0xff]^rk[3];
        }
        s0=(Te2[(t0>>24)]&0xff000000)^(Te3[(t1>>16)&0xff]&0x00ff0000)^
            (Te0[(t2>>8)&0xff]&0x0000ff00)^(Te1[(t3)&0xff]&0x000000ff)^rk[0];
        PUTU32(out,s0);
        s1=(Te2[(t1>>24)]&0xff000000)^(Te3[(t2>>16)&0xff]&0x00ff0000)^
            (Te0[(t3>>8)&0xff]&0x0000ff00)^(Te1[(t0)&0xff]&0x000000ff)^rk[1];
        PUTU32(out+4,s1);
        s2=(Te2[(t2>>24)]&0xff000000)^(Te3[(t3>>16)&0xff]&0x00ff0000)^
            (Te0[(t0>>8)&0xff]&0x0000ff00)^(Te1[(t1)&0xff]&0x000000ff)^rk[2];
        PUTU32(out+8,s2);
        s3=(Te2[(t3>>24)]&0xff000000)^(Te3[(t0>>16)&0xff]&0x00ff0000)^
            (Te0[(t1>>8)&0xff]&0x0000ff00)^(Te1[(t2)&0xff]&0x000000ff)^rk[3];
        PUTU32(out+12,s3);
        iv=out;
        l-=16;
        out+=16;
    }
}

void aes_dec(unsigned char *data)
{
    uint32_t rd[4*15],*rk=rd,n,s0,s1,s2,s3,t0,t1,t2,t3,l=secsize;
    unsigned char ivec[16],c[sizeof(ivec)],d,*out=data;
    memcpy(&rd,&decrd,sizeof(rd));
    memcpy(ivec,aes_iv,sizeof(ivec));
    while(l>=16) {
        rk=rd;s0=GETU32(out)^rk[0];s1=GETU32(out+4)^rk[1];s2=GETU32(out+8)^rk[2];s3=GETU32(out+12)^rk[3];
        for(n=14>>1;;){
            t0=Td0[(s0>>24)]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[(s1)&0xff]^rk[4];
            t1=Td0[(s1>>24)]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[(s2)&0xff]^rk[5];
            t2=Td0[(s2>>24)]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[(s3)&0xff]^rk[6];
            t3=Td0[(s3>>24)]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[(s0)&0xff]^rk[7];
            rk+=8;
            if(--n==0) break;
            s0=Td0[(t0>>24)]^Td1[(t3>>16)&0xff]^Td2[(t2>>8)&0xff]^Td3[(t1)&0xff]^rk[0];
            s1=Td0[(t1>>24)]^Td1[(t0>>16)&0xff]^Td2[(t3>>8)&0xff]^Td3[(t2)&0xff]^rk[1];
            s2=Td0[(t2>>24)]^Td1[(t1>>16)&0xff]^Td2[(t0>>8)&0xff]^Td3[(t3)&0xff]^rk[2];
            s3=Td0[(t3>>24)]^Td1[(t2>>16)&0xff]^Td2[(t1>>8)&0xff]^Td3[(t0)&0xff]^rk[3];
        }
        s0=((uint32_t)Td4[(t0>>24)]<<24)^((uint32_t)Td4[(t3>>16)&0xff]<<16)^
            ((uint32_t)Td4[(t2>>8)&0xff]<<8)^((uint32_t)Td4[(t1)&0xff])^rk[0];
        PUTU32(c,s0);
        s1=((uint32_t)Td4[(t1>>24)]<<24)^((uint32_t)Td4[(t0>>16)&0xff]<<16)^
            ((uint32_t)Td4[(t3>>8)&0xff]<<8)^((uint32_t)Td4[(t2)&0xff])^rk[1];
        PUTU32(c+4,s1);
        s2=((uint32_t)Td4[(t2>>24)]<<24)^((uint32_t)Td4[(t1>>16)&0xff]<<16)^
            ((uint32_t)Td4[(t0>>8)&0xff]<<8)^((uint32_t)Td4[(t3)&0xff])^rk[2];
        PUTU32(c+8,s2);
        s3=((uint32_t)Td4[(t3>>24)]<<24)^((uint32_t)Td4[(t2>>16)&0xff]<<16)^
            ((uint32_t)Td4[(t1>>8)&0xff]<<8)^((uint32_t)Td4[(t0)&0xff])^rk[3];
        PUTU32(c+12,s3);
        for(n=0;n<16;n++) {d=out[n];out[n]=c[n]^ivec[n];ivec[n]=d;}
        l-=16;
        out+=16;
    }
}
#endif

/**
 * fájlrendszer lemezkép titkosítása
 */
void encrypt(int argc, char **argv)
{
    unsigned char chk[32],iv[32];
    char *pass,passphrase[256];
    unsigned int i,j,l;
    SHA256_CTX ctx;
    FSZ_SuperBlock *sb;
    if(argc<4 || (strcmp(argv[3],"sha") && strcmp(argv[3],"aes"))) { fprintf(stderr,"mkfs: %s\n"
            "  sha - SHA-XOR-CBC\n"
#if HAS_AES
            "  aes - AES-256-CBC\n"
#endif
            ,hu?"ismeretlen kódolási algoritmus":"unknown cipher algorithm."); exit(1); }
    if(argc<5 || argv[4]==NULL) {
        printf(hu?"Jelszó? ":"Passphrase? ");
        memset(passphrase,0,sizeof(passphrase));
        fgets(passphrase, sizeof(passphrase)-1, stdin);
        pass=passphrase;
    } else
        pass=argv[4];
    for(l=0;pass[l] && pass[l]!='\r' && pass[l]!='\n';l++);

    fs=readfileall(argv[1]);
    if(fs==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    sb=(FSZ_SuperBlock*)fs;
    if(sb->enchash!=0) { fprintf(stderr,"mkfs: %s\n",hu?"Már titkosítva van. Előbb kódold vissza az érvényes jelszóval.":
        "Already encrypted. Decrypt with current passphrase first."); exit(2); }
    /* jelszó hasítófüggvényértéke */
    sb->enchash=crc32a_calc(pass,l);
    /* titkosító kulcs feltöltése véletlen számokkal */
    srand(time(NULL));
    for(i=0;i<sizeof(sb->encrypt);i++) sb->encrypt[i]=rand();
    /* lemezkép elkódolása */
    SHA256_Init(&ctx);
    SHA256_Update(&ctx,&sb->encrypt,sizeof(sb->encrypt));
    SHA256_Final(iv,&ctx);
    if(!strcmp(argv[3],"aes")) {
#if HAS_AES
        sb->flags|=FSZ_SB_EALG_AESCBC;
        aes_init(iv);
        for(size=secsize;size<read_size;size+=secsize) {
            aes_enc((unsigned char*)fs+size);
        }
#else
        fprintf(stderr,"mkfs: aes %s.\n",hu?"nincs implementálva":"not implemented");
        exit(2);
#endif
    } else {
        for(size=secsize,j=1;size<read_size;size+=secsize,j++) {
            /* egy logikai szektor titkosítása */
            memcpy(chk,iv,32);
            for(i=0;i<secsize;i++) {
                if(i%32==0) {
                    SHA256_Init(&ctx);
                    SHA256_Update(&ctx,&chk,32);
                    SHA256_Update(&ctx,&j,4);
                    SHA256_Final(chk,&ctx);
                }
                fs[size+i]^=chk[i%32]^iv[i%32];
            }
        }
    }
    /* titkosító kulcs maszkolása a jelszó hasítófüggvényértékével */
    SHA256_Init(&ctx);
    SHA256_Update(&ctx,pass,l);
    SHA256_Update(&ctx,&sb->magic,6);
    SHA256_Final(chk,&ctx);
    for(i=0;i<sizeof(sb->encrypt);i++) sb->encrypt[i]^=chk[i];
    sb->checksum=crc32a_calc((char *)sb->magic,508);
    /* új lemezkép kiírása */
    f=fopen(argv[1],"wb");
    fwrite(fs,size,1,f);
    fclose(f);
}

/**
 * titkosított fájlrendszer lemezkép visszafejtése
 */
void decrypt(unsigned char *fs, long int size, char *pass)
{
    unsigned char chk[32],iv[32];
    char passphrase[256];
    uint32_t chksum;
    unsigned int i,j,l;
    SHA256_CTX ctx;
    FSZ_SuperBlock *sb=(FSZ_SuperBlock *)fs;
    if(fs==NULL || sb->enchash==0) return;
    if((((((FSZ_SuperBlock *)fs)->flags)>>2)&7)>HAS_AES) { fprintf(stderr,"mkfs: %s\n",
        hu?"ismeretlen kódolási algoritmus":"Unknown cipher"); exit(2); }
    while(!pass || !pass[0]) {
        printf(hu?"Jelszó? ":"Passphrase? ");
        memset(passphrase,0,sizeof(passphrase));
        fgets(passphrase, sizeof(passphrase)-1, stdin);
        pass=passphrase;
    }
    for(l=0;pass[l] && pass[l]!='\r' && pass[l]!='\n';l++);
    chksum=crc32a_calc(pass,l);
    if(sb->enchash!=chksum) {
        fprintf(stderr,"mkfs: %s\n",hu?"érvénytelen jelszó!":"Bad passphrase.");
        return;
    }
    /* titkosító kulcs kiszámítása */
    SHA256_Init(&ctx);
    SHA256_Update(&ctx,pass,l);
    SHA256_Update(&ctx,&sb->magic,6);
    SHA256_Final(chk,&ctx);
    for(i=0;i<sizeof(sb->encrypt);i++) sb->encrypt[i]^=chk[i];
    /* lemezkép visszafejtése */
    SHA256_Init(&ctx);
    SHA256_Update(&ctx,&sb->encrypt,sizeof(sb->encrypt));
    SHA256_Final(iv,&ctx);
    if(sb->flags&FSZ_SB_EALG_AESCBC) {
#if HAS_AES
        aes_init(iv);
        for(size=secsize,j=1;size<read_size;size+=secsize,j++) {
            aes_dec((unsigned char*)fs+size);
        }
#else
        fprintf(stderr,"mkfs: aes %s.\n",hu?"nincs implementálva":"not implemented");
        exit(2);
#endif
    } else {
        for(size=secsize,j=1;size<read_size;size+=secsize,j++) {
            /* egy logikai szektor visszafejtése */
            memcpy(chk,iv,32);
            for(i=0;i<secsize;i++) {
                if(i%32==0) {
                    SHA256_Init(&ctx);
                    SHA256_Update(&ctx,&chk,32);
                    SHA256_Update(&ctx,&j,4);
                    SHA256_Final(chk,&ctx);
                }
                fs[size+i]^=chk[i%32]^iv[i%32];
            }
        }
    }
    memset(sb->encrypt,0,32);
    sb->checksum=crc32a_calc((char *)sb->magic,508);
}

/**
 * Titkosító jelszó cseréje újrakódolás nélkül
 */
void chcrypt(int argc, char **argv)
{
    unsigned char chk[32];
    char *pass,passphrase[256];
    uint32_t chksum;
    unsigned int i,l;
    SHA256_CTX ctx;
    FSZ_SuperBlock *sb;
    fs=readfileall(argv[1]);
    if(fs==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    sb=(FSZ_SuperBlock*)fs;
    if(sb->enchash==0) { fprintf(stderr,"mkfs: %s\n",hu?"nincs titkosítva.":"Not encrypted."); exit(2); }
    /* titkosító jelszó bekérése */
    if(argc<4 || argv[3]==NULL) {
        printf(hu?"Régi jelszó? ":"Old passphrase? ");
        memset(passphrase,0,sizeof(passphrase));
        fgets(passphrase, sizeof(passphrase)-1, stdin);
        pass=passphrase;
    } else
        pass=argv[3];
    for(l=0;pass[l] && pass[l]!='\r' && pass[l]!='\n';l++);
    chksum=crc32a_calc(pass,l);
    if(sb->enchash!=chksum) {
        fprintf(stderr,"mkfs: %s\n",hu?"érvénytelen jelszó!":"Bad passphrase.");
        return;
    }
    SHA256_Init(&ctx);
    SHA256_Update(&ctx,pass,l);
    SHA256_Update(&ctx,&sb->magic,6);
    SHA256_Final(chk,&ctx);
    for(i=0;i<sizeof(sb->encrypt);i++) sb->encrypt[i]^=chk[i];
    /* új titkosító jelszó bekérése, új titkosító kulcs generálása */
    if(argc<5 || argv[4]==NULL) {
        printf(hu?"Új jelszó? ":"New passphrase? ");
        memset(passphrase,0,sizeof(passphrase));
        fgets(passphrase, sizeof(passphrase)-1, stdin);
        pass=passphrase;
    } else
        pass=argv[3];
    for(l=0;pass[l] && pass[l]!='\r' && pass[l]!='\n';l++);
    sb->enchash=crc32a_calc(pass,l);
    SHA256_Init(&ctx);
    SHA256_Update(&ctx,pass,l);
    SHA256_Update(&ctx,&sb->magic,6);
    SHA256_Final(chk,&ctx);
    for(i=0;i<sizeof(sb->encrypt);i++) sb->encrypt[i]^=chk[i];
    sb->checksum=crc32a_calc((char *)sb->magic,508);
    /* új lemezkép kiírása */
    f=fopen(argv[1],"wb");
    fwrite(fs,read_size,1,f);
    fclose(f);
}

/* fájlrendszer funkciók */
/**
 * FS/Z superblock hozzáadása a kimenethez
 */
void add_superblock()
{
    FSZ_SuperBlock *sb;
    unsigned int i;
    fs=realloc(fs,size+secsize);
    if(fs==NULL) exit(4);
    memset(fs+size,0,secsize);
    sb=(FSZ_SuperBlock *)(fs+size);
    memcpy(sb->magic,FSZ_MAGIC,4);
    memcpy((char*)&sb->owner,"root",4);
    sb->version_major=FSZ_VERSION_MAJOR;
    sb->version_minor=FSZ_VERSION_MINOR;
    sb->raidtype=FSZ_SB_SOFTRAID_NONE;
    sb->logsec=secsize==2048?0:(secsize==4096?1:2); /* 0=2048, 1=4096, 2=8192 */
    sb->physec=secsize/512;                         /* logsec/media szektorméret */
    sb->maxmounts=255;
    sb->currmounts=0;
    sb->createdate=sb->lastchangedate=ts;
    srand(time(NULL));
    for(i=0;i<sizeof(sb->uuid);i++) sb->uuid[i]=rand();
    memcpy(sb->magic2,FSZ_MAGIC,4);
    size+=secsize;
}

/**
 * inode hozzáadása, visszaadja az lsn-jét
 */
int add_inode(char *filetype, char *mimetype)
{
    unsigned int i,j=!strcmp(filetype,FSZ_FILETYPE_SYMLINK)||!strcmp(filetype,FSZ_FILETYPE_UNION)?secsize-1024:36;
    FSZ_Inode *in;
    if(((FSZ_SuperBlock *)fs)->enchash!=0) { fprintf(stderr,"mkfs: %s\n",
        hu?"titkosított lemezkép nem módosítható.":"Cannot modify an encrypted image."); exit(2); }
    fs=realloc(fs,size+secsize);
    if(fs==NULL) exit(4);
    memset(fs+size,0,secsize);
    in=(FSZ_Inode *)(fs+size);
    memcpy(in->magic,FSZ_IN_MAGIC,4);
    memcpy((char*)&in->owner,"root",5);
    in->owner.access=FSZ_READ|FSZ_WRITE|FSZ_DELETE|(
        !strcmp(filetype,FSZ_FILETYPE_DIR) || !strcmp(filetype,FSZ_FILETYPE_UNION)? FSZ_EXEC : 0);
    if(filetype!=NULL){
        i=strlen(filetype);
        memcpy(in->filetype,filetype,i>4?4:i);
        if(!strcmp(filetype,FSZ_FILETYPE_DIR)){
            FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)(in->data.small.inlinedata);
            in->sec=size/secsize;
            in->flags=FSZ_IN_FLAG_INLINE;
            in->size=sizeof(FSZ_DirEntHeader);
            memcpy(in->data.small.inlinedata,FSZ_DIR_MAGIC,4);
            hdr->checksum=crc32a_calc((char*)hdr+sizeof(FSZ_DirEntHeader),hdr->numentries*sizeof(FSZ_DirEnt));
        }
    }
    if(mimetype!=NULL){
        if(!strcmp(filetype,FSZ_FILETYPE_UNION)){
            for(i=1;i<j && !(mimetype[i-1]==0 && mimetype[i]==0);i++);
            i++;
        } else {
            i=strlen(mimetype);
        }
        memcpy(j==36?in->mimetype:in->data.small.inlinedata,mimetype,i>j?j:i);
        if(j!=36)
            in->size=i;
    }
    in->changedate=ts;
    in->modifydate=ts;
    in->checksum=crc32a_calc((char*)in->filetype,1016);
    size+=secsize;
    return size/secsize-1;
}

/**
 * inode beregisztrálása a fájlrendszer hierarchiába
 */
void link_inode(int inode, char *path, int toinode)
{
    unsigned int ns=0,cnt=0;
    FSZ_DirEntHeader *hdr;
    FSZ_DirEnt *ent;
    FSZ_Inode *in, *in2;
    if(((FSZ_SuperBlock *)fs)->enchash!=0) { fprintf(stderr,"mkfs: %s\n",
        hu?"titkosított lemezkép nem módosítható.":"Cannot modify an encrypted image."); exit(2); }
    if(toinode==0)
        toinode=((FSZ_SuperBlock *)fs)->rootdirfid;
    hdr=(FSZ_DirEntHeader *)(fs+toinode*secsize+1024);
    ent=(FSZ_DirEnt *)hdr; ent++;
    while(path[ns]!='/'&&path[ns]!=0) ns++;
    while(ent->fid!=0 && cnt<((secsize-1024)/128-1)) {
        if(!strncmp((char *)(ent->name),path,ns+1)) {
            link_inode(inode,path+ns+1,ent->fid);
            return;
        }
        ent++; cnt++;
    }
    in=((FSZ_Inode *)(fs+toinode*secsize));
    in2=((FSZ_Inode *)(fs+inode*secsize));
    ent->fid=inode;
    ent->length=strlen(path);
    memcpy(ent->name,path,strlen(path));
    if(!strncmp((char *)(((FSZ_Inode *)(fs+inode*secsize))->filetype),"dir:",4)){
        ent->name[ent->length++]='/';
    }
    if(hdr->numentries >= (secsize - 1024 - sizeof(FSZ_DirEntHeader)) / sizeof(FSZ_DirEnt)) {
        fprintf(stderr,"mkfs: %s: %s\n",hu?"túl sok könyvtárbejegyzés":"too many entries in directory",path); exit(1);
    }
    hdr->numentries++;
    in->modifydate=ts;
    in->size+=sizeof(FSZ_DirEnt);
    qsort((char*)hdr+sizeof(FSZ_DirEntHeader), hdr->numentries, sizeof(FSZ_DirEnt), direntcmp);
    hdr->checksum=crc32a_calc((char*)hdr+sizeof(FSZ_DirEntHeader),hdr->numentries*sizeof(FSZ_DirEnt));
    in->checksum=crc32a_calc((char*)in->filetype,1016);
    in2->numlinks++;
    in2->checksum=crc32a_calc((char*)in2->filetype,1016);
}

/**
 * fájl beolvasása és a kimenethez adása
 */
void add_file(char *name, char *datafile)
{
    FSZ_Inode *in;
    unsigned char *data=readfileall(datafile);
    long int i,j,k,l,inode,s=((read_size+secsize-1)/secsize)*secsize;
    if(!data) {
        fprintf(stderr,"mkfs: %s: %s\n",hu?"nem tudom beolvasni":"unable to read",datafile);
        exit(5);
    }
    /* ha a stage2-t a rendszerpartícióra tesszük a /boot helyett (ahová való),
     * akkor módosítjuk a mime típusát, nehogy a defrag áthelyezze. */
    inode=add_inode(data[0]==0x55 && data[1]==0xAA &&
               data[3]==0xE9 && data[8]=='B' &&
               data[12]=='B'?"boot":"application","octet-stream");
    fs=realloc(fs,size+s+secsize);
    if(fs==NULL) exit(4);
    memset(fs+size,0,s+secsize);
    in=(FSZ_Inode *)(fs+inode*secsize);
    in->changedate=ts;
    in->modifydate=ts;
    in->size=read_size;
    if(read_size<=secsize-1024) {
        /* kicsi, beágyazott adat */
        in->sec=inode;
        in->flags=FSZ_IN_FLAG_INLINE;
        in->numblocks=0;
        memcpy(in->data.small.inlinedata,data,read_size);
        s=0;
    } else {
        in->sec=size/secsize;
        if(read_size>secsize) {
            unsigned char *ptr;
            /* szektorkönyvtár */
            j=s/secsize;
            if(j*16>secsize){
                fprintf(stderr,"mkfs: %s: %s\n",hu?"túl nagy fájl":"file too big",datafile);
                exit(5);
            }
            /* beágyazható? */
            if(j*16<=secsize-1024) {
                ptr=(unsigned char*)&in->data.small.inlinedata;
                in->flags=FSZ_IN_FLAG_SDINLINE;
                in->numblocks=0;
                l=0;
            } else {
                ptr=fs+size;
                in->flags=FSZ_IN_FLAG_SD;
                in->numblocks=1;
                l=1;
            }
            k=inode+1+l;
            for(i=0;i<j;i++){
                /* az initrd/ben nem kezelünk lyukakat, mivel a core/fs.c csak
                 * egészben tudja kezelni a fájlokat, és egyébként is gzippelni fogjuk. */
                if(initrd || memcmp(data+i*secsize,emptysec,secsize)) {
                    memcpy(ptr,&k,4);
                    memcpy(fs+size+(i+l)*secsize,data+i*secsize,
                        (i+l)*secsize>read_size?read_size%secsize:secsize);
                    k++;
                    in->numblocks++;
                } else {
                    s-=secsize;
                }
                ptr+=16;
            }
            if(in->flags==FSZ_IN_FLAG_SD)
                size+=secsize;
        } else {
            /* közvetlen szektor hivatkozás */
            in->flags=FSZ_IN_FLAG_DIRECT;
            if(memcmp(data,emptysec,secsize)) {
                in->numblocks=1;
                memcpy(fs+size,data,read_size);
            } else {
                in->sec=0;
                in->numblocks=0;
            }
        }
    }
    /* fájltípus meghatározása */
    if(!strncmp((char*)data+1,"ELF",3) || !strncmp((char*)data,"OS/Z",4) || !strncmp((char*)data,"CSBC",4) ||
        !strncmp((char*)data,"\000asm",4))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"executable",10);in->owner.access|=FSZ_EXEC;}
    if(!strcmp(name+strlen(name)-3,".so"))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"sharedlib",9);}
    else
    /* ez egy egyszerű program, libmagic-et kéne használni. De ez is elég
     * a céljainkra, és eggyel kevesebb függőség. */
    if(!strcmp(name+strlen(name)-2,".h")||                              /* források, dokumentációk, konfiguráció */
       !strcmp(name+strlen(name)-2,".c")||
       !strcmp(name+strlen(name)-3,".md")||
       !strcmp(name+strlen(name)-4,".txt")||
       !strcmp(name+strlen(name)-5,".conf")
      ) {memset(in->mimetype,0,36);memcpy(in->mimetype,"plain",5);
         memcpy(in->filetype,"text",4);
        }
    else
    if(!strcmp(name+strlen(name)-3,".sh"))                              /* szkriptek */
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"shellscript",11);
         memcpy(in->filetype,"text",4);in->owner.access|=FSZ_EXEC;
        }
    else
    if(!strcmp(name+strlen(name)-4,".htm")||                            /* weblapok */
       !strcmp(name+strlen(name)-5,".html")
      )
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"html",4);
         memcpy(in->filetype,"text",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".css"))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"stylesheet",10);
         memcpy(in->filetype,"text",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".svg"))                             /* képformátumok */
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"svg",3);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".gif"))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"gif",3);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".png"))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"png",3);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".jpg"))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"jpeg",4);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".bmp"))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"bitmap",6);
         memcpy(in->filetype,"imag",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".sfn"))                             /* fontok */
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"ssfont",6);
         memcpy(in->filetype,"font",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".psf"))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"pc-screen-font",14);
         memcpy(in->filetype,"font",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".ttf"))
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"sfnt",4);
         memcpy(in->filetype,"font",4);
        }
    else
    if(!strcmp(name+strlen(name)-4,".m3d"))                             /* ikonok */
        {memset(in->mimetype,0,36);memcpy(in->mimetype,"3d-model",8);
         memcpy(in->filetype,data[1]=='d' ? "text" : "mode",4);
        }
    else {
        /* sima 7 bites ascii fájlok detektálása */
        j=1; for(i=0;i<read_size;i++) if(data[i]<9) { j=0; break; }
        if(j) {
         memset(in->mimetype,0,36);memcpy(in->mimetype,"plain",5);
         memcpy(in->filetype,"text",4);
        }
    }
    in->checksum=crc32a_calc((char*)in->filetype,1016);
    size+=s;
    link_inode(inode,name,0);
}

/**
 * egy könyvtár rekurzív hozzáadása a kimenethez
 */
void add_dirs(char *dirname,int parent,int level)
{
    DIR *dir;
    struct dirent *ent;
    char *full, *ptrto;
    int i,s;
    if(level>4) return;
    if(((FSZ_SuperBlock *)fs)->enchash!=0) { fprintf(stderr,"mkfs: %s\n",
        hu?"titkosított lemezkép nem módosítható.":"Cannot modify an encrypted image."); exit(2); }
    full=malloc(4096);
    ptrto=malloc(secsize-1024);
    if(parent==0) parent=strlen(dirname)+1;
    if ((dir = opendir (dirname)) != NULL) {
      while ((ent = readdir (dir)) != NULL) {
        if(!strcmp(ent->d_name,".")||!strcmp(ent->d_name,".."))
            continue;
        sprintf(full,"%s/%s",dirname,ent->d_name);
        if(ent->d_type==DT_DIR) {
            /* alkönyvtárak hozzáadása rekurzióval */
            i=add_inode("dir:",NULL);
            link_inode(i,full+parent,0);
            add_dirs(full,parent,level+1);
        }
        if(ent->d_type==DT_REG) {
            /* a sys/core-t már hozzáadtuk, mert annak kell a legelső futtathatónak lennie az initrd-ben */
            if(strcmp(full+parent,"sys/core")) {
                /* sima fájl hozzáadása */
                add_file(full+parent,full);
            }
        }
        if(ent->d_type==DT_LNK) {
            /* szimbólikus hivatkozás hozzáadása */
            s=readlink(full,ptrto,secsize-1024-1); ptrto[s]=0;
            i=add_inode(FSZ_FILETYPE_SYMLINK,ptrto);
            link_inode(i,full+parent,0);
        }
        /* esetleg más típusok, blkdev, socket stb. hozzáadása */
      }
      closedir (dir);
    }
    free(full);
    free(ptrto);
}

/* az mkfs funkciói */
/**
 * egy fájl keresése az FS/Z lemezképben
 */
FSZ_Inode *locate(unsigned char *data, int inode, char *path)
{
    unsigned int ns=0,cnt=0;
    FSZ_DirEntHeader *hdr;
    FSZ_DirEnt *ent;
    hdr=(FSZ_DirEntHeader *)(data+(inode?(uint64_t)inode:((FSZ_SuperBlock *)data)->rootdirfid)*secsize+1024);
    ent=(FSZ_DirEnt *)hdr; ent++;
    if(path==NULL || path[0]==0)
        return (FSZ_Inode *)(data + inode*secsize);
    if(path[0]=='/') {
        path++;
        if(path[0]==0)
            return (FSZ_Inode *)(data + (((FSZ_SuperBlock *)data)->rootdirfid)*secsize);
    }
    while(path[ns]!='/'&&path[ns]!=0) ns++;
    while(ent->fid!=0 && cnt<((secsize-1024)/128-1)) {
        if(!strncmp((char *)(ent->name),path,ns+1)) {
            if(path[ns]==0)
                return (FSZ_Inode *)(data + ent->fid*secsize);
            else
                return locate(data,ent->fid,path+ns+1);
        }
        if(!strncmp((char *)(ent->name),path,ns)&&path[ns]==0&&ent->name[ns]=='/') {
            return (FSZ_Inode *)(data + ent->fid*secsize);
        }
        ent++; cnt++;
    }
    return NULL;
}

/**
 * fordítás ellenőrzés, a struct elemeket nem szabad optimalizálni és átrendezni
 */
void checkcompilation()
{
    FSZ_SuperBlock sb;
    FSZ_Inode in;
    FSZ_DirEntHeader hdr;
    FSZ_DirEnt ent;
    /* ********* FIGYELMEZTETÉS *********
     * egyezniük kell az include/sys/fsZ.h-ban lévőkkel! */
    if( (uint64_t)(&sb.numsec) - (uint64_t)(&sb) != 528 ||
        (uint64_t)(&sb.rootdirfid) - (uint64_t)(&sb) != 560 ||
        (uint64_t)(&sb.owner) - (uint64_t)(&sb) != 760 ||
        (uint64_t)(&sb.magic2) - (uint64_t)(&sb) != 1016 ||
        (uint64_t)(&in.filetype) - (uint64_t)(&in) != 8 ||
        (uint64_t)(&in.version5) - (uint64_t)(&in) != 128 ||
        (uint64_t)(&in.sec) - (uint64_t)(&in) != 448 ||
        (uint64_t)(&in.size) - (uint64_t)(&in) != 464 ||
        (uint64_t)(&in.data.small.groups) - (uint64_t)(&in) != 512 ||
        ((uint64_t)(&in.data.small.inlinedata) - (uint64_t)(&in) != 1024 &&
         (uint64_t)(&in.data.big.inlinedata) - (uint64_t)(&in) != 2048) ||
        (uint64_t)(&hdr.numentries) - (uint64_t)(&hdr) != 16 ||
        (uint64_t)(&ent.name) - (uint64_t)(&ent) != 17) {
        fprintf(stderr,"mkfs: %s\n",hu?"a fordító átrendezte a struktúra tagjait. Fordítsd újra packed struct attribútummal.":
            "Your compiler rearranged structure members. Recompile me with packed struct.");
        exit(1);
    }
}

/* parancssori opciók */
/**
 * partíciós képek beolvasása és komplett lemezkép összeállítása
 */
int createdisk(int neediso)
{
    unsigned long int i,j=0,gs=(neediso?63:7)*512,rs1=0,rs2=0,es,us1=0,us2=0,vs,hs,ss=4096,bbs=0,ds=0;
    unsigned long int uuid[4]={0x12345678,0x12345678,0x12345678,0x12345678};
    unsigned char *esp, *ssp1, *ssp2, *usr1, *usr2, *var, *home, *gpt, *iso, *swap, *p, *loader;
    time_t t=time(NULL);
    struct tm *ts=gmtime(&t);
    char isodate[17];
    esp=readfileall(espfile);   es=read_size;
    ssp1=readfileall(sys1file); rs1=read_size;
    ssp2=readfileall(sys2file); rs2=read_size;
    usr1=readfileall(usr1file); us1=read_size;
    usr2=readfileall(usr2file); us2=read_size;
    var=readfileall(varfile);   vs=read_size;
    home=readfileall(homefile); hs=read_size;
    gpt=malloc(gs+512);
    memset(gpt,0,gs+512);
    iso=malloc(32768);
    swap=malloc(ss);
    if(neediso)
        memset(iso,0,32768);
    /* MBR / VBR kód beolvasása (ha van) */
    loader=readfileall(stage1);   /* stage1 betöltő */
    if(loader==NULL) {
        loader=malloc(512);
        memset(loader,0,512);
    } else {
        memset(loader+0x1B8,0,0x1FE - 0x1B8);
    }
    j=0;
    /* stage2 betöltő (FS0:\BOOTBOOT\LOADER) keresése az ESP-n */
    if(es>0) {
        for(i=0;i<es-512;i+=512) {
            if((unsigned char)esp[i+0]==0x55 &&
               (unsigned char)esp[i+1]==0xAA &&
               (unsigned char)esp[i+3]==0xE9 &&
               (unsigned char)esp[i+8]=='B' &&
               (unsigned char)esp[i+12]=='B') {
                bbs=((i+(neediso?65536:gs+512))/512);
                j=1;
                break;
            }
        }
    }
    /* stage2 betöltő (/sys/loader) keresése a rendszer partíción */
    if(!j && rs1>0) {
        for(i=0;i<rs1-512;i+=512) {
            if((unsigned char)ssp1[i+0]==0x55 &&
               (unsigned char)ssp1[i+1]==0xAA &&
               (unsigned char)ssp1[i+3]==0xE9 &&
               (unsigned char)ssp1[i+8]=='B' &&
               (unsigned char)ssp1[i+12]=='B') {
                bbs=((i+(neediso?65536:gs+512)+es)/512);
                break;
            }
        }
    }
    /* stage2 címének lementése a stage1-be */
    setint(bbs,loader+0x1B0);
    /* azonosító bájtok */
    loader[0x1FE]=0x55; loader[0x1FF]=0xAA;

    /* stage1 betöltő másolása a VBR-be is. Ez akkor használt,
     * ha később egy boot menedzsert installálnak a lemezre */
    if(loader[0]!=0) {
        if(es>0) {
            /* ha van ESP partíciónk */
            memcpy(esp, loader, 11);
            /* BPB terület kihagyása */
            memcpy(esp+0x5A, loader+0x5A, 512-0x5A);
        } else
            /* egyébként az initrd partíciót használjuk, vagy végső esetben a rendszer partíciót */
            memcpy(rs1>0?ssp1:usr1, loader, 512);
    }

    /* WinNT disk id */
    setint(uuid[0],loader+0x1B8);

    /* partíciós táblák generálása */

    /* MEGJEGYZÉS: ahol nincs GPT, ott az OS/Z a 0x30-0x33 MBR típusokat használja
     * 1. partíció: hardver függő (valószínűleg 0xC, /boot)
     * 2. partíció: OS/Z rendszer (0x30, /usr)
     * 3. partíció: OS/Z publikus adat (0x31, /var)
     * 4. partíció: OS/Z privát adat (0x32, /home) */

    j=0x1C0;
    if(es>0) {
        /* MBR, EFI Rendszer Partíció (ESP) */
        loader[j-2]=0x80;                           /* bootolható jelző */
        setint(129,loader+j);                       /* eleje CHS */
        loader[j+2]=0xC;                            /* típus, FAT */
        setint(((gs+es)/512)+2,loader+j+4);         /* vége CHS */
        setint(neediso?128:(gs/512)+1,loader+j+6);  /* eleje LBA */
        setint(((es)/512),loader+j+10);             /* szektorok száma */
    }
    j+=16;
    /* MBR, GPT bejegyzés */
    setint(1,loader+j);                             /* eleje CHS */
    loader[j+2]=0xEE;                               /* típus */
    setint((gs/512)+1,loader+j+4);                  /* vége CHS */
    setint(1,loader+j+6);                           /* eleje LBA */
    setint((gs/512),loader+j+10);                   /* szektorok száma */
    j+=16;
    /* MBR, bootolható OS/Z rendszer partíció (az opcionális boot menedzsernek)
     * ugyanazokat a szektorszámokat használjuk, függetlenül attól, hogy van-e initrd partíció */
    if(rs1>0) {
        if(es==0)
            loader[j-2]=0x80;                       /* bootolható jelző */
        setint(((es)/512)+128,loader+j);            /* eleje CHS */
        loader[j+2]=0x30;                           /* típus */
        setint(((es)/512)+128+512,loader+j+4);      /* vége CHS */
        setint(((es+rs1)/512)+(neediso?128:(gs/512)+1),loader+j+6);/* eleje LBA */
        setint(((rs1)/512),loader+j+10);            /* szektorok száma */
    }
    /* GPT Fejléc */
    ds = 512+2*gs+es+rs1+rs2+us1+us2+vs+hs+ss+(neediso?32768:0);
    memset(gpt,0,gs);
    memcpy(gpt,"EFI PART",8);                       /* azonosító */
    setint(1,gpt+10);                               /* revizió */
    setint(92,gpt+12);                              /* méret */
    setint(0xDEADCC32,gpt+16);                      /* crc */
    setint(1,gpt+24);                               /* elsődleges LBA */
    setint(ds/512-1,gpt+32);                        /* másodlagos LBA */
    setint(64,gpt+40);                              /* első használható LBA */
    setint(ds/512-64,gpt+48);                       /* utolsó használható LBA */
    setint(uuid[0],gpt+56);                         /* lemez UUID */
    setint(uuid[1],gpt+60);
    setint(uuid[2],gpt+64);
    setint(uuid[3],gpt+68);
    setint(2,gpt+72);                               /* partícióstábla LBA */
    setint(248,gpt+80);                             /* bejegyzések száma */
    setint(128,gpt+84);                             /* egy bejegyzés mérete */
    setint(0xDEADCC32,gpt+88);                      /* bejegyzések crc-je */

    p=gpt+512;
    /* GPT, EFI Rendszer partíció (ESP, /boot) OPCIONÁLIS */
    if(es>0) {
        setint(0x0C12A7328,p);                      /* bejegyzéstípus */
        setint(0x011D2F81F,p+4);
        setint(0x0A0004BBA,p+8);
        setint(0x03BC93EC9,p+12);
        setint(uuid[0]+1,p+16);                     /* partíció UUID */
        setint(uuid[1],p+20);
        setint(uuid[2],p+24);
        setint(uuid[3],p+28);
        setint((neediso?128:(gs/512)+1),p+32);      /* eleje LBA */
        setint(((es)/512)+(neediso?127:(gs/512)),p+40); /* vége LBA */
        memcpy(p+56,L"EFI System Partition",42);    /* név */
        p+=128;
    }
    j=(neediso?65536-512:gs)+es;

    /* GPT, OS/Z initrd partíció (gyökér, /) OPCIONÁLIS MBR típus 0x30 */
    if(rs1>0) {
        memcpy(p,"OS/Z",4);                      /* bejegyzéstípus, azonosító */
        setint(0x8664,p+4);                      /* verzió */
        memcpy(p+6,"FS/Z",4);
        memcpy(p+12,"root",4);                   /* felcsatolási pont */
        setint(uuid[0]+2,p+16);                  /* partíció UUID */
        setint(uuid[1],p+20);
        setint(uuid[2],p+24);
        setint(uuid[3],p+28);
        setint((j/512)+1,p+32);                  /* eleje LBA */
        setint(((j+rs1)/512),p+40);              /* vége LBA */
        setint(4,p+48);                          /* bootolható jelző a BOOTBOOT-nak */
        memcpy(p+56,hu?L"OS/Z Alaprendszer":L"OS/Z Root",22); /* név */
        p+=128;
        j+=rs1;
    }
    if(rs2>0) {
        memcpy(p,"OS/Z",4);                      /* bejegyzéstípus, azonosító */
        setint(0xAA64,p+4);                      /* verzió */
        memcpy(p+6,"FS/Z",4);
        memcpy(p+12,"root",4);                   /* felcsatolási pont */
        setint(uuid[0]+2,p+16);                  /* partíció UUID */
        setint(uuid[1],p+20);
        setint(uuid[2],p+24);
        setint(uuid[3],p+28);
        setint((j/512)+1,p+32);                  /* eleje LBA */
        setint(((j+rs2)/512),p+40);              /* vége LBA */
        setint(4,p+48);                          /* bootolható jelző a BOOTBOOT-nak */
        memcpy(p+56,hu?L"OS/Z Alaprendszer":L"OS/Z Root",22); /* név */
        p+=128;
        j+=rs2;
    }
    /* GPT, OS/Z Rendszer partíció (/usr) MBR típus 0x30 */
    if(us1>0) {
        memcpy(p,"OS/Z",4);                      /* bejegyzéstípus, azonosító */
        setint(0x8664,p+4);                      /* verzió */
        memcpy(p+6,"FS/Z",4);
        memcpy(p+12,"usr",3);                    /* felcsatolási pont */
        setint(uuid[0]+3,p+16);                  /* partíció UUID */
        setint(uuid[1],p+20);
        setint(uuid[2],p+24);
        setint(uuid[3],p+28);
        setint((j/512)+1,p+32);                  /* eleje LBA */
        setint(((j+us1)/512),p+40);              /* vége LBA */
        memcpy(p+56,hu?L"OS/Z Rendszer":L"OS/Z System",22); /* név */
        p+=128;
        j+=us1;
    }
    if(us2>0) {
        memcpy(p,"OS/Z",4);                      /* bejegyzéstípus, azonosító */
        setint(0xAA64,p+4);                      /* verzió */
        memcpy(p+6,"FS/Z",4);
        memcpy(p+12,"usr",3);                    /* felcsatolási pont */
        setint(uuid[0]+3,p+16);                  /* partíció UUID */
        setint(uuid[1],p+20);
        setint(uuid[2],p+24);
        setint(uuid[3],p+28);
        setint((j/512)+1,p+32);                  /* eleje LBA */
        setint(((j+us2)/512),p+40);              /* vége LBA */
        memcpy(p+56,hu?L"OS/Z Rendszer":L"OS/Z System",22); /* név */
        p+=128;
        j+=us2;
    }
    /* GPT, OS/Z Publikus Adat partíció (/var) MBR típus 0x31 */
    memcpy(p,"OS/Z",4);                          /* bejegyzéstípus, azonosító */
    memcpy(p+6,"FS/Z",4);
    memcpy(p+12,"var",3);                        /* felcsatolási pont */
    setint(uuid[0]+4,p+16);                      /* partíció UUID */
    setint(uuid[1],p+20);
    setint(uuid[2],p+24);
    setint(uuid[3],p+28);
    setint((j/512)+1,p+32);                      /* eleje LBA */
    setint(((j+vs)/512),p+40);                   /* vége LBA */
    memcpy(p+56,hu?L"OS/Z Publikus adatok":L"OS/Z Public Data",32); /* név */
    p+=128;
    j+=vs;

    /* GPT, OS/Z Privát Adat partíció (/home) MBR típus 0x32 */
    memcpy(p,"OS/Z",4);                          /* bejegyzéstípus, azonosító */
    memcpy(p+6,"FS/Z",4);
    memcpy(p+12,"home",4);                       /* felcsatolási pont */
    setint(uuid[0]+5,p+16);                      /* partíció UUID */
    setint(uuid[1],p+20);
    setint(uuid[2],p+24);
    setint(uuid[3],p+28);
    setint((j/512)+1,p+32);                      /* eleje LBA */
    setint(((j+hs)/512),p+40);                   /* vége LBA */
    memcpy(p+56,hu?L"OS/Z Privát adatok":L"OS/Z Private Data",34); /* név */
    p+=128;
    j+=hs;

    /* GPT, OS/Z Swap partíció (/dev/swap) MBR típus 0x33 */
    memcpy(p,"OS/Z",4);                          /* bejegyzéstípus, azonosító */
    memcpy(p+6,"FS/Z",4);
    memcpy(p+12,"swap",4);                       /* felcsatolási pont */
    setint(uuid[0]+6,p+16);                      /* partíció UUID */
    setint(uuid[1],p+20);
    setint(uuid[2],p+24);
    setint(uuid[3],p+28);
    setint((j/512)+1,p+32);                      /* eleje LBA */
    setint(((j+ss)/512),p+40);                   /* vége LBA */
    memcpy(p+56,hu?L"Lapcsere tárhely":L"OS/Z Swap Area",30); /* név */
    p+=128;
    j+=ss;

    /* Ellenőrzőösszegek számítása */
    /* partíciós tábla */
    i=(int)(gpt[80]*gpt[84]); /* hossz=bejegyzések száma*bejegyzés mérete */
    setint(crc32a_calc((char*)gpt+512,i),gpt+88);
    /* fejléc */
    i=getint(gpt+12);   /* fejléc mérete */
    setint(0,gpt+16);   /* nullának számolandó */
    setint(crc32a_calc((char*)gpt,i),gpt+16);

    /* ISO9660 cdrom lemezkép */
    if(neediso) {
        if(bbs%4!=0) {
            fprintf(stderr,"mkfs: %s\n",hu?"stage2 nincs 2048 bájt határon":"Stage2 is not 2048 byte sector aligned");
            exit(3);
        }
        sprintf((char*)&isodate, "%04d%02d%02d%02d%02d%02d00",
            ts->tm_year+1900,ts->tm_mon+1,ts->tm_mday,ts->tm_hour,ts->tm_min,ts->tm_sec);
        /* 16. szektor: Elsődleges Kötet Leíró, Primary Volume Descriptor */
        iso[0]=1;   /* Header ID */
        memcpy(&iso[1], "CD001", 5);
        iso[6]=1;   /* verzió */
        for(i=8;i<72;i++) iso[i]=' ';
        memcpy(&iso[40], "OSZ_LIVE_CD", 11);   /* Kötetazonosító, Volume Identifier */
        setinte((65536+es+rs1+rs2+us1+us2+vs+hs+ss+2047)/2048, &iso[80]);
        iso[120]=iso[123]=1;    /* Kötetek száma, Volume Set Size */
        iso[124]=iso[127]=1;    /* Kötetsorszám, Volume Sequence Number */
        iso[129]=iso[130]=8;    /* Logikai blokkméret (0x800) */
        iso[156]=0x22;              /* gyökérkönyvtár rekordméret */
        setinte(20, &iso[158]);     /* gyökérkönyvtár LBA */
        setinte(2048, &iso[166]);   /* gyökérkönyvtár mérete */
        iso[174]=ts->tm_year;       /* gyökérkönyvtár dátuma */
        iso[175]=ts->tm_mon+1;
        iso[176]=ts->tm_mday;
        iso[177]=ts->tm_hour;
        iso[178]=ts->tm_min;
        iso[179]=ts->tm_sec;
        iso[180]=0; /* időzóna UTC (GMT) */
        iso[181]=2;                 /* gyökérkönyvtár jelzők (0=rejtett,1=könyvtár) */
        iso[184]=1;                 /* gyökérkönyvtár sorszám */
        iso[188]=1;                 /* gyökérkönyvtár fájlnév hossza */
        for(i=190;i<813;i++) iso[i]=' ';    /* Kötet adatok */
        memcpy(&iso[318], "OS/Z <HTTPS://BZTSRC.GITLAB.IO/OSZ>", 35);   /* kibocsátó */
        memcpy(&iso[446], "BOOTBOOT", 8);   /* adat előkészítő */
        memcpy(&iso[574], "OS/Z LIVE CD", 12); /* alkalmazás */
        for(i=702;i<813;i++) iso[i]=' ';    /* fájl azonosítók */
        memcpy(&iso[813], &isodate, 16);    /* kötet létrehozási dátuma */
        memcpy(&iso[830], &isodate, 16);    /* kötet módosítási dátuma */
        for(i=847;i<863;i++) iso[i]='0';    /* kötet lejárati dátuma */
        for(i=864;i<880;i++) iso[i]='0';    /* kötet látszólagos dátuma */
        iso[881]=1;                         /* fájlstruktúra verziója */
        for(i=883;i<1395;i++) iso[i]=' ';   /* fájlazonosítók */
        /* 17. szektor: Betöltőkód Leíró, Boot Record Descriptor */
        iso[2048]=0;    /* Header ID */
        memcpy(&iso[2049], "CD001", 5);
        iso[2054]=1;    /* verzió */
        memcpy(&iso[2055], "EL TORITO SPECIFICATION", 23);
        setinte(19, &iso[2048+71]);         /* Boot Catalog LBA */
        /* 18. szektor: Lezáró Leíró, Volume Descritor Terminator */
        iso[4096]=0xFF; /* Header ID */
        memcpy(&iso[4097], "CD001", 5);
        iso[4102]=1;    /* verzió */
        /* 19. szektor: Boot Catalog */
        /* --- BIOS, Validation Entry + Initial/Default Entry --- */
        iso[6144]=1;    /* Header ID, Validation Entry */
        iso[6145]=0;    /* Platform 80x86 */
        iso[6172]=0xaa; /* ellenörző összeg */
        iso[6173]=0x55;
        iso[6174]=0x55; /* azonosító */
        iso[6175]=0xaa;
        iso[6176]=0x88; /* Bootable, Initial/Default Entry */
        iso[6182]=4;    /* Sector Count */
        setint(128/4, &iso[6184]);  /* Boot Record LBA */
        /* --- UEFI, Final Section Header Entry + Section Entry --- */
        iso[6208]=0x91; /* Header ID, Final Section Header Entry */
        iso[6209]=0xEF; /* Platform EFI */
        iso[6210]=1;    /* Number of entries */
        iso[6240]=0x88; /* Bootable, Section Entry */
        setint(128/4, &iso[6248]);  /* ESP Start LBA */
        /* 20. szektor: Gyökérkönyvtár, Root Directory */
        /* . */
        iso[8192]=0x22;              /* rekordméret */
        setinte(20, &iso[8194]);     /* LBA */
        setinte(2048, &iso[8202]);   /* méret */
        iso[8210]=ts->tm_year;       /* dátum */
        iso[8211]=ts->tm_mon+1;
        iso[8212]=ts->tm_mday;
        iso[8213]=ts->tm_hour;
        iso[8214]=ts->tm_min;
        iso[8215]=ts->tm_sec;
        iso[8216]=0; /* időzóna UTC (GMT) */
        iso[8217]=2;                 /* jelzők (0=rejtett,1=könyvtár) */
        iso[8220]=1;                 /* sorszám */
        iso[8224]=1;                 /* fájlnévméret */
        /* .. */
        iso[8226]=0x22;              /* rekordméret */
        setinte(20, &iso[8228]);     /* LBA */
        setinte(2048, &iso[8236]);   /* méret */
        iso[8244]=ts->tm_year;       /* dátum */
        iso[8245]=ts->tm_mon+1;
        iso[8246]=ts->tm_mday;
        iso[8247]=ts->tm_hour;
        iso[8248]=ts->tm_min;
        iso[8249]=ts->tm_sec;
        iso[8250]=0; /* időzóna UTC (GMT) */
        iso[8251]=2;                 /* jelzők (0=rejtett,1=könyvtár) */
        iso[8254]=1;                 /* sorszám */
        iso[8258]=2;                 /* fájlnévméret */
        /* README.TXT */
        iso[8260]=0x22+12;           /* rekordméret */
        setinte(21, &iso[8262]);     /* LBA */
        setinte(125, &iso[8270]);    /* méret */
        iso[8278]=ts->tm_year;       /* dátum */
        iso[8279]=ts->tm_mon+1;
        iso[8280]=ts->tm_mday;
        iso[8281]=ts->tm_hour;
        iso[8282]=ts->tm_min;
        iso[8283]=ts->tm_sec;
        iso[8284]=0; /* időzóna UTC (GMT) */
        iso[8285]=0;                 /* jelzők (0=rejtett,1=könyvtár) */
        iso[8288]=1;                 /* sorszám */
        iso[8292]=12;                /* fájlnévméret */
        memcpy(&iso[8293], "README.TXT;1", 12);
        /* 21. szektor: README.TXT tartalma */
        memcpy(&iso[10240], "OS/Z Live Image\r\n\r\nBootable as\r\n"
            " - CDROM (El Torito, UEFI)\r\n"
            " - USB stick (BIOS, Multiboot, UEFI)\r\n"
            " - SD card (Raspberry Pi 3)", 125);
    }

    f=fopen(diskname,"wb");
    /* (P)MBR */
    fwrite(loader,512,1,f);
    /* GPT fejléc + bejegyzések */
    fwrite(gpt,gs,1,f);
    /* ISO9660 leírók */
    if(neediso)
        fwrite(iso,32768,1,f);
    /* Partíciók */
    if(es>0)    fwrite(esp,es,1,f);
    if(rs1>0)   fwrite(ssp1,rs1,1,f);
    if(rs2>0)   fwrite(ssp2,rs2,1,f);
    if(us1>0)   fwrite(usr1,us1,1,f);
    if(us2>0)   fwrite(usr2,us2,1,f);
    fwrite(var,vs,1,f);
    fwrite(home,hs,1,f);
    fwrite(swap,ss,1,f);

    /* GPT bejegyzések ismét */
    fwrite(gpt+512,gs-512,1,f);
    /* GPT másodlagos fejléc */
    i=getint(gpt+32);
    setint(getint(gpt+24),gpt+32);                     /* másodlagos lba */
    setint(i,gpt+24);                                  /* elsődleges lba */

    setint((ds-gs)/512,gpt+72);                        /* partíció lba */
    i=getint(gpt+12);   /* fejléc mérete */
    setint(0,gpt+16);   /* nullával kell számolni */
    setint(crc32a_calc((char*)gpt,i),gpt+16);
    fwrite(gpt,512,1,f);
    fclose(f);
    return 1;
}

/**
 * fájlrendszer lemezkép létrehozása egy könyvtárstruktúrából
 */
int createimage(char *image,char *dir)
{
    /* initrd-t gyártunk? Az add_file-nak tudnia kell, hogy ne legyenek benne lyukak */
    initrd=(strlen(dir)>7 && !memcmp(dir+strlen(dir)-6,"initrd",6));

    fs=malloc(secsize);
    if(fs==NULL) exit(3);
    size=0;
    /* szuperblokk */
    add_superblock();
    /* gyökérkönyvtár hozzáadása */
    ((FSZ_SuperBlock *)fs)->rootdirfid = add_inode("dir:",FSZ_MIMETYPE_DIR_ROOT);
    ((FSZ_Inode*)(fs+secsize))->numlinks++;
    ((FSZ_Inode*)(fs+secsize))->checksum=crc32a_calc((char*)(((FSZ_Inode*)(fs+secsize))->filetype),1016);

    /* ezt külön adjuk hozzá, mert a legelső futtathatónak kell lennie az initrd-ben */
    if(initrd)
        add_file("sys/core", "../bin/initrd/sys/core");

    /* rekurzívan mindent hozzáadunk a könyvtárból */
    add_dirs(dir,0,0);

    /* összes szektor számának módosítása és ellenörzőösszeg számítás */
    ((FSZ_SuperBlock *)fs)->numsec=size/secsize;
    ((FSZ_SuperBlock *)fs)->freesec=size/secsize;
    ((FSZ_SuperBlock *)fs)->checksum=crc32a_calc((char *)((FSZ_SuperBlock *)fs)->magic,508);

    /* új lemezkép kiírása */
    f=fopen(image,"wb");
    fwrite(fs,size,1,f);
    fclose(f);
    return 1;
}

/**
 * könyvtár vagy unió listázása egy lemezképben
 */
void ls(int argc, char **argv)
{
    unsigned char *data=readfileall(argv[1]);
    FSZ_DirEntHeader *hdr;
    FSZ_Inode *dir;
    FSZ_DirEnt *ent;
    unsigned int cnt=0;
    if(data==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    while(((FSZ_SuperBlock *)data)->enchash!=0) {
        printf(hu?"Titkosított lemezkép. ":"Encrypted image. ");
        decrypt(data,read_size,NULL);
    }
    if(argc<4) {
        dir=(FSZ_Inode*)(data+(((FSZ_SuperBlock *)data)->rootdirfid)*secsize);
    } else
        dir = locate(data,0,argv[3]);

    if(dir==NULL) { fprintf(stderr,"mkfs: %s\n",
        hu?"nem találom az elérési utat a lemezképben":"Unable to find path in image"); exit(2); }
    if(!memcmp(dir->filetype,FSZ_FILETYPE_UNION,4)){
        char *c=((char*)dir+1024);
        printf(hu?"A következők uniója:\n":"Union list of:\n");
        while(*c!=0) {
            printf("  %s\n",c);
            while(*c!=0) c++;
            c++;
        }
        return;
    }
    if(memcmp(dir->filetype,FSZ_FILETYPE_DIR,4)) { fprintf(stderr,"mkfs: %s\n",hu?"nem könyvtár":"not a directory"); exit(2); }
    hdr=(FSZ_DirEntHeader *)((char*)dir+1024);
    ent=(FSZ_DirEnt *)hdr; ent++;
    while(ent->fid!=0 && cnt<((secsize-1024)/128-1)) {
        FSZ_Inode *in = (FSZ_Inode *)((char*)data+ent->fid*secsize);
        printf("  %c%c%c%c %6ld %6ld %s\n",
            in->filetype[0],in->filetype[1],in->filetype[2],in->filetype[3],
            in->sec, in->size, ent->name);
        ent++; cnt++;
    }
}

/**
 * fájl tartalmának kinyerése egy lemezképből
 */
void cat(int argc, char **argv)
{
    unsigned char *data=readfileall(argv[1]);
    FSZ_Inode *file;
    if(argc<1 || data==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    while(((FSZ_SuperBlock *)data)->enchash!=0) {
        printf(hu?"Titkosított lemezkép. ":"Encrypted image. ");
        decrypt(data,read_size,NULL);
    }
    file = locate(data,0,argv[3]);
    if(file==NULL) { fprintf(stderr,"mkfs: %s\n",
        hu?"nem találom az elérési utat a lemezképben":"Unable to find path in image"); exit(2); }
    switch(FSZ_FLAG_TRANSLATION(file->flags)) {
        case FSZ_IN_FLAG_INLINE:
            /* beágyazott adat */
            fwrite((char*)file + 1024,file->size,1,stdout);
            break;
        case FSZ_IN_FLAG_SECLIST:
        case FSZ_IN_FLAG_SDINLINE:
            /* beágyazott szektorkönyvtár vagy szektorlista */
            fwrite(data + *((uint64_t*)&file->data.small.inlinedata)*secsize,file->size,1,stdout);
            break;
        case FSZ_IN_FLAG_DIRECT:
            /* közvetlen adat */
            fwrite(data + file->sec*secsize,file->size,1,stdout);
            break;
        /* szektorkönyvtár (csak egy szintet támogatunk, és nem kezeljük a lyukakat itt) */
        case FSZ_IN_FLAG_SECLIST0:
        case FSZ_IN_FLAG_SD:
            fwrite(data + *((uint64_t*)(data + file->sec*secsize))*secsize,file->size,1,stdout);
            break;
        default:
            fprintf(stderr,"mkfs: %s %x\n",
                hu?"nem támogatott leképezés":"Unsupported translation",FSZ_FLAG_TRANSLATION(file->flags));
            break;
    }
}

/**
 * fájl mime típusának megváltoztatása a lemezképben
 */
void changemime(int argc, char **argv)
{
    unsigned char *data=readfileall(argv[1]);
    FSZ_Inode *in=NULL;
    if(data==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    if(((FSZ_SuperBlock *)fs)->enchash!=0) { fprintf(stderr,"mkfs: %s\n",
        hu?"titkosított lemezkép nem módosítható.":"Cannot modify an encrypted image."); exit(2); }
    if(argc>=4)
        in = locate(data,0,argv[3]);
    if(in==NULL) { fprintf(stderr,"mkfs: %s\n",
        hu?"nem találom az elérési utat a lemezképben":"Unable to find path in image"); exit(2); }
    if(argc<5) {
        printf("%c%c%c%c/%s\n",
            in->filetype[0],in->filetype[1],in->filetype[2],in->filetype[3],in->mimetype);
    } else {
        char *c=argv[4];
        while(*c!=0&&*c!='/')c++;
        if(in->filetype[3]==':') { fprintf(stderr,"mkfs: %s\n",hu?"speciális inode mime typusa nem változtatható":
            "Unable to change mime type of special inode"); exit(2); }
        if(*c!='/') { fprintf(stderr,"mkfs: %s '%s'\n", hu?"Hibás mime típus":"Bad mime type", argv[4]); exit(2); } else c++;
        memcpy(in->filetype,argv[4],4);
        memset(in->mimetype,0,36);
        memcpy(in->mimetype,c,strlen(c)<36?strlen(c):36);
        in->checksum=crc32a_calc((char*)in->filetype,1016);
        /* új lemezkép kiírása */
        f=fopen(argv[1],"wb");
        fwrite(data,read_size,1,f);
        fclose(f);
    }
}

/**
 * unió hozzáadása a lemezképhez
 */
void addunion(int argc, char **argv)
{
    int i;
    char items[BUFSIZ];
    char *c=(char*)&items;
    memset(&items,0,sizeof(items));
    for(i=4;i<argc;i++) {
        memcpy(c,argv[i],strlen(argv[i]));
        c+=strlen(argv[i])+1;
    }
    fs=readfileall(argv[1]); size=read_size;
    if(fs==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    if(((FSZ_SuperBlock *)fs)->enchash!=0) { fprintf(stderr,"mkfs: %s\n",
        hu?"titkosított lemezkép nem módosítható.":"Cannot modify an encrypted image."); exit(2); }
    i=add_inode(FSZ_FILETYPE_UNION,(char*)&items);
    memset(&items,0,sizeof(items));
    memcpy(&items,argv[3],strlen(argv[3]));
    if(items[strlen(argv[3])-1]!='/')
        items[strlen(argv[3])]='/';
    link_inode(i,(char*)&items,0);
    /* új lemezkép kiírása */
    f=fopen(argv[1],"wb");
    fwrite(fs,size,1,f);
    fclose(f);
}

/**
 * új hard link hozzáadása a lemezképhez
 */
void addlink(int argc, char **argv)
{
    FSZ_Inode *file;
    fs=readfileall(argv[1]); size=read_size;
    if(argc<1 || fs==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    if(((FSZ_SuperBlock *)fs)->enchash!=0) { fprintf(stderr,"mkfs: %s\n",
        hu?"titkosított lemezkép nem módosítható.":"Cannot modify an encrypted image."); exit(2); }
    file = locate(fs,0,argv[4]);
    if(file==NULL) { fprintf(stderr,"mkfs: %s\n",
        hu?"nem találom a cél elérési utat a lemezképben":"Unable to find target in image"); exit(2); }
    link_inode(((uint64_t)file-(uint64_t)fs)/secsize,argv[3],0);
    /* új lemezkép kiírása */
    f=fopen(argv[1],"wb");
    fwrite(fs,size,1,f);
    fclose(f);
}

/**
 * új szimbólikus hivatkozás hozzáadása a lemezképhez
 */
void addsymlink(int argc, char **argv)
{
    int i;
    fs=readfileall(argv[1]); size=read_size;
    if(argc<1 || fs==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    if(((FSZ_SuperBlock *)fs)->enchash!=0) { fprintf(stderr,"mkfs: %s\n",
        hu?"titkosított lemezkép nem módosítható.":"Cannot modify an encrypted image."); exit(2); }
    i=add_inode(FSZ_FILETYPE_SYMLINK,argv[4]);
    link_inode(i,argv[3],0);
    /* új lemezkép kiírása */
    f=fopen(argv[1],"wb");
    fwrite(fs,size,1,f);
    fclose(f);
}


/**
 * Option ROM gyártása initrd lemezképből
 */
void initrdrom(int argc, char **argv)
{
    int i;
    unsigned char *buf, c=0;
    fs=readfileall(argv[1]); size=((read_size+32+511)/512)*512;
    if(argc<1 || fs==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    buf=(unsigned char*)malloc(size+1);
    /* Option ROM fejléc */
    buf[0]=0x55; buf[1]=0xAA; buf[2]=(read_size+32+511)/512;
    /* asm "xor ax,ax; retf" */
    buf[3]=0x31; buf[4]=0xC0; buf[5]=0xCB;
    /* azonosító, méret és adat */
    memcpy(buf+8,"INITRD",6);
    memcpy(buf+16,&read_size,4);
    memcpy(buf+32,fs,read_size);
    /* ellenörző összeg */
    for(i=0;i<size;i++) c+=buf[i];
    buf[6]=(unsigned char)((int)(256-c));
    /* új lemezkép kiírása */
    f=fopen(argc>2&&argv[3]!=NULL?argv[3]:"initrd.rom","wb");
    fwrite(buf,size,1,f);
    fclose(f);
}

/**
 * lemezkép szektorainak listázása C struct-ként
 */
void dump(int argc, char **argv)
{
    unsigned int i=argv[3]!=NULL?atoi(argv[3]):0,n;
    uint64_t j,*k,l,secs=secsize;
    char unit='k',*c;
    FSZ_SuperBlock *sb;
    FSZ_DirEntHeader *hdr;
    FSZ_DirEnt *d;
    fs=readfileall(argv[1]);
    if(argc<1 || fs==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
    sb=(FSZ_SuperBlock *)fs;
    hdr=(FSZ_DirEntHeader*)(fs+i*secsize);
    if(memcmp(&sb->magic, FSZ_MAGIC, 4) || memcmp(&sb->magic2, FSZ_MAGIC, 4)) {
        fprintf(stderr,"mkfs: %s\n",hu?"Nem FS/Z lemezkép, hibás azonosító.":"Not an FS/Z image, bad magic."); exit(2);
    }
    secs=1<<(sb->logsec+11);
    if(i*secs>(uint64_t)read_size) { fprintf(stderr,hu?"mkfs: nem találom a %d. szektort (max %ld)\n":
        "mkfs: Unable to locate %d sector (max %ld)\n",i,read_size/secs); exit(2); }
    while(i>0 && sb->enchash!=0) {
        printf(hu?"Titkosított lemezkép. ":"Encrypted image. ");
        decrypt(fs,read_size,NULL);
    }
    printf("  ---------------- %s #%d ----------------\n",hu?"Szektor dump":"Dumping sector",i);
    if(i==0) {
        printf("FSZ_SuperBlock {\n");
        printf("\tmagic: %s\n",FSZ_MAGIC);
        printf("\tversion_major.version_minor: %d.%d\n",sb->version_major,sb->version_minor);
        printf("\tflags: 0x%02x %s %s %s\n",sb->flags, sb->flags&FSZ_SB_FLAG_BIGINODE?"FSZ_SB_FLAG_BIGINODE":"",
            sb->flags&FSZ_SB_JOURNAL_DATA?"FSZ_SB_JOURNAL_DATA":"",sb->flags&FSZ_SB_EALG_AESCBC?"FSZ_SB_EALG_AESCBC":
            "FSZ_SB_EALG_SHACBC");
        printf("\traidtype: 0x%02x %s\n",sb->raidtype,sb->raidtype==FSZ_SB_SOFTRAID_NONE?"none":"");
        printf("\tlogsec: %d (%ld %s)\n",sb->logsec,secs,hu?"bájt szektoronként":"bytes per sector");
        printf("\tphysec: %d (%ld %s)\n",sb->physec,secs/sb->physec,hu?"bájt szektoronként":"bytes per sector");
        printf("\tcurrmounts / maxmounts: %d / %d\n",sb->currmounts,sb->maxmounts);
        j=sb->numsec*(1<<(sb->logsec+11))/1024;
        if(j>1024) { j/=1024; unit='M'; } printf("\tnumsec: %ld (%s %ld %c%s)\n",sb->numsec,
            hu?"teljes kapacitás":"total capacity",j,unit,hu?"bájt":"bytes");
        printf("\tfreesec: %ld (%ld%% %s)\n",sb->freesec,
            (sb->numsec-sb->freesec)*100/sb->numsec,hu?"szabad, a szabad lyukak nélkül":"free, not including free holes");
        printf("\trootdirfid: LSN %ld\n",sb->rootdirfid);
        printf("\tfreesecfid: LSN %ld %s\n",sb->freesecfid,sb->freesecfid==0?(hu?"(nincs szabad lyuk)":"(no free holes)"):"");
        printf("\tbadsecfid: LSN %ld\n",sb->badsecfid);
        printf("\tindexfid: LSN %ld\n",sb->indexfid);
        printf("\tmetafid: LSN %ld\n",sb->metafid);
        printf("\tjournalfid: LSN %ld (%s %ld, %s %ld, max %ld)\n",sb->journalfid, hu?"eleje":"start",sb->journalhead,
            hu?"vége":"end",sb->journaltail,sb->journalmax);
        printf("\tenchash: 0x%08x %s %s\n",sb->enchash, sb->enchash==0?(hu?"nincs":"none"):(hu?"titkosított":"encrypted"),
            sb->enchash==0?"":(sb->flags&FSZ_SB_EALG_AESCBC?"(AES-256-CBC)":"(SHA-XOR-CBC)"
        ));
        j=sb->createdate/1000000; printf("\tcreatedate: %ld %s",sb->createdate,ctime((time_t*)&j));
        j=sb->lastmountdate/1000000; printf("\tlastmountdate: %ld %s",sb->lastmountdate,ctime((time_t*)&j));
        j=sb->lastcheckdate/1000000; printf("\tlastcheckdate: %ld %s",sb->lastcheckdate,ctime((time_t*)&j));
        j=sb->lastchangedate/1000000; printf("\tlastchangedate: %ld %s",sb->lastchangedate,ctime((time_t*)&j));
        printf("\tuuid: "); printf_uuid((FSZ_Access*)&sb->uuid,0); printf(" (%s)\n",
            hu?"egyedi kötet azonosító":"volume unique identifier");
        printf("\towner: "); printf_uuid((FSZ_Access*)&sb->owner,1); printf("\n");
        printf("\tmagic2: %s\n",FSZ_MAGIC);
        n=crc32a_calc((char *)sb->magic,508);
        printf("\tchecksum: 0x%04x %s (0x%04x)\n",sb->checksum,
            sb->checksum==n?(hu?"helyes":"correct"):(hu?"hibás":"invalid"),n);
        printf("\traidspecific: %s\n",sb->raidspecific);
        printf("};\n");
    } else
    if(!memcmp(fs+i*secs,FSZ_IN_MAGIC,4)) {
        FSZ_Inode *in=(FSZ_Inode*)(fs+i*secs);
        printf("FSZ_Inode {\n");
        printf("\tmagic: %s\n",FSZ_IN_MAGIC);
        n = crc32a_calc((char*)in->filetype,1016);
        printf("\tchecksum: 0x%04x %s (0x%04x)\n",in->checksum,
            in->checksum==n?(hu?"helyes":"correct"):(hu?"hibás":"invalid"),n);
        printf("\tfiletype: \"%c%c%c%c\", mimetype: \"%s\"\n",in->filetype[0],in->filetype[1],
            in->filetype[2],in->filetype[3],in->mimetype);
        printf("\tenchash: 0x%08x %s %s\n",in->enchash, in->enchash==0?(hu?"titkosítatlan":"not encrypted"):
            (hu?"titkosított":"encrypted"),in->enchash==0?"":(in->flags&FSZ_IN_EALG_AESCBC?"(AES-256-CBC)":"(SHA-XOR-CBC)"
        ));
        j=in->changedate/1000000; printf("\tchangedate: %ld %s",in->changedate,ctime((time_t*)&j));
        j=in->accessdate/1000000; printf("\taccessdate: %ld %s",in->accessdate,ctime((time_t*)&j));
        printf("\tnumblocks: %ld\n\tnumlinks: %ld\n",in->numblocks,in->numlinks);
        printf("\tmetalabel: LSN %ld\n",in->metalabel);
        printf("\tFSZ_Version {\n");
        printf("\t\tsec: LSN %ld %s\n",in->sec,in->sec==0?(hu?"lyuk":"sparse"):(in->sec==i?(hu?"önhivatkozás (beágyazott)":
            "self-reference (inlined)"):""));
        printf("\t\tsize: %ld bytes\n",in->size);
        j=in->modifydate/1000000; printf("\t\tmodifydate: %ld %s",in->modifydate,ctime((time_t*)&j));
        printf("\t\tflags: 0x%08lx ",in->flags);
        switch(FSZ_FLAG_TRANSLATION(in->flags)){
            case FSZ_IN_FLAG_INLINE: printf("FSZ_IN_FLAG_INLINE"); break;
            case FSZ_IN_FLAG_DIRECT: printf("FSZ_IN_FLAG_DIRECT"); break;
            case FSZ_IN_FLAG_SECLIST: printf("FSZ_IN_FLAG_SECLIST"); break;
            case FSZ_IN_FLAG_SECLIST0: printf("FSZ_IN_FLAG_SECLIST0"); break;
            case FSZ_IN_FLAG_SECLIST1: printf("FSZ_IN_FLAG_SECLIST1"); break;
            case FSZ_IN_FLAG_SECLIST2: printf("FSZ_IN_FLAG_SECLIST2"); break;
            case FSZ_IN_FLAG_SECLIST3: printf("FSZ_IN_FLAG_SECLIST3"); break;
            case FSZ_IN_FLAG_SDINLINE: printf("FSZ_IN_FLAG_SDINLINE"); break;
            case FSZ_IN_FLAG_SD: printf("FSZ_IN_FLAG_SD"); break;
            default: printf("FSZ_IN_FLAG_SD%d",FSZ_FLAG_TRANSLATION(in->flags)); break;
        }
        printf(" %s %s\n\t};\n",in->flags&FSZ_IN_FLAG_HIST?"FSZ_IN_FLAG_HIST":"",in->enchash?(
            in->flags&FSZ_IN_EALG_AESCBC?"FSZ_IN_EALG_AESCBC":"FSZ_IN_EALG_SHACBC"):"");
        printf("\towner: "); printf_uuid(&in->owner,1);
        printf("\n\tinlinedata: ");
        if(FSZ_FLAG_TRANSLATION(in->flags)==FSZ_IN_FLAG_SDINLINE)
            printf(hu?"szektorkönyvtár (sd)\n":"sector directory\n");
        else
        if(FSZ_FLAG_TRANSLATION(in->flags)==FSZ_IN_FLAG_SECLIST) {
            printf(hu?"szektorlista (sl)\n":"sector list\n");
        } else
        if(!memcmp(in->filetype,FSZ_FILETYPE_SYMLINK,4))
            printf("%s\n\t  '%s'",hu?"szimbólikus hivatkozás cél":"symlink target",in->data.small.inlinedata);
        else
        if(!memcmp(in->filetype,FSZ_FILETYPE_UNION,4)) {
            printf(hu?"unió\n":"union list\n");
            c=(char*)in->data.small.inlinedata;
            while(*c!=0) {
                printf("\t  '%s'\n",c);
                while(*c!=0) c++;
                c++;
            }
        } else
        if(!memcmp(in->data.small.inlinedata,FSZ_DIR_MAGIC,4))
            printf(hu?"könyvtár bejegyzések\n":"directory entries\n");
        else {
            printf("\n");
            for(j=0;j<8;j++) {
                printf("\t  ");
                for(l=0;l<16;l++) printf("%02x %s",in->data.small.inlinedata[j*16+l],l%4==3?" ":"");
                for(l=0;l<16;l++) printf("%c",in->data.small.inlinedata[j*16+l]>=30&&
                    in->data.small.inlinedata[j*16+l]<127?in->data.small.inlinedata[j*16+l]:'.');
                printf("\n");
            }
            printf("\t  ... etc.\n");
        }
        printf("\n};\n");
        printf(hu?"Adat szektorok: ":"Data sectors: ");
        switch(FSZ_FLAG_TRANSLATION(in->flags)){
            case FSZ_IN_FLAG_INLINE: printf("%ld", in->sec); break;
            case FSZ_IN_FLAG_DIRECT: printf("%ld", in->sec); break;
            case FSZ_IN_FLAG_SECLIST: printf("sl"); break;
            case FSZ_IN_FLAG_SECLIST0: printf("%ld sl", in->sec); break;
            case FSZ_IN_FLAG_SECLIST1: printf("sd %ld [ sl ]", in->sec); break;
            case FSZ_IN_FLAG_SECLIST2: printf("sd %ld [ sd [ sl ]]", in->sec); break;
            case FSZ_IN_FLAG_SECLIST3: printf("sd %ld [ sd [ sd [ sl ]]]", in->sec); break;
            case FSZ_IN_FLAG_SDINLINE:
                printf(hu?" beágyazott [":" inline [");
                k=(uint64_t*)&in->data.small.inlinedata;
                l=(secs-1024)/16;
                goto sddump;
            default:
                printf("sd %ld [", in->sec);
                k=(uint64_t*)(fs+in->sec*secs);
                l=secs/16;
sddump:         while(l>1 && k[l*2-1]==0 && k[l*2-2]==0) l--;
                for(j=0;j<l;j++) {
                    if(FSZ_FLAG_TRANSLATION(in->flags)==FSZ_IN_FLAG_SD ||
                       FSZ_FLAG_TRANSLATION(in->flags)==FSZ_IN_FLAG_SDINLINE)
                        printf(" %ld",*k);
                    else
                        printf(" sd %ld [...]",*k);
                    k+=2;
                }
                printf(" ]");
                break;
        }
        printf("\n");
        if(!memcmp(in->data.small.inlinedata,FSZ_DIR_MAGIC,4)) {
            hdr=(FSZ_DirEntHeader*)in->data.small.inlinedata;
            printf("\n");
            goto dumpdir;
        }
    } else
    if(!memcmp(fs+i*secs,FSZ_DIR_MAGIC,4)) {
dumpdir:
        d=(FSZ_DirEnt *)hdr;
        printf("FSZ_DirEntHeader {\n");
        printf("\tmagic: %s\n",FSZ_DIR_MAGIC);
        printf("\tchecksum: %04x\n",hdr->checksum);
        printf("\tflags: 0x%04lx %s %s\n",hdr->flags,hdr->flags&FSZ_DIR_FLAG_UNSORTED?"FSZ_DIR_FLAG_UNSORTED":"",
            hdr->flags&FSZ_DIR_FLAG_HASHED?"FSZ_DIR_FLAG_HASHED":"");
        printf("\tnumentries: %ld\n",hdr->numentries);
        printf("};\n");
        for(j=0;j<hdr->numentries;j++) {
            d++;
            printf("FSZ_DirEnt {\n");
            printf("\tfid: LSN %ld\n",d->fid);
            printf("\tlength: %d unicode %s\n",d->length,hu?"karakter":"characters");
            printf("\tname: \"%s\"\n",d->name);
            printf("};\n");
        }
    } else {
        k=(uint64_t*)(fs+i*secs); l=1;
        for(j=0;j<16;j++) {
            k++;
            if(*k!=0) { l=0; break; }
            k++;
        }
        if(l) {
            printf(hu?"Valószínűleg szektorkönyvtár vagy szektorlista\n":"Probably Sector Directory or Sector List\n");
            k=(uint64_t*)(fs+i*secs);
            l=secs/32; while(l>1 && k[l*4-1]==0 && k[l*4-2]==0 && k[l*4-3]==0 && k[l*4-4]==0) l--;
            for(j=0;j<l;j++) {
                printf("\tLSN %ld\t\t(LSN/%s) %ld%s",*k,hu?"méret":"size",*(k+2),j%2==0?"\t\t":"\n");
                k+=4;
            }
            printf("\n");
        } else {
            printf(hu?"Fájl adat\n":"File data\n");
            c=(char*)(fs+i*secs);
            for(j=0;j<8;j++) {
                printf("  ");
                for(l=0;l<16;l++) printf("%02x %s",(unsigned char)(c[j*16+l]),l%4==3?" ":"");
                for(l=0;l<16;l++) printf("%c",(unsigned char)(c[j*16+l]>=30&&c[j*16+l]<127?c[j*16+l]:'.'));
                printf("\n");
            }
            printf("  ... %s.\n",hu?"stb":"etc");
        }
    }
}

/**
 * Nem igazán FS/Z-hez kapcsolódó, de jobb hijján ide került. Nyelvi fájlokat formáz, amik
 * a lemezképbe kerülnek. 16 bájtos fejléc után nullával lezárt sztringek következnek.
 */
void createlang(char *srcdir,char *dstdir)
{
    DIR *dir;
    FILE *f;
    struct dirent *ent;
    char *full=malloc(4096), *data, *s;
    int l,siz;
    if(srcdir==NULL || dstdir==NULL) return;
    mkdir(dstdir,0755);
    if ((dir = opendir (srcdir)) != NULL) {
      while ((ent = readdir (dir)) != NULL) {
        if(!strcmp(ent->d_name,".")||!strcmp(ent->d_name,".."))
            continue;
        sprintf(full,"%s/%s",srcdir,ent->d_name);
        s=data=(char*)readfileall(full);
        l=1; while(*s) { if(*s=='\n') { *s=0; if(*(s+1)) l++; } s++; }
        if(*(s-1)==0 && *(s-2)==0) s--;
        sprintf(full,"%s/%s",dstdir,ent->d_name);
        f=fopen(full,"wb");
        fwrite("LANG",4,1,f);
        siz=(s-data)+16;
        fwrite(&siz,4,1,f);
        siz=crc32a_calc(data, (s-data));
        fwrite(&siz,4,1,f);
        fwrite(&l,4,1,f);
        fwrite(data,(s-data),1,f);
        fclose(f);
      }
      closedir (dir);
    }
    free(full);
}

/**
 * belépési pont, fő függvény
 */
int main(int argc, char **argv)
{
    char *path=strdup(argv[0]),*lang=getenv("LANG");
    int i;
    /* aktuális időbélyeg lekérése */
    ts = (long int)time(NULL) * 1000000;
    if(lang && lang[0]=='h' && lang[1]=='u') hu=1;

    checkcompilation();

    /* fájlnevek összeszerkesztése */
    for(i=strlen(path);i>0;i--) {if(path[i-1]=='/') break;}
    memcpy(path+i,"../bin/",8); i+=8; path[i]=0;
    stage1=malloc(i+16); sprintf(stage1,"%s/../loader/boot.bin",path);
    sys1file=malloc(i+16); sprintf(sys1file,"%ssys.x86_64.part",path);
    sys2file=malloc(i+16); sprintf(sys2file,"%ssys.aarch64.part",path);
    espfile=malloc(i+16); sprintf(espfile,"%sesp.part",path);
    usr1file=malloc(i+16); sprintf(usr1file,"%susr.x86_64.part",path);
    usr2file=malloc(i+16); sprintf(usr2file,"%susr.aarch64.part",path);
    varfile=malloc(i+16); sprintf(varfile,"%svar.part",path);
    homefile=malloc(i+16); sprintf(homefile,"%shome.part",path);

    /* üres szektor kreálása */
    emptysec=(char*)malloc(secsize);
    if(emptysec==NULL) { fprintf(stderr,hu?"mkfs: nem tudok allokálni %ld bájtnyi memóriát\n":
            "mkfs: Unable to allocate %ld memory\n",(unsigned long int)secsize); exit(1); }
    memset(emptysec,0,secsize);

    /* kapcsolók és paraméterek értelmezése */
    if(argv[1]==NULL||!strcmp(argv[1],"help")) {
        printf( "FS/Z mkfs %s - Copyright (c) 2017 CC-by-nc-sa-4.0 bztsrc@gitlab\n\n"
                "./mkfs (imagefile) (createfromdir)           - %s\n"
                "./mkfs (imagefile) initrdrom (romfile)       - %s\n"
                "./mkfs (imagefile) union (path) (members...) - %s\n"
                "./mkfs (imagefile) symlink (path) (target)   - %s\n"
                "./mkfs (imagefile) link (path) (target)      - %s\n",
                hu?"segédprogram":"utility",
                hu?"új FS/Z lemezkép készítése egy könyvtárból":"create new FS/Z image file of a directory",
                hu?"Option ROM készítése lemezképből":"create Option ROM from file",
                hu?"unió hozzáadása a lemezképhez":"add an union directory to image file",
                hu?"szimbolikus hivatkozás hozzáadása":"add a symbolic link to image file",
                hu?"hard link hozzáadása a lemezképhez":"add a hard link to image file");
        printf( "./mkfs (imagefile) mime (path) (mimetype)    - %s\n"
                "./mkfs (imagefile) ls (path)                 - %s\n"
                "./mkfs (imagefile) cat (path)                - %s\n"
                "./mkfs (imagefile) dump (lsn)                - %s\n"
                "./mkfs (imagefile) encrypt (algo) (password) - %s\n",
                hu?"fájl mimetípusának módosítása a lemezképben":"change the mime type of a file in image file",
                hu?"könyvtár kilistázása a lemezképből":"parse FS/Z image and list contents",
                hu?"fájl tartalmának kinyerése lemezképből":"parse FS/Z image and return file content",
                hu?"szektor dumpolása C header formátumba":"dump a sector in image into C header format",
                hu?"lemezkép titkosítása (sha / aes) algoritmussal":"encrypt whole filesystem image using algo (sha / aes)");
        printf( "./mkfs (imagefile) decrypt (password)        - %s\n"
                "./mkfs (imagefile) chcrypt (old) (new)       - %s\n"
                "./mkfs disk (ddfile)                         - %s\n"
                "./mkfs diskiso (ddfile)                      - %s\n"
                "\n%s\n",
                hu?"titkosított lemezkép visszafejtése":"decrypt filesystem image",
                hu?"jelszócsere újratitkosítás nélkül":"change password without re-encrypting image",
                hu?"teljes GPT lemez összeállítása OS/Z partíciós lemezképekből":"assemble OS/Z partition images into one GPT disk",
                hu?"teljes lemez összeállítása, de ISO9660 leírókkal":"assemble disk, but add ISO9660 descriptors too",
                hu?"MEGJEGYZÉS az FS/Z lemezformátuma szabad közkincs, de ez a segédprogram nem az.":
                    "NOTE the FS/Z on disk format is Public Domain, but this utility is not.");
        exit(0);
    }
    if(!strcmp(argv[1],"disk")) {
        diskname=malloc(i+64); sprintf(diskname,"%s%s",path,argv[2]!=NULL?argv[2]:"disk.img");
        createdisk(0);
    } else
    if(!strcmp(argv[1],"diskiso")) {
        diskname=malloc(i+64); sprintf(diskname,"%s%s",path,argv[2]!=NULL?argv[2]:"disk.img");
        createdisk(1);
    } else
    if(!strcmp(argv[1],"lang")) {
        createlang(argv[2],argv[3]);
    } else
    if(argc>2){
        if(!strcmp(argv[2],"initrdrom")) {
            initrdrom(argc,argv);
        } else
        if(!strcmp(argv[2],"union") && argc>3) {
            addunion(argc,argv);
        } else
        if(!strcmp(argv[2],"symlink") && argc>3) {
            addsymlink(argc,argv);
        } else
        if(!strcmp(argv[2],"link") && argc>3) {
            addlink(argc,argv);
        } else
        if(!strcmp(argv[2],"mime") && argc>3) {
            changemime(argc,argv);
        } else
        if(!strcmp(argv[2],"cat") && argc>2) {
            cat(argc,argv);
        } else
        if(!strcmp(argv[2],"ls") && argc>=2) {
            ls(argc,argv);
        } else
        if(!strcmp(argv[2],"dump") && argc>=2) {
            dump(argc,argv);
        } else
        if(!strcmp(argv[2],"encrypt") && argc>=2) {
            encrypt(argc,argv);
        } else
        if(!strcmp(argv[2],"decrypt") && argc>=2) {
            fs=readfileall(argv[1]); size=read_size;
            if(fs==NULL) { fprintf(stderr,"mkfs: %s %s\n",hu?"Nem sikerült betölteni:":"Unable to load",argv[1]); exit(2); }
            if(((FSZ_SuperBlock *)fs)->enchash!=0) { fprintf(stderr,"mkfs: %s\n",hu?"Nincs titkosítva a lemezkép.":
                "Not an encrypted image."); exit(2); }
            decrypt(fs,size,argv[3]);
            /* új lemezkép kiírása */
            f=fopen(argv[1],"wb");
            fwrite(fs,size,1,f);
            fclose(f);
        } else
        if(!strcmp(argv[2],"chcrypt") && argc>=2) {
            chcrypt(argc,argv);
        } else
            createimage(argv[1],argv[2]);
    } else {
        fprintf(stderr,"mkfs: %s\n",hu?"ismeretlen parancs.":"Unknown command.");
    }
    return 0;
}
