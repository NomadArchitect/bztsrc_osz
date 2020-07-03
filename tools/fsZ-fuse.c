/*
 * tools/fsZ-fuse.c
 *
 * Copyright (c) 2019 bzt (bztsrc@gitlab)
 *
 * Ez a program szabad szoftver; terjeszthető illetve módosítható a Free Software
 * Fundation által kiadott GNU General Public License dokumentumban leírtak;
 * akár a licensz 3-as, akár (tetszőleges) későbbi változata szerint.
 *
 * Ez a program abban a reményben kerül közreadásra, hogy hasznos lesz, de minden
 * egyéb GARANCIA NÉLKÜL, az ELADHATÓSÁGRA vagy VALAMELY CÉLRA VALÓ
 * ALKALMAZHATÓSÁGRA való származtatott garanciát is beleértve. További
 * részleteket a GNU General Public License tartalmaz.
 *
 * A felhasználónak a programmal együtt meg kell kapnia a GNU General Public
 * License egy példányát; ha mégsem kapta meg, akkor tekintse meg a
 * <http://www.gnu,org/licenses/> oldalon.
 *
 * @subsystem eszközök
 * @brief FUSE meghajtó FS/Z fájlrendszer felcsatolásához Linux alatt
 *
 * Ez egy minimális implementáció, a FS/Z specifikációhoz képest rengeteg megszorítást
 * tartalmaz. Például csak 24 bejegyzést kezel könyvtáranként (csak inode-ba ágyazott bejegyzések),
 * és nem kezel titkosított lemezképeket. Kevés indirekciót kezel. Egyelőre csak olvasni lehet vele.
 */

#define FUSE_USE_VERSION    29

#include <fuse.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#define public
#include "../include/osZ/fsZ.h"
#include "../src/libc/crc32.h"

#define EX_FAILURE 1

int hu=0;
unsigned long int lastino = 0;

typedef struct {
    int devfile;
    unsigned int secsize;
    char fsZuser[16];
    FSZ_SuperBlock sb;
    FSZ_Inode in;
    FSZ_SDEntry *sd;
    char *data;
} fsZ_data_t;
#define get_fsZ_context fsZ_data_t *fsZ_data = (fsZ_data_t*)(fuse_get_context()->private_data)

/*** segédfüggvények ***/
FSZ_Inode *locate(int inode, const char *path)
{
    unsigned int ns=0,cnt=0;
    FSZ_DirEntHeader *hdr;
    FSZ_DirEnt *ent;
    get_fsZ_context;

    if(!inode) inode = fsZ_data->sb.rootdirfid;
    if(path && path[0]=='/') path++;
    lastino = inode;
    lseek(fsZ_data->devfile, inode*fsZ_data->secsize, SEEK_SET);
    if(read(fsZ_data->devfile, &fsZ_data->in, fsZ_data->secsize) == -1 || memcmp(&fsZ_data->in.magic, FSZ_IN_MAGIC, 4))
        return NULL;
    if(path==NULL || path[0]==0 || path[0]==';' || memcmp(fsZ_data->in.filetype, FSZ_FILETYPE_DIR, 4))
        return &fsZ_data->in;

    /* ez csak inode-ba ágyazott könyvtárlistát támogat egyelőre */
    hdr=(FSZ_DirEntHeader *)(fsZ_data->sb.flags & FSZ_SB_FLAG_BIGINODE?
        fsZ_data->in.data.big.inlinedata : fsZ_data->in.data.small.inlinedata);
    if(memcmp(hdr->magic, FSZ_DIR_MAGIC, 4)) return &fsZ_data->in;
    ent=(FSZ_DirEnt *)hdr; ent++;
    while(path[ns]!='/'&&path[ns]!=';'&&path[ns]!=0) ns++;
    while(ent->fid!=0 && cnt<hdr->numentries) {
        if(!strncmp((char *)(ent->name),path,ns) &&
            (path[ns]==0||path[ns]=='/'||path[ns]==';') && (ent->name[ns]==0||ent->name[ns]=='/'))
            return locate(ent->fid,path+ns);
        ent++; cnt++;
    }
    return NULL;
}

