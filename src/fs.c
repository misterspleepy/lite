#include "sys.h"
static struct {
    spinlock_t lock;
    inode_t inodes[INUM];
} inode_table;

static superblock_t sb;
uint32 balloc(uint32 dev);
void readsb(int32 dev)
{
    buf_t* buf = bread(dev, SUPER_BLOCK_NUM);
    memmove(&sb, buf->data, sizeof(sb));
    if (sb.magic != 0x10203040) {
        panic("readsb");
    }
    brelease(buf);
}

void iinit()
{
    lock_init(&inode_table.lock, "inode_table");
    for (int i = 0; i < INUM; i++) {
        inode_t* in = &inode_table.inodes[i];
        in->ino = 0;
        in->refcount = 0;
        sleeplock_init(&in->lock, "inode lock");
        in->valid = 0;
    }

}

void fsinit()
{
    readsb(0);
    iinit();
}

// don't lock, don't read disk, must called in process context
inode_t* iget(uint32 dev, uint32 ino)
{
    if (myproc() == 0) {
        panic("iget");
    }
    lock_acquire(&inode_table.lock);
    while (1) {
        for (int i = 0; i < INUM; i++) {
            inode_t* inode = &inode_table.inodes[i];
            if (inode->refcount == 0) {
                inode->refcount = 1;
                inode->ino = ino;
                inode->dev = dev;
                inode->valid = 0;
                lock_release(&inode_table.lock);
                return inode;
            }
        }
        sleep(&inode_table.inodes[0], &inode_table.lock);
    }
    return 0;
}

inode_t* ialloc(uint32 dev, short type)
{
    buf_t* b;
    dinode_t* dip;
    for (uint32 i = 0; i < sb.ninodes; i++) {
        b = bread(dev, IBLOCK(i, &sb));
        dip = (dinode_t*)((void*)b + (i % IPB) * sizeof(dinode_t));
        if (dip->type == T_FREE) {
            dip->type = type;
            dip->nlink = 0;
            dip->size = 0;
            memset(dip->addr, 0, sizeof(dip->addr));
            bwrite(b);
            brelease(b);
            return iget(dev, i);
        }
        brelease(b);
    }
    panic("ialloc");
    return 0;
}

void iput(inode_t* inode)
{
    if (sleeplock_holding(&inode->lock)) {
        panic("iput");
    }
    if (inode->refcount < 1) {
        panic("iput1");
    }
    lock_acquire(&inode_table.lock);
    if (inode->refcount == 1 && inode->valid && inode->dinode.nlink == 0) {
        // this will not block, since refcount == 1
        sleeplock_acquire(&inode->lock);
        lock_release(&inode_table.lock);
        itruncate(inode);
        inode->dinode.type = T_FREE;
        iupdate(inode);
        inode->valid = 0;
        sleeplock_release(&inode->lock);
        lock_acquire(&inode_table.lock);
    }
    inode->refcount -= 1;
    lock_release(&inode_table.lock);
}

inode_t* idup(inode_t* ip)
{
    lock_acquire(&inode_table.lock);
    ip->refcount += 1;
    lock_release(&inode_table.lock);
    return ip;
}

// holding ip->lock is needed
void iupdate(inode_t* ip)
{
    buf_t* b = bread(ip->dev, IBLOCK(ip->ino, &sb));
    dinode_t* dip = (dinode_t*)((void*)b->data + (ip->ino % IPB) * sizeof(dinode_t));
    memmove(dip, &ip->dinode, sizeof(ip->dinode));
    bwrite(b);
    brelease(b);
}

#define ROUNDDOWN(a, size) ((a) & (~((size) - 1)))
#define ROUNDUP(a, size) (((a) + (size - 1)) & (~(size - 1)))
// holding ip->lock is needed
void itruncate(inode_t* ip)
{
    uint32 bn = 0;
    uint32* b;
    buf_t* bp;
    for (; bn < NDIRBLOCKS; bn++) {
        if (ip->dinode.addr[bn] == 0) {
            continue;
        }
        bfree(ip->dev, ip->dinode.addr[bn]);
        ip->dinode.addr[bn] = 0;
    }
    if (ip->dinode.addr[bn] != 0) {
        bp = bread(ip->dev, ip->dinode.addr[bn]);
        b = (uint32*)(bp->data);
        bn -= NDIRBLOCKS;
        for (; bn < BLOCK_SIZE / sizeof(uint32); bn++) {
            if (b[bn] == 0) {
                continue;
            }
            bfree(ip->dev, b[bn]);
            b[bn] = 0;
        }
        brelease(bp);
        bfree(ip->dev, ip->dinode.addr[NDIRBLOCKS]);
        ip->dinode.addr[NDIRBLOCKS] = 0;
    }
    ip->dinode.size = 0;
    iupdate(ip);
}

