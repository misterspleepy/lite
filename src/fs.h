#define INUM 30
#define SUPER_BLOCK_NUM 1
#define MAGIC 0x10203040
#define ROOTINO  1  // root i-number
#define ROOTDEV 0
#define BLOCK_SIZE 1024
typedef struct {
    uint32 magic;
    uint32 size;
    uint32 nblocks;
    uint32 ninodes;
    uint32 nlogs;
    uint32 logstart;
    uint32 inodestart;
    uint32 bmapstart;
} superblock_t;
// #define T_DIR     1   // Directory
// #define T_FILE    2   // File
// #define T_DEVICE  3   // Device

#define T_FREE 0
#define T_DIR 1
#define T_FILE  2
#define T_DEVICE  3
#define NDIRBLOCKS 12
#define NINDIRECT (BLOCK_SIZE / sizeof(uint32))

typedef struct {
    short type;
    short major;
    short minor;
    short nlink;
    uint32 size;
    uint32 addr[NDIRBLOCKS + 1];
} dinode_t;

#define IPB (BLOCK_SIZE / sizeof(dinode_t))
#define IBLOCK(num, sb) ((sb)->inodestart + (num) / IPB)

#define BPB (BLOCK_SIZE)
#define BBLOCK(num, sb) ((sb)->bmapstart + (num) / BPB)

#define DIRSIZ 14
typedef struct {
    uint16 inum;
    char name[DIRSIZ];
} dirent_t;


void fsinit();
void readsb(int32 dev);

struct inode;
typedef struct inode inode_t;

inode_t* namei(const char* path);
inode_t* nameiparent(const char* path);
inode_t* nameix(char* path, short parent);

inode_t* dirlookup(inode_t* dir, const char* name);
int dirlink(inode_t* dir, const char* name, uint16 ino);
void dirunlink(inode_t* dir, const char* name);

// inode
inode_t* iget(uint32 dev, uint32 ino);
// inode_t* ialloc(uint32 dev, short type);
void iput(inode_t*);
inode_t* idup(inode_t*);
uint32 iread(inode_t* i, uint32 user, char* buf, uint32 size, uint32 offset);
uint32 iwrite(inode_t* i, uint32 user, char* buf, uint32 size, uint32 offset);
void iupdate(inode_t*);
void ilock(inode_t* i);
void iunlock(inode_t* i);
void itruncate(inode_t* ip);
uint32 bmap(inode_t* i, uint32 bn);
// uint32 balloc(uint32 dev);
void bfree(uint32 dev, uint32 bn);
// void bzero(uint32 dev, uint32 bn);
char* skipelem(char* path, char** name);