uid_t fsZuser2unixuser(const char *username)
{
    char user[16];
    struct passwd *pw;

    strncpy(user, username, 15);
    pw = getpwnam(user);
    return pw? pw->pw_uid : 0;
}

void unixuser2fsZuser(char *username)
{
    struct passwd *pw;

    memset(username, 0, 16);
    pw = getpwuid(getuid());
    if(pw)
        strncpy(username, pw->pw_name, 15);
}

/*** FS/Z implementáció ***/
int fsZ_getattr(const char *path, struct stat *st)
{
    FSZ_Inode *in = locate(0, path);
    unsigned int i, acc;

    if(!in || !memcmp(in->filetype, FSZ_FILETYPE_INTERNAL, 4)) return -ENOENT;

    acc = in->owner.access;
    st->st_size = in->size;
    st->st_mtime = in->modifydate/1000000;
    i = strlen(path);
    if((in->flags & FSZ_IN_FLAG_HIST) && i > 3 && path[i-3]==';' && path[i-2]=='-' && path[i-1]>='1' && path[i-1]<='5') {
        switch(path[i-1]) {
            case '1':
                acc = in->version1.owner.access;
                st->st_size = in->version1.size;
                st->st_mtime = in->version1.modifydate/1000000;
                break;
            case '2':
                acc = in->version2.owner.access;
                st->st_size = in->version2.size;
                st->st_mtime = in->version2.modifydate/1000000;
                break;
            case '3':
                acc = in->version3.owner.access;
                st->st_size = in->version3.size;
                st->st_mtime = in->version3.modifydate/1000000;
                break;
            case '4':
                acc = in->version4.owner.access;
                st->st_size = in->version4.size;
                st->st_mtime = in->version4.modifydate/1000000;
                break;
            case '5':
                acc = in->version5.owner.access;
                st->st_size = in->version5.size;
                st->st_mtime = in->version5.modifydate/1000000;
                break;
        }
    }
    st->st_uid = getuid(); /* fs2user2unixuser(in->owner); */
    st->st_gid = getgid();
    st->st_mode =
        (acc & FSZ_READ ?   S_IRUSR : 0) |
        (acc & FSZ_WRITE ?  S_IWUSR : 0) |
        (acc & FSZ_EXEC ?   S_IXUSR : 0) |
        (acc & FSZ_APPEND ? S_IWUSR : 0) |
        (acc & FSZ_SUID ?   S_ISUID : 0);
    if(!memcmp(in->filetype, FSZ_FILETYPE_DIR, 4)) st->st_mode |= S_IFDIR; else
    if(!memcmp(in->filetype, FSZ_FILETYPE_SYMLINK, 4)) st->st_mode |= S_IFLNK; else
    /* Linux nem ismeri az uniót, úgyhogy jobb híjján symlinknek értelmezzük... */
    if(!memcmp(in->filetype, FSZ_FILETYPE_UNION, 4)) st->st_mode |= S_IFLNK; else
        st->st_mode |= S_IFREG;
    st->st_nlink = in->numlinks;
    st->st_atime = in->accessdate/1000000;
    st->st_ino = lastino;
    return 0;
}

int fsZ_readlink(const char *path, char *link, size_t size)
{
    FSZ_Inode *in = locate(0, path);
    get_fsZ_context;
    unsigned int i;

    if(!in || (memcmp(in->filetype, FSZ_FILETYPE_SYMLINK, 4) && memcmp(in->filetype, FSZ_FILETYPE_UNION, 4))) return -ENOENT;

    if(size > in->size) size = in->size;
    memcpy(link, (char*)(fsZ_data->sb.flags & FSZ_SB_FLAG_BIGINODE? in->data.big.inlinedata : in->data.small.inlinedata), size);
    link[size] = 0;
    if(!memcmp(in->filetype, FSZ_FILETYPE_UNION, 4) && size>2) { for(i=0; i<size-2; i++) if(!link[i]) link[i]=','; }
    return 0;
}