void ilock(inode_t* ip)
{
    buf_t* b;
    dinode_t* dip;
    sleeplock_acquire(&ip->lock);
    if (ip->valid == 0) {
        b = bread(ip->dev, IBLOCK(ip->ino, &sb));
        dip = (dinode_t*)b->data;
        ip->dinode = dip[ip->ino % IPB];
        ip->valid = 1;
        brelease(b);
    }
}

void iunlock(inode_t* ip)
{
    sleeplock_release(&ip->lock);
}

// create if not exist
uint32 bmap(inode_t* ip, uint32 bn)
{
    buf_t* bp;
    uint32* ibp = 0;
    uint32* pos;
    uint32 target = 0;
    if (bn < NDIRBLOCKS) {
        pos = &ip->dinode.addr[bn];
        if (*pos == 0) {
            *pos = balloc(ip->dev);
        }
        target = *pos;
    } else {
        if (ip->dinode.addr[NDIRBLOCKS] == 0) {
            ip->dinode.addr[NDIRBLOCKS] = balloc(ip->dev);
        }
        bp = bread(ip->dev, ip->dinode.addr[NDIRBLOCKS]);
        ibp = (uint32*)(bp->data);
        pos = &ibp[bn - NDIRBLOCKS];
        if (*pos == 0) {
            *pos = balloc(ip->dev);
            bwrite((buf_t*)ibp);
        }
        target = *pos;
        brelease(bp);
    }
    return target;
}


uint32 balloc(uint32 dev)
{
    buf_t* bp;
    char* data;
    uint32 index = 0;
    for (uint32 i = 0; i < sb.nblocks; i += sizeof(char)) {
        bp = bread(dev, BBLOCK(i, &sb));
        data = (char*)bp->data;
        index = i % BPB / sizeof(char);
        if (data[index] == 0xff) {
            brelease(bp);
            continue;
        }
        for (int j = 0; j < sizeof(char); j++) {
            if ((data[index] & (0x1 << j)) == 0) {
                data[index] |= (0x1 << j);
                bwrite(bp);
                brelease(bp);
                return i + j;
            }
        }
    }
    return 0;
}

void bfree(uint32 dev, uint32 bn)
{
    buf_t* bp = bread(dev, BBLOCK(bn, &sb));
    char* data = (char*)bp->data;
    data[bn % BPB / sizeof(char)] &= ~(0x1 << bn % BPB % sizeof(char));
    bwrite(bp);
    brelease(bp);
}

void bzero(uint32 dev, uint32 bn)
{
    buf_t* bp = bread(dev, BBLOCK(bn, &sb));
    memset(bp->data, 0, sizeof(bp->data));
    bwrite(bp);
    brelease(bp);
}


// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
char* skipelem(char* path, char** name)
{
    while(*path == '/') {
        path++;
    }
    *name = path;
    while (*path != 0 && *path != '/') {
        path++;
    }
    if (*path == '/') {
        *(path++) = 0;
    }
    return path;
}

uint64 namecmp(const char* n1, const char* n2)
{
    uint64 i = 0;
    while (n1[i] != 0) {
        if (n1[i] != n2[i]) {
            return 0;
        }
        i++;
    }
    return n2[i] == 0;
}

inode_t* namei(const char* path)
{
    return nameix(path, 0);
}

inode_t* nameiparent(const char* path)
{
    return nameix(path, 1);
}

// Return 0 if not found
inode_t* nameix(char* path, short parent)
{
    inode_t* ip;
    inode_t* iparent;
    char* elem;
    if (path == 0) {
        return 0;
    }
    if (*path == '/') {
        iparent = iget(ROOTDEV, ROOTINO);
    } else {
        iparent = idup(myproc()->pwd);
    }
    if (*path == '\0') {
        return iparent;
    }
    for (;;) {
        ilock(iparent);
        if (iparent->dinode.type != T_DIR) {
            iunlock(iparent);
            iput(iparent);
            return 0;
        }
        path = skipelem(path, &elem);
        if (parent &&  *path == '\0') {
            iunlock(iparent);
            return iparent;
        }
        ip = dirlookup(iparent, elem);
        if (*path == '\0' || ip == 0) {
            iunlock(iparent);
            iput(iparent);
            return ip;
        }
        iunlock(iparent);
        iput(iparent);
        iparent = ip;
    }
    return 0;
}

