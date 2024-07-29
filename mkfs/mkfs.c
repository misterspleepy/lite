#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "src/type.h"
#include "src/fs.h"
#include "src/stat.h"
#include "src/conf.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int nbitmap = FSSIZE/(BLOCK_SIZE*8) + 1;
int ninodeblocks = NINODES / IPB + 1;
int nlog = LOGSIZE;
int nmeta;    // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;  // Number of data blocks

int fsfd;
superblock_t sb;
char zeroes[BLOCK_SIZE];
uint freeinode = 1;
uint freeblock;


void balloc(int);
void wsect(uint, void*);
void winode(uint, dinode_t*);
void rinode(uint inum, dinode_t *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);
void die(const char *);

// convert to riscv byte order
ushort
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

int
main(int argc, char *argv[])
{
  int i, cc, fd;
  uint rootino, inum, off;
  dirent_t de;
  char buf[BLOCK_SIZE];
  dinode_t din;


  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

  if(argc < 2){
    fprintf(stderr, "Usage: mkfs fs.img files...\n");
    exit(1);
  }

  assert((BLOCK_SIZE % sizeof(dinode_t)) == 0);
  assert((BLOCK_SIZE % sizeof(dirent_t)) == 0);

  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0)
    die(argv[1]);

  // 1 fs block = 1 disk sector
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  nblocks = FSSIZE - nmeta;

  sb.magic = MAGIC;
  sb.size = xint(FSSIZE);
  sb.nblocks = xint(nblocks);
  sb.ninodes = xint(NINODES);
  sb.nlogs= xint(nlog);
  sb.logstart = xint(2);
  sb.inodestart = xint(2+nlog);
  sb.bmapstart = xint(2+nlog+ninodeblocks);

  printf("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
         nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

  freeblock = nmeta;     // the first free block that we can allocate

  for(i = 0; i < FSSIZE; i++)
    wsect(i, zeroes);

  memset(buf, 0, sizeof(buf));
  memmove(buf, &sb, sizeof(sb));
  wsect(1, buf);

  rootino = ialloc(T_DIR);
  assert(rootino == ROOTINO);

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, ".");
  iappend(rootino, &de, sizeof(de));

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, "..");
  iappend(rootino, &de, sizeof(de));

  for(i = 2; i < argc; i++){
    // get rid of "user/"
    char *shortname;
    if(strncmp(argv[i], "user/", 5) == 0)
      shortname = argv[i] + 5;
    else
      shortname = argv[i];
    
    assert(index(shortname, '/') == 0);

    if((fd = open(argv[i], 0)) < 0)
      die(argv[i]);

    // Skip leading _ in name when writing to file system.
    // The binaries are named _rm, _cat, etc. to keep the
    // build operating system from trying to execute them
    // in place of system binaries like rm and cat.
    if(shortname[0] == '_')
      shortname += 1;

    inum = ialloc(T_FILE);

    bzero(&de, sizeof(de));
    de.inum = xshort(inum);
    strncpy(de.name, shortname, DIRSIZ);
    iappend(rootino, &de, sizeof(de));

    while((cc = read(fd, buf, sizeof(buf))) > 0)
      iappend(inum, buf, cc);

    close(fd);
  }

  // fix size of root inode dir
  rinode(rootino, &din);
  off = xint(din.size);
  off = ((off/BLOCK_SIZE) + 1) * BLOCK_SIZE;
  din.size = xint(off);
  winode(rootino, &din);

  balloc(freeblock);

  exit(0);
}

void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BLOCK_SIZE, 0) != sec * BLOCK_SIZE)
    die("lseek");
  if(write(fsfd, buf, BLOCK_SIZE) != BLOCK_SIZE)
    die("write");
}

void
winode(uint inum, dinode_t *ip)
{
  char buf[BLOCK_SIZE];
  uint bn;
  dinode_t *dip;

  bn = IBLOCK(inum, &sb);
  rsect(bn, buf);
  dip = ((dinode_t*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

void
rinode(uint inum, dinode_t *ip)
{
  char buf[BLOCK_SIZE];
  uint bn;
  dinode_t *dip;

  bn = IBLOCK(inum, &sb);
  rsect(bn, buf);
  dip = ((dinode_t*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BLOCK_SIZE, 0) != sec * BLOCK_SIZE)
    die("lseek");
  if(read(fsfd, buf, BLOCK_SIZE) != BLOCK_SIZE)
    die("read");
}

uint
ialloc(ushort type)
{
  uint inum = freeinode++;
  dinode_t din;

  bzero(&din, sizeof(din));
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  winode(inum, &din);
  return inum;
}

void
balloc(int used)
{
  uchar buf[BLOCK_SIZE];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < BLOCK_SIZE*8);
  bzero(buf, BLOCK_SIZE);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
  wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  dinode_t din;
  char buf[BLOCK_SIZE];
  uint iNDIRBLOCKS[NINDIRECT];
  uint x;

  rinode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0){
    fbn = off / BLOCK_SIZE;
    assert(fbn < MAXFILE);
    if(fbn < NDIRBLOCKS){
      if(xint(din.addr[fbn]) == 0){
        din.addr[fbn] = xint(freeblock++);
      }
      x = xint(din.addr[fbn]);
    } else {
      if(xint(din.addr[NDIRBLOCKS]) == 0){
        din.addr[NDIRBLOCKS] = xint(freeblock++);
      }
      rsect(xint(din.addr[NDIRBLOCKS]), (char*)iNDIRBLOCKS);
      if(iNDIRBLOCKS[fbn - NDIRBLOCKS] == 0){
        iNDIRBLOCKS[fbn - NDIRBLOCKS] = xint(freeblock++);
        wsect(xint(din.addr[NDIRBLOCKS]), (char*)iNDIRBLOCKS);
      }
      x = xint(iNDIRBLOCKS[fbn-NDIRBLOCKS]);
    }
    n1 = min(n, (fbn + 1) * BLOCK_SIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BLOCK_SIZE), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}

void
die(const char *s)
{
  perror(s);
  exit(1);
}