int fsZ_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    unsigned int cnt = 0, i;
    char fn[128];
    FSZ_DirEntHeader *hdr;
    FSZ_DirEnt *ent;
    FSZ_Inode *in = locate(0, path), *in2;
    get_fsZ_context;

    if(!in || memcmp(in->filetype, FSZ_FILETYPE_DIR, 4)) return -ENOTDIR;

    /* ez csak inode-ba ágyazott könyvtárlistát támogat egyelőre */
    hdr=(FSZ_DirEntHeader *)(fsZ_data->sb.flags & FSZ_SB_FLAG_BIGINODE?
        fsZ_data->in.data.big.inlinedata : fsZ_data->in.data.small.inlinedata);
    if(memcmp(hdr->magic, FSZ_DIR_MAGIC, 4)) return -ENOTDIR;
    ent=(FSZ_DirEnt *)hdr; ent++;

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    while(ent->fid!=0 && cnt<hdr->numentries) {
        strcpy(fn, (char*)(ent->name));
        i = strlen(fn);
        if(fn[i-1] == '/') fn[--i]=0;
        lseek(fsZ_data->devfile, ent->fid*fsZ_data->secsize, SEEK_SET);
        if(read(fsZ_data->devfile, fsZ_data->data, fsZ_data->secsize) != -1 && !memcmp(fsZ_data->data, FSZ_IN_MAGIC, 4)) {
            filler(buffer, fn, NULL, 0);
            in2 = (FSZ_Inode *)fsZ_data->data;
            if((in2->flags & FSZ_IN_FLAG_HIST) && memcmp(in2->filetype, FSZ_FILETYPE_DIR, 4) &&
                memcmp(in2->filetype, FSZ_FILETYPE_UNION, 4) && memcmp(in2->filetype, FSZ_FILETYPE_SYMLINK, 4)) {
                if(in2->version1.sec) { memcpy(fn+i, ";-1", 4); filler(buffer, fn, NULL, 0); }
                if(in2->version2.sec) { memcpy(fn+i, ";-2", 4); filler(buffer, fn, NULL, 0); }
                if(in2->version3.sec) { memcpy(fn+i, ";-3", 4); filler(buffer, fn, NULL, 0); }
                if(in2->version4.sec) { memcpy(fn+i, ";-4", 4); filler(buffer, fn, NULL, 0); }
                if(in2->version5.sec) { memcpy(fn+i, ";-5", 4); filler(buffer, fn, NULL, 0); }
            }
        }
        ent++; cnt++;
    }

    return 0;
}