// inode lock should be holded
inode_t* dirlookup(inode_t* dir, const char* name)
{
    char data[BLOCK_SIZE];
    dirent_t* dp;
    if (iread(dir, 0, data, BLOCK_SIZE, 0) != BLOCK_SIZE) {
        panic("iread\n");
    }
    dp = (dirent_t*)data;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dirent_t); i++) {
        if (namecmp(dp[i].name, name) == 1) {
            return iget(dir->dev, dp[i].inum);
        }
    }
    return 0;
}


// inode lock should be holded
int dirlink(inode_t* dir, const char* name, uint16 ino)
{
    char data[BLOCK_SIZE];
    dirent_t* dp;
    int ret = 0;
    inode_t* ip;
    if ((ip = dirlookup(dir, name)) != 0) {
        iput(ip);
        return -1;
    }
    if (iread(dir, 0, data, BLOCK_SIZE, 0) != BLOCK_SIZE) {
        return -1;
    }
    dp = (dirent_t*)data;
    int i = 0;
    for (; i < BLOCK_SIZE / sizeof(dirent_t); i++) {
        if (dp[i].inum == 0) {
           break;
        }
    }
    if (i == (BLOCK_SIZE / sizeof(dirent_t))) {
        return -1;
    }
    dp[i].inum = ino;
    strcpy(dp[i].name, name);
    ret = iwrite(dir, 0, (char *)(&dp[i]), sizeof(dirent_t), i * sizeof(dirent_t));
    if (ret != sizeof(dirent_t)) {
        return -1;
    }
    return 0;
}

#define min(a, b) ((a) < (b) ? (a) : (b))

// should hold the ip->lk
uint32 iread(inode_t* ip, uint32 user, char* buf, uint32 size, uint32 offset)
{
    buf_t* bp;
    uint32 sz = 0;
    uint32 bindex = 0;
    uint64 bbegin = 0;
    uint32 bsize = 0;
    uint32 end = ((offset + size) < ip->dinode.size) ? offset + size : ip->dinode.size;
    while (offset < end) {
        bindex = offset / BLOCK_SIZE;
        bbegin = offset % BLOCK_SIZE;
        bsize = min(BLOCK_SIZE - bbegin, end - offset);
        bp = bread(ip->dev, bmap(ip, bindex));
        if (user == 1) {
            copyout(myproc()->pagetable, (uint64)buf + sz, bbegin, bsize);
        } else {
            memmove(buf + sz, (const char*)((uint64)bp->data + bbegin), bsize);
        }
        brelease(bp);
        sz += bsize;
        offset += bsize;
    }
    return sz;
}
#define MAX_FILE_SIZE ((NDIRBLOCKS + BLOCK_SIZE / sizeof(uint32)) * BLOCK_SIZE)
uint32 iwrite(inode_t* ip, uint32 user_src, char* buf, uint32 size, uint32 offset)
{
    uint32 sz = 0;
    uint32 block_num, block_begin, copy_size;
    uint32 bn;
    buf_t* bp;
    if (offset > ip->dinode.size) {
        return -1;
    }
    if (offset + size > MAX_FILE_SIZE) {
        return -1;
    }
    while (size > 0) {
        block_num = offset / BLOCK_SIZE;
        block_begin = offset % BLOCK_SIZE;
        copy_size = min(size, BLOCK_SIZE - block_begin);
        bn = bmap(ip, block_num);
        buf_t* bp = bread(ip->dev, bn);
        if (eithercopyin(user_src, (uint64)(bp->data) + block_begin,
            (uint64)buf, copy_size) != 0) {
            break;
        }
        bwrite(bp);
        brelease(bp);
        size -= copy_size;
        offset += copy_size;
        buf += copy_size;
        sz += copy_size;
    }
    if (offset > ip->dinode.size) {
        ip->dinode.size = offset;
    }
    iupdate(ip);
    return sz;
}