int fsZ_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    FSZ_Inode *in = locate(0, path);
    FSZ_SDEntry *sd;
    unsigned int i, k, v, o=0, flg;
    unsigned char *inlinedata;
    unsigned long int sec;
    size_t sz, s, r;
    get_fsZ_context;

    if(!in || !memcmp(in->filetype, FSZ_FILETYPE_SYMLINK, 4) || !memcmp(in->filetype, FSZ_FILETYPE_DIR, 4) ||
        !memcmp(in->filetype, FSZ_FILETYPE_UNION, 4)) return -ENOENT;

    if(in->enchash) {
        fprintf(stderr, "fsZ-fuse: %s: %d\n", hu?"fájl titkosítás nincs támogatva":"file encryption not supported", path);
        return -ENOSYS;
    }

    sz = in->size;
    sec = in->sec;
    inlinedata = (char*)(fsZ_data->sb.flags & FSZ_SB_FLAG_BIGINODE? in->data.big.inlinedata : in->data.small.inlinedata);
    flg = FSZ_FLAG_TRANSLATION(in->flags);
    i = strlen(path);
    if((in->flags & FSZ_IN_FLAG_HIST) && i > 3 && path[i-3]==';' && path[i-2]=='-' && path[i-1]>='1' && path[i-1]<='5') {
        switch(path[i-1]) {
            case '1': sec = in->version1.sec; sz = in->version1.size; flg = FSZ_FLAG_TRANSLATION(in->version1.flags); break;
            case '2': sec = in->version2.sec; sz = in->version2.size; flg = FSZ_FLAG_TRANSLATION(in->version2.flags); break;
            case '3': sec = in->version3.sec; sz = in->version3.size; flg = FSZ_FLAG_TRANSLATION(in->version3.flags); break;
            case '4': sec = in->version4.sec; sz = in->version4.size; flg = FSZ_FLAG_TRANSLATION(in->version4.flags); break;
            case '5': sec = in->version5.sec; sz = in->version5.size; flg = FSZ_FLAG_TRANSLATION(in->version5.flags); break;
        }
        if(flg == FSZ_IN_FLAG_INLINE || flg == FSZ_IN_FLAG_SDINLINE || flg == FSZ_IN_FLAG_SECLIST) {
            fprintf(stderr, "fsZ-fuse: %s: %d\n", hu?"régi verzió nem lehet inline":"old version cannot be inlined", path);
            return -ENOSYS;
        }
    }

    if(offset >= sz) return 0;
    if(offset + size > sz) size = sz - offset;

    /* a többi indirekt címzési típust is implementálni kell */
    switch(flg) {
        case FSZ_IN_FLAG_INLINE:            /* FSZ_inode.inline (data), 3k */
            memcpy(buf, inlinedata + offset, size);
            break;

        case FSZ_IN_FLAG_SDINLINE:          /* FSZ_inode.inline (sd) -> data, 768k */
            sd = (FSZ_SDEntry*)(inlinedata);
readsd:     r = size;
            k = offset / fsZ_data->secsize;
            v = (offset + size -1) / fsZ_data->secsize;
            offset %= fsZ_data->secsize;
            for(i = k; i <= v && r>0; i++) {
                s = r > fsZ_data->secsize? fsZ_data->secsize : r;
                if(!sd[i].sec) {
                    memset(fsZ_data->data, 0, fsZ_data->secsize);
                } else {
                    lseek(fsZ_data->devfile, sd[i].sec*fsZ_data->secsize, SEEK_SET);
                    if(read(fsZ_data->devfile, fsZ_data->data, fsZ_data->secsize) == -1)
                        return -errno;
                }
                if(i == k) {
                    if(s > fsZ_data->secsize - offset) s = fsZ_data->secsize - offset;
                    memcpy(buf, fsZ_data->data + offset, s);
                } else {
                    memcpy(buf + o, fsZ_data->data, s);
                }
                o += s;
                r -= s;
            }
            break;

        case FSZ_IN_FLAG_DIRECT:        /* sec -> data, 4k */
            if(!sec) {
                memset(fsZ_data->data, 0, fsZ_data->secsize);
            } else {
                lseek(fsZ_data->devfile, sec*fsZ_data->secsize, SEEK_SET);
                if(read(fsZ_data->devfile, fsZ_data->data, fsZ_data->secsize) == -1)
                    return -errno;
            }
            memcpy(buf, fsZ_data->data + offset, size);
            break;

        case FSZ_IN_FLAG_SD:            /* sec -> sd -> data, 1M */
            lseek(fsZ_data->devfile, sec*fsZ_data->secsize, SEEK_SET);
            if(read(fsZ_data->devfile, fsZ_data->sd, fsZ_data->secsize) == -1)
                return -errno;
            sd = fsZ_data->sd;
            goto readsd;

        case FSZ_IN_FLAG_SD2:           /* sec -> sd -> sd -> data, 256M */
        case FSZ_IN_FLAG_SD3:           /* sec -> sd -> sd -> sd -> data, 64G */
        case FSZ_IN_FLAG_SD4:           /* sec -> sd -> sd -> sd -> sd -> data, 16T */
        case FSZ_IN_FLAG_SECLIST:       /* FSZ_inode.inline (sl) -> data */
        case FSZ_IN_FLAG_SECLIST0:      /* sec -> sl -> data */
        case FSZ_IN_FLAG_SECLIST1:      /* sec -> sd -> sl -> data */
        case FSZ_IN_FLAG_SECLIST2:      /* sec -> sd -> sd -> sd -> sl -> data */
        default:
            fprintf(stderr, "fsZ-fuse: %s: %x %s\n", hu?"nem támogatott címfordítás":"unsupported translation", flg, path);
            return -ENOSYS;
    }

    return size;
}

void *fsZ_init(struct fuse_conn_info *conn)
{
    get_fsZ_context;

    fsZ_data->sb.currmounts++;
    fsZ_data->sb.lastmountdate = (long int)time(NULL) * 1000000;
    return fsZ_data;
}

void fsZ_destroy(void *private_data)
{
    fsZ_data_t *fsZ_data = (fsZ_data_t*)private_data;

    fsZ_data->sb.checksum = crc32c_calc((char *)(&fsZ_data->sb.magic), 508);
    lseek(fsZ_data->devfile, 0, SEEK_SET);
    write(fsZ_data->devfile, &fsZ_data->sb, sizeof(FSZ_SuperBlock));
    close(fsZ_data->devfile);
    fsZ_data->devfile = -1;
    free(fsZ_data->sd);
    free(fsZ_data->data);
}

/*** implementált funkciók listája ***/
static struct fuse_operations operations = {
    .getattr =  fsZ_getattr,
    .readlink = fsZ_readlink,
    .readdir =  fsZ_readdir,
    .read =     fsZ_read,
    .init =     fsZ_init,
    .destroy =  fsZ_destroy
};

/*** fő FUSE program ***/
int main(int argc, char **argv)
{
    int i;
    char *fn, *lang=getenv("LANG");
    fsZ_data_t *fsZ_data = malloc(sizeof(fsZ_data_t));

    if(lang && lang[0]=='h' && lang[1]=='u') hu=1;

    if(!fsZ_data) {
        perror("fsZ-fuse: malloc");
        exit(EX_FAILURE);
    }
    memset(fsZ_data, 0, sizeof(fsZ_data_t));
    fsZ_data->devfile = -1;

    for(i=1; i<argc && argv[i][0]=='-'; i++);
    if(i<argc) {
        fn = realpath(argv[i], NULL);
        if(fn) {
            fsZ_data->devfile = open(fn, O_RDWR|O_SYNC);
            free(fn);
        }
        if(fsZ_data->devfile == -1) {
            fprintf(stderr, "fsZ-fuse: %s: %s\n", hu?"nem tudom megnyitni az eszközt":"unable to open device file", argv[i]);
            exit(EX_FAILURE);
        }
        lseek(fsZ_data->devfile, 0, SEEK_SET);
        if(read(fsZ_data->devfile, &fsZ_data->sb, sizeof(FSZ_SuperBlock))==-1) {
            perror(hu?"fsZ-fuse: nem tudom beolvasni a szuperblokkot":"fsZ-fuse: unable to read superblock");
            exit(EX_FAILURE);
        }
        if(memcmp(fsZ_data->sb.magic, FSZ_MAGIC, 4) || memcmp(fsZ_data->sb.magic2, FSZ_MAGIC, 4)) {
            fprintf(stderr, "fsZ-fuse: %s: %s\n", hu?"érvénytelen fájlrendszer":"not a valid file system image", argv[i]);
            exit(EX_FAILURE);
        }
        if(fsZ_data->sb.raidtype != FSZ_SB_SOFTRAID_NONE) {
            fprintf(stderr, hu?"fsZ-fuse: szoft raid%d nem támogatott: %s\n":"fsZ-fuse: soft raid%d not supported: %s\n",
                fsZ_data->sb.raidtype, argv[i]);
            exit(EX_FAILURE);
        }
        if(fsZ_data->sb.enchash) {
            fprintf(stderr, "fsZ-fuse: %s: %s\n",
                hu?"titkosított fájlrendszer nem támogatott":"encrypted file system not supported", argv[i]);
            exit(EX_FAILURE);
        }
        fsZ_data->secsize = 1U<<(fsZ_data->sb.logsec+11);
        fsZ_data->sd = (FSZ_SDEntry*)malloc(fsZ_data->secsize);
        if(!fsZ_data->sd) {
            perror("fsZ-fuse: malloc");
            exit(EX_FAILURE);
        }
        fsZ_data->data = malloc(fsZ_data->secsize);
        if(!fsZ_data->data) {
            perror("fsZ-fuse: malloc");
            exit(EX_FAILURE);
        }
        unixuser2fsZuser(fsZ_data->fsZuser);
        memcpy(&argv[i], &argv[i+1], (argc-i)*sizeof(argv[0]));
        argc--;
    } else {
        fprintf(stderr, "FUSE fsZ %d.%d.0\n%s: fsZ-fuse <%s]\n\n"
            "fuse: %s\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION, hu?"használat":"usage",
            hu?"eszköz|képfájl> <felcsatlásipont> [opciók":"device|imagefile> <mountpoint> [options",
            hu?"nincs eszköz/képfájl megadva":"missing device/imagefile parameter");
    }
    return fuse_main(argc, argv, &operations, fsZ_data);
}
